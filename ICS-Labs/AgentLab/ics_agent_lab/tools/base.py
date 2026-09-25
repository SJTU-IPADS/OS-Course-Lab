from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable

ToolHandler = Callable[[dict[str, Any]], str]


@dataclass(frozen=True)
class Tool:
    name: str
    description: str
    schema: dict[str, Any]
    handler: ToolHandler

    def docs(self) -> str:
        return (
            f"- {self.name}: {self.description}\n"
            f"  arguments schema: {json.dumps(self.schema, ensure_ascii=False)}"
        )


@dataclass(frozen=True)
class Workspace:
    root: Path

    def __post_init__(self) -> None:
        self.root.resolve().mkdir(parents=True, exist_ok=True)

    @property
    def resolved_root(self) -> Path:
        return self.root.resolve()

    def resolve(self, path: str) -> Path:
        root = self.resolved_root
        candidate = (root / path).resolve()
        if root not in candidate.parents and candidate != root:
            raise ValueError("Path escapes lab workspace.")
        return candidate


class ToolRegistry:

    def __init__(self) -> None:
        self._tools: dict[str, Tool] = {}

    def register(self, tool: Tool) -> None:
        if tool.name in self._tools:
            raise ValueError(f"Duplicate tool name: {tool.name}")
        self._tools[tool.name] = tool

    def docs(self) -> str:
        return "\n".join(tool.docs() for tool in self._tools.values())

    def run(self, name: str, arguments: dict[str, Any]) -> str:
        tool = self._tools.get(name)
        if tool is None:
            return json_result(ok=False, error=f"Unknown tool `{name}`.")

        validation_error = validate_arguments(tool.schema, arguments)
        if validation_error:
            return json_result(ok=False, error=validation_error)

        try:
            return tool.handler(arguments)
        except Exception as exc:
            return json_result(ok=False, error=str(exc))


def json_result(**payload: Any) -> str:
    return json.dumps(payload, ensure_ascii=False)


def validate_arguments(schema: dict[str, Any], arguments: dict[str, Any]) -> str | None:
    required = schema.get("required", [])
    properties = schema.get("properties", {})

    for key in required:
        if key not in arguments:
            return f"Missing required argument `{key}`."

    for key, value in arguments.items():
        expected = properties.get(key)
        if expected is None:
            return f"Unexpected argument `{key}`."
        expected_type = expected.get("type")
        if expected_type == "string" and not isinstance(value, str):
            return f"Argument `{key}` must be a string."
        if expected_type == "integer" and not isinstance(value, int):
            return f"Argument `{key}` must be an integer."
    return None
