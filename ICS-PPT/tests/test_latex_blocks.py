import tempfile
import unittest
from pathlib import Path

from lecturekit import model
from lecturekit.renderers.latex.assets import AssetCopier
from lecturekit.renderers.latex.blocks import LATEX_KINDS, Ctx, emit_block, emit_news


def image_content(**kw):
    content = {
        "src": "assets/fig.png", "alt": "", "caption": None,
        "width": None, "height": None, "framed": False,
        "caption_align": "center",
    }
    content.update(kw)
    return content


class BlocksTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        (self.tmp / "src" / "assets").mkdir(parents=True)
        (self.tmp / "src" / "assets" / "fig.png").write_bytes(b"png")
        self.ctx = Ctx(
            lecture_id="lec-a",
            page_id="p1",
            slide_width=1280,
            assets=AssetCopier(self.tmp / "out"),
            asset_root=self.tmp / "src",
        )

    def emit(self, kind, content, **kw):
        return emit_block(model.Block(kind=kind, content=content, **kw), self.ctx)

    def test_slide_is_not_a_latex_kind_but_prose_is(self):
        self.assertNotIn("slide", LATEX_KINDS)
        self.assertIn("prose", LATEX_KINDS)

    def test_notes_and_projection_only_kinds_are_absent(self):
        for kind in ("notes", "side_image", "cover"):
            self.assertNotIn(kind, LATEX_KINDS)

    def test_prose_is_body_text(self):
        self.assertEqual(self.emit("prose", "hi **there**"), r"hi \textbf{there}")

    def test_image_is_a_figure_with_caption_and_label(self):
        out = self.emit("image", image_content(caption="图 1", width="480px"))
        self.assertIn(r"\begin{figure}", out)
        self.assertIn(
            r"\includegraphics[width=0.375\textwidth]{assets/lec-a/fig.png}", out
        )
        self.assertIn(r"\caption{图 1}", out)
        self.assertIn(r"\label{fig:lec-a-p1-1}", out)

    def test_image_percent_width(self):
        out = self.emit("image", image_content(width="60%"))
        self.assertIn(r"width=0.6\textwidth", out)

    def test_image_without_width_gets_a_default(self):
        out = self.emit("image", image_content())
        self.assertIn(r"width=0.8\textwidth", out)

    def test_uncaptioned_figure_emits_no_empty_caption(self):
        out = self.emit("image", image_content())
        self.assertNotIn(r"\caption{}", out)
        self.assertNotIn(r"\caption", out)
        self.assertNotIn(r"\label", out)

    def test_uncaptioned_figure_with_a_footnote_still_carries_the_mark(self):
        out = self.emit("image", image_content(), footnotes=("s",))
        self.assertIn(r"\protect\footnotemark", out)
        self.assertIn(r"\footnotetext{s}", out)

    def test_unembeddable_format_becomes_a_visible_placeholder(self):
        (self.tmp / "src" / "assets" / "anim.gif").write_bytes(b"gif")
        out = self.emit("image", image_content(src="assets/anim.gif"))
        self.assertNotIn(r"\includegraphics", out)
        self.assertIn(r"\bookunrenderable", out)
        self.assertIn("anim.gif", out)

    def test_framed_image_is_boxed(self):
        out = self.emit("image", image_content(framed=True))
        self.assertIn(r"\fbox{", out)

    def test_figure_labels_increment_within_a_page(self):
        first = self.emit("image", image_content(caption="a"))
        second = self.emit("image", image_content(caption="b"))
        self.assertIn("fig:lec-a-p1-1", first)
        self.assertIn("fig:lec-a-p1-2", second)

    def test_table_uses_booktabs_and_align(self):
        out = self.emit("table", {
            "headers": ["a", "b"], "rows": [["1", "2"]], "align": ["left", "right"],
        })
        self.assertIn(r"\begin{tabular}{lr}", out)
        self.assertIn(r"\toprule", out)
        self.assertIn(r"\midrule", out)
        self.assertIn(r"\bottomrule", out)
        self.assertIn("1 & 2", out)

    def test_table_without_align_is_all_left(self):
        out = self.emit("table", {"headers": ["a", "b"], "rows": [], "align": None})
        self.assertIn(r"\begin{tabular}{ll}", out)

    def test_table_cells_get_inline_conversion(self):
        out = self.emit("table", {
            "headers": ["**h**"], "rows": [["a_b"]], "align": None,
        })
        self.assertIn(r"\textbf{h}", out)
        self.assertIn(r"a\_b", out)

    def test_code_is_lstlisting_and_is_not_escaped(self):
        out = self.emit("code", {"language": "c", "content": "int x = 1 % 2;"})
        self.assertIn(r"\begin{lstlisting}[language=c]", out)
        self.assertIn("int x = 1 % 2;", out)

    def test_link_is_href(self):
        out = self.emit("link", {"label": "L", "url": "http://x"})
        self.assertIn(r"\href{http://x}{L}", out)

    def test_sidenote_is_a_box_with_a_linked_title(self):
        out = self.emit("sidenote", {
            "title": "T", "text": "body", "link": "http://x", "logo": None,
        })
        self.assertIn(r"\begin{booksidenote}", out)
        self.assertIn(r"\href{http://x}{T}", out)
        self.assertIn("body", out)

    def test_sidenote_without_link_keeps_a_plain_title(self):
        out = self.emit("sidenote", {
            "title": "T", "text": "body", "link": None, "logo": None,
        })
        self.assertIn("T", out)
        self.assertNotIn(r"\href", out)

    def test_aside_is_a_quote(self):
        out = self.emit("aside", "remark")
        self.assertIn(r"\begin{quote}", out)
        self.assertIn("remark", out)

    def test_demo_is_a_box_with_the_command(self):
        out = self.emit("demo", {
            "name": "N", "command": "make run", "description": None,
        })
        self.assertIn("动手试试", out)
        self.assertIn("$ make run", out)

    def test_demo_prints_the_recorded_output_too(self):
        # The book cannot run anything, so what the author recorded is the whole
        # of what a reader gets.
        out = self.emit("demo", {
            "name": "N", "command": "make run", "output": "built 3 targets",
            "description": None,
        })
        self.assertIn("built 3 targets", out)

    def test_architecture_stacks_layers(self):
        out = self.emit("architecture", {
            "caption": "图", "flow": "down",
            "layers": [
                {"title": "App", "modules": ["Shell", "Editor"]},
                {"title": "Kernel", "modules": ["Sched"]},
            ],
        })
        self.assertIn("App", out)
        self.assertIn("Shell & Editor", out)
        self.assertIn(r"$\downarrow$", out)
        self.assertIn(r"\caption{图}", out)

    def test_row_puts_images_side_by_side(self):
        out = self.emit("row", {
            "caption": "两图",
            "items": [image_content(), image_content(src="assets/fig.png")],
        })
        self.assertIn(r"\begin{figure}", out)
        self.assertEqual(out.count(r"\includegraphics"), 2)
        self.assertIn(r"\caption{两图}", out)

    def test_footnotes_attach_to_a_text_block(self):
        out = self.emit("prose", "claim", footnotes=("src **a**",))
        self.assertIn(r"\footnote{src \textbf{a}}", out)

    def test_figure_footnotes_use_footnotemark_so_they_survive_the_float(self):
        out = self.emit("image", image_content(caption="c"), footnotes=("s",))
        self.assertIn(r"\protect\footnotemark", out)
        self.assertIn(r"\footnotetext{s}", out)

    def test_highlight_is_a_latex_kind(self):
        self.assertIn("highlight", LATEX_KINDS)

    def test_highlight_emits_the_toned_macro(self):
        out = self.emit("highlight", {"text": "Underloaded", "tone": "yellow"})
        self.assertEqual(out, r"\lkhighlight{lkChipYellow}{Underloaded}")

    def test_highlight_tone_picks_the_color(self):
        out = self.emit("highlight", {"text": "x", "tone": "orange"})
        self.assertIn("lkChipOrange", out)

    def test_highlight_joins_lines_with_a_tabular_row_break(self):
        out = self.emit(
            "highlight",
            {"text": "  Underloaded  \n 一半的队列是空的 ", "tone": "yellow"},
        )
        self.assertEqual(
            out, r"\lkhighlight{lkChipYellow}{Underloaded \\ 一半的队列是空的}"
        )

    def test_highlight_converts_inline_markdown_and_escapes(self):
        out = self.emit(
            "highlight", {"text": "发给了 **空闲的** 队列 100%", "tone": "yellow"}
        )
        self.assertIn(r"\textbf{空闲的}", out)
        self.assertIn(r"100\%", out)

    def test_highlight_footnote_marks_ride_the_chip_not_the_margin(self):
        out = self.emit(
            "highlight", {"text": "x", "tone": "yellow"}, footnotes=["src"]
        )
        # the mark goes in the optional arg (inside the center group), the text
        # after — a bare \footnote here would open its own paragraph
        self.assertIn(r"\lkhighlight[\footnotemark]{lkChipYellow}{x}", out)
        self.assertTrue(out.endswith("\n" + r"\footnotetext{src}"))
        self.assertNotIn(r"\footnote{", out)

    def test_unknown_kind_raises(self):
        with self.assertRaises(model.ValidationError):
            self.emit("notes", "x")


class NewsTest(unittest.TestCase):
    def test_news_renders_a_further_reading_list(self):
        item = model.NewsItem(
            title="T", url="http://x", source="S", date="2020", why="read it"
        )
        out = emit_news((item,))
        self.assertIn(r"\section*{延伸阅读}", out)
        self.assertIn(r"\href{http://x}{T}", out)
        self.assertIn("S", out)
        self.assertIn("read it", out)

    def test_no_news_is_empty(self):
        self.assertEqual(emit_news(()), "")
