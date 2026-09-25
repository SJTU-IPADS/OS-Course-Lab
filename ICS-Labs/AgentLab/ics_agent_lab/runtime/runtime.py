from __future__ import annotations

from dataclasses import dataclass

from ..core import Agent
from ..llm import LLMTransport
from ..memory import MemoryLoader
from ..skills import SkillLoader
from .config import RuntimeConfig


@dataclass
class AgentRuntime:
    agent: Agent
    llm: LLMTransport
    skill_loader: SkillLoader
    memory_loader: MemoryLoader
    config: RuntimeConfig

    def ensure_workspace(self) -> None:
        self.config.workspace.mkdir(parents=True, exist_ok=True)
