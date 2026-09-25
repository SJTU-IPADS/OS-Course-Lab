from __future__ import annotations

import os
from dataclasses import dataclass
from typing import Any

from dotenv import load_dotenv
from openai import APIStatusError, OpenAI

from .base import LLMTransport, Message


@dataclass
class OpenRouterChatTransport(LLMTransport):
    """OpenAI-compatible OpenRouter transport.

    This class intentionally does not pass `tools`, `tool_choice`, or any
    built-in agent feature. The lab agent owns the protocol and loop.
    """

    model: str | None = None
    fallback_model: str | None = None
    temperature: float | None = None
    seed: int | None = None
    api_key_env: str = "OPENROUTER_API_KEY"
    base_url: str | None = None
    last_usage: dict[str, int] | None = None

    def __post_init__(self) -> None:
        load_dotenv(override=True)
        normalize_socks_proxy_env()
        if self.model is None:
            self.model = os.getenv("MODEL_ID", "openrouter/free")
        if self.fallback_model is None:
            self.fallback_model = os.getenv(
                "OPENROUTER_FALLBACK_MODEL", "openrouter/free"
            )
        if self.base_url is None:
            self.base_url = os.getenv(
                "OPENROUTER_BASE_URL", "https://openrouter.ai/api/v1"
            )
        env_temperature = os.getenv("LLM_TEMPERATURE")
        if self.temperature is None and env_temperature is not None:
            self.temperature = float(env_temperature)
        if self.temperature is None:
            self.temperature = 0.0
        env_seed = os.getenv("LLM_SEED")
        if self.seed is None and env_seed:
            self.seed = int(env_seed)

    def complete(self, messages: list[Message]) -> str:
        api_key = os.environ.get(self.api_key_env)
        if not api_key:
            raise RuntimeError(
                f"Missing {self.api_key_env}. Put it in .env or export it."
            )

        client = OpenAI(api_key=api_key, base_url=self.base_url)
        request = {
            "model": self.model,
            "messages": messages,
            "temperature": self.temperature,
        }
        if self.seed is not None:
            request["seed"] = self.seed

        try:
            response = client.chat.completions.create(**request)
        except APIStatusError as error:
            if error.status_code != 404 or self.model == self.fallback_model:
                raise
            request["model"] = self.fallback_model
            response = client.chat.completions.create(**request)

        content = response.choices[0].message.content
        self.last_usage = usage_to_dict(getattr(response, "usage", None))
        return content or ""


def normalize_socks_proxy_env() -> None:
    """httpx accepts socks5://, but many desktop proxy tools export socks://."""

    for key in (
        "HTTP_PROXY",
        "HTTPS_PROXY",
        "ALL_PROXY",
        "http_proxy",
        "https_proxy",
        "all_proxy",
    ):
        value = os.environ.get(key)
        if value and value.startswith("socks://"):
            os.environ[key] = "socks5://" + value.removeprefix("socks://")


def usage_to_dict(usage: Any) -> dict[str, int] | None:
    if usage is None:
        return None
    if hasattr(usage, "model_dump"):
        data = usage.model_dump()
    elif isinstance(usage, dict):
        data = usage
    else:
        data = {
            "prompt_tokens": getattr(usage, "prompt_tokens", None),
            "completion_tokens": getattr(usage, "completion_tokens", None),
            "total_tokens": getattr(usage, "total_tokens", None),
        }
    result = {}
    for key in ("prompt_tokens", "completion_tokens", "total_tokens"):
        value = data.get(key)
        if isinstance(value, int):
            result[key] = value
    return result or None
