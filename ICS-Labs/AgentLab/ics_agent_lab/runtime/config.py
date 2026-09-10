from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RuntimeConfig:
    provider: str = "openrouter"
    workspace: Path = Path("workspace")
    skills_dir: Path = Path("skills")
    model: str | None = None
    seed: int | None = None
    temperature: float | None = None
    subagent_max_steps: int = 25
    extra_tool_dirs: tuple[Path, ...] = ()
