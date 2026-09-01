import tempfile
from pathlib import Path
from unittest.mock import patch

from lecturekit.renderers.viewer import marp
from lecturekit.renderers.viewer.marp import build_deck


def _outputs(run_mock):
    """The `-o` argument from each Marp invocation, in call order."""
    outs = []
    for call in run_mock.call_args_list:
        cmd = call.args[0]
        outs.append(cmd[cmd.index("-o") + 1])
    return outs


def _command_for(run_mock, output):
    """The full Marp command whose `-o` target is ``output``."""
    for call in run_mock.call_args_list:
        cmd = call.args[0]
        if cmd[cmd.index("-o") + 1] == output:
            return cmd
    raise AssertionError(f"no Marp call produced {output}")


def test_build_deck_builds_html_only_by_default():
    with tempfile.TemporaryDirectory() as tmp:
        with patch("lecturekit.renderers.viewer.marp.subprocess.run") as run:
            build_deck(Path(tmp))
    assert _outputs(run) == ["slides.html"]


def test_html_build_patches_slides_with_svg_scope():
    # A rendered bundle outlives the session that made it, and whoever opens it
    # in Safari pays for marpit-svg-polyfill, so the scoper is written into the
    # file rather than injected on serve (the watch server's slides.html belongs
    # to `marp --watch`, so that path injects instead).
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)

        def fake_run(command, **kwargs):
            (out / "slides.html").write_text(
                "<html><body><section>slide</section></body></html>", encoding="utf-8"
            )

        with patch("lecturekit.renderers.viewer.marp.subprocess.run", fake_run):
            build_deck(out)
        body = (out / "slides.html").read_text(encoding="utf-8")

    assert "data-lk-marpit-svg" in body
    assert body.index("data-lk-marpit-svg") < body.rindex("</body>")
    assert "<section>slide</section>" in body


def test_html_build_does_not_allow_local_files():
    with tempfile.TemporaryDirectory() as tmp:
        with patch("lecturekit.renderers.viewer.marp.subprocess.run") as run:
            build_deck(Path(tmp))
    cmd = _command_for(run, "slides.html")
    assert "--allow-local-files" not in cmd
    assert "theme.css" in cmd  # the unmodified viewer theme


def test_build_deck_pdf_assembles_outline_and_pages(monkeypatch):
    import lecturekit.renderers.viewer.pdf as pdf

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        merged = {}
        monkeypatch.setattr(pdf, "render_outline_pdf", lambda d: d / "outline.pdf")
        linked = {}
        monkeypatch.setattr(
            pdf, "merge_pdfs",
            lambda parts, dest: merged.update(parts=parts, dest=dest),
        )
        monkeypatch.setattr(
            pdf, "link_outline_to_slides",
            lambda path: linked.update(path=path) or 0,
        )
        with patch("lecturekit.renderers.viewer.marp.subprocess.run") as run:
            build_deck(out, ("html", "pdf"))

        # Marp produces the html deck and the pages-pdf intermediate, not slides.pdf
        assert _outputs(run) == ["slides.html", "pages.pdf"]
        cmd = _command_for(run, "pages.pdf")
        assert "--allow-local-files" in cmd
        assert "theme-pdf.css" in cmd
        # the outline page and the pages pdf are merged into slides.pdf
        assert merged["parts"] == [out / "outline.pdf", out / "pages.pdf"]
        assert merged["dest"] == out / "slides.pdf"
        # the outline page's slide links are rewritten in the merged deck
        assert linked["path"] == out / "slides.pdf"
        # the embedded CJK font is copied into the bundle for the outline render
        assert (out / "NotoSansSC-Regular.woff2").exists()


def test_build_deck_pdf_name_overrides_output(monkeypatch):
    import lecturekit.renderers.viewer.pdf as pdf

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        merged = {}
        monkeypatch.setattr(pdf, "render_outline_pdf", lambda d: d / "outline.pdf")
        linked = {}
        monkeypatch.setattr(
            pdf, "merge_pdfs",
            lambda parts, dest: merged.update(parts=parts, dest=dest),
        )
        monkeypatch.setattr(
            pdf, "link_outline_to_slides",
            lambda path: linked.update(path=path) or 0,
        )
        with patch("lecturekit.renderers.viewer.marp.subprocess.run"):
            build_deck(out, ("pdf",), name="operating-systems")

        # the final, user-facing PDF carries the lecture name, not "slides"
        assert merged["dest"] == out / "operating-systems.pdf"
        assert linked["path"] == out / "operating-systems.pdf"


def test_build_deck_png_exports_per_page_images():
    with tempfile.TemporaryDirectory() as tmp:
        with patch("lecturekit.renderers.viewer.marp.subprocess.run") as run:
            build_deck(Path(tmp), ("html", "png"))
    assert _outputs(run) == ["slides.html", "slides.png"]
    cmd = _command_for(run, "slides.png")
    assert "--images" in cmd and "png" in cmd
    # PNG renders headless (like the PDF pages), so it needs local files + the
    # CJK-embedding theme.
    assert "--allow-local-files" in cmd
    assert "theme-pdf.css" in cmd


