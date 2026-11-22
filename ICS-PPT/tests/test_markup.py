"""The one parse of slide text: the deck's engine, and the document read from its HTML."""

import subprocess
import sys
from unittest import mock

import pytest

from lecturekit.markup import Reader, engine
from lecturekit.markup.document import (
    Code, Columns, Formula, Image, List, Paragraph, Span, Table, Text, phrasing,
)


@pytest.fixture(scope="module")
def read():
    with Reader() as reader:
        yield reader


def test_markdown_and_html_are_one_document(read):
    written = read.markdown("a **b** `c` [d](https://x.test)")
    tagged = read.html('<p>a <b>b</b> <code>c</code> <a href="https://x.test">d</a></p>')
    assert written == tagged


def test_a_formula_is_its_mathml(read):
    [formula] = phrasing(read.markdown(r"$\hat{w}$"))
    assert isinstance(formula, Formula) and not formula.display
    assert formula.math.tag == "math" and formula.math.find(".//mover") is not None


def test_a_display_formula_is_marked(read):
    [formula] = phrasing(read.markdown("$$\nx\n$$"))
    assert formula.display


def test_dashes_on_the_first_line_are_not_front_matter(read):
    [items] = read.markdown("- `--watch` reloads")
    assert items == List([[Paragraph([Span("code", [Text("--watch")]), Text(" reloads")])]])


def test_the_kinds_of_block(read):
    blocks = read.markdown("```c\nint x;\n```\n\n| a |\n|---|\n| b |")
    assert [type(block) for block in blocks] == [Code, Table]


def test_a_picture_shown_as_a_block_is_kept(read):
    assert read.html('<img src="a.png" style="display:block">') == [
        Paragraph([Image("a.png")])]


def test_a_grid_is_columns_row_by_row(read):
    html = ('<div style="display:grid;grid-template-columns:repeat(2, 1fr)">'
            "<div>a</div><div>b</div><div>c</div><div>d</div></div>")

    def cell(text):
        return [Paragraph([Text(text)])]

    assert read.html(html) == [Columns([cell("a"), cell("b")]), Columns([cell("c"), cell("d")])]


def test_what_is_not_shown_is_not_content(read):
    html = ('shown<script>x()</script><style>p{}</style><span hidden>a</span>'
            '<span style="display: none">b</span>')
    assert read.html(html) == [Paragraph([Text("shown")])]


def test_an_unknown_tag_keeps_its_text(read):
    inlines = phrasing(read.html("a <blink>b</blink> c"))
    assert "".join(node.text for node in inlines) == "a b c"


def test_each_field_is_read_as_the_deck_reads_it(read):
    assert read.plain("*as written* $x$") == [Text("*as written* $x$")]
    assert phrasing(read.markdown("*x*")) == [Span("em", [Text("x")])]
    # A caption is raw HTML in the deck: emphasis is read, a dollar is a dollar.
    assert read.inline("*x* $5") == [Span("em", [Text("x")]), Text(" $5")]


def test_the_engine_is_one_process_and_ends_with_the_reader():
    started = []
    popen = subprocess.Popen

    def record(*args, **options):
        started.append(popen(*args, **options))
        return started[-1]

    with mock.patch.object(engine.subprocess, "Popen", side_effect=record):
        with Reader() as reader:
            assert not started          # nothing is started before it is needed
            for word in ("one", "two", "three"):
                assert reader.markdown(word) == [Paragraph([Text(word)])]
            assert len(started) == 1
            assert started[0].poll() is None
    assert started[0].poll() == 0


def test_without_node_the_message_says_what_to_run():
    with mock.patch.object(engine.shutil, "which", return_value=None):
        with Reader() as reader, pytest.raises(RuntimeError, match="scripts/prepare.sh"):
            reader.markdown("text")


def test_markup_needs_no_target():
    program = (
        "import sys\n"
        "from lecturekit import markup\n"
        "assert not any(m.startswith(('pptx', 'lxml', 'lecturekit.renderers')) "
        "for m in sys.modules), sorted(sys.modules)\n"
    )
    subprocess.run([sys.executable, "-c", program], check=True,
                   cwd=engine.FILTER.parents[2])
