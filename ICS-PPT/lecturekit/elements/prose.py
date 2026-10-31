"""The `prose` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Prose(Block):
    """Book-only running text."""
    KIND = "prose"
    BOOK_TEXT = True
    TARGETS = frozenset({"latex"})

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks
        return md_blocks(self.content, refs=ctx.resolve_ref) + footnotes(self, ctx)
