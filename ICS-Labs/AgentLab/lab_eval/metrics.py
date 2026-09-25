from __future__ import annotations

import time
from dataclasses import asdict, dataclass, field

from ics_agent_lab.llm import LLMTransport, Message


@dataclass
class EvalMetrics:
    request_count: int = 0
    actual_prompt_tokens: int = 0
    actual_completion_tokens: int = 0
    actual_total_tokens: int = 0
    estimated_prompt_tokens: int = 0
    estimated_completion_tokens: int = 0
    estimated_total_tokens: int = 0
    total_latency_ms: float = 0.0
    per_request_latency_ms: list[float] = field(default_factory=list)

    def to_dict(self) -> dict:
        return asdict(self)


class MeasuringTransport(LLMTransport):
    """Wrap an LLM transport and collect provider-independent metrics."""

    def __init__(self, inner: LLMTransport) -> None:
        self.inner = inner
        self.metrics = EvalMetrics()

    def complete(self, messages: list[Message]) -> str:
        self.metrics.request_count += 1
        self.metrics.estimated_prompt_tokens += estimate_messages_tokens(messages)

        started = time.perf_counter()
        response = self.inner.complete(messages)
        latency_ms = (time.perf_counter() - started) * 1000

        self.metrics.per_request_latency_ms.append(latency_ms)
        self.metrics.total_latency_ms += latency_ms
        self.metrics.estimated_completion_tokens += estimate_text_tokens(response)
        self.metrics.estimated_total_tokens = (
            self.metrics.estimated_prompt_tokens
            + self.metrics.estimated_completion_tokens
        )

        usage = getattr(self.inner, "last_usage", None)
        if usage:
            self.metrics.actual_prompt_tokens += usage.get("prompt_tokens", 0)
            self.metrics.actual_completion_tokens += usage.get("completion_tokens", 0)
            self.metrics.actual_total_tokens += usage.get("total_tokens", 0)

        return response


def estimate_messages_tokens(messages: list[Message]) -> int:
    return sum(
        estimate_text_tokens(f"{msg['role']}: {msg['content']}") for msg in messages
    )


def estimate_text_tokens(text: str) -> int:
    # Good enough for provider-independent eval trends; use actual usage when available.
    return max(1, (len(text) + 3) // 4)
