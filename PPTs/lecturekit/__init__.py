from .dsl import Lecture, PageBuilder, SectionBuilder
from .model import Block, Section, ValidationError

Page = PageBuilder

__all__ = [
    "Block",
    "Lecture",
    "Page",
    "PageBuilder",
    "Section",
    "SectionBuilder",
    "ValidationError",
]
