"""Code languages that are another language plus a few words.

`p.code("cuda", ...)` is C++ with a dozen extra words: the qualifiers that
place a function or a variable (``__global__``, ``__shared__``), the variables
every thread is given (``threadIdx``, ``blockIdx``), ``__syncthreads``, and the
vector types. No highlighter a renderer uses knows the name, and asking for
``cpp`` instead leaves exactly those words, the ones the page is about, in the
plain ink.

A dialect is therefore data, not a lexer: the language it extends and the words
it adds, by the role a highlighter colours them in. ``dialects.json`` holds
them, one entry per dialect:

    hljs       the highlight.js language it extends (the deck: Marp highlights
               fenced code with highlight.js)
    listings   the same language under its LaTeX `listings` name (the book)
    keyword    words coloured as keywords
    built_in   names the language provides: variables and functions
    type       type names

The file is JSON because two programs read it: Marp's engine
(``renderers/viewer/marp-engine.cjs``) registers each entry with highlight.js,
and the book's preamble defines each as a `listings` language. The word lists
of ``cuda`` are those of Pygments' CUDA lexer, which colours a ``.cu`` file in
the source panel, so a listing on the slide and the file beside it agree.

The base language must keep its keywords as lists by role, as highlight.js's
``c`` and ``cpp`` do. PowerPoint and the transcript sheet print every language
in one ink, a dialect included.
"""

from __future__ import annotations

import json
from pathlib import Path

#: Read by `marp-engine.cjs` as well; keep it plain JSON.
FILE = Path(__file__).resolve().parent / "dialects.json"

#: The roles a dialect may add words to; highlight.js's names for them.
ROLES = ("keyword", "built_in", "type")

DIALECTS: dict[str, dict] = json.loads(FILE.read_text(encoding="utf-8"))


def words(name: str) -> list[str]:
    """Every word the dialect ``name`` adds, in role order."""
    dialect = DIALECTS[name]
    return [word for role in ROLES for word in dialect.get(role, ())]
