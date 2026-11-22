"""A formula becomes a native PowerPoint equation (OMML) in the pptx export.

The deck's own MathJax reads the TeX and writes MathML; `omml` turns that
MathML into OMML, one function for each element.
"""

import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest
from lxml import etree
from pptx import Presentation

from lecturekit import Lecture
from lecturekit.markup import Reader, document
from lecturekit.renderers.pptx import PptxRenderer, omml
from lecturekit.renderers.pptx.omml import A, M, plain_text
from lecturekit.renderers.pptx.text import flow


def mathml(tex, display=False):
    with Reader() as read:
        blocks = read.markdown(f"$$\n{tex}\n$$" if display else f"${tex}$")
    [formula] = document.phrasing(blocks)
    return formula.math


def convert(math, display=False):
    node = omml.to_omml(math, size_pt=20, color="000000", display=display)
    return node.find(f".//{{{M}}}" + ("oMathPara" if display else "oMath"))


def to_omml(tex, display=False):
    return convert(mathml(tex, display), display)


def xml(tex, display=False):
    return etree.tostring(to_omml(tex, display), encoding="unicode")


def tags(tex, display=False):
    """Local names of the OMML nodes, so a test can assert on structure."""
    return [etree.QName(n).localname for n in to_omml(tex, display).iter()]


def said(node):
    return "".join(t.text for t in node.iter(f"{{{M}}}t"))


def values(node, name):
    return [found.get(f"{{{M}}}val") for found in node.iter(f"{{{M}}}{name}")]


class TokenTest(unittest.TestCase):
    """What was typed as commands and spaces arrives as symbols."""

    def test_commands_and_characters_become_symbols(self):
        self.assertEqual(said(to_omml("x^2")), "x2")
        fraction = to_omml(r"\frac{a}{b}").find(f"{{{M}}}f")
        self.assertEqual(said(fraction.find(f"{{{M}}}num")), "a")
        self.assertEqual(said(fraction.find(f"{{{M}}}den")), "b")

    def test_the_space_that_ends_a_control_word_is_no_space(self):
        # `\times D` is TeX for two symbols with no gap between them.
        self.assertEqual(said(to_omml(r"B\times D")), "B×D")

    def test_typed_spaces_are_tex_s_and_a_spacing_command_is_a_space(self):
        self.assertEqual(said(to_omml("a = b")), "a=b")
        self.assertEqual(said(to_omml(r"a\quad b")), "a\u2003b")


