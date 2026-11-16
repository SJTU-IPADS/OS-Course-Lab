"""The expression language of bound figures (``lecturekit.bind``) and its twin.

The viewer evaluates the same expressions in ``bind.js``; a figure is right
in the browser only if both give the same values and print them alike. The
parity tests run ``bind.js`` under Node (which rendering needs anyway) and
skip without it.
"""

import json
import math
import random
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from lecturekit import bind
from lecturekit.bind import BindError, Bindings
from lecturekit.renderers.viewer.marp import ASSETS_DIR

X = [6.0, -5.0, 4.0, 1.5, 9.9, -2.3, 4.0]
ENV = {"T": 1.0, "x": X}


def value(source, env=ENV):
    return bind.evaluate(bind.parse(source), env)


class LanguageTest(unittest.TestCase):
    def test_arithmetic_and_precedence(self):
        cases = {
            "1 + 2 * 3 - 4 / 2": 5.0, "(1 + 2) * 3": 9.0, "-2^2": -4.0, "2^3^2": 512.0,
            "2^-1": 0.5, "7 % 3": 1.0, "-7 % 3": -1.0, "1 < 2 && 2 <= 2": True,
            "!0 || 0": True, "T == 1 ? 10 : 20": 10.0, "x[4]": 9.9, "len(x)": 7.0,
            "max(x)": 9.9, "min(3, 1, 2)": 1.0, "clamp(5, 0, 3)": 3.0, "abs(-2)": 2.0,
            "round(2.5)": 3.0, "round(0.125, 2)": 0.13, "floor(-0.5)": -1.0,
            "'T = ' + fmt(T)": "T = 1", "pow(2, 10)": 1024.0,
        }
        for source, want in cases.items():
            with self.subTest(source):
                self.assertEqual(value(source), want)

    def test_lists_combine_item_by_item(self):
        self.assertEqual(value("[1, 2] * 3 + [1, 1]"), [4.0, 7.0])
        self.assertEqual(value("x / 2")[:2], [3.0, -2.5])
        self.assertEqual(value("abs([-1, 2])"), [1.0, 2.0])
        with self.assertRaises(BindError):
            value("[1, 2] + [1]")

    def test_division_is_ieee(self):
        self.assertEqual(value("1 / 0"), math.inf)
        self.assertEqual(value("-1 / 0"), -math.inf)
        self.assertTrue(math.isnan(value("0 / 0")))
        self.assertEqual(value("log(0)"), -math.inf)

    def test_softmax_and_its_limit(self):
        p = value("softmax(x, T)")
        self.assertAlmostEqual(sum(p), 1.0)
        self.assertAlmostEqual(p[4], 0.9747053845620733)
        self.assertEqual(value("softmax(x, 0)"), [0, 0, 0, 0, 1, 0, 0])
        self.assertEqual(value("softmax([2, 2, 0], 0)"), [0.5, 0.5, 0])
        self.assertEqual(value("softmax([1000, 0])")[0], 1.0)   # no overflow

    def test_fmt_prints_as_a_figure_does(self):
        cases = {"fmt(0.973, 2)": "0.97", "fmt(1)": "1", "fmt(0.37)": "0.37",
                 "fmt(-2.5, 0)": "−3", "fmt(2.5, 0)": "3", "fmt(-0.001, 2)": "0.00",
                 "fmt(1e-7)": "0.0000001", "fmt(1 / 0)": "∞"}
        for source, want in cases.items():
            with self.subTest(source):
                self.assertEqual(value(source), want)

    def test_errors_name_what_went_wrong(self):
        cases = {"1 +": "the end", "a < b < c": "do not chain", "foo(1)": "no function",
                 "x[7]": "outside", "'a' + 1": "takes a number", "(1": "expected ')'",
                 "y": "not defined", "max()": "arguments", "1 # 2": "unexpected",
                 "fmt(1, 2.5)": "whole number"}
        for source, message in cases.items():
            with self.subTest(source), self.assertRaises(BindError) as caught:
                value(source)
            self.assertIn(message, str(caught.exception))

    def test_programs_and_attribute_lists(self):
        program = bind.parse_program("x = [1, 2]; s = 'a;b'; p = softmax(x, T);")
        self.assertEqual([name for name, _ in program], ["x", "s", "p"])
        env = bind.run(program, {"T": 1.0})
        self.assertEqual(env["s"], "a;b")
        attrs = bind.parse_attrs("width: max(1.5, p[1] * 290); stroke-width: 2")
        self.assertEqual([name for name, _ in attrs], ["width", "stroke-width"])
        with self.assertRaises(BindError):
            bind.parse_program("x == 1")
        self.assertEqual(bind.names(bind.parse("softmax(x, T)[i] + y")), {"x", "T", "i", "y"})


