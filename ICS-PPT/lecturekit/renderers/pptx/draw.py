"""What the elements draw with: text, a document, and the parts of a document.

A document and an element share each part: a table in slide text is drawn by
what draws ``p.table``, a listing by what draws ``p.code``, a picture by what
draws ``p.image`` and ``p.row``, columns by what draws ``p.columns``. Each
``draw_*`` draws at the cursor and moves it past what it drew. The gap after a
block is its element's.
"""

from __future__ import annotations

from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import dataclass, replace

from pptx.dml.color import RGBColor
from pptx.enum.dml import MSO_FILL
from pptx.enum.shapes import MSO_SHAPE, MSO_SHAPE_TYPE
from pptx.enum.text import PP_ALIGN
from pptx.oxml.ns import qn
from pptx.util import Emu, Pt

from ...markup import document as doc
from . import omml, theme
from .blocks import (
    ALIGN, GAP, Ctx, add_picture, no_line, requested_image_size, resolve_image, row_widths, shape_at,
    natural_size, set_line_spacing, solid, style_run, textbox,
)
from .layout import LINE_HEIGHT, Cursor, estimate_text_height, estimate_text_width, fit_within
from .text import BREAK, BULLET, Para, Pictures, Run, cell, flow, plain, runs, split_lines

PARA_GAP_EM = 0.45          # space after each paragraph (~viewer p/li margins)
GROUP_GAP_EM = 0.55         # extra space before a numbered group heading
LIST_INDENT = Emu(Pt(18))   # approx indent per list level
COLUMN_GAP_EM = 2.0         # theme: `.lk-columns { gap: 2em }`
ROW_GAP_EM = 1.2            # theme: `.lk-row-track { gap: 1.2em }`
SMALL_EM = 0.8              # <small>
BASELINE = {1: "30000", -1: "-25000"}
CODE_PAD_PT = 4             # above and below a listing
CODE_INDENT_PT = 10
BAR = Pt(4)                 # the rule left of a listing and of a quote
QUOTE_INDENT = Emu(Pt(12))
CELL_PT = theme.BODY_PT * 0.85
CELL_MARGINS = Emu(Pt(14.4))    # PowerPoint's own, both sides
# Beside a picture that floats right (theme: 0.8em), and the most of the
# column it takes, so the text beside it keeps a readable measure.
FLOAT_GAP = Emu(Pt(16))
FLOAT_MAX_SHARE = 0.45
MARKER_PT = 12
MARKER_WIDTH = Emu(Pt(28))
# A formula taller than this many lines is given the room it needs.
TALL_LINES = 1.45


@dataclass(frozen=True)
class Look:
    """What text takes when its runs say nothing else."""

    size: float = theme.BODY_PT
    color: RGBColor = theme.FG
    bold: bool = False
    italic: bool = False
    mono: bool = False
    align: str | None = None


BODY = Look()
CAPTION = Look(size=theme.CAPTION_PT, color=theme.DK2, italic=True, align="center")


# --- runs and paragraphs ---


def write_runs(paragraph, items: list[Run], look: Look, *, display: bool = False) -> None:
    for item in items:
        if item.math is not None:
            node = omml.to_omml(item.math.math, size_pt=look.size, color=f"{look.color}",
                                display=display)
            # <a:endParaRPr> closes a paragraph; whatever is added goes before it.
            end = paragraph._p.find(qn("a:endParaRPr"))
            if end is None:
                paragraph._p.append(node)
            else:
                end.addprevious(node)
        elif item.text == BREAK:
            paragraph.add_line_break()
        else:
            _write_text(paragraph.add_run(), item, look)


def _write_text(run, item: Run, look: Look) -> None:
    color = look.color
    if item.link:
        color = theme.LINK
    elif look.mono:
        pass                    # a listing is in the colour of its line
    elif item.code:
        color = theme.ACCENT1
    elif item.bold and not look.bold:
        color = theme.DK2
    style_run(run, item.text,
              size_pt=look.size * (SMALL_EM if item.small else 1), color=color,
              bold=item.bold or look.bold, italic=item.italic or look.italic,
              mono=item.code or look.mono, mark=item.mark)
    if item.underline:
        run.font.underline = True
    if item.strike:
        run.font._rPr.set("strike", "sngStrike")
    if item.baseline:
        run.font._rPr.set("baseline", BASELINE[item.baseline])
    if item.link and not item.link.startswith("#"):
        run.hyperlink.address = item.link


