"""The `aside` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Aside(Block):
    """A quoted aside."""
    KIND = "aside"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})

    def to_marp(self, ctx):
        return [f"> {str(self.content).strip()}", ""]

    def to_pptx(self, ctx):
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_document
        draw_document(ctx, self.read_deck(ctx.read))
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks
        body = md_blocks(self.content, refs=ctx.resolve_ref) + footnotes(self, ctx)
        return "\\begin{quote}\n\\small %s\n\\end{quote}" % body

    def to_transcript(self, ctx):
        return ctx.blocks(self.read_deck(ctx.read))
