"""The ``pseudo`` code language: a small lexer for lecture pseudocode.

Pseudocode belongs to no real language, so highlight.js has nothing useful to
say about it — ask it for `text` and you get a flat black wall. And the three
categories a syntax highlighter *does* know (keyword / string / comment) are
the wrong three: lecture pseudocode has no strings, few comments, and maybe two
control-flow words on a page.

What a reader needs coloured is what pseudocode is actually made of:

    keyword   a fixed, case-insensitive word list (``if``, ``else``, ``send``…)
    message   anything in angle brackets — ``<accept, N, V>``, what crosses
              the wire
    state     any name the block assigns to: ``Nh = N`` makes *every* ``Nh``
              in the block a state name, and ``read-set[d] = …`` names
              ``read-set``
    comment   ``#`` or ``//`` to end of line
    plain     everything else

The `state` rule is the point of this module. It is purely syntactic — it asks
the author for no annotation, no marker syntax, no list of variable names — yet
it lands on exactly the names a lecture wants to trace, because the state of a
protocol is by definition what its steps write to.

This module only decides what a token *is*. Emitting it is the renderer's job:
the viewer wraps tokens in ``<span>``, the book in colour macros.
"""

from __future__ import annotations

import re

#: Control-flow and message-passing words, matched case-insensitively.
#: Deliberately short: every entry here is a word that would be miscoloured if
#: it turned up as ordinary prose inside a step, so `and`, `or`, `do`, `on`,
#: `end` and friends are left out on purpose.
KEYWORDS = frozenset(
    """
    if else elif then while for foreach repeat until return break continue
    upon receive receives send reply broadcast wait null nil true false
    """.split()
)

# A name assigned to: a whole word, then nothing but spaces, then a `=` that is
# not `==`. That "nothing but spaces" is the whole trick — a comparison always
# puts an operator character where the `=` would have to be, so `V != null` and
# `N <= Nh` are excluded without a list of operators to keep in sync. The
# assignment need not start the line: `Leader chooses Mn = …` still names `Mn`.
#
# Two shapes of name, both of which pseudocode writes constantly:
#   * a hyphenated one — `read-set`, `write-set`. Lecture pseudocode names its
#     state the way the slide's prose does, and the prose says "read set".
#   * a subscripted or dotted target — `read-set[d] = …`, `bank[a] = …`,
#     `d.version = …`. Writing one field of a table is still writing the table,
#     so the name the reader wants traced is the one in front of the bracket.
_ASSIGN_RE = re.compile(
    r"(?<![\w.-])([A-Za-z_]\w*(?:-[A-Za-z_]\w*)*)"
    r"(?:\[[^\]\n]*\]|\.[A-Za-z_]\w*)*[ \t]*=(?!=)"
)

_TOKEN_PATTERN = r"""
      (?P<comment>(?:\#|//)[^\n]*)
    | (?P<message><[^<>\n]*>)
    | %s(?P<word>[A-Za-z_]\w*)
    | (?P<other>.)
    """

_TOKEN_RE = re.compile(_TOKEN_PATTERN % "", re.VERBOSE)


def _token_re(names: set[str]) -> re.Pattern[str]:
    """`_TOKEN_RE`, taught to read the hyphenated names in `names` as one word.

    A hyphen is an operator everywhere else in a step (`P-1`, `a-b`), so the
    lexer cannot simply admit it into an identifier. It does not have to: the
    only hyphenated names that matter are the ones the block assigns to, and
    those are known before lexing starts. Longest first, so `read-set-hint`
    wins over `read-set`.
    """
    hyphenated = sorted((n for n in names if "-" in n), key=len, reverse=True)
    if not hyphenated:
        return _TOKEN_RE
    alt = "|".join(re.escape(n) for n in hyphenated)
    return re.compile(
        _TOKEN_PATTERN % rf"(?P<hyphen>(?:{alt})(?![\w-])) | ", re.VERBOSE
    )


def state_names(text: str) -> set[str]:
    """The names `text` assigns to — its state, read off by pure syntax."""
    return {m.group(1) for m in _ASSIGN_RE.finditer(text)}


def tokenize(text: str) -> list[list[tuple[str, str]]]:
    """Lex `text` into lines, each a list of ``(kind, text)`` tokens.

    `kind` is one of `keyword`, `message`, `state`, `comment`, `plain`.
    Adjacent tokens of the same kind are coalesced, so an emitter wraps one
    run of plain text in one element rather than one per character.
    """
    names = state_names(text)
    token_re = _token_re(names)
    lines: list[list[tuple[str, str]]] = []
    for raw in text.split("\n"):
        toks: list[list[str]] = []
        for match in token_re.finditer(raw):
            kind, piece = match.lastgroup, match.group()
            if kind == "hyphen":
                kind = "state"
            elif kind == "word":
                if piece.lower() in KEYWORDS:
                    kind = "keyword"
                elif piece in names:
                    kind = "state"
                else:
                    kind = "plain"
            elif kind == "other":
                kind = "plain"
            if toks and toks[-1][0] == kind:
                toks[-1][1] += piece
            else:
                toks.append([kind, piece])
        lines.append([(kind, piece) for kind, piece in toks])
    return lines
