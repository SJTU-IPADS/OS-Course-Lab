"""Native PPTX renderer: lecture AST → an editable PowerPoint deck.

One slide per page (deck order). The cover and the transition pages draw
themselves; every other page is laid out by :mod:`page` as three bands, the
title, the body and the footnote band, with the body split into an image
column and a main column when the page has a ``side_image``. Shapes are native
(text frames, tables, pictures, autoshapes), so the deck is editable in
PowerPoint; styling is ported from the viewer theme, but vertical spacing is
estimated (no browser to measure text).
"""

from __future__ import annotations

import tempfile
from pathlib import Path

from pptx import Presentation

from lecturekit import model, rasterize, variables
from lecturekit.dsl import slugify
from .blocks import Ctx
from .layout import Cursor, Layout
from .page import draw_page

# python-pptx's built-in blank layout (no placeholders).
_BLANK_LAYOUT = 6


class PptxRenderer:
    name = "pptx"

    def __init__(self, *, asset_root: Path | None = None):
        self.asset_root = asset_root

    def render(self, lecture: model.Lecture, output_dir: Path) -> Path:
        output_dir.mkdir(parents=True, exist_ok=True)
        # Nothing moves in PowerPoint: each variable takes its default.
        lecture = variables.for_static(lecture)
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
                      borrowed=lecture.borrowed, title=page.title)
            candidates = model.select_blocks(page, "pptx")
            # New boxes reserve the finished animation's extent on every
            # frame. Legacy pages retain their original geometry.
            reserve_held = page.valign is not None or page.halign is not None or any(
                b.kind in model.MARKER_KINDS or b.halign is not None or b.valign is not None
                for b in candidates
            )
            visible_blocks = [b for b in candidates
                              if reserve_held or not model.block_held(page, b)]
            if len(visible_blocks) == 1 and visible_blocks[0].kind == "cover":
                visible_blocks[0].to_pptx(ctx)
                continue
            if model.is_bridge_page(page):
                visible_blocks[0].to_pptx(ctx)
                continue
            draw_page(page, visible_blocks, ctx)

        out = output_dir / f"{slugify(lecture.title)}.pptx"
        prs.save(str(out))
        return out
