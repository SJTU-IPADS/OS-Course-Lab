"""Print the transcript sheet to PDF with headless Chrome.

The sheet is already a paged document — A4, two columns, its own `@page` box —
so there is nothing to assemble here the way the deck's PDF has to merge an
outline in front of its slides: Chrome prints the file and that is the PDF.
Chrome is found the same way the deck's export finds it.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

from ..viewer.pdf import find_chrome

# Every figure is a data: URI, so a lecture's worth of them has to decode before
# the print snapshot — more generous than the deck's outline page needs.
_VIRTUAL_TIME_MS = 15000


def print_pdf(html_path: Path, *, chrome: str | None = None) -> Path:
    """Render ``html_path`` to a PDF beside it, and return that path."""
    html_path = Path(html_path).resolve()
    if not html_path.exists():
        raise FileNotFoundError(f"no transcript to print: {html_path}")
    dest = html_path.with_suffix(".pdf")
    subprocess.run(
        [
            chrome or find_chrome(),
            "--headless=new",
            "--disable-gpu",
            "--no-pdf-header-footer",
            f"--virtual-time-budget={_VIRTUAL_TIME_MS}",
            f"--print-to-pdf={dest}",
            html_path.as_uri(),
        ],
        check=True,
    )
    return dest