class BindingsTest(unittest.TestCase):
    def setUp(self):
        self.bind = Bindings(T=1)
        self.bind.let("x", X)
        self.bind.let("p", "softmax(x, T)")

    def test_attrs_set_the_value_at_the_defaults_and_keep_the_expression(self):
        out = self.bind.attrs('<rect x="1.0" width="0.0" stroke-width="1.4"/>',
                              width="max(1.5, p[4] * 290)", stroke_width="T > 0 ? 2 : 1")
        el = ET.fromstring(out)
        self.assertEqual(el.get("width"), "282.66")
        self.assertEqual(el.get("stroke-width"), "2")
        self.assertEqual(el.get("x"), "1.0")
        self.assertEqual(el.get("data-lk-attr"),
                         "width: max(1.5, p[4] * 290); stroke-width: T > 0 ? 2 : 1")

    def test_text_replaces_the_content_and_escapes_it(self):
        out = self.bind.text('<text x="1">old</text>', "'a < ' + fmt(p[4], 2)")
        el = ET.fromstring(out)
        self.assertEqual(el.text, "a < 0.97")
        self.assertEqual(el.get("data-lk-text"), "'a < ' + fmt(p[4], 2)")

    def test_declare_writes_the_inputs_and_the_derived_values(self):
        el = ET.fromstring(self.bind.declare())
        self.assertEqual(el.tag, "metadata")
        self.assertEqual(el.get("data-lk-vars"), "T = 1")
        self.assertEqual(el.get("data-lk-let"),
                         "x = [6, -5, 4, 1.5, 9.9, -2.3, 4]; p = softmax(x, T)")

    def test_a_name_is_defined_once(self):
        for name in ("p", "T", "max", "1x"):
            with self.subTest(name), self.assertRaises(BindError):
                self.bind.let(name, 1)

    def test_a_written_figure_checks_and_a_changed_one_does_not(self):
        body = (self.bind.declare()
                + self.bind.attrs('<rect width="0"/>', width="p[4] * 100")
                + self.bind.text("<text></text>", "fmt(p[4], 2)"))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp, "f.svg")
            path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg">{body}</svg>')
            self.assertTrue(bind.is_bound(path))
            self.assertEqual(bind.check_figure(path), {"T": 1.0})
            path.write_text(path.read_text().replace(">0.97<", ">0.98<"))
            with self.assertRaises(BindError) as caught:
                bind.check_figure(path)
            self.assertIn("<text>", str(caught.exception))
            path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg">'
                            f'{body.replace("p[4] * 100", "q[4] * 100")}</svg>')
            with self.assertRaises(BindError) as caught:
                bind.check_figure(path)
            self.assertIn("'q' is not defined", str(caught.exception))


NODE = shutil.which("node")


