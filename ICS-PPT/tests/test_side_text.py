"""Slide text set under a side image: `p.slide(...).under_side_image()`.

The image column, on either side, then holds the picture and the text below
it; the other blocks keep the main column.
"""

import base64
import tempfile
from pathlib import Path

import pytest
from pptx import Presentation
from pptx.enum.shapes import MSO_SHAPE_TYPE

from lecturekit import Lecture, model, serialize
from lecturekit.renderers.pptx import PptxRenderer
from lecturekit.renderers.pptx.layout import Layout, px
from lecturekit.renderers.viewer import render_marp_page


_PNG = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+M9Q"
    "DwADhgGAWjR9awAAAABJRU5ErkJggg=="
)


def _lecture(side="left", *, image=True):
    lecture = Lecture(id="lec", title="T")

    def body(p):
        p.title("Title")
        if image:
            p.side_image("assets/pic.png", width="40%", alt="contain", side=side)
        p.slide("- under the picture").under_side_image()
        p.code("c", "int beside;")
    lecture.page("pg", body=body)
    return lecture


def test_the_handle_marks_the_slide_block():
    page = _lecture().build().children[0]
    marked = [block.kind for block in page.blocks if block.in_side]
    assert marked == ["slide"]
    blocks = serialize.lecture_to_dict(_lecture().build())["children"][0]["blocks"]
    assert [block["in_side"] for block in blocks] == [False, True, False]


def test_only_a_slide_block_may_be_set_under_the_image():
    lecture = Lecture(id="lec", title="T")

    def body(p):
        p.title("Title")
        p.side_image("assets/pic.png", side="left")
        p.code("c", "int x;").under_side_image()
    lecture.page("pg", body=body)
    with pytest.raises(model.ValidationError, match="slide block"):
        lecture.build()


def test_it_needs_a_side_image():
    with pytest.raises(model.ValidationError, match="side_image"):
        _lecture(image=False).build()
    _lecture("right").build()                   # either side holds the text


def test_the_viewer_draws_the_text_in_the_image_column():
    md = render_marp_page(_lecture().build().children[0], reveal=True)
    lines = md.split("\n")
    start = lines.index('<div class="lk-side" style="--lk-side-width: 448px;">')
    end = lines.index("</div>", start)
    # the picture first, as wide as the column
    assert lines[start + 2] == (
        '<figure class="lk-side-img lk-side-fit"><img src="assets/pic.png" alt=""></figure>'
    )
    # the text is markdown of its own, between blank lines, inside the column
    assert lines[start + 3] == ""
    assert "- under the picture" in lines[start + 4:end]
    assert md.count("under the picture") == 1
    # the column is not a reveal step; the code block beside it is the only one
    assert md.count('class="reveal-block"') == 1
    # the image column is on the left: it comes before the main column
    assert md.index("</div>", md.index('<div class="lk-side"')) < md.index("int beside;")


def test_a_column_without_text_holds_only_its_picture():
    lecture = Lecture(id="lec", title="T")

    def body(p):
        p.title("Title")
        p.side_image("assets/pic.png", width="40%", alt="contain", side="left")
        p.slide("beside the picture")
    lecture.page("pg", body=body)
    md = render_marp_page(lecture.build().children[0])
    column = md[md.index('<div class="lk-side"'):]
    column = column[:column.index("</div>")]
    assert column.count("<figure") == 1
    assert "beside the picture" not in column


def test_powerpoint_stacks_the_picture_and_the_text_in_the_column():
    src = Path(tempfile.mkdtemp())
    (src / "assets").mkdir()
    (src / "assets" / "pic.png").write_bytes(_PNG)
    out = PptxRenderer(asset_root=src).render(_lecture().build(), Path(tempfile.mkdtemp()))
    slide = Presentation(str(out)).slides[0]
    layout = Layout.from_ratio("16:9")
    column = px(model.side_column_px("40%", 1120))
    pic = next(s for s in slide.shapes if s.shape_type == MSO_SHAPE_TYPE.PICTURE)
    under = next(s for s in slide.shapes if s.has_text_frame
                 and s.text_frame.text.endswith("under the picture"))
    beside = next(s for s in slide.shapes if s.has_text_frame
                  and "int beside;" in s.text_frame.text)
    # the picture is as wide as the column, its share of the body
    assert pic.left == layout.content_left
    assert pic.width == column
    # the text is in the same column, below the picture
    assert under.left == pic.left
    assert under.top >= pic.top + pic.height
    assert under.left + under.width <= pic.left + pic.width
    # the other block takes the main column, 40px on, and starts under the title
    assert beside.left == layout.content_left + column + px(40)
    assert beside.top < under.top


def test_the_viewer_stacks_a_right_column_the_same_way():
    md = render_marp_page(_lecture("right").build().children[0])
    side = md.index('<div class="lk-side" style="--lk-side-width: 448px;">')
    assert md.count("under the picture") == 1
    # the main column comes first, the image column, with the text, after it
    assert md.index("int beside;") < side < md.index("under the picture")


def test_powerpoint_stacks_a_right_column_against_the_right_margin():
    src = Path(tempfile.mkdtemp())
    (src / "assets").mkdir()
    (src / "assets" / "pic.png").write_bytes(_PNG)
    out = PptxRenderer(asset_root=src).render(
        _lecture("right").build(), Path(tempfile.mkdtemp()))
    slide = Presentation(str(out)).slides[0]
    layout = Layout.from_ratio("16:9")
    column = px(model.side_column_px("40%", 1120))
    pic = next(s for s in slide.shapes if s.shape_type == MSO_SHAPE_TYPE.PICTURE)
    under = next(s for s in slide.shapes if s.has_text_frame
                 and s.text_frame.text.endswith("under the picture"))
    beside = next(s for s in slide.shapes if s.has_text_frame
                  and "int beside;" in s.text_frame.text)
    # the mirror of the left column: the outer edge at the side padding, the
    # main column 40px short of it
    assert pic.left + pic.width == layout.width - layout.content_left
    assert pic.left == layout.width - layout.content_left - column
    assert under.left == pic.left
    assert under.top >= pic.top + pic.height
    assert beside.left == layout.content_left
    assert beside.left + beside.width <= pic.left - px(40)