def _tallest(items: list[Run]) -> float:
    return max((omml.visual_lines(run.math.math) for run in items if run.math), default=1.0)


def runs_height(items: list[Run], size: float, width: int) -> int:
    total = 0
    for line in split_lines(items):
        wrapped = estimate_text_height(plain(line), font_pt=size, width=max(1, width))
        total += max(wrapped, Emu(Pt(_tallest(line) * size * LINE_HEIGHT)))
    return total


def space_lines(paragraph, items: list[Run], size: float) -> None:
    """Exact spacing, unless a formula in the paragraph needs more than a line."""
    if _tallest(items) <= TALL_LINES:
        set_line_spacing(paragraph, size)


def _look_of(para: Para, look: Look) -> Look:
    if para.heading:
        return replace(look, size=theme.HEADING_PT[para.heading], bold=True)
    return look


def write_paragraph(paragraph, para: Para, look: Look) -> None:
    look = _look_of(para, look)
    align = "center" if para.display else para.align or look.align
    if align:
        paragraph.alignment = ALIGN[align]
    if para.level is not None:
        paragraph.level = min(para.level, 8)
        _hang(paragraph, para, look.size)
    if para.marker:
        style_run(paragraph.add_run(), para.marker, size_pt=look.size,
                  color=theme.ACCENT1, bold=True)
    write_runs(paragraph, para.runs, look, display=para.display and not para.marker)


def _hang(paragraph, para: Para, size: float) -> None:
    """Hang an item's lines under its text, not under its marker.

    The marker is an ordinary run here. The level's indent plus the marker's
    width is where the text begins, and the negative ``indent`` pulls the
    marker back out. An item's later paragraphs begin where its text does.
    """
    width = estimate_text_width(para.marker or BULLET, font_pt=size)
    properties = paragraph._p.get_or_add_pPr()
    properties.set("marL", str(LIST_INDENT * para.level + width))
    properties.set("indent", str(-width if para.marker else 0))


def _space_before(para: Para, index: int) -> float:
    """A numbered top-level item starts a group, with a gap above it."""
    grouped = index and para.level == 0 and para.marker[:1].isdigit()
    return theme.BODY_PT * GROUP_GAP_EM if grouped else 0.0


def fill_paras(frame, paras: list[Para], look: Look = BODY) -> None:
    for index, para in enumerate(paras):
        paragraph = frame.add_paragraph() if index else frame.paragraphs[0]
        size = _look_of(para, look).size
        write_paragraph(paragraph, para, look)
        if not para.display:
            space_lines(paragraph, para.runs, size)
        paragraph.space_after = Pt(size * PARA_GAP_EM)
        if before := _space_before(para, index):
            paragraph.space_before = Pt(before)