@unittest.skipUnless(NODE, "Node is not installed")
class ParityTest(unittest.TestCase):
    """bind.js gives what bind.py gives, to the last printed digit."""

    def js(self, cases: list[tuple[str, dict]]) -> list:
        script = """
            const b = require(process.argv[1]);
            const cases = JSON.parse(require('fs').readFileSync(0, 'utf8'));
            const out = cases.map(([source, env]) => {
              try {
                const v = b.evaluate(b.parse(source), env);
                return (typeof v === 'number' && !isFinite(v)) ? 'num:' + String(v) : v;
              } catch (e) { return 'error'; }
            });
            console.log(JSON.stringify(out));
        """
        done = subprocess.run([NODE, "-e", script, str(ASSETS_DIR / "bind.js")],
                              input=json.dumps(cases), capture_output=True, text=True,
                              check=True)
        return json.loads(done.stdout)

    def py(self, source: str, env: dict):
        try:
            v = bind.evaluate(bind.parse(source), env)
        except BindError:
            return "error"
        if isinstance(v, float) and not math.isfinite(v):
            return "num:" + ("NaN" if math.isnan(v) else ("Infinity" if v > 0 else "-Infinity"))
        return v

    def assertSame(self, cases):
        for (source, env), theirs in zip(cases, self.js(cases)):
            ours = self.py(source, env)
            with self.subTest(source=source, env=env):
                if isinstance(ours, float):
                    self.assertAlmostEqual(ours, theirs, delta=1e-12 * max(1, abs(ours)))
                elif isinstance(ours, list):
                    for a, b in zip(ours, theirs):
                        self.assertAlmostEqual(a, b, delta=1e-12)
                else:
                    self.assertEqual(ours, theirs)

    def test_the_language(self):
        sources = ["1 + 2 * 3 - 4 / 2", "-2^2", "2^3^2", "-7 % 3", "1 / 0", "0 / 0",
                   "T == 1 ? 'a' : 'b'", "max(x)", "min(1, 2, 3)", "[1, 2] * 3 + [1, 1]",
                   "'T = ' + fmt(T)", "len(x)", "sum(x)", "clamp(5, 0, 3)", "round(0.125, 2)",
                   "x[4] > 9 && !(T < 0)", "exp(1)", "log(10)", "sqrt(2)", "floor(-0.5)",
                   "abs([-1, 2])", "pow(2, 10)", "softmax(x, 0)", "softmax([2, 2, 0], 0)",
                   "x[7]", "foo(1)", "'a' + 1", "1 +", "a < b < c", "fmt(1, 2.5)"]
        self.assertSame([(source, ENV) for source in sources])

    def test_printed_numbers(self):
        rng = random.Random(7)
        numbers = [0, 1, -1, 0.5, 2.5, -2.5, 0.125, 1.005, 0.1 + 0.2, 1e-7, 123456.789, -0.001]
        numbers += [rng.uniform(-1000, 1000) for _ in range(200)]
        numbers += [rng.uniform(0, 1) for _ in range(200)]
        cases = [(f"fmt(v, {nd})", {"v": v}) for v in numbers for nd in range(4)]
        cases += [("fmt(v)", {"v": v}) for v in numbers]
        self.assertSame(cases)
        # What an attribute holds, through Bindings on one side and bind.js
        # on the other.
        script = """
            const b = require(process.argv[1]);
            const vs = JSON.parse(require('fs').readFileSync(0, 'utf8'));
            console.log(JSON.stringify(vs.map(v => b.attrValue(v, 'x'))));
        """
        done = subprocess.run([NODE, "-e", script, str(ASSETS_DIR / "bind.js")],
                              input=json.dumps(numbers), capture_output=True, text=True,
                              check=True)
        self.assertEqual(json.loads(done.stdout), [bind.attr_value(v, "x") for v in numbers])

    def test_the_temperature_figure_at_any_value(self):
        # What the viewer prints for a temperature typed in, against Python.
        temperatures = [0, 0.37, 1, 1.76, 7.25, 10] + [random.Random(3).uniform(0, 10)
                                                       for _ in range(50)]
        cases = [(f"fmt(softmax(x, T)[{i}], 2)", {"T": t, "x": X})
                 for t in temperatures for i in range(len(X))]
        cases += [("max(1.5, softmax(x, T)[4] * 290)", {"T": t, "x": X}) for t in temperatures]
        self.assertSame(cases)


if __name__ == "__main__":
    unittest.main()
