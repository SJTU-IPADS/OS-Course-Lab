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

    def check_content(self, page_id: str) -> None:
        content = self.content
        px = content.get("px")
        # bool is an int subclass; reject it so True/False can't pass as a height.
        if type(px) is not int or px <= 0:
            raise model.ValidationError(
                f"Spacer px on {page_id} must be a positive integer, got {px!r}"
            )

    def to_marp(self, ctx):
        px = int(self.content["px"])
        return [f'<div class="lk-spacer" aria-hidden="true" style="height:{px}px"></div>', ""]
