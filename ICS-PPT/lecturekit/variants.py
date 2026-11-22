"""A picture drawn again for a colour scheme: ``fig.dark.svg`` beside ``fig.svg``.

The file's name is the whole interface. A page names ``assets/fig.svg``; a
scheme shows ``fig.<its name>.svg`` in its place, or failing that
``fig.<its ground>.svg`` (so a second dark scheme takes the ``.dark`` pictures
already drawn), or failing that the picture itself. The default scheme, which
is every export's, always shows the picture itself.

A variant has the shape of the picture it replaces, which :func:`check` sees to.
"""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from pathlib import Path

from . import model, tokens

_SUFFIXES = frozenset({".svg", ".png", ".jpg", ".jpeg", ".gif", ".webp"})
_LENGTH = re.compile(r"\s*([\d.]+)\s*(?:px|pt|pc|in|cm|mm)?\s*")
# What a browser takes an SVG without a size or a viewBox to be.
_UNSIZED = (300.0, 150.0)


def suffixes(scheme: tokens.Scheme) -> tuple[str, ...]:
    """The names a picture for ``scheme`` can carry, the preferred first."""
    if scheme.name == tokens.DEFAULT_SCHEME:
        return ()
    return tuple(dict.fromkeys((scheme.name, scheme.base)))


def pictures(assets: Path) -> list[Path]:
    """Every picture file under ``assets``."""
    return sorted(path for path in assets.rglob("*") if path.suffix.lower() in _SUFFIXES)


def chosen(picture: Path, scheme: tokens.Scheme) -> Path | None:
    """What ``scheme`` shows in place of ``picture``, if anything."""
    found = (_named(picture, suffix) for suffix in suffixes(scheme))
    return next((path for path in found if path.is_file()), None)


def drawn(picture: Path) -> list[Path]:
    """Every variant of ``picture`` that some scheme would show."""
    found = (chosen(picture, tokens.scheme(name)) for name in tokens.schemes())
    return list(dict.fromkeys(path for path in found if path))


def check(lecture_dir: Path) -> None:
    """Every variant under the lecture's ``assets`` has its picture's shape.

    Up to a pixel of rounding. A browser gives the picture's box to the
    variant, so another shape would be drawn squashed or letterboxed in it.
    """
    for picture in pictures(Path(lecture_dir, "assets")):
        for variant in drawn(picture):
            width, height = _size(picture)
            across, down = _size(variant)
            if abs(across * height - down * width) > max(width, height, across, down):
                raise model.ValidationError(
                    f"{variant.relative_to(lecture_dir).as_posix()} is {across:g}x{down:g}, "
                    f"and {picture.relative_to(lecture_dir).as_posix()}, which it replaces "
                    f"in a colour scheme, is {width:g}x{height:g}; draw both on one canvas"
                )


def _named(picture: Path, suffix: str) -> Path:
    return picture.with_name(f"{picture.stem}.{suffix}{picture.suffix}")


def _size(path: Path) -> tuple[float, float]:
    """A picture's intrinsic size, in its own units."""
    try:
        if path.suffix.lower() == ".svg":
            return _svg_size(path)
        from PIL import Image

        with Image.open(path) as image:
            return image.size
    except (OSError, ValueError, ET.ParseError) as error:
        raise model.ValidationError(f"cannot read the size of {path}: {error}") from None


def _svg_size(path: Path) -> tuple[float, float]:
    with path.open("rb") as file:
        root = next(ET.iterparse(file, events=("start",)))[1]
    stated = [_LENGTH.fullmatch(root.get(side, "")) for side in ("width", "height")]
    if all(stated):
        return float(stated[0].group(1)), float(stated[1].group(1))
    box = root.get("viewBox", "").replace(",", " ").split()
    return (float(box[2]), float(box[3])) if len(box) == 4 else _UNSIZED
