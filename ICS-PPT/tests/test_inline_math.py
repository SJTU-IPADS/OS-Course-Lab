"""A `$…$` is a formula in every target exactly when it is one in the deck.

The deck's markdown engine (Marp's math rule) decides; `lecturekit.inlinemath`
states that rule once and every renderer's inline grammar uses it. The table
below is checked against Marp itself, then against each target.
"""

import json
import shutil
import subprocess

import pytest

from lecturekit.renderers.latex import text as latex_text
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


@pytest.mark.parametrize("text", CASES)
def test_pptx_reads_math_as_the_deck_does(text):
    runs = pptx_text.parse_inline(text)
    assert [run.math for run in runs if run.math] == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_the_transcript_reads_math_as_the_deck_does(text):
    assert _formulas(transcript_text._INLINE_RE, text) == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_the_book_reads_math_as_the_deck_does(text):
    assert _formulas(latex_text._PROTECT_RE, text) == CASES[text]


@pytest.mark.parametrize("text", CASES)
def test_a_highlight_chip_reads_math_as_the_deck_does(text):
    assert _formulas(viewer_blocks.CHIP_MATH_RE, text) == CASES[text]


def test_dollar_amounts_stay_text():
    line = "两种情况输入的费用约为 $55 和 $4"
    assert "".join(run.text for run in pptx_text.parse_inline(line)) == line
    assert latex_text.inline(line) == r"两种情况输入的费用约为 \$55 和 \$4"
    assert transcript_text.inline(line) == line


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
