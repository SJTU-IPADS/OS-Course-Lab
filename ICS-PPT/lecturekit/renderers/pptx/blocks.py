"""Per-block drawers: map one AST block onto native PowerPoint shapes.

Each drawer takes a :class:`Ctx` (slide, layout, vertical cursor, asset root)
and the block, draws shape(s) at the cursor, and advances the cursor by the
block's estimated height. Styling comes from :mod:`theme`. Annotation bubbles
are the one deferred overlay: they are not in ``PPTX_KINDS`` and never reach
here. An ``image_right`` figure does not wrap text the way CSS floats do —
PowerPoint has no text wrap around a shape — so the text box is narrowed to
sit beside the picture instead.
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

from pptx.dml.color import RGBColor
from pptx.enum.dml import MSO_LINE_DASH_STYLE
from pptx.enum.shapes import MSO_SHAPE
from pptx.enum.text import MSO_ANCHOR, MSO_AUTO_SIZE, PP_ALIGN
from pptx.oxml.ns import qn
from pptx.util import Emu, Pt

from lecturekit import model, rasterize, tokens
from . import omml, theme
from .layout import (
    LINE_HEIGHT, Cursor, Layout, estimate_text_height, estimate_text_width,
    fit_within, px,
)
from .text import Para, Run, emphasize, parse_inline, parse_markdown

# Block kinds the PPTX renderer draws. Only the annotation overlay is deferred,
# so select_blocks drops that one.
PPTX_KINDS = {
    "cover", "slide", "code", "demo", "link", "image", "side_image", "aside",
    "sidenote", "table", "highlight", "bridge", "architecture", "row",
}

# Image formats python-pptx can embed. `.svg` is not one of them — it is
# rasterized to PNG first (see `_resolve_image`), so authors need no separate
# raster copy of a vector figure.
_RASTER_EXTS = (".png", ".jpg", ".jpeg", ".gif", ".bmp", ".tif", ".tiff")
_EMBEDDABLE_EXTS = _RASTER_EXTS + (".svg",)

_GAP = Emu(Pt(8))            # vertical gap between blocks (~0.6em of body)
_PARA_GAP_EM = 0.45          # space after each paragraph (~viewer p/li margins)
_GROUP_GAP_EM = 0.55         # extra space before a numbered group heading
_LIST_INDENT = Emu(Pt(18))   # approx indent per list level (drives width estimate)


@dataclass
class Ctx:
    slide: object
    layout: Layout
    cursor: Cursor
    asset_root: Path | None = None
    sidenote_index: int = 0
    rasterizer: rasterize.SvgRasterizer | None = None
    # Review sources, so an `assets/<source id>/…` src resolves back to the
    # lecture it was borrowed from. See model.resolve_asset.
    borrowed: tuple = ()


def draw_block(block, ctx: Ctx) -> None:
    drawer = _DRAWERS.get(block.kind)
    if drawer is not None:
        drawer(block, ctx)


# --- shared helpers ---


def _solid(shape, color: RGBColor) -> None:
    shape.fill.solid()
    shape.fill.fore_color.rgb = color


def _line(shape, color: RGBColor, width_pt: float) -> None:
    shape.line.color.rgb = color
    shape.line.width = Pt(width_pt)


def _shape(ctx: Ctx, preset, left: int, top: int, width: int, height: int):
    """An autoshape carrying no styling of its own — the caller sets all of it.

    ``add_shape`` attaches a ``<p:style>`` pointing at the Office theme's accent
    fill, line and shadow. The deck theme owns all three, and a renderer is free
    to prefer that reference over the explicit fill and line set here, so the
    reference goes and what this module sets is the whole story. The empty
    effect list is what says "no shadow": only the highlight chip casts one, and
    it sets its own with :func:`_drop_shadow`.
    """
    shape = ctx.slide.shapes.add_shape(preset, left, top, width, height)
    style = shape._element.find(qn("p:style"))
    if style is not None:
        shape._element.remove(style)
    shape.shadow.inherit = False
    return shape


def _no_line(shape) -> None:
    shape.line.fill.background()


def _drop_shadow(shape, color: RGBColor, *, alpha: float,
                 blur_pt: float, dist_pt: float) -> None:
    """Give a shape the theme's soft downward shadow.

    python-pptx exposes only shadow *inheritance*, so the effect goes in by
    hand. `<a:effectLst>` follows the fill and the line inside `<a:spPr>`, and
    PowerPoint rejects a file that orders them otherwise, so it is appended
    last — after `_solid`/`_line` have written theirs. `dir` is in 60000ths of
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


def _textbox(ctx: Ctx, height: int, *, left: int | None = None, width: int | None = None):
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


def _set_line_spacing(paragraph, size_pt: float) -> None:
    """Exact point line spacing (size * 1.4), matching estimate_text_height.

    A float multiple is scaled by the font's own leading in PowerPoint, so the
    rendered height exceeds the estimate; an exact point value does not.
    """
    paragraph.line_spacing = Pt(size_pt * LINE_HEIGHT)


def _set_east_asian(run, typeface: str) -> None:
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


def _set_highlight(run, tone: str) -> None:
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


def _style_run(run, text: str, *, size_pt: float, color: RGBColor,
               bold: bool = False, italic: bool = False, mono: bool = False,
               mark: str | None = None) -> None:
    run.text = text
    font = run.font
    font.name = theme.FONT_MONO if mono else theme.FONT_BASE
    font.size = Pt(size_pt)
    font.bold = bold
    font.italic = italic
    font.color.rgb = color
    _set_east_asian(run, theme.cjk_font())
    if mark:
        _set_highlight(run, mark)


