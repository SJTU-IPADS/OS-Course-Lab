"""The inline keyword highlight: `<mark>`.

A slide is a headline claim over supporting bullets, and `autobold` already
spends **bold** on the headline — so bold cannot also mark the one word inside a
line that the page is about (and the CJK face has no heavier cut to fall back
on). `<mark>` is that second, orthogonal gesture: a marker-pen wash under a
word, in the same four tones as the `p.highlight` chip, so a marked keyword and
a chip read as two scales of one thing.

    p.slide("Multi-Paxos: replicate a <mark>sequence</mark> of values")
    p.slide("The leader is <mark class=\"orange\">not</mark> the primary")

Marking a word is the commonest edit a slide gets, and thirteen characters of
tag around a four-character word is enough friction to skip it. So the same two
spellings have a shorthand — `==sequence==`, `==orange:not==` — which
:func:`expand` rewrites into the tag before anything downstream looks at the
text. The tag stays the one form the validator and the three renderers know.

It is deliberately legal in **`p.slide(...)` text only**. Elsewhere — a title, a
caption, a sidenote, prose — a highlight is either meaningless or the sign that
the text is too long to have a single point, so the tag is refused rather than
silently dropped. That refusal is the whole reason this module reports errors
instead of stripping: a tone typo must not render as a word that looks perfectly
normal, because the author would only find out on the projector.

This module knows nothing about the lecture model or any renderer. It owns the
vocabulary (the tag, the tones, the shorthand) and answers three questions:
*what does this text mean* (:func:`expand`), *is its use of `<mark>` legal*
(:func:`errors`), and *where are the marks* (:data:`SPAN_RE`, for the renderers
that must translate them).
"""

from __future__ import annotations

import re

# The four washes, shared with the `p.highlight` chip — one closed set, so a
# marked word and a chip on the same page cannot drift apart. The hex values
# live in the themes, not here.
TONE_ORDER = ("yellow", "orange", "green", "blue")
TONES = frozenset(TONE_ORDER)
DEFAULT_TONE = TONE_ORDER[0]

_TONE_ALT = "|".join(TONE_ORDER)

# Any mark-ish tag at all, well-formed or not. This is the net that catches the
# typos: whatever it finds must then match one of the two legal spellings below,
# so `<mark class='orange'>` and `<mark class="ornage">` are errors rather than
# text that quietly reaches the slide.
TAG_RE = re.compile(r"</?mark\b[^>]*>", re.IGNORECASE)

# The only legal opening spellings: bare (yellow) or one quoted tone, nothing
# else. One spelling keeps the docs one line long and the three renderers'
# parsers identical.
OPEN_RE = re.compile(rf'<mark(?: class="(?P<tone>{_TONE_ALT})")?>')
CLOSE = "</mark>"

# A whole mark, for the renderers that translate rather than validate. The body
# is non-greedy and nesting is refused, so this cannot swallow a later mark.
SPAN_RE = re.compile(
    rf'<mark(?: class="(?P<tone>{_TONE_ALT})")?>(?P<body>.*?)</mark>',
    re.DOTALL,
)

# Verbatim spans, blanked before scanning: a fenced block or a code span may
# legitimately *show* `<mark>` as the HTML it is. Blanked to spaces rather than
# deleted so the surrounding text keeps its shape.
_VERBATIM_RE = re.compile(r"```.*?```|~~~.*?~~~|`[^`\n]*`", re.DOTALL)

# The shorthand: `==keyword==`, or `==tone:keyword==` for one of the other three
# washes. The body may not span a line, may not contain `==`, and must begin and
# end with a non-space — which is what keeps an arithmetic `a == b == c` from
# reading as a mark, since a comparison always pads its operator with spaces.
_BODY_CHAR = r"(?:[^=\n]|=(?!=))"
SHORT_RE = re.compile(
    rf"==(?:(?P<tone>{_TONE_ALT}):)?"
    rf"(?P<body>[^\s=](?:{_BODY_CHAR}*?[^\s=])?)=="
)


def tone_of(match: re.Match) -> str:
    """The tone an :data:`OPEN_RE` / :data:`SPAN_RE` match carries."""
    return match.group("tone") or DEFAULT_TONE


def strip_verbatim(text: str) -> str:
    """Blank out code fences and code spans, so their contents are not scanned."""
    return _VERBATIM_RE.sub(lambda m: " " * len(m.group()), text)


def _expand_one(match: re.Match) -> str:
    tone = match.group("tone")
    open_tag = "<mark>" if tone is None else f'<mark class="{tone}">'
    return f"{open_tag}{match.group('body')}{CLOSE}"


def expand(text: str) -> str:
    """Rewrite every `==keyword==` shorthand into the `<mark>` tag it means.

    Called once, where slide text enters the model, so the rest of the system —
    :func:`errors`, `autobold`, all three renderers — keeps seeing exactly one
    spelling. Verbatim spans are left alone for the same reason they are not
    scanned: a code sample showing `==` means the characters, not a highlight.

    The shorthand is convenience, not a second dialect: nothing checks for a
    half-written one, because an unpaired `==` is ordinary text everywhere else
    in markdown and refusing it here would outlaw writing `==` at all.
    """
    if "==" not in text:
        return text
    out: list[str] = []
    last = 0
    for verbatim in _VERBATIM_RE.finditer(text):
        out.append(SHORT_RE.sub(_expand_one, text[last:verbatim.start()]))
        out.append(verbatim.group())
        last = verbatim.end()
    out.append(SHORT_RE.sub(_expand_one, text[last:]))
    return "".join(out)


def errors(text: str, *, allowed: bool) -> list[str]:
    """Everything wrong with this text's use of `<mark>`; empty when it is fine.

    ``allowed`` says whether `<mark>` may appear here at all — true for
    `p.slide(...)` text, false everywhere else. The caller turns each message
    into a ``ValidationError``; this module raises nothing itself so it stays
    usable from a linter, a test, or an editor.
    """
    scan = strip_verbatim(text)
    tags = list(TAG_RE.finditer(scan))
    if not tags:
        return []
    if not allowed:
        return ["<mark> is allowed in p.slide(...) text only"]

    problems: list[str] = []
    open_at: int | None = None
    for tag in tags:
        raw = tag.group()
        if raw.startswith("</"):
            if raw != CLOSE:
                problems.append(f"malformed closing tag {raw!r} (write {CLOSE})")
            elif open_at is None:
                problems.append(f"{CLOSE} with no <mark> before it")
            else:
                if not scan[open_at:tag.start()].strip():
                    problems.append("empty <mark></mark>")
                open_at = None
            continue
        if not OPEN_RE.fullmatch(raw):
            problems.append(
                f"bad <mark> tag {raw!r} (write <mark> or "
                f'<mark class="{_TONE_ALT}">)'
            )
            continue
        if open_at is not None:
            problems.append("nested <mark>")
            continue
        open_at = tag.end()
    if open_at is not None:
        problems.append(f"<mark> with no {CLOSE}")
    return problems
