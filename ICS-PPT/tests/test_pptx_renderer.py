import base64
import tempfile
import unittest
from pathlib import Path

from pptx import Presentation
from pptx.enum.dml import MSO_FILL
from pptx.enum.shapes import MSO_SHAPE, MSO_SHAPE_TYPE
from pptx.util import Emu, Inches, Pt

from lecturekit import Lecture, model
from lecturekit.renderers import RENDERERS, get_renderer
from lecturekit.renderers.pptx import PptxRenderer, theme
from lecturekit.renderers.pptx.layout import Layout, px

# 1x1 transparent PNG (python-pptx can embed it; svg is unsupported upstream).
_PNG = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+M9Q"
    "DwADhgGAWjR9awAAAABJRU5ErkJggg=="
)


def shape_texts(slide):
    return [s.text_frame.text for s in slide.shapes if s.has_text_frame]


def rounded_boxes(slide):
    return [s for s in slide.shapes
            if s.shape_type == MSO_SHAPE_TYPE.AUTO_SHAPE
            and s.auto_shape_type == MSO_SHAPE.ROUNDED_RECTANGLE]


def all_text(slide):
    return "\n".join(shape_texts(slide))


class PptxRendererTest(unittest.TestCase):
    def render(self, lecture, *, asset_root=None):
        tmp = Path(tempfile.mkdtemp())
        out = PptxRenderer(asset_root=asset_root).render(lecture.build(), tmp)
        return Presentation(str(out)), out

    def sample(self):
        lecture = Lecture(id="lec", title="Deck Title")

        def body(p):
            p.title("Welcome")
            p.slide("- hello\n- **world**")
            p.code("python", "print('hi')")
            p.link("OSTEP", "https://example.test")
            p.table([["a", "b"]], headers=["H1", "H2"])
            p.aside("an aside")
            p.sidenote("Ref", "see this")
            p.notes("teacher only")

        with lecture.section("Intro", id="intro") as s:
            s.page("welcome", body=body)
        return lecture

    def test_registered_as_render_target(self):
        self.assertIn("pptx", RENDERERS)
        self.assertIs(get_renderer("pptx"), PptxRenderer)

    def test_one_slide_per_page(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.slide("x")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
            s.page("p2", body=body)
        prs, _ = self.render(lecture)
        self.assertEqual(len(prs.slides), 2)

    def test_output_named_after_lecture_title(self):
        _, out = self.render(self.sample())
        self.assertEqual(out.name, "deck-title.pptx")

    def test_slide_size_follows_ratio(self):
        lecture = Lecture(id="lec", title="L", ratio="4:3")

        def body(p):
            p.title("P")
            p.slide("x")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture)
        self.assertEqual(prs.slide_width, Emu(Inches(960 / 96)))

    def test_title_and_bullet_text_present(self):
        prs, _ = self.render(self.sample())
        text = all_text(prs.slides[0])
        self.assertIn("Welcome", text)
        self.assertIn("hello", text)
        self.assertIn("world", text)

    def test_nested_bullets_do_not_restart_the_outer_numbering(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("Steps")
            p.slide("1. one\n   - a\n2. two\n   - b\n3. three\n\n- x\n1. again")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture)
        lines = all_text(prs.slides[0]).split("\n")
        numbered = [l.split()[0] for l in lines if l[:1].isdigit()]
        self.assertEqual(numbered, ["1.", "2.", "3.", "1."])

    def test_a_paragraph_between_two_ordered_lists_restarts_the_numbering(self):
        lecture = Lecture(id="lec", title="T")

        def body(p):
            p.title("Steps")
            p.slide("**call**:\n1. push\n2. jump\n\n**ret**:\n1. pop\n2. resume")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture)
        lines = all_text(prs.slides[0]).split("\n")
        numbered = [l.split()[0] for l in lines if l[:1].isdigit()]
        self.assertEqual(numbered, ["1.", "2.", "1.", "2."])

    def test_cover_slide_draws_title_metadata_and_logos(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "left.png").write_bytes(_PNG)
        (src / "assets" / "right.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")
        lecture.cover(
            "Elastic model serving via efficient autoscaling",
            author="Xingda Wei",
            time="July 2026",
            logo=("assets/left.png", "assets/right.png"),
        )

        prs, _ = self.render(lecture, asset_root=src)
        slide = prs.slides[0]
        text = all_text(slide)

        self.assertIn("Elastic model serving via efficient autoscaling", text)
        self.assertIn("Xingda Wei", text)
        self.assertIn("July 2026", text)
        pics = [
            sh for sh in slide.shapes if sh.shape_type == MSO_SHAPE_TYPE.PICTURE
        ]
        self.assertEqual(len(pics), 2)

    def test_runs_carry_east_asian_typeface(self):
        # font.name only sets the Latin face; CJK text needs an <a:ea> typeface
        # or PowerPoint substitutes its default CJK font (format mismatch).
        from pptx.oxml.ns import qn
        prs, _ = self.render(self.sample())
        eas = []
        for shape in prs.slides[0].shapes:
            if not shape.has_text_frame:
                continue
            for para in shape.text_frame.paragraphs:
                for run in para.runs:
                    ea = run.font._rPr.find(qn("a:ea"))
                    if ea is not None:
                        eas.append(ea.get("typeface"))
        self.assertTrue(eas, "no run set an East Asian typeface")
        self.assertIn("PingFang SC", eas)

    def test_title_rule_is_thin_but_visible(self):
        # visible (not a hairline) yet not a heavy banner
        from pptx.enum.shapes import MSO_SHAPE_TYPE
        from pptx.util import Pt
        prs, _ = self.render(self.sample())
        rules = [
            s for s in prs.slides[0].shapes
            if s.shape_type == MSO_SHAPE_TYPE.LINE
        ]
        self.assertEqual(len(rules), 1)
        self.assertGreaterEqual(rules[0].line.width, Pt(1))
        self.assertLessEqual(rules[0].line.width, Pt(2))

    def test_slide_body_uses_exact_line_spacing_matching_the_estimate(self):
        # exact point spacing (size * 1.4), not the 1.4 multiple — PowerPoint
        # multiplies a float by the font's leading, rendering taller than the
        # estimate and overlapping the next block
        from pptx.util import Pt
        prs, _ = self.render(self.sample())
        body = max(
            (s for s in prs.slides[0].shapes if s.has_text_frame),
            key=lambda s: len(s.text_frame.paragraphs),
        )
        for p in body.text_frame.paragraphs:
            self.assertEqual(p.line_spacing, Pt(20 * 1.4))   # sample body is 20pt

    def test_text_boxes_have_zero_vertical_margin(self):
        # auto-fit grows a box by its margins; nonzero margins make the grown
        # box overlap the block below
        prs, _ = self.render(self.sample())
        body = max(
            (s for s in prs.slides[0].shapes if s.has_text_frame),
            key=lambda s: len(s.text_frame.paragraphs),
        )
        self.assertEqual(body.text_frame.margin_top, 0)
        self.assertEqual(body.text_frame.margin_bottom, 0)

    def test_notes_block_is_excluded(self):
        prs, _ = self.render(self.sample())
        self.assertNotIn("teacher only", all_text(prs.slides[0]))

    def test_code_text_present(self):
        prs, _ = self.render(self.sample())
        self.assertIn("print('hi')", all_text(prs.slides[0]))

    def test_a_demo_reaches_the_pptx_as_a_still_transcript(self):
        # A PPTX is a file on someone else's laptop: nothing in it runs, so the
        # block has to print what the deck prints when nobody presses -- and it
        # has to print it at all, which it did not before demos were runnable.
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.demo("see the assembly", "gcc -S a.c", output="a.s written")

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture)
        text = all_text(prs.slides[0])
        self.assertIn("see the assembly", text)
        self.assertIn("$ gcc -S a.c", text)
        self.assertIn("a.s written", text)

    def test_a_nameless_demo_is_its_listing_alone(self):
        # The name is optional (the deck's drawer labels such a run with its
        # first line), so the handout must not trip over its absence.
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.demo("", "make run", output="built")
            p.demo("", "make test", description="the checks")

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture)
        text = all_text(prs.slides[0])
        self.assertIn("$ make run", text)
        self.assertIn("built", text)
        self.assertIn("the checks", text)
        self.assertNotIn("— the checks", text)

    def test_code_background_matches_rows_without_an_extra_blank_line(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.code("text", "one\ntwo\nthree")

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture)
        code_box = next(
            s for s in prs.slides[0].shapes
            if s.has_text_frame and s.text_frame.text == "one\ntwo\nthree"
        )
        expected = Pt(3 * theme.CODE_PT * 1.4 + 8)
        self.assertEqual(code_box.height, expected)
        self.assertEqual(code_box.text_frame.margin_top, Pt(4))
        self.assertEqual(code_box.text_frame.margin_bottom, Pt(4))
        for paragraph in code_box.text_frame.paragraphs:
            self.assertAlmostEqual(
                paragraph.line_spacing, Pt(theme.CODE_PT * 1.4), delta=Pt(0.02)
            )

    def test_page_gap_spreads_pptx_block_groups(self):
        def make(fill):
            lecture = Lecture(id="lec", title="L")

            def body(p):
                p.title("P")
                if fill:
                    p.gap("fill")
                p.slide("first")
                p.slide("second")

            lecture.page("p1", body=body)
            return lecture

        normal, _ = self.render(make(False))
        spread, _ = self.render(make(True))

        def top_of(prs, text):
            return next(
                s.top for s in prs.slides[0].shapes
                if s.has_text_frame and s.text_frame.text == text
            )

        self.assertGreater(top_of(spread, "second"), top_of(normal, "second"))
        self.assertEqual(top_of(spread, "first"), top_of(normal, "first"))

    def test_link_carries_hyperlink(self):
        prs, _ = self.render(self.sample())
        urls = [
            run.hyperlink.address
            for shape in prs.slides[0].shapes
            if shape.has_text_frame
            for para in shape.text_frame.paragraphs
            for run in para.runs
        ]
        self.assertIn("https://example.test", urls)

    def test_table_rendered_as_native_table(self):
        prs, _ = self.render(self.sample())
        tables = [
            s for s in prs.slides[0].shapes if s.has_table
        ]
        self.assertEqual(len(tables), 1)
        table = tables[0].table
        self.assertEqual(len(table.rows), 2)   # header + 1 data row
        self.assertEqual(len(table.columns), 2)
        self.assertEqual(table.cell(0, 0).text, "H1")
        self.assertEqual(table.cell(1, 1).text, "b")

    def test_table_widths_split_the_content_width(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.table([["a", "b", "c"]], headers=["H1", "H2", "H3"], widths=[1, 1, 2])

        with lecture.section("S", id="s") as s:
            s.page("p", body=body)
        prs, _ = self.render(lecture)
        table = next(s for s in prs.slides[0].shapes if s.has_table)
        cols = [c.width for c in table.table.columns]
        self.assertEqual(sum(cols), table.width)
        self.assertEqual(cols[0], cols[1])
        self.assertAlmostEqual(cols[2] / table.width, 0.5, places=4)

    def test_sidenote_and_aside_text_present(self):
        prs, _ = self.render(self.sample())
        text = all_text(prs.slides[0])
        self.assertIn("Ref", text)
        self.assertIn("see this", text)
        self.assertIn("an aside", text)

    def test_sidenote_image_logo_is_embedded_not_shown_as_path(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "logo.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.sidenote("Book", "a note", logo="assets/logo.png")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        slide = prs.slides[0]
        # the path must not leak into the text, and the logo must be a picture
        self.assertNotIn("assets/logo.png", all_text(slide))
        pics = [
            sh for sh in slide.shapes if sh.shape_type == MSO_SHAPE_TYPE.PICTURE
        ]
        self.assertEqual(len(pics), 1)

    def test_image_embedded_as_picture(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "pic.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.image("assets/pic.png", caption="a cap")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        pics = [
            s for s in prs.slides[0].shapes
            if s.shape_type == MSO_SHAPE_TYPE.PICTURE
        ]
        self.assertEqual(len(pics), 1)
        self.assertIn("a cap", all_text(prs.slides[0]))

    def test_explicit_image_height_is_honored_without_distortion(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "pic.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.image("assets/pic.png", height_px=180)

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        pic = next(
            s for s in prs.slides[0].shapes
            if s.shape_type == MSO_SHAPE_TYPE.PICTURE
        )
        self.assertEqual(pic.height, Inches(180 / 96))
        self.assertEqual(pic.width, pic.height)  # source fixture is square

    def test_highlight_is_a_centered_toned_chip(self):
        from pptx.enum.text import PP_ALIGN

        lecture = Lecture(id="lec", title="Deck")
        lecture.page(
            "p1", body=lambda p: (p.title("T"), p.highlight("Underloaded")),
        )
        prs, _ = self.render(lecture)
        slide = prs.slides[0]
        chips = [s for s in slide.shapes if s.has_text_frame
                 and s.text_frame.text == "Underloaded"]
        self.assertEqual(len(chips), 1)
        chip = chips[0]
        para = chip.text_frame.paragraphs[0]
        self.assertEqual(para.alignment, PP_ALIGN.CENTER)
        self.assertTrue(para.runs[0].font.bold)
        self.assertEqual(chip.fill.type, MSO_FILL.BACKGROUND)
        self.assertEqual(para.runs[0].font.color.rgb, theme.CHIP_INK["yellow"])
        # the frame carries the tone too — it is what makes the chip read as one
        # object with nothing painted inside it
        self.assertEqual(chip.line.color.rgb, theme.CHIP_INK["yellow"])

    def test_highlight_chip_is_narrower_than_the_content_column(self):
        lecture = Lecture(id="lec", title="Deck")
        lecture.page("p1", body=lambda p: (p.title("T"), p.highlight("短")))
        prs, _ = self.render(lecture)
        chip = next(s for s in prs.slides[0].shapes if s.has_text_frame
                    and s.text_frame.text == "短")
        layout = Layout.from_ratio("16:9")
        self.assertLess(chip.width, layout.content_width)
        self.assertGreater(chip.width, 0)

    def test_highlight_uses_one_paragraph_per_line(self):
        lecture = Lecture(id="lec", title="Deck")
        lecture.page(
            "p1",
            body=lambda p: (p.title("T"), p.highlight("a\nb", tone="orange")),
        )
        prs, _ = self.render(lecture)
        chip = next(s for s in prs.slides[0].shapes if s.has_text_frame
                    and s.text_frame.text.replace("\n", "") == "ab")
        self.assertEqual(len(chip.text_frame.paragraphs), 2)
        self.assertEqual(
            chip.text_frame.paragraphs[0].runs[0].font.color.rgb,
            theme.CHIP_INK["orange"],
        )

    def test_annotation_is_the_only_deferred_block(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.slide("text").annotate("a bubble")

        with lecture.section("S", id="s") as s:
            s.page("p1", body=body)
        prs, _ = self.render(lecture)   # must not raise
        self.assertEqual(len(prs.slides), 1)
        self.assertNotIn("a bubble", all_text(prs.slides[0]))

    def test_image_right_sits_beside_narrowed_text(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "pic.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.slide("beside the picture").image_right("assets/pic.png", width_px=200)

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        slide = prs.slides[0]
        pic = next(
            s for s in slide.shapes if s.shape_type == MSO_SHAPE_TYPE.PICTURE
        )
        layout = Layout.from_ratio("16:9")
        self.assertEqual(pic.width, Inches(200 / 96))
        # flush with the content box's right edge
        self.assertEqual(
            pic.left + pic.width, layout.content_left + layout.content_width
        )
        text = next(s for s in slide.shapes if s.has_text_frame
                    and "beside the picture" in s.text_frame.text)
        # the text box stops short of the picture rather than running under it
        self.assertLessEqual(text.left + text.width, pic.left)

    def _side_deck(self, side_image, *, notes=(), text_notes=()):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "pic.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            handle = p.side_image("assets/pic.png", **side_image)
            for note in notes:
                handle.footnote(note)
            text = p.slide("beside the picture")
            for note in text_notes:
                text.footnote(note)

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        slide = prs.slides[0]
        pic = next(s for s in slide.shapes if s.shape_type == MSO_SHAPE_TYPE.PICTURE)
        title, text = (
            next(s for s in slide.shapes if s.has_text_frame and s.text_frame.text == name)
            for name in ("P", "beside the picture")
        )
        return slide, pic, title, text

    def test_side_image_takes_a_column_and_the_text_reflows_beside_it(self):
        _, pic, title, text = self._side_deck({"width": "40%", "alt": "contain"})
        layout = Layout.from_ratio("16:9")
        column = px(model.side_column_px("40%", 1120))
        # the title keeps the padding box's width, rule included
        self.assertEqual(title.left, layout.content_left)
        self.assertEqual(title.width, layout.content_width)
        # the column is its share of the body, against the right padding; the
        # square picture is as wide as it and sits at the top of the body
        self.assertEqual(pic.left + pic.width, layout.content_left + layout.content_width)
        self.assertEqual(pic.width, column)
        self.assertEqual(pic.height, column)
        self.assertGreater(text.top, title.top + title.height)
        self.assertEqual(pic.top, text.top)
        # the text takes the rest of the body, 40px short of the column
        self.assertEqual(text.left, layout.content_left)
        self.assertLessEqual(text.left + text.width, pic.left - px(40))

    def test_fitted_side_image_on_the_left_keeps_the_slide_margins(self):
        _, pic, _, text = self._side_deck(
            {"width": "30%", "alt": "contain", "side": "left"})
        layout = Layout.from_ratio("16:9")
        column = px(model.side_column_px("30%", 1120))
        # the picture starts in line with the title and ends 40px from the text
        self.assertEqual(pic.left, layout.content_left)
        self.assertEqual(pic.left + pic.width, text.left - px(40))
        self.assertEqual(text.left, layout.content_left + column + px(40))
        self.assertLessEqual(pic.top + pic.height, px(720 - 70))

    def test_side_image_on_the_left_covers_its_column(self):
        _, pic, title, text = self._side_deck({"side": "left"})
        layout = Layout.from_ratio("16:9")
        self.assertEqual(title.left, layout.content_left)
        self.assertEqual(title.width, layout.content_width)
        self.assertGreater(pic.top, title.top + title.height)
        # half the body, filled from under the title to the footnote band and
        # cropped; nothing reaches the slide's edges
        self.assertEqual(pic.left, layout.content_left)
        self.assertEqual(pic.width, px(560))
        self.assertEqual(pic.top + pic.height, px(720 - 70))
        # under the title the column is wider than it is tall, so the square
        # picture fills its width and loses its top and bottom
        self.assertGreater(pic.crop_top, 0)
        self.assertAlmostEqual(pic.crop_top, pic.crop_bottom)
        self.assertEqual(pic.crop_left, 0)
        self.assertEqual(text.left, layout.content_left + px(560 + 40))
        # the text starts level with the column, under the title
        self.assertEqual(text.top, pic.top)

    def _footnote_box(self, slide):
        return next(s for s in slide.shapes if s.has_text_frame
                    and s.text_frame.text.startswith("1 "))

    def test_footnotes_span_the_padding_box_on_a_side_page(self):
        layout = Layout.from_ratio("16:9")
        for side in ("left", "right"):
            with self.subTest(side=side):
                slide, pic, _, text = self._side_deck(
                    {"width": "40%", "alt": "contain", "side": side},
                    notes=["picture source"], text_notes=["claim"])
                band = self._footnote_box(slide)
                # one band under both columns, numbered image column first
                self.assertEqual(band.text_frame.text, "1 picture source\n2 claim")
                self.assertEqual(band.left, layout.content_left)
                self.assertEqual(band.width, layout.content_width)
                self.assertEqual(band.top, px(720 - 70))
                self.assertEqual(band.top + band.height, layout.content_bottom)
                self.assertLessEqual(pic.top + pic.height, band.top)

    def test_the_footnote_band_grows_and_the_body_ends_where_it_begins(self):
        from PIL import Image

        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        # far taller than the body, so its height is the room the body leaves
        Image.new("RGB", (100, 1000), "white").save(src / "assets" / "pic.png")

        def deck(count):
            lecture = Lecture(id="lec", title="L")

            def body(p):
                p.title("P")
                image = p.image("assets/pic.png")      # fills the body's height
                for i in range(count):
                    image.footnote(f"note {i}")

            lecture.page("p1", body=body)
            prs, _ = self.render(lecture, asset_root=src)
            slide = prs.slides[0]
            pic = next(s for s in slide.shapes if s.shape_type == MSO_SHAPE_TYPE.PICTURE)
            return pic, self._footnote_box(slide)

        layout = Layout.from_ratio("16:9")
        three, band3 = deck(3)
        five, band5 = deck(5)
        # up to three lines the band keeps its least height
        self.assertEqual(band3.height, px(56))
        self.assertEqual(band5.height, px(56) + 2 * Emu(Pt(10 * 1.3)))
        for pic, band in ((three, band3), (five, band5)):
            self.assertEqual(band.top + band.height, layout.content_bottom)
            self.assertEqual(band.width, layout.content_width)
            self.assertLessEqual(pic.top + pic.height, band.top)
        # the taller band leaves the picture less room
        self.assertLess(five.height, three.height)

    def test_architecture_draws_a_band_and_a_box_per_module(self):
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            arch = p.architecture(caption="a cap", flow="down")
            arch.layer("Top", ["m1", "m2"])
            arch.layer("Bottom", ["m3", ...])

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture)
        slide = prs.slides[0]
        layout = Layout.from_ratio("16:9")
        boxes = rounded_boxes(slide)
        bands = [s for s in boxes if s.width == layout.content_width]
        modules = [s for s in boxes if s.width != layout.content_width]
        self.assertEqual(len(bands), 2)
        self.assertEqual(len(modules), 4)
        text = all_text(slide)
        for label in ("Top", "Bottom", "m1", "m2", "m3", "a cap"):
            self.assertIn(label, text)
        self.assertIn(model.ARCH_FLOW_GLYPH["down"], text)
        self.assertIn(model.ARCH_ELLIPSIS, text)

    def test_architecture_shrinks_to_fit_the_room_left(self):
        def deck(rows):
            lecture = Lecture(id="lec", title="L")

            def body(p):
                p.title("P")
                arch = p.architecture()
                for i in range(rows):
                    arch.layer(f"L{i}", ["m1", "m2"])

            lecture.page("p1", body=body)
            prs, _ = self.render(lecture)
            return prs.slides[0]

        layout = Layout.from_ratio("16:9")
        tall = [s for s in rounded_boxes(deck(12))
                if s.width == layout.content_width]
        short = [s for s in rounded_boxes(deck(3))
                 if s.width == layout.content_width]
        self.assertLess(tall[0].height, short[0].height)
        # within a point of the content box's floor — per-band rounding is the
        # only slack, not another band's worth of height
        self.assertLessEqual(
            tall[-1].top + tall[-1].height, px(720 - 70) + Emu(Pt(1))
        )

    def test_row_centers_top_aligned_pictures(self):
        src = Path(tempfile.mkdtemp())
        (src / "assets").mkdir()
        (src / "assets" / "pic.png").write_bytes(_PNG)
        lecture = Lecture(id="lec", title="L")

        def body(p):
            p.title("P")
            p.row(caption="row cap")\
             .image("assets/pic.png", height_px=100, caption="left")\
             .image("assets/pic.png", height_px=100, caption="right")

        lecture.page("p1", body=body)
        prs, _ = self.render(lecture, asset_root=src)
        slide = prs.slides[0]
        pics = [s for s in slide.shapes
                if s.shape_type == MSO_SHAPE_TYPE.PICTURE]
        self.assertEqual(len(pics), 2)
        self.assertEqual(pics[0].top, pics[1].top)       # top-aligned
        self.assertEqual(pics[0].height, Inches(100 / 96))
        layout = Layout.from_ratio("16:9")
        margin_left = pics[0].left - layout.content_left
        margin_right = (layout.content_left + layout.content_width
                        - (pics[1].left + pics[1].width))
        self.assertAlmostEqual(margin_left, margin_right, delta=Emu(Pt(1)))
        text = all_text(slide)
        for label in ("left", "right", "row cap"):
            self.assertIn(label, text)

if __name__ == "__main__":
    unittest.main()
