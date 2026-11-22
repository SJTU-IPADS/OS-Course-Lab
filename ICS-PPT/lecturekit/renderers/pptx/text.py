"""The document tree as PowerPoint sets text: paragraphs of styled runs.

A PowerPoint text frame has paragraphs, runs and line breaks, and nothing
nested. :func:`flow` lays a document out as that, giving a list item its
marker, and leaves between the paragraphs what is a shape of its own there: a
table, a listing, pictures, columns, a quote. No text is parsed here.
"""

from __future__ import annotations

from dataclasses import dataclass, field, replace

from ...markup import document as doc
from . import omml

BULLET = "•  "
NUMBER = "{}.  "
BREAK = "\n"        # the text of a run that is a line break


@dataclass(frozen=True)
class Run:
    """One styled piece of a paragraph.

    A formula is a run too. It keeps its MathML in ``math``, and ``text`` is
    then its one-line rendering, which is what the layout measures.
    """

    text: str
    bold: bool = False
    italic: bool = False
    code: bool = False
    strike: bool = False
    underline: bool = False
    baseline: int = 0               # 1 superscript, -1 subscript
    small: bool = False
    link: str | None = None
    mark: str | None = None         # a tone; the colour is the theme's
    math: doc.Formula | None = None


@dataclass
class Para:
    runs: list[Run] = field(default_factory=list)
    marker: str = ""
    level: int | None = None        # the depth in a list, from 0
    heading: int = 0
    align: str | None = None

    @property
    def display(self) -> bool:
        """Whether the paragraph is one display formula and nothing else."""
        return (len(self.runs) == 1 and self.runs[0].math is not None
                and self.runs[0].math.display)


@dataclass
class Pictures:
    images: list[doc.Image]


Part = Para | Pictures | doc.Table | doc.Code | doc.Columns | doc.Quote

_STYLE = {
    "strong": {"bold": True}, "em": {"italic": True}, "code": {"code": True},
    "strike": {"strike": True}, "underline": {"underline": True},
    "sup": {"baseline": 1}, "sub": {"baseline": -1}, "small": {"small": True},
}


def pieces(inlines: list[doc.Inline], style: Run = Run("")) -> list[Run | doc.Image]:
    """Inlines as runs, with the pictures among them where they stand."""
    out: list[Run | doc.Image] = []
    for node in inlines:
        match node:
            case doc.Text(text):
                out.append(replace(style, text=text))
            case doc.Break():
                out.append(Run(BREAK))
            case doc.Formula(math):
                out.append(replace(style, text=omml.plain_text(math), math=node))
            case doc.Image():
                out.append(node)
            case doc.Span("link", children, value):
                out.extend(pieces(children, replace(style, link=value)))
            case doc.Span("mark", children, value):
                out.extend(pieces(children, replace(style, mark=value)))
            case doc.Span(kind, children):
                out.extend(pieces(children, replace(style, **_STYLE[kind])))
    return _merged(out)


def _merged(items: list[Run | doc.Image]) -> list[Run | doc.Image]:
    out: list[Run | doc.Image] = []
    for item in items:
        last = out[-1] if out else None
        if (isinstance(item, Run) and isinstance(last, Run)
                and item.math is None and BREAK not in (item.text, last.text)
                and replace(item, text="") == replace(last, text="")):
            out[-1] = replace(last, text=last.text + item.text)
        else:
            out.append(item)
    return out


def runs(inlines: list[doc.Inline]) -> list[Run]:
    """A field that holds text only (a title, a caption) as runs."""
    return [item for item in pieces(inlines) if isinstance(item, Run)]


def plain(items: list[Run]) -> str:
    return "".join(run.text for run in items)


def split_lines(items: list[Run]) -> list[list[Run]]:
    """Runs cut at their line breaks."""
    lines: list[list[Run]] = [[]]
    for run in items:
        for number, text in enumerate([run.text] if run.math else run.text.split("\n")):
            if number:
                lines.append([])
            if text:
                lines[-1].append(replace(run, text=text))
    return lines


def flow(blocks: list[doc.Node], level: int | None = None) -> list[Part]:
    """A document as paragraphs, and what stands between paragraphs."""
    out: list[Part] = []
    for block in blocks:
        match block:
            case doc.Paragraph():
                out.extend(_paragraph(block, level))
            case doc.List():
                out.extend(_list(block, 0 if level is None else level + 1))
            case _:
                out.append(block)
    return out


def _paragraph(block: doc.Paragraph, level: int | None) -> list[Part]:
    out: list[Part] = []
    pending: list[Run] = []

    def end_text() -> None:
        while pending and pending[0].text == BREAK:
            del pending[0]
        while pending and pending[-1].text == BREAK:
            del pending[-1]
        # The space beside a picture on the line is not text.
        if any(run.math or run.text.strip(" \n") for run in pending):
            if not pending[0].math:
                pending[0] = replace(pending[0], text=pending[0].text.lstrip(" "))
            if not pending[-1].math:
                pending[-1] = replace(pending[-1], text=pending[-1].text.rstrip(" "))
            out.append(Para(pending.copy(), level=level, heading=block.heading, align=block.align))
        pending.clear()

    for item in pieces(block.children):
        if isinstance(item, Run):
            pending.append(item)
            continue
        end_text()
        if out and isinstance(out[-1], Pictures):
            out[-1].images.append(item)
        else:
            out.append(Pictures([item]))
    end_text()
    return out


def _list(block: doc.List, level: int) -> list[Part]:
    out: list[Part] = []
    for number, item in enumerate(block.items, block.start):
        marker = NUMBER.format(number) if block.ordered else BULLET
        parts = flow(item, level)
        first = parts[0] if parts else None
        if isinstance(first, Para) and first.level == level and not first.display:
            first.marker = marker
        else:  # the item opens with something that is not its text
            out.append(Para(marker=marker, level=level))
        out.extend(parts)
    return out


def cell(blocks: list[doc.Node]) -> tuple[list[Para], list[doc.Image]]:
    """A table cell, which holds paragraphs only; its pictures are drawn after the table."""
    paras: list[Para] = []
    images: list[doc.Image] = []

    def add(parts: list[Part]) -> None:
        for part in parts:
            match part:
                case Para():
                    paras.append(part)
                case Pictures(found):
                    images.extend(found)
                case doc.Code(children):
                    paras.extend(
                        Para([replace(run, code=True) for run in line])
                        for line in split_lines(runs(children))
                    )
                case doc.Table(rows):
                    for row in rows:
                        for inner in row:
                            add(flow(inner.blocks))
                case doc.Columns(items):
                    for item in items:
                        add(flow(item))
                case doc.Quote(inner):
                    add(flow(inner))

    add(flow(blocks))
    return paras, images
