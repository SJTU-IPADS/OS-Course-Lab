import unittest

from lecturekit import Lecture
from lecturekit.model import Block, Page, select_blocks


def _page(body):
    lecture = Lecture(id="l", title="T")
    lecture.page("p", body=body)
    return lecture.build().children[0]


class ProseBlockTest(unittest.TestCase):
    def test_prose_emits_a_prose_block(self):
        def body(p):
            p.title("T")
            p.prose("a paragraph")

        page = _page(body)
        self.assertEqual([b.kind for b in page.blocks], ["prose"])
        self.assertEqual(page.blocks[0].content, "a paragraph")

    def test_handout_is_a_deprecated_alias_for_prose(self):
        def body(p):
            p.title("T")
            p.handout("legacy")

        self.assertEqual([b.kind for b in _page(body).blocks], ["prose"])

    def test_prose_is_not_autobolded(self):
        def body(p):
            p.title("T")
            p.prose("headline\n- bullet")

        self.assertEqual(_page(body).blocks[0].content, "headline\n- bullet")


class SelectBlocksOverrideTest(unittest.TestCase):
    def page_with(self, *blocks):
        return Page(id="p", title="T", blocks=list(blocks))

    def test_only_forces_a_block_in_despite_the_renderer_table(self):
        block = Block(kind="slide", content="x", only={"latex"})
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "latex", {"prose"}), [block])

    def test_only_still_excludes_other_targets(self):
        block = Block(kind="slide", content="x", only={"latex"})
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "viewer", {"slide"}), [])

    def test_except_still_excludes_a_kind_in_the_table(self):
        block = Block(kind="slide", content="x", except_={"viewer"})
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "viewer", {"slide"}), [])

    def test_kind_outside_the_table_is_still_skipped_without_only(self):
        block = Block(kind="notes", content="x")
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "viewer", {"slide"}), [])


class DisableBlockTest(unittest.TestCase):
    def page_with(self, *blocks):
        return Page(id="p", title="T", blocks=list(blocks))

    def test_disable_marks_the_block_and_returns_the_handle(self):
        seen = {}

        def body(p):
            p.title("T")
            seen["handle"] = p.prose("draft")
            seen["returned"] = seen["handle"].disable()

        page = _page(body)
        self.assertTrue(page.blocks[0].disabled)
        self.assertIs(seen["returned"], seen["handle"])

    def test_a_block_is_enabled_by_default(self):
        def body(p):
            p.title("T")
            p.prose("kept")

        self.assertFalse(_page(body).blocks[0].disabled)

    def test_disabled_block_reaches_no_renderer(self):
        block = Block(kind="prose", content="x", disabled=True)
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "latex", {"prose"}), [])

    def test_disable_beats_an_only_override(self):
        block = Block(kind="slide", content="x", only={"latex"}, disabled=True)
        page = self.page_with(block)
        self.assertEqual(select_blocks(page, "latex", {"prose"}), [])

    def test_disable_chains_after_a_footnote(self):
        def body(p):
            p.title("T")
            p.prose("draft").footnote("src").disable()

        block = _page(body).blocks[0]
        self.assertTrue(block.disabled)
        self.assertEqual(block.footnotes, ("src",))
