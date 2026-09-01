"""Design tokens: the one palette every renderer paints from.

A colour in this project means the same thing on the projector, in the book, in
PowerPoint and on the printed sheet — a marked keyword is *that* yellow
everywhere. Until this module the four renderers each held their own literal
for it, kept in step by hand and by a comment saying "one source of truth", and
they had already drifted: the book flattened the deck's stroke onto white, the
PPTX and transcript exports reached for a sidenote pastel instead.

So the palette lives in the theme's ``:root`` block — a stylesheet is where
style belongs, and it is the file an author already edits to change a colour —
and everything else reads it from here. No renderer spells a colour out.

Not only colours: a font stack, the face PowerPoint should pick out of it, and
the geometry of a marker stroke are style too, and live in the same block. A
number in a coordinate system only one target has (``--mark-top: 38%`` of an
inline box for the browser, ``--mark-tex-height: 1.28ex`` for LaTeX) is still
the theme's to state — the theme says what the stroke looks like in each
target; the code only carries it there.

One thing this module does *not* try to be is a second config format. The
tokens are CSS custom properties, read with a regex. The deck loads the theme
as-is, so there is nothing to generate, keep in sync, or forget to regenerate.

Alpha is the one translation. The deck paints marker strokes translucent so the
word shows through; PDF and PowerPoint fills here are opaque, so ``opaque()``
composites the token onto the page's white once, in one place, instead of four
hand-flattened hex literals.
"""

from __future__ import annotations

import re
from functools import lru_cache
from pathlib import Path

PKG_ROOT = Path(__file__).resolve().parents[1]
THEME_DIR = PKG_ROOT / "themes"
DEFAULT_THEME = "basic-office"

# `--name: value;` inside the theme's `:root { … }`. Comments are stripped
# first, so a `/* … */` between declarations cannot be read as a value.
_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_ROOT_BLOCK = re.compile(r":root\s*\{(.*?)\}", re.DOTALL)
_DECL = re.compile(r"(--[\w-]+)\s*:\s*([^;]+);")
_RGBA = re.compile(
    r"rgba?\(\s*([\d.]+)[\s,]+([\d.]+)[\s,]+([\d.]+)\s*(?:[,/]\s*([\d.]+)\s*)?\)"
)
_HEX = re.compile(r"#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$")


class UnknownToken(KeyError):
    """A token the theme does not define — a typo, or a renamed property."""


@lru_cache(maxsize=None)
def tokens(theme: str = DEFAULT_THEME) -> dict[str, str]:
    """Every custom property in ``theme``'s ``:root``, as authored."""
    path = THEME_DIR / f"{theme}.css"
    if not path.is_file():
        # `themes/` lives beside the package, not inside it, so a wheel does
        # not carry it: lecturekit runs from a checkout (`pip install -e`, or
        # PYTHONPATH), and this is the first thing that notices otherwise.
        raise FileNotFoundError(
            f"{path} not found — lecturekit reads its palette from the checkout's "
            "themes/ directory; install with `pip install -e .` or put the "
            "repository on PYTHONPATH"
        )
    css = _COMMENT.sub("", path.read_text(encoding="utf-8"))
    block = _ROOT_BLOCK.search(css)
    if not block:
        raise ValueError(f"theme {theme!r} has no :root block to read tokens from")
    return {name: value.strip() for name, value in _DECL.findall(block.group(1))}


def value(name: str, theme: str = DEFAULT_THEME) -> str:
    """The token's CSS value, verbatim (``#156082``, ``rgba(…)``, a font stack)."""
    try:
        return tokens(theme)[name]
    except KeyError:
        raise UnknownToken(
            f"{name} is not in themes/{theme}.css :root — renamed, or a typo?"
        ) from None


def opaque(name: str, theme: str = DEFAULT_THEME) -> str:
    """The token as ``#rrggbb``, translucency composited onto the page's white.

    Print and PowerPoint have no alpha where these are used, and a wash is only
    ever laid over the white of a page, so flattening it here reproduces what
    the deck shows. An already-opaque token comes back unchanged, so a caller
    never has to ask which kind it holds.
    """
    return _flatten(value(name, theme), name)


