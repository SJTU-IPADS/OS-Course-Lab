"""The sheet's stylesheet, inlined into the document.

The style itself lives in ``themes/transcript.css``, beside the deck theme it
borrows its palette from — a stylesheet is a stylesheet, not a Python string —
and is read from there with its ``@--token@`` references filled in.
"""

from __future__ import annotations

from ... import tokens

STYLESHEET = tokens.THEME_DIR / "transcript.css"

CSS = tokens.substitute(
    STYLESHEET.read_text(encoding="utf-8"), form=tokens.css
)
