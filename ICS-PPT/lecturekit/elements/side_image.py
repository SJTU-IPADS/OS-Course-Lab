"""The `side_image` element."""
from __future__ import annotations

from ..model import Block, element


@element
class SideImage(Block):
    """A picture in the page's image column, beside the main column."""
    KIND = "side_image"
    TARGETS = frozenset({"viewer", "pptx", "transcript"})
    HOLDABLE = False
    IN_CONTAINER = False
    REVEAL_STEP = False
    # The page draws it in the image column, not in the main column.
    IN_FLOW = False
    # Its path may name a variable; its width is laid out in Python, so not.

    def to_marp(self, ctx):
        from ..renderers.viewer.blocks import side_figure
        return [side_figure(self.content), ""]

    def footnote_marker(self, lines, numbers):
        from ..renderers.viewer import blocks
        # At the picture's corner, as on a captionless `p.image`.
        return blocks.mark_figure(lines, numbers, inside_image=True)

    def to_transcript(self, embedder):
        from ..renderers.transcript.blocks import img
        # A side column on the slide; on paper it is simply another figure.
        return (
            f'<figure class="tx-fig">'
            f'{img(self.content["src"], self.content.get("alt", ""), embedder)}</figure>'
        )
