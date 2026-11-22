"""HTML into a small document tree that knows no target.

A document is a list of blocks:

* ``Paragraph``, a line of inlines, perhaps a heading;
* ``List``, whose items are documents;
* ``Table``, whose cells are documents;
* ``Code``, inlines in which a newline is a line;
* ``Quote``, a document;
* ``Columns``, documents side by side.

An inline is ``Text``, a ``Break``, a ``Formula`` (its MathML), an ``Image``,
or a ``Span``: inlines under one meaning (strong, code, a link, a mark, ...).
That is the whole set. Markdown and the HTML an author writes both arrive as
HTML, so both take this one path. A tag maps by what it means, a tag without a
meaning here is transparent, and text is never lost with it.
"""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from collections.abc import Iterator
from dataclasses import dataclass
from html.parser import HTMLParser

from lecturekit import marks


@dataclass
class Text:
    text: str


@dataclass
class Break:
    """A line break."""


@dataclass
class Formula:
    math: ET.Element            # the <math> element
    display: bool


@dataclass
class Image:
    src: str
    alt: str = ""
    width: str | None = None    # CSS lengths
    height: str | None = None


@dataclass
class Span:
    kind: str                   # one of SPANS
    children: list[Inline]
    value: str | None = None    # a link's address, a mark's tone


SPANS = frozenset({
    "strong", "em", "code", "strike", "underline", "sup", "sub", "small", "mark", "link",
})


@dataclass
class Paragraph:
    children: list[Inline]
    heading: int = 0            # 1 to 6, or 0 for text
    align: str | None = None


@dataclass
class List:
    items: list[list[Node]]
    ordered: bool = False
    start: int = 1


@dataclass
class Cell:
    blocks: list[Node]
    header: bool = False
    align: str | None = None


@dataclass
class Table:
    rows: list[list[Cell]]


@dataclass
class Code:
    children: list[Inline]


@dataclass
class Quote:
    blocks: list[Node]


@dataclass
class Columns:
    items: list[list[Node]]


Inline = Text | Break | Formula | Image | Span
Node = Paragraph | List | Table | Code | Quote | Columns


def read(html: str) -> list[Node]:
    """The document of an HTML fragment."""
    tree = _Tree()
    tree.feed(html)
    tree.close()
    return _blocks(tree.root, _Context())


def phrasing(blocks: list[Node]) -> list[Inline]:
    """The inlines of a document, one line for each paragraph.

    For a place that holds text and nothing else: a caption, a title.
    """
    out: list[Inline] = []
    for block in blocks:
        if out:
            out.append(Break())
        match block:
            case Paragraph(children) | Code(children):
                out.extend(children)
            case Quote(inner):
                out.extend(phrasing(inner))
            case List(items) | Columns(items):
                out.extend(phrasing([node for item in items for node in item]))
            case Table(rows):
                out.extend(phrasing([n for row in rows for cell in row for n in cell.blocks]))
    return out


# --- HTML into elements ---

VOID = frozenset("area base br col embed hr img input link meta source track wbr".split())
BLOCK = frozenset(
    "address article aside blockquote center dd details div dl dt figcaption figure "
    "footer h1 h2 h3 h4 h5 h6 header hr li main nav ol p pre section summary table "
    "tbody td tfoot th thead tr ul".split()
)
# An open element of the key ends where one of the value begins.
ENDED_BY = {
    "p": BLOCK,
    "li": {"li"},
    "dt": {"dt", "dd"},
    "dd": {"dt", "dd"},
    "td": {"td", "th", "tr", "tbody", "tfoot"},
    "th": {"td", "th", "tr", "tbody", "tfoot"},
    "tr": {"tr", "tbody", "tfoot"},
    "thead": {"tbody", "tfoot"},
    "tbody": {"tbody", "tfoot"},
}