def _draw_math(paragraph, latex: str, *, size_pt: float, display: bool = False) -> None:
    """Append a native PowerPoint equation to the paragraph (see :mod:`omml`)."""
    node = omml.to_alternate_content(latex, display=display)
    omml.apply_run_props(node, size_pt=size_pt, color=f"{theme.FG}")
    paragraph._p.append(node)


def _apply_runs(paragraph, runs: list[Run], *, size_pt: float, color: RGBColor) -> None:
    for r in runs:
        if r.math:
            _draw_math(paragraph, r.math, size_pt=size_pt)
            continue
        run = paragraph.add_run()
        if r.link:
            _style_run(run, r.text, size_pt=size_pt, color=theme.LINK, mark=r.mark)
            run.hyperlink.address = r.link
        elif r.code:
            _style_run(run, r.text, size_pt=size_pt, color=theme.ACCENT1,
                       mono=True, mark=r.mark)
        elif r.bold:
            _style_run(run, r.text, size_pt=size_pt, color=theme.DK2,
                       bold=True, mark=r.mark)
        else:
            _style_run(run, r.text, size_pt=size_pt, color=color,
                       italic=r.italic, mark=r.mark)


def _para_text_len(para: Para) -> int:
    return sum(len(r.text) for r in para.runs)


# --- title (the slide's h1) ---


def draw_title(title: str, ctx: Ctx) -> None:
    size = theme.TITLE_PT
    height = estimate_text_height(
        title, font_pt=size, width=ctx.layout.content_width
    )
    box, tf = _textbox(ctx, height)
    _set_line_spacing(tf.paragraphs[0], size)
    _style_run(tf.paragraphs[0].add_run(), title,
               size_pt=size, color=theme.FG, bold=True)
    top = ctx.cursor.place(height)
    # The accent rule under the title (h1 border-bottom), with a deliberate
    # weight — a hairline reads as nearly invisible in PowerPoint.
    rule_y = top + height
    rule = ctx.slide.shapes.add_connector(
        1, ctx.layout.content_left, rule_y,
        ctx.layout.content_left + ctx.layout.content_width, rule_y,
    )
    rule.line.color.rgb = theme.ACCENT1
    rule.line.width = Pt(theme.TITLE_RULE_PT)
    ctx.cursor.advance(_GAP)


# --- cover ---


_COVER_TITLE_PT = 40
_COVER_META_PT = 22
_COVER_TIME_PT = 19
_COVER_ACCENT = RGBColor.from_string(tokens.hex6("--cover-accent"))
_COVER_LOGO_MAX_W = Emu(Pt(240))
_COVER_LOGO_MAX_H = Emu(Pt(58))


def draw_cover(title: str, block, ctx: Ctx) -> None:
    cover = block.content
    _draw_cover_logos(cover.get("logo"), ctx)

    title_width = int(ctx.layout.width * 0.78)
    title_left = (ctx.layout.width - title_width) // 2
    title_top = int(ctx.layout.height * 0.24)
    title_height = int(ctx.layout.height * 0.26)
    box = ctx.slide.shapes.add_textbox(title_left, title_top, title_width, title_height)
    tf = box.text_frame
    tf.word_wrap = True
    tf.auto_size = MSO_AUTO_SIZE.TEXT_TO_FIT_SHAPE
    tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
    p = tf.paragraphs[0]
    p.alignment = PP_ALIGN.CENTER
    _set_line_spacing(p, _COVER_TITLE_PT)
    _style_run(p.add_run(), title, size_pt=_COVER_TITLE_PT,
               color=_COVER_ACCENT)

    meta_top = int(ctx.layout.height * 0.57)
    meta_height = int(ctx.layout.height * 0.2)
    meta = ctx.slide.shapes.add_textbox(
        title_left, meta_top, title_width, meta_height
    )
    mtf = meta.text_frame
    mtf.word_wrap = True
    mtf.auto_size = MSO_AUTO_SIZE.SHAPE_TO_FIT_TEXT
    mtf.margin_left = mtf.margin_right = mtf.margin_top = mtf.margin_bottom = 0
    first = True
    if cover.get("author"):
        p = mtf.paragraphs[0]
        p.alignment = PP_ALIGN.CENTER
        _set_line_spacing(p, _COVER_META_PT)
        _style_run(p.add_run(), cover["author"], size_pt=_COVER_META_PT,
                   color=theme.FG, bold=True)
        first = False
    if cover.get("time"):
        p = mtf.paragraphs[0] if first else mtf.add_paragraph()
        p.alignment = PP_ALIGN.CENTER
        _set_line_spacing(p, _COVER_TIME_PT)
        _style_run(p.add_run(), cover["time"], size_pt=_COVER_TIME_PT,
                   color=theme.DK2)


_BRIDGE_PT = 30.0    # above TITLE_PT, matching the deck's 30pt


def draw_bridge(block, ctx: Ctx) -> None:
    """A transition page: its plain-text lines centered on an empty slide.

    Like ``draw_cover``, this owns the whole slide — no title, no footnotes —
    so the renderer calls it instead of the block loop.
    """
    width = int(ctx.layout.width * 0.82)
    left = (ctx.layout.width - width) // 2
    box = ctx.slide.shapes.add_textbox(left, 0, width, ctx.layout.height)
    tf = box.text_frame
    tf.word_wrap = True
    tf.vertical_anchor = MSO_ANCHOR.MIDDLE
    tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
    first = True
    for line in str(block.content["text"]).splitlines():
        p = tf.paragraphs[0] if first else tf.add_paragraph()
        first = False
        p.alignment = PP_ALIGN.CENTER
        _set_line_spacing(p, _BRIDGE_PT)
        _style_run(p.add_run(), line, size_pt=_BRIDGE_PT, color=theme.FG)


