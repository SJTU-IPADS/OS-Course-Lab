"""The `spacer` element."""
from __future__ import annotations

from ..model import Block, element
from .. import model


@element
class Spacer(Block):
    """A fixed vertical gap."""
    KIND = "spacer"
    TARGETS = frozenset({"viewer"})
    REVEAL_STEP = False
    VARIABLE_KEYS = frozenset({"px"})

    def check_content(self, page_id: str) -> None:
        content = self.content
        px = content.get("px")
        # A "$name" is checked against the variable it names (see
        # `variables.check_block`).
        if isinstance(px, str) and "$" in px:
            return
        # bool is an int subclass; reject it so True/False can't pass as a height.
        if type(px) is not int or px <= 0:
            raise model.ValidationError(
                f"Spacer px on {page_id} must be a positive integer, got {px!r}"
            )

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import css_length
        # An int, or the CSS a variable became on the deck (`variables.for_deck`).
        height = css_length(self.content["px"])
        return [f'<div class="lk-spacer" aria-hidden="true" style="height:{height}"></div>', ""]