class _Tree(HTMLParser):
    """As forgiving as a browser about the tags authors leave open."""

    def __init__(self) -> None:
        super().__init__()
        self.root = ET.Element("root")
        self.open = [self.root]

    def handle_starttag(self, tag, attrs):
        while tag in ENDED_BY.get(self.open[-1].tag, ()):
            self.open.pop()
        element = ET.SubElement(self.open[-1], tag, {k: v or "" for k, v in attrs})
        if tag not in VOID:
            self.open.append(element)

    def handle_endtag(self, tag):
        for at in range(len(self.open) - 1, 0, -1):
            if self.open[at].tag == tag:
                del self.open[at:]
                return

    def handle_data(self, data):
        parent = self.open[-1]
        if len(parent):
            parent[-1].tail = (parent[-1].tail or "") + data
        else:
            parent.text = (parent.text or "") + data


# --- elements into the document ---

DROPPED = frozenset({"script", "style", "template", "title"})
SPAN_OF_TAG = {
    "strong": "strong", "b": "strong", "em": "em", "i": "em",
    "code": "code", "kbd": "code", "samp": "code", "tt": "code",
    "s": "strike", "del": "strike", "strike": "strike",
    "u": "underline", "ins": "underline",
    "sup": "sup", "sub": "sub", "small": "small",
}
# The one class of lecturekit's own markup that is a meaning (markup.fields).
SPAN_OF_CLASS = {"title-parenthetical": "underline"}
HEADING = {f"h{level}": level for level in range(1, 7)}
BLOCK_DISPLAY = frozenset({"block", "flow-root", "flex", "grid", "list-item", "table"})
PRESERVED = frozenset({"pre", "pre-wrap", "break-spaces"})
ALIGNS = frozenset({"left", "center", "right"})
REPEAT = re.compile(r"repeat\(\s*(\d+)\s*,((?:[^()]|\([^()]*\))*)\)")
SPACE = re.compile(r"[ \t\n\r\f]+")


@dataclass(frozen=True)
class _Context:
    """What an element inherits."""
    code: bool = False          # inside a <pre>
    preserved: bool = False     # white space is content
    align: str | None = None


def _style(element: ET.Element) -> dict[str, str]:
    out = {}
    for declaration in element.get("style", "").split(";"):
        name, colon, value = declaration.partition(":")
        if colon:
            out[name.strip().lower()] = value.replace("!important", "").strip()
    return out


def _within(element: ET.Element, context: _Context) -> _Context:
    """The context of ``element``'s content."""
    style = _style(element)
    code = context.code or element.tag == "pre"
    preserved = context.preserved or code
    if "white-space" in style:
        preserved = style["white-space"] in PRESERVED
    align = style.get("text-align", element.get("align"))
    if align not in ALIGNS:
        align = "center" if element.tag == "center" else context.align
    return _Context(code, preserved, align)


def _content(element: ET.Element) -> Iterator[str | ET.Element]:
    """What of ``element`` is shown, in order."""
    if element.text:
        yield element.text
    for child in element:
        if not _hidden(child):
            yield child
        if child.tail:
            yield child.tail


def _hidden(element: ET.Element) -> bool:
    return (element.tag in DROPPED or "hidden" in element.attrib
            or _style(element).get("display") == "none")


def _is_block(element: ET.Element) -> bool:
    if element.tag == "math":
        return False
    display = _style(element).get("display")
    if display in BLOCK_DISPLAY:
        return True
    if display is None and element.tag in BLOCK:
        return True
    # An inline element around blocks is laid out as blocks.
    return any(_is_block(child) for child in element if not _hidden(child))


def _blocks(element: ET.Element, context: _Context) -> list[Node]:
    out: list[Node] = []
    line: list[Inline] = []

    def end_line() -> None:
        out.extend(_paragraph(line, context))
        line.clear()

    for node in _content(element):
        if isinstance(node, str):
            line.extend(_text(node, context))
        elif _is_block(node):
            end_line()
            out.extend(_block(node, _within(node, context)))
        else:
            line.extend(_inline(node, context))
    end_line()
    return out


def _paragraph(line: list[Inline], context: _Context) -> list[Node]:
    """A line as a paragraph, if anything of it shows."""
    children = _tidy(line)
    return [Paragraph(children, align=context.align)] if children else []


