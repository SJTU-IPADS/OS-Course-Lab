"""``lec.bridge(...)`` / ``s.bridge(...)``: the transition page and its fallout."""

import unittest

from lecturekit import model
from lecturekit.cli import format_tree
from lecturekit.dsl import Lecture, SectionBuilder
from lecturekit.model import ValidationError
from lecturekit.renderers.viewer import (
    build_data,
    build_marp_markdown,
    build_outline_html,
)
from lecturekit.renderers.viewer.pdf import SLIDE_LINK_PREFIX
from lecturekit.renderers.transcript.renderer import kept_page_ids

TEXT = "排队模型建好了，回到 GPU：est() 从哪来？"


def _page(p_title, body_text="正文"):
    def body(p):
        p.title(p_title)
        p.slide(body_text)
    return body


def _lecture():
    """A / bridge / B — deck order 1..3."""
    lec = Lecture(id="lec-b", title="Lecture B")
    with lec.section("S") as s:
        s.page(id="a", body=_page("A"))
        s.bridge(TEXT)
        s.page(id="b", body=_page("B"))
    return lec.build()


class BridgeDsl(unittest.TestCase):
    def test_returns_none(self):
        lec = Lecture(id="l", title="L")
        self.assertIsNone(lec.bridge(TEXT))
        self.assertIsNone(lec.section("S").bridge(TEXT))

    def test_auto_ids_are_lecture_wide(self):
        lec = Lecture(id="l", title="L")
        lec.bridge("一")
        with lec.section("S1") as s1:
            s1.bridge("二")
        with lec.section("S2") as s2:
            s2.bridge("三")
        s2.page(id="p", body=_page("P"))
        ids = [page.id for page in model.flatten_pages(lec.build().children)]
        self.assertEqual(ids[:3], ["bridge-1", "bridge-2", "bridge-3"])

    def test_explicit_id(self):
        lec = Lecture(id="l", title="L")
        lec.bridge(TEXT, id="turn-to-gpu")
        built = lec.build()
        self.assertEqual(model.flatten_pages(built.children)[0].id, "turn-to-gpu")

    def test_page_shape(self):
        page = model.flatten_pages(_lecture().children)[1]
        self.assertTrue(model.is_bridge_page(page))
        self.assertEqual(page.book, "skip")
        self.assertEqual(page.title, TEXT)          # first line doubles as label
        self.assertEqual(page.blocks[0].content["text"], TEXT)

    def test_lines_are_stripped_and_blank_lines_dropped(self):
        lec = Lecture(id="l", title="L")
        lec.bridge("""
        第一行

        第二行
        """)
        page = model.flatten_pages(lec.build().children)[0]
        self.assertEqual(page.blocks[0].content["text"], "第一行\n第二行")
        self.assertEqual(page.title, "第一行")

    def test_empty_text_refused(self):
        lec = Lecture(id="l", title="L")
        with self.assertRaises(ValidationError):
            lec.bridge("   \n  ")

    def test_non_string_refused(self):
        lec = Lecture(id="l", title="L")
        with self.assertRaises(ValidationError):
            lec.bridge(None)

    def test_too_many_lines_refused(self):
        lec = Lecture(id="l", title="L")
        with self.assertRaises(ValidationError):
            lec.bridge("一\n二\n三\n四")

    def test_three_lines_allowed(self):
        lec = Lecture(id="l", title="L")
        lec.bridge("一\n二\n三")
        lec.build()

    def test_mark_refused(self):
        lec = Lecture(id="l", title="L")
        lec.bridge("<mark>不许</mark>高亮")
        with self.assertRaises(ValidationError):
            lec.build()

    def test_standalone_section_needs_explicit_id(self):
        section = SectionBuilder(id="s", title="S")
        with self.assertRaises(ValidationError):
            section.bridge(TEXT)
        section.bridge(TEXT, id="named")     # explicit id is fine


