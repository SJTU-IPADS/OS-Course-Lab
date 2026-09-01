import unittest

from lecturekit.autobold import autobold
from lecturekit.dsl import PageBuilder


class AutoboldTest(unittest.TestCase):
    def test_flush_left_prose_line_is_bolded(self):
        self.assertEqual(autobold("数字化系统 scale 本质要做两件事"), "**数字化系统 scale 本质要做两件事**")

    def test_indented_prose_line_is_untouched(self):
        self.assertEqual(autobold(" 这一行缩进了"), " 这一行缩进了")

    def test_trailing_whitespace_stays_outside_the_bold(self):
        self.assertEqual(autobold("性能：快不快  "), "**性能：快不快**  ")

    def test_blank_and_whitespace_only_lines_are_untouched(self):
        self.assertEqual(autobold("\n   \n"), "\n   \n")

    def test_empty_content_round_trips(self):
        self.assertEqual(autobold(""), "")

    def test_bullet_list_items_are_untouched(self):
        for line in ("- a", "* a", "+ a"):
            with self.subTest(line=line):
                self.assertEqual(autobold(line), line)

    def test_ordered_list_items_are_untouched(self):
        for line in ("1. a", "1) a", "12. a"):
            with self.subTest(line=line):
                self.assertEqual(autobold(line), line)

    def test_heading_is_untouched(self):
        self.assertEqual(autobold("## Hi"), "## Hi")

    def test_blockquote_is_untouched(self):
        self.assertEqual(autobold("> quoted"), "> quoted")

    def test_table_row_is_untouched(self):
        self.assertEqual(autobold("| a | b |"), "| a | b |")

    def test_thematic_break_is_untouched(self):
        self.assertEqual(autobold("---"), "---")

    def test_image_and_html_lines_are_untouched(self):
        self.assertEqual(autobold("![alt](x.png)"), "![alt](x.png)")
        self.assertEqual(autobold("<div>x</div>"), "<div>x</div>")

    def test_line_with_existing_emphasis_is_untouched(self):
        self.assertEqual(autobold("硬件是 **raw** byte array"), "硬件是 **raw** byte array")
        self.assertEqual(autobold("硬件是 __raw__ byte array"), "硬件是 __raw__ byte array")

    def test_fenced_code_block_contents_are_untouched(self):
        content = "before\n```python\nprint(1)\n```\nafter"
        self.assertEqual(autobold(content), "**before**\n```python\nprint(1)\n```\n**after**")

    def test_tilde_fence_is_untouched(self):
        content = "~~~\nraw text\n~~~"
        self.assertEqual(autobold(content), content)

    def test_unclosed_fence_swallows_the_rest(self):
        self.assertEqual(autobold("```\nraw"), "```\nraw")

    def test_headline_with_bullets_keeps_bullets_normal(self):
        content = "\nAI 有很多数据\n- 因为数字化服务\n  - 嵌套项\n"
        self.assertEqual(autobold(content), "\n**AI 有很多数据**\n- 因为数字化服务\n  - 嵌套项\n")


class SlideAutoboldTest(unittest.TestCase):
    def test_slide_block_content_is_autobolded(self):
        p = PageBuilder()
        p.slide("headline\n- bullet")
        self.assertEqual(p.blocks[0].content, "**headline**\n- bullet")

    def test_notes_prose_and_aside_are_not_autobolded(self):
        p = PageBuilder()
        p.notes("headline")
        p.prose("headline")
        p.aside("headline")
        self.assertEqual([b.content for b in p.blocks], ["headline"] * 3)

    def test_autobold_false_leaves_every_line_alone(self):
        p = PageBuilder()
        p.slide("第一行\n第二行\n- bullet", autobold=False)
        self.assertEqual(p.blocks[0].content, "第一行\n第二行\n- bullet")

    def test_autobold_false_still_expands_marks(self):
        p = PageBuilder()
        p.slide("a ==b== c", autobold=False)
        self.assertEqual(p.blocks[0].content, "a <mark>b</mark> c")

    def test_autobold_false_keeps_explicit_bold(self):
        p = PageBuilder()
        p.slide("这里只有 **一个词** 加粗", autobold=False)
        self.assertEqual(p.blocks[0].content, "这里只有 **一个词** 加粗")

    def test_the_choice_is_recorded_on_the_block(self):
        p = PageBuilder()
        p.slide("headline")
        p.slide("headline", autobold=False)
        self.assertEqual([b.autobold for b in p.blocks], [True, False])


if __name__ == "__main__":
    unittest.main()