def test_pdf_pages_theme_embeds_cjk_font(monkeypatch):
    import lecturekit.renderers.viewer.pdf as pdf

    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        monkeypatch.setattr(pdf, "render_outline_pdf", lambda d: d / "outline.pdf")
        monkeypatch.setattr(pdf, "merge_pdfs", lambda parts, dest: None)
        monkeypatch.setattr(pdf, "link_outline_to_slides", lambda path: 0)
        with patch("lecturekit.renderers.viewer.marp.subprocess.run"):
            build_deck(out, ("pdf",))
        pdf_theme = (out / "theme-pdf.css").read_text(encoding="utf-8")
        assert "@font-face" in pdf_theme
        assert "Noto Sans SC" in pdf_theme
        assert "PingFang SC" not in pdf_theme.split("--font-base")[-1].split(";")[0]
        assert (out / "NotoSansSC-Regular.woff2").exists()


def test_theme_gives_list_lead_paragraphs_subtle_weight():
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    rule = theme.split("p:has(+ ul),", 1)[1].split("}", 1)[0]

    assert "p:has(+ ol)" in rule
    assert "margin-bottom: 0.1em;" in rule
    assert "font-weight: 600;" in rule


def test_theme_no_logo_cover_uses_full_height_for_main_content():
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    selector = "section.cover > .lk-cover:not(:has(.lk-cover-logos))"
    rule = theme.split(selector, 1)[1].split("}", 1)[0]

    assert "grid-template-rows: 1fr;" in rule


def test_theme_cover_title_can_use_full_slide_width():
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    rule = theme.split(".lk-cover-title", 1)[1].split("}", 1)[0]

    assert "width: 100%;" in rule
    assert "min(980px" not in rule


def test_theme_styles_auto_gap_flow_and_spacers():
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    assert "section.lk-gap-auto > .lk-gap-flow" in theme
    flow_rule = theme.split("section.lk-gap-auto > .lk-gap-flow", 1)[1].split("}", 1)[0]
    spacer_rule = theme.split(".lk-gap-spacer", 1)[1].split("}", 1)[0]

    assert "display: flex;" in flow_rule
    assert "flex-direction: column;" in flow_rule
    assert "flex: 1 1 auto;" in flow_rule
    assert "flex: 1 0 var(--lk-gap-min);" in spacer_rule
    assert "min-height: var(--lk-gap-min);" in spacer_rule
    assert "max-height: var(--lk-gap-max);" in spacer_rule


def test_theme_preserves_block_layout_inside_auto_gap_wrappers():
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    assert "section.lk-gap-auto .lk-gap-block > .lk-float" in theme
    assert "section.lk-gap-auto .lk-gap-block > figure.lk-figure" in theme
    assert "section.lk-gap-auto .lk-gap-block > figure.lk-row" in theme
    assert "section.lk-gap-auto .lk-gap-block > figure.lk-arch" in theme


def test_theme_bundles_its_latin_faces_instead_of_fetching_them():
    """A remote @import would stall every deck load on an unreachable network."""
    theme = (Path(__file__).parents[1] / "themes" / "basic-office.css").read_text(
        encoding="utf-8"
    )

    assert "@import url(\"http" not in theme  # nothing render-blocking off-host
    for name in marp.LATIN_FONTS:
        assert f'url("{name}")' in theme
        assert (Path(__file__).parents[1] / "themes" / "fonts" / name).exists()


def test_theme_copy_brings_the_font_files_along():
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        with patch("lecturekit.renderers.viewer.marp.subprocess.run"):
            build_deck(out)

        for name in marp.LATIN_FONTS:
            assert (out / name).exists()


def test_marp_command_prefers_a_local_install_over_npx(tmp_path, monkeypatch):
    monkeypatch.delenv("LECTUREKIT_MARP", raising=False)
    monkeypatch.setattr(marp, "PKG_ROOT", tmp_path)
    monkeypatch.setattr(marp.shutil, "which", lambda name: None)

    assert marp.marp_command() == [
        "npx", "--yes", "--prefer-offline", marp.MARP_PACKAGE,
    ]

    vendored = tmp_path / "node_modules" / ".bin" / "marp"
    vendored.parent.mkdir(parents=True)
    vendored.touch()

    assert marp.marp_command() == [str(vendored)]


def test_npx_fallback_is_pinned_and_offline_first():
    """`@latest` + a live registry lookup is what hangs on a bad network."""
    assert "@latest" not in marp.MARP_PACKAGE
    assert marp.MARP_PACKAGE == f"@marp-team/marp-cli@{marp.MARP_VERSION}"
