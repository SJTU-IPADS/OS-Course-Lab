from __future__ import annotations

import io
import os
import shutil
import subprocess
import sys
from pathlib import Path

# The outline page's `<a href>`s use this scheme so headless Chrome emits URI
# link annotations; ``link_outline_to_slides`` rewrites them into internal page
# links after the deck is merged. ``.invalid`` is reserved (RFC 6761) so the
# links can never resolve on the network even if a reader follows one before
# the rewrite.
SLIDE_LINK_PREFIX = "https://lecturekit.invalid/slide/"

_CHROME_NAMES = [
    "google-chrome", "google-chrome-stable",
    "chromium", "chromium-browser", "chrome",
]


def _candidate_paths() -> list[str]:
    paths: list[str] = []
    for var in ("CHROME_PATH", "CHROME_BIN"):
        val = os.environ.get(var)
        if val:
            paths.append(val)
    if sys.platform == "darwin":
        paths += [
            "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
            "/Applications/Chromium.app/Contents/MacOS/Chromium",
        ]
    elif sys.platform.startswith("win"):
        paths += [
            r"C:\Program Files\Google\Chrome\Application\chrome.exe",
            r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        ]
    return paths


def find_chrome() -> str:
    """Locate a Chrome/Chromium for the outline page: env, then platform paths."""
    for path in _candidate_paths():
        if path and Path(path).exists():
            return path
    for name in _CHROME_NAMES:
        found = shutil.which(name)
        if found:
            return found
    raise RuntimeError(
        "no Chrome/Chromium found for the outline PDF page; set CHROME_PATH"
    )


def render_outline_pdf(output_dir: Path, *, chrome: str | None = None) -> Path:
    """Print ``outline.html`` to a single content-sized ``outline.pdf``.

    ``--virtual-time-budget`` lets headless Chrome wait for the page's
    ``fonts.ready`` -> ``@page`` height injection before it prints.
    """
    chrome = chrome or find_chrome()
    source = (output_dir / "outline.html").resolve()
    if not source.exists():
        raise FileNotFoundError(f"no outline.html to render in {output_dir}")
    dest = output_dir / "outline.pdf"
    command = [
        chrome,
        "--headless=new",
        "--disable-gpu",
        "--no-pdf-header-footer",
        "--allow-file-access-from-files",
        "--virtual-time-budget=3000",
        f"--print-to-pdf={dest}",
        source.as_uri(),
    ]
    subprocess.run(command, check=True)
    return dest


def merge_pdfs(parts: list[Path], dest: Path) -> None:
    """Concatenate ``parts`` (in order) into ``dest`` using pypdf."""
    from pypdf import PdfWriter

    writer = PdfWriter()
    for part in parts:
        writer.append(str(part))
    with open(dest, "wb") as fh:
        writer.write(fh)


def link_outline_to_slides(path: Path) -> int:
    """Repoint the outline page's slide links at their pages within the merged PDF.

    The outline is page 0, printed with ``SLIDE_LINK_PREFIX`` URI links that
    headless Chrome turned into URI link annotations. Each one carries a flat
    page index ``i``; here it becomes an internal ``GoTo`` to merged page
    ``i + 1`` (the slides follow the single outline page). Returns the number of
    links rewritten.
    """
    from pypdf import PdfReader, PdfWriter
    from pypdf.generic import ArrayObject, DictionaryObject, NameObject

    # Detach from the file so we can write the result back to the same path.
    reader = PdfReader(io.BytesIO(path.read_bytes()))
    writer = PdfWriter()
    writer.append(reader)

    rewritten = 0
    for ref in writer.pages[0].get("/Annots") or []:
        annot = ref.get_object()
        action = annot.get("/A")
        uri = action.get("/URI") if action else None
        if not uri or not str(uri).startswith(SLIDE_LINK_PREFIX):
            continue
        target = int(str(uri)[len(SLIDE_LINK_PREFIX):]) + 1
        if target >= len(writer.pages):
            continue
        annot[NameObject("/A")] = DictionaryObject({
            NameObject("/S"): NameObject("/GoTo"),
            NameObject("/D"): ArrayObject([
                writer.pages[target].indirect_reference, NameObject("/Fit"),
            ]),
        })
        rewritten += 1

    with open(path, "wb") as fh:
        writer.write(fh)
    return rewritten
