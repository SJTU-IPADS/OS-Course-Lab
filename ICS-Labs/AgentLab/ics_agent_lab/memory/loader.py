from __future__ import annotations

import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Memory:
    key: str
    content: str
    path: Path


class MemoryLoader:
    """Load and save persistent Markdown memories."""

    def __init__(self, memory_dir: Path) -> None:
        self.memory_dir = memory_dir
        self.reload()

    def reload(self) -> None:
        # TODO: scan memory_dir/*.md files and load metadata of them.
        pass

    def descriptions(self) -> str:
        # TODO: return a bullet list of memory keys.
        return "(no memories saved)"

    def content(self, key: str) -> str:
        # TODO: read the memory file for the given key,
        #       parse it and return its content.
        return "Not implemented yet."

    def save(self, key: str, content: str) -> Memory:
        key = key.strip()
        if not key:
            raise ValueError("Memory key must not be empty.")
        content = content.strip()
        if not content:
            raise ValueError("Memory content must not be empty.")

        # TODO: save the memory content to a Markdown file named by the key,
        #       and return the Memory object.
        raise NotImplementedError("Not implemented yet.")
