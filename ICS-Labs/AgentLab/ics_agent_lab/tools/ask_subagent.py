from __future__ import annotations

from typing import Any, Callable

from .base import Tool, json_result


def make_tool(subagent_runner: Callable[[str], str] | None) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        if subagent_runner is None:
            return json_result(ok=False, error="Subagent is not configured.")

        # TODO: hand the bounded task text to the subagent runner and return its result.
        return json_result(ok=False, error="TODO: implement ask_subagent")

    return Tool(
        name="ask_subagent",
        description="Ask a smaller subagent to solve a bounded text-only task.",
        schema={
            "type": "object",
            "required": ["task"],
            "properties": {"task": {"type": "string"}},
        },
        handler=handler,
    )
