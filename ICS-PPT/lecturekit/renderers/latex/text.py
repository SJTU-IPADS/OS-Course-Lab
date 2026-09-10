"""markdown → LaTeX, for the slice of markdown the DSL documents.

Deliberately not pandoc: no external binary, and only the inline vocabulary the
DSL promises (bold, italic, code, link, math) plus lists, paragraphs, and
fences.

Conversion is **protect → escape → convert → restore**. Math, code spans, and
link URLs are lifted out before escaping — math is TeX already, a code span's
body must escape but keep its markers, and an escaped URL would break `\\href`.
So `$x_1$` survives, `` `a_b` `` becomes ``\\texttt{a\\_b}``, and
`[a](http://x/a_b)` keeps its underscore in the URL while escaping the label.
"""

from __future__ import annotations

import re

from ... import marks

# Tone -> the \definecolor name of the marker ink `\lkmark` draws with, shared
# with the marked code line `blocks.py` washes with `\lkpshl` — a marked word and
# a marked line are the same gesture at two scales. The chip has its own set
# (`_HIGHLIGHT_COLORS` in blocks.py): a stroke is ink laid under a word, a chip
# is ink the words are set in, so those two cannot share a colour.
MARK_COLORS = {
    "yellow": "lkMarkYellow",
    "orange": "lkMarkOrange",
    "green": "lkMarkGreen",
    "blue": "lkMarkBlue",
}

# Backslash maps to a macro, so it must be substituted in the same single pass
# as the others — a second pass would re-escape the backslashes it just wrote.
_SPECIALS = {
    "\\": r"\textbackslash{}",
    "^": r"\textasciicircum{}",
    "~": r"\textasciitilde{}",
    "&": r"\&",
    "%": r"\%",
    "$": r"\$",
    "#": r"\#",
    "_": r"\_",
    "{": r"\{",
    "}": r"\}",
}
_SPECIAL_RE = re.compile("|".join(re.escape(char) for char in _SPECIALS))

# A placeholder that survives `escape`: NUL, letters, digits. No LaTeX specials.
_SENTINEL = "\x00LKPH{}\x00"
_SENTINEL_RE = re.compile("\x00LKPH(\\d+)\x00")

# Spans lifted before escaping. Order matters: display math before inline math,
# and a link before a figure ref so `[@x](url)` still reads as a link. A mark is
# lifted whole and its body converted recursively, so `**` around it still bolds
# (auto-bold wraps the entire headline, marks included) and markup inside it
# still converts.
_PROTECT_RE = re.compile(
    rf"(?P<mark>{marks.SPAN_RE.pattern})"
    r"|(?P<dmath>\$\$.+?\$\$)"
    r"|(?P<math>\$[^$]+?\$)"
    r"|(?P<code>`[^`]+?`)"
    r"|(?P<link>\[[^\]]+?\]\([^)]+?\))"
    r"|(?P<ref>\[@[A-Za-z0-9:_-]+\])",
    re.DOTALL,
)
_LINK_PARTS_RE = re.compile(r"\[(.+?)\]\((.+?)\)", re.DOTALL)

_BOLD_RE = re.compile(r"\*\*(.+?)\*\*", re.DOTALL)
_ITALIC_RE = re.compile(r"(?<!\*)\*(?!\*)(.+?)(?<!\*)\*(?!\*)", re.DOTALL)

_BULLET_RE = re.compile(r"^\s*[-*+]\s+(.*)$")
_NUMBERED_RE = re.compile(r"^\s*\d+[.)]\s+(.*)$")
_FENCE_RE = re.compile(r"^\s*(?:```|~~~)\s*(\w*)\s*$")


def escape(text: str) -> str:
    """Escape LaTeX specials in a literal span (no markdown, no math)."""
    return _SPECIAL_RE.sub(lambda m: _SPECIALS[m.group()], text)


