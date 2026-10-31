"""The `notes` element."""
from __future__ import annotations

from ..model import Block, element


@element
class Notes(Block):
    """Speaker notes: the presenter view of the deck, nothing else."""
    KIND = "notes"
    TARGETS = frozenset({})
