"""Book-side page layout: book="merge"/"skip", book_title, and figure refs."""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path

from lecturekit import model
from lecturekit.book import BookModel
from lecturekit.dsl import Lecture
from lecturekit.renderers.latex import LatexRenderer, coverage


def _lecture():
    lec = Lecture(id="lec-x", title="Lecture X")

    def first(p):
        p.title("Slide title for the deck")
        p.slide("deck-only bullet")
        p.prose("Opening prose, see [@eval-fn].")
        p.image("a.png", caption="the eval function", ref="eval-fn")

    def second(p):
        p.title("Second slide of the same topic")
        p.prose("Merged prose paragraph.")

    def reveal(p):
        p.title("Reveal step")
        p.image("a.png", caption="dup figure")

    with lec.section("One Section") as s:
        s.page(id="first", body=first, book_title="The Book Heading")
        s.page(id="second", body=second, book="merge")
        s.page(id="reveal", body=reveal, book="skip")
    return lec.build()


def _render(lecture):
    tmp = Path(tempfile.mkdtemp())
    (tmp / "a.png").write_bytes(b"png")  # AssetCopier insists images exist
    book = BookModel(
        title="T", author=None, subtitle=None, preface=None,
        lectures=(lecture,), asset_roots={lecture.id: tmp},
    )
    stderr = io.StringIO()
    with contextlib.redirect_stderr(stderr):
        LatexRenderer().render(book, tmp)
    tex = (tmp / "chapters" / f"{lecture.id}.tex").read_text()
    return tex, stderr.getvalue()


class BookPageLayoutTest(unittest.TestCase):
    def test_book_title_overrides_the_heading(self):
        tex, _ = _render(_lecture())
        self.assertIn(r"\subsection{The Book Heading}", tex)
        self.assertNotIn("Slide title for the deck", tex)

    def test_merged_page_loses_its_heading_but_keeps_its_blocks(self):
        tex, _ = _render(_lecture())
        self.assertNotIn("Second slide of the same topic", tex)
        self.assertIn("Merged prose paragraph.", tex)

    def test_merged_page_without_prose_gets_no_todo_of_its_own(self):
        tex, _ = _render(_lecture())
        self.assertNotIn(r"\booktodo", tex)

    def test_skipped_page_is_not_in_the_book(self):
        tex, _ = _render(_lecture())
        self.assertNotIn("Reveal step", tex)
        self.assertNotIn("dup figure", tex)

    def test_coverage_counts_book_units_not_deck_pages(self):
        book = BookModel(
            title="T", author=None, subtitle=None, preface=None,
            lectures=(_lecture(),), asset_roots={"lec-x": Path(".")},
        )
        [(lecture_id, written, total)] = coverage(book)
        self.assertEqual((lecture_id, written, total), ("lec-x", 1, 1))

    def test_deck_projection_ignores_book_knobs(self):
        pages = model.flatten_pages(_lecture().children)
        self.assertEqual([page.id for page in pages], ["first", "second", "reveal"])
        self.assertEqual(pages[0].title, "Slide title for the deck")


def _disabled_prose_lecture():
    lec = Lecture(id="lec-d", title="Lecture D")

    def figonly(p):
        p.title("Figure-only page")
        p.prose("draft prose, not ready for the book").disable()
        p.image("a.png", caption="the surviving figure")

    def blank(p):
        p.title("Truly unwritten page")
        p.image("a.png", caption="another figure")

    with lec.section("S") as s:
        s.page(id="figonly", body=figonly)
        s.page(id="blank", body=blank)
    return lec.build()


class DisabledProseTest(unittest.TestCase):
    def test_disabled_prose_page_is_silent_no_todo(self):
        tex, _ = _render(_disabled_prose_lecture())
        self.assertNotIn(r"\booktodo{figonly}", tex)
        self.assertNotIn("draft prose, not ready", tex)
        self.assertIn("the surviving figure", tex)

    def test_genuinely_unwritten_page_still_gets_a_todo(self):
        tex, _ = _render(_disabled_prose_lecture())
        self.assertIn(r"\booktodo{blank}", tex)

    def test_disabled_deck_slide_is_not_prose_opt_out(self):
        # A plain deck slide, disabled, is not book text — the page still owes
        # prose, so it keeps its TODO.
        lec = Lecture(id="lec-s", title="Lecture S")

        def only_a_slide(p):
            p.title("Deck-only page")
            p.slide("a deck bullet").disable()

        with lec.section("S") as s:
            s.page(id="deckpage", body=only_a_slide)
        tex, _ = _render(lec.build())
        self.assertIn(r"\booktodo{deckpage}", tex)

    def test_opted_out_page_leaves_the_coverage_denominator(self):
        book = BookModel(
            title="T", author=None, subtitle=None, preface=None,
            lectures=(_disabled_prose_lecture(),), asset_roots={"lec-d": Path(".")},
        )
        [(lecture_id, written, total)] = coverage(book)
        self.assertEqual((lecture_id, written, total), ("lec-d", 0, 1))


