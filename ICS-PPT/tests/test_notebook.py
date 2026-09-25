import base64
import shutil
from pathlib import Path

import pytest

from lecturekit import dsl, model
from lecturekit import notebook


@pytest.fixture(autouse=True)
def _clear_caches():
    """Clear both in-memory and on-disk caches before each test."""
    notebook._MEM_CACHE.clear()
    if notebook._CACHE_ROOT.exists():
        shutil.rmtree(notebook._CACHE_ROOT)
    yield


def _spec(title="T", body_extra=None):
    def body(p):
        p.title(title)
        p.slide("hello")
        if body_extra:
            body_extra(p)
    return dsl.PageSpec(id="pg", body=body)


def test_build_one_page_lecture_projects_single_page():
    lec = dsl.Lecture(id="live", title="A Story", subtitle="sub", ratio="4:3")
    one = notebook.build_one_page_lecture(lec, _spec(title="Only Me"))
    assert isinstance(one, model.Lecture)
    assert one.id == "live"
    assert one.title == "A Story"
    assert one.subtitle == "sub"
    assert one.ratio == "4:3"
    assert len(one.children) == 1
    assert isinstance(one.children[0], model.Page)
    assert one.children[0].title == "Only Me"


def test_cache_key_changes_when_page_gap_changes(tmp_path):
    lec = dsl.Lecture(id="live", title="A Story")
    no_gap = notebook.build_one_page_lecture(lec, _spec())

    def add_gap(p):
        p.gap("auto", min_px=6, max_px=30)

    with_gap = notebook.build_one_page_lecture(lec, _spec(body_extra=add_gap))

    assert notebook.cache_key(no_gap, tmp_path) != notebook.cache_key(with_gap, tmp_path)


def test_page_handle_news_renders_related_reading_panel():
    lec = dsl.Lecture(id="live", title="A Story")

    def body(p):
        p.title("Scaling")
        p.slide("hello")
        p.news(
            "Scaling Laws <Paper>",
            url="https://arxiv.org/abs/2001.08361",
            source="Kaplan et al.",
            date="2020",
            kind="paper",
            why="Read Figure 1 before class discussion.",
            tags=["scaling-law", "paper"],
        )

    handle = lec.page("scaling", body=body)
    html = handle.news()._repr_html_()

    assert "Related Reading" in html
    assert '<a href="https://arxiv.org/abs/2001.08361"' in html
    assert "Scaling Laws &lt;Paper&gt;" in html
    assert "Kaplan et al." in html
    assert "2020" in html
    assert "paper" in html
    assert "Read Figure 1 before class discussion." in html
    assert "scaling-law" in html


def test_render_news_html_escapes_fields_and_shows_empty_state():
    lec = dsl.Lecture(id="live", title="A Story")
    empty = notebook.render_news_html(lec, _spec())
    assert "No related reading for this page." in empty

    def add_news(p):
        p.news(
            "Title <x>",
            url='https://example.com/?q="x"',
            source="Source <s>",
            why="Why <script>",
        )

    html = notebook.render_news_html(lec, _spec(body_extra=add_news))
    assert "Title &lt;x&gt;" in html
    assert "Source &lt;s&gt;" in html
    assert "Why &lt;script&gt;" in html
    assert "https://example.com/?q=&quot;x&quot;" in html


def test_build_one_page_lecture_rejects_invalid_ratio():
    lec = dsl.Lecture(id="live", title="A Story", ratio="16x9")
    with pytest.raises(model.ValidationError):
        notebook.build_one_page_lecture(lec, _spec())


def test_inline_assets_replaces_local_img_src(tmp_path):
    (tmp_path / "assets").mkdir()
    img = tmp_path / "assets" / "foo.png"
    img.write_bytes(b"\x89PNG\r\n\x1a\nDATA")
    html = '<img src="assets/foo.png" alt="x" />'
    out = notebook.inline_assets(html, tmp_path)
    expected_b64 = base64.b64encode(b"\x89PNG\r\n\x1a\nDATA").decode("ascii")
    assert f"data:image/png;base64,{expected_b64}" in out
    assert "assets/foo.png" not in out


def test_inline_assets_leaves_missing_and_remote_untouched(tmp_path):
    html = '<img src="assets/missing.png"><img src="https://x/y.png">'
    out = notebook.inline_assets(html, tmp_path)
    assert out == html  # nothing resolved, nothing rewritten


def test_inline_assets_handles_css_url_and_longest_match(tmp_path):
    (tmp_path / "assets").mkdir()
    (tmp_path / "assets" / "a.png").write_bytes(b"A")
    (tmp_path / "assets" / "a.png2").write_bytes(b"B")
    html = 'style="background:url(assets/a.png2)" x="assets/a.png"'
    out = notebook.inline_assets(html, tmp_path)
    assert "assets/a.png2" not in out
    assert "assets/a.png" not in out
    assert base64.b64encode(b"A").decode() in out
    assert base64.b64encode(b"B").decode() in out


def test_wrap_iframe_sizes_by_ratio_and_escapes():
    html = '<b>x & "y"</b>'
    out = notebook.wrap_iframe(html, "16:9")
    assert out.startswith("<iframe")
    assert "aspect-ratio:1280/720" in out
    assert "&amp;" in out and "&quot;" in out
    assert "srcdoc=" in out


