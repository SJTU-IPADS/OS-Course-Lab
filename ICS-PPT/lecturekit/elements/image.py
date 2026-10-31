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
        from pptx.util import Emu, Pt
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import GAP, _caption, requested_image_size, resolve_image
        from ..renderers.pptx.layout import fit_within
        image = self.content
        path = resolve_image(image["src"], ctx)
        if path is not None:
            pic = ctx.slide.shapes.add_picture(
                str(path), ctx.layout.content_left, ctx.cursor.top
            )
            # Fit the picture into the content width and the leftover vertical space
            # (the viewer's max-width/max-height:100%), reserving room for a caption.
            caption_reserve = Emu(Pt(theme.CAPTION_PT * 1.4)) + GAP if image.get("caption") else 0
            max_h = max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining() - caption_reserve)
            requested = requested_image_size(image, pic.width, pic.height, ctx.layout)
            if requested is not None:
                pic.width, pic.height = requested
            pic.width, pic.height = fit_within(
                pic.width, pic.height, ctx.layout.content_width, max_h
            )
            # center horizontally within the content box
            pic.left = ctx.layout.content_left + (ctx.layout.content_width - pic.width) // 2
            ctx.cursor.place(pic.height)
            ctx.cursor.advance(GAP)
        if image.get("caption"):
            _caption(image["caption"], image.get("caption_align") or "center", ctx)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import figure, includegraphics
        return figure(includegraphics(self.content, ctx), self.content.get("caption"), self, ctx)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import caption, img
        content = self.content
        figure = img(content["src"], content.get("alt", ""), embedder)
        framed = " tx-framed" if content.get("framed") else ""
        return f'<figure class="tx-fig{framed}">{figure}{caption(content.get("caption"))}</figure>'
