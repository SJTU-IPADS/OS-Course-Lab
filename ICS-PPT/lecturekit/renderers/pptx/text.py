"""Markdown → structured paragraphs/runs for the PPTX renderer.

Marp parses the slide markdown in the viewer; the native PPTX renderer has no
markdown engine, so this module turns the small markdown subset the DSL emits
(headings, bullet/ordered lists, paragraphs, and inline bold/italic/code/links)
into plain data the block drawers map onto python-pptx paragraphs and runs.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

from ... import marks
from . import omml


@dataclass
class Run:
    """One styled span of text within a paragraph.

    A math run carries its LaTeX source in ``math``; ``text`` then holds a
    plain-text rendering of it, which is what the layout measures.
    """

    text: str
    bold: bool = False
    italic: bool = False
    code: bool = False
    link: str | None = None
    math: str | None = None
    # The wash a `<mark>` puts under this run, or None. A tone name, not a
    # colour: the palette belongs to the theme.
    mark: str | None = None


@dataclass
class Para:
    """One paragraph: a kind, a nesting/heading level, and styled runs."""

    kind: str  # "para" | "heading" | "bullet" | "ordered" | "math"
    runs: list[Run] = field(default_factory=list)
    level: int = 0


_HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")
_BULLET_RE = re.compile(r"^(\s*)[-*]\s+(.*)$")
_ORDERED_RE = re.compile(r"^(\s*)\d+\.\s+(.*)$")

# Inline spans, longest-delimiter-first so `**` wins over `*`. Math comes first
# so a `*` or `_` inside a formula is never read as markdown emphasis. A mark's
# body is parsed recursively, so emphasis and a wash compose either way round.
_INLINE_RE = re.compile(
    rf"{marks.SPAN_RE.pattern}"                     # <mark>keyword</mark>
    r"|\$\$(?P<dmath>.+?)\$\$"                      # $$\frac{a}{b}$$
    r"|\$(?P<math>[^$\n]+)\$"                       # $x^2$
    r"|\[(?P<ltext>[^\]]+)\]\((?P<lurl>[^)\s]+)\)"  # [label](url)
    r"|\*\*(?P<bold>.+?)\*\*"                       # **bold**
    r"|\*(?P<italic>[^*]+?)\*"                       # *italic*
    r"|`(?P<code>[^`]+)`"                            # `code`
)

_DISPLAY_MATH = "$$"


def parse_markdown(md: str) -> list[Para]:
    """Parse the DSL's markdown subset into a flat list of paragraphs."""
    paras: list[Para] = []
    lines = md.splitlines()
    index = 0
    while index < len(lines):
        line = lines[index]
        index += 1
        if line.strip() == _DISPLAY_MATH:
            # $$ … $$ — everything up to the closing fence is one formula, so a
            # multi-line display equation stays a single paragraph.
            body: list[str] = []
            while index < len(lines) and lines[index].strip() != _DISPLAY_MATH:
                body.append(lines[index])
                index += 1
            index += 1  # the closing fence
            latex = " ".join(part.strip() for part in body if part.strip())
            if latex:
                paras.append(Para("math", [Run(omml.plain_text(latex), math=latex)]))
            continue
        if not line.strip():
            continue
        heading = _HEADING_RE.match(line)
        if heading:
            level = len(heading.group(1))
            paras.append(Para("heading", parse_inline(heading.group(2)), level))
            continue
        bullet = _BULLET_RE.match(line)
        if bullet:
            level = len(bullet.group(1)) // 2
            paras.append(Para("bullet", parse_inline(bullet.group(2)), level))
            continue
        ordered = _ORDERED_RE.match(line)
        if ordered:
            level = len(ordered.group(1)) // 2
            paras.append(Para("ordered", parse_inline(ordered.group(2)), level))
            continue
        paras.append(Para("para", parse_inline(line.strip())))
    return paras


def emphasize(runs: list[Run], *, bold: bool = False, italic: bool = False) -> list[Run]:
    """Carry emphasis onto already-parsed runs.

    `**…**` is parsed recursively, because the DSL's auto-bold wraps a whole
    headline — including any `$math$` in it — in one pair of asterisks. An
    equation has no bold cut, so a math run just keeps its own styling.
    """
    for run in runs:
        if run.math is None:
            run.bold = run.bold or bold
            run.italic = run.italic or italic
    return runs


def carry_mark(runs: list[Run], tone: str) -> list[Run]:
    """Put a `<mark>`'s wash on every text run of its body.

    A math run is skipped for the same reason `emphasize` skips it: an equation
    leaves this renderer as a native OMML node carrying its own run properties,
    not as a text run this flag could reach.
    """
    for run in runs:
        if run.math is None:
            run.mark = run.mark or tone
    return runs


def parse_inline(text: str) -> list[Run]:
    """Split a line into styled runs (bold/italic/code/link + plain spans)."""
    runs: list[Run] = []
    pos = 0
    for match in _INLINE_RE.finditer(text):
        if match.start() > pos:
            runs.append(Run(text[pos:match.start()]))
        if match.group("body") is not None:
            runs.extend(carry_mark(parse_inline(match.group("body")),
                                   marks.tone_of(match)))
        elif match.group("dmath") is not None:
            # Display math written inside a line — a `p.highlight` row, say.
            # It is one formula like any other; only a `$$` on its own line
            # (above) makes a paragraph of it.
            latex = match.group("dmath").strip()
            runs.append(Run(omml.plain_text(latex), math=latex))
        elif match.group("math") is not None:
            latex = match.group("math")
            runs.append(Run(omml.plain_text(latex), math=latex))
        elif match.group("ltext") is not None:
            runs.append(Run(match.group("ltext"), link=match.group("lurl")))
        elif match.group("bold") is not None:
            runs.extend(emphasize(parse_inline(match.group("bold")), bold=True))
        elif match.group("italic") is not None:
            runs.extend(emphasize(parse_inline(match.group("italic")), italic=True))
        elif match.group("code") is not None:
            runs.append(Run(match.group("code"), code=True))
        pos = match.end()
    if pos < len(text):
        runs.append(Run(text[pos:]))
    return runs or [Run("")]
