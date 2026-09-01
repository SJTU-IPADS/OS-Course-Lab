"""Slide geometry and a vertical layout cursor for the PPTX renderer.

No browser measures text here, so blocks are stacked top-to-bottom by an
estimated height instead of a real layout pass. ``Layout`` derives the slide
size and content box from the deck ratio (the viewer's pixel sizes are at 96
dpi, so ``inches = px / 96``) and the theme's ``padding: 44px 80px 70px``.
``Cursor`` tracks the current top and advances by each block's estimated height;
``estimate_text_height`` is the rough wrapped-line heuristic that drives it.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

from pptx.util import Emu, Inches, Pt

from lecturekit import model

# Theme padding (themes/basic-office.css `section`): 44px top, 80px sides, 70px bottom.
_PAD_TOP = 44
_PAD_SIDE = 80
_PAD_BOTTOM = 70

# python-pptx length units.
_EMU_PER_PT = 12700

# Default body line-height (theme `section { line-height: 1.4 }`).
LINE_HEIGHT = 1.4

# Glyph advance as a fraction of the font size — a coarse stand-in for real text
# metrics. Latin/proportional glyphs average ~0.5em; CJK glyphs are full-width
# (~1em), so a line of Chinese fits about half as many characters as Latin. The
# split matters: treating CJK as 0.5em badly underestimates line count and lets
# a wrapped title overlap the content below it.
_LATIN_GLYPH_EM = 0.5
_WIDE_GLYPH_EM = 1.0


def _is_wide(ch: str) -> bool:
    """Whether a character renders full-width (CJK ideographs, kana, CJK punct)."""
    o = ord(ch)
    return (
        0x1100 <= o <= 0x115F        # Hangul Jamo
        or 0x2E80 <= o <= 0x303E     # CJK radicals + Kangxi + CJK symbols/punct
        or 0x3041 <= o <= 0x33FF     # kana, CJK compat
        or 0x3400 <= o <= 0x9FFF     # CJK ideographs (ext A + unified)
        or 0xF900 <= o <= 0xFAFF     # CJK compat ideographs
        or 0xFF00 <= o <= 0xFF60     # fullwidth forms
        or 0xFFE0 <= o <= 0xFFE6     # fullwidth signs
    )


def _text_width_em(text: str) -> float:
    """Approximate rendered width of ``text`` in em units (CJK-aware)."""
    return sum(_WIDE_GLYPH_EM if _is_wide(ch) else _LATIN_GLYPH_EM for ch in text)


def _px_in(px: float) -> int:
    return Emu(Inches(px / 96))


@dataclass
class Layout:
    """Slide size and content box, all in EMU."""

    width: int
    height: int
    content_left: int
    content_top: int
    content_width: int
    content_bottom: int

    @classmethod
    def from_ratio(cls, ratio: str) -> "Layout":
        px_w, px_h = model.RATIOS[ratio]
        return cls(
            width=_px_in(px_w),
            height=_px_in(px_h),
            content_left=_px_in(_PAD_SIDE),
            content_top=_px_in(_PAD_TOP),
            content_width=_px_in(px_w - 2 * _PAD_SIDE),
            content_bottom=_px_in(px_h - _PAD_BOTTOM),
        )


class Cursor:
    """A vertical cursor within a slide's content box, advancing per block."""

    def __init__(self, layout: Layout):
        self.layout = layout
        self.top = layout.content_top

    def place(self, height: int) -> int:
        """Return the current top, then advance the cursor by ``height`` (EMU)."""
        start = self.top
        self.top += height
        return start

    def advance(self, height: int) -> None:
        self.top += height

    def remaining(self) -> int:
        return self.layout.content_bottom - self.top


def fit_within(
    width: int, height: int, max_width: int, max_height: int
) -> tuple[int, int]:
    """Scale ``(width, height)`` down to fit both maxima, preserving aspect ratio.

    Mirrors the viewer's ``max-width: 100%`` + ``max-height: 100%`` on images:
    the tighter of the two axis ratios wins, and an image already within budget
    is left unchanged (never scaled up).
    """
    scale = min(1.0, max_width / width, max_height / height)
    return round(width * scale), round(height * scale)


def estimate_text_height(
    text: str, *, font_pt: float, width: int, line_height: float = LINE_HEIGHT
) -> int:
    """Estimate a wrapped text block's height (EMU) from its character count.

    Coarse: characters per line = box width / average glyph advance; the line
    count is the character count divided by that, at least one. Drives where the
    next block starts; PowerPoint re-fits the actual text height on open.
    """
    width_pt = width / _EMU_PER_PT
    text_pt = _text_width_em(text) * font_pt
    lines = max(1, math.ceil(text_pt / width_pt))
    return Emu(Pt(lines * font_pt * line_height))


def estimate_text_width(text: str, *, font_pt: float) -> int:
    """Estimate one unwrapped line's width (EMU) from its character count.

    The counterpart of :func:`estimate_text_height`, and equally coarse — it
    reuses the same CJK-aware em metric. Used to size a shape that must hug its
    text (a highlight chip) rather than span the content column.
    """
    return Emu(Pt(_text_width_em(text) * font_pt))