class BridgeModel(unittest.TestCase):
    def test_slide_numbers_hold(self):
        pages = model.flatten_pages(_lecture().children)
        self.assertEqual(model.slide_numbers(pages), [1, 1, 2])

    def test_leading_bridge_number_stays_positive(self):
        lec = Lecture(id="l", title="L")
        lec.bridge(TEXT)
        lec.page(id="a", body=_page("A"))
        pages = model.flatten_pages(lec.build().children)
        self.assertEqual(model.slide_numbers(pages), [1, 1])

    def test_bridge_block_must_be_alone(self):
        page = model.Page(id="p", title="t", book="skip", blocks=[
            model.Block(kind="bridge", content={"text": TEXT}),
            model.Block(kind="slide", content="正文"),
        ])
        lecture = model.Lecture(id="l", title="L", children=[page])
        with self.assertRaises(ValidationError):
            model.validate_lecture(lecture)

    def test_bridge_page_must_skip_book(self):
        page = model.Page(id="p", title="t", blocks=[
            model.Block(kind="bridge", content={"text": TEXT}),
        ])
        lecture = model.Lecture(id="l", title="L", children=[page])
        with self.assertRaises(ValidationError):
            model.validate_lecture(lecture)

    def test_bridge_block_refuses_footnotes(self):
        page = model.Page(id="p", title="t", book="skip", blocks=[
            model.Block(kind="bridge", content={"text": TEXT}, footnotes=("源",)),
        ])
        lecture = model.Lecture(id="l", title="L", children=[page])
        with self.assertRaises(ValidationError):
            model.validate_lecture(lecture)

    def test_pages_selection_by_id_and_index(self):
        lecture = _lecture()
        picked = model.flatten_pages(
            model.select_pages(lecture, "bridge-1").children
        )
        self.assertEqual([p.id for p in picked], ["bridge-1"])
        picked = model.flatten_pages(model.select_pages(lecture, "2").children)
        self.assertEqual([p.id for p in picked], ["bridge-1"])


class BridgeDeck(unittest.TestCase):
    def test_marp_slide(self):
        deck = build_marp_markdown(_lecture())
        slides = deck.split("\n\n---\n\n")
        bridge = slides[1]
        self.assertIn("<!-- _class: lk-bridge -->", bridge)
        self.assertIn("<!-- _paginate: skip -->", bridge)
        self.assertIn(f'<div class="lk-bridge-body">{TEXT}</div>', bridge)
        self.assertNotIn("# ", bridge)               # no title heading
        self.assertNotIn("_paginate: hold", bridge)

    def test_page_after_bridge_does_not_hold(self):
        deck = build_marp_markdown(_lecture())
        self.assertNotIn("_paginate: hold", deck)

    def test_reveal_mode_leaves_bridge_fully_lit(self):
        deck = build_marp_markdown(_lecture(), reveal=True)
        bridge = deck.split("\n\n---\n\n")[1]
        self.assertNotIn("reveal-block", bridge)

    def test_plain_text_is_escaped(self):
        lec = Lecture(id="l", title="L")
        lec.bridge("a < b 且 **不解析**")
        lec.page(id="p", body=_page("P"))
        deck = build_marp_markdown(lec.build())
        self.assertIn("a &lt; b 且 **不解析**", deck)


class BridgeOutline(unittest.TestCase):
    def test_tree_omits_bridge(self):
        data = build_data(_lecture())
        section = data["tree"][0]
        self.assertEqual(
            [node["id"] for node in section["children"]], ["a", "b"]
        )

    def test_pages_keep_the_slide_with_a_held_number(self):
        data = build_data(_lecture())
        self.assertEqual(
            [(p["id"], p["number"]) for p in data["pages"]],
            [("a", 1), ("bridge-1", 1), ("b", 2)],
        )

    def test_print_outline_links_step_over_the_bridge(self):
        html = build_outline_html(_lecture())
        self.assertNotIn(TEXT, html)
        # 0-based physical indices: a is 0, the bridge slide 1, so b must be 2
        self.assertIn(f'href="{SLIDE_LINK_PREFIX}0"', html)
        self.assertIn(f'href="{SLIDE_LINK_PREFIX}2"', html)


class BridgeElsewhere(unittest.TestCase):
    def test_inspect_marks_bridge(self):
        tree = format_tree(_lecture())
        self.assertIn(f"* {TEXT} (bridge-1) [bridge]", tree)

    def test_transcript_drops_bridge(self):
        self.assertEqual(kept_page_ids(_lecture()), {"a", "b"})


if __name__ == "__main__":
    unittest.main()
