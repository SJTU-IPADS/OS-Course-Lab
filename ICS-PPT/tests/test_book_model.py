import unittest
from pathlib import Path

from lecturekit.book import Book, BookModel, load_book
from lecturekit.model import ValidationError

FIXTURE = Path("tests/fixtures/book")


class LoadBookTest(unittest.TestCase):
    def test_loads_lectures_in_declaration_order(self):
        book = load_book(FIXTURE)
        self.assertIsInstance(book, BookModel)
        self.assertEqual(book.title, "Test Book")
        self.assertEqual(book.author, "Nobody")
        self.assertEqual(book.preface, "Why this book exists.")
        self.assertEqual([lec.id for lec in book.lectures], ["lec-a", "lec-b"])

    def test_asset_roots_point_at_each_lecture_dir(self):
        book = load_book(FIXTURE)
        self.assertEqual(book.asset_roots["lec-a"], (FIXTURE / "lec_a").resolve())

    def test_missing_lecture_dir_is_an_error(self):
        book = Book(title="T")
        book.lecture("nope")
        with self.assertRaises(FileNotFoundError):
            book.build(FIXTURE)

    def test_duplicate_lecture_ids_are_rejected(self):
        book = Book(title="T")
        book.lecture("lec_a")
        book.lecture("lec_a")
        with self.assertRaisesRegex(ValidationError, "Duplicate lecture id"):
            book.build(FIXTURE)

    def test_a_book_needs_a_title(self):
        book = Book(title="  ")
        book.lecture("lec_a")
        with self.assertRaisesRegex(ValidationError, "title"):
            book.build(FIXTURE)

    def test_a_book_needs_at_least_one_lecture(self):
        with self.assertRaisesRegex(ValidationError, "at least one lecture"):
            Book(title="T").build(FIXTURE)

    def test_preface_is_optional_and_set_once(self):
        book = Book(title="T")
        book.preface("a")
        with self.assertRaisesRegex(ValidationError, "preface"):
            book.preface("b")

    def test_missing_book_py_is_an_error(self):
        with self.assertRaises(FileNotFoundError):
            load_book(FIXTURE / "lec_a")
