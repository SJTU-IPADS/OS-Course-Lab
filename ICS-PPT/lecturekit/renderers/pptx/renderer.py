"""Native PPTX renderer: lecture AST → an editable PowerPoint deck.

One slide per page (deck order). Each slide draws the page title, then the
page's PPTX-visible blocks stacked top-to-bottom by the layout cursor, then any
accumulated footnotes. A ``side_image`` takes a column under the title, on
either side, and the rest of the page is laid out beside it. Shapes are native (text frames, tables, pictures,
autoshapes), so the deck is editable in PowerPoint; styling is ported from the
viewer theme, but vertical spacing is estimated (no browser to measure text).
"""

from __future__ import annotations

import tempfile
from pathlib import Path

from pptx import Presentation
from pptx.util import Inches

from lecturekit import model, rasterize
from lecturekit.dsl import slugify
from .blocks import (
    PPTX_KINDS, Ctx, draw_block, draw_bridge, draw_cover, draw_footnotes,
    draw_side_images, draw_title, redraw_architecture, side_layout,
)
from .layout import Cursor, Layout
from .elements import draw_elements, place_shapes

# python-pptx's built-in blank layout (no placeholders).
_BLANK_LAYOUT = 6


class PptxRenderer:
    name = "pptx"

    def __init__(self, *, asset_root: Path | None = None):
        self.asset_root = asset_root

    def render(self, lecture: model.Lecture, output_dir: Path) -> Path:
        output_dir.mkdir(parents=True, exist_ok=True)
        # PowerPoint cannot embed SVG, so vector figures are rasterized on the
        # way in. python-pptx copies a picture's bytes into the package as the
        # picture is added, so the PNGs are only needed while slides are built.
        with tempfile.TemporaryDirectory(prefix="lecturekit-svg-") as tmp:
            return self._render(lecture, output_dir,
                                rasterize.SvgRasterizer(Path(tmp)))

    def _render(self, lecture: model.Lecture, output_dir: Path,
                rasterizer: rasterize.SvgRasterizer) -> Path:
        layout = Layout.from_ratio(lecture.ratio)

        prs = Presentation()
        prs.slide_width = layout.width
        prs.slide_height = layout.height
        blank = prs.slide_layouts[_BLANK_LAYOUT]

        for page in model.flatten_pages(lecture.children):
            slide = prs.slides.add_slide(blank)
            ctx = Ctx(slide=slide, layout=layout, cursor=Cursor(layout),
                      asset_root=self.asset_root, rasterizer=rasterizer,
                      borrowed=lecture.borrowed)
            candidates = model.select_blocks(page, "pptx", PPTX_KINDS | model.MARKER_KINDS)
            # New boxes reserve the finished animation's extent on every
            # frame. Legacy pages retain their original geometry.
            reserve_held = page.valign is not None or page.halign is not None or any(
                b.kind in model.MARKER_KINDS or b.halign is not None or b.valign is not None
                for b in candidates
            )
            visible_blocks = [b for b in candidates
                              if reserve_held or not model.block_held(page, b)]
            if len(visible_blocks) == 1 and visible_blocks[0].kind == "cover":
                draw_cover(page.title, visible_blocks[0], ctx)
                continue
            if model.is_bridge_page(page):
                draw_bridge(visible_blocks[0], ctx)
                continue
            # A side_image column narrows the content box. The title keeps the
            # slide's width, and the column starts under it, on either side.
            sides = [block for block in visible_blocks if block.kind == "side_image"]
            visible_blocks = [b for b in visible_blocks if b.kind != "side_image"]
            # Slide text set under a side image is drawn in its column.
            side_texts = [b for b in visible_blocks if b.in_side] if sides else []
            if side_texts:
                visible_blocks = [b for b in visible_blocks if not b.in_side]
            # `valign="middle"`: both columns are centred in the band from
            # under the title to the content box's bottom, once drawn.
            valign = model.page_valign(page, (*sides, *visible_blocks))
            middle = valign == "middle"
            draw_title(page.title, ctx)
            under_title = ctx.cursor.top
            stack_shapes: list[object] = []
            if sides:
                before = len(slide.shapes)
                draw_side_images(
                    sides, ctx, top=under_title, texts=side_texts,
                    fit_bottom=layout.content_bottom if valign in ("middle", "bottom") else None,
                )
                if side_texts or valign == "bottom":
                    stack_shapes = list(slide.shapes)[before:]
                ctx.layout = side_layout(sides, layout)
                ctx.cursor = Cursor(ctx.layout)
                ctx.cursor.top = under_title
            footnotes: list[str] = [
                note for block in (*sides, *side_texts) for note in block.footnotes
            ]
            block_shapes: list[list[object]] = []
            held_shapes = []
            has_layout = page.halign is not None or page.valign is not None or any(
                b.kind in model.MARKER_KINDS or b.halign is not None
                or b.valign is not None for b in visible_blocks
            )
            if has_layout:
                block_shapes = draw_elements(
                    model.layout(visible_blocks), ctx,
                    inherited=(page.halign, valign),
                    held_shapes=held_shapes, page=page,
                )
                footnotes.extend(note for b in visible_blocks
                                 if not model.block_held(page, b) for note in b.footnotes)
            else:
                for block in visible_blocks:
                    before = len(slide.shapes)
                    draw_block(block, ctx)
                    shapes = list(slide.shapes)[before:]
                    block_shapes.append(shapes)
                    if model.block_held(page, block):
                        held_shapes.extend(shapes)
                    else:
                        footnotes.extend(block.footnotes)
                _reclaim_overflow(visible_blocks, block_shapes, slide, ctx)
            _spread_page_blocks(page.gap, block_shapes, ctx)
            if middle:
                band = (under_title, ctx.layout.content_bottom)
                _centre_shapes([s for group in block_shapes for s in group], *band)
                _centre_shapes(stack_shapes, *band)
            elif valign == "bottom":
                place_shapes([s for group in block_shapes for s in group],
                             ctx.layout.content_left, under_title,
                             ctx.layout.content_width,
                             ctx.layout.content_bottom - under_title, valign="bottom")
                place_shapes(stack_shapes, 0, under_title, layout.width,
                             layout.content_bottom - under_title, valign="bottom")
            draw_footnotes(footnotes, ctx)
            for shape in held_shapes:
                shape._element.getparent().remove(shape._element)

        out = output_dir / f"{slugify(lecture.title)}.pptx"
        prs.save(str(out))
        return out