class FigureRefTest(unittest.TestCase):
    def test_ref_becomes_the_figure_label(self):
        tex, _ = _render(_lecture())
        self.assertIn(r"\label{fig:lec-x-eval-fn}", tex)

    def test_prose_token_becomes_a_ref(self):
        tex, _ = _render(_lecture())
        self.assertIn(r"图~\ref{fig:lec-x-eval-fn}", tex)

    def test_cross_lecture_token_keeps_its_namespace(self):
        lec = Lecture(id="lec-y", title="Y")

        def page(p):
            p.title("T")
            p.prose("See [@lec-x:eval-fn].")

        lec.page(id="only", body=page)
        tex, stderr = _render(lec.build())
        self.assertIn(r"图~\ref{fig:lec-x-eval-fn}", tex)
        self.assertNotIn("does not match", stderr)

    def test_dangling_ref_warns(self):
        lec = Lecture(id="lec-z", title="Z")

        def page(p):
            p.title("T")
            p.prose("See [@nope].")

        lec.page(id="only", body=page)
        _, stderr = _render(lec.build())
        self.assertIn("[@nope] does not match any figure ref", stderr)

    def test_chained_ref_setter(self):
        lec = Lecture(id="lec-c", title="C")

        def page(p):
            p.title("T")
            p.image("a.png", caption="cap").ref("via-chain")
            p.prose("See [@via-chain].")

        lec.page(id="only", body=page)
        tex, _ = _render(lec.build())
        self.assertIn(r"\label{fig:lec-c-via-chain}", tex)


class ValidationTest(unittest.TestCase):
    def _page(self, body):
        lec = Lecture(id="lec-v", title="V")
        lec.page(id="only", body=body)
        return lec

    def test_ref_requires_a_caption(self):
        def body(p):
            p.title("T")
            p.image("a.png", ref="no-caption")

        with self.assertRaisesRegex(model.ValidationError, "needs a caption"):
            self._page(body).build()

    def test_ref_rejects_bad_names(self):
        def body(p):
            p.title("T")
            p.image("a.png", caption="c", ref="has space")

        with self.assertRaisesRegex(model.ValidationError, "Figure ref"):
            self._page(body).build()

    def test_duplicate_refs_collide(self):
        def body(p):
            p.title("T")
            p.image("a.png", caption="c", ref="twice")
            p.image("b.png", caption="c", ref="twice")

        with self.assertRaisesRegex(model.ValidationError, "Duplicate figure ref"):
            self._page(body).build()

    def test_ref_only_attaches_to_figures(self):
        def body(p):
            p.title("T")
            p.slide("text").ref("nope")

        with self.assertRaisesRegex(model.ValidationError, "figure block"):
            self._page(body).build()

    def test_merge_needs_an_earlier_page(self):
        lec = Lecture(id="lec-m", title="M")

        def body(p):
            p.title("T")
            p.slide("x")

        with lec.section("S") as s:
            s.page(id="lonely", body=body, book="merge")
        with self.assertRaisesRegex(model.ValidationError, "merge"):
            lec.build()

    def test_a_section_breaks_the_merge_run(self):
        lec = Lecture(id="lec-s", title="S")

        def body(p):
            p.title("T")
            p.slide("x")

        with lec.section("Outer") as outer:
            outer.page(id="host", body=body)
            outer.section("Inner").page(id="inner", body=body)
            outer.page(id="after", body=body, book="merge")
        with self.assertRaisesRegex(model.ValidationError, "merge"):
            lec.build()

    def test_unknown_book_mode(self):
        lec = Lecture(id="lec-u", title="U")

        def body(p):
            p.title("T")
            p.slide("x")

        lec.page(id="only", body=body, book="bogus")
        with self.assertRaisesRegex(model.ValidationError, "book mode"):
            lec.build()


if __name__ == "__main__":
    unittest.main()
