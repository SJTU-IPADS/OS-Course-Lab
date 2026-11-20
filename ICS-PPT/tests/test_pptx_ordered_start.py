"""An ordered list in PPTX starts at the number its first item is written with."""

import unittest

from pptx import Presentation
from pptx.util import Inches

from lecturekit.renderers.pptx.blocks import fill_paras
from lecturekit.renderers.pptx.text import parse_markdown


def markers(md: str) -> list[str]:
    """Render `md` into a fresh text frame and return each paragraph's marker."""
    deck = Presentation()
    slide = deck.slides.add_slide(deck.slide_layouts[6])
    frame = slide.shapes.add_textbox(0, 0, Inches(8), Inches(4)).text_frame
    fill_paras(frame, parse_markdown(md))
    return [p.runs[0].text.strip() if p.runs else "" for p in frame.paragraphs]


class OrderedStartTest(unittest.TestCase):
    def test_list_from_one(self):
        self.assertEqual(markers("1. a\n2. b\n3. c"), ["1.", "2.", "3."])

    def test_list_starts_at_first_number(self):
        self.assertEqual(markers("2. b\n3. c"), ["2.", "3."])

    def test_later_numbers_count_from_the_first(self):
        # CommonMark: only the first item's number matters.
        self.assertEqual(markers("4. a\n1. b\n1. c"), ["4.", "5.", "6."])

    def test_nested_list_starts_at_its_own_number(self):
        self.assertEqual(markers("1. a\n  3. x\n  4. y\n2. b"),
                         ["1.", "3.", "4.", "2."])

    def test_paragraph_ends_the_list(self):
        self.assertEqual(markers("3. a\n\ntext\n\n1. b")[0::2], ["3.", "1."])


if __name__ == "__main__":
    unittest.main()
