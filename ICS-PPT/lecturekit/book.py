"""The book root: an ordered collection of lectures rendered as one document.

A ``Book`` is an *ordering*, not a document — it carries no content blocks of
its own beyond front matter. Each listed directory is loaded through the normal
lecture pipeline, so a lecture is authored once and read by both the deck and
the book (see ``renderers/latex``).
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from . import model


@dataclass(frozen=True)
class BookModel:
    title: str
    author: str | None
    subtitle: str | None
    preface: str | None
    lectures: tuple[model.Lecture, ...]
    # lecture id -> the directory its relative asset paths resolve against. A
    # lecture's images are relative to its own folder; the book merges many.
    asset_roots: dict[str, Path]
    # The translation overlay applied to this book, or None for the baseline.
    # The renderer reads it only to pick its own fixed strings (see `i18n`).
    lang: str | None = None


class Book:
    def __init__(
        self,
        *,
        title: str,
        author: str | None = None,
        subtitle: str | None = None,
    ):
        self.title = title
        self.author = author
        self.subtitle = subtitle
        self._preface: str | None = None
        self._lecture_dirs: list[str] = []

    def preface(self, text: str) -> None:
        if self._preface is not None:
            raise model.ValidationError("Book sets its preface more than once")
        self._preface = text

    def lecture(self, directory: str) -> None:
        """Append a lecture by directory, resolved relative to ``book.py``.

        Order is call order. A lecture that is not listed is not in the book,
        which is how an unfinished one is excluded.
        """
        self._lecture_dirs.append(directory)

    def build(self, base_dir: Path) -> BookModel:
        # Check the book's own shape before touching the filesystem, so an empty
        # book fails as an empty book rather than as a missing directory.
        if not self.title or not self.title.strip():
            raise model.ValidationError("Book must have a non-empty title")
        if not self._lecture_dirs:
            raise model.ValidationError("Book must contain at least one lecture")

        # Local import: cli imports the renderers, which import this module.
        from .cli import load_lecture

        lectures: list[model.Lecture] = []
        asset_roots: dict[str, Path] = {}
        for name in self._lecture_dirs:
            lecture_dir = (Path(base_dir) / name).resolve()
            lecture = load_lecture(lecture_dir)
            if lecture.id in asset_roots:
                raise model.ValidationError(f"Duplicate lecture id: {lecture.id}")
            lectures.append(lecture)
            asset_roots[lecture.id] = lecture_dir

        book = BookModel(
            title=self.title,
            author=self.author,
            subtitle=self.subtitle,
            preface=self._preface,
            lectures=tuple(lectures),
            asset_roots=asset_roots,
        )
        validate_book(book)
        return book


def validate_book(book: BookModel) -> None:
    if not book.title or not book.title.strip():
        raise model.ValidationError("Book must have a non-empty title")
    if not book.lectures:
        raise model.ValidationError("Book must contain at least one lecture")
    seen: set[str] = set()
    for lecture in book.lectures:
        if lecture.id in seen:
            raise model.ValidationError(f"Duplicate lecture id: {lecture.id}")
        seen.add(lecture.id)


def load_book(book_dir: Path) -> BookModel:
    from .cli import load_module

    book_dir = Path(book_dir)
    book_file = book_dir / "book.py"
    if not book_file.exists():
        raise FileNotFoundError(f"missing book source: {book_file}")

    module = load_module(book_file)
    if not hasattr(module, "book"):
        raise ValueError(f"{book_file} must define a variable named 'book'")

    book = module.book
    if isinstance(book, BookModel):
        return book
    if isinstance(book, Book):
        return book.build(book_dir)
    raise TypeError("'book' must be a lecturekit Book builder or model")
