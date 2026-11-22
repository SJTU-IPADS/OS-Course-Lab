"""The drawing context, and the shapes the elements draw with.

An element class (``lecturekit.elements``) draws itself from a :class:`Ctx`
(slide, layout, vertical cursor, reader): it adds shape(s) at the cursor and
advances the cursor by the block's estimated height. Shapes, pictures and the
architecture diagram are here. What is written in them is drawn by :mod:`draw`,
and the page itself (bands, image column, footnotes, bubbles) by :mod:`page`.
Styling comes from :mod:`theme`.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

from pptx.dml.color import RGBColor
from pptx.enum.dml import MSO_LINE_DASH_STYLE
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.text import MSO_ANCHOR, MSO_AUTO_SIZE, PP_ALIGN
from pptx.oxml.ns import qn
from pptx.parts.image import Image as ImageFile
from pptx.util import Emu, Pt

from lecturekit import model, rasterize, tokens
from lecturekit.markup import Reader
from . import theme
from .layout import (
    LINE_HEIGHT, Cursor, Layout, estimate_text_height, estimate_text_width,
    fit_within, px,
)

# Image formats python-pptx can embed. `.svg` is not one of them — it is
# rasterized to PNG first (see `resolve_image`), so authors need no separate
# raster copy of a vector figure.
RASTER_EXTS = (".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tif", ".tiff")
EMBEDDABLE_EXTS = RASTER_EXTS + (".svg",)

GAP = Emu(Pt(8))            # vertical gap between blocks (~0.6em of body)


@dataclass
class Ctx:
    slide: object
    layout: Layout
    cursor: Cursor
    read: Reader
    asset_root: Path | None = None
    sidenote_index: int = 0
    rasterizer: rasterize.SvgRasterizer | None = None
    # Review sources, so an `assets/<source id>/…` src resolves back to the
    # lecture it was borrowed from. See model.resolve_asset.
    borrowed: tuple = ()
    # The page's title, for the cover that draws it.
    title: str = ""
    # The footnote numbers each block is marked with, by the block's id().
    numbers: dict[int, tuple[int, ...]] = field(default_factory=dict)
    # Elements the block being drawn set beside its text (a float), which its
    # footnote marker does not follow.
    floated: list[object] = field(default_factory=list)


# --- shared helpers ---


def solid(shape, color: RGBColor) -> None:
    shape.fill.solid()
    shape.fill.fore_color.rgb = color


def outline(shape, color: RGBColor, width_pt: float) -> None:
    shape.line.color.rgb = color
    shape.line.width = Pt(width_pt)


def shape_at(ctx: Ctx, preset, left: int, top: int, width: int, height: int):
    """An autoshape carrying no styling of its own — the caller sets all of it.

    ``add_shape`` attaches a ``<p:style>`` pointing at the Office theme's accent
    fill, line and shadow. The deck theme owns all three, and a renderer is free
    to prefer that reference over the explicit fill and line set here, so the
    reference goes and what this module sets is the whole story. The empty
    effect list is what says "no shadow": only the highlight chip casts one, and
    it sets its own with :func:`drop_shadow`.
    """
    shape = ctx.slide.shapes.add_shape(preset, left, top, width, height)
    style = shape._element.find(qn("p:style"))
    if style is not None:
        shape._element.remove(style)
    shape.shadow.inherit = False
    return shape


def no_line(shape) -> None:
    shape.line.fill.background()


def drop_shadow(shape, color: RGBColor, *, alpha: float,
                 blur_pt: float, dist_pt: float) -> None:
    """Give a shape the theme's soft downward shadow.

    python-pptx exposes only shadow *inheritance*, so the effect goes in by
    hand. `<a:effectLst>` follows the fill and the line inside `<a:spPr>`, and
    PowerPoint rejects a file that orders them otherwise, so it is appended
    last — after `solid`/`outline` have written theirs. `dir` is in 60000ths of
    a degree, and 90 degrees points straight down.
    """
    spPr = shape._element.spPr
    for stale in spPr.findall(qn("a:effectLst")):
        spPr.remove(stale)
    lst = spPr.makeelement(qn("a:effectLst"), {})
    shdw = lst.makeelement(qn("a:outerShdw"), {
        "blurRad": str(Emu(Pt(blur_pt))),
        "dist": str(Emu(Pt(dist_pt))),
        "dir": "5400000",
        "rotWithShape": "0",
    })
    clr = shdw.makeelement(qn("a:srgbClr"), {"val": f"{color}"})
    clr.append(clr.makeelement(qn("a:alpha"), {"val": str(int(alpha * 100000))}))
    shdw.append(clr)
    lst.append(shdw)
    spPr.append(lst)


def textbox(ctx: Ctx, height: int, *, left: int | None = None, width: int | None = None):
    box = ctx.slide.shapes.add_textbox(
        ctx.layout.content_left if left is None else left,
        ctx.cursor.top,
        ctx.layout.content_width if width is None else width,
        height,
    )
    tf = box.text_frame
    tf.word_wrap = True
    tf.auto_size = MSO_AUTO_SIZE.SHAPE_TO_FIT_TEXT
    # Zero margins so the box's auto-fit height equals the laid-out text height
    # (and the estimate that placed the next block) — nonzero margins grow the
    # box on open and overlap what follows. Drawers needing inset set their own.
    tf.margin_left = tf.margin_right = 0
    tf.margin_top = tf.margin_bottom = 0
    return box, tf


def set_line_spacing(paragraph, size_pt: float) -> None:
    """Exact point line spacing (size * 1.4), matching estimate_text_height.

    A float multiple is scaled by the font's own leading in PowerPoint, so the
    rendered height exceeds the estimate; an exact point value does not.
    """
    paragraph.line_spacing = Pt(size_pt * LINE_HEIGHT)


def set_east_asian(run, typeface: str) -> None:
    """Set the run's CJK typeface (<a:ea>); font.name only sets the Latin face.

    The element schema orders <a:ea> right after <a:latin>, which font.name has
    already added, so the new node is inserted after it.
    """
    rPr = run.font._rPr
    ea = rPr.find(qn("a:ea"))
    if ea is None:
        ea = rPr.makeelement(qn("a:ea"), {})
        latin = rPr.find(qn("a:latin"))
        latin.addnext(ea) if latin is not None else rPr.append(ea)
    ea.set("typeface", typeface)


def set_highlight(run, tone: str) -> None:
    """Wash the run with a `<mark>` tone, via PowerPoint's own text highlight.

    python-pptx has no API for it, so the element goes in by hand. The schema
    orders <a:highlight> *before* <a:latin>, which font.name has already added,
    so it is inserted ahead of it — appending would produce a file PowerPoint
    rejects.
    """
    rPr = run.font._rPr
    node = rPr.find(qn("a:highlight"))
    if node is None:
        node = rPr.makeelement(qn("a:highlight"), {})
        latin = rPr.find(qn("a:latin"))
        latin.addprevious(node) if latin is not None else rPr.append(node)
    for child in list(node):
        node.remove(child)
    node.append(node.makeelement(
        qn("a:srgbClr"), {"val": f"{theme.MARK_WHEEL[tone]}"}
    ))


def style_run(run, text: str, *, size_pt: float, color: RGBColor,
               bold: bool = False, italic: bool = False, mono: bool = False,
               mark: str | None = None) -> None:
    run.text = text
    font = run.font
    font.name = theme.FONT_MONO if mono else theme.FONT_BASE
    font.size = Pt(size_pt)
    font.bold = bold
    font.italic = italic
    font.color.rgb = color
    set_east_asian(run, theme.cjk_font())
    if mark:
        set_highlight(run, mark)


# --- cover ---


COVER_TITLE_PT = 40
COVER_META_PT = 22
COVER_TIME_PT = 19
COVER_ACCENT = RGBColor.from_string(tokens.hex6("--cover-accent"))
COVER_LOGO_MAX_W = Emu(Pt(240))
COVER_LOGO_MAX_H = Emu(Pt(58))


BRIDGE_PT = 30.0    # above TITLE_PT, matching the deck's 30pt


def draw_cover_logos(logo, ctx: Ctx) -> None:
    if logo is None:
        return
    top = Emu(Pt(32))
    margin = Emu(Pt(54))
    if isinstance(logo, str):
        draw_cover_logo(logo, ctx.layout.width - margin - COVER_LOGO_MAX_W, top, ctx)
        return
    left = logo.get("left")
    right = logo.get("right")
    if left:
        draw_cover_logo(left, margin, top, ctx)
    if right:
        draw_cover_logo(right, ctx.layout.width - margin - COVER_LOGO_MAX_W, top, ctx)


def draw_cover_logo(src: str, left: int, top: int, ctx: Ctx) -> None:
    path = resolve_image(src, ctx)
    if path is None:
        return
    pic = add_picture(ctx, path, src, left, top, width=COVER_LOGO_MAX_W)
    if pic.height > COVER_LOGO_MAX_H:
        pic.width, pic.height = fit_within(
            pic.width, pic.height, COVER_LOGO_MAX_W, COVER_LOGO_MAX_H
        )
    if pic.width < COVER_LOGO_MAX_W:
        pic.left = left + (COVER_LOGO_MAX_W - pic.width) // 2


# --- highlight (centered emphasis chip) ---


HIGHLIGHT_PAD = Emu(Pt(10))     # matches the theme's 0.28em/0.8em chip padding
HIGHLIGHT_RULE_PT = 1.0         # the theme's 1.5px frame, in points
# The theme's `box-shadow: 0 2px 8px rgba(13, 35, 50, 0.18)`, in points.
HIGHLIGHT_SHADOW = {"alpha": 0.18, "blur_pt": 6.0, "dist_pt": 1.5}


# --- sidenote (boxed callout) ---


LOGO_SIZE = Emu(Pt(36))     # sidenote image-logo box (~2.4em at 18pt, like the theme)


# --- text ---


ALIGN = {"left": PP_ALIGN.LEFT, "center": PP_ALIGN.CENTER, "right": PP_ALIGN.RIGHT}


# --- image ---


def resolve_image(src: str, ctx: Ctx) -> Path | None:
    """The local file to embed for ``src``, rasterizing an SVG on the way.

    ``None`` for what cannot be embedded, which is named on stderr.
    """
    if src.startswith(("http://", "https://")):
        return skipped(src, "a picture on the web is not downloaded")
    if not src.lower().endswith(EMBEDDABLE_EXTS):
        return skipped(src, "PowerPoint cannot embed this format")
    path = model.resolve_asset(src, ctx.asset_root, ctx.borrowed)
    if not path.exists():
        return skipped(src, "no such file")
    if path.suffix.lower() != ".svg":
        return path
    png = ctx.rasterizer.png(path) if ctx.rasterizer else None
    if png is not None:
        return png
    if ctx.rasterizer is None or ctx.rasterizer.backend is None:
        return skipped(path, f"no SVG renderer found — {rasterize.BACKEND_HINT}")
    return skipped(path, f"{ctx.rasterizer.backend} could not convert it")


def add_picture(ctx: Ctx, path: Path, src: str, left: int, top: int,
                width: int | None = None, height: int | None = None):
    """Embed ``path``, which is what ``src`` resolved to.

    The picture's description names ``src``: a rasterized figure's own file is
    a throwaway PNG, and the author looks for the figure by the name written.
    """
    picture = ctx.slide.shapes.add_picture(str(path), left, top, width, height)
    picture._element._nvXxPr.cNvPr.set("descr", str(src))
    return picture


def skipped(source: str | Path, reason: str) -> None:
    print(f"lecturekit: skipping {source} in PPTX: {reason}", file=sys.stderr)


def natural_size(path: Path) -> tuple[int, int]:
    """A picture's own size in EMU, the size python-pptx would add it at."""
    image = ImageFile.from_file(str(path))
    (width, height), (dpi_x, dpi_y) = image.size, image.dpi
    return round(914400 * width / dpi_x), round(914400 * height / dpi_y)


