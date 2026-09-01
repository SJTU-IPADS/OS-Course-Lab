import unittest

from lecturekit import Lecture, pseudo
from lecturekit.model import Block, ValidationError
from lecturekit.renderers.latex.blocks import Ctx, emit_block
from lecturekit.renderers.transcript.blocks import render_block as render_sheet
from lecturekit.renderers.viewer.blocks import render_block

SRC = """Leader chooses Mn = a number greater than Nh
Leader sends <proposal, Mn> to all nodes

Acceptor receives <proposal, N>
    if  N < Nh          # a stale proposal
        reply <promise-reject>
    else
        Nh = N
        reply <promise-ok, Na, Va>"""


def kinds(text):
    """Every non-plain token in `text`, flattened, as (kind, text)."""
    return [tok for line in pseudo.tokenize(text) for tok in line if tok[0] != "plain"]


class LexerTest(unittest.TestCase):
    def test_state_names_are_the_assignment_targets(self):
        self.assertEqual(pseudo.state_names(SRC), {"Mn", "Nh"})

    def test_several_assignments_on_one_line_all_count(self):
        self.assertEqual(
            pseudo.state_names("Na = N;  Va = V;  Nh = N;"), {"Na", "Va", "Nh"}
        )

    def test_a_subscripted_or_dotted_target_names_what_it_writes(self):
        # Writing one entry of a table is writing the table.
        self.assertEqual(
            pseudo.state_names("bank[a] = bank[a] - amt\nd.version = v"),
            {"bank", "d"},
        )

    def test_a_hyphenated_name_is_one_name(self):
        self.assertEqual(pseudo.state_names("read-set[d] = read(d)"), {"read-set"})

    def test_a_hyphenated_state_name_is_one_token(self):
        src = "read-set[d] = read(d)\nif read-set[d] != read(d)"
        self.assertEqual(len([t for t in kinds(src) if t == ("state", "read-set")]), 2)

    def test_a_hyphen_stays_arithmetic_when_nothing_assigns_to_it(self):
        # Nothing here assigns to `P-1`, so the hyphen is the operator it looks
        # like and the block's only state name is `Nh`.
        src = "Nh = P-1"
        self.assertEqual(pseudo.state_names(src), {"Nh"})
        self.assertEqual(pseudo.tokenize(src)[0][1], ("plain", " = P-1"))

    def test_comparisons_are_not_assignments(self):
        # The operator sits where the `=` would have to be, so none of these
        # names is state.
        self.assertEqual(pseudo.state_names("if V != null and N <= Nh"), set())

    def test_a_state_name_is_colored_everywhere_including_reads(self):
        # `Nh` is assigned once, on the last-but-one line, and read twice.
        self.assertEqual(len([t for t in kinds(SRC) if t == ("state", "Nh")]), 3)

    def test_messages_are_angle_bracketed_spans(self):
        self.assertIn(("message", "<promise-ok, Na, Va>"), kinds(SRC))

    def test_keywords_match_case_insensitively(self):
        self.assertIn(("keyword", "If"), kinds("If  N < Nh"))

    def test_comment_runs_to_end_of_line(self):
        self.assertIn(("comment", "# a stale proposal"), kinds(SRC))

    def test_a_double_slash_also_opens_a_comment(self):
        # C-style pseudocode is as common as shell-style, and a comment left
        # black is what makes a listing read as a flat wall.
        self.assertIn(
            ("comment", "// abort releases the locks"),
            kinds("    abort() // abort releases the locks"),
        )

    def test_prose_words_stay_plain(self):
        self.assertNotIn("keyword", [k for k, _ in kinds("Leader gets a majority")])

    def test_blank_line_survives_as_an_empty_token_list(self):
        self.assertEqual(pseudo.tokenize("a\n\nb")[1], [])


class ViewerTest(unittest.TestCase):
    def render(self, content):
        block = Block(kind="code", content={"language": "pseudo", "content": content})
        return render_block(block)[0]

    def test_pseudo_is_raw_html_not_a_fence(self):
        html = self.render("Nh = N")
        self.assertTrue(html.startswith('<pre class="lk-pseudo"><code>'))
        self.assertTrue(html.endswith("</code></pre>"))

    def test_tokens_carry_their_class(self):
        html = self.render("    if  N < Nh\n        Nh = N")
        self.assertIn('<span class="tok-kw">if</span>', html)
        self.assertIn('<span class="tok-st">Nh</span>', html)

    def test_angle_brackets_are_escaped_inside_the_span(self):
        html = self.render("reply <promise-ok>")
        self.assertIn('<span class="tok-msg">&lt;promise-ok&gt;</span>', html)

    def test_indentation_and_blank_lines_are_preserved(self):
        html = self.render("a\n\n    b")
        self.assertIn("a\n\n    b", html)

    def test_other_languages_still_fence(self):
        block = Block(kind="code", content={"language": "python", "content": "x = 1"})
        self.assertEqual(render_block(block), ["```python", "x = 1", "```", ""])


