"""A content page is three bands: the title, the body and the footnote band.

The body is the main column, and with a side image the image column beside it;
the footnote band spans the content width at the bottom of the slide, at least
56px tall, and grows with its notes. The markup and the theme rules are checked
here, and the geometry once in a browser: one Marp run, one headless Chrome.
"""

import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

import pytest

from lecturekit import Lecture, model, tokens
from lecturekit.renderers.viewer import build_marp_markdown, marp, render_marp_page
from lecturekit.renderers.viewer.pdf import find_chrome, print_command

THEME = (Path(__file__).resolve().parents[1] / "themes" / "basic-office.css").read_text(
    encoding="utf-8"
)

_ROWS = [["a", "1"], ["b", "2"], ["c", "3"]]


def _plain(p):
    p.title("Plain")
    p.slide("- one\n- two").footnote("a source")


def _three(p):
    p.title("Three notes")
    p.slide("text").footnote("one").footnote("two").footnote("three")


def _five(p):
    p.title("Five notes")
    p.slide("text").footnote("a").footnote("b").footnote("c").footnote("d").footnote("e")


def _side(p):
    p.title("Side, right")
    p.side_image("pic.svg", width="40%", alt="contain").footnote("picture source")
    p.slide("- beside the picture").footnote("text source")


def _middle(p):
    p.title("Side, left, middle")
    p.side_image("pic.svg", width="44%", alt="contain", side="left", valign="middle")
    p.slide("the byte layout").under_side_image()
    p.table(_ROWS, headers=["field", "bits"])


def _cover_left(p):
    p.title("Side, left, covering")
    p.side_image("pic.svg", side="left")
    p.slide("- beside the picture")


PAGES = {"plain": _plain, "three": _three, "five": _five, "side": _side,
         "middle": _middle, "cover-left": _cover_left}


def _lecture():
    lecture = Lecture(id="bands", title="Bands")
    lecture.cover("Bands", author="a")
    for name, body in PAGES.items():
        lecture.page(name, body=body)
    lecture.bridge("A bridge")
    return lecture.build()


def _page(name):
    return next(page for page in model.flatten_pages(_lecture().children)
                if page.id == name)


# --- the markup ---

def test_every_content_page_is_title_body_and_band():
    md = render_marp_page(_page("plain"))
    title = md.index("# Plain")
    body = md.index('<div class="lk-body">\n<div class="lk-main">')
    band = md.index('<div class="lk-footnote-band"><aside class="footnotes">')
    assert title < body < md.index("- one") < band
    # the band follows the body's closing tags, outside it
    assert md[:band].rstrip().endswith("</div>\n\n</div>")


def test_the_cover_and_a_bridge_have_no_bands():
    pages = model.flatten_pages(_lecture().children)
    for page in (pages[0], pages[-1]):
        md = render_marp_page(page)
        assert "lk-body" not in md and "lk-footnote-band" not in md


def test_the_band_spans_the_body_on_a_side_page():
    md = render_marp_page(_page("side"))
    band = md.index('<div class="lk-footnote-band">')
    # both columns close before the band; the band lists the picture's note
    # first, then the text's
    assert md.index('<div class="lk-side"') < md.index("</div>", md.index('<div class="lk-side"')) < band
    assert md.index("</sup> picture source") < md.index("</sup> text source")
    assert "footnotes" not in md[:band]


def test_the_theme_has_no_page_level_overlay_but_callouts_and_markers():
    rule = THEME.split("\n.footnotes {", 1)[1].split("}", 1)[0]
    assert "position" not in rule
    band = THEME.split("\n.lk-footnote-band {", 1)[1].split("}", 1)[0]
    assert "min-height: var(--footnote-band-min);" in band
    assert "justify-content: flex-end;" in band
    body = THEME.split("\n.lk-body {", 1)[1].split("}", 1)[0]
    assert "flex: 1 1 0;" in body and "min-height: 0;" in body
    assert "gap: var(--side-gap);" in body
    assert ("padding: var(--slide-padding-top) var(--slide-padding-x) "
            "var(--slide-padding-bottom);") in THEME
    for gone in ("lk-side-left", "lk-side-right", "lk-valign", "anchor(",
                 "footnote-ref--side", "lk-side-stack", "lk-gap-flow"):
        assert gone not in THEME, gone


# --- the geometry, in a browser ---

_MEASURE = """<script>
document.fonts.ready.then(function () { requestAnimationFrame(function () {
  var out = {};
  document.querySelectorAll('section').forEach(function (s) {
    var id = s.getAttribute('id');
    var r = s.getBoundingClientRect(), k = 1280 / r.width;
    function box(el) {
      if (!el) return null;
      var b = el.getBoundingClientRect();
      return [(b.left - r.left) * k, (b.top - r.top) * k,
              (b.right - r.left) * k, (b.bottom - r.top) * k];
    }
    // The extent of a column's blocks, their margins included.
    function content(el) {
      if (!el) return null;
      var top = 1e9, bottom = -1e9;
      for (var c of el.children) {
        var b = box(c), m = getComputedStyle(c);
        // A computed margin is in the slide's own px, as `box` returns.
        top = Math.min(top, b[1] - parseFloat(m.marginTop));
        bottom = Math.max(bottom, b[3] + parseFloat(m.marginBottom));
      }
      return [top, bottom];
    }
    var side = s.querySelector('.lk-side'), main = s.querySelector('.lk-main');
    out[id] = {body: box(s.querySelector('.lk-body')),
               band: box(s.querySelector('.lk-footnote-band')),
               main: box(main), side: box(side),
               mainContent: content(main), sideContent: content(side),
               sideImg: box(side && side.querySelector('img'))};
  });
  var pre = document.createElement('pre');
  pre.id = 'lk-geometry';
  pre.textContent = JSON.stringify(out);
  document.body.appendChild(pre);
}); });
</script>"""