class StructureTest(unittest.TestCase):
    def test_subscript_and_superscript(self):
        self.assertIn("sSub", tags(r"x_i"))
        self.assertIn("sSup", tags(r"x^2"))

    def test_both_scripts_fold_into_one_base(self):
        structure = tags(r"C_{a}^{b}")
        self.assertIn("sSubSup", structure)
        self.assertNotIn("sSub", structure)

    def test_scripts_before_the_base(self):
        node = to_omml(r"\sideset{_a^b}{}\sum x").find(f".//{{{M}}}sPre")
        self.assertEqual([etree.QName(part).localname for part in node],
                         ["sPrePr", "sub", "sup", "e"])

    def test_fraction_and_root(self):
        self.assertIn("f", tags(r"\frac{a}{b}"))
        self.assertIn("rad", tags(r"\sqrt{x}"))
        root = to_omml(r"\sqrt[3]{x}").find(f"{{{M}}}rad")
        self.assertEqual(said(root.find(f"{{{M}}}deg")), "3")
        self.assertEqual(values(to_omml(r"\binom{n}{k}"), "type"), ["noBar"])

    def test_big_operator_carries_its_limits(self):
        out = xml(r"\sum_{i=1}^{N}x_i", display=True)
        self.assertIn("nary", tags(r"\sum_{i=1}^{N}x_i"))
        self.assertIn("∑", out)
        self.assertIn("undOvr", out)  # limits stack above and below

    def test_limits_move_beside_the_operator_in_a_line_as_in_the_deck(self):
        self.assertEqual(values(to_omml(r"\sum_{i}x_i"), "limLoc"), ["subSup"])
        self.assertEqual(values(to_omml(r"\sum\limits_{i}x_i"), "limLoc"), ["undOvr"])
        self.assertIn("limLow", tags(r"\lim_{n} a", display=True))
        self.assertIn("sSub", tags(r"\lim_{n} a"))

    def test_the_sum_body_stops_at_a_relation(self):
        # `O(i)` belongs under the ∑; `= O(N)` does not.
        node = to_omml(r"\sum_{i=1}^{N}O(i) = O(N)")
        nary = node.find(f"{{{M}}}nary")
        self.assertEqual(said(nary.find(f"{{{M}}}e")), "O(i)")

    def test_brackets_shield_what_would_end_the_body(self):
        for tex, body in ((r"\sum_{i} f(a=b, c) = d", "f(a=b,c)"),
                          (r"\sum_{i} [a_i] + b, c", "[ai]+b"),
                          (r"(\sum_{i} a) + b", "a")):
            with self.subTest(tex=tex):
                nary = to_omml(tex).find(f"{{{M}}}nary")
                self.assertEqual(said(nary.find(f"{{{M}}}e")), body)

    def test_left_right_becomes_a_sized_delimiter(self):
        out = xml(r"\left(\frac{a}{b}\right)")
        self.assertIn("d", tags(r"\left(\frac{a}{b}\right)"))
        self.assertIn('begChr', out)

    def test_a_group_inside_a_delimiter_does_not_swallow_the_closer(self):
        # Regression: the sum must not eat \right, leaving an unclosed paren.
        node = to_omml(r"\left(\sum_{i} x_i\right) + 1")
        self.assertEqual(values(node, "endChr"), [")"])
        self.assertEqual(said(node.find(f"{{{M}}}d")).count("+"), 0)
        self.assertTrue(said(node).endswith("+1"))

    def test_an_invisible_side_and_a_middle_bar(self):
        self.assertEqual(values(to_omml(r"\left\{x\right."), "endChr"), [""])
        node = to_omml(r"\left(a \middle| b\right)")
        self.assertEqual(values(node, "sepChr"), ["|"])
        self.assertEqual(len(node.find(f"{{{M}}}d").findall(f"{{{M}}}e")), 2)

    def test_delimiters_that_do_not_grow_stay_symbols(self):
        self.assertNotIn("d", tags("f(x) + |y|"))

    def test_accents(self):
        for tex, mark in ((r"\hat{w}", "\u0302"), (r"\widehat{abc}", "\u0302"),
                          (r"\tilde{n}", "\u0303"), (r"\vec{x}", "\u20d7")):
            with self.subTest(tex=tex):
                node = to_omml(tex)
                self.assertIn("acc", [etree.QName(n).localname for n in node.iter()])
                self.assertEqual(values(node, "chr"), [mark])
        self.assertEqual(values(to_omml(r"\overline{AB}"), "pos"), ["top"])
        self.assertIn("bar", tags(r"\underline{AB}"))

    def test_a_brace_over_or_under_carries_its_label(self):
        node = to_omml(r"\underbrace{a+b}_{\text{键}}", display=True)
        self.assertEqual(values(node, "chr"), ["⏟"])
        self.assertEqual(values(node, "pos"), ["bot"])
        self.assertEqual(said(node.find(f".//{{{M}}}lim")), "键")
        self.assertEqual(values(to_omml(r"\overbrace{ab}^{n}", display=True), "pos"), ["top"])

    def test_aligned_rows_keep_their_columns(self):
        node = to_omml(r"\begin{aligned}a&=b+1\\cc&=d\end{aligned}", display=True)
        self.assertEqual(len(node.findall(f".//{{{M}}}mr")), 2)
        self.assertEqual(values(node, "mcJc"), ["right", "left"])
        self.assertEqual(values(node, "cGp"), ["0"])  # nothing between a and =

    def test_cases_are_a_matrix_in_one_brace(self):
        node = to_omml(r"\begin{cases}1&x>0\\0&\text{否则}\end{cases}")
        self.assertEqual(values(node, "begChr"), ["{"])
        self.assertEqual(values(node, "endChr"), [""])
        self.assertEqual(values(node, "mcJc"), ["left", "left"])

    def test_function_names_are_upright(self):
        out = xml(r"\operatorname{softmax}(x)")
        self.assertIn('<m:sty m:val="p"/>', out)
        self.assertIn("softmax", out)

    def test_a_named_operator_is_spaced_from_its_argument(self):
        self.assertEqual(said(to_omml(r"n\log n")), "n\u2009log\u2009n")
        self.assertEqual(said(to_omml(r"\log(n)")), "log(n)")

    def test_a_single_letter_is_italic(self):
        self.assertEqual(values(to_omml("x"), "sty"), ["i"])

    def test_blackboard_bold_is_double_struck_and_upright(self):
        out = xml(r"\mathbb{R}")
        self.assertIn('double-struck', out)
        self.assertIn('<m:sty m:val="p"/>', out)

    def test_words_in_a_formula_are_normal_text(self):
        self.assertIn("nor", tags(r"\text{若 } x"))
        self.assertIn("若", said(to_omml(r"\text{若 } x")))

    def test_symbols_become_their_unicode_character(self):
        self.assertIn("×", xml(r"B\times D"))
        self.assertIn("≈", xml(r"a\approx b"))
        self.assertIn("⊕", xml(r"a\oplus b"))

    def test_a_colour_rides_the_runs(self):
        node = to_omml(r"\color{red}{z}")
        self.assertEqual([c.get("val") for c in node.iter(f"{{{A}}}prstClr")], ["red"])

    def test_display_math_is_centered(self):
        self.assertIn('<m:jc m:val="center"/>', xml(r"x=1", display=True))

    def test_plain_text_reads_as_the_formula(self):
        self.assertEqual(plain_text(mathml(r"O(N^2)")), "O(N^2)")
        self.assertEqual(plain_text(mathml(r"B\times D")), "B×D")

    def test_a_stacked_formula_is_measured_taller(self):
        self.assertEqual(omml.visual_lines(mathml("a+b")), 1.0)
        self.assertGreater(omml.visual_lines(mathml(r"\frac{a}{b}")), 2.0)
        self.assertGreater(omml.visual_lines(
            mathml(r"\begin{aligned}a&=b\\c&=d\\e&=f\end{aligned}", True)), 2.9)


