import unittest

from lecturekit.markup import Reader, document
from lecturekit.renderers.pptx.text import BULLET, Pictures, Run, flow


class FlowTest(unittest.TestCase):
    """Slide text as PowerPoint paragraphs and runs."""

    @classmethod
    def setUpClass(cls):
        cls.read = Reader()
        cls.addClassCleanup(cls.read.__exit__)

    def flow(self, md):
        return flow(self.read.slide(md))

    def test_plain_paragraph_is_one_para_one_run(self):
        blocks = self.flow("hello world")
        self.assertEqual(len(blocks), 1)
        self.assertEqual((blocks[0].marker, blocks[0].level, blocks[0].heading), ("", None, 0))
        self.assertEqual(blocks[0].runs, [Run("hello world")])

    def test_bullet_items_become_bullet_paras(self):
        blocks = self.flow("- one\n- two")
        self.assertEqual([b.marker for b in blocks], [BULLET, BULLET])
        self.assertEqual(blocks[0].runs, [Run("one")])
        self.assertEqual(blocks[1].runs, [Run("two")])

    def test_nested_bullet_carries_level(self):
        blocks = self.flow("- top\n  - child")
        self.assertEqual(blocks[0].level, 0)
        self.assertEqual(blocks[1].level, 1)

    def test_ordered_items(self):
        blocks = self.flow("1. first\n2. second")
        self.assertEqual([b.marker.strip() for b in blocks], ["1.", "2."])
        self.assertEqual(blocks[0].runs, [Run("first")])

    def test_ordered_item_keeps_its_number(self):
        blocks = self.flow("2. second\n3. third")
        self.assertEqual([b.marker.strip() for b in blocks], ["2.", "3."])

    def test_headings_carry_level(self):
        blocks = self.flow("## Section\n### Sub")
        self.assertEqual(blocks[0].heading, 2)
        self.assertEqual(blocks[1].heading, 3)

    def test_inline_bold_italic_code(self):
        runs = self.flow("a **b** c *d* `e`")[0].runs
        self.assertEqual(
            runs,
            [
                Run("a "),
                Run("b", bold=True),
                Run(" c "),
                Run("d", italic=True),
                Run(" "),
                Run("e", code=True),
            ],
        )

    def test_inline_link(self):
        runs = self.flow("see [docs](https://x.test) now")[0].runs
        self.assertEqual(
            runs,
            [Run("see "), Run("docs", link="https://x.test"), Run(" now")],
        )

    def test_blank_line_separates_paragraphs(self):
        blocks = self.flow("one\n\ntwo")
        self.assertEqual([b.runs for b in blocks], [[Run("one")], [Run("two")]])

    def test_what_the_old_subset_did_not_know(self):
        self.assertEqual(
            self.flow(r"~~gone~~ ***both*** a &amp; b \* <b>tag</b>")[0].runs,
            [Run("gone", strike=True), Run(" "), Run("both", bold=True, italic=True),
             Run(" a & b * "), Run("tag", bold=True)],
        )

    def test_a_listing_and_a_table_stand_between_the_paragraphs(self):
        parts = self.flow("before\n\n```c\nint x;\n```\n\n| a |\n|---|\n| b |\n\nafter")
        self.assertEqual([type(p).__name__ for p in parts], ["Para", "Code", "Table", "Para"])

    def test_a_picture_splits_its_paragraph(self):
        before, pictures, after = self.flow('see <img src="a.png"><img src="b.png"> here')
        self.assertIsInstance(pictures, Pictures)
        self.assertEqual([image.src for image in pictures.images], ["a.png", "b.png"])
        self.assertEqual(("".join(r.text for r in before.runs).strip(),
                          "".join(r.text for r in after.runs).strip()), ("see", "here"))

    def test_an_item_that_opens_with_a_listing_keeps_its_marker(self):
        item, code = self.flow("- ```c\n  x;\n  ```")
        self.assertEqual((item.marker, item.runs), (BULLET, []))
        self.assertIsInstance(code, document.Code)


if __name__ == "__main__":
    unittest.main()
