from __future__ import annotations

from typing import Any

from .base import Tool, Workspace, json_result


def make_tool(workspace: Workspace) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        # TODO: run the command inside the workspace with a timeout and return
        # structured stdout/stderr information. Add at least a small safety filter.
        return json_result(ok=False, error="TODO: implement bash")

    return Tool(
        name="bash",
        description="Run a shell command in the lab workspace with timeout and basic safety checks.",
        schema={
            "type": "object",
            "required": ["command"],
            "properties": {"command": {"type": "string"}},
        },
        handler=handler,
    )
