"""Collect every lecture's images into one asset tree under the book.

A lecture's image paths are relative to its own directory, and a book merges
many lectures, so the images are copied into ``assets/<lecture-id>/`` — the
lecture id namespaces them, and two lectures may ship the same basename.
"""

from __future__ import annotations

import shutil
from pathlib import Path


class AssetCopier:
    def __init__(self, output_dir: Path):
        self.output_dir = Path(output_dir)
        # (lecture_id, src) -> relative path already written.
        self._copied: dict[tuple[str, str], str] = {}
        # (lecture_id, basename) -> how many distinct sources claimed it.
        self._names: dict[tuple[str, str], int] = {}

    def copy(self, lecture_id: str, asset_root: Path, src: str) -> str:
        """Copy ``asset_root/src`` into the book, returning its path from ``book.tex``."""
        key = (lecture_id, src)
        if key in self._copied:
            return self._copied[key]

        source = Path(asset_root) / src
        if not source.exists():
            raise FileNotFoundError(f"missing image: {source}")

        name = Path(src).name
        taken = self._names.get((lecture_id, name), 0)
        self._names[(lecture_id, name)] = taken + 1
        if taken:
            # Same basename, different source dir: fig.png, fig-2.png, fig-3.png
            stem, suffix = Path(name).stem, Path(name).suffix
            name = f"{stem}-{taken + 1}{suffix}"

        destination = self.output_dir / "assets" / lecture_id / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)

        rel = f"assets/{lecture_id}/{name}"
        self._copied[key] = rel
        return rel
