from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class EfficiencyLimits:
    max_requests: int | None = None
    max_tool_calls: int | None = None
    max_estimated_total_tokens: int | None = None
    max_actual_total_tokens: int | None = None
    max_latency_ms: float | None = None


@dataclass(frozen=True)
class EvalScenario:
    name: str
    prompt: str
    turns: list[str] = field(default_factory=list)
    sessions: list[list[str]] = field(default_factory=list)
    workspace_files: dict[str, str] = field(default_factory=dict)
    skill_files: dict[str, str] = field(default_factory=dict)
    skill_source_dirs: tuple[Path, ...] = ()
    extra_tool_dirs: tuple[Path, ...] = ()
    expect_tool_calls: list[str] = field(default_factory=list)
    expect_trace_events: dict[str, int] = field(default_factory=dict)
    expect_output_contains: list[str] = field(default_factory=list)
    expect_output_not_contains: list[str] = field(default_factory=list)
    expect_files_contains: dict[str, str] = field(default_factory=dict)
    agent_config: dict[str, Any] = field(default_factory=dict)
    limits: EfficiencyLimits = field(default_factory=EfficiencyLimits)

    @classmethod
    def from_file(cls, path: Path) -> "EvalScenario":
        data = json.loads(path.read_text(encoding="utf-8"))
        return cls.from_dict(data, base_dir=path.parent)

    @classmethod
    def from_dict(
        cls, data: dict[str, Any], base_dir: Path | None = None
    ) -> "EvalScenario":
        base_dir = base_dir or Path.cwd()
        skill_source_dirs = tuple(
            resolve_scenario_path(base_dir, path)
            for path in data.get("skill_source_dirs", [])
        )
        extra_tool_dirs = tuple(
            resolve_scenario_path(base_dir, path)
            for path in data.get("extra_tool_dirs", [])
        )
        return cls(
            name=data["name"],
            prompt=data["prompt"],
            turns=list(data.get("turns", [])),
            sessions=[list(session) for session in data.get("sessions", [])],
            workspace_files=data.get("workspace_files", {}),
            skill_files=data.get("skill_files", {}),
            skill_source_dirs=skill_source_dirs,
            extra_tool_dirs=extra_tool_dirs,
            expect_tool_calls=data.get("expect_tool_calls", []),
            expect_trace_events=data.get("expect_trace_events", {}),
            expect_output_contains=data.get("expect_output_contains", []),
            expect_output_not_contains=data.get("expect_output_not_contains", []),
            expect_files_contains=data.get("expect_files_contains", {}),
            agent_config=data.get("agent_config", {}),
            limits=EfficiencyLimits(**data.get("limits", {})),
        )


def resolve_scenario_path(base_dir: Path, value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (base_dir / path).resolve()