def _block(element: ET.Element, context: _Context) -> list[Node]:
    tag, style = element.tag, _style(element)
    if tag == "img":            # a picture on a line of its own
        return [Paragraph(_inline(element, context), align=context.align)]
    if tag == "pre":
        return [Code(_code(element, context))]
    if tag in ("ul", "ol"):
        return _list(element, context)
    if tag == "table":
        return _table(element, context)
    if tag == "blockquote":
        return [Quote(_blocks(element, context))]
    display = style.get("display", "")
    if "grid" in display or (
        "flex" in display and not style.get("flex-direction", "row").startswith("column")
    ):
        return _side_by_side(element, style, context)
    blocks = _blocks(element, context)
    spans = _spans(element, context)
    for block in blocks:
        if isinstance(block, Paragraph):
            block.heading = HEADING.get(tag, block.heading)
            if spans:
                block.children = _spanned(block.children, spans)
    return blocks


def _inline(element: ET.Element, context: _Context) -> list[Inline]:
    if element.tag == "br":
        return [Text("\n") if context.code else Break()]
    if element.tag == "math":
        return [Formula(element, element.get("display") == "block")]
    if element.tag == "img":
        style = _style(element)
        return [Image(element.get("src", ""), element.get("alt", ""),
                      style.get("width", element.get("width")),
                      style.get("height", element.get("height")))]
    inside = _within(element, context)
    children: list[Inline] = []
    for node in _content(element):
        children.extend(_text(node, inside) if isinstance(node, str)
                        else _inline(node, inside))
    return _spanned(children, _spans(element, context)) if children else []


def _spans(element: ET.Element, context: _Context) -> list[tuple[str, str | None]]:
    """The meanings of an element, the outermost first."""
    found: list[tuple[str, str | None]] = []
    tag, style = element.tag, _style(element)
    classes = element.get("class", "").split()
    if tag == "a" and element.get("href"):
        found.append(("link", element.get("href")))
    if tag == "mark":
        found.append(("mark", classes[0] if classes else marks.DEFAULT_TONE))
    kinds = [SPAN_OF_TAG.get(tag), *(SPAN_OF_CLASS.get(name) for name in classes)]
    if style.get("font-weight") in ("bold", "bolder", "600", "700", "800", "900"):
        kinds.append("strong")
    if style.get("font-style") == "italic":
        kinds.append("em")
    decoration = style.get("text-decoration", "")
    if "line-through" in decoration:
        kinds.append("strike")
    if "underline" in decoration:
        kinds.append("underline")
    for kind in kinds:
        # In a <pre> everything is code already.
        if kind and not (kind == "code" and context.code):
            found.append((kind, None))
    return found


def _spanned(children: list[Inline], spans: list[tuple[str, str | None]]) -> list[Inline]:
    for kind, value in reversed(spans):
        children = [Span(kind, children, value)]
    return children


def _text(text: str, context: _Context) -> list[Inline]:
    if context.code:
        return [Text(text)]
    if not context.preserved:
        return [Text(SPACE.sub(" ", text))]
    out: list[Inline] = []
    for number, line in enumerate(text.split("\n")):
        if number:
            out.append(Break())
        if line:
            out.append(Text(line.replace(" ", "\u00a0")))
    return out


def _code(element: ET.Element, context: _Context) -> list[Inline]:
    """The content of a <pre>, less the newline a browser drops at either end."""
    children: list[Inline] = []
    for node in _content(element):
        if isinstance(node, str):
            children.extend(_text(node, context))
        else:
            children.extend(_inline(node, context))
    leaves = [node for node in _leaves(children) if isinstance(node, Text) and node.text]
    if leaves:
        leaves[0].text = leaves[0].text.removeprefix("\n")
        leaves[-1].text = leaves[-1].text.removesuffix("\n")
    return children