def _reclaim_overflow(blocks: list[object], groups: list[list[object]],
                      slide, ctx: Ctx) -> None:
    """Draw a page's architecture diagram smaller when the page ran past the box.

    Blocks are drawn in one pass, so a diagram claims its natural height before
    the blocks under it exist; a title that wrapped to two lines, or a
    translation with longer module labels, can then push the last block off the
    slide. The diagram is the one block here that can be drawn smaller, so it is
    drawn again in the height that leaves the rest of the page room, and
    everything below it moves up by what that gave back. The counterpart of
    :func:`_spread_page_blocks`, which hands out space instead of reclaiming it.
    """
    overflow = ctx.cursor.top - ctx.layout.content_bottom
    index = next(
        (i for i, block in enumerate(blocks) if block.kind == "architecture"),
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
    mark = len(slide.shapes)
    redrawn = redraw_architecture(blocks[index], ctx, top, height - overflow)
    groups[index] = list(slide.shapes)[mark:]
    lift = height - redrawn
    for group in groups[index + 1:]:
        for shape in group:
            shape.top -= lift
    ctx.cursor.top = end - lift


def _spread_page_blocks(gap: model.PageGap | None,
                        groups: list[list[object]], ctx: Ctx) -> None:
    """Honor ``p.gap(...)`` by distributing PPTX's remaining vertical space.

    Blocks are first drawn with the renderer's ordinary rhythm so their native
    geometry remains unchanged.  We then move each later block group down by a
    cumulative share of the unused canvas.  This mirrors the viewer's flex
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


def _centre_shapes(shapes: list[object], top: int, bottom: int) -> None:
    """Honor ``side_image(..., valign="middle")``: move a column's shapes down
    so that their extent is centred between ``top`` and ``bottom``.

    The shapes are drawn with the renderer's ordinary rhythm from ``top``, so
    the group is moved as one and its geometry stays. The viewer's flex
    spacers split the leftover space the same way. A group taller than the
    band is left where it is: it already overflows, and moving it up would
    put it under the title.
    """
    if not shapes:
        return
    first = min(shape.top for shape in shapes)
    last = max(shape.top + shape.height for shape in shapes)
    shift = ((top + bottom) - (first + last)) // 2
    if shift <= 0:
        return
    for shape in shapes:
        shape.top += shift