# One sample for each element the converter has a function for.
ELEMENTS = {
    "math": "", "mrow": "<mrow><mi>x</mi></mrow>",
    "mstyle": "<mstyle><mi>x</mi></mstyle>", "mpadded": "<mpadded><mi>x</mi></mpadded>",
    "merror": "<merror><mtext>x</mtext></merror>",
    "mi": "<mi>x</mi>", "mn": "<mn>1</mn>", "mo": "<mo>+</mo>", "ms": "<ms>s</ms>",
    "mtext": "<mtext>word</mtext>", "mspace": '<mspace width="1em"/>',
    "mglyph": '<mglyph alt="g"/>',
    "mfrac": "<mfrac><mi>a</mi><mi>b</mi></mfrac>", "msqrt": "<msqrt><mi>a</mi></msqrt>",
    "mroot": "<mroot><mi>a</mi><mn>3</mn></mroot>",
    "msub": "<msub><mi>a</mi><mi>b</mi></msub>", "msup": "<msup><mi>a</mi><mi>b</mi></msup>",
    "msubsup": "<msubsup><mi>a</mi><mi>b</mi><mi>c</mi></msubsup>",
    "munder": "<munder><mi>a</mi><mi>b</mi></munder>",
    "mover": "<mover><mi>a</mi><mi>b</mi></mover>",
    "munderover": "<munderover><mi>a</mi><mi>b</mi><mi>c</mi></munderover>",
    "mmultiscripts": "<mmultiscripts><mi>a</mi><mi>b</mi><none/><mprescripts/><none/><mi>c</mi></mmultiscripts>",
    "none": "<none/>", "mprescripts": "<mprescripts/>",
    "mtable": "<mtable><mtr><mtd><mi>a</mi></mtd></mtr></mtable>",
    "mtr": "<mtr><mtd><mi>a</mi></mtd></mtr>",
    "mlabeledtr": "<mlabeledtr><mtd><mn>1</mn></mtd><mtd><mi>a</mi></mtd></mlabeledtr>",
    "mtd": "<mtd><mi>a</mi></mtd>",
    "menclose": '<menclose notation="box"><mi>a</mi></menclose>',
    "mphantom": "<mphantom><mi>a</mi></mphantom>",
    "mfenced": "<mfenced><mi>a</mi><mi>b</mi></mfenced>",
    "semantics": "<semantics><mi>a</mi><annotation>a</annotation></semantics>",
    "maction": "<maction><mi>a</mi></maction>",
    "maligngroup": "<maligngroup/>", "malignmark": "<malignmark/>",
    "annotation": "<annotation>a</annotation>", "annotation-xml": "<annotation-xml/>",
}


