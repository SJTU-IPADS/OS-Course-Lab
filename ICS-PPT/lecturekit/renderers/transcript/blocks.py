"""Helpers the elements' ``to_transcript`` methods draw with.

Each element class (``lecturekit.elements``) says how it prints in the
transcript from a :class:`Sheet`; this module holds that and what several of
them share. What is *not* drawn is the point of the target: `notes`, `prose`,
`demo`, `cover` and
`spacer` have no transcript method, and footnotes are dropped by the renderer
— a printed cheat sheet has no room for a source line it will never follow.
"""

from __future__ import annotations

import html

from lecturekit import model
from lecturekit.markup import Reader

from .images import DEFAULT_WIDTH_PX, Embedder
from .text import HtmlWriter, escape

# Layout markers are deliberately absent: paper prints their leaves in order
# and ignores slide alignment, so a container contributes no box of its own.
PSEUDO_CLASSES = {"keyword": "kw", "message": "msg", "state": "st", "comment": "cm"}

# A figure prints at one uniform scale — the intrinsic width that fills a
# column — so two figures drawn at the same font size come out the same size on
# paper. Anything at or above the reference width takes the full column; a small
# icon keeps its proportion, down to a floor that stays legible.
FULL_WIDTH_PX = DEFAULT_WIDTH_PX
MIN_FIGURE_PCT = 22


class Sheet(HtmlWriter):
    """What a block prints with: the reader, the HTML writer and the embedder."""

    def __init__(self, embedder: Embedder, read: Reader) -> None:
        super().__init__(self._uri)
        self.embedder = embedder
        self.read = read

    def _uri(self, src: str) -> str | None:
        if src.startswith("data:"):
            return html.escape(src, quote=True)
        figure = self.embedder.embed(src)
        return figure.uri if figure else None


def render_block(block: model.Block, sheet: Sheet) -> str:
    return block.to_transcript(sheet)


def render_annotations(block: model.Block, sheet: Sheet) -> str:
    """A block's callout bubbles, as the lines under it a sheet can carry.

    On the slide these float over the figure, positioned by author coordinates;
    paper has nothing to float over, so each becomes one marked line below the
    block it was attached to.
    """
    return "".join(
        f'<p class="tx-anno">{sheet.inline(sheet.read.inline(note.text))}</p>'
        for note in block.annotations
    )


def figure_pct(width_px: int) -> int:
    return max(MIN_FIGURE_PCT, min(100, round(100 * width_px / FULL_WIDTH_PX)))


def img(src: str, alt: str, sheet: Sheet, *, pct: int | None = None) -> str:
    figure = sheet.embedder.embed(src)
    if figure is None:
        return f'<p class="tx-missing">[缺图：{escape(src)}]</p>'
    width = figure_pct(figure.width_px) if pct is None else pct
    return (
        f'<img src="{figure.uri}" alt="{html.escape(alt, quote=True)}" '
        f'style="width:{width}%">'
    )


def caption(text: str | None, sheet: Sheet) -> str:
    return f'<figcaption>{sheet.inline(sheet.read.inline(text))}</figcaption>' if text else ""


FLOW_GLYPH = {"down": "↓", "up": "↑", "both": "↕"}


def is_more(module) -> bool:
    return str(module) in ("...", "…")
