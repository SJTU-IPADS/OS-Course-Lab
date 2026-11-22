"""A `$…$` is a formula in every target exactly when it is one in the deck.

The deck's markdown engine (Marp's math rule) decides. PPTX and the transcript
are read by that engine; `lecturekit.inlinemath` states the rule once for the
two grammars that are not (the book's, a chip's). The table below is checked
against Marp itself, then against each target.
"""

import json
import re
import shutil
import subprocess

import pytest

from lecturekit.renderers.latex import text as latex_text
from lecturekit.markup import Reader, document
from lecturekit.renderers.pptx import text as pptx_text
from lecturekit.renderers.transcript import text as transcript_text
from lecturekit.renderers.viewer import blocks as viewer_blocks
from lecturekit.renderers.viewer import marp

# text -> the formulas in it, in order
CASES = {
    "约为 $55 和 $4，相差": [],
    "价格 $8 到 $10 之间": [],
    "$8，$10": [],
    "$x$5": [],
    "$ x$": [],
    "$x $": [],
    "$x$，好": ["x"],
    "a $b$ c": ["b"],
    "$a $b$": ["b"],
    "A$x$B": ["x"],
    "$a$$b$": ["a", "b"],
    "`movl $0, %eax` 与 `addl $1, %eax`": [],
}


def _formulas(regex, text, group="math"):
    return [m.group(group) for m in regex.finditer(text) if m.group(group) is not None]


@pytest.fixture(scope="module")
def line():
    """A line of slide text, read."""
    with Reader() as read:
        yield lambda text: document.phrasing(read.slide(text))


def _sheet(inlines):
    return transcript_text.HtmlWriter(lambda src: None).inline(inlines)


@pytest.mark.parametrize("text", CASES)
def test_pptx_reads_math_as_the_deck_does(text, line):
    runs = pptx_text.runs(line(text))
    assert [run.text for run in runs if run.math] == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_the_transcript_reads_math_as_the_deck_does(text, line):
    written = re.findall(r"<mi>(.*?)</mi>", _sheet(line(text)))
    assert written == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_the_book_reads_math_as_the_deck_does(text):
    assert _formulas(latex_text._PROTECT_RE, text) == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_a_highlight_chip_reads_math_as_the_deck_does(text):
    assert _formulas(viewer_blocks.CHIP_MATH_RE, text) == CASES[text]


def test_dollar_amounts_stay_text(line):
    text = "两种情况输入的费用约为 $55 和 $4"
    assert pptx_text.plain(pptx_text.runs(line(text))) == text
    assert latex_text.inline(text) == r"两种情况输入的费用约为 \$55 和 \$4"
    assert _sheet(line(text)) == text


_MARP_COUNT = """
const { Marp } = require(process.argv[1]);
const marp = new Marp({ math: 'mathjax' });
const out = {};
for (const text of JSON.parse(process.argv[2])) {
  const { html } = marp.render(text);
  out[text] = (html.match(/<mjx-container/g) || []).length;
}
console.log(JSON.stringify(out));
"""


def test_the_table_is_what_marp_renders():
    core = marp.PKG_ROOT / "node_modules" / "@marp-team" / "marp-core"
    if shutil.which("node") is None or not core.is_dir():
        pytest.skip("needs node and the vendored marp-core (scripts/prepare.sh)")
    found = subprocess.run(
        ["node", "-e", _MARP_COUNT, str(core), json.dumps(list(CASES))],
        capture_output=True, text=True, check=True, stdin=subprocess.DEVNULL,
    ).stdout
    assert json.loads(found) == {text: len(formulas) for text, formulas in CASES.items()}
