"""The `architecture` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Architecture(Block):
    """A layered architecture diagram."""
    KIND = "architecture"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    REF = True
    CENTERED = True
    SHRINKABLE = True

    def check_content(self, page_id: str) -> None:
        content = self.content
        flow = content.get("flow")
        if flow is not None and flow not in model.ARCH_FLOWS:
            raise model.ValidationError(
                f"Architecture flow on {page_id} must be one of "
                f"{sorted(model.ARCH_FLOWS)} or None, got {flow!r}"
            )
        layers = content.get("layers") or []
        if not layers:
            raise model.ValidationError(f"Architecture has no layers on {page_id}")
        for layer in layers:
            modules = layer.get("modules") or []
            if not modules:
                raise model.ValidationError(
                    f"Architecture layer has no modules on {page_id}"
                )
            for module in modules:
                if not isinstance(module, str) or not module.strip():
                    raise model.ValidationError(
                        f"Empty architecture module on {page_id}"
                    )

    def to_marp(self, ctx):
        """A layered + modular diagram: stacked layer bands of module boxes.

        Pure HTML styled by the theme (`.lk-arch*`) — layers stack and modules sit
        side by side via CSS, no measured geometry. A `flow` chevron is emitted
        between adjacent layers only. Emitted as one line so Marp passes the raw
        HTML through untouched rather than reparsing it as markdown.
        """
        from .. import model
        from ..renderers.viewer.blocks import escape_html, inline_md
        diagram = self.content
        flow = diagram.get("flow")
        parts = ['<figure class="lk-arch">']
        layers = diagram["layers"]
        for i, layer in enumerate(layers):
            if i and flow:
                glyph = model.ARCH_FLOW_GLYPH.get(flow, "")
                parts.append(
                    f'<div class="lk-arch-flow lk-arch-flow--{flow}">{glyph}</div>'
                )
            parts.append('<div class="lk-arch-layer">')
            title = layer.get("title")
            if title:
                parts.append(f'<div class="lk-arch-label">{escape_html(title)}</div>')
            parts.append('<div class="lk-arch-modules">')
            for module in layer["modules"]:
                cls = "lk-arch-module"
                if module == model.ARCH_ELLIPSIS:
                    cls += " lk-arch-module--more"
                parts.append(f'<div class="{cls}">{escape_html(module)}</div>')
            parts.append("</div></div>")
        if diagram.get("caption"):
            parts.append(f"<figcaption>{inline_md(diagram['caption'])}</figcaption>")
        parts.append("</figure>")
        return ["".join(parts), ""]

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        return blocks.mark_figure(lines, numbers, inside_image=False)

    def to_pptx(self, ctx, *, ceiling=None):
        """Stacked layer bands, each a label gutter beside a row of module boxes.

        The viewer hands this to flexbox; here every width is computed from the
        content column and every height estimated from the text. A diagram that
        would not fit the room left on the page — or a ``ceiling`` the renderer
        imposes, once it knows what the blocks below it need — is measured again at
        a smaller scale rather than running off the slide.
        """
        from pptx.util import Emu, Pt
        from .. import model
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            ARCH_MIN_SCALE,
            ArchMetrics,
            GAP,
            _caption,
            arch_band,
            arch_flow,
            arch_layout,
        )
        from ..renderers.pptx.layout import LINE_HEIGHT
        diagram = self.content
        caption = diagram.get("caption")
        reserve = Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + GAP if caption else 0
        m = ArchMetrics.at(1.0)
        height, bands = arch_layout(diagram, ctx, m)
        room = ctx.cursor.remaining() if ceiling is None else min(ceiling, ctx.cursor.remaining())
        budget = room - reserve
        if height > budget > 0:
            m = ArchMetrics.at(max(ARCH_MIN_SCALE, budget / height))
            height, bands = arch_layout(diagram, ctx, m)

        glyph = model.ARCH_FLOW_GLYPH.get(diagram.get("flow"), "")
        top = ctx.cursor.place(height)
        for i, (layer, band) in enumerate(zip(diagram["layers"], bands)):
            if i:
                top += m.fig_gap
                if glyph:
                    top += arch_flow(glyph, top, m, ctx) + m.fig_gap
            arch_band(layer, band, top, m, ctx)
            top += band["height"]
        ctx.cursor.advance(GAP)
        if caption:
            _caption(caption, "center", ctx)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import FLOW_ARROWS, figure
        from ..renderers.latex.text import inline
        content = self.content
        arrow = FLOW_ARROWS.get(content.get("flow") or "")
        layers = []
        for layer in content["layers"]:
            modules = " & ".join(inline(m) for m in layer["modules"])
            spec = "|" + "c|" * len(layer["modules"])
            rows = [r"\hline", modules + r" \\", r"\hline"]
            title = layer.get("title")
            head = r"\textbf{%s}\\[2pt]" % inline(title) if title else ""
            layers.append(
                "%s\\begin{tabular}{%s}\n%s\n\\end{tabular}" % (head, spec, "\n".join(rows))
            )
        separator = f"\n\n{arrow}\n\n" if arrow else "\n\n"
        return figure(separator.join(layers), content.get("caption"), self, ctx)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import FLOW_GLYPH, caption, is_more
        from ..renderers.transcript.text import escape
        content = self.content
        flow = FLOW_GLYPH.get(content.get("flow") or "")
        bands = []
        for index, layer in enumerate(content["layers"]):
            if index and flow:
                bands.append(f'<div class="tx-flow">{flow}</div>')
            title = (
                f'<div class="tx-layer-title">{escape(layer["title"])}</div>'
                if layer.get("title") else ""
            )
            modules = "".join(
                f'<span class="tx-mod{" tx-mod-more" if is_more(m) else ""}">'
                f'{escape(str(m))}</span>'
                for m in layer["modules"]
            )
            bands.append(f'<div class="tx-layer">{title}<div class="tx-mods">{modules}</div></div>')
        return (
            f'<figure class="tx-fig tx-arch">{"".join(bands)}'
            f'{caption(content.get("caption"))}</figure>'
        )

    def redraw_pptx(self, ctx, top, max_height):
        """Draw again at ``top``, within ``max_height``.

        Blocks are drawn in one pass, so a diagram takes its natural size before
        the blocks below it exist. The renderer calls this when that left the
        page overflowing; it returns the height the diagram took the second time.
        """
        ctx.cursor.top = top
        self.to_pptx(ctx, ceiling=max_height)
        return ctx.cursor.top - top
