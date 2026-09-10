import unittest

from lecturekit.renderers.latex.text import blocks, escape, inline


class EscapeTest(unittest.TestCase):
    def test_escapes_latex_specials(self):
        self.assertEqual(escape("100% & rising"), r"100\% \& rising")
        self.assertEqual(escape("a_b"), r"a\_b")
        self.assertEqual(escape("#{}"), r"\#\{\}")

    def test_escapes_backslash_and_carets_without_recursion(self):
        self.assertEqual(escape("a\\b"), r"a\textbackslash{}b")
        self.assertEqual(escape("x^2"), r"x\textasciicircum{}2")
        self.assertEqual(escape("~a"), r"\textasciitilde{}a")


class InlineTest(unittest.TestCase):
    def test_bold_italic_code(self):
        self.assertEqual(inline("**b**"), r"\textbf{b}")
        self.assertEqual(inline("*i*"), r"\textit{i}")
        self.assertEqual(inline("`c`"), r"\texttt{c}")

    def test_link(self):
        self.assertEqual(inline("[l](http://x)"), r"\href{http://x}{l}")

    def test_link_url_is_not_escaped(self):
        self.assertEqual(
            inline("[a](http://x.com/a_b?q=1%2)"),
            r"\href{http://x.com/a_b?q=1%2}{a}",
        )

    def test_link_label_is_escaped(self):
        self.assertEqual(inline("[a_b](http://x)"), r"\href{http://x}{a\_b}")

    def test_math_passes_through_untouched(self):
        self.assertEqual(inline("$x_1$"), "$x_1$")
        self.assertEqual(
            inline(r"$1.01^{365} \approx 37.8$"), r"$1.01^{365} \approx 37.8$"
        )

    def test_code_span_content_is_escaped(self):
        self.assertEqual(inline("`a_b`"), r"\texttt{a\_b}")

    def test_literal_around_math_is_escaped(self):
        self.assertEqual(inline("50% of $x_1$"), r"50\% of $x_1$")

    def test_bold_and_italic_together(self):
        self.assertEqual(inline("**a** and *b*"), r"\textbf{a} and \textit{b}")

    def test_asterisks_inside_math_are_not_emphasis(self):
        self.assertEqual(inline("$a * b * c$"), "$a * b * c$")

    def test_plain_text_is_untouched(self):
        self.assertEqual(inline("中文段落"), "中文段落")


class BlocksTest(unittest.TestCase):
    def test_paragraphs_separated_by_blank_line(self):
        self.assertEqual(blocks("one\n\ntwo"), "one\n\ntwo")

    def test_paragraph_lines_join(self):
        self.assertEqual(blocks("one\ntwo"), "one\ntwo")

    def test_bullets_become_itemize(self):
        out = blocks("- a\n- b")
        self.assertIn(r"\begin{itemize}", out)
        self.assertIn(r"\item a", out)
        self.assertIn(r"\item b", out)
        self.assertIn(r"\end{itemize}", out)

    def test_numbered_become_enumerate(self):
        out = blocks("1. a\n2. b")
        self.assertIn(r"\begin{enumerate}", out)
        self.assertIn(r"\item a", out)
        self.assertIn(r"\end{enumerate}", out)

    def test_list_items_get_inline_conversion(self):
        self.assertIn(r"\item \textbf{a}", blocks("- **a**"))

    def test_fence_becomes_lstlisting_without_escaping(self):
        out = blocks("```python\nx = a_b % 2\n```")
        self.assertIn(r"\begin{lstlisting}[language=python]", out)
        self.assertIn("x = a_b % 2", out)
        self.assertIn(r"\end{lstlisting}", out)

    def test_display_math_passes_through(self):
        self.assertEqual(blocks("$$x_1$$"), "$$x_1$$")

    def test_list_then_paragraph(self):
        out = blocks("- a\n\ntail")
        self.assertLess(out.index(r"\end{itemize}"), out.index("tail"))

    def test_empty_text_is_empty(self):
        self.assertEqual(blocks("  \n  "), "")
