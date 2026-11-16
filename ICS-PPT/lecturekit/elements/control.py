"""The `slider` and `input` elements: controls bound to a deck variable.

A control shows a variable's value and lets the room change it. On the deck
it is a form control that ``vars.js`` (written into every deck) arms, and the
value it sets reaches every use of the variable on every slide. Where nothing
can move, it is the line of text :func:`lecturekit.variables.control_text`
writes, which states the default the page is drawn at, the range and the
marks.

The content is ``{"var": name, "label": ..., "marks": [...], "step": ...}``
(the last two a slider's). The variable's definition is not stored on the
block: ``variables.for_deck`` and ``variables.for_static`` hand it in as
``content["variable"]`` when a target reads the lecture, and a slider's step
as ``content["step_used"]``.

Without a label, the control is labelled with the variable's name set as a
math symbol, in the face and slant the figures give it (``--font-math``).
"""
from __future__ import annotations

from ..model import Block, element
from .. import model


class Control(Block):
    """What a slider and an input box share. Not a kind of its own."""
    TARGETS = frozenset({"viewer", "pptx", "latex", "transcript"})
    CONTROL = True
    CENTERED = True

    def check_content(self, page_id: str) -> None:
        content = self.content
        if not isinstance(content, dict) or not isinstance(content.get("var"), str):
            raise model.ValidationError(
                f"the {self.kind} block on {page_id} names no variable"
            )
        label = content.get("label")
        if label is not None and (not isinstance(label, str) or not label.strip()):
            raise model.ValidationError(
                f"the {self.kind} block on {page_id} has an empty label"
            )

    # --- what the variable says, once a target has resolved it ---

    def variable(self) -> dict:
        try:
            return self.content["variable"]
        except KeyError:
            raise model.ValidationError(
                f"the {self.kind} block for ${self.content.get('var')} was drawn "
                "before its variable was resolved; render the lecture through "
                "variables.for_deck / for_static"
            ) from None

    def fmt(self, value: float) -> str:
        from .. import variables
        return variables.number_text(value, variables.control_places(self.content))

    def label_html(self) -> str:
        from ..renderers.viewer.blocks import escape_html
        label = self.content.get("label")
        if label:
            return f'<span class="lk-control-label">{escape_html(label)}</span>'
        return (f'<span class="lk-control-label lk-control-symbol">'
                f'{escape_html(self.content["var"])}</span>')

    def chars(self) -> int:
        """How wide the value box is, in characters: the longest of the range's
        ends and the default, and one more decimal for a value typed in."""
        var = self.variable()
        texts = [self.fmt(var["min"]), self.fmt(var["max"]), self.fmt(var["default"])]
        return max(len(text) for text in texts) + 1

    def box(self, extra: str = "") -> str:
        """The number box the value is shown and typed in."""
        from .. import variables
        from ..renderers.viewer.blocks import escape_html
        var = self.variable()
        step = variables.number_text(var["step"]) if var["step"] is not None else "any"
        name = escape_html(self.content.get("label") or var["name"])
        return (f'<input type="number" class="lk-control-value{extra}" '
                f'min="{variables.number_text(var["min"])}" '
                f'max="{variables.number_text(var["max"])}" step="{step}" '
                f'value="{self.fmt(var["default"])}" aria-label="{name}" '
                f'style="--lk-chars:{self.chars()}">')

    def head(self, extra: str = "") -> str:
        """The label, the sign and the value box."""
        return (f'<label class="lk-control-field">{self.label_html()}'
                f'<span class="lk-control-eq">=</span>{self.box(extra)}</label>')

    def reset(self) -> str:
        """The button that puts the variable back to its default.

        The theme shows it only once ``vars.js`` has armed the control, so a
        page without script (the PDF) carries no button that does nothing.
        """
        from .. import i18n
        from ..renderers.viewer.blocks import escape_html
        title = escape_html(i18n.ui(self.content.get("lang"), "control_reset").format(
            value=self.fmt(self.variable()["default"])))
        return (f'<button type="button" class="lk-control-reset" title="{title}" '
                f'aria-label="{title}">↺</button>')

    def opening(self, kind: str) -> str:
        from .. import variables
        return (f'<div class="lk-control lk-{kind}" data-lk-var="{self.content["var"]}" '
                f'data-lk-places="{variables.control_places(self.content)}">')

    def footnote_marker(self, lines, numbers):
        # The control is one line of HTML; the superscript goes inside it, after
        # the last part, so it claims no line of its own.
        from ..renderers.viewer.blocks import last_content_index, marker_html
        lines = list(lines)
        index = last_content_index(lines)
        head = lines[index][: -len("</div>")]
        lines[index] = f"{head}{marker_html(numbers)}</div>"
        return lines

    # --- the targets that cannot move it: one line of text ---

    def to_pptx(self, ctx):
        from pptx.enum.text import PP_ALIGN
        from .. import variables
        from ..renderers.pptx import theme
        from ..renderers.pptx.blocks import GAP, apply_runs, set_line_spacing, textbox
        from ..renderers.pptx.layout import estimate_text_height
        from ..renderers.pptx.text import parse_inline
        line = variables.control_text(self.content)
        height = estimate_text_height(line, font_pt=theme.BODY_PT,
                                      width=ctx.layout.content_width)
        box, tf = textbox(ctx, height)
        paragraph = tf.paragraphs[0]
        paragraph.alignment = PP_ALIGN.CENTER
        set_line_spacing(paragraph, theme.BODY_PT)
        apply_runs(paragraph, parse_inline(line), size_pt=theme.BODY_PT, color=theme.DK2)
        ctx.cursor.place(height)
        ctx.cursor.advance(GAP)

    def to_latex(self, ctx):
        from .. import variables
        from ..renderers.latex.blocks import footnotes
        from ..renderers.latex.text import inline
        line = inline(variables.control_text(self.content), refs=ctx.resolve_ref)
        return "\\begin{center}\n%s%s\n\\end{center}" % (line, footnotes(self, ctx))

    def to_transcript(self, embedder):
        from .. import variables
        from ..renderers.transcript.text import inline
        return f'<p class="tx-control">{inline(variables.control_text(self.content))}</p>'


