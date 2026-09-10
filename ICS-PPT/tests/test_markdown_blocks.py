import unittest

from lecturekit.model import Block
from lecturekit.renderers.viewer.blocks import render_block


class RenderBlockTest(unittest.TestCase):
    def test_slide_block_is_stripped_text(self):
        block = Block(kind="slide", content="\n## Hi\n\n- a\n")
        self.assertEqual(render_block(block), ["## Hi\n\n- a", ""])

    def test_code_block_fenced_with_language(self):
        block = Block(kind="code", content={"language": "python", "content": "\nprint(1)\n"})
        self.assertEqual(render_block(block), ["```python", "print(1)", "```", ""])

    def test_link_block(self):
        block = Block(kind="link", content={"label": "L", "url": "https://x"})
        self.assertEqual(
            render_block(block),
            ['- <a href="https://x" target="_blank" rel="noopener noreferrer">L</a>', ""],
        )

    def test_image_block_is_a_figure_with_centered_caption(self):
        block = Block(kind="image", content={"alt": "a", "src": "assets/x.svg", "caption": "c"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure"><img src="assets/x.svg" alt="a"><figcaption>c</figcaption></figure>', ""],
        )

    def test_image_block_with_width_emits_inline_style(self):
        block = Block(kind="image", content={"alt": "a", "src": "x.svg", "width": "60%"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure lk-figure--sized"><img src="x.svg" alt="a" style="width:60%"></figure>', ""],
        )

    def test_image_block_with_width_and_height(self):
        block = Block(kind="image", content={"alt": "", "src": "x.svg", "width": 300, "height": 200})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure lk-figure--sized"><img src="x.svg" alt="" style="width:300px;height:200px"></figure>', ""],
        )

    def test_image_block_without_size_is_not_marked_sized(self):
        # An unsized image keeps the fit-to-box default (no --sized modifier), so
        # the theme's max-height clamp still applies and the image can't overflow.
        block = Block(kind="image", content={"alt": "", "src": "x.svg"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure"><img src="x.svg" alt=""></figure>', ""],
        )

    def test_image_block_framed_and_sized_carries_both_modifiers(self):
        block = Block(kind="image", content={"alt": "", "src": "x.svg", "framed": True, "width": "60%"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure lk-figure--framed lk-figure--sized"><img src="x.svg" alt="" style="width:60%"></figure>', ""],
        )

    def test_image_block_framed_adds_modifier_class(self):
        block = Block(kind="image", content={"alt": "", "src": "x.svg", "framed": True})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure lk-figure--framed"><img src="x.svg" alt=""></figure>', ""],
        )

    def test_image_block_caption_align_left_overrides_default(self):
        block = Block(kind="image", content={"alt": "", "src": "x.svg", "caption": "c", "caption_align": "left"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure"><img src="x.svg" alt=""><figcaption style="text-align:left">c</figcaption></figure>', ""],
        )

    def test_image_block_caption_align_center_emits_no_inline_style(self):
        block = Block(kind="image", content={"alt": "", "src": "x.svg", "caption": "c", "caption_align": "center"})
        self.assertEqual(
            render_block(block),
            ['<figure class="lk-figure"><img src="x.svg" alt=""><figcaption>c</figcaption></figure>', ""],
        )

    def test_side_image_marp_emits_split_background(self):
        block = Block(kind="side_image", content={"src": "x.png", "alt": "", "side": "right", "width": None})
        self.assertEqual(render_block(block), ["![bg right](x.png)", ""])

    def test_side_image_marp_with_width_and_alt(self):
        block = Block(kind="side_image", content={"src": "x.png", "alt": "a", "side": "left", "width": "34%"})
        self.assertEqual(render_block(block), ["![bg left:34% a](x.png)", ""])

    def test_aside_block_is_blockquote(self):
        block = Block(kind="aside", content="note")
        self.assertEqual(render_block(block), ["> note", ""])

    def _table(self, **overrides):
        content = {"headers": ["机制", "开销"], "rows": [["进程", "高"]], "align": None}
        content.update(overrides)
        return Block(kind="table", content=content)

    def test_table_block_emits_gfm_rows(self):
        self.assertEqual(
            render_block(self._table()),
            ["| 机制 | 开销 |", "| --- | --- |", "| 进程 | 高 |", ""],
        )

    def test_table_align_sets_delimiter_colons(self):
        lines = render_block(self._table(
            headers=["a", "b", "c"],
            rows=[["1", "2", "3"]],
            align=["left", "right", "center"],
        ))
        self.assertEqual(lines[1], "| :--- | ---: | :---: |")

    def test_table_cell_escapes_pipe_and_collapses_newline(self):
        lines = render_block(self._table(rows=[["a|b", "x\ny"]]))
        self.assertEqual(lines[2], "| a\\|b | x y |")

    def test_table_cell_passes_inline_markdown_through(self):
        lines = render_block(self._table(rows=[["`fork()`", "**子进程**"]]))
        self.assertEqual(lines[2], "| `fork()` | **子进程** |")

    def _sidenote(self, **overrides):
        content = {"title": "T", "text": "B", "link": None, "logo": None}
        content.update(overrides)
        return Block(kind="sidenote", content=content)

    def test_sidenote_default_logo_is_book_emoji(self):
        lines = render_block(self._sidenote())
        self.assertEqual(
            lines,
            ['<aside class="sidenote sidenote--single-line"><span class="sidenote-logo">📖</span>'
             '<p class="sidenote-body"><span class="sidenote-title">T：</span>B</p></aside>',
             ""],
        )

    def test_sidenote_multiline_body_keeps_large_logo_treatment(self):
        html = render_block(self._sidenote(text="one\n\ntwo"))[0]
        self.assertIn('<aside class="sidenote">', html)
        self.assertNotIn("sidenote--single-line", html)

    def test_sidenote_custom_glyph_logo_is_a_span(self):
        html = render_block(self._sidenote(logo="💻"))[0]
        self.assertIn('<span class="sidenote-logo">💻</span>', html)
        self.assertNotIn("<img", html)

    def test_sidenote_path_logo_is_an_img(self):
        html = render_block(self._sidenote(logo="assets/laptop.png"))[0]
        self.assertIn('<img class="sidenote-logo" src="assets/laptop.png" alt="" />', html)

    def test_sidenote_url_logo_is_an_img(self):
        html = render_block(self._sidenote(logo="https://x.test/book.svg"))[0]
        self.assertIn('<img class="sidenote-logo" src="https://x.test/book.svg" alt="" />', html)

    def test_sidenote_with_link_renders_anchor_title(self):
        html = render_block(self._sidenote(link="https://x.test"))[0]
        self.assertIn(
            '<a class="sidenote-title" href="https://x.test"'
            ' target="_blank" rel="noopener noreferrer">T：</a>',
            html,
        )

    def test_sidenote_without_link_renders_span_title(self):
        html = render_block(self._sidenote())[0]
        self.assertIn('<span class="sidenote-title">T：</span>', html)
        self.assertNotIn("<a ", html)

    def test_sidenote_first_has_no_inline_background(self):
        html = render_block(self._sidenote(), sidenote_index=0)[0]
        self.assertTrue(html.startswith('<aside class="sidenote sidenote--single-line"'))
        self.assertNotIn("var(--sidenote-", html)

    def test_sidenote_later_notes_cycle_the_wheel(self):
        self.assertIn(
            'style="background: var(--sidenote-2)"',
            render_block(self._sidenote(), sidenote_index=1)[0],
        )
        self.assertIn(
            'style="background: var(--sidenote-3)"',
            render_block(self._sidenote(), sidenote_index=2)[0],
        )
        # wheel wraps after six slots, back to slot 1
        self.assertIn(
            'style="background: var(--sidenote-1)"',
            render_block(self._sidenote(), sidenote_index=6)[0],
        )

    def test_sidenote_escapes_html(self):
        html = render_block(
            self._sidenote(title='a<b&c"', text="x<y", link='h"&<')
        )[0]
        self.assertIn("a&lt;b&amp;c&quot;：", html)
        self.assertIn("x&lt;y", html)
        self.assertIn('href="h&quot;&amp;&lt;"', html)

    def test_sidenote_body_renders_inline_markdown(self):
        html = render_block(
            self._sidenote(text="a **bold** and *em* and `code`")
        )[0]
        self.assertIn("a <strong>bold</strong> and <em>em</em> and <code>code</code>", html)

    def test_sidenote_body_keeps_whitelisted_inline_tags(self):
        html = render_block(self._sidenote(text="<u>**x**</u>"))[0]
        self.assertIn("<u><strong>x</strong></u>", html)

    def test_sidenote_body_turns_blank_lines_into_breaks(self):
        html = render_block(self._sidenote(text="one\n\ntwo"))[0]
        self.assertIn("one<br><br>two", html)

    def test_sidenote_body_still_escapes_unknown_tags(self):
        html = render_block(self._sidenote(text="x<script>y"))[0]
        self.assertIn("x&lt;script&gt;y", html)
