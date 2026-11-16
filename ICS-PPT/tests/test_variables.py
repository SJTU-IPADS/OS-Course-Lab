"""Deck variables: ``lecture.variable``, ``$name`` references, the controls, bound figures.

What is checked here is the contract each target relies on: the model keeps a
reference as written and refuses one that cannot be drawn; the static targets
draw the default; the deck spells a variable as a CSS custom property with the
default as fallback, marks the pictures of bound figures and carries the
definitions and figure sources ``vars.js`` reads. The expression language of
bound figures is tested in ``test_bind.py``. The browser side is exercised in
headless Chrome by hand (see the report of the change); here its files are
checked for what the build side promises them.
"""

import json
import re
import tempfile
import textwrap
import unittest
from pathlib import Path

from lecturekit import i18n, model, variables
from lecturekit.bind import Bindings
from lecturekit.cli import format_tree, load_lecture
from lecturekit.dsl import Lecture
from lecturekit.model import ValidationError
from lecturekit.renderers.viewer import build_marp_markdown
from lecturekit.renderers.viewer.marp import ASSETS_DIR, DECK_SCRIPTS, inject_deck_scripts
from lecturekit.serialize import lecture_to_dict


def lecture_with(body, *declare, lang=None):
    """A one-page lecture declaring ``declare`` (``(name, default, kwargs)``)."""
    lecture = Lecture(id="lec-v", title="Variables", lang=lang)
    for name, default, kwargs in declare:
        lecture.variable(name, default, **kwargs)
    lecture.page("p", body=body)
    return lecture


T = ("T", 1, {"min": 0, "max": 10})
SIZE = ("size", 480, {"min": 100, "max": 800, "step": 10})
GAP = ("gap", 24, {"min": 8, "max": 64, "step": 8})


def blocks(lecture: model.Lecture) -> list[model.Block]:
    return model.flatten_pages(lecture.children)[0].blocks


class DeclarationTest(unittest.TestCase):
    def test_a_declaration_reaches_the_model_and_its_outputs(self):
        def body(p):
            p.title("t")
            p.slider("T")

        built = lecture_with(body, T, SIZE).build()
        self.assertEqual(built.variables, (model.Variable("T", 1, 0, 10),
                                           model.Variable("size", 480, 100, 800, 10)))
        tree = format_tree(built)
        self.assertIn("  $T = 1 in [0, 10]\n", tree)
        self.assertIn("  $size = 480 in [100, 800] step 10\n", tree)
        self.assertEqual(lecture_to_dict(built)["variables"][0],
                         {"name": "T", "default": 1, "min": 0, "max": 10, "step": None})

    def test_a_value_is_printed_with_the_decimals_it_needs(self):
        cases = [((1, 0), "1"), ((1, 1), "1.0"), ((0.37, 1), "0.37"), ((7.25, 0), "7.25"),
                 ((-0.0, 0), "0"), ((1e-7, 0), "0.0000001"), ((10.0, 0), "10"),
                 ((-2.5, 2), "-2.50")]
        for (value, places), want in cases:
            with self.subTest(value=value, places=places):
                self.assertEqual(variables.number_text(value, places), want)

    def test_a_slider_steps_by_its_own_step_the_variables_or_a_power_of_ten(self):
        free = model.Variable("T", 1, 0, 10)
        self.assertEqual(variables.slider_step(free), 0.1)
        self.assertEqual(variables.slider_step(free, 0.5), 0.5)
        self.assertEqual(variables.slider_step(model.Variable("w", 300, 0, 600)), 1)
        self.assertEqual(variables.slider_step(model.Variable("a", 1, 0, 3)), 0.01)
        self.assertEqual(variables.slider_step(model.Variable("n", 480, 100, 800, 10)), 10)

    def test_a_malformed_declaration_is_refused_at_build(self):
        def body(p):
            p.title("t")
            p.slide("x")

        cases = {
            "1T": (1, {"min": 0, "max": 2}),
            "a": (1, {"min": 2, "max": 2}),
            "b": (1, {"min": 0, "max": 2, "step": 0}),
            "c": (1, {"min": 0, "max": 1, "step": 0.3}),     # max off the grid
            "d": (5, {"min": 0, "max": 2}),                  # default outside
            "e": (0.5, {"min": 0, "max": 2, "step": 1}),     # default off the grid
            "f": (True, {"min": 0, "max": 2}),               # a bool is no number
            "g": (1, {"min": 0, "max": float("inf")}),
        }
        for name, (default, kwargs) in cases.items():
            with self.subTest(name=name), self.assertRaises(ValidationError) as caught:
                lecture_with(body, (name, default, kwargs)).build()
            self.assertIn(repr(name), str(caught.exception))

    def test_without_a_step_any_value_in_the_range_is_one(self):
        free = model.Variable("T", 1, 0, 10)
        self.assertTrue(variables.on_grid(free, 0.37))
        self.assertFalse(variables.on_grid(free, 10.5))
        self.assertFalse(variables.on_grid(model.Variable("n", 1, 0, 10, 1), 0.37))

    def test_a_name_is_declared_once(self):
        lecture = Lecture(id="l", title="t")
        lecture.variable("T", 1, min=0, max=2)
        with self.assertRaises(ValidationError):
            lecture.variable("T", 1, min=0, max=2)


