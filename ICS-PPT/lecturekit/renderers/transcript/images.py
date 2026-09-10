"""Figures, embedded into the sheet itself.

The transcript is one file a student mails, opens and prints, so every figure
travels inside it as a `data:` URI rather than beside it in an `assets/`
directory. Vector figures go in as `image/svg+xml`; a raster is downscaled to
print resolution first, when Pillow is around, because a 6 MB screenshot is
still 6 MB after base64 and nothing on paper can show it.

An SVG rides inside an `<img>` rather than inlined into the document on
purpose: the lecture figures share ids (`id="ah"`, the arrowhead marker, in
hundreds of them) and carry their own `<style>` blocks, which would collide the
moment two of them lived in one DOM. An `<img>` gives each figure its own
document, and collisions cannot happen.
"""

from __future__ import annotations

import base64
import re
import sys
from dataclasses import dataclass
from pathlib import Path

from lecturekit import model

# Figures print at most a column wide (~92 mm); 1200 px covers that past 300 dpi.
MAX_RASTER_PX = 1200

_MIME = {
    ".svg": "image/svg+xml",
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif": "image/gif",
    ".webp": "image/webp",
}

_VIEWBOX_RE = re.compile(r'viewBox\s*=\s*"([^"]+)"')
_WIDTH_RE = re.compile(r'\bwidth\s*=\s*"([0-9.]+)(px)?"')

#: The intrinsic width a figure is assumed to have when it cannot be measured.
#: The repo's own convention (`soul/figure.md`) draws a full-width figure at
#: ~900 px, which is also the width that maps to a full print column.
DEFAULT_WIDTH_PX = 900


@dataclass(frozen=True)
class Figure:
    """An embedded figure: its `data:` URI and its intrinsic width in px."""

    uri: str
    width_px: int


class Embedder:
    """Reads figures off disk and hands back `data:` URIs, memoized by source."""

    def __init__(self, asset_root: Path | None, borrowed: tuple = ()):
        self.asset_root = asset_root
        self.borrowed = borrowed
        self._cache: dict[str, Figure | None] = {}
        self.missing: list[str] = []

    def embed(self, src: str) -> Figure | None:
        if src not in self._cache:
            self._cache[src] = self._load(src)
        return self._cache[src]

    def _load(self, src: str) -> Figure | None:
        path = model.resolve_asset(src, self.asset_root, self.borrowed)
        if not path.exists():
            self._warn(f"missing figure: {path}")
            return None
        mime = _MIME.get(path.suffix.lower())
        if mime is None:
            self._warn(f"cannot embed {src} in the transcript (unknown format)")
            return None
        if mime == "image/svg+xml":
            data = path.read_bytes()
            return Figure(_uri(mime, data), _svg_width(data))
        return _raster(path, mime, self._warn)

    def _warn(self, message: str) -> None:
        self.missing.append(message)
        print(f"lecturekit: {message}", file=sys.stderr)


def _uri(mime: str, data: bytes) -> str:
    return f"data:{mime};base64,{base64.b64encode(data).decode('ascii')}"


def _svg_width(data: bytes) -> int:
    """The SVG's intrinsic width: its viewBox width, else its `width` attribute."""
    head = data[:2048].decode("utf-8", errors="replace")
    box = _VIEWBOX_RE.search(head)
    if box:
        parts = box.group(1).replace(",", " ").split()
        if len(parts) == 4:
            try:
                return max(1, round(float(parts[2])))
            except ValueError:
                pass
    width = _WIDTH_RE.search(head)
    if width:
        try:
            return max(1, round(float(width.group(1))))
        except ValueError:
            pass
    return DEFAULT_WIDTH_PX


def _raster(path: Path, mime: str, warn) -> Figure:
    """A bitmap, downscaled to print resolution when Pillow is available."""
    try:
        from PIL import Image
    except ImportError:
        data = path.read_bytes()
        return Figure(_uri(mime, data), _png_width(data) or DEFAULT_WIDTH_PX)

    import io

    try:
        with Image.open(path) as image:
            width, height = image.size
            if width <= MAX_RASTER_PX:
                return Figure(_uri(mime, path.read_bytes()), width)
            scaled = image.convert("RGBA" if _has_alpha(image) else "RGB").resize(
                (MAX_RASTER_PX, max(1, round(height * MAX_RASTER_PX / width))),
                Image.LANCZOS,
            )
            buffer = io.BytesIO()
            if scaled.mode == "RGBA":
                scaled.save(buffer, format="PNG", optimize=True)
                out_mime = "image/png"
            else:
                scaled.save(buffer, format="JPEG", quality=82, optimize=True)
                out_mime = "image/jpeg"
            return Figure(_uri(out_mime, buffer.getvalue()), MAX_RASTER_PX)
    except Exception as exc:  # a corrupt or exotic bitmap: ship it untouched
        warn(f"could not downscale {path.name} ({exc}); embedding as-is")
        data = path.read_bytes()
        return Figure(_uri(mime, data), DEFAULT_WIDTH_PX)


def _has_alpha(image) -> bool:
    return image.mode in ("RGBA", "LA", "PA") or "transparency" in image.info


def _png_width(data: bytes) -> int | None:
    """A PNG's width straight out of its IHDR, for machines without Pillow."""
    if data[:8] != b"\x89PNG\r\n\x1a\n" or len(data) < 24:
        return None
    return int.from_bytes(data[16:20], "big")
