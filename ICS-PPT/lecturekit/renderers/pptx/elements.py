"""Boxes and alignment for every PPTX element, independent of its drawer.

Draw leaves in their allotted width, measure the resulting shapes, then move
each group within its box. Rows share width; columns use the ordinary cursor.
"""

from __future__ import annotations

from dataclasses import replace

from pptx.enum.shapes import MSO_SHAPE_TYPE
from pptx.enum.text import PP_ALIGN

from lecturekit import model, tokens
from .blocks import Ctx, draw_block
from .layout import Cursor, estimate_text_width, px
from . import theme


def place_shapes(shapes, left, top, width, height, *, halign=None, valign=None):
    """Move content as one group in a box; overflowing content stays at its start."""
    if not shapes:
        return
    x0 = min(s.left for s in shapes)
    x1 = max(s.left + s.width for s in shapes)
    y0 = min(s.top for s in shapes)
    y1 = max(s.top + s.height for s in shapes)
    dx = (left + round(max(0, width - (x1 - x0)) * model.ALIGN_FRACTION[halign]) - x0
          if halign is not None else 0)
    dy = (top + round(max(0, height - (y1 - y0)) * model.ALIGN_FRACTION[valign]) - y0
          if valign is not None else 0)
    for shape in shapes:
        shape.left += dx
        shape.top += dy


def _hug_text(shapes):
    """Keep left-aligned lines together while their containing box is moved.

    Width is an estimate, like every PPTX text measurement. A wrapped line
    keeps the full measure; native tables keep their column geometry.
    """
    for shape in shapes:
        if not shape.has_text_frame or not shape.text.strip():
            continue
        # A label in its own textbox can hug its text. A bordered native
        # shape (chip, module, sidenote) already owns its geometry and padding.
        if shape.shape_type != MSO_SHAPE_TYPE.TEXT_BOX:
            continue
        tf = shape.text_frame
        widest = 0
        for paragraph in tf.paragraphs:
            line = 0
            for run in paragraph.runs:
                size = run.font.size.pt if run.font.size is not None else theme.BODY_PT
                line += estimate_text_width(run.text, font_pt=size)
            widest = max(widest, line)
        inset = tf.margin_left + tf.margin_right
        # A small allowance avoids changing wrapping because of an estimated
        # glyph advance. Never widen a drawer's box.
        original_width = shape.width
        shape.width = min(original_width, widest + inset + px(4))
        alignments = {p.alignment for p in tf.paragraphs}
        if alignments == {PP_ALIGN.CENTER}:
            shape.left += (original_width - shape.width) // 2
        elif alignments == {PP_ALIGN.RIGHT}:
            shape.left += original_width - shape.width


def _width(node, available):
    if isinstance(node, model.Block) and node.kind == "image":
        value = node.content.get("width")
        if value and value.endswith("px"):
            return px(float(value[:-2]))
        if value and value.endswith("%"):
            return round(available * float(value[:-1]) / 100)
    return None


def _group_halign(node, halign, width):
    # Containers span their box across a column. A row's children also fill
    # its width when any child is flexible; only fixed boxes leave slack.
    if isinstance(node, model.Flex) and (
        node.direction == "column" or any(_width(child, width) is None for child in node.children)
    ):
        return None
    return halign


def draw_elements(elements, ctx: Ctx, *, inherited=(None, None), held_shapes=None,
                  page=None):
    """Draw a column, returning each top-level element's shapes for page layout."""
    groups = []
    for element in elements:
        before = len(ctx.slide.shapes)
        top = ctx.cursor.top
        halign, valign = model.resolve_align(element, inherited)
        if isinstance(element, model.Block):
            draw_block(element, ctx)
            shapes = list(ctx.slide.shapes)[before:]
            if halign is not None:
                _hug_text(shapes)
            if held_shapes is not None and page is not None and model.block_held(page, element):
                held_shapes.extend(shapes)
        elif element.direction == "column":
            draw_elements(element.children, ctx, inherited=(halign, valign),
                          held_shapes=held_shapes, page=page)
            shapes = list(ctx.slide.shapes)[before:]
        else:
            _draw_row(element, ctx, inherited=(halign, valign),
                      held_shapes=held_shapes, page=page)
            shapes = list(ctx.slide.shapes)[before:]
        # A column child's box is its natural height. Vertical alignment has
        # room to act when a row gives siblings the tallest child's height.
        place_shapes(shapes, ctx.layout.content_left, top,
                     ctx.layout.content_width, ctx.cursor.top - top,
                     halign=_group_halign(element, halign, ctx.layout.content_width))
        groups.append(shapes)
    return groups


def _draw_row(row, ctx, *, inherited, held_shapes=None, page=None):
    layout, cursor = ctx.layout, ctx.cursor
    top = cursor.top
    gutter = px(float(tokens.value("--layout-gutter").removesuffix("px")))
    available = max(1, layout.content_width - gutter * (len(row.children) - 1))
    widths = [_width(child, layout.content_width) for child in row.children]
    fixed = sum(w for w in widths if w is not None)
    unsized = widths.count(None)
    # As in CSS flex-shrink, fixed requests fit the row when they exceed it.
    scale = min(1, available / max(1, fixed))
    widths = [max(1, round(w * scale)) if w is not None
              else max(1, (available - fixed) // max(1, unsized)) for w in widths]
    left = layout.content_left
    groups = []
    used = []
    try:
        for child, width in zip(row.children, widths):
            ctx.layout = replace(layout, content_left=left, content_top=top,
                                 content_width=width)
            ctx.cursor = Cursor(ctx.layout)
            shapes = draw_elements([child], ctx, inherited=inherited,
                                   held_shapes=held_shapes, page=page)[0]
            groups.append((child, shapes, left, width))
            used.append(ctx.cursor.top - top)
            left += width + gutter
        height = max(used, default=0)
        # Alignment uses the visible extent, excluding the drawer's trailing
        # rhythm, so a short text box centres exactly against a tall picture.
        visible_height = max((max((s.top + s.height for s in shapes), default=top) - top
                              for _, shapes, _, _ in groups), default=0)
        for child, shapes, left, width in groups:
            halign, valign = model.resolve_align(child, inherited)
            place_shapes(shapes, left, top, width, visible_height,
                         halign=_group_halign(child, halign, width), valign=valign)
    finally:
        ctx.layout, ctx.cursor = layout, cursor
    ctx.cursor.top = top + height