class ReferenceTest(unittest.TestCase):
    def test_sizes_store_the_reference_with_its_unit(self):
        def body(p):
            p.title("t")
            p.image("assets/a.svg", width_px="$size", height_pct="${size}")
            p.image("assets/b.svg").width_pct("$size").height_px("$size")
            p.row().image("assets/c.svg", width_px="$size")
            p.slide("x").image_right("assets/d.svg", width_px="$size")
            p.frames("assets/e.svg", "assets/f.svg", width_px="$size")

        pages = model.flatten_pages(lecture_with(body, SIZE).build().children)
        b = pages[0].blocks
        self.assertEqual((b[0].content["width"], b[0].content["height"]), ("${size}px", "${size}%"))
        self.assertEqual((b[1].content["width"], b[1].content["height"]), ("${size}%", "${size}px"))
        self.assertEqual(b[2].content["items"][0]["width"], "${size}px")
        self.assertEqual(b[3].float_image["width"], "${size}px")
        self.assertEqual(b[4].content["width"], "${size}px")

    def test_a_string_that_is_no_reference_is_refused_where_it_is_written(self):
        def body(p):
            p.title("t")
            p.image("assets/a.svg", width_px="480")

        with self.assertRaises(ValidationError) as caught:
            lecture_with(body, SIZE).build()
        self.assertIn("width_px", str(caught.exception))

    def test_a_number_laid_out_at_build_time_takes_no_variable(self):
        for call in (lambda p: p.side_image("assets/a.svg", width="$size"),
                     lambda p: p.slide("x").annotate("note", dx="$size")):
            def body(p, call=call):
                p.title("t")
                call(p)

            with self.subTest(), self.assertRaises(ValidationError) as caught:
                lecture_with(body, SIZE).build()
            self.assertIn("does not take a variable", str(caught.exception))

    def test_an_image_path_is_a_path(self):
        def body(p):
            p.title("t")
            p.image("assets/${T}.svg")

        built = lecture_with(body, T).build()
        self.assertEqual(blocks(variables.for_static(built))[0].content["src"], "assets/${T}.svg")

    def test_an_undeclared_variable_is_refused_and_the_declared_ones_named(self):
        for call in (lambda p: p.image("assets/a.svg", width_px="$wide"),
                     lambda p: p.spacer("$wide"),
                     lambda p: p.slider("wide"),
                     lambda p: p.input("wide")):
            def body(p, call=call):
                p.title("t")
                call(p)

            with self.subTest(), self.assertRaises(ValidationError) as caught:
                lecture_with(body, SIZE).build()
            self.assertIn("$wide", str(caught.exception))
            self.assertIn("$size", str(caught.exception))

    def test_a_variable_must_fit_where_it_is_used(self):
        negative = ("shift", 0, {"min": -10, "max": 10})
        fraction = ("half", 1.5, {"min": 0.5, "max": 4, "step": 0.5})
        free = ("free", 4, {"min": 1, "max": 8})
        zero = ("n", 4, {"min": 0, "max": 8, "step": 1})
        cases = [
            (lambda p: p.image("assets/a.svg", width_px="$shift"), negative, "negative"),
            (lambda p: p.spacer("$half"), fraction, "whole number"),
            (lambda p: p.spacer("$free"), free, "whole number"),
            (lambda p: p.spacer("$n"), zero, "from 1 up"),
        ]
        for call, declared, message in cases:
            def body(p, call=call):
                p.title("t")
                call(p)

            with self.subTest(message), self.assertRaises(ValidationError) as caught:
                lecture_with(body, declared).build()
            self.assertIn(message, str(caught.exception))

    def test_a_slider_mark_is_a_value_of_its_variable(self):
        half = ("h", 1, {"min": 0, "max": 2, "step": 0.5})
        for declared, marks in ((T, [0, 11]), (half, [0, 0.25])):
            def body(p, marks=marks, name=declared[0]):
                p.title("t")
                p.slider(name, marks=marks)

            with self.subTest(marks=marks), self.assertRaises(ValidationError) as caught:
                lecture_with(body, declared).build()
            self.assertIn("mark", str(caught.exception))

    def test_a_slider_step_is_positive_and_on_the_variables_grid(self):
        half = ("h", 1, {"min": 0, "max": 2, "step": 0.5})
        for declared, step in ((T, 0), (T, -1), (half, 0.3)):
            def body(p, step=step, name=declared[0]):
                p.title("t")
                p.slider(name, step=step)

            with self.subTest(step=step), self.assertRaises(ValidationError) as caught:
                lecture_with(body, declared).build()
            self.assertIn("step", str(caught.exception))

    def test_an_empty_label_is_refused(self):
        def body(p):
            p.title("t")
            p.input("T", label=" ")

        with self.assertRaises(ValidationError):
            lecture_with(body, T).build()