def requested_image_size(image: dict, natural_w: int, natural_h: int,
                          layout: Layout) -> tuple[int, int] | None:
    """Resolve the DSL's explicit px/% image size while preserving aspect ratio."""
    target_w = size_to_emu(image.get("width"), layout.width)
    target_h = size_to_emu(image.get("height"), layout.height)
    if target_w is None and target_h is None:
        return None
    scales = []
    if target_w is not None:
        scales.append(target_w / natural_w)
    if target_h is not None:
        scales.append(target_h / natural_h)
    scale = min(scales)
    return round(natural_w * scale), round(natural_h * scale)


def size_to_emu(value, whole: int) -> int | None:
    """A CSS length (px, %, em of the body's type) as EMU; None for any other."""
    if value is None:
        return None
    if isinstance(value, int):
        return px(value)
    found = re.fullmatch(r"([\d.]+)(px|%|em)?", str(value).strip())
    if found is None:
        return None
    number, unit = float(found[1]), found[2]
    if unit == "%":
        return round(whole * number / 100)
    return Emu(Pt(number * theme.BODY_PT)) if unit == "em" else px(number)


# --- architecture diagram (layered bands of module boxes) ---


# The theme's `.lk-arch*` metrics, in CSS pixels at the diagram's own size.
ARCH_FIG_GAP = 6          # figure gap: between bands, and around a chevron
ARCH_BAND_PAD_X = 12      # .lk-arch-layer padding
ARCH_BAND_PAD_Y = 10
ARCH_BAND_GAP = 12        # label gutter to module track
ARCH_LABEL_W = 140        # .lk-arch-label flex basis
ARCH_MOD_GAP = 8          # .lk-arch-modules gap
ARCH_MOD_PAD_X = 10       # .lk-arch-module padding
ARCH_MOD_PAD_Y = 8
ARCH_MORE_PAD_X = 14      # the dashed "more modules" box is padded wider
ARCH_BORDER_PT = 0.75     # the 1px hairline of a band and of a module box

