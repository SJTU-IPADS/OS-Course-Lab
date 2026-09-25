from pathlib import Path
from unittest.mock import patch

import pytest

from lecturekit.renderers.viewer.pdf import find_chrome, render_outline_pdf


def test_find_chrome_honors_chrome_path(monkeypatch, tmp_path):
    fake = tmp_path / "chrome"
    fake.write_text("")
    monkeypatch.setenv("CHROME_PATH", str(fake))
    assert find_chrome() == str(fake)


def test_render_outline_pdf_raises_if_no_outline_html(tmp_path):
    """Guard: raise FileNotFoundError before invoking Chrome when outline.html is absent."""
    with patch("lecturekit.renderers.viewer.pdf.subprocess.run") as run:
        with pytest.raises(FileNotFoundError):
            render_outline_pdf(tmp_path, chrome="/bin/chrome")
    run.assert_not_called()


def test_find_chrome_falls_through_stale_chrome_path_to_chrome_bin(monkeypatch, tmp_path):
    """CHROME_PATH set but stale; CHROME_BIN points to a real file — should return CHROME_BIN."""
    real = tmp_path / "chrome_bin"
    real.write_text("")
    monkeypatch.setenv("CHROME_PATH", str(tmp_path / "nonexistent_chrome"))
    monkeypatch.setenv("CHROME_BIN", str(real))
    assert find_chrome() == str(real)


def test_render_outline_pdf_prints_outline_html(tmp_path):
    (tmp_path / "outline.html").write_text("<html></html>", encoding="utf-8")
    with patch("lecturekit.renderers.viewer.pdf.subprocess.run") as run:
        dest = render_outline_pdf(tmp_path, chrome="/bin/chrome")
    cmd = run.call_args.args[0]
    assert cmd[0] == "/bin/chrome"
    assert "--headless=new" in cmd
    assert f"--print-to-pdf={tmp_path / 'outline.pdf'}" in cmd
    assert (tmp_path / "outline.html").resolve().as_uri() in cmd
    assert dest == tmp_path / "outline.pdf"


def test_merge_pdfs_concatenates_in_order(tmp_path):
    from pypdf import PdfReader, PdfWriter

    from lecturekit.renderers.viewer.pdf import merge_pdfs

    for name in ("a.pdf", "b.pdf"):
        writer = PdfWriter()
        writer.add_blank_page(width=72, height=72)
        with open(tmp_path / name, "wb") as fh:
            writer.write(fh)

    merge_pdfs([tmp_path / "a.pdf", tmp_path / "b.pdf"], tmp_path / "out.pdf")

    assert len(PdfReader(str(tmp_path / "out.pdf")).pages) == 2


def test_link_outline_to_slides_rewrites_uri_to_goto(tmp_path):
    from pypdf import PdfReader, PdfWriter
    from pypdf.annotations import Link

    from lecturekit.renderers.viewer.pdf import SLIDE_LINK_PREFIX, link_outline_to_slides

    # outline page 0 + two slide pages; the outline links to slide index 1.
    writer = PdfWriter()
    for _ in range(3):
        writer.add_blank_page(width=200, height=200)
    writer.add_annotation(
        page_number=0,
        annotation=Link(rect=(0, 0, 50, 20), url=f"{SLIDE_LINK_PREFIX}1"),
    )
    out = tmp_path / "slides.pdf"
    with open(out, "wb") as fh:
        writer.write(fh)

    assert link_outline_to_slides(out) == 1

    reader = PdfReader(str(out))
    action = reader.pages[0]["/Annots"][0].get_object()["/A"]
    assert action["/S"] == "/GoTo"
    # slide index 1 -> merged page 2 (the outline is page 0)
    target = action["/D"][0].get_object()
    assert reader.pages[2].get_object() == target


def test_link_outline_to_slides_ignores_out_of_range_links(tmp_path):
    from pypdf import PdfReader, PdfWriter
    from pypdf.annotations import Link

    from lecturekit.renderers.viewer.pdf import SLIDE_LINK_PREFIX, link_outline_to_slides

    # a link to slide index 5 but only one slide page exists -> left untouched
    writer = PdfWriter()
    for _ in range(2):
        writer.add_blank_page(width=200, height=200)
    writer.add_annotation(
        page_number=0,
        annotation=Link(rect=(0, 0, 50, 20), url=f"{SLIDE_LINK_PREFIX}5"),
    )
    out = tmp_path / "slides.pdf"
    with open(out, "wb") as fh:
        writer.write(fh)

    assert link_outline_to_slides(out) == 0

    reader = PdfReader(str(out))
    action = reader.pages[0]["/Annots"][0].get_object()["/A"]
    assert action["/S"] == "/URI"
