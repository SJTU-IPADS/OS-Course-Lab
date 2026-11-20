"""One content slide: the title, the body and the footnote band.

A content page is three bands, top to bottom. The title takes the padding
box's width. The footnote band sits at the bottom of the padding box, as wide
as it, and is as tall as its notes, at least ``--footnote-band-min``. The body
takes everything between the two. With a ``side_image`` the body is two
columns, ``--side-gap`` apart: the image column, on the side the first side
image names and as wide as ``model.side_column_px`` makes it, and the main
column in the rest. The page's valign places the content of each column in
the body, the same for both.

Every box is a :class:`Layout`: the slide with a smaller box on it. The blocks
of a column draw into its box with the ordinary drawers, and the column is
then moved as one group.
"""

from __future__ import annotations

from dataclasses import replace

from pptx.enum.text import MSO_ANCHOR, MSO_AUTO_SIZE
from pptx.util import Emu, Inches, Pt

from lecturekit import model, tokens
from . import theme
from .blocks import GAP, Ctx, apply_runs, draw_title, resolve_image, style_run
from .elements import draw_elements
from .layout import Cursor, Layout, estimate_text_height, px
from .text import parse_markdown

# The footnote band's least height and the gap beside the image column, in px.
FOOTNOTE_BAND_MIN = tokens.px("--footnote-band-min")
SIDE_GAP = tokens.px("--side-gap")

# The band's own type (theme `.footnotes`): 0.5em of the body, line-height 1.3,
# and 4px of chrome above the text (the viewer's padding and rule).
FOOTNOTE_LINE_HEIGHT = 1.3
FOOTNOTE_INSET = 4


def draw_page(page: model.Page, blocks: list[model.Block], ctx: Ctx) -> None:
    """Draw a content page's ``blocks`` (those this frame shows) into its slide."""
    sides = [block for block in blocks if block.kind == "side_image"]
    rest = [block for block in blocks if block.kind != "side_image"]
    under = [block for block in rest if sides and block.in_side]
    main = [block for block in rest if not (sides and block.in_side)]
    valign = model.page_valign(page, blocks)
    # Numbered as the viewer numbers them: the image column first.
    notes = [note for block in (*sides, *under, *main)
             if not model.block_held(page, block) for note in block.footnotes]

    draw_title(page.title, ctx)
    body, band = bands(ctx.layout, ctx.cursor.top,
                       footnote_height(notes, ctx.layout.content_width))
    main_box, column = columns(body, sides)
    held: list[object] = []
    if column is not None:
        align_column(draw_side_column(sides, under, column, ctx, page, held),
                     column, valign)

    ctx.layout, ctx.cursor = main_box, Cursor(main_box)
    elements = model.layout(main)
    groups = draw_elements(elements, ctx, inherited=(page.halign, valign),
                           held_shapes=held, page=page)
    reclaim_overflow(elements, groups, ctx)
    spread_page_blocks(page.gap, groups, ctx)
    align_column([shape for group in groups for shape in group], main_box, valign)

    draw_footnotes(notes, band, ctx)
    for shape in held:
        shape._element.getparent().remove(shape._element)


# --- the boxes ---


def bands(layout: Layout, top: int, band_height: int) -> tuple[Layout, Layout]:
    """The body and the footnote band under a title that ends at ``top``.

    The band is ``band_height`` tall at the bottom of the padding box; the
    body runs from ``top`` to where the band begins.
    """
    band_top = layout.content_bottom - band_height
    body = replace(layout, content_top=top, content_bottom=band_top)
    band = replace(layout, content_top=band_top)
    return body, band


def columns(body: Layout, sides: list) -> tuple[Layout, Layout | None]:
    """The main column and the image column (``None`` without a side image)."""
    if not sides:
        return body, None
    gap = px(SIDE_GAP)
    total = body.content_width
    width = px(model.side_column_px(sides[0].content.get("width"), total / px(1)))
    width = max(1, min(width, total - gap))
    rest = max(1, total - width - gap)
    if sides[0].content.get("side") == "left":
        return (replace(body, content_left=body.content_left + width + gap,
                        content_width=rest),
                replace(body, content_width=width))
    return (replace(body, content_width=rest),
            replace(body, content_left=body.content_left + total - width,
                        content_width=width))