def sample(p):
    p.title("Temperature")
    p.slider("T", marks=[0, 1, 10])
    p.input("T", label="温度 T")
    p.image("assets/t.svg", width_px="$size")
    p.spacer("$gap")


class StaticTest(unittest.TestCase):
    def test_every_reference_takes_the_default(self):
        built = variables.for_static(lecture_with(sample, T, SIZE, GAP).build())
        b = blocks(built)
        self.assertEqual(b[2].content["src"], "assets/t.svg")
        self.assertNotIn("figure", b[2].content)
        self.assertEqual(b[2].content["width"], "480px")
        self.assertEqual(b[3].content["px"], 24)
        self.assertEqual(b[0].content["variable"]["default"], 1)
        self.assertEqual(b[0].content["step_used"], 0.1)

    def test_a_control_prints_as_one_complete_line(self):
        b = blocks(variables.for_static(lecture_with(sample, T, SIZE, GAP).build()))
        # Without a label, the variable's name as a math symbol; the slider's
        # value with the decimals of its step; range and marks as written.
        self.assertEqual(variables.control_text(b[0].content),
                         "$T$ = 1.0（取值 0 – 10，刻度 0、1、10）")
        self.assertEqual(variables.control_text(b[1].content),
                         "温度 T = 1（取值 0 – 10）")
        english = variables.for_static(lecture_with(sample, T, SIZE, GAP, lang="en").build())
        self.assertEqual(variables.control_text(blocks(english)[0].content),
                         "$T$ = 1.0 (range 0 – 10, ticks 0, 1, 10)")

    def test_pptx_prints_the_line_with_the_symbol_as_math(self):
        from pptx import Presentation
        from lecturekit.renderers.pptx import PptxRenderer
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "assets").mkdir()
            (root / "assets" / "t.png").write_bytes(_PNG)

            def body(p):
                p.title("Temperature")
                p.slider("T", marks=[0, 1, 10])
                p.image("assets/t.png", width_px="$size")

            out = PptxRenderer(asset_root=root).render(
                lecture_with(body, T, SIZE).build(), root / "out")
            slide = Presentation(str(out)).slides[0]
            boxes = [s for s in slide.shapes if s.has_text_frame
                     and "（取值 0 – 10，刻度 0、1、10）" in s.text_frame.text]
            self.assertEqual(len(boxes), 1)
            self.assertIn("oMath", boxes[0]._element.xml)
            self.assertEqual(sum(1 for s in slide.shapes if s.shape_type == 13), 1)

    def test_transcript_and_book_print_the_line(self):
        from lecturekit.renderers.transcript import build_html
        from lecturekit.renderers.transcript.images import Embedder
        built = lecture_with(sample, T, SIZE, GAP).build()
        html = build_html(built, Embedder(None))
        self.assertIn('<p class="tx-control"><span class="tx-mathspan">T</span> = 1.0'
                      '（取值 0 – 10，刻度 0、1、10）</p>', html)
        self.assertIn('<p class="tx-control">温度 T = 1（取值 0 – 10）</p>', html)

        from lecturekit.renderers.latex.assets import AssetCopier
        from lecturekit.renderers.latex.blocks import Ctx
        with tempfile.TemporaryDirectory() as tmp:
            ctx = Ctx(lecture_id="lec-v", page_id="p", slide_width=1280,
                      assets=AssetCopier(Path(tmp)), asset_root=Path(tmp))
            slider, box = blocks(variables.for_static(built))[:2]
            self.assertEqual(slider.to_latex(ctx),
                             "\\begin{center}\n$T$ = 1.0（取值 0 – 10，刻度 0、1、10）\n\\end{center}")
            self.assertEqual(box.to_latex(ctx),
                             "\\begin{center}\n温度 T = 1（取值 0 – 10）\n\\end{center}")

    def test_a_control_drawn_before_resolution_says_so(self):
        control = blocks(lecture_with(sample, T, SIZE, GAP).build())[0]
        with self.assertRaises(ValidationError) as caught:
            control.to_marp(None)
        self.assertIn("for_deck", str(caught.exception))


