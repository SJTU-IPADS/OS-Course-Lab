from __future__ import annotations

from abc import ABC, abstractmethod

Message = dict[str, str]


class LLMTransport(ABC):
    """Transport layer: it only sends messages and returns assistant text."""

    @abstractmethod
    def complete(self, messages: list[Message]) -> str:
        raise NotImplementedError
