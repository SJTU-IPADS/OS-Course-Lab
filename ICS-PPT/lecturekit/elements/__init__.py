"""One class per kind of block; the renderers ask each class to draw itself.

Importing this package registers every class with :func:`lecturekit.model.element`,
which is how ``Block(kind=...)`` becomes the right subclass. See the docstring of
``lecturekit.model.Block`` for what a class says about its kind.
"""
from __future__ import annotations

from .architecture import Architecture
from .aside import Aside
from .bridge import Bridge
from .code import Code
from .columns import Columns
from .cover import Cover
from .demo import Demo
from .highlight import Highlight
from .image import Image
from .link import Link
from .notes import Notes
from .prose import Prose
from .row import Row
from .side_image import SideImage
from .sidenote import Sidenote
from .slide import Slide
from .spacer import Spacer
from .table import Table
from .container import EndMarker, FlexMarker

__all__ = ["Architecture", "Aside", "Bridge", "Code", "Columns", "Cover", "Demo", "EndMarker", "FlexMarker", "Highlight", "Image", "Link", "Notes", "Prose", "Row", "SideImage", "Sidenote", "Slide", "Spacer", "Table"]
