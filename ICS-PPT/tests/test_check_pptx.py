"""The PPTX says what the web deck says, and scripts/check-pptx.py sees when it does not."""

import shutil
import subprocess
import sys
from pathlib import Path

import pytest
from pptx import Presentation
from pptx.oxml.ns import qn

from lecturekit import cli, model, rasterize
from lecturekit.renderers.pptx import PptxRenderer, omml
from lecturekit.renderers.viewer import build_marp_markdown, marp

ROOT = Path(__file__).resolve().parents[1]
CHECK = ROOT / "scripts" / "check-pptx.py"
EVERY_BLOCK = Path(__file__).parent / "fixtures" / "allblocks"
SHOWCASE = ROOT / "examples" / "showcase"
VENDORED = marp.PKG_ROOT / "node_modules" / "@marp-team" / "marp-cli" / "marp-cli.js"

pytestmark = pytest.mark.skipif(
    shutil.which("node") is None or not VENDORED.is_file(),
    reason="needs node and the vendored marp-cli (scripts/prepare.sh)",
)


def export(source: Path, dest: Path) -> tuple[Path, Path]:
    """A lecture as slides.html and as a .pptx."""
    lecture = cli.load_lecture(source)
    web = dest / "web"
    web.mkdir()
    (web / "slides.md").write_text(build_marp_markdown(lecture, asset_root=source),
                                   encoding="utf-8")
    marp.build_deck(web, ("html",))
    deck = PptxRenderer(asset_root=source).render(lecture, dest / "pptx")
    return web / "slides.html", deck


def check(*arguments) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(CHECK), *map(str, arguments)],
                          capture_output=True, text=True)


@pytest.fixture(scope="module")
def exported(tmp_path_factory):
    return export(EVERY_BLOCK, tmp_path_factory.mktemp("allblocks"))


def test_the_fixture_has_every_block_the_pptx_draws():
    used = {block.kind for page in model.flatten_pages(cli.load_lecture(EVERY_BLOCK).children)
            for block in page.blocks}
    assert model.kinds_drawn_by("pptx") <= used


def test_every_block_says_what_the_deck_says(exported):
    found = check(*exported)
    assert (found.returncode, found.stdout, found.stderr) == (0, "", "")


@pytest.mark.skipif(rasterize.find_backend() is None, reason="needs an SVG rasterizer")
def test_the_showcase_says_what_the_deck_says(tmp_path):
    found = check(*export(SHOWCASE, tmp_path))
    assert (found.returncode, found.stdout, found.stderr) == (0, "", "")


# --- what the check must see ---


def first(deck, tag):
    return next(node for slide in deck.slides for node in slide._element.iter(tag))


def drop_a_picture(deck):
    picture = first(deck, qn("p:pic"))
    picture.getparent().remove(picture)


def change_a_symbol(deck):
    first(deck, f"{{{omml.M}}}t").text = "Q"


def lose_a_bullet(deck):
    next(t for slide in deck.slides for t in slide._element.iter(qn("a:t"))
         if t.text and t.text.startswith("•")).text = " "


def change_a_word(deck):
    next(t for slide in deck.slides for t in slide._element.iter(qn("a:t"))
         if t.text == "nested, with a ").text = "nested with a "


def change_a_cell(deck):
    next(shape for slide in deck.slides for shape in slide.shapes
         if shape.has_table).table.cell(0, 0).text = "changed"


def indent_a_listing(deck):
    next(t for slide in deck.slides for t in slide._element.iter(qn("a:t"))
         if t.text and t.text.startswith("return x")).text = "  return x;"


def swap_two_blocks(deck):
    texts = [shape for shape in deck.slides[2].shapes
             if shape.has_text_frame and shape.text_frame.text.strip()]
    texts[-1]._element.addnext(texts[0]._element)


def drop_a_slide(deck):
    slides = deck.slides._sldIdLst
    slides.remove(slides[-1])


@pytest.mark.parametrize("damage", [
    drop_a_picture, change_a_symbol, lose_a_bullet, change_a_word,
    change_a_cell, indent_a_listing, swap_two_blocks, drop_a_slide,
])
def test_the_check_names_a_difference(damage, exported, tmp_path):
    html, pptx = exported
    deck = Presentation(str(pptx))
    damage(deck)
    deck.save(str(tmp_path / "damaged.pptx"))
    found = check(html, tmp_path / "damaged.pptx")
    assert found.returncode == 1
    assert "damaged.pptx" in found.stdout


def test_bad_usage_is_not_a_difference(tmp_path):
    assert check().returncode == 2
    found = check(tmp_path / "no.html", tmp_path / "no.pptx")
    assert (found.returncode, found.stdout) == (2, "")
