"""Render a lecture as a compact, printable transcript sheet.

A lecture's deck is one idea per slide, which is the right shape for a
projector and the wrong shape for an open-book exam: printed one-up, a lecture
is seventy sheets of paper. The transcript keeps the same content and drops the
slide — the section tree becomes a numbered outline, each slide becomes one
entry under it, and the figures come along beside the entry they belong to, at
a size a student can still read.

It derives everything from what the deck already says: there is no third body
to write. What it leaves behind is what a sheet cannot use — speaker notes,
book prose, source footnotes, the cover, the reference list — and what would be
printed twice: an animation collapses to its finished frame, a reveal pair to
its revealed half, and a borrowed review page to nothing at all (the reader has
that lecture's own sheet).
"""

from __future__ import annotations

import html
import re
from pathlib import Path

from lecturekit import model
from lecturekit.dsl import slugify

from .blocks import TRANSCRIPT_KINDS, render_annotations, render_block
from .images import Embedder
from .style import CSS
from .text import inline

# The deepest heading the sheet prints; below it, entries keep the numbering but
# reuse the last heading level rather than shrinking into the body text.
_MAX_HEADING = 4


class TranscriptRenderer:
    name = "transcript"

    def __init__(self, *, asset_root: Path | None = None):
        self.asset_root = asset_root

    def render(self, lecture: model.Lecture, output_dir: Path) -> Path:
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        embedder = Embedder(self.asset_root, lecture.borrowed)
        document = build_html(lecture, embedder)
        entry = output_dir / f"{slugify(lecture.title)}-transcript.html"
        entry.write_text(document, encoding="utf-8")
        return entry


def build_html(lecture: model.Lecture, embedder: Embedder) -> str:
    """The whole sheet as one self-contained HTML document."""
    keep = kept_page_ids(lecture)
    body = _embed_inline_images(
        "".join(_nodes(lecture.children, keep, embedder, prefix="", depth=0)),
        embedder,
    )
    subtitle = (
        f'<div class="tx-sub">{html.escape(lecture.subtitle)}</div>'
        if lecture.subtitle else ""
    )
    return (
        "<!doctype html>\n"
        '<html lang="zh">\n<head>\n<meta charset="utf-8">\n'
        f"<title>{html.escape(lecture.title)}</title>\n"
        f"<style>{CSS}</style>\n</head>\n<body>\n"
        f'<header class="tx-head"><h1>{html.escape(lecture.title)}</h1>{subtitle}</header>\n'
        f'<main class="tx-doc">{body}</main>\n'
        "</body>\n</html>\n"
    )


_INLINE_IMG_RE = re.compile(r"<img\b[^>]*>", re.IGNORECASE)
_SRC_RE = re.compile(r'(\bsrc=")([^"]*)(")', re.IGNORECASE)


def _embed_inline_images(document: str, embedder: Embedder) -> str:
    """Embed the figures an author wrote as raw `<img>` inside slide markdown.

    The block renderers embed their own figures as they go; this catches the
    ones that arrived as HTML in a table cell, where the src is still a path
    relative to the lecture. One that cannot be read is dropped whole rather
    than left as a broken-image box — the row it labels still reads.
    """
    def replace(match: re.Match) -> str:
        tag = match.group(0)
        src = _SRC_RE.search(tag)
        if src is None or src.group(2).startswith("data:"):
            return tag
        figure = embedder.embed(src.group(2))
        if figure is None:
            return ""
        return _SRC_RE.sub(lambda m: f"{m.group(1)}{figure.uri}{m.group(3)}", tag, count=1)

    return _INLINE_IMG_RE.sub(replace, document)


def kept_page_ids(lecture: model.Lecture) -> set[str]:
    """The ids of the pages the sheet prints, after every de-duplication.

    Four things drop out, in order: the cover and any borrowed review page, the
    earlier frames of an animation, the unrevealed half of an annotation reveal
    pair, and finally any page left with nothing to show (which is how an author
    keeps a whole page off the sheet — mark its blocks `except_`).
    """
    pages = model.flatten_pages(lecture.children)
    borrowed = tuple(f"{entry.lecture_id}/" for entry in lecture.borrowed)

    # An animation is one figure built up over N slides: only the last surviving
    # frame carries the finished picture — and, with it, any blocks written
    # after `p.frames(...)`; the earlier frames repeat the text before it.
    last_frame = {
        positions[-1] for positions in model.frame_groups(pages).values()
    }
    frame_positions = {
        position
        for positions in model.frame_groups(pages).values()
        for position in positions
    }

    kept: list[model.Page] = []
    for position, page in enumerate(pages):
        if any(block.kind == "cover" for block in page.blocks):
            continue
        if borrowed and page.id.startswith(borrowed):
            continue
        if position in frame_positions and position not in last_frame:
            continue
        if not visible_blocks(page):
            continue
        kept.append(page)

    # A reveal pair is the same body authored twice, bubbles off then on. On
    # paper there is no paging forward, so the revealed half stands for both.
    ids = set()
    for index, page in enumerate(kept):
        following = kept[index + 1] if index + 1 < len(kept) else None
        if following is not None and _same_content(page, following):
            continue
        ids.add(page.id)
    return ids


def visible_blocks(page: model.Page) -> list[model.Block]:
    return model.select_blocks(page, "transcript", TRANSCRIPT_KINDS)


def _same_content(page: model.Page, other: model.Page) -> bool:
    return page.title == other.title and visible_blocks(page) == visible_blocks(other)


def _nodes(
    children: list[model.Section | model.Page],
    keep: set[str],
    embedder: Embedder,
    *,
    prefix: str,
    depth: int,
) -> list[str]:
    """One outline level: sections and pages share the numbering at each level."""
    out: list[str] = []
    counter = 0
    for child in children:
        if isinstance(child, model.Page):
            if child.id not in keep:
                continue
            counter += 1
            out.append(_page(child, f"{prefix}{counter}", embedder, depth))
            continue
        body = _nodes(child.children, keep, embedder,
                      prefix=f"{prefix}{counter + 1}.", depth=depth + 1)
        if not body:
            # A section whose every page dropped out has nothing to head.
            continue
        counter += 1
        out.append(_heading(depth, f"{prefix}{counter}", child.title))
        out.extend(body)
    return out


def _page(page: model.Page, number: str, embedder: Embedder, depth: int) -> str:
    parts = [_heading(depth, number, page.title)]
    for block in visible_blocks(page):
        parts.append(render_block(block, embedder))
        if page.show_annotations:
            parts.append(render_annotations(block))
    return "".join(parts)


def _heading(depth: int, number: str, title: str) -> str:
    tag = f"h{min(depth + 2, _MAX_HEADING)}"
    return f'<{tag}><span class="tx-num">{number}</span>{inline(title)}</{tag}>'
