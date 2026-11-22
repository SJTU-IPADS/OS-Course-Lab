"""The `slide` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Slide(Block):
    """Slide text: markdown, with `<mark>` and file links."""
    KIND = "slide"
    BOOK_TEXT = True
    TARGETS = frozenset({"viewer", "pptx", "transcript"})
    MARKS = True
    FILE_LINKS = True

    def to_marp(self, ctx):
        from ..markup.fields import slide_text
        from ..renderers.viewer.blocks import float_image_html
        content = slide_text(self.content)
        image = self.float_image
        if image and image.get("valign") == "middle":
            # `valign="middle"`: the text and the image side by side in one flex
            # row (`.lk-float-middle` in the theme), each centred against the
            # other. The text gets a wrapper of its own so its paragraphs and list
            # stay one flex item; the image follows it, on the right.
            return [
                '<div class="lk-float lk-float-middle">',
                '<div class="lk-float-text">',
                "",
                content,
                "",
                "</div>",
                float_image_html(image),
                "</div>",
                "",
            ]
        if image:
            # The theme lays the slide body out as a flex column, where a bare
            # floated <img> becomes a flex item and `float` is ignored (it would
            # stack above the text). Wrapping the image and the text in one
            # block-flow container — `.lk-float`, a `flow-root` in the theme —
            # restores normal flow so the text wraps to the image's left. The float
            # must precede the text in document order; blank lines around the inner
            # content keep Marp parsing it as markdown rather than raw HTML.
            return [
                '<div class="lk-float">',
                "",
                float_image_html(image),
                "",
                content,
                "",
                "</div>",
                "",
            ]
        return [content, ""]

    def to_pptx(self, ctx):
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_document, draw_float
        blocks = ctx.read.slide(self.content)
        if self.float_image:
            draw_float(ctx, self.float_image, blocks)
        else:
            draw_document(ctx, blocks)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks
        return md_blocks(self.content, refs=ctx.resolve_ref) + footnotes(self, ctx)

    def to_transcript(self, ctx):
        from ..renderers.transcript.blocks import img
        body = ctx.blocks(ctx.read.slide(self.content))
        if self.float_image:
            floated = img(
                self.float_image["src"], self.float_image.get("alt", ""), ctx, pct=38,
            )
            body = f'<span class="tx-float">{floated}</span>{body}'
        return f'<div class="tx-slide">{body}</div>'