@element
class Slider(Control):
    """A slider over a variable's range, with marks under it.

    The value beside it is a box: dragging the thumb fills it in, and a value
    typed there is set as typed (on the variable's grid, if it has a step).
    """
    KIND = "slider"

    def to_marp(self, ctx):
        from .. import variables
        from ..renderers.viewer.blocks import escape_html
        var = self.variable()
        step = self.content["step_used"]
        marks = self.content.get("marks") or [var["min"], var["max"]]
        span = var["max"] - var["min"]
        ticks = "".join(
            f'<span class="lk-slider-mark" data-lk-value="{variables.number_text(mark)}" '
            f'style="--lk-at:{(mark - var["min"]) / span:.4f}">'
            f'{variables.number_text(mark)}</span>'
            for mark in marks
        )
        label = escape_html(self.content.get("label") or var["name"])
        html = (
            f'{self.opening("slider")}{self.head(" lk-slider-value")}'
            '<span class="lk-slider-track">'
            f'<input type="range" min="{variables.number_text(var["min"])}" '
            f'max="{variables.number_text(var["max"])}" '
            f'step="{variables.number_text(step)}" '
            f'value="{variables.number_text(var["default"])}" aria-label="{label}">'
            f'<span class="lk-slider-marks">{ticks}</span>'
            '</span>'
            f'{self.reset()}'
            '</div>'
        )
        return [html, ""]


@element
class NumberInput(Control):
    """An input box for a variable's value, with its range beside it.

    Any value in the range is taken as typed, or the nearest on the
    variable's grid when it has a step.
    """
    KIND = "input"

    def to_marp(self, ctx):
        from .. import variables
        var = self.variable()
        html = (
            f'{self.opening("input")}{self.head()}'
            f'<span class="lk-control-range">{variables.number_text(var["min"])} – '
            f'{variables.number_text(var["max"])}</span>'
            f'{self.reset()}'
            '</div>'
        )
        return [html, ""]
