"""Consecutive sibling pages titled the same: one outline row, own slide numbers."""

import re
import unittest

from lecturekit import model, references
from lecturekit.cli import format_tree
from lecturekit.dsl import Lecture
from lecturekit.renderers.viewer import (
    build_data,
    build_marp_markdown,
    build_outline_html,
    outline_spans,
)
from lecturekit.renderers.viewer.pdf import SLIDE_LINK_PREFIX


def _body(title, text="x"):
    def fill(p):
        p.title(title)
        p.slide(text)
    return fill


def _pages(lecture):
    return model.flatten_pages(lecture.children)


def _labels(html):
    """The page numbers the printed outline labels its rows with."""
    return re.findall(r'<span class="page-number">(\d+)</span>', html)


def _tree_pages(nodes):
    out = []
    for node in nodes:
        if node["type"] == "page":
            out.append(node)
        else:
            out.extend(_tree_pages(node["children"]))
    return out


def _run():
    """before / two pages titled "Same" / after — deck order 1..4."""
    lec = Lecture(id="lec-t", title="Lecture T")
    with lec.section("S") as s:
        s.page(id="before", body=_body("Before"))
        s.page(id="same-1", body=_body("Same", "first"))
        s.page(id="same-2", body=_body("Same", "second"))
        s.page(id="after", body=_body("After"))
    return lec.build()


class FoldTest(unittest.TestCase):
    def test_the_second_page_folds_into_the_first(self):
        self.assertEqual(model.outline_folds(_run().children), frozenset({"same-2"}))

    def test_different_titles_never_fold(self):
        lec = Lecture(id="plain", title="Plain")
        lec.page(id="one", body=_body("One"))
        lec.page(id="two", body=_body("Two"))
        self.assertEqual(model.outline_folds(lec.build().children), frozenset())

    def test_a_run_of_three_all_folds_into_the_first(self):
        lec = Lecture(id="three", title="Three")
        for index in range(1, 4):
            lec.page(id=f"p{index}", body=_body("Same"))
        self.assertEqual(
            model.outline_folds(lec.build().children), frozenset({"p2", "p3"})
        )

    def test_a_section_boundary_breaks_the_run(self):
        # Folding across it would leave the next section opening with no row.
        lec = Lecture(id="cross", title="Cross")
        with lec.section("A") as a:
            a.page(id="p1", body=_body("Same"))
        with lec.section("B") as b:
            b.page(id="p2", body=_body("Same"))
        self.assertEqual(model.outline_folds(lec.build().children), frozenset())

    def test_a_bridge_between_them_breaks_the_run(self):
        lec = Lecture(id="bridged", title="Bridged")
        lec.page(id="p1", body=_body("Same"))
        lec.bridge("换个话题")
        lec.page(id="p2", body=_body("Same"))
        self.assertEqual(model.outline_folds(lec.build().children), frozenset())

    def test_a_page_apart_from_the_run_never_folds(self):
        lec = Lecture(id="apart", title="Apart")
        lec.page(id="p1", body=_body("Same"))
        lec.page(id="middle", body=_body("Other"))
        lec.page(id="p2", body=_body("Same"))
        self.assertEqual(model.outline_folds(lec.build().children), frozenset())


class OutlineTest(unittest.TestCase):
    def test_the_run_is_one_row(self):
        rows = _tree_pages(build_data(_run())["tree"])
        self.assertEqual([row["id"] for row in rows], ["before", "same-1", "after"])

    def test_the_row_spans_both_slides(self):
        rows = {row["id"]: row for row in _tree_pages(build_data(_run())["tree"])}
        self.assertEqual(rows["same-1"]["frames"], 2)
        self.assertNotIn("frames", rows["before"])

    def test_both_pages_are_still_slides(self):
        pages = build_data(_run())["pages"]
        self.assertEqual(
            [page["id"] for page in pages], ["before", "same-1", "same-2", "after"]
        )

    def test_the_printed_outline_labels_each_row_with_its_first_slide(self):
        # before / the run / after start at slides 1 / 2 / 4 — the run is one
        # row, but its second page still counts.
        self.assertEqual(_labels(build_outline_html(_run())), ["1", "2", "4"])

    def test_the_links_address_physical_slides(self):
        # The row after the run addresses slide 3 (0-based), the slide the deck
        # really holds there.
        html = build_outline_html(_run())
        self.assertEqual(html.count(SLIDE_LINK_PREFIX), 3)
        for index in (0, 1, 3):
            self.assertIn(f'href="{SLIDE_LINK_PREFIX}{index}"', html)
        self.assertNotIn(f'href="{SLIDE_LINK_PREFIX}2"', html)


class SlideNumberTest(unittest.TestCase):
    def test_no_page_of_the_run_holds_the_number(self):
        # Every page prints its own deck position; Marp counts each one.
        self.assertNotIn("_paginate: hold", build_marp_markdown(_run()))


class AnimationTest(unittest.TestCase):
    """An animation is one unit: it folds, and is folded into, as a whole."""

    def _lecture(self):
        def animated(p):
            p.title("Same")
            p.frames("a.png", "b.png")

        lec = Lecture(id="lec-a", title="Lecture A")
        with lec.section("S") as s:
            s.page(id="still", body=_body("Same"))
            s.page(id="anim", body=animated)
            s.page(id="after", body=_body("After"))
        return lec.build()

    def test_the_animation_folds_on_its_first_frame(self):
        self.assertEqual(
            model.outline_folds(self._lecture().children), frozenset({"anim-1"})
        )

    def test_the_row_spans_the_still_page_and_every_frame(self):
        rows = {row["id"]: row for row in _tree_pages(build_data(self._lecture())["tree"])}
        self.assertEqual(list(rows), ["still", "after"])
        self.assertEqual(rows["still"]["frames"], 3)

    def test_the_row_after_the_run_counts_every_frame(self):
        # still / anim-1 / anim-2 / after: the row after the run starts at 4.
        self.assertEqual(_labels(build_outline_html(self._lecture())), ["1", "4"])


class CitationTest(unittest.TestCase):
    def test_a_run_backrefs_every_page(self):
        def cited(title):
            def fill(p):
                p.title(title)
                p.slide("x")
                p.cite(title="A Paper", year="2020")
            return fill

        lec = Lecture(id="lec-c", title="Lecture C")
        with lec.section("S") as s:
            s.page(id="p1", body=cited("Same"))
            s.page(id="p2", body=cited("Same"))
        entries = references.collect_citations(_pages(lec.build()))
        self.assertEqual([entry.pages for entry in entries], [(1, 2)])


class InspectTest(unittest.TestCase):
    def test_the_tree_marks_the_folded_page(self):
        # Every page keeps its line — it still has its own id for `--pages`.
        lines = format_tree(_run()).splitlines()
        self.assertIn("    * Same (same-1)", lines)
        self.assertIn("    * Same (same-2) [folded]", lines)


class SpansTest(unittest.TestCase):
    def test_a_pruned_run_spans_only_what_survived(self):
        lecture = model.select_pages(_run(), "same-1")
        pages = model.flatten_pages(lecture.children)
        spans = outline_spans(pages, model.outline_folds(lecture.children))
        self.assertEqual(spans, {"same-1": 1})


if __name__ == "__main__":
    unittest.main()
