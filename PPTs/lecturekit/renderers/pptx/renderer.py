"""Native PPTX renderer: lecture AST → an editable PowerPoint deck.

One slide per page (deck order). Each slide draws the page title, then the
page's PPTX-visible blocks stacked top-to-bottom by the layout cursor, then any
accumulated footnotes. Shapes are native (text frames, tables, pictures,
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
    draw_title,
)
from .layout import Cursor, Layout

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
            # A held block (written after `p.frames(...)`, on a frame before
            # the last) is dropped outright: slides here share no geometry, so
            # there is no layout to reserve space in.
            visible_blocks = [
                block
                for block in model.select_blocks(page, "pptx", PPTX_KINDS)
                if not model.block_held(page, block)
            ]
            if len(visible_blocks) == 1 and visible_blocks[0].kind == "cover":
                draw_cover(page.title, visible_blocks[0], ctx)
                continue
            if model.is_bridge_page(page):
                draw_bridge(visible_blocks[0], ctx)
                continue
            draw_title(page.title, ctx)
            footnotes: list[str] = []
            block_shapes: list[list[object]] = []
            for block in visible_blocks:
                before = len(slide.shapes)
                draw_block(block, ctx)
                block_shapes.append(list(slide.shapes)[before:])
                footnotes.extend(block.footnotes)
            _spread_page_blocks(page.gap, block_shapes, ctx)
            draw_footnotes(footnotes, ctx)

        out = output_dir / f"{slugify(lecture.title)}.pptx"
        prs.save(str(out))
        return out


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
