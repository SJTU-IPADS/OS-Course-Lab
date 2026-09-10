from __future__ import annotations

from typing import Any

from .base import Tool, Workspace, json_result


def make_tool(workspace: Workspace) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        # TODO: resolve the path with workspace.resolve(...), read the file,
        # and optionally honor a line limit for large files.
        return json_result(ok=False, error="TODO: implement read_file")

    return Tool(
        name="read_file",
        description="Read a UTF-8 text file inside the lab workspace.",
        schema={
            "type": "object",
            "required": ["path"],
            "properties": {
                "path": {"type": "string"},
                "limit": {"type": "integer"},
            },
        },
        handler=handler,
    )
