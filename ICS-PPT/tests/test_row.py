import unittest

from lecturekit import Lecture
from lecturekit.model import Block, Page, ValidationError, check_block
from lecturekit.renderers.viewer.blocks import render_block
from lecturekit.renderers.viewer import render_marp_page


def _item(src="a.svg", **over):
    item = {"src": src, "alt": "", "caption": None, "width": None,
            "height": None, "framed": False, "caption_align": "center"}
    item.update(over)
    return item


def _row_block(items, caption=None):
    return Block(kind="row", content={"caption": caption, "items": items})


class RowModelTest(unittest.TestCase):
    def test_check_block_accepts_a_row_with_items(self):
        check_block(_row_block([_item("a.svg"), _item("b.svg")]), page_id="p")

    def test_check_block_rejects_empty_row(self):
        with self.assertRaises(ValidationError):
            check_block(_row_block([]), page_id="p")

    def test_check_block_rejects_blank_item_src(self):
        with self.assertRaises(ValidationError):
            check_block(_row_block([_item(src="  ")]), page_id="p")

    def test_check_block_rejects_bad_item_caption_align(self):
        with self.assertRaises(ValidationError):
            check_block(_row_block([_item(caption_align="up")]), page_id="p")

    def test_row_is_a_known_block_kind(self):
        from lecturekit.model import BLOCK_KINDS
        self.assertIn("row", BLOCK_KINDS)


class RowAuthoringTest(unittest.TestCase):
    def _block(self, body):
        lecture = Lecture(id="lec", title="L")
        lecture.page("p", body=body)
        return lecture.build().children[0].blocks[0]

    def test_row_builds_items_with_image_shape(self):
        def body(p):
            p.title("W")
            p.row(caption="对比") \
                .image("a.svg", width_px=300, caption="图1") \
                .image("b.svg", framed=True)

        block = self._block(body)
        self.assertEqual(block.kind, "row")
        self.assertEqual(block.content["caption"], "对比")
        self.assertEqual(
            block.content["items"],
            [
                {"src": "a.svg", "alt": "", "caption": "图1", "width": "300px",
                 "height": None, "framed": False, "caption_align": "center"},
                {"src": "b.svg", "alt": "", "caption": None, "width": None,
                 "height": None, "framed": True, "caption_align": "center"},
            ],
        )

    def test_row_caption_defaults_to_none(self):
        def body(p):
            p.title("W")
            p.row().image("a.svg")

        self.assertIsNone(self._block(body).content["caption"])

    def test_image_chains_and_footnote_applies_to_row(self):
        def body(p):
            p.title("W")
            p.row().image("a.svg").image("b.svg").footnote("src")

        block = self._block(body)
        self.assertEqual(len(block.content["items"]), 2)
        self.assertEqual(block.footnotes, ("src",))

    def test_empty_row_raises_on_build(self):
        def body(p):
            p.title("W")
            p.row()  # no images

        with self.assertRaises(ValidationError):
            self._block(body)

    def test_row_image_rejects_both_units_for_one_dimension(self):
        def body(p):
            p.title("W")
            p.row().image("a.svg", width_px=300, width_pct=50)

        with self.assertRaises(ValidationError):
            self._block(body)


class RowRenderTest(unittest.TestCase):
    def _html(self, items, caption=None):
        block = Block(kind="row", content={"caption": caption, "items": items})
        return "\n".join(render_block(block))

    def test_one_item_per_image(self):
        html = self._html([_item("a.svg"), _item("b.svg"), _item("c.svg")])
        self.assertIn('<figure class="lk-row">', html)
        self.assertIn('<div class="lk-row-track">', html)
        self.assertEqual(html.count("lk-row-item"), 3)

    def test_unsized_items_share_evenly(self):
        html = self._html([_item("a.svg")])
        self.assertIn("flex:1 1 0", html)

    def test_sized_item_gets_fixed_basis(self):
        html = self._html([_item("a.svg", width="300px")])
        self.assertIn("flex:0 0 300px", html)
        self.assertNotIn("flex:1 1 0", html)

    def test_item_and_row_captions_render(self):
        html = self._html([_item("a.svg", caption="图1")], caption="对比")
        self.assertIn("图1", html)
        # row-level caption is the trailing figcaption before </figure>
        self.assertIn("对比", html)

    def test_no_row_caption_when_absent(self):
        html = self._html([_item("a.svg")])
        self.assertEqual(html.count("<figcaption"), 0)

    def test_framed_item_carries_modifier(self):
        html = self._html([_item("a.svg", framed=True)])
        self.assertIn("lk-figure--framed", html)

    def test_row_is_single_line_of_html(self):
        lines = render_block(Block(kind="row",
                                   content={"caption": None,
                                            "items": [_item("a.svg")]}))
        # one content line + trailing blank, like _architecture
        self.assertEqual(lines[0].count("\n"), 0)
        self.assertEqual(lines[1], "")


class RowPageIntegrationTest(unittest.TestCase):
    def test_row_emits_flex_track_in_section(self):
        block = Block(kind="row", content={
            "caption": "对比",
            "items": [_item("a.svg", caption="图1"),
                      _item("b.svg", width="300px")],
        })
        page = Page(id="p", title="T", blocks=[block])
        md = render_marp_page(page)
        self.assertIn('<figure class="lk-row">', md)
        self.assertIn('<div class="lk-row-track">', md)
        self.assertEqual(md.count("lk-row-item"), 2)
        self.assertIn("flex:1 1 0", md)       # unsized item shares evenly
        self.assertIn("flex:0 0 300px", md)   # sized item fixed
        self.assertIn("图1", md)              # per-item caption
        self.assertIn("对比", md)             # row-level caption


class RowThemeCssTest(unittest.TestCase):
    """The flex-column section needs lk-row to be a flex item, and the track a
    flex row. Guard the CSS contract so a theme edit can't silently break it."""

    def _css(self):
        from pathlib import Path
        root = Path(__file__).resolve().parent.parent
        return (root / "themes" / "basic-office.css").read_text(encoding="utf-8")

    def _rule_body(self, css, selector):
        """The declarations inside `selector { ... }`, so an assertion pins a
        property to its own rule instead of matching anywhere in the file."""
        start = css.index(selector)
        open_brace = css.index("{", start)
        close_brace = css.index("}", open_brace)
        return css[open_brace + 1:close_brace]

    def test_row_is_flex_item_in_column(self):
        # The slide <section> is a flex column; figure.lk-row must declare itself
        # a flex item or it stretches the column. Pin the properties, not just
        # the selector — an empty `figure.lk-row {}` rule must not pass.
        body = self._rule_body(self._css(), "section > figure.lk-row {")
        self.assertIn("flex: 0 1 auto", body)
        self.assertIn("min-height: 0", body)

    def test_track_is_a_top_aligned_flex_row(self):
        body = self._rule_body(self._css(), ".lk-row-track {")
        self.assertIn("display: flex", body)
        self.assertIn("align-items: flex-start", body)
