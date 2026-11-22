"""The `columns` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Columns(Block):
    """Slide text set in several columns."""
    KIND = "columns"
    BOOK_TEXT = True
    TARGETS = frozenset({"viewer", "pptx", "transcript"})
    MARKS = True
    FILE_LINKS = True

    def check_content(self, page_id: str) -> None:
        content = self.content
        items = content.get("items") or []
        if len(items) < 2:
            raise model.ValidationError(
                f"Columns on {page_id} needs at least two columns, got {len(items)}"
            )
        for item in items:
            if not isinstance(item, str) or not item.strip():
                raise model.ValidationError(f"Empty column on {page_id}")
        widths = content.get("widths")
        if widths is not None and len(widths) != len(items):
            raise model.ValidationError(
                f"Columns widths on {page_id} has {len(widths)} entries, "
                f"expected {len(items)}"
            )

    def to_marp(self, ctx):
        """Slide text in side-by-side columns.

        A flex track of `lk-column` divs. Each column's markdown sits between blank
        lines so Marp parses it as markdown (an HTML block ends at a blank line).
        Columns share the track evenly; a weighted column carries its share as the
        flex grow factor.
        """
        from ..markup.fields import slide_text
        content = self.content
        widths = content.get("widths")
        lines = ['<div class="lk-columns">']
        for i, text in enumerate(content["items"]):
            style = f' style="flex-grow:{100 * widths[i]:.2f}"' if widths else ""
            lines += [
                f'<div class="lk-column"{style}>', "",
                slide_text(text), "", "</div>",
            ]
        lines += ["</div>", ""]
        return lines

    def to_pptx(self, ctx):
        """Slide text in side-by-side text boxes that share the content width.

        All start at the cursor's top, and the block owns the tallest of them.
        Columns split the width left after the gaps evenly, or by the block's
        ``widths`` shares.
        """
        from ..renderers.pptx.blocks import GAP
        from ..renderers.pptx.draw import draw_columns
        items = [ctx.read.slide(text) for text in self.content["items"]]
        draw_columns(ctx, items, self.content.get("widths"))
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        """Slide text the deck sets side by side; the book prints it in order."""
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import blocks as md_blocks
        text = "\n\n".join(str(item).strip() for item in self.content["items"])
        return md_blocks(text, refs=ctx.resolve_ref) + footnotes(self, ctx)

    def to_transcript(self, ctx):
        """Slide text the deck sets side by side; the sheet prints it in order."""
        return "".join(
            f'<div class="tx-slide">{ctx.blocks(ctx.read.slide(text))}</div>'
            for text in self.content["items"]
        )
