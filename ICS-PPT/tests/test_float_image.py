import unittest

from lecturekit import Lecture
from lecturekit.model import Block, Page, ValidationError, check_block
from lecturekit.renderers.viewer import render_marp_page
from lecturekit.renderers.viewer.blocks import render_block
from lecturekit.serialize import lecture_to_dict


class FloatImageModelTest(unittest.TestCase):
    """float_image is an optional attribute on Block, like footnotes."""

    def test_block_defaults_float_image_to_none(self):
        self.assertIsNone(Block(kind="slide", content="x").float_image)

    def test_check_block_accepts_a_float_image_with_src(self):
        block = Block(
            kind="slide",
            content="x",
            float_image={"src": "a.png", "alt": "", "width": "160px",
                         "height": None, "side": "right"},
        )
        check_block(block, page_id="p")  # does not raise

    def test_check_block_rejects_blank_float_image_src(self):
        block = Block(
            kind="slide",
            content="x",
            float_image={"src": "  ", "alt": "", "width": None,
                         "height": None, "side": "right"},
        )
        with self.assertRaises(ValidationError):
            check_block(block, page_id="p")


class FloatImageSerializeTest(unittest.TestCase):
    def test_float_image_is_dumped(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.slide("hi").image_right("weekly.png", width_px=160)

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertEqual(
            block["float_image"],
            {"src": "weekly.png", "alt": "", "width": "160px",
             "height": None, "side": "right"},
        )

    def test_plain_block_dumps_float_image_none(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("W")
            p.slide("hi")

        lecture.page("p", body=body)
        block = lecture_to_dict(lecture.build())["children"][0]["blocks"][0]
        self.assertIsNone(block["float_image"])


class FloatImageAuthoringTest(unittest.TestCase):
    """image_right chains onto a slide block and stores a float_image dict."""

    def _block(self, body):
        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        return lecture.build().children[0].blocks[0]

    def test_image_right_sets_float_image(self):
        def body(p):
            p.title("W")
            p.slide("核心是讲清楚三件事").image_right("weekly.png", width_px=160)

        block = self._block(body)
        self.assertEqual(block.kind, "slide")
        self.assertEqual(
            block.float_image,
            {"src": "weekly.png", "alt": "", "width": "160px",
             "height": None, "side": "right"},
        )

    def test_image_right_accepts_pct_and_alt(self):
        def body(p):
            p.title("W")
            p.slide("x").image_right("a.png", alt="diagram", width_pct=20)

        block = self._block(body)
        self.assertEqual(block.float_image["width"], "20%")
        self.assertEqual(block.float_image["alt"], "diagram")

    def test_image_right_last_call_wins(self):
        def body(p):
            p.title("W")
            (p.slide("x")
             .image_right("first.png", width_px=80)
             .image_right("second.png", width_px=160))

        block = self._block(body)
        self.assertEqual(block.float_image["src"], "second.png")
        self.assertEqual(block.float_image["width"], "160px")

    def test_image_right_chains_with_footnote(self):
        def body(p):
            p.title("W")
            p.slide("x").image_right("a.png", width_px=120).footnote("来源：内部周报")

        block = self._block(body)
        self.assertEqual(block.float_image["src"], "a.png")
        self.assertEqual(block.footnotes, ("来源：内部周报",))

    def test_image_right_on_non_slide_block_is_rejected(self):
        def body(p):
            p.title("W")
            p.code("py", "print(1)").image_right("a.png", width_px=120)

        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()

    def test_image_right_rejects_both_units_for_one_dimension(self):
        def body(p):
            p.title("W")
            p.slide("x").image_right("a.png", width_px=120, width_pct=20)

        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        with self.assertRaises(ValidationError):
            lecture.build()


class FloatImageRenderTest(unittest.TestCase):
    """_slide wraps a float:right <img> + text in a .lk-float block so the
    theme's flex-column section doesn't turn the image into a stacked flex item.
    """

    def _slide_block(self, **fi):
        image = {"src": "weekly.png", "alt": "", "width": "160px",
                 "height": None, "side": "right"}
        image.update(fi)
        return Block(kind="slide", content="核心是讲清楚三件事", float_image=image)

    def test_plain_slide_unchanged(self):
        self.assertEqual(render_block(Block(kind="slide", content="hi")), ["hi", ""])

    def test_float_image_wrapped_before_text(self):
        lines = render_block(self._slide_block())
        # <div.lk-float> , "" , <img> , "" , content , "" , </div> , ""
        self.assertEqual(lines[0], '<div class="lk-float">')
        self.assertEqual(lines[1], "")
        self.assertTrue(lines[2].startswith("<img "))
        self.assertIn("float: right", lines[2])
        self.assertIn("width: 160px", lines[2])
        self.assertNotIn("height", lines[2])      # height None → declaration omitted
        self.assertIn('src="weekly.png"', lines[2])
        self.assertEqual(lines[3], "")            # blank line: inner markdown parses
        self.assertEqual(lines[4], "核心是讲清楚三件事")
        self.assertEqual(lines[5], "")
        self.assertEqual(lines[6], "</div>")
        self.assertEqual(lines[7], "")

    def test_height_included_when_set(self):
        lines = render_block(self._slide_block(height="90px"))
        self.assertIn("height: 90px", lines[2])

    def test_float_image_escapes_src_and_alt(self):
        lines = render_block(self._slide_block(src='a"b.png', alt="x<y"))
        self.assertIn('src="a&quot;b.png"', lines[2])
        self.assertIn('alt="x&lt;y"', lines[2])

    def test_footnote_marker_lands_on_text_not_image_or_div(self):
        block = Block(kind="slide", content="claim",
                      float_image=self._slide_block().float_image, footnotes=("n",))
        lines = render_block(block, footnote_numbers=(1,))
        # marker skips the lone <img>/<div>/</div> tags and lands on the text
        self.assertNotIn("<sup", lines[2])                 # the <img> line
        self.assertIn('claim&#8288;<sup class="footnote-ref footnote-ref--hang">1</sup>', lines[4])
        self.assertEqual(lines[6], "</div>")               # closer untouched

    def test_page_integration_emits_wrapper_img_and_text(self):
        page = Page(id="p", title="T", blocks=[self._slide_block()])
        md = render_marp_page(page)
        self.assertIn('<div class="lk-float">', md)
        self.assertIn("float: right", md)
        self.assertIn("</div>", md)
        self.assertIn("核心是讲清楚三件事", md)


if __name__ == "__main__":
    unittest.main()
