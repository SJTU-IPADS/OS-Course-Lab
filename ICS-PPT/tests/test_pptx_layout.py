import unittest

from pptx.util import Emu, Inches, Pt

from lecturekit import model
from lecturekit.renderers.pptx.layout import (
    Cursor,
    Layout,
    estimate_text_height,
    fit_within,
    px,
)
from lecturekit.renderers.pptx.page import bands, columns, footnote_height


class LayoutGeometryTest(unittest.TestCase):
    def test_16_9_slide_size_from_pixels_at_96dpi(self):
        layout = Layout.from_ratio("16:9")
        self.assertEqual(layout.width, Emu(Inches(1280 / 96)))
        self.assertEqual(layout.height, Emu(Inches(720 / 96)))

    def test_4_3_slide_size(self):
        layout = Layout.from_ratio("4:3")
        self.assertEqual(layout.width, Emu(Inches(960 / 96)))
        self.assertEqual(layout.height, Emu(Inches(720 / 96)))

    def test_content_box_is_the_slides_padding_box(self):
        layout = Layout.from_ratio("16:9")
        self.assertEqual(layout.content_left, Emu(Inches(80 / 96)))
        self.assertEqual(layout.content_top, Emu(Inches(44 / 96)))
        self.assertEqual(layout.content_width, Emu(Inches((1280 - 160) / 96)))
        self.assertEqual(layout.content_bottom, Emu(Inches((720 - 14) / 96)))


class PageBandsTest(unittest.TestCase):
    """Title, body, footnote band; the body as one or two columns."""

    layout = Layout.from_ratio("16:9")

    def test_the_body_ends_where_the_band_begins(self):
        body, band = bands(self.layout, px(100), footnote_height([], self.layout.content_width))
        self.assertEqual(body.content_top, px(100))
        self.assertEqual(body.content_bottom, band.content_top)
        # with the band at its least height, the body ends where it always did
        self.assertEqual(body.content_bottom, px(720 - 70))
        # the band is at the bottom of the padding box, as wide as it
        self.assertEqual(band.content_bottom, px(720 - 14))
        self.assertEqual((band.content_left, band.content_width),
                         (px(80), px(1120)))

    def test_three_lines_fill_the_least_height_and_a_fourth_grows_the_band(self):
        width = self.layout.content_width
        self.assertEqual(footnote_height([], width), px(56))
        self.assertEqual(footnote_height(["a", "b", "c"], width), px(56))
        four = footnote_height(["a", "b", "c", "d"], width)
        self.assertEqual(four, px(56) + Emu(Pt(10 * 1.3)))
        # a note that wraps counts as the lines it takes
        self.assertEqual(footnote_height(["a", "b", "x" * 300], width), four)
        body, band = bands(self.layout, px(100), four)
        self.assertEqual(body.content_bottom, band.content_top)
        self.assertLess(body.content_bottom, px(720 - 70))

    def _side(self, **content):
        return model.Block(kind="side_image", content={"src": "a.png", **content})

    def test_one_column_without_a_side_image(self):
        body, _ = bands(self.layout, px(100), px(56))
        self.assertEqual(columns(body, []), (body, None))

    def test_the_image_column_takes_its_share_of_the_body(self):
        body, _ = bands(self.layout, px(100), px(56))
        for width, side in (("40%", "right"), ("30%", "left"), (None, "left"), ("480px", "right")):
            with self.subTest(width=width, side=side):
                main, column = columns(body, [self._side(width=width, side=side)])
                share = px(model.side_column_px(width, 1120))
                self.assertEqual(column.content_width, share)
                self.assertEqual(main.content_width, px(1120 - 40) - share)
                for box in (main, column):
                    self.assertEqual((box.content_top, box.content_bottom),
                                     (body.content_top, body.content_bottom))
                first, second = (column, main) if side == "left" else (main, column)
                self.assertEqual(first.content_left, px(80))
                self.assertEqual(second.content_left,
                                 first.content_left + first.content_width + px(40))
                self.assertEqual(second.content_left + second.content_width, px(1200))


class CursorTest(unittest.TestCase):
    def test_starts_at_content_top(self):
        layout = Layout.from_ratio("16:9")
        cursor = Cursor(layout)
        self.assertEqual(cursor.top, layout.content_top)

    def test_place_returns_current_top_then_advances(self):
        layout = Layout.from_ratio("16:9")
        cursor = Cursor(layout)
        start = cursor.top
        placed = cursor.place(Emu(Inches(1)))
        self.assertEqual(placed, start)
        self.assertEqual(cursor.top, start + Emu(Inches(1)))


class EstimateHeightTest(unittest.TestCase):
    def test_short_line_is_one_line_tall(self):
        h = estimate_text_height("hi", font_pt=20, width=Emu(Inches(10)))
        self.assertEqual(h, Emu(Pt(20 * 1.4)))

    def test_long_text_wraps_to_more_lines(self):
        short = estimate_text_height("hi", font_pt=20, width=Emu(Inches(10)))
        long = estimate_text_height("x" * 400, font_pt=20, width=Emu(Inches(10)))
        self.assertGreater(long, short)

    def test_cjk_is_wider_than_latin_so_wraps_taller(self):
        # full-width CJK glyphs consume ~1em vs ~0.5em for latin, so the same
        # character count wraps to more lines (the title-overlap bug)
        w = Emu(Inches(4))
        cjk = estimate_text_height("中" * 40, font_pt=20, width=w)
        latin = estimate_text_height("a" * 40, font_pt=20, width=w)
        self.assertGreater(cjk, latin)

    def test_long_mixed_title_wraps_past_one_line(self):
        # the real slide title that overlapped: at the 16:9 content width it
        # must estimate to more than a single line
        content_width = Layout.from_ratio("16:9").content_width
        one_line = estimate_text_height("标题", font_pt=26, width=content_width)
        wrapped = estimate_text_height(
            "习得端到端科研的难点 #1：很多 Techniques 是隐性的 + Context-dependent",
            font_pt=26, width=content_width,
        )
        self.assertGreater(wrapped, one_line)


class FitWithinTest(unittest.TestCase):
    def test_small_image_is_unchanged(self):
        self.assertEqual(fit_within(100, 80, 1000, 1000), (100, 80))

    def test_wide_image_is_scaled_to_max_width_keeping_ratio(self):
        w, h = fit_within(2000, 1000, 1000, 1000)
        self.assertEqual(w, 1000)
        self.assertEqual(h, 500)

    def test_tall_image_is_scaled_to_max_height_keeping_ratio(self):
        # a portrait image must not exceed the available height
        w, h = fit_within(1000, 4000, 1000, 1000)
        self.assertEqual(h, 1000)
        self.assertEqual(w, 250)

    def test_uses_the_more_restrictive_dimension(self):
        # over budget on both axes; the tighter scale (height) wins
        w, h = fit_within(2000, 8000, 1000, 1000)
        self.assertEqual(h, 1000)
        self.assertEqual(w, 250)


if __name__ == "__main__":
    unittest.main()
