from __future__ import annotations

from typing import Any

from .base import Tool, Workspace, json_result


def make_tool(workspace: Workspace) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        # TODO: resolve the destination path inside the workspace and write the content.
        return json_result(ok=False, error="TODO: implement write_file")

    return Tool(
        name="write_file",
        description="Write a UTF-8 text file inside the lab workspace.",
        schema={
            "type": "object",
            "required": ["path", "content"],
            "properties": {
                "path": {"type": "string"},
                "content": {"type": "string"},
            },
        },
        handler=handler,
    )
