"""``--to transcript``: a lecture compressed onto printable A4 sheets."""

import base64
import textwrap
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from lecturekit.cli import load_lecture
from lecturekit.dsl import Lecture
from lecturekit.renderers import get_renderer
from lecturekit.renderers.transcript import TranscriptRenderer, build_html
from lecturekit.renderers.transcript.images import Embedder
from lecturekit.renderers.transcript.renderer import kept_page_ids
from lecturekit.renderers.transcript.text import markdown

SVG = b'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 400"></svg>'


def _built(lecture):
    return lecture.build() if hasattr(lecture, "build") else lecture


def _render(lecture, asset_root=None):
    built = _built(lecture)
    return build_html(built, Embedder(asset_root, built.borrowed))


def _lecture_with(body, **page_kwargs):
    lec = Lecture(id="lec-t", title="Lecture T")
    lec.page(id="p1", body=body, **page_kwargs)
    return lec.build()


class BlockSelection(unittest.TestCase):
    def test_deck_body_is_in_and_the_other_bodies_are_out(self):
        def body(p):
            p.title("Consensus")
            p.slide("a majority decides")
            p.prose("书稿正文")
            p.notes("口头讲的话")
            p.demo("run", "python3 demo.py")

        html = _render(_lecture_with(body))
        self.assertIn("a majority decides", html)
        self.assertNotIn("书稿正文", html)
        self.assertNotIn("口头讲的话", html)
        self.assertNotIn("python3 demo.py", html)

    def test_footnotes_never_reach_the_sheet(self):
        def body(p):
            p.title("Quorums")
            p.slide("any two majorities meet").footnote("来源：Lamport 2001")

        html = _render(_lecture_with(body))
        self.assertIn("any two majorities meet", html)
        self.assertNotIn("Lamport 2001", html)

    def test_annotations_become_lines_under_the_block(self):
        def body(p):
            p.title("Trace")
            p.slide("S2 promises").annotate("promise 带回了 foo", at="center")

        html = _render(_lecture_with(body))
        self.assertIn("tx-anno", html)
        self.assertIn("promise 带回了 foo", html)

    def test_a_page_may_hide_its_annotations(self):
        def body(p):
            p.title("Trace")
            p.slide("S2 promises").annotate("看这条曲线", at="center")

        html = _render(_lecture_with(body, annotation=False))
        self.assertNotIn("看这条曲线", html)

    def test_except_transcript_takes_a_block_out(self):
        def body(p):
            p.title("Roles")
            p.slide("kept")
            p.slide("dropped", except_=["transcript"])

        html = _render(_lecture_with(body))
        self.assertIn("kept", html)
        self.assertNotIn("dropped", html)

    def test_only_transcript_forces_a_block_in(self):
        def body(p):
            p.title("Roles")
            p.slide("deck only")
            p.prose("sheet only", only=["transcript"])

        html = _render(_lecture_with(body))
        self.assertIn("sheet only", html)

    def test_a_page_whose_blocks_all_dropped_is_not_printed(self):
        lec = Lecture(id="lec-t", title="Lecture T")

        def kept(p):
            p.title("Kept page")
            p.slide("body")

        def gone(p):
            p.title("Transition page")
            p.slide("filler", except_=["transcript"])

        lec.page(id="a", body=kept)
        lec.page(id="b", body=gone)
        html = _render(lec)
        self.assertIn("Kept page", html)
        self.assertNotIn("Transition page", html)


