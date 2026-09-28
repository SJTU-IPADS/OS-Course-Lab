"""Collect a lecture's page citations into a deduplicated, ordered reference list.

Shared by every renderer that surfaces references (the deck's trailing 参考文献
page, the book's chapter-end section): each distinct citation appears once, in
first-appearance order, tagged with the 1-based deck positions that cited it.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

from . import model


@dataclass(frozen=True)
class CitationEntry:
    citation: model.Citation
    pages: tuple[int, ...]  # 1-based deck positions that cited it


def collect_citations(pages: list[model.Page]) -> list[CitationEntry]:
    """Merge the citations of ``pages`` (deck order) into unique entries.

    Each entry is backreffed by the deck position of every page that cited it —
    the number that page prints, whatever its title or animation.
    """
    order: list[str] = []
    merged: dict[str, list] = {}  # key -> [citation, [page numbers]]
    for position, page in enumerate(pages, start=1):
        for citation in page.citations:
            key = _dedup_key(citation)
            if key not in merged:
                merged[key] = [citation, []]
                order.append(key)
            if position not in merged[key][1]:
                merged[key][1].append(position)
    return [CitationEntry(merged[key][0], tuple(merged[key][1])) for key in order]


def dedup_citations(citations: list[model.Citation]) -> list[model.Citation]:
    """Unique citations in first-appearance order (same dedup identity as above).

    For surfaces that list references without deck page numbers — the book's
    chapter-end section — where ``collect_citations`` would be overkill.
    """
    seen: set[str] = set()
    out: list[model.Citation] = []
    for citation in citations:
        key = _dedup_key(citation)
        if key not in seen:
            seen.add(key)
            out.append(citation)
    return out


def _dedup_key(citation: model.Citation) -> str:
    if citation.key:
        return citation.key
    return _slug(f"{citation.title} {citation.year or ''}")


def _slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.strip().lower()).strip("-")