# A diagram taller than the room left is drawn smaller, but only this far: past
# it the module labels stop being readable from the back of a lecture hall.
ARCH_MIN_SCALE = 0.55


@dataclass
class ArchMetrics:
    """The architecture metrics at one scale: ``size`` in points, rest in EMU."""

    size: float
    fig_gap: int
    band_pad_x: int
    band_pad_y: int
    band_gap: int
    label_w: int
    mod_gap: int
    mod_pad_x: int
    mod_pad_y: int
    more_pad_x: int

    @classmethod
    def at(cls, scale: float) -> "ArchMetrics":
        return cls(
            size=theme.ARCH_PT * scale,
            fig_gap=px(ARCH_FIG_GAP * scale),
            band_pad_x=px(ARCH_BAND_PAD_X * scale),
            band_pad_y=px(ARCH_BAND_PAD_Y * scale),
            band_gap=px(ARCH_BAND_GAP * scale),
            label_w=px(ARCH_LABEL_W * scale),
            mod_gap=px(ARCH_MOD_GAP * scale),
            mod_pad_x=px(ARCH_MOD_PAD_X * scale),
            mod_pad_y=px(ARCH_MOD_PAD_Y * scale),
            more_pad_x=px(ARCH_MORE_PAD_X * scale),
        )


