"""The markers that open and close a `flex` container.

A container is two blocks around its children; :func:`lecturekit.model.layout`
reads them back into a ``Flex`` node. Neither draws anything itself.
"""
from __future__ import annotations

from ..model import Block, END_KIND, FLEX_KIND, element


class _Marker(Block):
    TARGETS = frozenset({"viewer", "pptx"})
    HOLDABLE = False
    REVEAL_STEP = False


@element
class FlexMarker(_Marker):
    """Opens a container: ``content`` is ``{"direction": ...}``."""
    KIND = FLEX_KIND


@element
class EndMarker(_Marker):
    """Closes the container opened last."""
    KIND = END_KIND
