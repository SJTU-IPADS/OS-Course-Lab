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
        from ..renderers.viewer.blocks import file_buttons, float_image_html
        content = file_buttons(str(self.content).strip())
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
        from ..renderers.pptx.blocks import (
            FLOAT_GAP,
            GAP,
            estimate_paras_height,
            fill_paras,
            float_picture,
            textbox,
        )
        from ..renderers.pptx.text import parse_markdown
        pic = None
        width = ctx.layout.content_width
        if self.float_image:
            pic = float_picture(self.float_image, ctx, ctx.cursor.top)
            if pic is not None:
                width -= pic.width + FLOAT_GAP
        paras = parse_markdown(str(self.content))
        height = estimate_paras_height(paras, width)
        box, tf = textbox(ctx, height, width=width)
        fill_paras(tf, paras)
        # A floated picture may be taller than the text beside it; the block owns
        # whichever is taller, so the next block clears both.
        band = max(height, pic.height) if pic is not None else height
        if pic is not None and self.float_image.get("valign") == "middle":
            # `valign="middle"`: each is centred in the band the taller one sets.
            pic.top = box.top + (band - pic.height) // 2
            box.top = box.top + (band - height) // 2
        ctx.cursor.place(band)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks
        return md_blocks(self.content, refs=ctx.resolve_ref) + footnotes(self, ctx)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import img
        from ..renderers.transcript.text import markdown
        body = markdown(self.content)
        if self.float_image:
            floated = img(
                self.float_image["src"], self.float_image.get("alt", ""), embedder,
                pct=38,
            )
            body = f'<span class="tx-float">{floated}</span>{body}'
        return f'<div class="tx-slide">{body}</div>'
