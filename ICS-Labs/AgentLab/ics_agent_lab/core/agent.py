from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from ..llm import LLMTransport, Message
from ..memory import MemoryLoader
from ..tools import ToolRegistry
from .protocol import ManualJsonProtocol, ParseError
from .trace import TraceRecorder


@dataclass
class AgentConfig:
    max_steps: int = 100
    max_parse_repairs: int = 2
    compact_after_messages: int = 20
    compact_recent_messages: int = 4
    compact_summary_limit: int = 12000
    tool_result_limit: int = 6000


class Agent:
    def __init__(
        self,
        llm: LLMTransport,
        tools: ToolRegistry,
        *,
        config: AgentConfig | None = None,
        protocol: ManualJsonProtocol | None = None,
        trace: TraceRecorder | None = None,
        skill_docs: str = "(no skills available)",
        memory_docs: str = "(no memory available)",
        name: str = "main",
    ) -> None:
        self.llm = llm
        self.tools = tools
        self.config = config or AgentConfig()
        self.protocol = protocol or ManualJsonProtocol()
        self.trace = trace or TraceRecorder()
        self.skill_docs = skill_docs
        self.memory_docs = memory_docs
        self.name = name

    def save_trace(self, path: Path) -> None:
        self.trace.save_jsonl(path)

    def run(self, user_input: str) -> str:
        messages = self.new_session()
        return self.run_turn(messages, user_input)

    def new_session(self) -> list[Message]:
        return [
            {
                "role": "system",
                "content": self.protocol.build_system_prompt(
                    self.tools.docs(), self.skill_docs, self.memory_docs
                ),
            }
        ]

    def run_turn(self, messages: list[Message], user_input: str) -> str:
        messages.append(
            {
                "role": "user",
                "content": user_input,
            }
        )

        # TODO: implement the manual JSON agent loop here.
        # Suggested flow:
        # 1. call llm.complete(messages)
        # 2. trace llm_response
        # 3. parse with self.protocol.parse(...)
        # 4. on tool_call: run tool, trace it, append tool result, continue
        # 5. on final: trace final and return
        # 6. on parse error: trace parse_error and append self.protocol.repair_prompt(...)
        # 7. compact old messages and trace context_compacted when the history grows too large
        messages.append(
            {
                "role": "assistant",
                "content": f"I cannot answer `{user_input}` yet.",
            }
        )

        raw = messages[-1]["content"]
        self.trace.add(1, "llm_response", agent=self.name, raw=raw)
        self.trace.add(1, "final", agent=self.name, content=raw)
        return raw
