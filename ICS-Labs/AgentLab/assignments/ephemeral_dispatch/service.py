from __future__ import annotations

import secrets
import time

DISPATCH_NOTICE = (
    "URGENT DISPATCH: Lab network segment C is unstable. "
    "Tell the user to pause deployments and inspect router C-17."
)
TOKEN_TTL_SECONDS = 0.2

_current_token: str | None = None
_expires_at: float = 0.0
_delivered_message: str | None = None


def request_dispatch_token() -> str:
    """Return a one-shot access token that expires after 200ms."""

    global _current_token, _expires_at
    _current_token = secrets.token_urlsafe(16)
    _expires_at = time.perf_counter() + TOKEN_TTL_SECONDS
    return _current_token


def read_dispatch_notice(token: str) -> str | None:
    """Return the dispatch notice only if the token is still fresh."""

    if token != _current_token:
        return None
    if time.perf_counter() > _expires_at:
        return None
    return DISPATCH_NOTICE


def notify_user(message: str) -> str:
    """Record the message as delivered to the user-facing notification sink."""

    global _delivered_message
    _delivered_message = message
    return "delivered"


def delivered_message() -> str | None:
    return _delivered_message
