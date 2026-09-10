from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Skill:
    name: str
    description: str
    body: str
    path: Path


class SkillLoader:
    """Load skills from SKILL.md files under a directory."""

    def __init__(self, skills_dir: Path) -> None:
        self.skills_dir = skills_dir
        self.reload()

    def reload(self) -> None:
        # TODO: scan skills_dir/<name>/SKILL.md files and load metadata of them.
        pass

    def descriptions(self) -> str:
        # TODO: return a bullet list of skill names and descriptions.
        return "(no skills available)"

    def content(self, name: str) -> str:
        # TODO: read the SKILL.md file for the given skill name,
        #       parse it and return its content.
        return "Not implemented yet."
