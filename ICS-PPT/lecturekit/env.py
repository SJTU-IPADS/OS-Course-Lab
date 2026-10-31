"""The environment a demo runs in: the project root and the project's ``.env``.

A demo is the author's own command, run by the live preview. Two kinds of value
reach it from outside the slide: the project root (``$LECTUREKIT_ROOT``) and
whatever the course keeps in ``.env`` at that root, such as the token of a
course server. Both are injected here, so `lecturekit.demo` only asks for the
finished environment and the slide never carries a secret.

``.env`` is read by python-dotenv (``KEY=VALUE`` lines, ``#`` comments, quotes,
``export`` prefix), without variable expansion. A variable already in the
process environment wins over the file, so a value set on the command line
overrides the file for one run. The file is read at every launch, so an edit
takes effect on the next press of the button.

Every variable of ``.env`` is visible to every demo of every lecture the preview
serves, as the author's commands are trusted to the same degree. ``.env`` is
therefore for values a demo needs and nothing else, and it is git-ignored.
Only the live preview reads it: the exported viewer, the PDF and the PPTX never
run a demo and never contain it.
"""

from __future__ import annotations

import os
from collections.abc import Mapping
from pathlib import Path

from dotenv import dotenv_values

#: The file at the project root that holds the values injected into demos.
FILENAME = ".env"

#: Set by the framework, never by the file.
ROOT_VARIABLE = "LECTUREKIT_ROOT"


def read(root: str | Path) -> dict[str, str]:
    """The variables of ``<root>/.env``, ``{}`` when there is no such file.

    A line without a value (``KEY`` alone) is dropped.
    """
    path = Path(root) / FILENAME
    if not path.is_file():
        return {}
    values = dotenv_values(path, interpolate=False)
    return {key: value for key, value in values.items() if value is not None}


def demo_environment(root: str, base: Mapping[str, str] | None = None) -> dict[str, str]:
    """The environment of a demo: ``base`` (the process environment by default),
    the variables of ``<root>/.env`` under it, and ``LECTUREKIT_ROOT``.

    ``root`` is the project root as a demo sees it. A value already in ``base``
    is kept over the file's, and the file cannot set ``LECTUREKIT_ROOT``.
    """
    env = dict(os.environ if base is None else base)
    for key, value in read(root).items():
        if key != ROOT_VARIABLE:
            env.setdefault(key, value)
    env[ROOT_VARIABLE] = root
    return env

