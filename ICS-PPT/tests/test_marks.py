import unittest

from lecturekit import marks, model
from lecturekit.autobold import autobold
from lecturekit.dsl import Lecture
from lecturekit.renderers.latex.text import inline as latex_inline
from lecturekit.renderers.pptx.text import parse_inline


def slide(content: str) -> model.Block:
    return model.Block("slide", content)


class MarkSyntaxTest(unittest.TestCase):
    """What counts as a well-formed `<mark>`."""

    def test_bare_tag_is_legal_and_means_yellow(self):
        self.assertEqual(marks.errors("a <mark>x</mark> b", allowed=True), [])
        match = marks.SPAN_RE.search("a <mark>x</mark> b")
        self.assertEqual(marks.tone_of(match), "yellow")

    def test_every_tone_is_legal(self):
        for tone in marks.TONES:
            with self.subTest(tone=tone):
                text = f'a <mark class="{tone}">x</mark>'
                self.assertEqual(marks.errors(text, allowed=True), [])
                self.assertEqual(marks.tone_of(marks.SPAN_RE.search(text)), tone)

    def test_chip_and_mark_share_one_tone_set(self):
        # The chip's closed set *is* the inline set; a tone can never be legal
        # in one and not the other.
        self.assertIs(model.HIGHLIGHT_TONES, marks.TONES)
        self.assertEqual(model.DEFAULT_HIGHLIGHT_TONE, marks.DEFAULT_TONE)

    def test_misspelled_tone_is_an_error(self):
        self.assertTrue(marks.errors('<mark class="ornage">x</mark>', allowed=True))

    def test_single_quotes_are_an_error(self):
        self.assertTrue(marks.errors("<mark class='orange'>x</mark>", allowed=True))

    def test_other_attributes_are_an_error(self):
        self.assertTrue(marks.errors('<mark style="color:red">x</mark>', allowed=True))

    def test_unclosed_tag_is_an_error(self):
        self.assertTrue(marks.errors("a <mark>x", allowed=True))

    def test_stray_closing_tag_is_an_error(self):
        self.assertTrue(marks.errors("a </mark>", allowed=True))

    def test_nesting_is_an_error(self):
        self.assertTrue(marks.errors("<mark>a<mark>b</mark></mark>", allowed=True))

    def test_empty_mark_is_an_error(self):
        self.assertTrue(marks.errors("a <mark></mark>", allowed=True))

    def test_two_marks_on_one_line_are_fine(self):
        self.assertEqual(
            marks.errors('<mark>a</mark> and <mark class="blue">b</mark>', allowed=True),
            [],
        )

    def test_a_word_merely_starting_with_mark_is_not_a_tag(self):
        self.assertEqual(marks.errors("<marker>x</marker>", allowed=True), [])

    def test_a_code_span_may_show_the_tag_verbatim(self):
        # `x == y` in a code span is why the scan blanks verbatim text first;
        # the tag inside one is a sample, not markup.
        self.assertEqual(marks.errors("`<mark class='anything'>`", allowed=True), [])
        self.assertEqual(marks.errors("`<mark>`", allowed=False), [])

    def test_a_fence_may_show_the_tag_verbatim(self):
        self.assertEqual(
            marks.errors("```\n<mark class='x'>\n```", allowed=False), []
        )