class BookTest(unittest.TestCase):
    def setUp(self):
        self.ctx = Ctx(
            lecture_id="lec-a", page_id="p1", slide_width=1280, assets=None
        )

    def render(self, content):
        block = Block(kind="code", content={"language": "pseudo", "content": content})
        return emit_block(block, self.ctx)

    def test_pseudo_uses_its_own_environment_not_lstlisting(self):
        out = self.render("Nh = N")
        self.assertIn(r"\begin{lkpseudo}", out)
        self.assertNotIn("lstlisting", out)

    def test_tokens_are_wrapped_in_color_macros(self):
        out = self.render("    if  N < Nh\n        Nh = N\n        reply <ok>")
        self.assertIn(r"\lkpskw{if}", out)
        self.assertIn(r"\lkpsst{Nh}", out)
        self.assertIn(r"\lkpsmsg{<ok>}", out)

    def test_indentation_becomes_hard_spaces(self):
        self.assertIn("~~~~", self.render("    Nh = N"))

    def test_a_blank_line_is_not_an_empty_row(self):
        # `\\` after an empty line is a LaTeX error, so blank lines carry a `~`.
        self.assertIn("~\\\\", self.render("a\n\nb"))

    def test_other_languages_still_use_lstlisting(self):
        block = Block(kind="code", content={"language": "python", "content": "x = 1"})
        self.assertIn("lstlisting", emit_block(block, self.ctx))


MARKED = """transfer(bank, a, b, amt):
    fcopy(bank, bank_temp)
    records = mmap(bank_temp, ...)
    fsync(bank_temp, ...)
    rename(bank_temp, bank)"""


def code_block(**content):
    return Block(kind="code", content={"language": "pseudo", **content})


def marked_page(**kwargs):
    """A one-page lecture whose only block is a marked code block, built."""
    lecture = Lecture(id="lec01", title="Test Lecture")
    lecture.page(
        "p1",
        body=lambda p: (p.title("Shadow copy"), p.code("pseudo", MARKED, **kwargs)),
    )
    return lecture.build().children[0].blocks[0]


class MarkAuthoringTest(unittest.TestCase):
    def test_an_unmarked_block_carries_no_mark_keys(self):
        # The stored content is what every renderer branches on, so a block the
        # author never marked must look exactly as it did before `mark` existed.
        page = Lecture(id="lec01", title="T")
        page.page("p1", body=lambda p: (p.title("T"), p.code("pseudo", "Nh = N")))
        block = page.build().children[0].blocks[0]
        self.assertEqual(block.content, {"language": "pseudo", "content": "Nh = N"})

    def test_mark_stores_the_lines_and_the_default_tone(self):
        block = marked_page(mark=[2, 5])
        self.assertEqual(block.content["mark"], [2, 5])
        self.assertEqual(block.content["tone"], "yellow")

    def test_line_numbers_count_the_block_as_shown(self):
        # A triple-quoted listing usually opens with a newline; the renderers
        # strip it, so line 1 is the first line of code, not the blank.
        lecture = Lecture(id="lec01", title="T")
        lecture.page("p1", body=lambda p: (p.title("T"), p.code("pseudo", "\na\nb\n", mark=[2])))
        lecture.build()  # line 2 is `b`, so this validates

    def test_a_line_past_the_end_is_refused(self):
        with self.assertRaises(ValidationError) as ctx:
            marked_page(mark=[9])
        self.assertIn("5 lines", str(ctx.exception))

    def test_a_blank_line_cannot_be_marked(self):
        lecture = Lecture(id="lec01", title="T")
        lecture.page("p1", body=lambda p: (p.title("T"), p.code("pseudo", "a\n\nb", mark=[2])))
        with self.assertRaises(ValidationError) as ctx:
            lecture.build()
        self.assertIn("blank", str(ctx.exception))

    def test_zero_and_booleans_are_not_line_numbers(self):
        for bad in ([0], [-1], [True]):
            with self.assertRaises(ValidationError):
                marked_page(mark=bad)

    def test_an_unknown_tone_is_refused(self):
        with self.assertRaises(ValidationError):
            marked_page(mark=[2], tone="purple")

    def test_only_pseudo_can_be_marked(self):
        lecture = Lecture(id="lec01", title="T")
        lecture.page(
            "p1", body=lambda p: (p.title("T"), p.code("python", "x = 1", mark=[1]))
        )
        with self.assertRaises(ValidationError) as ctx:
            lecture.build()
        self.assertIn("pseudo", str(ctx.exception))


class MarkRenderTest(unittest.TestCase):
    def viewer(self, **content):
        return render_block(code_block(content=MARKED, **content))[0]

    def test_the_marked_line_is_one_span_around_its_tokens(self):
        html = self.viewer(mark=[2], tone="yellow")
        self.assertIn(
            '<span class="lk-codemark lk-codemark--yellow">'
            '    fcopy(bank, bank_temp)</span>',
            html,
        )

    def test_the_wash_stops_at_the_line_end(self):
        # The newlines stay outside the spans, so a mark cannot bleed into the
        # line below it.
        html = self.viewer(mark=[2], tone="yellow")
        self.assertNotIn("\n</span>", html)

    def test_an_unmarked_line_is_untouched(self):
        html = self.viewer(mark=[2], tone="yellow")
        self.assertIn("\n    rename(bank_temp, bank)</code>", html)

    def test_the_tone_rides_the_class(self):
        self.assertIn("lk-codemark--orange", self.viewer(mark=[5], tone="orange"))

    def test_the_book_washes_the_line(self):
        ctx = Ctx(lecture_id="lec-a", page_id="p1", slide_width=1280, assets=None)
        out = emit_block(code_block(content=MARKED, mark=[5], tone="green"), ctx)
        self.assertIn(r"\lkpshl{lkMarkGreen}{", out)
        self.assertEqual(out.count(r"\lkpshl"), 1)

    def test_the_transcript_washes_the_line(self):
        out = render_sheet(code_block(content=MARKED, mark=[2], tone="blue"), None)
        self.assertIn('class="tx-codemark tx-codemark--blue"', out)


if __name__ == "__main__":
    unittest.main()
