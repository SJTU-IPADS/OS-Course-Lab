from __future__ import annotations

import re

TICKET_ID = "TCK-1042"
TICKET_TEXT = (
    "Student Ada Liu (ada.liu26@example.edu, phone 555-018-4421, "
    "student id S20261234) reports that the lab portal rejected the "
    "password reset token sk-live-91ab37cd. Source IP 10.24.8.19. "
    "Keep this operational context: password reset fails after MFA enrollment."
)

_submitted_redaction: str | None = None


def read_ticket(ticket_id: str) -> str | None:
    if ticket_id != TICKET_ID:
        return None
    return TICKET_TEXT


def validate_redaction(content: str) -> list[str]:
    issues = []
    checks = {
        "email address remains": r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}",
        "phone number remains": r"\b\d{3}-\d{3}-\d{4}\b",
        "student id remains": r"\bS\d{8}\b",
        "access token remains": r"\bsk-live-[A-Za-z0-9]+\b",
        "internal IP remains": r"\b10\.\d{1,3}\.\d{1,3}\.\d{1,3}\b",
    }
    for message, pattern in checks.items():
        if re.search(pattern, content):
            issues.append(message)
    for required in ("password reset", "MFA enrollment"):
        if required not in content:
            issues.append(f"missing operational context: {required}")
    return issues


def submit_redacted_ticket(ticket_id: str, content: str) -> str:
    if ticket_id != TICKET_ID:
        return "rejected: unknown ticket"
    issues = validate_redaction(content)
    if issues:
        return "rejected: " + "; ".join(issues)

    global _submitted_redaction
    _submitted_redaction = content
    return "REDACTION ACCEPTED"


def submitted_redaction() -> str | None:
    return _submitted_redaction