def _list(element: ET.Element, context: _Context) -> list[Node]:
    items: list[list[Node]] = []
    for node in _content(element):
        if isinstance(node, str):
            if node.strip():
                items.append([Paragraph([Text(node.strip())])])
        elif node.tag == "li":
            items.append(_block(node, _within(node, context)))
        else:  # a list straight inside a list hangs under the item before it
            blocks = _block(node, _within(node, context))
            if items:
                items[-1].extend(blocks)
            elif blocks:
                items.append(blocks)
    if not items:
        return []
    ordered = element.tag == "ol"
    start = element.get("start", "1").strip()
    return [List(items, ordered, int(start) if start.lstrip("-").isdigit() else 1)]


def _table(element: ET.Element, context: _Context) -> list[Node]:
    above: list[Node] = []
    rows: list[list[Cell]] = []

    def read_rows(parent: ET.Element) -> None:
        for node in _content(parent):
            if isinstance(node, str):
                continue
            if node.tag in ("thead", "tbody", "tfoot"):
                read_rows(node)
            elif node.tag == "tr":
                cells = []
                for cell in _content(node):
                    if not isinstance(cell, str):
                        inside = _within(cell, context)
                        cells.append(Cell(_block(cell, inside), cell.tag == "th", inside.align))
                rows.append(cells)
            elif node.tag == "caption":
                above.extend(_blocks(node, _within(node, context)))

    read_rows(element)
    return [*above, Table(rows)] if any(rows) else above


def _side_by_side(element: ET.Element, style: dict[str, str],
                  context: _Context) -> list[Node]:
    """A flex row or a grid: one ``Columns`` for each row it draws."""
    grid = "grid" in style.get("display", "")
    across = _tracks(style.get("grid-template-columns", "")) if grid else None
    rows: list[list[list[Node]]] = [[]]
    for node in _content(element):
        whole = False
        if isinstance(node, str):
            blocks = _paragraph(_text(node, context), context)
        else:
            inside = _within(node, context)
            blocks = (_block(node, inside) if _is_block(node)
                      else _paragraph(_inline(node, context), inside))
            whole = grid and _style(node).get("grid-column", "").replace(" ", "") == "1/-1"
        if not blocks:
            continue
        if whole:
            rows.extend(([blocks], []))
        else:
            if across and len(rows[-1]) >= across:
                rows.append([])
            rows[-1].append(blocks)
    out: list[Node] = []
    for row in rows:
        if len(row) == 1:
            out.extend(row[0])
        elif row:
            out.append(Columns(row))
    return out


def _tracks(template: str) -> int:
    """How many columns a ``grid-template-columns`` names."""
    written_out = REPEAT.sub(lambda found: f" {found[2]} " * int(found[1]), template)
    # A track's own arguments, as in minmax(0, 1fr), are not tracks.
    return len(re.sub(r"\([^()]*\)", "", written_out).split()) or 1


def _leaves(inlines: list[Inline]) -> Iterator[Inline]:
    for node in inlines:
        if isinstance(node, Span):
            yield from _leaves(node.children)
        else:
            yield node


def _tidy(inlines: list[Inline]) -> list[Inline]:
    """One paragraph with its spaces collapsed as a browser shows them, or
    nothing when nothing of it shows."""
    spaced = True               # at a line's start, a space is not shown

    def walk(nodes: list[Inline]) -> list[Inline]:
        nonlocal spaced
        out: list[Inline] = []
        for node in nodes:
            if isinstance(node, Span):
                children = walk(node.children)
                if children:
                    out.append(Span(node.kind, children, node.value))
            elif isinstance(node, Text):
                text = node.text.lstrip(" ") if spaced else node.text
                if text:
                    out.append(Text(text))
                    spaced = text.endswith(" ")
            else:
                out.append(node)
                spaced = isinstance(node, Break)
        return out

    out = walk(inlines)
    while out and isinstance(out[-1], Break):
        out.pop()
    leaves = list(_leaves(out))
    if leaves and isinstance(leaves[-1], Text):
        leaves[-1].text = leaves[-1].text.rstrip(" ")
    shown = any(not isinstance(node, Break) and (not isinstance(node, Text) or node.text)
                for node in leaves)
    return out if shown else []
