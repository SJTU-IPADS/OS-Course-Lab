"""The design-token layer: one palette, read by every renderer.

The tests that matter here are about the *invariant*, not about individual
colours: no renderer spells a colour out, and every target's translation of a
token (LaTeX's opaque `\\definecolor`, the transcript sheet's `#rrggbb`,
PowerPoint's `RGBColor`) comes from the same value in `themes/basic-office.css`.
"""

import re
import unittest
from pathlib import Path

from lecturekit import tokens

REPO = Path(__file__).resolve().parents[1]
PKG = REPO / "lecturekit"


class ParseTest(unittest.TestCase):
    def test_reads_the_theme_root_block(self):
        self.assertEqual(tokens.value("--color-accent1"), "#156082")

    def test_a_comment_between_declarations_is_not_read_as_a_value(self):
        # The `:root` block is half commentary; a `/* … */` swallowed into the
        # value would put prose in a \definecolor.
        for name, value in tokens.tokens().items():
            with self.subTest(name=name):
                self.assertNotIn("/*", value)

    def test_unknown_token_names_the_theme(self):
        with self.assertRaises(tokens.UnknownToken) as caught:
            tokens.value("--color-taupe")
        self.assertIn("basic-office", str(caught.exception))


class FlattenTest(unittest.TestCase):
    def test_translucent_token_is_composited_onto_white(self):
        # rgba(246, 197, 21, .45) over white — the value the book printed by
        # hand before this module computed it.
        self.assertEqual(tokens.hex6("--stroke-yellow"), "FBE596")

    def test_opaque_token_passes_through(self):
        self.assertEqual(tokens.opaque("--chip-yellow"), "#156082")

    def test_hex6_is_the_hash_less_upper_case_spelling(self):
        self.assertEqual(tokens.hex6("--chip-orange"), "C2410C")

    def test_every_colour_token_can_be_flattened(self):
        # A token a renderer cannot translate is one the deck can use and the
        # book cannot, which is exactly the drift this module exists to stop.
        for name in tokens.tokens():
            if not tokens.is_colour(name):
                continue
            with self.subTest(name=name):
                self.assertRegex(tokens.opaque(name), r"^#[0-9a-f]{6}$")

    def test_only_fonts_and_mark_geometry_are_not_colours(self):
        # Everything else in :root is paint. A new non-colour token is fine —
        # add it here so the list stays a statement of what the theme carries.
        others = {n for n in tokens.tokens() if not tokens.is_colour(n)}
        self.assertEqual(others, {
            "--font-base", "--font-mono", "--pptx-font-cjk", "--pptx-font-mono",
            "--mark-top", "--mark-bottom", "--mark-tex-raise", "--mark-tex-height",
        })


class SubstituteTest(unittest.TestCase):
    def test_fills_a_reference_in_the_form_the_target_wants(self):
        self.assertEqual(
            tokens.substitute("{HTML}{@--chip-blue@}"), "{HTML}{1769C2}"
        )
        self.assertEqual(
            tokens.substitute("color: @--chip-blue@;", form=tokens.css),
            "color: #1769c2;",
        )

    def test_a_length_or_font_passes_through_verbatim_in_either_form(self):
        self.assertEqual(tokens.substitute("@--mark-tex-height@"), "1.28ex")
        self.assertEqual(
            tokens.substitute("top: @--mark-top@;", form=tokens.css), "top: 38%;"
        )
        self.assertIn("Lato", tokens.substitute("@--font-base@", form=tokens.css))

    def test_unknown_reference_raises_instead_of_reaching_the_page(self):
        with self.assertRaises(tokens.UnknownToken):
            tokens.substitute("@--not-a-token@")


class FontTest(unittest.TestCase):
    def test_families_splits_the_stack(self):
        self.assertEqual(tokens.families()[0], "Lato")
        self.assertIn("PingFang SC", tokens.families())

    def test_face_unquotes_a_single_family(self):
        self.assertEqual(tokens.face("--pptx-font-cjk"), "PingFang SC")

    def test_numbered_lists_a_wheel_in_order_and_skips_the_border(self):
        wheel = tokens.numbered("--sidenote-")
        self.assertEqual(wheel[:2], ["--sidenote-1", "--sidenote-2"])
        self.assertNotIn("--sidenote-border", wheel)


class NoLiteralsTest(unittest.TestCase):
    """The invariant: colours live in the theme, code reads them."""

    HEX = re.compile(r"#[0-9a-fA-F]{6}\b|from_string\(\"[0-9A-Fa-f]{6}\"\)")

    def test_no_renderer_spells_a_colour_out(self):
        for path in sorted(PKG.rglob("*.py")):
            if path.name == "tokens.py":
                continue
            with self.subTest(module=path.relative_to(REPO)):
                self.assertIsNone(
                    self.HEX.search(path.read_text(encoding="utf-8")),
                    f"{path.name} holds a colour literal; take it from a token",
                )


class TargetsTest(unittest.TestCase):
    def test_latex_preamble_inks_come_from_the_theme(self):
        from lecturekit.renderers.latex import preamble

        self.assertIn(
            f"\\definecolor{{lkMarkYellow}}{{HTML}}{{{tokens.hex6('--stroke-yellow')}}}",
            preamble._MACROS,
        )
        self.assertNotIn("@--", preamble._MACROS)

    def test_latex_mark_geometry_comes_from_the_theme(self):
        from lecturekit.renderers.latex import preamble

        raise_, height = tokens.value("--mark-tex-raise"), tokens.value("--mark-tex-height")
        self.assertIn(f"\\rule[{raise_}]{{\\wd\\lkmarkbox}}{{{height}}}", preamble._MACROS)

    def test_transcript_sheet_inks_come_from_the_theme(self):
        from lecturekit.renderers.transcript.style import CSS

        self.assertIn(
            f"--highlight-yellow: {tokens.opaque('--stroke-yellow')};", CSS
        )
        self.assertIn(f"transparent {tokens.value('--mark-top')}", CSS)
        self.assertIn(tokens.value("--font-mono").split(",")[0], CSS)
        self.assertNotIn("@--", CSS)

    def test_pptx_palette_comes_from_the_theme(self):
        from pptx.dml.color import RGBColor

        from lecturekit.renderers.pptx import theme

        self.assertEqual(
            theme.MARK_WHEEL["yellow"],
            RGBColor.from_string(tokens.hex6("--stroke-yellow")),
        )
        self.assertEqual(theme.ACCENT1, RGBColor.from_string(tokens.hex6("--color-accent1")))

    def test_one_ink_reaches_every_target(self):
        # The point of the whole module: a marked keyword is the same yellow on
        # the projector, in the book, on paper and in PowerPoint.
        from pptx.dml.color import RGBColor

        from lecturekit.renderers.latex import preamble
        from lecturekit.renderers.pptx import theme
        from lecturekit.renderers.transcript.style import CSS

        ink = tokens.hex6("--stroke-yellow")
        self.assertIn(f"{{HTML}}{{{ink}}}", preamble._MACROS)
        self.assertIn(f"--highlight-yellow: #{ink.lower()};", CSS)
        self.assertEqual(theme.MARK_WHEEL["yellow"], RGBColor.from_string(ink))


if __name__ == "__main__":
    unittest.main()
