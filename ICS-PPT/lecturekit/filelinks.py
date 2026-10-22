"""The file button in a sentence: ``[label](file:path)``.

``p.demo(..., files=[...])`` puts a source file one press away from the command
that works on it. A page that only *mentions* a file has no command to hang the
button on, and a block of its own costs the page a line. A link whose address
is ``file:`` followed by a path says the same thing where the sentence is:

    p.slide("`main` 的定义在 [main.c](file:examples/main.c) 中。")

The path is relative to the lecture's own directory, as an entry of ``files=``
is; one that begins with ``/`` is relative to the project root, the directory a
demo reaches as ``$LECTUREKIT_ROOT`` (see :func:`lecturekit.demo.project_root`),
so a lab lecture names its lab's sources in the directory beside it:

    p.slide("取指在 [seq.c](file:/lectures/mini-cpu/src/lab2/seq.c) 中。")

The live preview draws the label as a button that opens the file in the
panel (see :mod:`lecturekit.source`); every other target prints the label as
the text it is, since nothing can be pressed there.

It is legal in the text every renderer reads as inline markdown and lays out as
a line of the page: ``p.slide(...)``, ``p.columns(...)`` and the cells of
``p.table(...)``. Anywhere else — a title, a caption, a footnote, a sidenote —
it is refused rather than printed, because a button that arrives as its own
markup is found on the projector and not at build time.

This module knows nothing about the lecture model or any renderer, as
:mod:`lecturekit.marks` does not. It owns the spelling and answers three
questions: *where are the links* (:func:`find`), *is this text's use of them
legal* (:func:`errors`), and *what does the text become* (:func:`replace`, for
the renderer that draws buttons; :func:`path_of`, for the ones that parse
``[label](url)`` themselves).
"""

from __future__ import annotations

import re
from pathlib import PurePosixPath
from typing import Callable

from . import marks

#: What the address of a file link begins with.
SCHEME = "file:"

#: What a path relative to the project root begins with. Any other path is
#: relative to the lecture's own directory.
ROOT_PREFIX = "/"

# A whole link. The label may not span a line or hold a `]`; the path holds no
# space and no `)`, which is what a markdown link's address is anyway.
LINK_RE = re.compile(r"\[(?P<label>[^\]\n]+)\]\(file:(?P<path>[^)\s]+)\)")

# Any link that opens a `file:` address, well-formed or not. This is the net
# that catches the near misses — `[](file:a.c)`, `[a.c](file: a.c)`,
# `[a.c](File:a.c)` — which markdown would otherwise print as the characters
# they are.
_NET_RE = re.compile(r"\]\(\s*file:", re.IGNORECASE)

_WHERE = "p.slide(...), p.columns(...) and p.table(...) text"


def path_of(url: str) -> str | None:
    """The path a link address names when it is a file link, else ``None``."""
    return url[len(SCHEME):] if url.startswith(SCHEME) else None


def is_rooted(path: str) -> bool:
    """Whether ``path`` is relative to the project root rather than the lecture."""
    return path.startswith(ROOT_PREFIX)


def path_problem(path: str) -> str | None:
    """What is wrong with ``path`` as a file a lecture may show; ``None`` if nothing.

    One rule for ``files=`` and for a link: the file is named relative to the
    lecture's directory, or after one leading ``/`` relative to the project
    root, and stays inside that directory. A second ``/`` would make the rest
    an absolute path again, so it is refused with ``..``.
    """
    if not path.strip():
        return "must not be empty"
    relative = path[len(ROOT_PREFIX):] if is_rooted(path) else path
    pure = PurePosixPath(relative)
    if not relative or pure.is_absolute() or ".." in pure.parts:
        return (
            "must be relative to the lecture directory, or begin with one /"
            " and be relative to the project root"
        )
    return None


def _matches(text: str) -> list[re.Match]:
    """The links of ``text`` outside its code fences and code spans.

    Matched on the text with its verbatim spans blanked, which keeps every
    position where it was: a span of the match is a span of ``text``.
    """
    if SCHEME not in text:
        return []
    return list(LINK_RE.finditer(marks.strip_verbatim(text)))


def find(text: str) -> list[tuple[str, str]]:
    """``(label, path)`` for every file link in ``text``, in order."""
    return [
        (text[m.start("label"):m.end("label")], text[m.start("path"):m.end("path")])
        for m in _matches(text)
    ]


def replace(text: str, render: Callable[[str, str], str]) -> str:
    """``text`` with every file link rewritten to ``render(label, path)``."""
    out: list[str] = []
    last = 0
    for m in _matches(text):
        out.append(text[last:m.start()])
        out.append(render(
            text[m.start("label"):m.end("label")],
            text[m.start("path"):m.end("path")],
        ))
        last = m.end()
    out.append(text[last:])
    return "".join(out)


def errors(text: str, *, allowed: bool) -> list[str]:
    """Everything wrong with this text's file links; empty when it is fine.

    ``allowed`` says whether a file link may appear here at all. The caller
    turns each message into a ``ValidationError``, as it does for
    :func:`lecturekit.marks.errors`.
    """
    if SCHEME not in text.lower():
        return []
    scan = marks.strip_verbatim(text)
    nets = list(_NET_RE.finditer(scan))
    if not nets:
        return []
    if not allowed:
        return [f"a file link ([label](file:path)) is allowed in {_WHERE} only"]

    problems: list[str] = []
    links = {m.start("path"): m for m in LINK_RE.finditer(scan)}
    for net in nets:
        link = links.get(net.end())
        if link is None:
            near = text[max(net.start() - 20, 0):net.end() + 20]
            problems.append(
                f"malformed file link near {near!r} (write [label](file:path))"
            )
            continue
        path = text[link.start("path"):link.end("path")]
        problem = path_problem(path)
        if problem:
            problems.append(f"file link path {problem}, got {path!r}")
    return problems
