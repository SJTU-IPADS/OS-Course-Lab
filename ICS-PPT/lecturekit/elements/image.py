"""The `image` element."""
from __future__ import annotations

from dataclasses import replace

from ..model import Block, element


@element
class Image(Block):
    """A figure: one image with an optional caption."""
    KIND = "image"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    REF = True
    CENTERED = True
    VARIABLE_KEYS = frozenset({"width", "height"})

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import figure_classes, figure_inner
        # Emitted as a semantic <figure> (not Marp image markdown) so the caption can
        # be styled and centered and an opt-in frame can hang off a modifier class.
        image = self.content
        img, caption = figure_inner(image)
        return [f'<figure class="{figure_classes(image)}">{img}{caption}</figure>', ""]

    def sized_in_box(self, in_row):
        requested = self.content.get("width")
        if not requested:
            return self, ""
        # The row already assigned this width to the box. Resolving a
        # percentage against that box a second time would halve it.
        block = replace(self, content={**self.content, "width": "100%"})
        return block, f' style="width:{"100%" if in_row else requested}"'

    def row_basis(self):
        return self.content.get("width") or None

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        return blocks.mark_figure(lines, numbers, inside_image=True)

    def to_pptx(self, ctx):
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_pictures
        draw_pictures(ctx, [self.content])
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import figure, includegraphics
        return figure(includegraphics(self.content, ctx), self.content.get("caption"), self, ctx)

    def to_transcript(self, ctx):
        from ..renderers.transcript.blocks import caption, img
        content = self.content
        figure = img(content["src"], content.get("alt", ""), ctx)
        framed = " tx-framed" if content.get("framed") else ""
        return f'<figure class="tx-fig{framed}">{figure}{caption(content.get("caption"), ctx)}</figure>'
