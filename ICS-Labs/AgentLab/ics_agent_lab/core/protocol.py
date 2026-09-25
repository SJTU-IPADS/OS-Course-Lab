from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Literal


@dataclass(frozen=True)
class ParsedMessage:
    kind: Literal["tool_call", "final"]
    name: str | None = None
    arguments: dict[str, Any] | None = None
    content: str | None = None


@dataclass(frozen=True)
class ParseError:
    raw: str
    reason: str


class ManualJsonProtocol:

    def build_system_prompt(
        self,
        tool_docs: str,
        skill_docs: str = "(no skills available)",
        memory_docs: str = "(no memory available)",
    ) -> str:
        # TODO: your system prompt, including protocol and any available tools/skills.
        return "You are a helpful assistant."

    def parse(self, text: str) -> ParsedMessage | ParseError:
        raise NotImplementedError("TODO: parse one manual-JSON assistant message.")

    def repair_prompt(self, bad_text: str, reason: str) -> str:
        return (
            "Your previous response was invalid for the JSON protocol.\n"
            f"Reason: {reason}\n\n"
            "Repair it now. Output exactly one JSON object and no Markdown.\n"
            f"Previous response:\n{bad_text}"
        )
