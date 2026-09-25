"""What the PPTX theme still decides for itself.

Its colours are the theme's, read through :mod:`lecturekit.tokens` — that
coupling is structural now and is covered by ``test_tokens.py``. What is left
here is the one thing PowerPoint needs and a browser does not: a single CJK
typeface, picked out of the stack the browser would have fallen through.
"""

import os
import unittest
from unittest.mock import patch

from lecturekit import tokens
from lecturekit.renderers.pptx import theme


class FontTest(unittest.TestCase):
    def test_base_font_is_the_css_stack_head(self):
        self.assertEqual(theme.FONT_BASE, tokens.families()[0])

    def test_cjk_font_is_in_the_css_stack(self):
        # PPTX carries one typeface per script, so the <a:ea> face is pulled out
        # of the stack the browser would have fallen through to for CJK.
        self.assertEqual(theme.FONT_CJK, tokens.face("--pptx-font-cjk"))
        self.assertIn(theme.FONT_CJK, tokens.families())

    def test_mono_font_is_in_the_css_stack(self):
        self.assertEqual(theme.FONT_MONO, tokens.face("--pptx-font-mono"))
        self.assertIn(theme.FONT_MONO, tokens.families("--font-mono"))

    def test_cjk_font_defaults_to_pingfang(self):
        with patch.dict(os.environ, {}, clear=True):
            self.assertEqual(theme.cjk_font(), "PingFang SC")

    def test_cjk_font_honors_env_override(self):
        with patch.dict(os.environ, {"LECTUREKIT_PPTX_CJK_FONT": "Hiragino Sans GB"}):
            self.assertEqual(theme.cjk_font(), "Hiragino Sans GB")


class SidenoteWheelTest(unittest.TestCase):
    def test_wheel_is_the_css_slots_in_order(self):
        # The renderer cycles a per-page counter through the wheel, so the order
        # decides which colour a callout gets. It is built from the theme's
        # `--sidenote-N` tokens, and a slot added there joins it here.
        self.assertEqual(len(theme.SIDENOTE_WHEEL), len(tokens.numbered("--sidenote-")))
        self.assertEqual(
            f"{theme.SIDENOTE_WHEEL[0]}", tokens.hex6("--sidenote-1")
        )


if __name__ == "__main__":
    unittest.main()