class DeckTest(unittest.TestCase):
    def markdown(self, *declare, body=sample) -> str:
        return build_marp_markdown(lecture_with(body, *declare).build())

    def test_sizes_become_custom_properties_with_the_default_as_fallback(self):
        md = self.markdown(T, SIZE, GAP)
        self.assertIn('style="width:calc(var(--lkv-size, 480) * 1px)"', md)
        self.assertIn('style="height:calc(var(--lkv-gap, 24) * 1px)"', md)

    def test_the_controls_carry_their_range_and_default(self):
        md = self.markdown(T, SIZE, GAP)
        # The slider: the symbol, a box for the value (any value), the range
        # input in the slider's step, the marks as written.
        self.assertIn('<div class="lk-control lk-slider" data-lk-var="T" data-lk-places="1">', md)
        self.assertIn('<span class="lk-control-label lk-control-symbol">T</span>', md)
        self.assertIn('<input type="number" class="lk-control-value lk-slider-value" min="0" '
                      'max="10" step="any" value="1.0" aria-label="T" style="--lk-chars:5">', md)
        self.assertIn('<input type="range" min="0" max="10" step="0.1" value="1" aria-label="T">', md)
        self.assertIn('data-lk-value="10" style="--lk-at:1.0000">10</span>', md)
        self.assertIn('title="恢复默认值 1.0"', md)
        # The input box: its label as text, the value as written, the range.
        self.assertIn('<div class="lk-control lk-input" data-lk-var="T" data-lk-places="0">', md)
        self.assertIn('<span class="lk-control-label">温度 T</span>', md)
        self.assertIn('<input type="number" class="lk-control-value" min="0" max="10" '
                      'step="any" value="1" aria-label="温度 T"', md)
        self.assertIn('<span class="lk-control-range">0 – 10</span>', md)
        # One line of HTML each, so Marp keeps it one block.
        self.assertTrue(all("\n" not in line for line in md.splitlines()))

    def test_a_variable_with_a_step_steps_the_box_too(self):
        def body(p):
            p.title("t")
            p.input("size")

        md = self.markdown(SIZE, body=body)
        self.assertIn('min="100" max="800" step="10" value="480"', md)

    def test_each_page_that_uses_a_variable_carries_its_definitions(self):
        lecture = Lecture(id="lec-v", title="Variables")
        lecture.variable("T", 1, min=0, max=10)
        lecture.variable("size", 480, min=100, max=800, step=10)

        def plain(p):
            p.title("plain")
            p.slide("no variable here")

        def sized(p):
            p.title("sized")
            p.image("assets/a.svg", width_px="$size")

        lecture.page("plain", body=plain)
        lecture.page("sized", body=sized)
        md = build_marp_markdown(lecture.build())
        tables = re.findall(r'<script type="application/json" data-lk-vars>(.*?)</script>', md)
        self.assertEqual(len(tables), 1)
        self.assertEqual(json.loads(tables[0]), {"lecture": "lec-v", "vars": {"size": {
            "name": "size", "default": 480, "min": 100, "max": 800, "step": 10}}})

    def test_a_deck_without_variables_is_drawn_as_before(self):
        def body(p):
            p.title("t")
            p.image("assets/a.svg", width_px=480)
            p.spacer(24)

        md = build_marp_markdown(lecture_with(body).build())
        self.assertNotIn("data-lk-vars", md)
        self.assertIn('style="width:480px"', md)
        self.assertIn('style="height:24px"', md)

    def test_bind_js_and_vars_js_ride_every_deck(self):
        self.assertLess(DECK_SCRIPTS.index("bind.js"), DECK_SCRIPTS.index("vars.js"))
        script = (ASSETS_DIR / "vars.js").read_text(encoding="utf-8")
        # The names the build side writes and the script reads.
        for promise in ('"lecturekit:vars:"', '"--lkv-"', "script[data-lk-vars]",
                        "script[data-lk-figure]", "img[data-lk-figure]", "data-lk-var",
                        "data-lk-places", "data-lk-value", "data-lk-live",
                        "lk-control-reset", '"storage"', "window.lkBind"):
            self.assertIn(promise, script)
        for name in ("vars.js", "bind.js"):
            self.assertNotIn("</script", (ASSETS_DIR / name).read_text(encoding="utf-8").lower())
        html = inject_deck_scripts("<html><body></body></html>")
        self.assertIn("lecturekit:vars:", html)
        self.assertIn("lkBind", html)


