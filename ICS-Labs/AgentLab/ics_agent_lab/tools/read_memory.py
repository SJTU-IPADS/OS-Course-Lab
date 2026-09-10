from __future__ import annotations

from typing import Any

from ..memory import MemoryLoader
from .base import Tool, json_result


def make_tool(memory_loader: MemoryLoader | None) -> Tool:

    def handler(arguments: dict[str, Any]) -> str:
        if memory_loader is None:
            return json_result(ok=False, error="Memory loader is not configured.")

        # TODO: read memory by key
        return json_result(ok=False, key=None, content="Not implemented yet.")

    return Tool(
        name="read_memory",
        description="Read one persistent Markdown memory by exact key.",
        schema={
            "type": "object",
            "required": ["key"],
            "properties": {
                "key": {"type": "string"},
            },
        },
        handler=handler,
    )
