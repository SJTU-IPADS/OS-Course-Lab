"""The `link` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Link(Block):
    """A hyperlink line."""
    KIND = "link"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})

    def to_marp(self, ctx):
        from ..markup.fields import escape_html
        label = escape_html(self.content["label"])
        url = escape_html(self.content["url"])
        return [f'- <a href="{url}" target="_blank" rel="noopener noreferrer">{label}</a>', ""]

    def to_pptx(self, ctx):
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_document
        draw_document(ctx, self.read_deck(ctx.read))
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import inline
        return r"\href{%s}{%s}%s" % (
            self.content["url"], inline(self.content["label"]), footnotes(self, ctx)
        )

    def to_transcript(self, ctx):
        from ..markup import document
        line = document.phrasing(self.read_deck(ctx.read))
        return f'<p class="tx-link">{ctx.inline(line)}</p>'
