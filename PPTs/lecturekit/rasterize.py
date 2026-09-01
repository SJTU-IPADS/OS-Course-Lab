"""Turn an SVG file into a PNG file.

Some targets cannot embed SVG: python-pptx has no SVG support, and LaTeX's
graphicx reads only png/jpg/pdf/eps. A renderer for one of those hands its SVGs
to an ``SvgRasterizer`` and embeds the PNG it gets back.

The conversion itself is delegated to whatever SVG renderer the machine has —
rsvg-convert, cairosvg, or Inkscape — because a correct one needs a CSS and
font stack that is not worth reimplementing here. With none of them installed
the rasterizer reports that it cannot convert, and the caller decides what to
do about it (the pptx renderer warns and skips the figure, as it did before
any of this existed).
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

# Rasterize at this multiple of the SVG's intrinsic size. A slide figure is
# often shown larger than its nominal width, and PowerPoint resamples on zoom
# and on print, so 1x reads visibly soft.
SCALE = 2

# Named in the "no backend" warning, so the message says what to install.
BACKEND_HINT = (
    "install one of: rsvg-convert (brew install librsvg), "
    "cairosvg (pip install cairosvg), or Inkscape"
)


def _rsvg(src: Path, dest: Path, scale: int) -> None:
    subprocess.run(
        ["rsvg-convert", "-z", str(scale), "-o", str(dest), str(src)],
        check=True, capture_output=True,
    )


def _inkscape(src: Path, dest: Path, scale: int) -> None:
    subprocess.run(
        ["inkscape", str(src), "-o", str(dest), f"--export-dpi={96 * scale}"],
        check=True, capture_output=True,
    )


def _cairosvg(src: Path, dest: Path, scale: int) -> None:
    import cairosvg

    cairosvg.svg2png(url=str(src), write_to=str(dest), scale=scale)


# In preference order. rsvg-convert and Inkscape are external binaries, so they
# are probed with `which`; cairosvg is a Python module, so it is probed by
# importing it.
_COMMAND_BACKENDS = (("rsvg-convert", _rsvg), ("inkscape", _inkscape))


def find_backend():
    """Return the first available ``(name, convert)`` pair, or ``None``."""
    for command, convert in _COMMAND_BACKENDS:
        if shutil.which(command):
            return command, convert
    try:
        import cairosvg  # noqa: F401
    except ImportError:
        return None
    return "cairosvg", _cairosvg


class SvgRasterizer:
    """Convert SVGs to PNGs under ``workdir``, once per source file.

    The PNGs are throwaway: a renderer embeds their bytes and never refers to
    the files again, so ``workdir`` is normally a temporary directory the
    caller owns and removes.
    """

    def __init__(self, workdir: Path, *, scale: int = SCALE):
        self.workdir = Path(workdir)
        self.scale = scale
        self._backend = find_backend()
        self._done: dict[Path, Path | None] = {}

    @property
    def backend(self) -> str | None:
        """Name of the SVG renderer in use, or ``None`` if there is none."""
        return self._backend[0] if self._backend else None

    def png(self, svg: Path) -> Path | None:
        """Return a PNG of ``svg``, or ``None`` if it could not be produced."""
        svg = Path(svg)
        if svg not in self._done:
            self._done[svg] = self._convert(svg)
        return self._done[svg]

    def _convert(self, svg: Path) -> Path | None:
        if self._backend is None:
            return None
        # Two lectures may ship assets/timeline.svg; number the outputs so the
        # second one does not overwrite the first.
        self.workdir.mkdir(parents=True, exist_ok=True)
        dest = self.workdir / f"{svg.stem}-{len(self._done)}.png"
        _, convert = self._backend
        try:
            convert(svg, dest, self.scale)
        except Exception:
            # A malformed SVG or a backend that dies on it costs one figure,
            # not the whole export — the caller reports the missing picture.
            return None
        return dest if dest.exists() else None
