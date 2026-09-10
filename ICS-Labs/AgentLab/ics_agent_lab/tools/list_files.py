from __future__ import annotations

from typing import Any

from .base import Tool, Workspace, json_result


def make_tool(workspace: Workspace) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        # TODO: resolve the directory inside the workspace and return a stable,
        # recursive file listing relative to the workspace root.
        return json_result(ok=False, error="TODO: implement list_files")

    return Tool(
        name="list_files",
        description="List files inside the lab workspace.",
        schema={
            "type": "object",
            "required": ["path"],
            "properties": {"path": {"type": "string"}},
        },
        handler=handler,
    )
