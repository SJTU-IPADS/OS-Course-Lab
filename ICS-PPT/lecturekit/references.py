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


def collect_citations(
    pages: list[model.Page], folds: frozenset[str] = frozenset()
) -> list[CitationEntry]:
    """Merge the citations of ``pages`` (deck order) into unique entries.

    ``folds`` is :func:`model.outline_folds`, passed straight through to the
    numbering: pages sharing one slide number backref it once.
    """
    order: list[str] = []
    merged: dict[str, list] = {}  # key -> [citation, [page numbers]]
    numbers = _backref_numbers(pages, folds)
    for position, page in enumerate(pages):
        number = numbers[position]
        for citation in page.citations:
            key = _dedup_key(citation)
            if key not in merged:
                merged[key] = [citation, []]
                order.append(key)
            if number not in merged[key][1]:
                merged[key][1].append(number)
    return [CitationEntry(merged[key][0], tuple(merged[key][1])) for key in order]


def _backref_numbers(
    pages: list[model.Page], folds: frozenset[str] = frozenset()
) -> list[int]:
    """The slide number each page reports when it cites something (1-based).

    An animation is one slide as far as a reference is concerned, as is a run of
    same-titled pages — which is just the shown slide number, see
    ``model.slide_numbers``.
    """
    return model.slide_numbers(pages, folds)


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
