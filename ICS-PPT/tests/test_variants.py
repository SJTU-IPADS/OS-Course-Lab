"""A picture drawn again for a colour scheme: ``fig.dark.svg`` beside ``fig.svg``."""

import re
from pathlib import Path

import pytest
from PIL import Image

from lecturekit import cli, dev_server, model, tokens, variants
from lecturekit.renderers.viewer import colorscheme

DEFAULT = tokens.DEFAULT_SCHEME
OTHERS = tuple(name for name in tokens.schemes() if name != DEFAULT)
SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="{}" height="{}"/>'
BOUND = (
    '<svg xmlns="http://www.w3.org/2000/svg" width="200" height="100">'
    '<metadata data-lk-vars="T = 1" data-lk-let=""/>'
    '<rect width="50" height="9" data-lk-attr="width: T * 50"/></svg>'
)
PAGE = "lecture.page('p', body=lambda p: (p.title('P'), p.image('assets/{}')))\n"


def put(path: Path, text: str = "") -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text or SVG.format(200, 100), encoding="utf-8")
    return path


def lecture(directory: Path, body: str, id: str = "t") -> Path:
    source = (
        "from lecturekit.dsl import Lecture, review_section\n"
        f"lecture = Lecture(id='{id}', title='T')\n" + body
    )
    put(directory / "lecture.py", source)
    return directory


def render(source: Path, out: Path, scheme: str = "dark") -> str:
    args = ["render", str(source), "--out", str(out), "--no-build", "--color-scheme", scheme]
    assert cli.main(args) == 0
    return (out / colorscheme.HEAD_FILE).read_text(encoding="utf-8")


# --- which picture ---


@pytest.mark.parametrize("name", OTHERS)
def test_a_scheme_takes_its_own_picture_then_its_grounds_then_the_picture(tmp_path, name):
    scheme = tokens.scheme(name)
    picture = put(tmp_path / "fig.svg")
    assert variants.chosen(picture, scheme) is None
    shared = put(tmp_path / f"fig.{scheme.base}.svg")
    assert variants.chosen(picture, scheme) == shared
    own = put(tmp_path / f"fig.{name}.svg")
    assert variants.chosen(picture, scheme) == own


def test_a_second_dark_scheme_takes_the_dark_pictures(tmp_path):
    second = tokens.Scheme("second-dark", "dark", {})
    picture = put(tmp_path / "fig.svg")
    dark = put(tmp_path / "fig.dark.svg")
    assert variants.chosen(picture, second) == dark


def test_the_default_scheme_looks_for_no_suffix(tmp_path):
    picture = put(tmp_path / "fig.svg")
    put(tmp_path / f"fig.{DEFAULT}.svg")
    assert variants.chosen(picture, tokens.scheme(DEFAULT)) is None


def test_a_variant_has_the_pictures_extension(tmp_path):
    picture = put(tmp_path / "fig.svg")
    put(tmp_path / "fig.dark.png", "x")
    assert variants.chosen(picture, tokens.scheme("dark")) is None


# --- its shape ---


@pytest.mark.parametrize(
    "picture, variant",
    [
        (SVG.format(200, 100), SVG.format(200, 100)),
        (SVG.format(200, 100), SVG.format(400, 200)),
        (SVG.format(333, 100), SVG.format("332.6pt", "100pt")),
        (SVG.format(200, 100), '<svg viewBox="0 0 20 10"/>'),
    ],
)
def test_a_variant_of_the_same_shape_passes(tmp_path, picture, variant):
    put(tmp_path / "assets/fig.svg", picture)
    put(tmp_path / "assets/fig.dark.svg", variant)
    variants.check(tmp_path)


def test_another_shape_names_both_files_and_both_sizes(tmp_path):
    put(tmp_path / "assets/sub/fig.svg")
    put(tmp_path / "assets/sub/fig.dark.svg", SVG.format(200, 120))
    with pytest.raises(model.ValidationError) as caught:
        variants.check(tmp_path)
    said = str(caught.value)
    for part in ("assets/sub/fig.dark.svg", "200x120", "assets/sub/fig.svg", "200x100"):
        assert part in said


def test_a_raster_variant_is_measured(tmp_path):
    (tmp_path / "assets").mkdir()
    Image.new("RGB", (40, 20)).save(tmp_path / "assets/photo.png")
    Image.new("RGB", (40, 30)).save(tmp_path / "assets/photo.dark.png")
    with pytest.raises(model.ValidationError, match="40x30"):
        variants.check(tmp_path)


def test_an_unreadable_variant_is_named(tmp_path):
    put(tmp_path / "assets/fig.svg")
    put(tmp_path / "assets/fig.dark.svg", "<svg")
    with pytest.raises(model.ValidationError, match="fig.dark.svg"):
        variants.check(tmp_path)


def test_a_picture_alone_is_not_measured(tmp_path):
    put(tmp_path / "assets/fig.svg", "not a drawing")
    variants.check(tmp_path)


def test_a_lecture_is_checked_as_it_is_loaded(tmp_path):
    source = lecture(tmp_path, PAGE.format("fig.svg"))
    put(tmp_path / "assets/fig.svg")
    put(tmp_path / "assets/fig.dark.svg", SVG.format(100, 100))
    with pytest.raises(model.ValidationError, match="one canvas"):
        cli.load_lecture(source)


# --- what a page gets ---


def test_without_a_variant_the_head_is_the_schemes_alone(tmp_path):
    source = lecture(tmp_path / "in", PAGE.format("fig.svg"))
    put(source / "assets/fig.svg")
    assert render(source, tmp_path / "out") == colorscheme.head("dark")