def arch_module_widths(modules: list[str], track: int,
                        m: ArchMetrics) -> list[int]:
    """Split a layer's module track between its boxes.

    Follows the theme's flex bases: a `…` box hugs its own text
    (``flex:0 1 auto``) and the ordinary boxes share what is left
    (``flex:1 1 0``).
    """
    hug = {
        i: estimate_text_width(module, font_pt=m.size) + 2 * m.more_pad_x
        for i, module in enumerate(modules) if module == model.ARCH_ELLIPSIS
    }
    shared = len(modules) - len(hug)
    floor = 2 * m.mod_pad_x + Emu(Pt(m.size))
    each = max(floor, (track - sum(hug.values())) // shared) if shared else 0
    return [hug.get(i, each) for i in range(len(modules))]


def arch_layout(diagram: dict, ctx: Ctx,
                 m: ArchMetrics) -> tuple[int, list[dict]]:
    """Measure the diagram at ``m``: total height, then one entry per band.

    Every width comes from the content column, so only the heights depend on the
    text — a band is as tall as its tallest module box, or as its label.
    """
    bands = []
    for layer in diagram["layers"]:
        title = layer.get("title")
        gutter = m.label_w if title else 0
        track = ctx.layout.content_width - 2 * m.band_pad_x - gutter
        if title:
            track -= m.band_gap
        modules = layer["modules"]
        widths = arch_module_widths(
            modules, track - m.mod_gap * (len(modules) - 1), m
        )
        module_height = 2 * m.mod_pad_y + max(
            estimate_text_height(module, font_pt=m.size,
                                 width=max(width - 2 * m.mod_pad_x, 1))
            for module, width in zip(modules, widths)
        )
        label_height = (
            estimate_text_height(title, font_pt=m.size, width=gutter)
            if title else 0
        )
        bands.append({
            "height": max(module_height, label_height) + 2 * m.band_pad_y,
            "module_height": module_height,
            "gutter": gutter,
            "widths": widths,
        })
    seams = len(bands) - 1
    height = sum(band["height"] for band in bands) + seams * m.fig_gap
    if diagram.get("flow"):
        height += seams * (Emu(Pt(m.size)) + m.fig_gap)
    return height, bands


def arch_flow(glyph: str, top: int, m: ArchMetrics, ctx: Ctx) -> int:
    """The direction chevron between two bands; returns the height it took."""
    height = Emu(Pt(m.size))
    box = ctx.slide.shapes.add_textbox(
        ctx.layout.content_left, top, ctx.layout.content_width, height
    )
    tf = box.text_frame
    tf.word_wrap = False
    tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
    p = tf.paragraphs[0]
    p.alignment = PP_ALIGN.CENTER
    p.line_spacing = Pt(m.size)     # the theme's line-height: 1
    style_run(p.add_run(), glyph, size_pt=m.size, color=theme.ACCENT1)
    return height


def arch_band(layer: dict, band: dict, top: int, m: ArchMetrics,
               ctx: Ctx) -> None:
    """One layer: the tinted band, its bold label, and its module boxes."""
    shape = shape_at(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, ctx.layout.content_left,
                   top, ctx.layout.content_width, band["height"])
    solid(shape, theme.LT2)
    outline(shape, theme.BOX_BORDER, ARCH_BORDER_PT)
    left = ctx.layout.content_left + m.band_pad_x
    title = layer.get("title")
    if title:
        box = ctx.slide.shapes.add_textbox(
            left, top + m.band_pad_y, band["gutter"],
            band["height"] - 2 * m.band_pad_y,
        )
        tf = box.text_frame
        tf.word_wrap = True
        tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
        tf.vertical_anchor = MSO_ANCHOR.MIDDLE
        p = tf.paragraphs[0]
        set_line_spacing(p, m.size)
        style_run(p.add_run(), title, size_pt=m.size, color=theme.FG, bold=True)
        left += band["gutter"] + m.band_gap
    module_top = top + (band["height"] - band["module_height"]) // 2
    for module, width in zip(layer["modules"], band["widths"]):
        arch_module(module, left, module_top, width, band["module_height"],
                     m, ctx)
        left += width + m.mod_gap


def arch_module(text: str, left: int, top: int, width: int, height: int,
                 m: ArchMetrics, ctx: Ctx) -> None:
    """One module box: white and solid, or faded and dashed for `…`."""
    more = text == model.ARCH_ELLIPSIS
    shape = shape_at(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
    if more:
        shape.fill.background()
    else:
        solid(shape, theme.BG)
    outline(shape, theme.BOX_BORDER, ARCH_BORDER_PT)
    if more:
        shape.line.dash_style = MSO_LINE_DASH_STYLE.DASH
    tf = shape.text_frame
    tf.word_wrap = True
    tf.margin_left = tf.margin_right = m.mod_pad_x
    tf.margin_top = tf.margin_bottom = m.mod_pad_y
    tf.vertical_anchor = MSO_ANCHOR.MIDDLE
    p = tf.paragraphs[0]
    p.alignment = PP_ALIGN.CENTER
    set_line_spacing(p, m.size)
    style_run(p.add_run(), text, size_pt=m.size,
               color=theme.DK2 if more else theme.FG)


# --- row (side-by-side image figures) ---


def row_widths(items: list[dict], track: int, layout: Layout) -> list[int]:
    """Each item's share of the track: sized items first, the rest split evenly."""
    sized = {}
    for i, item in enumerate(items):
        width = size_to_emu(item.get("width"), layout.width)
        if width is not None:
            sized[i] = width
    shared = len(items) - len(sized)
    each = max(0, track - sum(sized.values())) // shared if shared else 0
    widths = [sized.get(i, each) for i in range(len(items))]
    if sum(widths) > track > 0:     # too wide together: shrink, as flex does
        widths = [round(width * track / sum(widths)) for width in widths]
    return widths