def align_column(shapes: list, box: Layout, valign: str | None) -> None:
    """Move a column's shapes as one group to the page's ``valign`` in ``box``.

    The shapes are drawn from the box's top, so ``top`` (and no valign)
    leaves them there. Content taller than the box starts at its top.
    """
    if valign not in ("middle", "bottom") or not shapes:
        return
    first = min(shape.top for shape in shapes)
    last = max(shape.top + shape.height for shape in shapes)
    slack = max(0, box.content_bottom - box.content_top - (last - first))
    shift = box.content_top + int(slack * model.ALIGN_FRACTION[valign]) - first
    for shape in shapes:
        shape.top += shift


# --- the image column ---


def fitted(block) -> bool:
    """``contain`` or ``fit`` in ``alt``: the whole picture, not a crop of it."""
    return bool({"contain", "fit"} & set(str(block.content.get("alt") or "").split()))


def draw_side_column(sides: list, texts: list, column: Layout, ctx: Ctx,
                     page: model.Page, held: list) -> list:
    """Draw the image column into ``column``; return its shapes.

    The side images stack from the column's top, the slide text set under
    them (``under_side_image``) follows at the column's width. A fitted image
    is as wide as the column and as tall as its aspect ratio makes it; when
    the pictures and the text would not fit, every fitted picture is drawn
    smaller by the same factor. A covering image takes the height the others
    leave and is cropped to it.
    """
    slide = ctx.slide
    left, top, width = column.content_left, column.content_top, column.content_width
    pictures = []
    for block in sides:
        path = resolve_image(block.content["src"], ctx)
        if path is not None:
            pictures.append((block, slide.shapes.add_picture(str(path), left, top)))

    saved = ctx.layout, ctx.cursor
    ctx.layout, ctx.cursor = column, Cursor(column)
    text_shapes = []
    for block in texts:
        mark = len(slide.shapes)
        block.to_pptx(ctx)
        shapes = list(slide.shapes)[mark:]
        text_shapes.extend(shapes)
        if model.block_held(page, block):
            held.extend(shapes)
    ctx.layout, ctx.cursor = saved
    text_height = max((s.top + s.height for s in text_shapes), default=top) - top

    gaps = GAP * (len(pictures) - 1 + bool(text_shapes and pictures))
    room = max(Emu(Pt(theme.BODY_PT)),
               column.content_bottom - top - text_height - gaps)
    natural = {id(pic): round(pic.height * width / pic.width)
               for block, pic in pictures if fitted(block)}
    fixed = sum(natural.values())
    scale = min(1.0, room / fixed) if fixed else 1.0
    covers = sum(1 for block, _ in pictures if not fitted(block))
    share = max(1, round(room - fixed * scale) // covers) if covers else 0

    y = top
    for block, pic in pictures:
        if fitted(block):
            pic.width, pic.height = round(width * scale), round(natural[id(pic)] * scale)
        else:
            _cover(pic, width, share)
        pic.left, pic.top = left + (width - pic.width) // 2, y
        y += pic.height + GAP
    shift = y - top if pictures else 0
    for shape in text_shapes:
        shape.top += shift
    return [pic for _, pic in pictures] + text_shapes


def _cover(pic, width: int, height: int) -> None:
    """Scale ``pic`` to cover ``width`` x ``height`` and crop it to that box."""
    w, h = pic.width, pic.height
    scale = max(width / w, height / h)
    pic.crop_left = pic.crop_right = (1 - width / (w * scale)) / 2
    pic.crop_top = pic.crop_bottom = (1 - height / (h * scale)) / 2
    pic.width, pic.height = width, height


# --- the main column ---


def reclaim_overflow(elements: list, groups: list[list[object]], ctx: Ctx) -> None:
    """Draw the main column's architecture diagram smaller when it ran past the body.

    Blocks are drawn in one pass, so a diagram claims its natural height before
    the blocks under it exist; a title that wrapped to two lines, or a
    translation with longer module labels, can then push the last block off the
    slide. The diagram is the one block here that can be drawn smaller, so it is
    drawn again in the height that leaves the rest of the page room, and
    everything below it moves up by what that gave back. The counterpart of
    :func:`spread_page_blocks`, which hands out space instead of reclaiming it.
    """
    overflow = ctx.cursor.top - ctx.layout.content_bottom
    index = next(
        (i for i, element in enumerate(elements)
         if isinstance(element, model.Block) and element.SHRINKABLE),
        None,
    )
    if overflow <= 0 or index is None or not groups[index]:
        return
    shapes = groups[index]
    top = min(shape.top for shape in shapes)
    height = max(shape.top + shape.height for shape in shapes) - top
    for shape in shapes:                       # python-pptx has no shape delete
        shape._element.getparent().remove(shape._element)

    end = ctx.cursor.top
    mark = len(ctx.slide.shapes)
    redrawn = elements[index].redraw_pptx(ctx, top, height - overflow)
    groups[index] = list(ctx.slide.shapes)[mark:]
    lift = height - redrawn
    for group in groups[index + 1:]:
        for shape in group:
            shape.top -= lift
    ctx.cursor.top = end - lift


def spread_page_blocks(gap: model.PageGap | None,
                       groups: list[list[object]], ctx: Ctx) -> None:
    """Honor ``p.gap(...)`` by distributing the main column's remaining height.

    Blocks are first drawn with the renderer's ordinary rhythm so their native
    geometry remains unchanged. We then move each later block group down by a
    cumulative share of the unused height. This mirrors the viewer's flex
    spacers without requiring a second geometry pass.
    """
    seams = len(groups) - 1
    if gap is None or seams <= 0:
        return
    remaining = max(0, ctx.cursor.remaining())
    if remaining == 0:
        return

    per_seam = remaining // seams
    if gap.max_px is not None:
        per_seam = min(per_seam, int(Inches(gap.max_px / 96)))
    if per_seam <= 0:
        return

    for index, shapes in enumerate(groups[1:], start=1):
        offset = per_seam * index
        for shape in shapes:
            shape.top += offset


# --- the footnote band ---


def footnote_height(texts: list[str], width: int) -> int:
    """The band's height (EMU) for ``texts`` set ``width`` wide, at least the minimum.

    Each note is counted in wrapped lines at the band's type, its number
    included, over the band's inset; three one-line notes fill the minimum.
    """
    lines = sum(
        estimate_text_height(f"{i} {_plain(text)}", font_pt=theme.FOOTNOTE_PT,
                             width=width, line_height=FOOTNOTE_LINE_HEIGHT)
        for i, text in enumerate(texts, 1)
    )
    return max(px(FOOTNOTE_BAND_MIN), lines + px(FOOTNOTE_INSET))


def _plain(text: str) -> str:
    paras = parse_markdown(text)
    return "".join(run.text for run in paras[0].runs) if paras else text


def draw_footnotes(texts: list[str], band: Layout, ctx: Ctx) -> None:
    """The page's footnotes, numbered, in one text box that is the band.

    The text sits at the band's bottom; the box keeps the band's size, since
    PowerPoint fitting it to the text would move the text to the band's top.
    """
    if not texts:
        return
    size = theme.FOOTNOTE_PT
    box = ctx.slide.shapes.add_textbox(
        band.content_left, band.content_top, band.content_width,
        band.content_bottom - band.content_top,
    )
    tf = box.text_frame
    tf.word_wrap = True
    tf.auto_size = MSO_AUTO_SIZE.NONE
    tf.vertical_anchor = MSO_ANCHOR.BOTTOM
    tf.margin_left = tf.margin_right = tf.margin_bottom = 0
    tf.margin_top = px(FOOTNOTE_INSET)
    for i, text in enumerate(texts, 1):
        p = tf.paragraphs[0] if i == 1 else tf.add_paragraph()
        p.line_spacing = Pt(size * FOOTNOTE_LINE_HEIGHT)
        style_run(p.add_run(), f"{i} ", size_pt=size, color=theme.ACCENT1)
        apply_runs(p, parse_markdown(text)[0].runs, size_pt=size, color=theme.DK2)
