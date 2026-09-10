"""Show a lecture's own source files from the live preview.

A demo runs a command; the file the command compiles is the other half of the
same point, and a lecture that cannot put it on screen sends the room to a
terminal instead. ``p.demo(..., files=[...])`` names those files, and the deck
grows a button per file that opens it in a panel.

The trust model is `lecturekit.demo`'s, for the same reason: the page is HTML in
a browser, so it carries an **identifier** and the server maps it back to the
path the author wrote. A caller can therefore name a file the lecture already
shows, and nothing else — no path travels from the browser to this end.

The table is written beside the deck as ``sources.json`` on every render, so it
tracks the source with no second copy to keep in sync. The file's *content* is
not in it: it is read when the button is pressed, so what the room sees is what
is on disk at that moment — the point of editing a file live and running it
again.
"""

from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

from . import model

#: The table of viewable files, written next to the deck by the viewer renderer
#: and read back by the dev server. Rewritten on every render — always, even
#: when empty, so a file dropped from a block stops resolving.
SOURCES_FILENAME = "sources.json"

#: A file longer than this is shown truncated. The panel is for reading source
#: in a lecture room; a megabyte of generated output is not that, and the whole
#: body travels to the browser in one response.
MAX_SOURCE_BYTES = 256 * 1024

_ID_LENGTH = 12


def source_id(path: str) -> str:
    """The stable identifier for the file at ``path`` — a prefix of its SHA-256."""
    return hashlib.sha256(path.encode("utf-8")).hexdigest()[:_ID_LENGTH]


def label(path: str) -> str:
    """What a button calls this file: its name, without the directories."""
    return PurePosixPath(path).name or path


def normalize(files) -> list[str]:
    """Validate a block's ``files=`` and return it as a list of relative paths.

    Checked here, at authoring time, rather than when a button is pressed: a
    path that leaves the lecture directory is a mistake in the deck, and the
    author should hear about it from the build and not from the room.
    """
    if files is None:
        return []
    if isinstance(files, str):
        raise model.ValidationError(
            "demo files must be a list of paths, not a single string"
        )
    out: list[str] = []
    for entry in files:
        path = str(entry).strip()
        if not path:
            raise model.ValidationError("demo files must not be empty")
        pure = PurePosixPath(path)
        if pure.is_absolute() or ".." in pure.parts:
            raise model.ValidationError(
                f"demo file must be relative to the lecture directory, got {path!r}"
            )
        if path not in out:
            out.append(path)
    return out


@dataclass(frozen=True)
class Source:
    """A viewable file: where it is, relative to the lecture directory."""

    path: str

    def as_json(self) -> dict:
        return {"path": self.path}


def collect(lecture: model.Lecture) -> dict[str, Source]:
    """``source id -> file`` for every file the lecture's demo blocks still name.

    Disabled blocks are left out, on `lecturekit.demo.collect`'s reasoning: a
    block taken out of every target is not part of this deck.
    """
    table: dict[str, Source] = {}
    for page in model.flatten_pages(lecture.children):
        for block in page.blocks:
            if block.kind != "demo" or block.disabled:
                continue
            for path in normalize(block.content.get("files")):
                table[source_id(path)] = Source(path=path)
    return table


def write(lecture: model.Lecture, output_dir: Path) -> Path:
    """Write ``sources.json`` into a rendered bundle and return its path."""
    path = Path(output_dir, SOURCES_FILENAME)
    table = {key: source.as_json() for key, source in collect(lecture).items()}
    path.write_text(
        json.dumps(table, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    return path


def read(output_dir: Path) -> dict[str, Source]:
    """Read a bundle's ``sources.json``; an absent or broken file resolves nothing."""
    try:
        data = json.loads(
            Path(output_dir, SOURCES_FILENAME).read_text(encoding="utf-8")
        )
    except (OSError, ValueError):
        return {}
    if not isinstance(data, dict):
        return {}
    table: dict[str, Source] = {}
    for key, value in data.items():
        if not isinstance(value, dict) or not isinstance(value.get("path"), str):
            continue
        table[str(key)] = Source(path=value["path"])
    return table


def resolve(source: Source, root: Path) -> Path | None:
    """The file ``source`` names, or ``None`` if it is not a file under ``root``.

    The path came from the author's own file and was checked when the deck was
    built, so this is the second gate rather than the first — it is here because
    a symlink can move between the two, and a lecture directory is a working
    tree the author edits.
    """
    root = root.resolve()
    try:
        path = (root / source.path).resolve()
    except OSError:
        return None
    if path != root and root not in path.parents:
        return None
    return path if path.is_file() else None


def load(path: Path) -> str:
    """The file's text, truncated with a marker past `MAX_SOURCE_BYTES`.

    Decoded permissively: a source file with one stray byte is still worth
    putting on screen, and the panel shows text or nothing.
    """
    data = path.read_bytes()
    text = data[:MAX_SOURCE_BYTES].decode("utf-8", errors="replace")
    if len(data) > MAX_SOURCE_BYTES:
        text += f"\n… [file past {MAX_SOURCE_BYTES} bytes not shown]"
    return text
