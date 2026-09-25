import tempfile
import unittest
from pathlib import Path

from lecturekit.book import load_book
from lecturekit.renderers.latex import LatexRenderer, coverage

FIXTURE = Path("tests/fixtures/book")


class LatexRendererTest(unittest.TestCase):
    def render(self):
        tmp = Path(tempfile.mkdtemp())
        entry = LatexRenderer().render(load_book(FIXTURE), tmp)
        return tmp, entry, entry.read_text()

    def test_writes_book_tex_chapters_and_makefile(self):
        tmp, entry, _ = self.render()
        self.assertEqual(entry, tmp / "book.tex")
        self.assertTrue((tmp / "chapters" / "lec-a.tex").exists())
        self.assertTrue((tmp / "chapters" / "lec-b.tex").exists())
        self.assertTrue((tmp / "Makefile").exists())

    def test_book_tex_includes_each_chapter_in_order(self):
        _, _, tex = self.render()
        self.assertLess(tex.index("chapters/lec-a"), tex.index("chapters/lec-b"))
        self.assertIn(r"\documentclass", tex)
        self.assertIn(r"\begin{document}", tex)
        self.assertIn(r"\end{document}", tex)
        self.assertIn(r"\tableofcontents", tex)

    def test_preface_is_an_unnumbered_chapter(self):
        _, _, tex = self.render()
        self.assertIn(r"\chapter*{前言}", tex)
        self.assertIn("Why this book exists.", tex)

    def test_lecture_is_a_chapter_section_is_a_section_page_is_a_subsection(self):
        tmp, _, _ = self.render()
        tex = (tmp / "chapters" / "lec-a.tex").read_text()
        self.assertIn(r"\chapter{Lecture A}", tex)
        self.assertIn(r"\section{First Section}", tex)
        self.assertIn(r"\subsection{A page with prose}", tex)

    def test_slide_text_never_reaches_the_book(self):
        tmp, _, _ = self.render()
        tex = (tmp / "chapters" / "lec-a.tex").read_text()
        self.assertNotIn("bullet only on slides", tex)
        self.assertIn(r"A paragraph of \textbf{book} text.", tex)

    def test_a_page_without_prose_keeps_its_heading_and_gets_a_todo(self):
        tmp, _, _ = self.render()
        tex = (tmp / "chapters" / "lec-a.tex").read_text()
        self.assertIn(r"\subsection{A page without prose}", tex)
        self.assertIn(r"\booktodo{a-bare}", tex)

    def test_a_page_with_prose_gets_no_todo(self):
        tmp, _, _ = self.render()
        tex = (tmp / "chapters" / "lec-a.tex").read_text()
        self.assertNotIn(r"\booktodo{a-intro}", tex)

    def test_top_level_page_is_a_section(self):
        tmp, _, _ = self.render()
        tex = (tmp / "chapters" / "lec-b.tex").read_text()
        self.assertIn(r"\section{B page}", tex)

    def test_headings_carry_markdown_like_the_deck_does(self):
        from lecturekit import Lecture
        from lecturekit.book import BookModel

        lecture = Lecture(id="lec-h", title="H")

        def body(p):
            p.title("A **bold** 100% title")
            p.prose("x")

        lecture.page("h", body=body)
        book = BookModel(
            title="B", author=None, subtitle=None, preface=None,
            lectures=(lecture.build(),), asset_roots={"lec-h": Path(".")},
        )
        tmp = Path(tempfile.mkdtemp())
        LatexRenderer().render(book, tmp)
        tex = (tmp / "chapters" / "lec-h.tex").read_text()
        self.assertIn(r"\section{A \textbf{bold} 100\% title}", tex)

    def test_coverage_counts_pages_with_prose(self):
        self.assertEqual(
            coverage(load_book(FIXTURE)), [("lec-a", 1, 2), ("lec-b", 1, 1)]
        )


class UnderscoreIdTest(unittest.TestCase):
    """A page id with an underscore is typeset by \\booktodo — it must be escaped."""

    def render(self):
        from lecturekit import Lecture
        from lecturekit.book import BookModel

        lecture = Lecture(id="lec-u", title="U")

        def body(p):
            p.title("No prose here")
            p.slide("bullet")

        lecture.page("bridge_0", body=body)
        book = BookModel(
            title="B", author=None, subtitle=None, preface=None,
            lectures=(lecture.build(),), asset_roots={"lec-u": Path(".")},
        )
        tmp = Path(tempfile.mkdtemp())
        LatexRenderer().render(book, tmp)
        return (tmp / "chapters" / "lec-u.tex").read_text()

    def test_booktodo_escapes_the_page_id(self):
        self.assertIn(r"\booktodo{bridge\_0}", self.render())


class ForcedSlideTest(unittest.TestCase):
    """`only=["latex"]` pulls one slide into the book as that page's prose."""

    def book(self):
        from lecturekit import Lecture
        from lecturekit.book import BookModel

        lecture = Lecture(id="lec-f", title="Forced")

        def body(p):
            p.title("Reused")
            p.slide("this bullet **is** the prose", only=["latex"])

        lecture.page("reused", body=body)
        built = lecture.build()
        return BookModel(
            title="B", author=None, subtitle=None, preface=None,
            lectures=(built,), asset_roots={"lec-f": Path(".")},
        )

    def render(self):
        tmp = Path(tempfile.mkdtemp())
        LatexRenderer().render(self.book(), tmp)
        return (tmp / "chapters" / "lec-f.tex").read_text()

    def test_forced_slide_renders_as_prose(self):
        self.assertIn(r"this bullet \textbf{is} the prose", self.render())

    def test_forced_slide_suppresses_the_todo(self):
        self.assertNotIn(r"\booktodo", self.render())

    def test_forced_slide_counts_as_coverage(self):
        self.assertEqual(coverage(self.book()), [("lec-f", 1, 1)])