def inline(text: str, refs=None) -> str:
    """One markdown line → LaTeX.

    ``refs``, when given, is a callable ``name -> LaTeX`` that resolves a
    figure reference token ``[@name]``. Without it the token is left as
    literal (escaped) text, so surfaces that never defined refs stay inert.
    """
    protected: list[str] = []

    def lift(match: re.Match) -> str:
        raw = match.group()
        kind = "mark" if match.group("mark") is not None else match.lastgroup
        if kind == "mark":
            rendered = r"\lkmark{%s}{%s}" % (
                MARK_COLORS[marks.tone_of(match)],
                inline(match.group("body"), refs=refs),
            )
        elif kind == "code":
            rendered = r"\texttt{%s}" % escape(raw[1:-1])
        elif kind == "link":
            label, url = _LINK_PARTS_RE.match(raw).groups()
            # The label is prose (escape it); the URL is a URL (leave it be).
            rendered = r"\href{%s}{%s}" % (url, inline(label, refs=refs))
        elif kind == "ref":
            rendered = refs(raw[2:-1]) if refs else escape(raw)
        else:
            rendered = raw  # math is already TeX
        protected.append(rendered)
        return _SENTINEL.format(len(protected) - 1)

    lifted = _PROTECT_RE.sub(lift, text)
    out = escape(lifted)
    out = _BOLD_RE.sub(lambda m: r"\textbf{%s}" % m.group(1), out)
    out = _ITALIC_RE.sub(lambda m: r"\textit{%s}" % m.group(1), out)
    return _SENTINEL_RE.sub(lambda m: protected[int(m.group(1))], out)


def blocks(text: str, refs=None) -> str:
    """A multi-line markdown body → LaTeX paragraphs, lists, and listings.

    ``refs`` resolves figure reference tokens exactly as in :func:`inline`.
    """
    chunks: list[str] = []
    paragraph: list[str] = []
    items: list[str] = []
    list_env: str | None = None
    fence_lang: str | None = None
    fence_body: list[str] = []

    def flush_paragraph() -> None:
        if paragraph:
            chunks.append("\n".join(inline(line, refs=refs) for line in paragraph))
            paragraph.clear()

    def flush_list() -> None:
        nonlocal list_env
        if list_env:
            body = "\n".join(r"\item %s" % inline(item, refs=refs) for item in items)
            chunks.append(
                "\\begin{%s}\n%s\n\\end{%s}" % (list_env, body, list_env)
            )
            items.clear()
            list_env = None

    for line in text.splitlines():
        fence = _FENCE_RE.match(line)
        if fence_lang is not None:
            # Inside a fence: the closing marker ends it, everything else is raw.
            if fence:
                lang = f"[language={fence_lang}]" if fence_lang else ""
                chunks.append(
                    "\\begin{lstlisting}%s\n%s\n\\end{lstlisting}"
                    % (lang, "\n".join(fence_body))
                )
                fence_lang, fence_body = None, []
            else:
                fence_body.append(line)
            continue
        if fence:
            flush_paragraph()
            flush_list()
            fence_lang = fence.group(1)
            continue

        stripped = line.strip()
        if not stripped:
            flush_paragraph()
            flush_list()
            continue

        # Display math owns its line and passes through untouched.
        if stripped.startswith("$$") and stripped.endswith("$$") and len(stripped) > 4:
            flush_paragraph()
            flush_list()
            chunks.append(stripped)
            continue

        bullet = _BULLET_RE.match(line)
        numbered = _NUMBERED_RE.match(line)
        if bullet or numbered:
            env = "itemize" if bullet else "enumerate"
            if list_env and list_env != env:
                flush_list()
            flush_paragraph()
            list_env = env
            items.append((bullet or numbered).group(1))
            continue

        flush_list()
        paragraph.append(stripped)

    if fence_lang is not None:  # unterminated fence: emit what we have
        lang = f"[language={fence_lang}]" if fence_lang else ""
        chunks.append(
            "\\begin{lstlisting}%s\n%s\n\\end{lstlisting}" % (lang, "\n".join(fence_body))
        )
    flush_paragraph()
    flush_list()
    return "\n\n".join(chunks)
