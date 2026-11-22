"""What the deck's parser is given for each field of the DSL.

The deck puts some fields on a markdown line (slide text, a table cell, a
title) and others inside raw HTML, which Marp does not parse (a caption, a
footnote, a sidenote). These functions say which markup a field becomes. The
viewer writes it into the deck, and :class:`lecturekit.markup.Reader` parses
the same markup for the other targets, so a field reads the same in each.
"""

from __future__ import annotations

import re

from lecturekit import filelinks


def escape_html(text: str) -> str:
    """Escape ``text`` for HTML text and for a double-quoted attribute."""
    return (
        str(text)
        .replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace('"', "&quot;")
    )


# Inline HTML tags allowed verbatim inside a sidenote body. Everything is
# escaped first; these are reintroduced afterwards so untrusted-looking input
# can't smuggle in arbitrary markup.
INLINE_TAGS = "u|b|i|em|strong|code|sup|sub|br"
TAG_RE = re.compile(rf"&lt;(/?(?:{INLINE_TAGS})\s*/?)&gt;")
CODE_RE = re.compile(r"`([^`]+)`")
LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\s]+)\)")
BOLD_RE = re.compile(r"\*\*(.+?)\*\*")
ITALIC_RE = re.compile(r"\*([^*]+?)\*")


def inline_md(text: str) -> str:
    """A field set in raw HTML: a small subset of inline markdown, as HTML.

    Marp does not parse markdown within raw HTML blocks, where a caption, a
    footnote, an annotation and a sidenote's body are written. Escape first,
    then reintroduce whitelisted inline tags, ``**bold**``/``*italic*``/
    ``` `code` ```, ``[label](url)`` links, and turn newlines into line breaks.
    """
    html = escape_html(text)
    html = TAG_RE.sub(r"<\1>", html)
    html = CODE_RE.sub(r"<code>\1</code>", html)
    html = LINK_RE.sub(
        r'<a href="\2" target="_blank" rel="noopener noreferrer">\1</a>', html
    )
    html = BOLD_RE.sub(r"<strong>\1</strong>", html)
    html = ITALIC_RE.sub(r"<em>\1</em>", html)
    return html.replace("\n", "<br>")


# The characters markdown still reads as markup in text that is already HTML.
MD_INERT = {"*": "&#42;", "_": "&#95;", "`": "&#96;", "\\": "&#92;"}
MD_ACTIVE_RE = re.compile("[*_`\\\\]")


def inert_md(html: str) -> str:
    """Neutralize what markdown would still read as markup in rendered HTML.

    For HTML that is emitted on a markdown line rather than as a raw HTML block
    — the math form of the chip, a file button in a sentence — where markdown-it
    reads the text a second time, and a stray `_` in it is not emphasis the
    author asked for. Entities are the safe spelling — they survive inside an
    attribute (a link's href) as well as in text.
    """
    return MD_ACTIVE_RE.sub(lambda m: MD_INERT[m.group()], html)


def file_button(label: str, path: str) -> str:
    """One ``[label](file:path)`` as the button the live preview arms.

    The markup of a demo block's file button, for the same reasons: an id and
    not a path, and ``disabled`` until ``source.js`` has a server behind it. It
    sits in a line markdown-it still parses, so its label is rendered here and
    made inert on the way out.
    """
    from lecturekit import source as source_module

    return (
        '<button class="lk-text-file" type="button" disabled'
        f' data-lk-source="{source_module.source_id(path)}"'
        f' aria-label="Show {inert_md(escape_html(path))}">'
        f"{inert_md(inline_md(label))}</button>"
    )


def slide_text(text: str) -> str:
    """Slide text as markdown: each file link drawn as its button."""
    return filelinks.replace(str(text).strip(), file_button)


def table_cell(text: str) -> str:
    # Keep cell markdown intact for Marp; only neutralize the two characters
    # that would break the single-line row syntax.
    return (
        filelinks.replace(str(text), file_button).replace("|", "\\|").replace("\n", " ")
    )


_TITLE_MARK_RE = re.compile(r"\\emph\{([^{}]+)\}")


def title_html(text: str) -> str:
    """A title: plain text, with ``\\emph{...}`` as its underlined parenthetical."""
    parts: list[str] = []
    last = 0
    for match in _TITLE_MARK_RE.finditer(text):
        parts.append(escape_html(text[last:match.start()]))
        parts.append(
            '<span class="title-parenthetical">'
            f"{escape_html(match.group(1))}</span>"
        )
        last = match.end()
    parts.append(escape_html(text[last:]))
    return "".join(parts)