def _draw_cover_logos(logo, ctx: Ctx) -> None:
    if logo is None:
        return
    top = Emu(Pt(32))
    margin = Emu(Pt(54))
    if isinstance(logo, str):
        _draw_cover_logo(logo, ctx.layout.width - margin - _COVER_LOGO_MAX_W, top, ctx)
        return
    left = logo.get("left")
    right = logo.get("right")
    if left:
        _draw_cover_logo(left, margin, top, ctx)
    if right:
        _draw_cover_logo(right, ctx.layout.width - margin - _COVER_LOGO_MAX_W, top, ctx)


def _draw_cover_logo(src: str, left: int, top: int, ctx: Ctx) -> None:
    path = _resolve_image(src, ctx)
    if path is None:
        return
    pic = ctx.slide.shapes.add_picture(str(path), left, top, width=_COVER_LOGO_MAX_W)
    if pic.height > _COVER_LOGO_MAX_H:
        pic.width, pic.height = fit_within(
            pic.width, pic.height, _COVER_LOGO_MAX_W, _COVER_LOGO_MAX_H
        )
    if pic.width < _COVER_LOGO_MAX_W:
        pic.left = left + (_COVER_LOGO_MAX_W - pic.width) // 2


# --- slide (markdown body) ---


def _para_size(para: Para) -> int:
    return theme.HEADING_PT[para.level] if para.kind == "heading" else theme.BODY_PT


def _para_space_before(para: Para, index: int) -> float:
    """Extra space (pt) above a paragraph, to group sections like the viewer.

    A numbered top-level item (an ordered list at level 0) starts a new group,
    so it gets a gap above it — except the very first paragraph on the slide.
    """
    if index and para.kind == "ordered" and para.level == 0:
        return theme.BODY_PT * _GROUP_GAP_EM
    return 0.0


# Gap between an `image_right` picture and the text beside it (theme: 0.8em).
_FLOAT_GAP = Emu(Pt(16))
# The float never takes more than this share of the column, so the text it sits
# beside keeps a readable measure even when the author asked for a wide image.
_FLOAT_MAX_SHARE = 0.45


def _float_picture(image: dict, ctx: Ctx, top: int):
    """Place an ``image_right`` picture flush with the content box's right edge.

    Returns the picture, or ``None`` when the source cannot be embedded (a URL,
    or an SVG on a machine with no rasterizer). The caller narrows its text box
    by the picture's width: PowerPoint cannot wrap text around a shape, so the
    two sit side by side rather than the text flowing around the figure.
    """
    path = _resolve_image(image["src"], ctx)
    if path is None:
        return None
    pic = ctx.slide.shapes.add_picture(str(path), ctx.layout.content_left, top)
    requested = _requested_image_size(image, pic.width, pic.height, ctx.layout)
    if requested is not None:
        pic.width, pic.height = requested
    pic.width, pic.height = fit_within(
        pic.width, pic.height,
        round(ctx.layout.content_width * _FLOAT_MAX_SHARE),
        max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining()),
    )
    pic.left = ctx.layout.content_left + ctx.layout.content_width - pic.width
    pic.top = top
    return pic


def _slide(block, ctx: Ctx) -> None:
    pic = None
    width = ctx.layout.content_width
    if block.float_image:
        pic = _float_picture(block.float_image, ctx, ctx.cursor.top)
        if pic is not None:
            width -= pic.width + _FLOAT_GAP
    paras = parse_markdown(str(block.content))
    height = _estimate_paras_height(paras, width)
    box, tf = _textbox(ctx, height, width=width)
    first = True
    ordinal = 0
    for index, para in enumerate(paras):
        p = tf.paragraphs[0] if first else tf.add_paragraph()
        first = False
        size = _para_size(para)
        if para.kind == "math":
            # Display equation: no exact line spacing, so PowerPoint can give
            # stacked limits and fractions the vertical room they need.
            p.space_after = Pt(size * _PARA_GAP_EM)
            p.alignment = PP_ALIGN.CENTER
            _draw_math(p, para.runs[0].math, size_pt=size, display=True)
            continue
        _set_line_spacing(p, size)
        p.space_after = Pt(size * _PARA_GAP_EM)
        before = _para_space_before(para, index)
        if before:
            p.space_before = Pt(before)
        if para.kind in ("bullet", "ordered"):
            p.level = min(para.level, 8)
            ordinal = ordinal + 1 if para.kind == "ordered" else 0
            marker = f"{ordinal}.  " if para.kind == "ordered" else "•  "
            _hanging_indent(p, marker, size_pt=size, level=para.level)
            _style_run(p.add_run(), marker, size_pt=size, color=theme.ACCENT1, bold=True)
        if para.kind == "heading":
            for r in para.runs:
                _style_run(p.add_run(), r.text, size_pt=size, color=theme.FG,
                           bold=True, mark=r.mark)
        else:
            _apply_runs(p, para.runs, size_pt=size, color=theme.FG)
    # A floated picture may be taller than the text beside it; the block owns
    # whichever is taller, so the next block clears both.
    ctx.cursor.place(max(height, pic.height) if pic is not None else height)
    ctx.cursor.advance(_GAP)


def _hanging_indent(paragraph, marker: str, *, size_pt: float, level: int) -> None:
    """Hang a list item's wrapped lines under its text, not under its marker.

    The marker is an ordinary run here, so without this a second line starts at
    the box edge and reads as a new paragraph. The list level's own indent plus
    the marker's estimated width is where the text begins; the negative
    ``indent`` pulls the marker itself back out to the level's indent.
    """
    marker_width = estimate_text_width(marker, font_pt=size_pt)
    pPr = paragraph._pPr if paragraph._pPr is not None else paragraph._p.get_or_add_pPr()
    pPr.set("marL", str(_LIST_INDENT * level + marker_width))
    pPr.set("indent", str(-marker_width))