class PageSelection(unittest.TestCase):
    def test_an_animation_keeps_only_its_finished_frame(self):
        def body(p):
            p.title("Commit logging")
            p.slide("log first")
            p.frames("a.svg", "b.svg", "c.svg")

        lec = _lecture_with(body)
        self.assertEqual(kept_page_ids(_built(lec)), {"p1-3"})
        html = _render(lec)
        # One entry, one figure, one copy of the text the frames shared.
        self.assertEqual(html.count("log first"), 1)
        self.assertEqual(html.count("Commit logging"), 1)

    def test_a_reveal_pair_prints_once_with_the_bubble(self):
        def figure(p):
            p.title("The jump")
            p.slide("look at the curve").annotate("the jump is here", at="center")

        lec = Lecture(id="lec-t", title="Lecture T")
        lec.page(id="jump-1", body=figure, annotation=False)
        lec.page(id="jump-2", body=figure, annotation=True)

        self.assertEqual(kept_page_ids(_built(lec)), {"jump-2"})
        html = _render(lec)
        self.assertEqual(html.count("The jump"), 1)
        self.assertIn("the jump is here", html)

    def test_the_cover_is_not_a_sheet_entry(self):
        lec = Lecture(id="lec-t", title="Lecture T")
        lec.cover("Paxos and Raft", author="Xingda Wei")

        def body(p):
            p.title("First real page")
            p.slide("body")

        lec.page(id="p1", body=body)
        self.assertEqual(kept_page_ids(_built(lec)), {"p1"})

    def test_citations_do_not_grow_a_reference_page(self):
        def body(p):
            p.title("Transformer")
            p.slide("self-attention")
            p.cite(title="Attention Is All You Need", year="2017")

        html = _render(_lecture_with(body))
        self.assertNotIn("参考文献", html)
        self.assertNotIn("Attention Is All You Need", html)


class Numbering(unittest.TestCase):
    def test_sections_and_pages_share_one_outline_numbering(self):
        lec = Lecture(id="lec-t", title="Lecture T")

        def page(text):
            def body(p):
                p.title(text)
                p.slide(text)
            return body

        with lec.section("Single-decree Paxos") as s:
            s.page(id="a", body=page("A"))
            s.page(id="b", body=page("B"))
        with lec.section("Multi-Paxos") as s:
            s.page(id="c", body=page("C"))
        lec.close("conclusion", body=page("End"))

        html = _render(lec)
        for number, title in (
            ("1", "Single-decree Paxos"), ("1.1", "A"), ("1.2", "B"),
            ("2", "Multi-Paxos"), ("2.1", "C"), ("3", "End"),
        ):
            self.assertIn(f'<span class="tx-num">{number}</span>{title}', html)

    def test_a_section_left_empty_prints_no_heading(self):
        lec = Lecture(id="lec-t", title="Lecture T")

        def gone(p):
            p.title("Dropped")
            p.slide("filler", except_=["transcript"])

        with lec.section("Empty section") as s:
            s.page(id="a", body=gone)
        self.assertNotIn("Empty section", _render(lec))


class Figures(unittest.TestCase):
    def setUp(self):
        self.tmp = TemporaryDirectory()
        root = Path(self.tmp.name)
        (root / "assets").mkdir()
        (root / "assets" / "fig.svg").write_bytes(SVG)
        self.root = root

    def tearDown(self):
        self.tmp.cleanup()

    def test_a_figure_is_embedded_not_linked(self):
        def body(p):
            p.title("Topology")
            p.image("assets/fig.svg", caption="拓扑")

        html = _render(_lecture_with(body), asset_root=self.root)
        self.assertIn("data:image/svg+xml;base64,", html)
        self.assertIn(base64.b64encode(SVG).decode(), html)
        self.assertNotIn('src="assets/fig.svg"', html)
        self.assertIn("拓扑", html)

    def test_an_img_written_into_a_table_cell_is_embedded_too(self):
        def body(p):
            p.title("Glossary")
            p.table(
                headers=["Parliament", "Machine"],
                rows=[['<img src="assets/fig.svg" width="30">legislator', "acceptor"]],
            )

        html = _render(_lecture_with(body), asset_root=self.root)
        self.assertIn('<img src="data:image/svg+xml;base64,', html)
        self.assertNotIn("assets/fig.svg", html)

    def test_a_missing_figure_is_reported_not_fatal(self):
        def body(p):
            p.title("Topology")
            p.image("assets/nope.svg")

        embedder = Embedder(self.root)
        html = build_html(_lecture_with(body), embedder)
        self.assertIn("缺图", html)
        self.assertTrue(embedder.missing)

    def test_a_wide_figure_fills_the_column_and_a_small_one_does_not(self):
        narrow = b'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 180 90"></svg>'
        (self.root / "assets" / "icon.svg").write_bytes(narrow)

        def body(p):
            p.title("Sizes")
            p.image("assets/fig.svg")
            p.image("assets/icon.svg")

        html = _render(_lecture_with(body), asset_root=self.root)
        self.assertIn("width:100%", html)
        self.assertIn("width:22%", html)