def paras_height(paras: list[Para], width: int, look: Look = BODY) -> int:
    total = 0
    for index, para in enumerate(paras):
        size = _look_of(para, look).size
        items, room = para.runs, width
        if para.level is not None:
            items = [Run(para.marker or BULLET), *items]
            room = max(width - LIST_INDENT * para.level, width // 4)
        total += runs_height(items, size, room)
        total += Emu(Pt(size * PARA_GAP_EM + _space_before(para, index)))
    return total


def draw_paras(ctx: Ctx, paras: list[Para], look: Look = BODY):
    height = paras_height(paras, ctx.layout.content_width, look)
    box, frame = textbox(ctx, height)
    fill_paras(frame, paras, look)
    ctx.cursor.place(height)
    return box


def draw_line(ctx: Ctx, items: list[Run], look: Look):
    """A field of one paragraph (a caption, a control) in a box of its own."""
    height = runs_height(items, look.size, ctx.layout.content_width)
    box, frame = textbox(ctx, height)
    paragraph = frame.paragraphs[0]
    if look.align:
        paragraph.alignment = ALIGN[look.align]
    space_lines(paragraph, items, look.size)
    write_runs(paragraph, items, look)
    ctx.cursor.place(height)
    return box


@contextmanager
def boxed(ctx: Ctx, left: int, width: int) -> Iterator[Cursor]:
    """Draw ``width`` wide from ``left``, from the cursor down."""
    saved = ctx.layout, ctx.cursor
    ctx.layout = replace(ctx.layout, content_left=left, content_width=width,
                         content_top=ctx.cursor.top)
    ctx.cursor = Cursor(ctx.layout)
    try:
        yield ctx.cursor
    finally:
        ctx.layout, ctx.cursor = saved


# --- a document and its parts ---


def draw_document(ctx: Ctx, blocks: list[doc.Node], look: Look = BODY) -> None:
    parts = flow(blocks)
    paras: list[Para] = []
    drawn = False

    def end_text() -> None:
        nonlocal drawn
        if paras:
            if drawn:
                ctx.cursor.advance(GAP)
            draw_paras(ctx, paras, look)
            paras.clear()
            drawn = True

    for index, part in enumerate(parts):
        if isinstance(part, Para):
            paras.append(part)
            continue
        end_text()
        if drawn:
            ctx.cursor.advance(GAP)
        match part:
            case Pictures(images):
                below = [later for later in parts[index + 1:] if isinstance(later, Para)]
                reserve = paras_height(below, ctx.layout.content_width, look) if below else 0
                draw_pictures(ctx, [vars(image) for image in images], reserve=reserve)
            case doc.Table():
                draw_table(ctx, part)
            case doc.Code(children):
                draw_code(ctx, split_lines(runs(children)))
            case doc.Columns(items):
                draw_columns(ctx, items, look=look)
            case doc.Quote(inner):
                draw_quote(ctx, inner, look)
        drawn = True
    end_text()


def draw_columns(ctx: Ctx, items: list[list[doc.Node]], shares: list[float] | None = None,
                 look: Look = BODY) -> None:
    """Documents side by side from one top. The tallest sets the height."""
    gap = Emu(Pt(look.size * COLUMN_GAP_EM))
    track = ctx.layout.content_width - gap * (len(items) - 1)
    left, top = ctx.layout.content_left, ctx.cursor.top
    tallest = 0
    for blocks, share in zip(items, shares or [1 / len(items)] * len(items)):
        width = max(1, round(track * share))
        with boxed(ctx, left, width) as cursor:
            draw_document(ctx, blocks, look)
        tallest = max(tallest, cursor.top - top)
        left += width + gap
    ctx.cursor.place(tallest)


def draw_quote(ctx: Ctx, blocks: list[doc.Node], look: Look = BODY) -> None:
    left, top = ctx.layout.content_left, ctx.cursor.top
    width = max(1, ctx.layout.content_width - QUOTE_INDENT)
    with boxed(ctx, left + QUOTE_INDENT, width) as cursor:
        draw_document(ctx, blocks, replace(look, color=theme.DK2, italic=True))
    height = cursor.top - top
    if not height:
        return
    bar = shape_at(ctx, MSO_SHAPE.RECTANGLE, left, top, BAR, height)
    solid(bar, theme.LT2)
    no_line(bar)
    ctx.cursor.place(height)


def draw_code(ctx: Ctx, lines: list[list[Run]], *, commands: int = 0,
              accent: RGBColor = theme.ACCENT1) -> None:
    """A listing. Its first ``commands`` lines are what was typed."""
    size = theme.CODE_PT
    height = Emu(Pt(len(lines) * size * LINE_HEIGHT + 2 * CODE_PAD_PT))
    box, frame = textbox(ctx, height)
    solid(box, theme.LT2)
    no_line(box)
    frame.margin_left = Pt(CODE_INDENT_PT)
    frame.margin_top = frame.margin_bottom = Pt(CODE_PAD_PT)
    for number, line in enumerate(lines):
        paragraph = frame.add_paragraph() if number else frame.paragraphs[0]
        set_line_spacing(paragraph, size)
        color = theme.ACCENT1 if number < commands else theme.FG
        write_runs(paragraph, line or [Run(" ")], Look(size=size, color=color, mono=True))
    top = ctx.cursor.place(height)
    bar = shape_at(ctx, MSO_SHAPE.RECTANGLE, ctx.layout.content_left, top, BAR, height)
    solid(bar, accent)
    no_line(bar)


def draw_table(ctx: Ctx, table: doc.Table, widths: list[float] | None = None) -> None:
    across = max(len(row) for row in table.rows)
    total = ctx.layout.content_width
    if widths:
        spans = [Emu(int(total * share)) for share in widths[:-1]]
    else:
        spans = [Emu(total // across)] * (across - 1)
    spans.append(Emu(total - sum(spans)))   # the last takes the rounding remainder
    read = [[cell(one.blocks) for one in row] for row in table.rows]

    least = Emu(Pt(CELL_PT * 1.6))
    heights = []
    for row in read:
        lines = 1
        for (paras, _), span in zip(row, spans):
            taken = sum(runs_height(para.runs, CELL_PT, span - CELL_MARGINS) for para in paras)
            lines = max(lines, round(taken / Pt(CELL_PT * LINE_HEIGHT)))
        heights.append(least + Emu(Pt((lines - 1) * CELL_PT * 1.2)))

    frame = ctx.slide.shapes.add_table(len(read), across, ctx.layout.content_left,
                                       ctx.cursor.top, total, sum(heights))
    grid = frame.table
    grid.first_row = any(one.header for one in table.rows[0])
    for column, span in zip(grid.columns, spans):
        column.width = span
    for r, height in enumerate(heights):
        grid.rows[r].height = height
        for c in range(across):
            target = grid.cell(r, c)
            # No fills: the theme's tables are rules, not blocks.
            target.fill.background()
            given = c < len(read[r])
            paras = read[r][c][0] if given else []
            look = Look(size=CELL_PT, bold=given and table.rows[r][c].header)
            body = target.text_frame
            for number, para in enumerate(paras or [Para([Run("")])]):
                paragraph = body.add_paragraph() if number else body.paragraphs[0]
                write_paragraph(paragraph, para, look)
    ctx.cursor.place(sum(heights))

    images = [vars(image) for row in read for _, found in row for image in found]
    if images:
        ctx.cursor.advance(GAP)
        draw_pictures(ctx, images)


def draw_caption(ctx: Ctx, text: str, align: str = "center"):
    return draw_line(ctx, runs(ctx.read.inline(text)), replace(CAPTION, align=align))


def draw_pictures(ctx: Ctx, items: list[dict], *, caption: str | None = None,
                  reserve: int = 0) -> None:
    """Pictures side by side, top-aligned, the track centred, each over its caption.

    An item says ``src`` and perhaps ``width``, ``height``, ``caption`` and
    ``caption_align``. A sized item takes its width and the others share the
    rest, as the theme's flex bases do. One picture has the whole width for
    its caption, several have each their own.
    """
    layout, top = ctx.layout, ctx.cursor.top
    gap = Emu(Pt(theme.BODY_PT * ROW_GAP_EM))
    caption_room = Emu(Pt(theme.CAPTION_PT * LINE_HEIGHT)) + GAP
    room = ctx.cursor.remaining() - reserve - (caption_room if caption else 0)
    alone = len(items) == 1
    shares = row_widths(items, layout.content_width - gap * (len(items) - 1), layout)

    sized = []
    for item, share in zip(items, shares):
        path = resolve_image(item["src"], ctx)
        if path is None:
            sized.append((item, None, 0, 0))
            continue
        width, height = natural_size(path)
        width, height = requested_image_size(item, width, height, layout) or (width, height)
        most = max(Emu(Pt(theme.BODY_PT)), room - (caption_room if item.get("caption") else 0))
        width, height = fit_within(
            width, height, layout.content_width if alone else max(1, share), most)
        sized.append((item, path, width, height))

    shown = [width for _, path, width, _ in sized if path is not None]
    left = layout.content_left + (layout.content_width - sum(shown)
                                  - gap * max(0, len(shown) - 1)) // 2
    tallest = 0
    for (item, path, width, height), share in zip(sized, shares):
        at, used = left, 0
        if path is not None:
            add_picture(ctx, path, item["src"], left, top, width, height)
            left += width + gap
            used = height
        if item.get("caption"):
            span = ((layout.content_left, layout.content_width) if alone
                    else (at, width or max(1, share)))
            with boxed(ctx, *span) as cursor:
                cursor.top = top + (used + GAP if used else 0)
                draw_caption(ctx, item["caption"], item.get("caption_align") or "center")
            used = cursor.top - top
        tallest = max(tallest, used)
    ctx.cursor.place(tallest)
    if caption:
        if tallest:
            ctx.cursor.advance(GAP)
        draw_caption(ctx, caption)


def draw_float(ctx: Ctx, image: dict, blocks: list[doc.Node]) -> None:
    """Slide text with ``image_right`` beside it.

    PowerPoint cannot wrap text around a shape, so the text box is narrowed by
    the picture. ``valign="middle"`` centres the two and, as in the deck, the
    picture then follows the text.
    """
    path = resolve_image(image["src"], ctx)
    if path is None:
        draw_document(ctx, blocks)
        return
    layout, top = ctx.layout, ctx.cursor.top
    width, height = natural_size(path)
    width, height = requested_image_size(image, width, height, layout) or (width, height)
    width, height = fit_within(width, height, round(layout.content_width * FLOAT_MAX_SHARE),
                               max(Emu(Pt(theme.BODY_PT)), ctx.cursor.remaining()))
    middle = image.get("valign") == "middle"

    def place():
        picture = add_picture(ctx, path, image["src"],
                              layout.content_left + layout.content_width - width, top, width, height)
        ctx.floated.append(picture._element)
        return picture

    picture = None if middle else place()
    first = len(ctx.slide.shapes)
    with boxed(ctx, layout.content_left,
               max(1, layout.content_width - width - FLOAT_GAP)) as cursor:
        draw_document(ctx, blocks)
    taken = cursor.top - top
    band = max(taken, height)
    if middle:
        for shape in list(ctx.slide.shapes)[first:]:
            shape.top += (band - taken) // 2
        picture = place()
        picture.top = top + (band - height) // 2
    ctx.cursor.place(band)


# --- one block, with its footnote marker ---


def draw_block(ctx: Ctx, block) -> list:
    """Draw ``block`` and its footnote marker; return the block's shapes."""
    first = len(ctx.slide.shapes)
    ctx.floated.clear()
    block.to_pptx(ctx)
    return finish_block(ctx, block, first)


def finish_block(ctx: Ctx, block, first: int) -> list:
    numbers = ctx.numbers.get(id(block))
    shapes = list(ctx.slide.shapes)[first:]
    if numbers:
        draw_marker(ctx, [s for s in shapes if s._element not in ctx.floated], numbers)
    return list(ctx.slide.shapes)[first:]


def _has_content(shape) -> bool:
    if shape.shape_type == MSO_SHAPE_TYPE.PICTURE or shape.has_table:
        return True
    return any((node.text or "").strip() for node in shape._element.iter(qn("a:t")))


def draw_marker(ctx: Ctx, shapes: list, numbers: tuple[int, ...]):
    """A block's footnote superscript, which ends the block and takes no height.

    It joins the last line when the block ends in text without a fill. After
    a picture, a table, a listing or a display formula it sits at the corner.
    """
    content = [shape for shape in shapes if _has_content(shape)]
    last = content[-1] if content else None
    if last is not None and last.has_text_frame and last.fill.type in (None, MSO_FILL.BACKGROUND):
        paragraph = last.text_frame.paragraphs[-1]
        if paragraph._p.find(f".//{{{omml.M}}}oMathPara") is None:
            size = next((run.font.size.pt for run in reversed(paragraph.runs) if run.font.size),
                        theme.BODY_PT)
            write_runs(paragraph, [Run(marker_text(numbers), baseline=1)], Look(size=size, color=theme.ACCENT1))
            return last
    if last is None:
        right, bottom = ctx.layout.content_left + ctx.layout.content_width, ctx.cursor.top
    else:
        right, bottom = last.left + last.width, last.top + last.height
    return marker_box(ctx, numbers, right, bottom)


def marker_text(numbers: tuple[int, ...]) -> str:
    return ", ".join(str(number) for number in numbers)


def marker_box(ctx: Ctx, numbers: tuple[int, ...], right: int, bottom: int):
    height = Emu(Pt(MARKER_PT * LINE_HEIGHT))
    box = ctx.slide.shapes.add_textbox(right - MARKER_WIDTH, bottom - height,
                                       MARKER_WIDTH, height)
    frame = box.text_frame
    frame.margin_left = frame.margin_right = frame.margin_top = frame.margin_bottom = 0
    paragraph = frame.paragraphs[0]
    paragraph.alignment = PP_ALIGN.RIGHT
    style_run(paragraph.add_run(), marker_text(numbers), size_pt=MARKER_PT, color=theme.ACCENT1)
    return box
