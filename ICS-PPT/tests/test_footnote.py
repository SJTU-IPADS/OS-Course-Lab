import unittest

from lecturekit import Lecture
from lecturekit.model import Block, ValidationError
from lecturekit.renderers.viewer import render_marp_page
from lecturekit.renderers.viewer.blocks import render_block
from lecturekit.serialize import lecture_to_dict


class FootnoteAuthoringTest(unittest.TestCase):
    """A footnote is chained onto the block it annotates."""

    def _block(self, body):
        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        return lecture.build().children[0].blocks[0]

    def test_block_method_returns_a_handle_carrying_the_footnote(self):
        def body(p):
            p.title("W")
            p.image("arch.png").footnote("来源：CSAPP 3e")

        block = self._block(body)
        self.assertEqual(block.kind, "image")
        self.assertEqual(block.footnotes, ("来源：CSAPP 3e",))

    def test_footnotes_default_to_empty(self):
        def body(p):
            p.title("W")
            p.slide("plain")

        self.assertEqual(self._block(body).footnotes, ())

    def test_footnote_stacks_onto_one_block(self):
        def body(p):
            p.title("W")
            p.slide("claim").footnote("a").footnote("b")

        self.assertEqual(self._block(body).footnotes, ("a", "b"))

    def test_empty_footnote_is_rejected(self):
        def body(p):
            p.title("W")
            p.slide("claim").footnote("")

        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()


class FootnoteMarkerTest(unittest.TestCase):
    """The annotated block carries a superscript number."""

    def test_marker_appended_inline_to_block_content(self):
        block = Block(kind="slide", content="hello", footnotes=("n",))
        self.assertEqual(
            render_block(block, footnote_numbers=(1,)),
            ['hello&#8288;<sup class="footnote-ref footnote-ref--hang">1</sup>', ""],
        )

    def test_no_numbers_leaves_block_unchanged(self):
        block = Block(kind="slide", content="hello")
        self.assertEqual(render_block(block), ["hello", ""])

    def test_several_numbers_join_with_comma(self):
        block = Block(kind="image", content={"alt": "a", "src": "x.svg"}, footnotes=("p", "q"))
        self.assertEqual(
            render_block(block, footnote_numbers=(2, 3)),
            ['<figure class="lk-figure"><span class="lk-figure-anchor">'
             '<img src="x.svg" alt="a">'
             '<sup class="footnote-ref footnote-ref--corner">2, 3</sup></span></figure>', ""],
        )

    def test_captioned_image_marker_rides_the_caption(self):
        block = Block(
            kind="image",
            content={"alt": "a", "src": "x.svg", "caption": "图1"},
            footnotes=("p",),
        )
        self.assertEqual(
            render_block(block, footnote_numbers=(1,)),
            ['<figure class="lk-figure"><img src="x.svg" alt="a">'
             '<figcaption>图1 <sup class="footnote-ref">1</sup></figcaption></figure>', ""],
        )

    def test_table_marker_is_tucked_after_the_table(self):
        block = Block(
            kind="table",
            content={"headers": ["a"], "rows": [["1"]], "align": None},
            footnotes=("n",),
        )
        self.assertEqual(
            render_block(block, footnote_numbers=(1,)),
            ["| a |", "| --- |", "| 1 |",
             '<div class="footnote-ref-tuck">'
             '<sup class="footnote-ref">1</sup></div>', ""],
        )

    def test_code_marker_is_tucked_after_the_fence(self):
        block = Block(
            kind="code", content={"language": "py", "content": "print(1)"}, footnotes=("n",)
        )
        self.assertEqual(
            render_block(block, footnote_numbers=(1,)),
            ["```py", "print(1)", "```",
             '<div class="footnote-ref-tuck">'
             '<sup class="footnote-ref">1</sup></div>', ""],
        )


class FootnotePageTest(unittest.TestCase):
    """Footnotes number sequentially across a page and collect bottom-left."""

    def _page(self, *blocks):
        from lecturekit.model import Page

        return Page(id="p", title="T", blocks=list(blocks))

    def test_numbers_run_across_blocks_and_aside_lists_them(self):
        page = self._page(
            Block(kind="slide", content="claim", footnotes=("first",)),
            Block(kind="image", content={"alt": "", "src": "x.svg"}, footnotes=("second", "third")),
        )
        md = render_marp_page(page)
        self.assertIn('claim&#8288;<sup class="footnote-ref footnote-ref--hang">1</sup>', md)
        self.assertIn('<figure class="lk-figure"><span class="lk-figure-anchor">'
                      '<img src="x.svg" alt="">'
                      '<sup class="footnote-ref footnote-ref--corner">2, 3</sup></span></figure>', md)
        self.assertIn('<aside class="footnotes">', md)
        self.assertIn('<sup class="footnote-ref">1</sup> first', md)
        self.assertIn('<sup class="footnote-ref">2</sup> second', md)
        self.assertIn('<sup class="footnote-ref">3</sup> third', md)

    def test_page_without_footnotes_has_no_aside(self):
        page = self._page(Block(kind="slide", content="x"))
        self.assertNotIn("footnotes", render_marp_page(page))

    def test_footnote_body_renders_inline_markdown(self):
        page = self._page(Block(kind="slide", content="x", footnotes=("see **bold**",)))
        self.assertIn("see <strong>bold</strong>", render_marp_page(page))


class FootnoteSerializeTest(unittest.TestCase):
    def test_footnotes_are_dumped(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.image("x.svg").footnote("a").footnote("b")

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertEqual(block["footnotes"], ["a", "b"])


if __name__ == "__main__":
    unittest.main()
