"""The `highlight` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Highlight(Block):
    """A chip that sets one statement apart."""
    KIND = "highlight"
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    CENTERED = True

    def check_content(self, page_id: str) -> None:
        content = self.content
        text = content.get("text")
        if not isinstance(text, str) or not text.strip():
            raise model.ValidationError(f"Empty highlight text on {page_id}")
        tone = content.get("tone")
        if tone not in model.HIGHLIGHT_TONES:
            raise model.ValidationError(
                f"Highlight tone on {page_id} must be one of "
                f"{sorted(model.HIGHLIGHT_TONES)}, got {tone!r}"
            )

    def to_marp(self, ctx):
        """A centered chip: the outer element centers, the inner <span> is the wash.

        The split is the point — an inline-block span hugs the widest line instead
        of becoming a full-width banner. `inline_md` already turns newlines into
        <br>, so a multi-line highlight grows the same chip taller.

        A chip carrying math swaps the outer <p> for a blocked-out <span>, because
        Marp renders math while parsing markdown and a line starting with `<p` is a
        raw HTML block it never parses — a `<span>` with text after it on the line
        is not, so the row arrives as a paragraph and the formula is typeset. The
        box is the same one; only the tag it is drawn on differs.
        """
        from .. import model
        from ..markup.fields import inline_md
        from ..renderers.viewer.blocks import CHIP_MATH_RE, chip_math_md
        content = self.content
        lines = model.highlight_lines(content["text"])
        tone = content["tone"]
        classes = f"lk-highlight lk-highlight--{tone}"
        if any(CHIP_MATH_RE.search(line) for line in lines):
            body = "<br>".join(chip_math_md(line) for line in lines)
            return [f'<span class="{classes}"><span>{body}</span></span>', ""]
        body = inline_md("\n".join(lines))
        return [f'<p class="{classes}"><span>{body}</span></p>', ""]

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        return blocks.mark_highlight(lines, numbers)

    def to_pptx(self, ctx):
        """A centered chip: a hairline frame in the tone, holding text in that tone.

        PowerPoint has no inline-block, so the width is *estimated* from the longest
        line (like every other box in this renderer) and centered in the content
        column. As in the deck the shape carries no fill — the tone is the outline
        and the ink, and what is inside the frame is white slide.
        """
        from pptx.enum.shapes import MSO_SHAPE
        from pptx.enum.text import PP_ALIGN
        from ..markup import document
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import (
            GAP,
            HIGHLIGHT_PAD,
            HIGHLIGHT_RULE_PT,
            HIGHLIGHT_SHADOW,
            drop_shadow,
            outline,
            shape_at,
        )
        from ..renderers.pptx.draw import Look, runs_height, space_lines, write_runs
        from ..renderers.pptx.layout import estimate_text_width
        from ..renderers.pptx.text import plain, runs, split_lines
        rows = split_lines(runs(document.phrasing(self.read_deck(ctx.read))))
        ink = theme.CHIP_INK[self.content["tone"]]
        look = Look(size=theme.HIGHLIGHT_PT, color=ink, bold=True)
        # Measured on what is shown, not on the source: a formula is as wide as
        # its typeset text, not as the markup that spelled it.
        wanted = max(estimate_text_width(plain(row), font_pt=look.size) for row in rows)
        room = ctx.layout.content_width - 2 * HIGHLIGHT_PAD
        wraps = wanted > room
        text_width = min(room, wanted)
        width = text_width + 2 * HIGHLIGHT_PAD
        height = 2 * HIGHLIGHT_PAD + sum(
            runs_height(row, look.size, room if wraps else ctx.layout.width) for row in rows)
        left = ctx.layout.content_left + (ctx.layout.content_width - width) // 2

        top = ctx.cursor.place(height)
        shape = shape_at(ctx, MSO_SHAPE.RECTANGLE, left, top, width, height)
        shape.fill.background()
        outline(shape, ink, HIGHLIGHT_RULE_PT)
        drop_shadow(shape, theme.SIDENOTE_BORDER, **HIGHLIGHT_SHADOW)

        frame = shape.text_frame
        frame.word_wrap = wraps
        for i, row in enumerate(rows):
            para = frame.add_paragraph() if i else frame.paragraphs[0]
            para.alignment = PP_ALIGN.CENTER
            space_lines(para, row, look.size)
            write_runs(para, row, look)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        """A centered chip. Lines become tabular rows, so it grows taller, not wider.

        A plain ``\\footnote`` after the macro would land outside the ``center``
        group and open a paragraph of its own, dropping the mark in the margin. So
        the marks ride the macro's optional argument — just outside the chip, as in
        the deck — and the texts follow, the same split ``figure`` uses.
        """
        from .. import model
        from ..renderers.latex.blocks import HIGHLIGHT_COLORS, display_math_inline
        from ..renderers.latex.text import inline
        content = self.content
        color = HIGHLIGHT_COLORS[content["tone"]]
        body = r" \\ ".join(
            inline(display_math_inline(line), refs=ctx.resolve_ref)
            for line in model.highlight_lines(content["text"])
        )
        marks = r"\footnotemark" * len(self.footnotes)
        out = r"\lkhighlight%s{%s}{%s}" % (f"[{marks}]" if marks else "", color, body)
        for note in self.footnotes:
            out += "\n" + r"\footnotetext{%s}" % inline(note, refs=ctx.resolve_ref)
        return out

    def to_transcript(self, ctx):
        from ..markup import document
        tone = self.content.get("tone", "yellow")
        body = ctx.inline(document.phrasing(self.read_deck(ctx.read)))
        return f'<p class="tx-highlight tx-{tone}"><span>{body}</span></p>'
