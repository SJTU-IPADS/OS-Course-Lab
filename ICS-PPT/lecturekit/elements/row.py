"""The `row` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Row(Block):
    """Several images side by side."""
    KIND = "row"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    REF = True
    CENTERED = True

    def check_content(self, page_id: str) -> None:
        content = self.content
        items = content.get("items") or []
        if not items:
            raise model.ValidationError(f"Row has no images on {page_id}")
        for item in items:
            src = item.get("src")
            if not isinstance(src, str) or not src.strip():
                raise model.ValidationError(f"Empty row image src on {page_id}")
            align = item.get("caption_align")
            if align is not None and align not in ("left", "center", "right"):
                raise model.ValidationError(
                    f"Row image caption_align on {page_id} must be "
                    f"left/center/right, got {align!r}"
                )

    def to_marp(self, ctx):
        """A row of side-by-side image figures.

        One raw-HTML line (like `_architecture`) so Marp passes it through. Each
        item is an `lk-figure` reusing the standalone-image markup; an unsized item
        shares the track evenly (`flex:1 1 0`), a sized item takes its width
        (`flex:0 0 <width>`) and the rest split the remainder. A row-level caption
        renders as a trailing <figcaption>.
        """
        from ..renderers.viewer.blocks import css_length, figure_classes, figure_inner, inline_md
        content = self.content
        parts = ['<figure class="lk-row"><div class="lk-row-track">']
        for image in content["items"]:
            img, caption = figure_inner(image)
            width = image.get("width")
            basis = f"flex:0 0 {css_length(width)}" if width is not None else "flex:1 1 0"
            parts.append(
                f'<figure class="{figure_classes(image)} lk-row-item" '
                f'style="{basis}">{img}{caption}</figure>'
            )
        parts.append("</div>")
        if content.get("caption"):
            parts.append(f'<figcaption>{inline_md(content["caption"])}</figcaption>')
        parts.append("</figure>")
        return ["".join(parts), ""]

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        return blocks.mark_figure(lines, numbers, inside_image=False)

    def to_pptx(self, ctx):
        """A track of pictures side by side, each under its own share of the width.

        Widths follow the theme's flex bases: a sized item takes its width and the
        unsized ones split what is left. Items are top-aligned and the track is
        centered, so pictures of differing heights line up at their tops and each
        caption sits directly under its own picture.
        """
        from pptx.util import Emu, Pt
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            GAP,
            _caption,
            requested_image_size,
            resolve_image,
            row_caption,
            row_widths,
        )
        from ..renderers.pptx.layout import fit_within, LINE_HEIGHT
        content = self.content
        items = content["items"]
        gap = Emu(Pt(theme.BODY_PT * 1.2))     # .lk-row-track gap: 1.2em
        caption = content.get("caption")
        reserve = Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + GAP if caption else 0
        max_height = max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining() - reserve)
        widths = row_widths(items, ctx.layout.content_width - gap * (len(items) - 1),
                             ctx.layout)

        # Pictures land at their natural size, so each has to be added before its
        # final width is known; the track can only be centered once all are placed.
        top = ctx.cursor.top
        placed = []
        for item, width in zip(items, widths):
            path = resolve_image(item["src"], ctx)
            if path is None:
                continue
            pic = ctx.slide.shapes.add_picture(str(path), ctx.layout.content_left, top)
            requested = requested_image_size(item, pic.width, pic.height, ctx.layout)
            if requested is not None:
                pic.width, pic.height = requested
            item_reserve = (
                Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + GAP
                if item.get("caption") else 0
            )
            pic.width, pic.height = fit_within(
                pic.width, pic.height, width, max(Emu(Pt(1)), max_height - item_reserve)
            )
            placed.append((pic, item))
        if not placed:
            return

        used = sum(pic.width for pic, _ in placed) + gap * (len(placed) - 1)
        left = ctx.layout.content_left + (ctx.layout.content_width - used) // 2
        height = 0
        for pic, item in placed:
            pic.left, pic.top = left, top
            left += pic.width + gap
            below = pic.height
            if item.get("caption"):
                below += row_caption(item["caption"], pic, ctx)
            height = max(height, below)
        ctx.cursor.place(height)
        ctx.cursor.advance(GAP)
        if caption:
            _caption(caption, "center", ctx)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import _graphic, figure, fmt
        from ..renderers.latex.text import inline
        items = self.content["items"]
        share = fmt(0.9 / max(len(items), 1))
        parts = []
        for item in items:
            path = ctx.assets.copy(ctx.lecture_id, ctx.asset_root, item["src"])
            caption = inline(item["caption"]) if item.get("caption") else ""
            graphic = _graphic(path, r"\linewidth", bool(item.get("framed")))
            parts.append(
                r"\begin{subfigure}[t]{%s\textwidth}%s\caption*{%s}\end{subfigure}"
                % (share, graphic, caption)
            )
        return figure("\n".join(parts), self.content.get("caption"), self, ctx)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import caption, img
        from ..renderers.transcript.images import DEFAULT_WIDTH_PX
        items = self.content["items"]
        widths = []
        for item in items:
            figure = embedder.embed(item["src"])
            widths.append(figure.width_px if figure else DEFAULT_WIDTH_PX)
        total = sum(widths) or 1
        cells = []
        for item, width in zip(items, widths):
            share = max(15, round(100 * width / total) - 2)
            cells.append(
                f'<span class="tx-cell" style="width:{share}%">'
                f'{img(item["src"], item.get("alt", ""), embedder, pct=100)}'
                f'{caption(item.get("caption"))}</span>'
            )
        return (
            f'<figure class="tx-fig tx-row">{"".join(cells)}'
            f'{caption(self.content.get("caption"))}</figure>'
        )
