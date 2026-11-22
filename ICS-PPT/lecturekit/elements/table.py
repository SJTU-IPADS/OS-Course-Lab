"""The `table` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Table(Block):
    """A table; its cells are markdown and may carry file links."""
    KIND = "table"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    FILE_LINKS = True
    CENTERED = True

    def markdown(self) -> list[str]:
        """The table as the deck's parser is given it."""
        from ..markup.fields import table_cell
        from ..renderers.viewer.blocks import ALIGN_DELIMITERS
        table = self.content
        headers = table["headers"]
        align = table["align"]
        if align is None:
            delimiters = ["---"] * len(headers)
        else:
            delimiters = [ALIGN_DELIMITERS[a] for a in align]
        lines = [
            "| " + " | ".join(table_cell(h) for h in headers) + " |",
            "| " + " | ".join(delimiters) + " |",
        ]
        for row in table["rows"]:
            lines.append("| " + " | ".join(table_cell(c) for c in row) + " |")
        return lines

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import fixed_table
        lines = self.markdown()
        widths = self.content.get("widths")
        if widths is None:
            return [*lines, ""]
        return fixed_table(lines, widths)

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        return blocks.mark_tucked(lines, numbers)

    def to_pptx(self, ctx):
        from ..markup import document
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_document, draw_table
        widths = self.content.get("widths")
        for block in ctx.read.markdown("\n".join(self.markdown())):
            if isinstance(block, document.Table):
                draw_table(ctx, block, widths)
            else:
                draw_document(ctx, [block])
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from ..renderers.latex.blocks import COLUMN_SPEC, footnotes, width_column
        from ..renderers.latex.text import inline
        headers = self.content["headers"]
        align = self.content.get("align") or ["left"] * len(headers)
        widths = self.content.get("widths")
        if widths is None:
            spec = "".join(COLUMN_SPEC[a] for a in align)
        else:
            spec = "".join(width_column(a, w) for a, w in zip(align, widths))
        lines = [
            r"\begin{center}",
            r"\begin{tabular}{%s}" % spec,
            r"\toprule",
            " & ".join(inline(h) for h in headers) + r" \\",
            r"\midrule",
        ]
        for row in self.content["rows"]:
            lines.append(" & ".join(inline(cell) for cell in row) + r" \\")
        lines += [r"\bottomrule", r"\end{tabular}", r"\end{center}"]
        return "\n".join(lines) + footnotes(self, ctx)

    def to_transcript(self, ctx):
        from ..markup import document
        widths = self.content.get("widths")
        return "".join(
            ctx.table(block, widths) if isinstance(block, document.Table) else ctx.blocks([block])
            for block in ctx.read.markdown("\n".join(self.markdown()))
        )
