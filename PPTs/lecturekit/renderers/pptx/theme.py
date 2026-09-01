"""Design tokens for the PPTX renderer.

Colours and font families come from :mod:`lecturekit.tokens` — the deck theme's
``:root``, read once and shared — so a colour changed there changes here too.
Sizes stay local: they are estimates from the theme's element rules, since this
renderer has no browser to measure text with.
"""

from __future__ import annotations

from pptx.dml.color import RGBColor

from ... import tokens
from ...marks import TONE_ORDER


def _rgb(token: str) -> RGBColor:
    return RGBColor.from_string(tokens.hex6(token))


# --- Colors (the theme's :root) ---
BG = _rgb("--color-bg")
FG = _rgb("--color-fg")
DK2 = _rgb("--color-dk2")
LT2 = _rgb("--color-lt2")
ACCENT1 = _rgb("--color-accent1")
ACCENT2 = _rgb("--color-accent2")
ACCENT3 = _rgb("--color-accent3")
ACCENT4 = _rgb("--color-accent4")
LINK = _rgb("--color-link")
LINK_VISITED = _rgb("--color-link-visited")

# Sidenote color wheel (six soft pastels); the renderer cycles per-page so a
# stack of callouts doesn't read as one monotonous block. The slot count is the
# theme's: a seventh added there joins the wheel here without an edit.
SIDENOTE_WHEEL = [_rgb(name) for name in tokens.numbered("--sidenote-")]
SIDENOTE_BORDER = _rgb("--sidenote-border")

# `<mark>` washes: the deck's marker inks, composited onto white. PowerPoint's
# text highlight is a solid fill and nothing else, so the stroke arrives
# full-height here — the same approximation this renderer makes everywhere
# geometry is not available — but it is the same ink the deck and the book use.
MARK_WHEEL = {tone: _rgb(f"--stroke-{tone}") for tone in TONE_ORDER}

# Highlight chip ink: the chip carries its tone in the text colour, not in a
# fill behind it.
CHIP_INK = {tone: _rgb(f"--chip-{tone}") for tone in TONE_ORDER}

# Architecture boxes share this hairline (kept for the deferred arch block).
BOX_BORDER = _rgb("--box-border")

# --- Fonts ---
# PowerPoint carries one typeface per script and cannot walk a CSS stack, so
# the Latin face is the head of the theme's stack and the other two are the
# faces the theme names for it (`--pptx-font-*`, each a member of its stack).
FONT_BASE = tokens.families()[0]
FONT_MONO = tokens.face("--pptx-font-mono")
# The East Asian (<a:ea>) face. Some machines can't resolve "PingFang SC" in
# PowerPoint (it's a reserved system font), so it is overridable via
# LECTUREKIT_PPTX_CJK_FONT — e.g. "Hiragino Sans GB", the stack's next fallback.
FONT_CJK = tokens.face("--pptx-font-cjk")


def cjk_font() -> str:
    """The CJK typeface for <a:ea>: env override, else the theme's."""
    import os

    return os.environ.get("LECTUREKIT_PPTX_CJK_FONT") or FONT_CJK

# --- Sizes (pt), from the theme's element rules ---
TITLE_PT = 26       # h1
H2_PT = 22
H3_PT = 20
H4_PT = 18          # h4-h6
BODY_PT = 20        # section font-size
CODE_PT = 14        # pre: ~0.7em of 20pt
CAPTION_PT = 17     # ~0.85em
FOOTNOTE_PT = 10    # ~0.5em
SIDENOTE_PT = 18
HIGHLIGHT_PT = TITLE_PT   # .lk-highlight span is set at the slide title's size

# Title underline rule weight (pt). The theme's h1 border is 1.5px; a true
# hairline reads as invisible in PowerPoint, so the rule is a thin-but-visible
# 1.5pt — heavier looks like a banner.
TITLE_RULE_PT = 1.5

# Heading point sizes by markdown level (1-6).
HEADING_PT = {1: TITLE_PT, 2: H2_PT, 3: H3_PT, 4: H4_PT, 5: H4_PT, 6: H4_PT}
