"""The colour scheme of the viewer's web pages: what a page gets from the one chosen.

A scheme is data, a file in ``themes/schemes/`` (see :mod:`lecturekit.tokens`).
What a page gets of it is a fragment for the top of its <head>: a <meta> that
says which grounds it can stand on, the tokens as CSS, and for the deck the
pictures drawn for the scheme (see :mod:`lecturekit.variants`), each scheme
under the media query it shows in. No script chooses a scheme, so a system that
changes over changes the page at once.

The renderer writes the deck's fragment into the bundle (``HEAD_FILE``);
``build_deck`` and the dev server both put it into the page with :func:`dress`,
and neither has to know what is in it. ``slides.md`` and the deck's theme hold
nothing of the choice, and each scheme's part is for the screen: print and
every export are the default scheme's.
"""

from __future__ import annotations

import hashlib
import re
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import quote

from ... import bind, tokens, variables, variants

HEAD_FILE = "head.html"

# The one value of --color-scheme that is no scheme: the default one, and where
# the system is dark, the dark one.
AUTO = "auto"
_DARK_SYSTEM = ("screen and (prefers-color-scheme: dark)", "dark")

_PAGE_CSS = Path(__file__).resolve().parent / "assets" / "page.css"
# Where Marp draws a `![bg]` picture.
_BACKGROUND = "[data-marpit-advanced-background-container] > figure"
# The one --adapt-* that goes with the ink, so with the palette.
_NUMBER_GROUND = "--adapt-page-number"
# A style with a picture behind it: asked first, so that other elements are cheap.
_WITH_BACKGROUND = '[style*="background-image"]'


@dataclass(frozen=True)
class _Swap:
    """A picture of the bundle, and the variant a scheme paints in its place."""

    picture: str  # its path in the bundle: assets/fig.svg
    variant: str
    drawing: str | None  # the variant's SVG, when it is a bound figure

    @property
    def paths(self) -> tuple[str, ...]:
        """The picture's path as it is, and encoded as markdown-it does."""
        encoded = quote(self.picture, safe="/;:@&=+$,-_.!~*'()")
        return tuple(dict.fromkeys((self.picture, encoded)))

    @property
    def in_style(self) -> list[str]:
        """Selectors of a ``style`` that has the picture as a background: the
        end of its ``url()``, whatever leads to it. Few and short, as every
        picture with a variant has them."""
        written = (f"{path}{close})" for path in self.paths for close in ('"', "'", ""))
        return [f"[style*={_string(end)}]" for end in written]

    @property
    def paint_property(self) -> str:
        """The custom property ``vars.js`` hands a bound variant's drawing in."""
        return f"--lk-drawn-{hashlib.sha1(self.variant.encode()).hexdigest()[:10]}"

    @property
    def paint(self) -> str:
        url = f"url({_string(quote(self.variant))})"
        return f"var({self.paint_property}, {url})" if self.drawing else url

    def css(self) -> str:
        """The box stays the picture's; only what is painted in it changes."""
        # A bound figure's src moves with its variables, its data-lk-figure does not.
        attrs = ("src", "data-lk-figure") if self.drawing else ("src",)
        shown = [f"[{attr}={_string(lead + path)}]"
                 for attr in attrs for lead in ("", "./") for path in self.paths]
        behind = f"{_WITH_BACKGROUND}:is({', '.join(self.in_style)})"
        return (
            f"img:is({', '.join(shown)}) {{\n"
            "  object-position: -99999px 0 !important;\n"
            f"  background: {self.paint} center / var(--lk-fit, 100% 100%) "
            "no-repeat content-box transparent !important;\n"
            "}\n"
            f"section{behind}, figure{behind} {{\n"
            f"  background-image: {self.paint} !important;\n"
            "}\n"
        )


def choices() -> tuple[str, ...]:
    """What ``--color-scheme`` takes."""
    return (AUTO, *tokens.schemes())


def theme(css: str) -> str:
    """The deck's theme ``css`` with the default scheme's colours in it.

    In a cascade layer, so that the scheme a page shows, declared in its head,
    wins over them.
    """
    return f"{css}\n{_layered(default_colours())}"


def default_colours() -> str:
    """The default scheme as a stylesheet's ``:root`` rule."""
    return f":root {{\n{_declarations(tokens.scheme(tokens.DEFAULT_SCHEME).colours)}}}\n"


def head(choice: str, bundle: Path | None = None) -> str:
    """What a page opens its <head> with, under ``choice``.

    ``bundle`` is the deck's, for the pictures in it. The order is for a page
    that arrives in pieces, which Firefox paints as far as it has got. The
    charset leads: until that is known, a page in an iframe is white. Then the
    grounds, the scheme, and only then the default, layered so that the scheme
    wins over it. The pictures, which can be many, come last.
    """
    shown = [(media, tokens.scheme(name)) for media, name in _shown(choice)]
    laid = [(media, scheme) for media, scheme in shown if media]
    grounds = dict.fromkeys(scheme.base for _, scheme in shown)
    return "".join([
        '<meta charset="utf-8">',
        f'<meta name="color-scheme" content="{" ".join(grounds)}">',
        f"<style>\n{_PAGE_CSS.read_text(encoding='utf-8')}</style>",
        *(f'<style data-lk-scheme media="{media}">\n{_palette(scheme)}</style>'
          for media, scheme in laid),
        f"<style>\n{_default()}</style>",
        *(_pictures(media, scheme, bundle) for media, scheme in laid),
    ])


