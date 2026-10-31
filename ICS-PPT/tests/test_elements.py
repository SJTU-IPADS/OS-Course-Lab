"""The element classes: one per block kind, each drawing itself in every target."""

import pytest

from lecturekit import Lecture, model
from lecturekit.dsl import PageBuilder
from lecturekit.model import Block, ValidationError, element_class, kinds_drawn_by
from lecturekit.renderers.viewer import render_marp_page


def _page(body):
    lecture = Lecture(id="el", title="Elements")
    lecture.page("page", body=body)
    return lecture.build().children[0]


def _kinds():
    element_class(None)
    return sorted(model._ELEMENTS)


def test_block_builds_the_class_of_its_kind():
    block = Block(kind="table", content={"headers": ["a"], "rows": [["1"]], "align": None})
    assert type(block) is element_class("table")
    assert type(block).__name__ == "Table"
    assert isinstance(block, Block)


def test_a_kind_nobody_registered_stays_a_plain_block_and_is_refused():
    block = Block(kind="no-such-kind", content="x")
    assert type(block) is Block
    with pytest.raises(ValidationError, match="Unknown block kind"):
        model.check_block(block, "page", set(), None)


def test_every_kind_is_registered_once_under_its_own_name():
    for kind in _kinds():
        assert element_class(kind).KIND == kind


@pytest.mark.parametrize("kind", _kinds())
def test_a_class_implements_the_method_of_each_target_that_draws_it(kind):
    cls = element_class(kind)
    if kind in ("flex", "end"):
        return  # layout markers: the layout walker draws the container
    methods = {"viewer": "to_marp", "latex": "to_latex", "transcript": "to_transcript"}
    for target, name in methods.items():
        if target in cls.TARGETS:
            assert getattr(cls, name) is not getattr(Block, name), (kind, target)
    # The image column is drawn once per page by the pptx renderer.
    if "pptx" in cls.TARGETS and kind != "side_image":
        assert cls.to_pptx is not Block.to_pptx, kind


def test_the_targets_of_a_kind_come_from_its_class():
    assert "prose" in kinds_drawn_by("latex")
    assert "prose" not in kinds_drawn_by("viewer")
    assert "slide" in kinds_drawn_by("viewer")
    assert "slide" not in kinds_drawn_by("latex")
    assert kinds_drawn_by("transcript") >= {"slide", "code", "image", "table"}


def test_every_element_carries_footnotes_and_alignment():
    def body(p):
        p.title("T")
        p.slide("a").footnote("n1")
        p.image("a.png").footnote("n2").halign("left")
        p.code("c", "x").footnote("n3")
        p.highlight("h").footnote("n4")
        p.spacer(8)
        with p.flex("row"):
            p.slide("b")
    page = _page(body)
    for block in page.blocks:
        assert hasattr(block, "footnotes") and hasattr(block, "halign")
    assert [b.footnotes for b in page.blocks[:4]] == [("n1",), ("n2",), ("n3",), ("n4",)]
    assert page.blocks[1].halign == "left"


def test_a_flex_container_takes_a_footnote():
    def body(p):
        p.title("T")
        with p.flex("row").footnote("shared source"):
            p.slide("left")
            p.slide("right")
    page = _page(body)
    root = model.layout(page.blocks)
    assert root[0].footnotes == ("shared source",)


def test_a_flex_footnote_marks_the_last_leaf_in_the_viewer():
    def body(p):
        p.title("T")
        with p.flex("row").footnote("shared source"):
            p.slide("left text")
            p.slide("right text")
    md = render_marp_page(_page(body))
    assert "shared source" in md
    # One superscript on the leaf, one in front of the note at the slide's foot.
    assert md.count("<sup") == 2
    assert md.index("right text") < md.index("<sup") < md.index('class="footnotes"')


@pytest.mark.parametrize("target", ["latex", "transcript"])
def test_a_flex_footnote_goes_to_the_last_leaf_that_remains(target):
    def body(p):
        p.title("T")
        with p.flex("column").footnote("shared source"):
            p.code("c", "first")
            p.code("c", "second")
    selected = model.select_blocks(_page(body), target)
    assert [b.footnotes for b in selected] == [(), ("shared source",)]


@pytest.mark.parametrize("side", ["right", "left"])
def test_a_side_image_footnote_draws_its_marker_beside_the_picture(side):
    def body(p):
        p.title("T")
        p.side_image("image.png", side=side).footnote("image source")
        p.slide("text")
    md = render_marp_page(_page(body))
    assert "image source" in md
    assert "footnote-ref--side" in md
    # One superscript, inside the column that holds the picture.
    column = md[md.index('<figure class="lk-side"'):md.index("</figure>")]
    assert column.count("<sup") == 1


def test_a_side_image_footnote_and_a_text_footnote_are_numbered_in_order():
    def body(p):
        p.title("T")
        p.side_image("image.png").footnote("picture")
        p.slide("text").footnote("claim")
    md = render_marp_page(_page(body))
    assert md.index(">1</sup>") < md.index(">2</sup>")


def test_the_handle_of_a_side_image_still_has_no_alignment():
    p = PageBuilder()
    with pytest.raises(ValidationError, match="page's alignment"):
        p.side_image("image.png").halign("left")