def _estimate_paras_height(paras: list[Para], width: int) -> int:
    total = 0
    for index, para in enumerate(paras):
        size = _para_size(para)
        # Estimate against the real text (CJK-aware), prefixing the list marker
        # so its width counts; indent eats into a bullet/ordered line's width.
        text = "".join(r.text for r in para.runs)
        if para.kind == "math":
            # A display equation is one centered line, but a fraction or a sum
            # with limits stacks well above it.
            lines = omml.visual_lines(para.runs[0].math or "")
            total += Emu(Pt(size * LINE_HEIGHT * lines + size * _PARA_GAP_EM))
            continue
        if para.kind in ("bullet", "ordered"):
            text = "•   " + text
        eff_width = width - _LIST_INDENT * (para.level + 1) if para.kind in ("bullet", "ordered") else width
        total += estimate_text_height(text, font_pt=size, width=max(eff_width, width // 4))
        total += Emu(Pt(size * _PARA_GAP_EM + _para_space_before(para, index)))
    return total or Emu(Pt(theme.BODY_PT))


# --- code ---


def _code(block, ctx: Ctx) -> None:
    code = str(block.content["content"]).strip("\n")
    lines = code.split("\n")
    # A marked line takes PowerPoint's own text highlight — the same wash a
    # `<mark>` gets here, at line scale.
    marked = set(block.content.get("mark") or ())
    tone = block.content.get("tone") or model.DEFAULT_HIGHLIGHT_TONE
    # Match the background to the actual code rows.  The previous ``n + 1``
    # estimate effectively reserved a blank line in every block, so short
    # listings looked especially loose.  Give each row the same exact leading
    # used elsewhere, plus a small and explicit vertical inset.
    vertical_pad_pt = 4
    height = Emu(Pt(
        len(lines) * theme.CODE_PT * LINE_HEIGHT + 2 * vertical_pad_pt
    ))
    box, tf = _textbox(ctx, height)
    _solid(box, theme.LT2)
    _no_line(box)
    tf.margin_left = Pt(10)
    tf.margin_top = tf.margin_bottom = Pt(vertical_pad_pt)
    first = True
    for number, line in enumerate(lines, start=1):
        p = tf.paragraphs[0] if first else tf.add_paragraph()
        first = False
        _set_line_spacing(p, theme.CODE_PT)
        _style_run(p.add_run(), line or " ", size_pt=theme.CODE_PT,
                   color=theme.FG, mono=True,
                   mark=tone if number in marked else None)
    top = ctx.cursor.place(height)
    # accent1 left bar (pre border-left).
    bar = _shape(ctx, MSO_SHAPE.RECTANGLE, ctx.layout.content_left, top,
                 Pt(4), height)
    _solid(bar, theme.ACCENT1)
    _no_line(bar)
    ctx.cursor.advance(_GAP)


# --- demo (a command, and what it printed) ---


def _demo(block, ctx: Ctx) -> None:
    """A `p.demo(...)` as a still transcript.

    Nothing here is runnable — a PPTX is a file on someone else's laptop — so
    the block prints what the deck prints when nobody presses the button: the
    name, then the command, then the output the author recorded. The box is the
    code block's, because on the slide it *is* the code block, and a handout
    that reflows every listing is a different handout.
    """
    from lecturekit import demo as demo_module

    content = block.content
    lines = demo_module.prompt_lines(str(content["command"]))
    output = content.get("output")
    outputs = str(output).strip("\n").split("\n") if output else []

    head = str(content["name"])
    if content.get("description"):
        head = f"{head} — {content['description']}"
    head_height = estimate_text_height(head, font_pt=theme.CAPTION_PT,
                                       width=ctx.layout.content_width)
    box, tf = _textbox(ctx, head_height)
    _set_line_spacing(tf.paragraphs[0], theme.CAPTION_PT)
    _apply_runs(tf.paragraphs[0], parse_markdown(head)[0].runs,
                size_pt=theme.CAPTION_PT, color=theme.DK2)
    for run in tf.paragraphs[0].runs:
        run.font.bold = True
    ctx.cursor.place(head_height)

    vertical_pad_pt = 4
    rows = lines + outputs
    height = Emu(Pt(len(rows) * theme.CODE_PT * LINE_HEIGHT + 2 * vertical_pad_pt))
    box, tf = _textbox(ctx, height)
    _solid(box, theme.LT2)
    _no_line(box)
    tf.margin_left = Pt(10)
    tf.margin_top = tf.margin_bottom = Pt(vertical_pad_pt)
    first = True
    for number, line in enumerate(rows, start=1):
        p = tf.paragraphs[0] if first else tf.add_paragraph()
        first = False
        _set_line_spacing(p, theme.CODE_PT)
        _style_run(p.add_run(), line or " ", size_pt=theme.CODE_PT, mono=True,
                   color=theme.ACCENT1 if number <= len(lines) else theme.FG)
    top = ctx.cursor.place(height)
    bar = _shape(ctx, MSO_SHAPE.RECTANGLE, ctx.layout.content_left, top,
                 Pt(4), height)
    _solid(bar, theme.ACCENT4)
    _no_line(bar)
    ctx.cursor.advance(_GAP)


# --- link ---


def _link(block, ctx: Ctx) -> None:
    label = str(block.content["label"])
    url = str(block.content["url"])
    height = estimate_text_height(label, font_pt=theme.BODY_PT,
                                  width=ctx.layout.content_width)
    box, tf = _textbox(ctx, height)
    p = tf.paragraphs[0]
    _style_run(p.add_run(), "•  ", size_pt=theme.BODY_PT, color=theme.ACCENT1, bold=True)
    run = p.add_run()
    _style_run(run, label, size_pt=theme.BODY_PT, color=theme.LINK)
    run.hyperlink.address = url
    ctx.cursor.place(height)
    ctx.cursor.advance(_GAP)


# --- aside (blockquote) ---


def _aside(block, ctx: Ctx) -> None:
    text = str(block.content).strip()
    height = estimate_text_height(text, font_pt=theme.BODY_PT,
                                  width=ctx.layout.content_width)
    box, tf = _textbox(ctx, height)
    tf.margin_left = Pt(12)
    _set_line_spacing(tf.paragraphs[0], theme.BODY_PT)
    _apply_runs(tf.paragraphs[0], parse_markdown(text)[0].runs if text else [Run("")],
                size_pt=theme.BODY_PT, color=theme.DK2)
    for run in tf.paragraphs[0].runs:
        run.font.italic = True
        run.font.color.rgb = theme.DK2
    top = ctx.cursor.place(height)
    bar = _shape(ctx, MSO_SHAPE.RECTANGLE, ctx.layout.content_left, top,
                 Pt(4), height)
    _solid(bar, theme.LT2)
    _no_line(bar)
    ctx.cursor.advance(_GAP)


# --- highlight (centered emphasis chip) ---


_HIGHLIGHT_PAD = Emu(Pt(10))     # matches the theme's 0.28em/0.8em chip padding
_HIGHLIGHT_RULE_PT = 1.0         # the theme's 1.5px frame, in points
# The theme's `box-shadow: 0 2px 8px rgba(13, 35, 50, 0.18)`, in points.
_HIGHLIGHT_SHADOW = {"alpha": 0.18, "blur_pt": 6.0, "dist_pt": 1.5}


def _highlight(block, ctx: Ctx) -> None:
    """A centered chip: a hairline frame in the tone, holding text in that tone.

    PowerPoint has no inline-block, so the width is *estimated* from the longest
    line (like every other box in this renderer) and centered in the content
    column. As in the deck the shape carries no fill — the tone is the outline
    and the ink, and what is inside the frame is white slide.
    """
    content = block.content
    lines = model.highlight_lines(content["text"])
    rows = [parse_markdown(line)[0].runs if line.strip() else [Run("")]
            for line in lines]
    size = theme.HIGHLIGHT_PT
    # Measured on the parsed runs, not the source: a `$$…$$` row is typeset as
    # an equation, so what the chip has to be wide enough for is the formula's
    # plain-text rendering, not the LaTeX that spelled it.
    width = min(
        ctx.layout.content_width,
        max(estimate_text_width("".join(run.text for run in runs), font_pt=size)
            for runs in rows)
        + 2 * _HIGHLIGHT_PAD,
    )
    height = Emu(Pt(len(lines) * size * LINE_HEIGHT)) + 2 * _HIGHLIGHT_PAD
    left = ctx.layout.content_left + (ctx.layout.content_width - width) // 2

    top = ctx.cursor.place(height)
    shape = _shape(ctx, MSO_SHAPE.RECTANGLE, left, top, width, height)
    shape.fill.background()
    _line(shape, theme.CHIP_INK[content["tone"]], _HIGHLIGHT_RULE_PT)
    _drop_shadow(shape, theme.SIDENOTE_BORDER, **_HIGHLIGHT_SHADOW)

    tf = shape.text_frame
    tf.word_wrap = False
    for i, runs in enumerate(rows):
        para = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
        para.alignment = PP_ALIGN.CENTER
        _set_line_spacing(para, size)
        _apply_runs(para, runs, size_pt=size,
                    color=theme.CHIP_INK[content["tone"]])
        for run in para.runs:
            run.font.bold = True
    ctx.cursor.advance(_GAP)


# --- sidenote (boxed callout) ---


_LOGO_SIZE = Emu(Pt(36))     # sidenote image-logo box (~2.4em at 18pt, like the theme)


def _sidenote(block, ctx: Ctx) -> None:
    note = block.content
    raw_logo = note.get("logo")
    title = f"{note['title']}："
    body = str(note["text"])
    fill = theme.SIDENOTE_WHEEL[ctx.sidenote_index % len(theme.SIDENOTE_WHEEL)]
    ctx.sidenote_index += 1

    # An image logo embeds as a picture in a left gutter (the theme floats it);
    # an emoji/glyph logo stays an inline text run. A path that won't resolve
    # falls back to the default book glyph rather than leaking the path as text.
    logo_path = _resolve_image(raw_logo, ctx) if _looks_like_image(raw_logo) else None
    logo_glyph = "" if logo_path is not None else (raw_logo or "📖")

    height = estimate_text_height(
        logo_glyph + title + body, font_pt=theme.SIDENOTE_PT,
        width=ctx.layout.content_width,
    ) + Emu(Pt(16))
    box_top = ctx.cursor.top
    shape = _shape(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, ctx.layout.content_left,
                   box_top, ctx.layout.content_width, height)
    _solid(shape, fill)
    _line(shape, theme.SIDENOTE_BORDER, 1.0)
    tf = shape.text_frame
    tf.word_wrap = True
    if logo_path is not None:
        tf.margin_left = _LOGO_SIZE + Pt(16)
    p = tf.paragraphs[0]
    _set_line_spacing(p, theme.SIDENOTE_PT)
    if logo_glyph:
        _style_run(p.add_run(), logo_glyph + " ", size_pt=theme.SIDENOTE_PT, color=theme.FG)
    title_run = p.add_run()
    _style_run(title_run, title, size_pt=theme.SIDENOTE_PT, color=theme.ACCENT1, bold=True)
    if note.get("link"):
        title_run.hyperlink.address = note["link"]
    _apply_runs(p, parse_inline(body), size_pt=theme.SIDENOTE_PT, color=theme.FG)

    if logo_path is not None:
        pic = ctx.slide.shapes.add_picture(
            str(logo_path), ctx.layout.content_left + Pt(10), box_top + Pt(8),
            width=_LOGO_SIZE,
        )
        # keep the logo inside the box even for a tall source image
        if pic.height > height - Pt(16):
            pic.width, pic.height = fit_within(pic.width, pic.height, _LOGO_SIZE, height - Pt(16))

    ctx.cursor.place(height)
    ctx.cursor.advance(_GAP)


# --- table (booktabs-ish) ---


def _table(block, ctx: Ctx) -> None:
    content = block.content
    headers = content["headers"]
    rows = content["rows"]
    align = content["align"]
    n_rows = len(rows) + 1
    n_cols = len(headers)
    row_h = Emu(Pt(theme.BODY_PT * 0.85 * 1.6))
    height = row_h * n_rows
    frame = ctx.slide.shapes.add_table(
        n_rows, n_cols, ctx.layout.content_left, ctx.cursor.top,
        ctx.layout.content_width, height,
    )
    table = frame.table
    table.first_row = True
    for c, header in enumerate(headers):
        _fill_cell(table.cell(0, c), str(header), bold=True, align=align, col=c)
    for r, row in enumerate(rows, start=1):
        for c, cell in enumerate(row):
            _fill_cell(table.cell(r, c), str(cell), bold=False, align=align, col=c)
    ctx.cursor.place(height)
    ctx.cursor.advance(_GAP)


_ALIGN = {"left": PP_ALIGN.LEFT, "center": PP_ALIGN.CENTER, "right": PP_ALIGN.RIGHT}


def _fill_cell(cell, text: str, *, bold: bool, align, col: int) -> None:
    # No fills (the theme's tables are booktabs-style — rules, not blocks).
    cell.fill.background()
    p = cell.text_frame.paragraphs[0]
    if align is not None:
        p.alignment = _ALIGN[align[col]]
    # A cell carries the same inline markdown as slide text — including $math$,
    # so a table of formulas is typeset, not printed as LaTeX source.
    runs = parse_inline(text) if text else [Run("")]
    _apply_runs(p, emphasize(runs, bold=bold), size_pt=theme.BODY_PT * 0.85,
                color=theme.FG)


# --- image / side_image ---


def _looks_like_image(logo: str | None) -> bool:
    """Whether a sidenote logo names an image (path/url) vs. an emoji/glyph."""
    if not logo:
        return False
    lowered = logo.lower()
    return (
        logo.startswith(("http://", "https://"))
        or "/" in logo
        or lowered.endswith(_EMBEDDABLE_EXTS)
    )


def _resolve_image(src: str, ctx: Ctx) -> Path | None:
    """The local file to embed for ``src``, rasterizing an SVG on the way."""
    if src.startswith(("http://", "https://")):
        return None
    if not src.lower().endswith(_EMBEDDABLE_EXTS):
        return None
    path = model.resolve_asset(src, ctx.asset_root, ctx.borrowed)
    if not path.exists():
        return None
    if path.suffix.lower() != ".svg":
        return path
    png = ctx.rasterizer.png(path) if ctx.rasterizer else None
    if png is None:
        _warn_svg_skipped(path, ctx)
    return png


def _warn_svg_skipped(path: Path, ctx: Ctx) -> None:
    reason = (
        f"no SVG renderer found — {rasterize.BACKEND_HINT}"
        if ctx.rasterizer is None or ctx.rasterizer.backend is None
        else f"{ctx.rasterizer.backend} could not convert it"
    )
    print(f"lecturekit: skipping {path} in PPTX: {reason}", file=sys.stderr)


def _image(block, ctx: Ctx) -> None:
    image = block.content
    path = _resolve_image(image["src"], ctx)
    if path is not None:
        pic = ctx.slide.shapes.add_picture(
            str(path), ctx.layout.content_left, ctx.cursor.top
        )
        # Fit the picture into the content width and the leftover vertical space
        # (the viewer's max-width/max-height:100%), reserving room for a caption.
        caption_reserve = Emu(Pt(theme.CAPTION_PT * 1.4)) + _GAP if image.get("caption") else 0
        max_h = max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining() - caption_reserve)
        requested = _requested_image_size(image, pic.width, pic.height, ctx.layout)
        if requested is not None:
            pic.width, pic.height = requested
        pic.width, pic.height = fit_within(
            pic.width, pic.height, ctx.layout.content_width, max_h
        )
        # center horizontally within the content box
        pic.left = ctx.layout.content_left + (ctx.layout.content_width - pic.width) // 2
        ctx.cursor.place(pic.height)
        ctx.cursor.advance(_GAP)
    if image.get("caption"):
        _caption(image["caption"], image.get("caption_align") or "center", ctx)


def _requested_image_size(image: dict, natural_w: int, natural_h: int,
                          layout: Layout) -> tuple[int, int] | None:
    """Resolve the DSL's explicit px/% image size while preserving aspect ratio."""
    target_w = _size_to_emu(image.get("width"), layout.width)
    target_h = _size_to_emu(image.get("height"), layout.height)
    if target_w is None and target_h is None:
        return None
    scales = []
    if target_w is not None:
        scales.append(target_w / natural_w)
    if target_h is not None:
        scales.append(target_h / natural_h)
    scale = min(scales)
    return round(natural_w * scale), round(natural_h * scale)


def _size_to_emu(value, whole: int) -> int | None:
    if value is None:
        return None
    if isinstance(value, int):
        return int(Emu(value * 914400 / 96))
    text = str(value).strip()
    if text.endswith("px"):
        return int(Emu(float(text[:-2]) * 914400 / 96))
    if text.endswith("%"):
        return round(whole * float(text[:-1]) / 100)
    return None


def _caption(text: str, align: str, ctx: Ctx) -> None:
    height = estimate_text_height(text, font_pt=theme.CAPTION_PT,
                                  width=ctx.layout.content_width)
    box, tf = _textbox(ctx, height)
    p = tf.paragraphs[0]
    p.alignment = _ALIGN.get(align, PP_ALIGN.CENTER)
    _style_run(p.add_run(), text, size_pt=theme.CAPTION_PT, color=theme.DK2, italic=True)
    ctx.cursor.place(height)
    ctx.cursor.advance(_GAP)


# --- architecture diagram (layered bands of module boxes) ---


# The theme's `.lk-arch*` metrics, in CSS pixels at the diagram's own size.
_ARCH_FIG_GAP = 6          # figure gap: between bands, and around a chevron
_ARCH_BAND_PAD_X = 12      # .lk-arch-layer padding
_ARCH_BAND_PAD_Y = 10
_ARCH_BAND_GAP = 12        # label gutter to module track
_ARCH_LABEL_W = 140        # .lk-arch-label flex basis
_ARCH_MOD_GAP = 8          # .lk-arch-modules gap
_ARCH_MOD_PAD_X = 10       # .lk-arch-module padding
_ARCH_MOD_PAD_Y = 8
_ARCH_MORE_PAD_X = 14      # the dashed "more modules" box is padded wider
_ARCH_BORDER_PT = 0.75     # the 1px hairline of a band and of a module box

# A diagram taller than the room left is drawn smaller, but only this far: past
# it the module labels stop being readable from the back of a lecture hall.
_ARCH_MIN_SCALE = 0.55


@dataclass
class _ArchMetrics:
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
    def at(cls, scale: float) -> "_ArchMetrics":
        return cls(
            size=theme.ARCH_PT * scale,
            fig_gap=px(_ARCH_FIG_GAP * scale),
            band_pad_x=px(_ARCH_BAND_PAD_X * scale),
            band_pad_y=px(_ARCH_BAND_PAD_Y * scale),
            band_gap=px(_ARCH_BAND_GAP * scale),
            label_w=px(_ARCH_LABEL_W * scale),
            mod_gap=px(_ARCH_MOD_GAP * scale),
            mod_pad_x=px(_ARCH_MOD_PAD_X * scale),
            mod_pad_y=px(_ARCH_MOD_PAD_Y * scale),
            more_pad_x=px(_ARCH_MORE_PAD_X * scale),
        )


def _arch_module_widths(modules: list[str], track: int,
                        m: _ArchMetrics) -> list[int]:
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


def _arch_layout(diagram: dict, ctx: Ctx,
                 m: _ArchMetrics) -> tuple[int, list[dict]]:
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
        widths = _arch_module_widths(
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


def redraw_architecture(block, ctx: Ctx, top: int, max_height: int) -> int:
    """Draw an architecture block again at ``top``, within ``max_height``.

    Blocks are drawn in one pass, so a diagram takes its natural size before the
    blocks below it exist. This is what the renderer calls when that left the
    page overflowing; it returns the height the diagram took the second time.
    """
    ctx.cursor.top = top
    _architecture(block, ctx, ceiling=max_height)
    return ctx.cursor.top - top


def _architecture(block, ctx: Ctx, *, ceiling: int | None = None) -> None:
    """Stacked layer bands, each a label gutter beside a row of module boxes.

    The viewer hands this to flexbox; here every width is computed from the
    content column and every height estimated from the text. A diagram that
    would not fit the room left on the page — or a ``ceiling`` the renderer
    imposes, once it knows what the blocks below it need — is measured again at
    a smaller scale rather than running off the slide.
    """
    diagram = block.content
    caption = diagram.get("caption")
    reserve = Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + _GAP if caption else 0
    m = _ArchMetrics.at(1.0)
    height, bands = _arch_layout(diagram, ctx, m)
    room = ctx.cursor.remaining() if ceiling is None else min(ceiling, ctx.cursor.remaining())
    budget = room - reserve
    if height > budget > 0:
        m = _ArchMetrics.at(max(_ARCH_MIN_SCALE, budget / height))
        height, bands = _arch_layout(diagram, ctx, m)

    glyph = model.ARCH_FLOW_GLYPH.get(diagram.get("flow"), "")
    top = ctx.cursor.place(height)
    for i, (layer, band) in enumerate(zip(diagram["layers"], bands)):
        if i:
            top += m.fig_gap
            if glyph:
                top += _arch_flow(glyph, top, m, ctx) + m.fig_gap
        _arch_band(layer, band, top, m, ctx)
        top += band["height"]
    ctx.cursor.advance(_GAP)
    if caption:
        _caption(caption, "center", ctx)


def _arch_flow(glyph: str, top: int, m: _ArchMetrics, ctx: Ctx) -> int:
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
    _style_run(p.add_run(), glyph, size_pt=m.size, color=theme.ACCENT1)
    return height


def _arch_band(layer: dict, band: dict, top: int, m: _ArchMetrics,
               ctx: Ctx) -> None:
    """One layer: the tinted band, its bold label, and its module boxes."""
    shape = _shape(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, ctx.layout.content_left,
                   top, ctx.layout.content_width, band["height"])
    _solid(shape, theme.LT2)
    _line(shape, theme.BOX_BORDER, _ARCH_BORDER_PT)
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
        _set_line_spacing(p, m.size)
        _style_run(p.add_run(), title, size_pt=m.size, color=theme.FG, bold=True)
        left += band["gutter"] + m.band_gap
    module_top = top + (band["height"] - band["module_height"]) // 2
    for module, width in zip(layer["modules"], band["widths"]):
        _arch_module(module, left, module_top, width, band["module_height"],
                     m, ctx)
        left += width + m.mod_gap


def _arch_module(text: str, left: int, top: int, width: int, height: int,
                 m: _ArchMetrics, ctx: Ctx) -> None:
    """One module box: white and solid, or faded and dashed for `…`."""
    more = text == model.ARCH_ELLIPSIS
    shape = _shape(ctx, MSO_SHAPE.ROUNDED_RECTANGLE, left, top, width, height)
    if more:
        shape.fill.background()
    else:
        _solid(shape, theme.BG)
    _line(shape, theme.BOX_BORDER, _ARCH_BORDER_PT)
    if more:
        shape.line.dash_style = MSO_LINE_DASH_STYLE.DASH
    tf = shape.text_frame
    tf.word_wrap = True
    tf.margin_left = tf.margin_right = m.mod_pad_x
    tf.margin_top = tf.margin_bottom = m.mod_pad_y
    tf.vertical_anchor = MSO_ANCHOR.MIDDLE
    p = tf.paragraphs[0]
    p.alignment = PP_ALIGN.CENTER
    _set_line_spacing(p, m.size)
    _style_run(p.add_run(), text, size_pt=m.size,
               color=theme.DK2 if more else theme.FG)


# --- row (side-by-side image figures) ---


def _row(block, ctx: Ctx) -> None:
    """A track of pictures side by side, each under its own share of the width.

    Widths follow the theme's flex bases: a sized item takes its width and the
    unsized ones split what is left. Items are top-aligned and the track is
    centered, so pictures of differing heights line up at their tops and each
    caption sits directly under its own picture.
    """
    content = block.content
    items = content["items"]
    gap = Emu(Pt(theme.BODY_PT * 1.2))     # .lk-row-track gap: 1.2em
    caption = content.get("caption")
    reserve = Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + _GAP if caption else 0
    max_height = max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining() - reserve)
    widths = _row_widths(items, ctx.layout.content_width - gap * (len(items) - 1),
                         ctx.layout)

    # Pictures land at their natural size, so each has to be added before its
    # final width is known; the track can only be centered once all are placed.
    top = ctx.cursor.top
    placed = []
    for item, width in zip(items, widths):
        path = _resolve_image(item["src"], ctx)
        if path is None:
            continue
        pic = ctx.slide.shapes.add_picture(str(path), ctx.layout.content_left, top)
        requested = _requested_image_size(item, pic.width, pic.height, ctx.layout)
        if requested is not None:
            pic.width, pic.height = requested
        item_reserve = (
            Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + _GAP
            if item.get("caption") else 0
        )
        pic.width, pic.height = fit_within(
            pic.width, pic.height, width, max(Emu(Pt(1)), max_height - item_reserve)
        )
        placed.append((pic, item))
    if not placed:
        return

    used = sum(pic.width for pic, _ in placed) + gap * (len(placed) - 1)
    left = ctx.layout.content_left + (ctx.layout.content_width - used) // 2
    height = 0
    for pic, item in placed:
        pic.left, pic.top = left, top
        left += pic.width + gap
        below = pic.height
        if item.get("caption"):
            below += _row_caption(item["caption"], pic, ctx)
        height = max(height, below)
    ctx.cursor.place(height)
    ctx.cursor.advance(_GAP)
    if caption:
        _caption(caption, "center", ctx)


def _row_widths(items: list[dict], track: int, layout: Layout) -> list[int]:
    """Each item's share of the track: sized items first, the rest split evenly."""
    sized = {}
    for i, item in enumerate(items):
        width = _size_to_emu(item.get("width"), layout.width)
        if width is not None:
            sized[i] = width
    shared = len(items) - len(sized)
    each = max(0, track - sum(sized.values())) // shared if shared else 0
    return [sized.get(i, each) for i in range(len(items))]


def _row_caption(text: str, pic, ctx: Ctx) -> int:
    """One item's caption, centered under its picture; returns the height used."""
    height = estimate_text_height(text, font_pt=theme.CAPTION_PT, width=pic.width)
    box = ctx.slide.shapes.add_textbox(
        pic.left, pic.top + pic.height + _GAP, pic.width, height
    )
    tf = box.text_frame
    tf.word_wrap = True
    tf.margin_left = tf.margin_right = tf.margin_top = tf.margin_bottom = 0
    p = tf.paragraphs[0]
    p.alignment = PP_ALIGN.CENTER
    _style_run(p.add_run(), text, size_pt=theme.CAPTION_PT, color=theme.DK2,
               italic=True)
    return _GAP + height


# --- footnotes (pinned bottom-left; drawn by the renderer after the blocks) ---


def draw_footnotes(texts: list[str], ctx: Ctx) -> None:
    if not texts:
        return
    size = theme.FOOTNOTE_PT
    height = Emu(Pt(len(texts) * size * 1.3 + 6))
    top = ctx.layout.content_bottom - height
    box = ctx.slide.shapes.add_textbox(
        ctx.layout.content_left, top, ctx.layout.content_width, height
    )
    tf = box.text_frame
    tf.word_wrap = True
    for i, text in enumerate(texts, 1):
        p = tf.paragraphs[0] if i == 1 else tf.add_paragraph()
        _style_run(p.add_run(), f"{i} ", size_pt=size, color=theme.ACCENT1)
        _apply_runs(p, parse_markdown(text)[0].runs, size_pt=size, color=theme.DK2)


_DRAWERS = {
    "slide": _slide,
    "code": _code,
    "demo": _demo,
    "link": _link,
    "aside": _aside,
    "highlight": _highlight,
    "sidenote": _sidenote,
    "table": _table,
    "image": _image,
    "side_image": _image,
    "architecture": _architecture,
    "row": _row,
}
