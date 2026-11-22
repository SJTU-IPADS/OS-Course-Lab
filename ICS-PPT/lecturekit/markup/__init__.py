"""The one parse of slide text.

    text --fields--> markdown --engine--> HTML + MathML --document--> tree

``fields`` is what the deck's parser is given for each field, ``engine`` is
that parser (``filter.cjs``, a filter around the deck's own engine), and
``document`` is the tree a target draws. Nothing here knows a target.
"""

from __future__ import annotations

from . import document, fields
from .engine import Engine


class Reader:
    """Reads each kind of field into the document tree. Close it after a render."""

    def __init__(self) -> None:
        self._engine = Engine()

    def __enter__(self) -> Reader:
        return self

    def __exit__(self, *exc) -> None:
        self._engine.close()

    def markdown(self, text: str) -> list[document.Node]:
        """What the deck puts on markdown lines."""
        return document.read(self._engine.render(text))

    def html(self, fragment: str) -> list[document.Node]:
        """What the deck puts in raw HTML, which Marp leaves as it is."""
        return document.read(fragment)

    def slide(self, text: str) -> list[document.Node]:
        """Slide text, or one column of it."""
        return self.markdown(fields.slide_text(text))

    def inline(self, text: str) -> list[document.Inline]:
        """A caption, footnote, annotation or sidenote body."""
        return document.phrasing(self.html(fields.inline_md(text)))

    def plain(self, text: str) -> list[document.Inline]:
        """A field the deck prints as written: an author, a label, a bridge line."""
        return document.phrasing(self.html(fields.escape_html(text)))

    def title(self, text: str) -> list[document.Inline]:
        """A content page's title."""
        return document.phrasing(self.markdown(f"# {fields.title_html(text)}"))

    def cover_title(self, text: str) -> list[document.Inline]:
        """A cover's title, which the deck writes in raw HTML."""
        return document.phrasing(self.html(fields.title_html(text)))


__all__ = ["Reader", "document", "fields"]