def test_every_element_the_converter_knows_has_a_sample():
    assert set(ELEMENTS) == set(omml.DRAW)


@pytest.mark.parametrize("sample", ELEMENTS.values(), ids=list(ELEMENTS))
def test_each_element_converts(sample):
    math = ET.fromstring(f"<math>{sample}</math>")
    assert convert(math).tag == f"{{{M}}}oMath"
    assert isinstance(plain_text(math), str)
    assert omml.visual_lines(math) >= 1


def test_an_element_without_a_form_is_an_error_not_a_loss():
    with pytest.raises(ValueError, match="mstack"):
        convert(ET.fromstring("<math><mstack/></math>"))


def test_an_unknown_command_keeps_what_the_deck_shows_and_is_named(capfd):
    assert r"\badcmd" in said(to_omml(r"\badcmd + 1"))
    warning = capfd.readouterr().err
    assert "lecturekit: math:" in warning and r"\badcmd + 1" in warning


def test_a_formula_mathjax_cannot_read_keeps_its_source_and_is_named(capfd):
    assert r"\frac{1}{" in said(to_omml(r"\frac{1}{"))
    assert r"$\frac{1}{$" in capfd.readouterr().err


class ParseTest(unittest.TestCase):
    def paras(self, md):
        with Reader() as read:
            return flow(read.slide(md))

    def math(self, para):
        return [run.text for run in para.runs if run.math]

    def test_inline_math_becomes_a_math_run(self):
        [para] = self.paras("每次 forward 是 $O(N)$")
        self.assertEqual(self.math(para)[-1], "O(N)")

    def test_math_inside_an_autobolded_headline_is_still_parsed(self):
        # The DSL bolds a flush-left headline, wrapping any $…$ with it.
        [para] = self.paras("**第 $i$ 次 forward**")
        self.assertIn("i", self.math(para))

    def test_display_math_is_its_own_paragraph(self):
        # `$$` is indented by one space: the DSL's escape from auto-bold, which
        # would otherwise wrap the fence itself in asterisks.
        paras = self.paras("前文\n $$\n C = O(N)\n $$\n后文")
        self.assertEqual([p.display for p in paras], [False, True, False])
        self.assertEqual(self.math(paras[1]), ["C=O(N)"])

    def test_a_multi_line_display_formula_stays_one_paragraph(self):
        paras = self.paras(" $$\n C\n = O(N)\n $$")
        self.assertEqual(len(paras), 1)
        self.assertEqual(self.math(paras[0]), ["C=O(N)"])


class RenderTest(unittest.TestCase):
    def deck(self, body_md):
        lecture = Lecture(id="lec", title="Deck")

        def body(p):
            p.title("Math")
            p.slide(body_md)

        lecture.page(id="math", body=body)
        tmp = Path(tempfile.mkdtemp())
        out = PptxRenderer().render(lecture.build(), tmp)
        return Presentation(str(out))

    def slide_xml(self, deck):
        return "\n".join(s._element.xml for s in deck.slides[0].shapes)

    def test_equation_is_embedded_not_printed_as_latex(self):
        deck = self.deck("成本是 $O(N^2)$")
        text = "\n".join(s.text_frame.text for s in deck.slides[0].shapes
                         if s.has_text_frame)
        self.assertNotIn("$", text)
        self.assertIn("oMath", self.slide_xml(deck))

    def test_equation_has_a_plain_text_fallback(self):
        deck = self.deck(" $$\n O(N^2)\n $$")
        self.assertIn("Fallback", self.slide_xml(deck))
        self.assertIn("O(N^2)", self.slide_xml(deck))

    def test_math_runs_carry_the_body_size(self):
        from lecturekit.renderers.pptx import theme
        deck = self.deck("成本是 $O(N)$")
        self.assertIn(f'sz="{theme.BODY_PT * 100}"', self.slide_xml(deck))


if __name__ == "__main__":
    unittest.main()
