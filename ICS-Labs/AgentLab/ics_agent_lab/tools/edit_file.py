from __future__ import annotations

from typing import Any

from .base import Tool, Workspace, json_result


def make_tool(workspace: Workspace) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        # TODO: resolve the path, replace one exact old_text match with new_text,
        # and avoid rewriting unrelated parts of the file.
        return json_result(ok=False, error="TODO: implement edit_file")

    return Tool(
        name="edit_file",
        description="Replace the first exact text match in a UTF-8 file inside the lab workspace.",
        schema={
            "type": "object",
            "required": ["path", "old_text", "new_text"],
            "properties": {
                "path": {"type": "string"},
                "old_text": {"type": "string"},
                "new_text": {"type": "string"},
            },
        },
        handler=handler,
    )