def test_slides_md_and_the_pictures_know_nothing_of_a_variant(tmp_path):
    source = lecture(tmp_path / "in", PAGE.format("fig.svg"))
    put(source / "assets/fig.svg")
    render(source, tmp_path / "one")
    put(source / "assets/fig.dark.svg")
    render(source, tmp_path / "two")
    for name in ("slides.md", "outline.html", "assets/fig.svg"):
        assert (tmp_path / "one" / name).read_bytes() == (tmp_path / "two" / name).read_bytes()


def test_the_swap_is_the_schemes_and_the_box_the_pictures(tmp_path):
    source = lecture(tmp_path / "in", PAGE.format("fig.svg"))
    put(source / "assets/fig.svg")
    put(source / "assets/fig.dark.svg")
    html = render(source, tmp_path / "out", "auto")
    always = re.sub(r'<style[^>]*media="screen and.*?</style>', "", html, flags=re.S)
    assert 'img:is([src="assets/fig.svg"]' not in always
    assert 'img:is([src="assets/fig.svg"]' in html
    assert 'url("assets/fig.dark.svg")' in html
    assert "object-position: -99999px 0" in html
    # Fetched with the page, for a screen only.
    assert re.search(r'<style media="screen">\n:root::after \{ content: url\("assets/fig', always)


def test_the_default_scheme_has_no_swap(tmp_path):
    source = lecture(tmp_path / "in", PAGE.format("fig.svg"))
    put(source / "assets/fig.svg")
    put(source / "assets/fig.dark.svg")
    assert "fig.dark.svg" not in render(source, tmp_path / "out", DEFAULT)


@pytest.mark.parametrize(
    "name, written",
    [
        ("sp ace", '"assets/sp ace.svg"'),
        ("sp ace", '"assets/sp%20ace.svg"'),
        ("sp ace", '"./assets/sp%20ace.svg"'),
        ("图", '"assets/%E5%9B%BE.svg"'),
        ("it's", '"assets/it\'s.svg"'),
        ('say"so', '"assets/say\\22 so.svg"'),
        ("a<b", '"assets/a\\3c b.svg"'),
    ],
)
def test_each_spelling_of_a_picture_is_matched(tmp_path, name, written):
    put(tmp_path / f"assets/{name}.svg")
    put(tmp_path / f"assets/{name}.dark.svg")
    assert f"[src={written}]" in colorscheme.head("dark", tmp_path)


def test_a_background_picture_is_matched_by_its_style(tmp_path):
    put(tmp_path / "assets/bg.svg")
    put(tmp_path / "assets/bg.dark.svg")
    html = colorscheme.head("dark", tmp_path)
    assert '[style*="assets/bg.svg\\22 )"]' in html
    assert "background-image:" in html


def test_a_borrowed_pages_picture_has_the_sources_variant(tmp_path):
    other = lecture(tmp_path / "other", PAGE.format("fig.svg"), id="other")
    put(other / "assets/fig.svg")
    put(other / "assets/fig.dark.svg")
    host = lecture(tmp_path / "host", "review_section(lecture, 'R', {'../other': ['p']})\n")
    html = render(host, tmp_path / "out")
    assert '[src="assets/other/fig.svg"]' in html
    assert 'url("assets/other/fig.dark.svg")' in html
    put(other / "assets/fig.dark.svg", SVG.format(10, 10))
    with pytest.raises(model.ValidationError, match="one canvas"):
        cli.load_lecture(host)


# --- a bound figure ---


@pytest.fixture
def bound(tmp_path):
    body = "lecture.variable('T', 1, min=0, max=3)\n" + PAGE.format("fig.svg")
    source = lecture(tmp_path / "in", body)
    put(source / "assets/fig.svg", BOUND)
    return source


def test_a_bound_figures_variant_moves_with_it(tmp_path, bound):
    put(bound / "assets/fig.dark.svg", BOUND)
    html = render(bound, tmp_path / "out")
    script = re.search(
        r'<script type="image/svg\+xml" data-lk-figure="assets/fig.dark.svg" '
        r'data-lk-paint="(--lk-drawn-\w+)">',
        html,
    )
    assert script
    assert f'var({script.group(1)}, url("assets/fig.dark.svg"))' in html
    assert '[data-lk-figure="assets/fig.svg"]' in html


@pytest.mark.parametrize(
    "variant, said",
    [
        (SVG.format(200, 100), "bind it to the same variables"),
        (BOUND.replace("T = 1", "T = 2"), "bind it to the same variables"),
        (BOUND.replace('width="50"', 'width="60"'), "fig.dark.svg"),
    ],
)
def test_a_bound_figures_variant_is_bound_alike(bound, variant, said):
    put(bound / "assets/fig.dark.svg", variant)
    with pytest.raises(model.ValidationError, match=said):
        cli.load_lecture(bound)


def test_a_variant_that_fails_a_render_leaves_the_watch_running(tmp_path, bound):
    out = tmp_path / "out"
    variant = put(bound / "assets/fig.dark.svg", BOUND.replace("</svg>", "<!-- </script> --></svg>"))
    changed = [(dev_server.Change.modified, str(variant))]
    broadcaster = dev_server.ReloadBroadcaster()
    dev_server.handle_changes(changed, bound, out, broadcaster, color_scheme="dark")
    assert "cannot contain '</script'" in (out / "slides.md").read_text(encoding="utf-8")