def is_colour(name: str, theme: str = DEFAULT_THEME) -> bool:
    """Whether the token holds a colour (as opposed to a font stack, a length)."""
    css = value(name, theme)
    return bool(_HEX.match(css) or _RGBA.match(css))


def hex6(name: str, theme: str = DEFAULT_THEME) -> str:
    """`opaque()` as ``RRGGBB`` — the spelling ``\\definecolor{…}{HTML}{…}`` and
    python-pptx's ``RGBColor.from_string`` both take."""
    return opaque(name, theme).lstrip("#").upper()


def families(name: str = "--font-base", theme: str = DEFAULT_THEME) -> list[str]:
    """A font-stack token split into family names, quotes stripped.

    The browser walks the stack; PowerPoint and LaTeX each carry one face per
    script and have to pick out of it.
    """
    return [f.strip().strip('"\'') for f in value(name, theme).split(",") if f.strip()]


def face(name: str, theme: str = DEFAULT_THEME) -> str:
    """A single-family token (``--pptx-font-cjk: "PingFang SC"``), unquoted."""
    return value(name, theme).strip().strip('"\'')


def numbered(prefix: str, theme: str = DEFAULT_THEME) -> list[str]:
    """The tokens ``<prefix>1``, ``<prefix>2``, … in numeric order — a wheel.

    ``numbered("--sidenote-")`` lists the sidenote slots; ``--sidenote-border``
    is not one of them.
    """
    names = [n for n in tokens(theme) if n.startswith(prefix) and n[len(prefix):].isdigit()]
    return sorted(names, key=lambda n: int(n[len(prefix):]))


def css(name: str, theme: str = DEFAULT_THEME) -> str:
    """The token as a standalone stylesheet wants it: a colour composited onto
    white (the printed sheet has no alpha), anything else verbatim."""
    return opaque(name, theme) if is_colour(name, theme) else value(name, theme)


def tex(name: str, theme: str = DEFAULT_THEME) -> str:
    """The token as a LaTeX preamble wants it: a colour as the ``RRGGBB`` of
    ``\\definecolor{…}{HTML}{…}``, a length (``1.28ex``) verbatim."""
    return hex6(name, theme) if is_colour(name, theme) else value(name, theme)


# `@--token@`, the spelling a target's own stylesheet or preamble uses to name a
# token it cannot look up itself: LaTeX has no CSS variables, and the transcript
# sheet is printed standalone rather than beside the theme.
_TOKEN_REF = re.compile(r"@(--[\w-]+)@")


def substitute(text: str, *, form=tex, theme: str = DEFAULT_THEME) -> str:
    """Replace every ``@--token@`` in ``text`` with that token's value.

    ``form`` picks the spelling the target wants — :func:`tex` for a LaTeX
    preamble, :func:`css` for a standalone stylesheet. An unknown token raises
    rather than reaching the page as its own name.
    """
    return _TOKEN_REF.sub(lambda m: form(m.group(1), theme), text)


def _flatten(css: str, name: str) -> str:
    hexed = _HEX.match(css)
    if hexed:
        digits = hexed.group(1)
        if len(digits) == 3:
            digits = "".join(c * 2 for c in digits)
        return f"#{digits.lower()}"
    rgba = _RGBA.match(css)
    if not rgba:
        raise ValueError(
            f"{name}: {css!r} is not a colour this module can flatten — "
            "write it as #rgb, #rrggbb, or rgb[a](r, g, b[, a]) with a in 0–1"
        )
    r, g, b = (float(rgba.group(i)) for i in (1, 2, 3))
    alpha = float(rgba.group(4)) if rgba.group(4) else 1.0
    return "#" + "".join(
        f"{round(255 - alpha * (255 - channel)):02x}" for channel in (r, g, b)
    )