class SelfContained(unittest.TestCase):
    def test_the_sheet_references_nothing_outside_itself(self):
        def body(p):
            p.title("Topology")
            p.slide("text")

        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "assets").mkdir()
            (root / "assets" / "fig.svg").write_bytes(SVG)

            def figure(p):
                p.title("Topology")
                p.image("assets/fig.svg")

            html = _render(_lecture_with(figure), asset_root=root)
        # No stylesheet link, no script, no remote font, no relative src.
        self.assertNotIn("<link", html)
        self.assertNotIn("<script", html)
        self.assertNotIn("http://", html)
        self.assertNotIn("@import", html)
        self.assertIn("<style>", html)

    def test_slide_text_cannot_smuggle_markup_onto_the_sheet(self):
        def body(p):
            p.title("Escaping")
            p.slide("<script>alert(1)</script> and 5 < 6")

        html = _render(_lecture_with(body))
        self.assertNotIn("<script", html)
        self.assertIn("&lt;script&gt;", html)


class Review(unittest.TestCase):
    SOURCE = textwrap.dedent(
        """
        from lecturekit.dsl import Lecture

        lecture = Lecture(id="src", title="Source Lecture")


        def quorum(p):
            p.title("Quorums intersect")
            p.slide("any two majorities share a member")


        lecture.page("quorum", body=quorum)
        """
    )

    HOST = textwrap.dedent(
        """
        from lecturekit.dsl import Lecture, review_section

        lecture = Lecture(id="host", title="Host Lecture")
        review_section(lecture, "回顾", {"../src": ["quorum"]})


        def today(p):
            p.title("Today")
            p.slide("new material")


        lecture.page("today", body=today)
        """
    )

    def test_a_borrowed_review_page_is_not_reprinted(self):
        with TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "lecture.py").write_text(self.SOURCE, encoding="utf-8")
            (root / "host").mkdir()
            (root / "host" / "lecture.py").write_text(self.HOST, encoding="utf-8")
            lecture = load_lecture(root / "host")

        self.assertEqual(kept_page_ids(lecture), {"today"})
        html = _render(lecture)
        self.assertNotIn("Quorums intersect", html)
        self.assertIn("new material", html)


class MarkdownSubset(unittest.TestCase):
    def test_bullets_one_blank_line_apart_stay_one_list(self):
        html = markdown("- first\n\n- second\n\n- third")
        self.assertEqual(html.count("<ul>"), 1)
        self.assertEqual(html.count("<li>"), 3)

    def test_a_marked_keyword_keeps_its_wash(self):
        # `==…==` is expanded into the tag by `p.slide`, so this is the
        # spelling a block actually stores.
        html = markdown(
            "<mark>Multi-Paxos</mark> replicates a "
            '<mark class="blue">sequence</mark>'
        )
        self.assertIn("<mark>Multi-Paxos</mark>", html)
        self.assertIn('<mark class="blue">sequence</mark>', html)

    def test_math_is_typeset_without_a_script(self):
        html = markdown(r"cost is $T \approx \frac{2PN}{W}$")
        self.assertIn("tx-mathspan", html)
        self.assertIn("≈", html)
        self.assertIn("tx-frac", html)

    def test_a_link_keeps_its_url(self):
        html = markdown("see [the paper](https://example.com/p.pdf)")
        self.assertIn('<a href="https://example.com/p.pdf">the paper</a>', html)


class RendererPlumbing(unittest.TestCase):
    def test_the_target_is_registered_under_its_name(self):
        self.assertIs(get_renderer("transcript"), TranscriptRenderer)

    def test_render_writes_one_file_named_after_the_lecture(self):
        def body(p):
            p.title("Page")
            p.slide("body")

        lec = Lecture(id="lec-t", title="Paxos and Raft")
        lec.page(id="p1", body=body)
        with TemporaryDirectory() as tmp:
            out = Path(tmp, "sheet")
            entry = TranscriptRenderer().render(_built(lec), out)
            self.assertEqual(entry.name, "paxos-and-raft-transcript.html")
            self.assertEqual([p.name for p in out.iterdir()], [entry.name])


if __name__ == "__main__":
    unittest.main()