def write_figure(path: Path, drawn: float = 1, *, reads: str = "T", tamper: bool = False) -> None:
    """A small bound figure: two bars from softmax over ``reads``."""
    bind = Bindings(**{reads: drawn})
    bind.let("p", f"softmax([1, 2], {reads})")
    width = bind.attrs('<rect x="0" y="0" width="0" height="10"/>', width="p[1] * 100")
    label = bind.text('<text x="0" y="30"></text>', "fmt(p[1], 2)")
    if tamper:
        label = label.replace(">0.73<", ">0.50<")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 40">'
                    f"{bind.declare()}{width}{label}</svg>", encoding="utf-8")


FIGURE_LECTURE = """
from lecturekit.dsl import Lecture
lecture = Lecture(id="fig", title="Figure")
lecture.variable("T", 1, min=0, max=10)

def body(p):
    p.title("t")
    p.slider("T")
    p.image("assets/f.svg", width_px=400)

def bare(p):
    p.title("bare")
    p.row().image("assets/f.svg")
    p.slide("x").image_right("assets/f.svg")

def plain(p):
    p.title("plain")
    p.image("assets/plain.svg")

lecture.page("p", body=body)
lecture.page("bare", body=bare)
lecture.page("plain", body=plain)
"""


class FigureTest(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        (self.root / "lecture.py").write_text(FIGURE_LECTURE, encoding="utf-8")
        (self.root / "assets").mkdir()
        (self.root / "assets" / "plain.svg").write_text("<svg xmlns='http://www.w3.org/2000/svg'/>")

    def test_a_bound_figure_at_the_defaults_loads(self):
        write_figure(self.root / "assets" / "f.svg")
        lecture = load_lecture(self.root)
        found = variables.figures(lecture, self.root)
        self.assertEqual(list(found), ["assets/f.svg"])
        self.assertEqual(found["assets/f.svg"].inputs, {"T": 1.0})

    def test_the_deck_marks_its_pictures_and_carries_its_source(self):
        write_figure(self.root / "assets" / "f.svg")
        md = build_marp_markdown(load_lecture(self.root), asset_root=self.root)
        slides = md.split("\n---\n")
        first, bare, plain = slides[1], slides[2], slides[3]
        self.assertIn('<img src="assets/f.svg" data-lk-figure="assets/f.svg"', first)
        self.assertRegex(first, r'<script type="image/svg\+xml" data-lk-figure="assets/f.svg">'
                                r'<svg xmlns=[^\n]*data-lk-vars="T = 1"')
        # A page that shows the figure but no control still carries T, and
        # the picture in a row and beside the text is marked as well.
        self.assertIn('"T": {"name": "T"', bare)
        self.assertEqual(bare.count('data-lk-figure="assets/f.svg"'), 3)
        self.assertNotIn("data-lk-", plain)

    def test_a_figure_not_drawn_at_the_default_is_refused(self):
        write_figure(self.root / "assets" / "f.svg", drawn=2)
        with self.assertRaises(ValidationError) as caught:
            load_lecture(self.root)
        self.assertIn("draw it at the default", str(caught.exception))

    def test_a_figure_reading_an_undeclared_variable_is_refused(self):
        write_figure(self.root / "assets" / "f.svg", reads="U")
        with self.assertRaises(ValidationError) as caught:
            load_lecture(self.root)
        self.assertIn("$U", str(caught.exception))

    def test_a_file_that_differs_from_its_bindings_is_refused(self):
        write_figure(self.root / "assets" / "f.svg", tamper=True)
        with self.assertRaises(ValidationError) as caught:
            load_lecture(self.root)
        self.assertIn("binding gives '0.73'", str(caught.exception))

    def test_static_targets_take_the_file_as_it_is(self):
        write_figure(self.root / "assets" / "f.svg")
        b = blocks(variables.for_static(load_lecture(self.root)))
        self.assertEqual(b[1].content["src"], "assets/f.svg")
        self.assertNotIn("figure", b[1].content)


class TranslationTest(unittest.TestCase):
    def test_a_label_is_a_translation_entry(self):
        lecture = lecture_with(sample, T, SIZE, GAP).build()
        keys = {entry.key: entry.text for entry in i18n.collect(lecture)}
        self.assertEqual(keys.get("p.input.1.label"), "温度 T")
        # A slider without a label has nothing to translate.
        self.assertFalse(any(key.startswith("p.slider") for key in keys))


class ReviewTest(unittest.TestCase):
    SOURCE = textwrap.dedent("""
        from lecturekit.dsl import Lecture
        lecture = Lecture(id="src", title="Source")
        lecture.variable("T", 1, min=0, max=2, step=1)
        lecture.variable("U", 1, min=0, max=10)

        def body(p):
            p.title("t")
            p.slider("T")

        def figure(p):
            p.title("f")
            p.image("assets/f.svg")

        lecture.page("dial", body=body)
        lecture.page("figure", body=figure)
    """)
    HOST = textwrap.dedent("""
        from lecturekit.dsl import Lecture, review_section
        lecture = Lecture(id="host", title="Host")
        {declare}
        review_section(lecture, "review", {{"../source": {pages}}})
    """)

    def load(self, declare: str, pages=("dial",)) -> model.Lecture:
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        root = Path(tmp.name)
        host = self.HOST.format(declare=declare, pages=list(pages))
        for name, text in (("source", self.SOURCE), ("host", host)):
            (root / name).mkdir()
            (root / name / "lecture.py").write_text(text)
        write_figure(root / "source" / "assets" / "f.svg", reads="U")
        return load_lecture(root / "host")

    def test_a_borrowed_page_brings_its_variables(self):
        self.assertEqual([v.name for v in self.load("").variables], ["T"])
        same = 'lecture.variable("T", 1, min=0, max=2, step=1)'
        self.assertEqual(len(self.load(same).variables), 1)

    def test_a_borrowed_bound_figure_brings_the_variables_it_reads(self):
        self.assertEqual([v.name for v in self.load("", pages=("figure",)).variables], ["U"])

    def test_one_name_declared_differently_is_refused(self):
        with self.assertRaises(ValidationError) as caught:
            self.load('lecture.variable("T", 1, min=0, max=3, step=1)')
        self.assertIn("declared twice", str(caught.exception))


# A 1x1 transparent PNG, for the PowerPoint test (it rasterizes no SVG).
_PNG = bytes.fromhex(
    "89504e470d0a1a0a0000000d4948445200000001000000010806000000"
    "1f15c4890000000d49444154789c6360000002000154a24f5d0000000049454e44ae426082"
)


if __name__ == "__main__":
    unittest.main()