def test_cache_key_stable_and_sensitive(tmp_path):
    lec = dsl.Lecture(id="live", title="A Story")
    a = notebook.build_one_page_lecture(lec, _spec(title="One"))
    b = notebook.build_one_page_lecture(lec, _spec(title="One"))
    c = notebook.build_one_page_lecture(lec, _spec(title="Two"))
    assert notebook.cache_key(a, tmp_path) == notebook.cache_key(b, tmp_path)
    assert notebook.cache_key(a, tmp_path) != notebook.cache_key(c, tmp_path)


def test_cache_key_tracks_asset_mtime(tmp_path):
    (tmp_path / "assets").mkdir()
    img = tmp_path / "assets" / "f.png"
    img.write_bytes(b"1")

    def add_img(p):
        p.image("assets/f.png")

    lec = dsl.Lecture(id="live", title="A Story")
    one = notebook.build_one_page_lecture(lec, _spec(body_extra=add_img))
    k1 = notebook.cache_key(one, tmp_path)
    import os, time
    later = time.time() + 5
    os.utime(img, (later, later))
    k2 = notebook.cache_key(one, tmp_path)
    assert k1 != k2


def _render_stub(calls):
    """A build_deck replacement that records calls and writes a slides.html."""
    def fake_build_deck(out_dir, formats=("html",), **kw):
        calls.append(Path(out_dir))
        (Path(out_dir) / "slides.html").write_text(
            '<main><img src="assets/f.png"></main>', encoding="utf-8"
        )
    return fake_build_deck


def test_render_page_html_returns_iframe(tmp_path, monkeypatch):
    (tmp_path / "assets").mkdir()
    (tmp_path / "assets" / "f.png").write_bytes(b"PNGBYTES")
    calls = []
    monkeypatch.setattr(notebook, "build_deck", _render_stub(calls))
    notebook._MEM_CACHE.clear()

    def add_img(p):
        p.image("assets/f.png")

    lec = dsl.Lecture(id="live", title="A Story", assets=str(tmp_path))
    spec = _spec(body_extra=add_img)
    html = notebook.render_page_html(lec, spec)
    assert html.startswith("<iframe")
    assert "aspect-ratio:1280/720" in html
    assert base64.b64encode(b"PNGBYTES").decode() in html  # asset inlined
    assert len(calls) == 1


def test_render_page_html_memoizes(tmp_path, monkeypatch):
    calls = []
    monkeypatch.setattr(notebook, "build_deck", _render_stub(calls))
    notebook._MEM_CACHE.clear()
    lec = dsl.Lecture(id="live", title="A Story", assets=str(tmp_path))
    spec = _spec()
    notebook.render_page_html(lec, spec)
    notebook.render_page_html(lec, spec)
    assert len(calls) == 1  # second call served from memory


def test_render_page_html_cache_false_rerenders(tmp_path, monkeypatch):
    calls = []
    monkeypatch.setattr(notebook, "build_deck", _render_stub(calls))
    notebook._MEM_CACHE.clear()
    lec = dsl.Lecture(id="live", title="A Story", assets=str(tmp_path))
    spec = _spec()
    notebook.render_page_html(lec, spec, cache=False)
    notebook.render_page_html(lec, spec, cache=False)
    assert len(calls) == 2  # forced fresh each time


def test_render_page_html_disk_cache_skips_marp(tmp_path, monkeypatch):
    calls = []
    monkeypatch.setattr(notebook, "build_deck", _render_stub(calls))
    notebook._MEM_CACHE.clear()
    lec = dsl.Lecture(id="live", title="A Story", assets=str(tmp_path))
    spec = _spec()

    notebook.render_page_html(lec, spec)
    assert len(calls) == 1

    notebook._MEM_CACHE.clear()  # cold in-process cache; disk bundle survives

    html = notebook.render_page_html(lec, spec)
    assert len(calls) == 1  # served from the on-disk bundle, no re-render
    assert html.startswith("<iframe")


def test_render_page_html_explicit_assets_wins(tmp_path, monkeypatch):
    (tmp_path / "assets").mkdir()
    (tmp_path / "assets" / "f.png").write_bytes(b"Z")
    calls = []
    monkeypatch.setattr(notebook, "build_deck", _render_stub(calls))
    notebook._MEM_CACHE.clear()

    def add_img(p):
        p.image("assets/f.png")

    lec = dsl.Lecture(id="live", title="A Story")  # no assets on the lecture
    html = notebook.render_page_html(lec, _spec(body_extra=add_img), assets=str(tmp_path))
    assert base64.b64encode(b"Z").decode() in html


def test_render_bundle_disables_pagination(tmp_path, monkeypatch):
    # A single inline slide has no meaningful page number, so the notebook path
    # renders the bundle with pagination off.
    monkeypatch.setattr(notebook, "build_deck", lambda *a, **k: None)
    lec = dsl.Lecture(id="live", title="A Story")
    one = notebook.build_one_page_lecture(lec, _spec())
    out = tmp_path / "bundle"
    notebook._render_bundle(one, tmp_path, out)
    md = (out / "slides.md").read_text(encoding="utf-8")
    assert "paginate: false" in md
    assert "paginate: true" not in md