def dress(html: str, bundle: Path) -> str:
    """``html``, a page of ``bundle``, with the head the renderer wrote for it."""
    return insert(html, page_head(bundle))


def page_head(bundle: Path) -> str:
    path = Path(bundle, HEAD_FILE)
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def insert(html: str, fragment: str) -> str:
    """Return ``html`` with ``fragment`` as the first thing in its <head>.

    First, because a deck is megabytes long and the first <meta> of a name is
    the one a browser obeys. A document without a <head> (a file caught half
    written) is led by the fragment.
    """
    before, tag, after = html.partition("<head>")
    return f"{before}{tag}{fragment}{after}" if tag else fragment + html


def _shown(choice: str) -> tuple[tuple[str | None, str], ...]:
    """The schemes a page can show, each with the media query it is laid over
    the default in. The default itself needs none."""
    if choice == AUTO:
        return ((None, tokens.DEFAULT_SCHEME), _DARK_SYSTEM)
    media = None if choice == tokens.DEFAULT_SCHEME else "screen"
    return ((media, choice),)


def _default() -> str:
    """The default scheme, under whichever scheme is laid over it."""
    ground = tokens.scheme(tokens.DEFAULT_SCHEME).base
    return _layered(f":root {{ color-scheme: {ground}; }}\n{default_colours()}")


def _layered(css: str) -> str:
    """``css`` where any rule of a scheme laid over it wins."""
    return f"@layer lk-scheme {{\n{css}}}\n"


def _palette(scheme: tokens.Scheme) -> str:
    """``scheme``'s colours, on the document and on each slide."""
    return (
        f":root {{ color-scheme: {scheme.base}; }}\n"
        f":root, section {{\n{_declarations(scheme.colours)}}}\n"
    )


def _pictures(media: str, scheme: tokens.Scheme, bundle: Path | None) -> str:
    """What ``scheme`` does with the bundle's pictures."""
    swaps = _swaps(scheme, bundle) if bundle else []
    css = _keep_default_over_backgrounds(swaps) + "".join(swap.css() for swap in swaps)
    parts = [f'<style media="{media}">\n{css}</style>']
    if swaps:
        parts.append(_fetch_ahead(swaps))
    parts += [
        variables.figure_script(swap.variant, swap.drawing, paint=swap.paint_property)
        for swap in swaps if swap.drawing
    ]
    return "".join(parts)


def _keep_default_over_backgrounds(swaps: list[_Swap]) -> str:
    """A page over a background picture that has no variant keeps the default
    palette, since the picture was drawn for that, and the page number's ground
    with it: the number is in that palette's ink, which the page's own colour
    can equal (an inverted page). What else the ground asks (``--adapt-*``)
    still holds."""
    drawn = [selector for swap in swaps for selector in swap.in_style]
    unless = f":not({', '.join(drawn)})" if drawn else ""
    palette = {
        name: value for name, value in tokens.scheme(tokens.DEFAULT_SCHEME).colours.items()
        if name == _NUMBER_GROUND or not name.startswith("--adapt-")
    }
    return (
        f"section{_WITH_BACKGROUND}{unless},\n"
        f"svg:has(> foreignObject > section > {_BACKGROUND}{unless}) > foreignObject > section "
        f"{{\n{_declarations(palette)}}}\n"
    )


def _fetch_ahead(swaps: list[_Swap]) -> str:
    """The variants fetched with the page. A browser loads a stylesheet's
    picture when it first paints it, which would blank it on a page turn and
    when the system changes over. (page.css keeps the box out of sight.)"""
    files = dict.fromkeys(f"url({_string(quote(swap.variant))})" for swap in swaps)
    css = f":root::after {{ content: {' '.join(files)}; }}"
    return f'<style media="screen">\n{css}\n</style>'


def _swaps(scheme: tokens.Scheme, bundle: Path) -> list[_Swap]:
    found = []
    for picture in variants.pictures(Path(bundle, "assets")):
        variant = variants.chosen(picture, scheme)
        if variant:
            found.append(_Swap(
                picture=picture.relative_to(bundle).as_posix(),
                variant=variant.relative_to(bundle).as_posix(),
                drawing=variant.read_text(encoding="utf-8") if bind.is_bound(variant) else None,
            ))
    return found


def _declarations(colours: dict[str, str]) -> str:
    return "".join(f"  {name}: {value};\n" for name, value in colours.items())


def _string(text: str) -> str:
    """``text`` as a CSS string, safe inside a <style> element."""
    escaped = re.sub(r'["\\<\n\r\f]', lambda char: f"\\{ord(char.group()):x} ", text)
    return f'"{escaped}"'
