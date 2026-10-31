"""The `side_image` element."""
from __future__ import annotations

from ..model import Block, element


@element
class SideImage(Block):
    """The page's image column, drawn beside the text."""
    KIND = "side_image"
    TARGETS = frozenset({"viewer", "pptx", "transcript"})
    HOLDABLE = False
    IN_CONTAINER = False
    REVEAL_STEP = False
    IN_FLOW = False

    def to_marp(self, ctx):
        from ..renderers.viewer import blocks
        return blocks._side_image(self)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import img
        # A side column on the slide; on paper it is simply another figure.
        return (
            f'<figure class="tx-fig">'
            f'{img(self.content["src"], self.content.get("alt", ""), embedder)}</figure>'
        )