_PIC = ('<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">'
        '<rect width="300" height="200" fill="#888"/></svg>')


def _browser_missing() -> str | None:
    if shutil.which("node") is None:
        return "needs node"
    vendored = marp.PKG_ROOT / "node_modules" / "@marp-team" / "marp-cli" / "marp-cli.js"
    if not (os.environ.get("LECTUREKIT_MARP") or vendored.exists() or shutil.which("marp")):
        return "needs marp-cli (scripts/prepare.sh)"
    try:
        find_chrome()
    except RuntimeError:
        return "needs Chrome"
    return None


@pytest.fixture(scope="module")
def geometry():
    missing = _browser_missing()
    if missing:
        pytest.skip(missing)
    # A short directory: Chrome puts its sockets under TMPDIR.
    work = Path(tempfile.mkdtemp(prefix="lkb.", dir="/tmp"))
    try:
        (work / "pic.svg").write_text(_PIC, encoding="utf-8")
        (work / "slides.md").write_text(build_marp_markdown(_lecture()), encoding="utf-8")
        marp._copy_theme(work, marp.THEME_DIR)
        subprocess.run(
            marp.marp_command() + ["slides.md", "-o", "slides.html", "--html",
                                   "--template", "bare", "--theme", "theme.css"]
            + marp.ENGINE_ARGS,
            cwd=work, check=True, stdin=subprocess.DEVNULL, capture_output=True,
        )
        html = (work / "slides.html").read_text(encoding="utf-8")
        (work / "slides.html").write_text(html.replace("</body>", _MEASURE + "</body>"),
                                          encoding="utf-8")
        dom = subprocess.run(
            print_command() + [f"--user-data-dir={work / 'profile'}",
                               "--virtual-time-budget=5000", "--dump-dom",
                               (work / "slides.html").as_uri()],
            capture_output=True, text=True, check=True, stdin=subprocess.DEVNULL,
            env=dict(os.environ, TMPDIR=str(work)), timeout=120,
        ).stdout
    finally:
        shutil.rmtree(work, ignore_errors=True)
    found = re.search(r'<pre id="lk-geometry">(.*?)</pre>', dom, re.S)
    assert found, "the measuring script did not run"
    by_section = json.loads(found.group(1).replace("&quot;", '"').replace("&amp;", "&"))
    # Section ids are Marp's 1-based slide numbers; the cover is 1.
    names = ["cover", *PAGES, "bridge"]
    return {names[int(key) - 1]: value for key, value in by_section.items()}


def _near(a, b, tolerance=0.5):
    return abs(a - b) <= tolerance


CONTENT_LEFT = tokens.px("--slide-padding-x")
CONTENT_RIGHT = 1280 - tokens.px("--slide-padding-x")
BODY_BOTTOM = 720 - tokens.px("--slide-padding-bottom") - tokens.px("--footnote-band-min")


def test_the_body_ends_where_the_band_begins(geometry):
    for name in PAGES:
        page = geometry[name]
        assert _near(page["body"][3], page["band"][1]), name
        assert _near(page["band"][3], 720 - tokens.px("--slide-padding-bottom")), name


def test_up_to_three_footnote_lines_leave_the_body_at_650(geometry):
    for name in ("plain", "three", "side", "middle", "cover-left"):
        band = geometry[name]["band"]
        assert _near(band[3] - band[1], tokens.px("--footnote-band-min")), name
        assert _near(geometry[name]["body"][3], BODY_BOTTOM), name
    assert BODY_BOTTOM == 650


def test_more_footnote_lines_grow_the_band_and_shrink_the_body(geometry):
    band, body = geometry["five"]["band"], geometry["five"]["body"]
    assert band[3] - band[1] > tokens.px("--footnote-band-min") + 25
    assert body[3] < BODY_BOTTOM - 25


def test_the_band_spans_the_content_width_on_every_page(geometry):
    for name in PAGES:
        band = geometry[name]["band"]
        assert _near(band[0], CONTENT_LEFT) and _near(band[2], CONTENT_RIGHT), name


def test_the_image_column_is_its_share_of_the_body(geometry):
    body_width = CONTENT_RIGHT - CONTENT_LEFT
    side, main = geometry["side"]["side"], geometry["side"]["main"]
    assert _near(side[2] - side[0], model.side_column_px("40%", body_width))
    assert _near(side[2], CONTENT_RIGHT) and _near(main[0], CONTENT_LEFT)
    assert _near(side[0] - main[2], tokens.px("--side-gap"))
    left, main = geometry["middle"]["side"], geometry["middle"]["main"]
    assert _near(left[0], CONTENT_LEFT)
    assert _near(left[2] - left[0], model.side_column_px("44%", body_width))
    assert _near(main[0] - left[2], tokens.px("--side-gap"))


def test_a_fitted_picture_is_as_wide_as_its_column_at_the_top(geometry):
    side, img = geometry["side"]["side"], geometry["side"]["sideImg"]
    assert _near(img[0], side[0]) and _near(img[2], side[2])
    assert _near(img[1], side[1])


def test_a_covering_picture_fills_its_column_inside_the_body(geometry):
    page = geometry["cover-left"]
    assert [round(v, 1) for v in page["sideImg"]] == [round(v, 1) for v in page["side"]]
    assert _near(page["sideImg"][3], page["body"][3])


def test_valign_middle_centres_both_columns(geometry):
    page = geometry["middle"]
    for column, content in ((page["side"], page["sideContent"]),
                            (page["main"], page["mainContent"])):
        # the blocks, margins included, sit in the middle of the body
        assert _near((content[0] + content[1]) / 2, (column[1] + column[3]) / 2)
