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
    VARIABLE_KEYS = frozenset({"width", "height"})

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
        from ..markup.fields import inline_md
        from ..renderers.viewer.blocks import css_length, figure_classes, figure_inner
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
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_pictures
        draw_pictures(ctx, self.content["items"], caption=self.content.get("caption"))
        ctx.cursor.advance(GAP)

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

    def to_transcript(self, ctx):
        from ..renderers.transcript.blocks import caption, img
        from ..renderers.transcript.images import DEFAULT_WIDTH_PX
        items = self.content["items"]
        widths = []
        for item in items:
            figure = ctx.embedder.embed(item["src"])
            widths.append(figure.width_px if figure else DEFAULT_WIDTH_PX)
        total = sum(widths) or 1
        cells = []
        for item, width in zip(items, widths):
            share = max(15, round(100 * width / total) - 2)
            cells.append(
                f'<span class="tx-cell" style="width:{share}%">'
                f'{img(item["src"], item.get("alt", ""), ctx, pct=100)}'
                f'{caption(item.get("caption"), ctx)}</span>'
            )
        return (
            f'<figure class="tx-fig tx-row">{"".join(cells)}'
            f'{caption(self.content.get("caption"), ctx)}</figure>'
        )
