"""The document tree as the HTML of the transcript sheet.

No text is parsed here. Text is escaped and only the elements written below
come out, so markup an author wrote cannot act on the sheet. A formula is
written as MathML, which browsers typeset and print by themselves.
"""

from __future__ import annotations

import html
import re
from collections.abc import Callable

from lecturekit import marks
from lecturekit.markup import document as doc

from . import mathml

SPAN_TAGS = {
    "strong": "strong", "em": "em", "code": "code", "strike": "s",
    "underline": "u", "sup": "sup", "sub": "sub", "small": "small",
}
_LINKABLE = re.compile(r"(https?:|mailto:|#)", re.IGNORECASE)
_LENGTH = re.compile(r"[\d.]+(px|%|em|rem|pt)?")


def is_display(node: doc.Inline) -> bool:
    return isinstance(node, doc.Formula) and node.display


def escape(text: str) -> str:
    return html.escape(str(text), quote=False)


class HtmlWriter:
    """Writes documents. ``embed`` turns a picture's path into a ``data:`` URI."""

    def __init__(self, embed: Callable[[str], str | None]) -> None:
        self.embed = embed

    def inline(self, inlines: list[doc.Inline]) -> str:
        return "".join(self._inline(node) for node in inlines)

    def _inline(self, node: doc.Inline) -> str:
        match node:
            case doc.Text(text):
                return escape(text)
            case doc.Break():
                return "<br>"
            case doc.Formula(math):
                return mathml.write(math)
            case doc.Image():
                return self._image(node)
            case doc.Span("link", children, address):
                body = self.inline(children)
                if not _LINKABLE.match(address or ""):
                    return body
                return f'<a href="{html.escape(address, quote=True)}">{body}</a>'
            case doc.Span("mark", children, tone):
                other = tone in marks.TONES and tone != marks.DEFAULT_TONE
                css = f' class="{tone}"' if other else ""
                return f"<mark{css}>{self.inline(children)}</mark>"
            case doc.Span(kind, children):
                tag = SPAN_TAGS[kind]
                return f"<{tag}>{self.inline(children)}</{tag}>"
        raise TypeError(f"not an inline: {node!r}")

    def _image(self, image: doc.Image) -> str:
        uri = self.embed(image.src)
        if uri is None:
            return ""   # the row it labels still reads; no broken-image box
        styles = ["max-width:100%"]
        for name, value in (("width", image.width), ("height", image.height)):
            if value and _LENGTH.fullmatch(value):
                styles.append(f"{name}:{value}px" if value.isdigit() else f"{name}:{value}")
        return (f'<img src="{uri}" alt="{html.escape(image.alt, quote=True)}" '
                f'style="{";".join(styles)}">')

    def blocks(self, nodes: list[doc.Node]) -> str:
        return "".join(self._block(node) for node in nodes)

    def _block(self, node: doc.Node) -> str:
        match node:
            case doc.Paragraph(children, heading, align):
                if heading:
                    return f'<p class="tx-h">{self.inline(children)}</p>'
                if len(children) == 1 and is_display(children[0]):
                    return f'<p class="tx-math">{self.inline(children)}</p>'
                style = f' style="text-align:{align}"' if align in ("center", "right") else ""
                return f"<p{style}>{self.inline(children)}</p>"
            case doc.List(items, ordered, start):
                tag = "ol" if ordered else "ul"
                first = f' start="{start}"' if ordered and start != 1 else ""
                body = "".join(f"<li>{self._compact(item)}</li>" for item in items)
                return f"<{tag}{first}>{body}</{tag}>"
            case doc.Code(children):
                return f'<pre class="tx-code">{self.inline(children)}</pre>'
            case doc.Table():
                return self.table(node)
            case doc.Quote(inner):
                return f'<div class="tx-aside">{self.blocks(inner)}</div>'
            case doc.Columns(items):
                return "".join(self.blocks(item) for item in items)     # paper prints them in order
        raise TypeError(f"not a block: {node!r}")

    def _compact(self, nodes: list[doc.Node]) -> str:
        """A list item or a cell: its first line takes no paragraph of its own."""
        match nodes:
            case [doc.Paragraph(children, 0), *rest] if not any(map(is_display, children)):
                return self.inline(children) + self.blocks(rest)
        return self.blocks(nodes)

    def table(self, table: doc.Table, widths: list[float] | None = None) -> str:
        def row(cells: list[doc.Cell]) -> str:
            written = []
            for cell in cells:
                tag = "th" if cell.header else "td"
                written.append(f'<{tag} style="text-align:{cell.align or "left"}">'
                               f"{self._compact(cell.blocks)}</{tag}>")
            return f"<tr>{''.join(written)}</tr>"

        rows, head = table.rows, ""
        if rows and rows[0] and all(cell.header for cell in rows[0]):
            head, rows = f"<thead>{row(rows[0])}</thead>", rows[1:]
        body = f"<tbody>{''.join(row(cells) for cells in rows)}</tbody>"
        if widths is None:
            return f'<table class="tx-table">{head}{body}</table>'
        cols = "".join(f'<col style="width:{100 * width:.2f}%">' for width in widths)
        return ('<table class="tx-table" style="table-layout:fixed;width:100%">'
                f"<colgroup>{cols}</colgroup>{head}{body}</table>")