class MarkShorthandTest(unittest.TestCase):
    """`==keyword==` is the tag, spelled short."""

    def test_it_expands_to_the_bare_tag(self):
        self.assertEqual(
            marks.expand("replicate a ==sequence== of values"),
            "replicate a <mark>sequence</mark> of values",
        )

    def test_every_tone_has_a_prefix(self):
        for tone in marks.TONES:
            with self.subTest(tone=tone):
                self.assertEqual(
                    marks.expand(f"is =={tone}:not== the primary"),
                    f'is <mark class="{tone}">not</mark> the primary',
                )

    def test_two_on_one_line_stay_separate(self):
        self.assertEqual(
            marks.expand("==a== and ==b=="),
            "<mark>a</mark> and <mark>b</mark>",
        )

    def test_a_multi_word_body_is_kept_whole(self):
        self.assertEqual(
            marks.expand("consensus on ==a single value=="),
            "consensus on <mark>a single value</mark>",
        )

    def test_a_word_that_is_not_a_tone_stays_in_the_body(self):
        self.assertEqual(marks.expand("==foo:bar=="), "<mark>foo:bar</mark>")

    def test_a_comparison_is_not_a_mark(self):
        # The padding around an operator is what tells the two apart.
        for text in ("a == b == c", "x == y", "N == Nh and V == null"):
            with self.subTest(text=text):
                self.assertEqual(marks.expand(text), text)

    def test_an_unpaired_marker_is_left_as_text(self):
        self.assertEqual(marks.expand("a == b"), "a == b")

    def test_a_body_may_hold_a_single_equals(self):
        self.assertEqual(marks.expand("==N = 5=="), "<mark>N = 5</mark>")

    def test_it_does_not_span_a_line(self):
        text = "==a\nb=="
        self.assertEqual(marks.expand(text), text)

    def test_a_code_span_shows_the_characters_verbatim(self):
        self.assertEqual(marks.expand("`a ==b==`"), "`a ==b==`")

    def test_a_fence_shows_the_characters_verbatim(self):
        self.assertEqual(marks.expand("```\n==x==\n```"), "```\n==x==\n```")

    def test_text_around_a_verbatim_span_still_expands(self):
        self.assertEqual(
            marks.expand("==a== `==b==` ==c=="),
            "<mark>a</mark> `==b==` <mark>c</mark>",
        )

    def test_the_expansion_is_what_the_validator_accepts(self):
        self.assertEqual(
            marks.errors(marks.expand('==x== and ==blue:y=='), allowed=True), []
        )

    def test_the_tag_itself_still_works(self):
        text = 'a <mark>x</mark> and <mark class="blue">y</mark>'
        self.assertEqual(marks.expand(text), text)

    def test_a_slide_block_expands_it(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("t")
            p.slide("- replicate a ==sequence== of ==orange:values==")

        lecture.page(id="p", body=body)
        block = lecture.build().children[0].blocks[0]
        self.assertEqual(
            block.content,
            '- replicate a <mark>sequence</mark> of '
            '<mark class="orange">values</mark>',
        )

    def test_a_marked_headline_still_autobolds(self):
        # expand runs first, so autobold sees the tag it special-cases.
        self.assertEqual(
            autobold(marks.expand("==Paxos==: a bottom-up approach")),
            "**<mark>Paxos</mark>: a bottom-up approach**",
        )


class MarkPlacementTest(unittest.TestCase):
    """`<mark>` is legal in `p.slide(...)` text and nowhere else."""

    def check(self, block: model.Block):
        model.check_marks(block, "p1")

    def test_slide_text_accepts_it(self):
        self.check(slide("a <mark>x</mark> b"))

    def test_prose_rejects_it(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block("prose", "a <mark>x</mark>"))

    def test_aside_rejects_it(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block("aside", "a <mark>x</mark>"))

    def test_sidenote_body_rejects_it(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block(
                "sidenote", {"title": "t", "text": "<mark>x</mark>",
                             "link": None, "logo": None},
            ))

    def test_table_cell_rejects_it(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block(
                "table", {"headers": ["a"], "rows": [["<mark>x</mark>"]],
                          "align": None},
            ))

    def test_caption_rejects_it(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block(
                "image", {"src": "a.svg", "alt": "", "caption": "<mark>x</mark>"},
            ))

    def test_highlight_chip_rejects_it(self):
        # Yellow on yellow: the chip is already the wash.
        with self.assertRaises(model.ValidationError):
            self.check(model.Block(
                "highlight", {"text": "<mark>x</mark>", "tone": "yellow"},
            ))

    def test_footnote_rejects_it_even_on_a_slide(self):
        with self.assertRaises(model.ValidationError):
            self.check(model.Block("slide", "ok", footnotes=("<mark>x</mark>",)))

    def test_annotation_rejects_it_even_on_a_slide(self):
        note = model.Annotation(text="<mark>x</mark>")
        with self.assertRaises(model.ValidationError):
            self.check(model.Block("slide", "ok", annotations=(note,)))

    def test_code_block_is_exempt_from_the_scan(self):
        # A code sample may legitimately *show* <mark> as the HTML it is.
        self.check(model.Block(
            "code", {"language": "html", "content": "<mark class='x'>hi</mark>"},
        ))

    def test_a_url_is_not_scanned_as_prose(self):
        self.check(model.Block("link", {"label": "docs", "url": "http://x/<mark>"}))

    def test_page_title_rejects_it(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("a <mark>x</mark>")
            p.slide("ok")

        lecture.page(id="p", body=body)
        with self.assertRaises(model.ValidationError):
            lecture.build()

    def test_book_title_rejects_it(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("t")
            p.slide("ok")

        lecture.page(id="p", body=body, book_title="a <mark>x</mark>")
        with self.assertRaises(model.ValidationError):
            lecture.build()

    def test_a_valid_marked_slide_builds(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("t")
            p.slide('a <mark class="orange">x</mark> b')

        lecture.page(id="p", body=body)
        lecture.build()

    def test_the_error_names_the_page_and_the_place(self):
        with self.assertRaises(model.ValidationError) as caught:
            self.check(model.Block("prose", "<mark>x</mark>"))
        self.assertIn("p1", str(caught.exception))
        self.assertIn("prose", str(caught.exception))


class MarkAutoboldTest(unittest.TestCase):
    """A headline whose first word is the point still bolds."""

    def test_a_line_starting_with_a_mark_is_still_bolded(self):
        self.assertEqual(
            autobold("<mark>Paxos</mark>: a bottom-up approach"),
            "**<mark>Paxos</mark>: a bottom-up approach**",
        )

    def test_a_toned_mark_at_the_start_is_still_bolded(self):
        self.assertEqual(
            autobold('<mark class="blue">Raft</mark>: the other way'),
            '**<mark class="blue">Raft</mark>: the other way**',
        )

    def test_other_raw_html_at_the_start_is_still_left_alone(self):
        for line in ("<div>x</div>", "<br>", "<u>x</u>"):
            with self.subTest(line=line):
                self.assertEqual(autobold(line), line)

    def test_a_mark_mid_line_never_mattered_and_still_bolds(self):
        self.assertEqual(
            autobold("replicate a <mark>sequence</mark> of values"),
            "**replicate a <mark>sequence</mark> of values**",
        )

    def test_a_bullet_holding_a_mark_stays_unbolded(self):
        line = "- Multi-Paxos: a <mark>sequence</mark>"
        self.assertEqual(autobold(line), line)


class MarkLatexTest(unittest.TestCase):
    def test_a_mark_becomes_the_wash_macro(self):
        self.assertEqual(
            latex_inline("a <mark>x</mark> b"), r"a \lkmark{lkMarkYellow}{x} b"
        )

    def test_each_tone_maps_to_its_color(self):
        for tone, color in (("orange", "lkMarkOrange"), ("green", "lkMarkGreen"),
                            ("blue", "lkMarkBlue")):
            with self.subTest(tone=tone):
                self.assertEqual(
                    latex_inline(f'<mark class="{tone}">x</mark>'),
                    r"\lkmark{%s}{x}" % color,
                )

    def test_autobold_around_a_mark_survives(self):
        # This is the shape every marked headline arrives in.
        self.assertEqual(
            latex_inline(autobold("<mark>Paxos</mark>: bottom-up")),
            r"\textbf{\lkmark{lkMarkYellow}{Paxos}: bottom-up}",
        )

    def test_markup_inside_a_mark_still_converts(self):
        self.assertEqual(
            latex_inline("<mark>a *b* `c`</mark>"),
            r"\lkmark{lkMarkYellow}{a \textit{b} \texttt{c}}",
        )

    def test_a_marked_span_escapes_its_body(self):
        self.assertEqual(
            latex_inline("<mark>100%</mark>"), r"\lkmark{lkMarkYellow}{100\%}"
        )

    def test_a_slide_forced_into_the_book_can_be_emitted(self):
        # `only=["latex"]` is the only path a mark reaches the book by.
        from lecturekit.renderers.latex import blocks as latex_blocks
        self.assertIn("slide", latex_blocks._EMITTERS)


class MarkPptxTest(unittest.TestCase):
    def test_a_mark_becomes_a_run_flag(self):
        runs = parse_inline("a <mark>x</mark> b")
        self.assertEqual([(r.text, r.mark) for r in runs],
                         [("a ", None), ("x", "yellow"), (" b", None)])

    def test_the_tone_rides_the_run(self):
        runs = parse_inline('<mark class="green">x</mark>')
        self.assertEqual(runs[0].mark, "green")

    def test_bold_and_mark_compose(self):
        runs = parse_inline(autobold("<mark>Paxos</mark>: bottom-up"))
        self.assertEqual([(r.text, r.bold, r.mark) for r in runs],
                         [("Paxos", True, "yellow"), (": bottom-up", True, None)])

    def test_markup_inside_a_mark_still_parses(self):
        runs = parse_inline("<mark>a `c`</mark>")
        self.assertEqual([(r.text, r.code, r.mark) for r in runs],
                         [("a ", False, "yellow"), ("c", True, "yellow")])

    def test_every_tone_resolves_to_a_theme_wash(self):
        from lecturekit.renderers.pptx import theme
        for tone in marks.TONES:
            with self.subTest(tone=tone):
                self.assertIn(tone, theme.MARK_WHEEL)


if __name__ == "__main__":
    unittest.main()
