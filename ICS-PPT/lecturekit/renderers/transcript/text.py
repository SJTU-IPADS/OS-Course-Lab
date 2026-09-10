"""Markdown → HTML for the transcript sheet.

The viewer hands its markdown to Marp and the book hands it to a LaTeX
converter; the transcript writes one standalone HTML file with no build step
behind it, so it converts the DSL's markdown subset itself. Same vocabulary the
DSL documents: headings, bullet/ordered lists, paragraphs, fenced code, and
inline `**bold**` / `*italic*` / `` `code` `` / `[label](url)` / `$math$` /
`<mark>`.

Everything else is escaped, except the inline tags a sidenote body may carry
(`<u> <b> <i> <em> <strong> <code> <sup> <sub> <br>`) — those are restored after
escaping, which is what keeps a stray `<script>` in slide text inert.
"""

from __future__ import annotations

import html
import re

from lecturekit import marks

from .math import math_html

# The inline tags the DSL whitelists in sidenote/footnote/annotation bodies.
_ALLOWED_TAGS = ("u", "b", "i", "em", "strong", "code", "sup", "sub", "br")
_TAG_RE = re.compile(
    rf"&lt;(/?)({'|'.join(_ALLOWED_TAGS)})\s*(/?)&gt;", re.IGNORECASE
)

# An `<img>` written straight into a table cell — the icon-in-a-cell idiom the
# viewer renders because Marp passes cell markdown through. This target is HTML
# too, so it honors it; the src is embedded afterwards, in `renderer`.
_IMG_RE = re.compile(r"&lt;img\s+([^&<>]*?)/?&gt;", re.IGNORECASE)

# Longest delimiter first, and math first so a `*` or `_` inside a formula is
# never read as emphasis. Mirrors the pptx renderer's inline grammar.
_INLINE_RE = re.compile(
    rf"{marks.SPAN_RE.pattern}"                      # <mark>keyword</mark>
    r"|\$\$(?P<dmath>.+?)\$\$"                       # $$\frac{a}{b}$$
    r"|\$(?P<math>[^$\n]+)\$"                        # $x^2$
    r"|\[(?P<ltext>[^\]]+)\]\((?P<lurl>[^)\s]+)\)"   # [label](url)
    r"|\*\*(?P<bold>.+?)\*\*"                        # **bold**
    r"|\*(?P<italic>[^*]+?)\*"                       # *italic*
    r"|`(?P<code>[^`]+)`"                            # `code`
)

_HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")
_BULLET_RE = re.compile(r"^(\s*)[-*+]\s+(.*)$")
_ORDERED_RE = re.compile(r"^(\s*)\d+[.)]\s+(.*)$")
_FENCE_RE = re.compile(r"^\s*(```|~~~)")
_DISPLAY_MATH = "$$"


def escape(text: str) -> str:
    """HTML-escape, then let the whitelisted inline tags back through."""
    escaped = html.escape(text, quote=False)
    return _IMG_RE.sub(r"<img \1>", _TAG_RE.sub(r"<\1\2\3>", escaped))


def inline(text: str) -> str:
    """One line of the DSL's inline markdown as HTML."""
    out: list[str] = []
    pos = 0
    for match in _INLINE_RE.finditer(text):
        if match.start() > pos:
            out.append(escape(text[pos:match.start()]))
        if match.group("body") is not None:
            tone = marks.tone_of(match)
            css = "" if tone == "yellow" else f' class="{tone}"'
            out.append(f"<mark{css}>{inline(match.group('body'))}</mark>")
        elif match.group("dmath") is not None:
            # Display math written inside a line — a `p.highlight` row, say. The
            # sheet has one math style, so it renders as the inline one does.
            out.append(math_html(match.group("dmath").strip()))
        elif match.group("math") is not None:
            out.append(math_html(match.group("math")))
        elif match.group("ltext") is not None:
            url = html.escape(match.group("lurl"), quote=True)
            out.append(f'<a href="{url}">{inline(match.group("ltext"))}</a>')
        elif match.group("bold") is not None:
            out.append(f"<strong>{inline(match.group('bold'))}</strong>")
        elif match.group("italic") is not None:
            out.append(f"<em>{inline(match.group('italic'))}</em>")
        elif match.group("code") is not None:
            out.append(f"<code>{escape(match.group('code'))}</code>")
        pos = match.end()
    if pos < len(text):
        out.append(escape(text[pos:]))
    return "".join(out)


def markdown(md: str) -> str:
    """A markdown block (a `slide` / `aside` / sidenote body) as HTML."""
    lines = md.splitlines()
    out: list[str] = []
    stack: list[str] = []          # open list tags, outermost first
    paragraph: list[str] = []
    index = 0

    def close_paragraph() -> None:
        if paragraph:
            out.append(f"<p>{inline(' '.join(paragraph))}</p>")
            paragraph.clear()

    def close_lists(depth: int = 0) -> None:
        while len(stack) > depth:
            out.append(f"</{stack.pop()}>")

    def open_item(tag: str, level: int, body: str) -> None:
        close_paragraph()
        # One level per two spaces of indent, but never skipping a level: a
        # deeper jump than the list has open would leave an unclosed <ul>.
        level = min(level, len(stack))
        close_lists(level + 1)
        if len(stack) == level:
            out.append(f"<{tag}>")
            stack.append(tag)
        elif stack[level] != tag:
            # Same depth, other kind of list: <ul> and <ol> don't interleave.
            out.append(f"</{stack.pop()}>")
            out.append(f"<{tag}>")
            stack.append(tag)
        out.append(f"<li>{inline(body)}</li>")

    while index < len(lines):
        line = lines[index]
        index += 1

        if line.strip() == _DISPLAY_MATH:
            close_paragraph()
            close_lists()
            body: list[str] = []
            while index < len(lines) and lines[index].strip() != _DISPLAY_MATH:
                body.append(lines[index])
                index += 1
            index += 1  # the closing fence
            latex = " ".join(part.strip() for part in body if part.strip())
            if latex:
                out.append(f'<p class="tx-math">{math_html(latex)}</p>')
            continue

        fence = _FENCE_RE.match(line)
        if fence:
            close_paragraph()
            close_lists()
            marker = fence.group(1)
            body = []
            while index < len(lines) and not lines[index].strip().startswith(marker):
                body.append(lines[index])
                index += 1
            index += 1
            out.append(f'<pre class="tx-code">{escape(chr(10).join(body))}</pre>')
            continue

        if not line.strip():
            # A blank line ends a paragraph but not a list: slide bullets are
            # commonly written one blank line apart, and closing the <ul> here
            # would spell each of them as a list of its own.
            close_paragraph()
            continue

        heading = _HEADING_RE.match(line)
        if heading:
            close_paragraph()
            close_lists()
            out.append(f'<p class="tx-h">{inline(heading.group(2))}</p>')
            continue

        bullet = _BULLET_RE.match(line)
        if bullet:
            open_item("ul", len(bullet.group(1)) // 2, bullet.group(2))
            continue

        ordered = _ORDERED_RE.match(line)
        if ordered:
            open_item("ol", len(ordered.group(1)) // 2, ordered.group(2))
            continue

        close_lists()
        paragraph.append(line.strip())

    close_paragraph()
    close_lists()
    return "".join(out)
