import unittest

from lecturekit.renderers.pptx.text import Run, parse_markdown


class ParseMarkdownTest(unittest.TestCase):
    def test_plain_paragraph_is_one_para_one_run(self):
        blocks = parse_markdown("hello world")
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0].kind, "para")
        self.assertEqual(blocks[0].runs, [Run("hello world")])

    def test_bullet_items_become_bullet_paras(self):
        blocks = parse_markdown("- one\n- two")
        self.assertEqual([b.kind for b in blocks], ["bullet", "bullet"])
        self.assertEqual(blocks[0].runs, [Run("one")])
        self.assertEqual(blocks[1].runs, [Run("two")])

    def test_nested_bullet_carries_level(self):
        blocks = parse_markdown("- top\n  - child")
        self.assertEqual(blocks[0].level, 0)
        self.assertEqual(blocks[1].level, 1)

    def test_ordered_items(self):
        blocks = parse_markdown("1. first\n2. second")
        self.assertEqual([b.kind for b in blocks], ["ordered", "ordered"])
        self.assertEqual(blocks[0].runs, [Run("first")])

    def test_headings_carry_level(self):
        blocks = parse_markdown("## Section\n### Sub")
        self.assertEqual(blocks[0].kind, "heading")
        self.assertEqual(blocks[0].level, 2)
        self.assertEqual(blocks[1].level, 3)

    def test_inline_bold_italic_code(self):
        runs = parse_markdown("a **b** c *d* `e`")[0].runs
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
        runs = parse_markdown("see [docs](https://x.test) now")[0].runs
        self.assertEqual(
            runs,
            [Run("see "), Run("docs", link="https://x.test"), Run(" now")],
        )

    def test_blank_line_separates_paragraphs(self):
        blocks = parse_markdown("one\n\ntwo")
        self.assertEqual([b.kind for b in blocks], ["para", "para"])
        self.assertEqual(blocks[0].runs, [Run("one")])
        self.assertEqual(blocks[1].runs, [Run("two")])


if __name__ == "__main__":
    unittest.main()
