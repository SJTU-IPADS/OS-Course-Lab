import unittest

from pptx.util import Emu, Inches, Pt

from lecturekit.renderers.pptx.layout import (
    Cursor,
    Layout,
    estimate_text_height,
    fit_within,
)


class LayoutGeometryTest(unittest.TestCase):
    def test_16_9_slide_size_from_pixels_at_96dpi(self):
        layout = Layout.from_ratio("16:9")
        self.assertEqual(layout.width, Emu(Inches(1280 / 96)))
        self.assertEqual(layout.height, Emu(Inches(720 / 96)))

    def test_4_3_slide_size(self):
        layout = Layout.from_ratio("4:3")
        self.assertEqual(layout.width, Emu(Inches(960 / 96)))
        self.assertEqual(layout.height, Emu(Inches(720 / 96)))

    def test_content_box_subtracts_theme_padding(self):
        layout = Layout.from_ratio("16:9")
        self.assertEqual(layout.content_left, Emu(Inches(80 / 96)))
        self.assertEqual(layout.content_top, Emu(Inches(44 / 96)))
        self.assertEqual(layout.content_width, Emu(Inches((1280 - 160) / 96)))
        self.assertEqual(layout.content_bottom, Emu(Inches((720 - 70) / 96)))


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
