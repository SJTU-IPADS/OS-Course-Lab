"""Helpers the elements' ``to_marp`` methods render with.

Each element class (``lecturekit.elements``) says how it reads in the deck;
this module holds the escaping, footnote markers and side column several of
them share, and :func:`render_block`, which asks a block to draw itself.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

from lecturekit import filelinks, model, pseudo


def escape_html(text: str) -> str:
    """Escape ``text`` for HTML text and for a double-quoted attribute."""
    return (
        str(text)
        .replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace('"', "&quot;")
    )


# Inline HTML tags allowed verbatim inside a sidenote body. Everything is
# escaped first; these are reintroduced afterwards so untrusted-looking input
# can't smuggle in arbitrary markup.
INLINE_TAGS = "u|b|i|em|strong|code|sup|sub|br"
TAG_RE = re.compile(rf"&lt;(/?(?:{INLINE_TAGS})\s*/?)&gt;")
CODE_RE = re.compile(r"`([^`]+)`")
LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\s]+)\)")
BOLD_RE = re.compile(r"\*\*(.+?)\*\*")
ITALIC_RE = re.compile(r"\*([^*]+?)\*")


def inline_md(text: str) -> str:
    """Render a small subset of inline markdown inside a sidenote body.

    Marp does not parse markdown within raw HTML blocks, and a sidenote is
    emitted as raw HTML, so its body would otherwise show ``**bold**`` and
    ``<u>`` literally. Escape first, then reintroduce whitelisted inline tags,
    ``**bold**``/``*italic*``/``` `code` ```, ``[label](url)`` links, and turn
    newlines into line breaks.
    """
    html = escape_html(text)
    html = TAG_RE.sub(r"<\1>", html)
    html = CODE_RE.sub(r"<code>\1</code>", html)
    html = LINK_RE.sub(
        r'<a href="\2" target="_blank" rel="noopener noreferrer">\1</a>', html
    )
    html = BOLD_RE.sub(r"<strong>\1</strong>", html)
    html = ITALIC_RE.sub(r"<em>\1</em>", html)
    return html.replace("\n", "<br>")


def sidenote_logo_html(logo: str | None) -> str:
    if logo is None:
        return '<span class="sidenote-logo">📖</span>'
    if model.logo_is_image(logo):
        return f'<img class="sidenote-logo" src="{escape_html(logo)}" alt="" />'
    return f'<span class="sidenote-logo">{escape_html(logo)}</span>'


def sidenote_classes(note: dict) -> str:
    classes = ["sidenote"]
    if "\n" not in str(note["title"]) and "\n" not in str(note["text"]):
        classes.append("sidenote--single-line")
    return " ".join(classes)


def float_image_html(image: dict) -> str:
    """A right-floated <img> emitted before a slide's text so the text wraps it.

    Marp's --html mode passes this through; absent width/height are omitted. The
    margin is a fixed cosmetic gutter between the image and the wrapped text.
    Under ``valign="middle"`` the image is a flex item after the text instead
    of a float, so it keeps only the gutter on its left.
    """
    middle = image.get("valign") == "middle"
    decls = [] if middle else ["float: right"]
    if image.get("width") is not None:
        decls.append(f"width: {image['width']}")
    if image.get("height") is not None:
        decls.append(f"height: {image['height']}")
    decls.append("margin: 0 0 0 1em" if middle else "margin: 0 0 .4em 1em")
    style = "; ".join(decls)
    src = escape_html(image["src"])
    alt = escape_html(image.get("alt") or "")
    return f'<img src="{src}" alt="{alt}" style="{style};" />'


def file_button(label: str, path: str) -> str:
    """One ``[label](file:path)`` as the button the live preview arms.

    The markup of a demo block's file button, for the same reasons: an id and
    not a path, and ``disabled`` until ``source.js`` has a server behind it. It
    sits in a line markdown-it still parses, so its label is rendered here and
    made inert on the way out.
    """
    from lecturekit import source as source_module

    return (
        '<button class="lk-text-file" type="button" disabled'
        f' data-lk-source="{source_module.source_id(path)}"'
        f' aria-label="Show {inert_md(escape_html(path))}">'
        f"{inert_md(inline_md(label))}</button>"
    )


def file_buttons(text: str) -> str:
    """Markdown with each file link drawn as its button. See `lecturekit.filelinks`."""
    return filelinks.replace(text, file_button)


# `pseudo` token kind -> the class suffix its <span> carries.
PSEUDO_CLASSES = {
    "keyword": "kw", "message": "msg", "state": "st", "comment": "cm",
}


def pseudo_html(content: str, marked: set[int], tone: str) -> str:
    """Lecture pseudocode as a pre-highlighted ``<pre>``.

    Marp highlights a fenced block with highlight.js, which knows no such
    language — so `pseudo` is lexed here (see `lecturekit.pseudo`) and handed
    to Marp as raw HTML. A `<pre>` opens a CommonMark raw-HTML block that runs
    until its closing tag, so the body passes through untouched, blank lines
    and leading indentation included.

    A line in `marked` is wrapped in one inline span, so the wash ends where the
    code does — a marker pen over the line, not a band across the block. The
    newlines stay outside the spans, which is what keeps an unmarked block's
    output byte-for-byte what it always was.
    """
    out = []
    for number, tokens in enumerate(pseudo.tokenize(content), start=1):
        line = "".join(
            f'<span class="tok-{cls}">{escape_html(text)}</span>' if cls else escape_html(text)
            for cls, text in (
                (PSEUDO_CLASSES.get(kind), text) for kind, text in tokens
            )
        )
        if number in marked:
            line = f'<span class="lk-codemark lk-codemark--{tone}">{line}</span>'
        out.append(line)
    return '<pre class="lk-pseudo"><code>%s</code></pre>' % "\n".join(out)


def css_length(value) -> str:
    """A unit-bearing CSS length string, treating a bare number as pixels."""
    return value if isinstance(value, str) else f"{value}px"


def figure_inner(image: dict) -> tuple[str, str]:
    """Build ``(img_html, caption_html)`` for one image dict.

    Shared by a standalone image (`_image`) and a row item (`_row`) so both
    render the <img> and caption identically. Caption runs through `inline_md`
    and honors `caption_align`; an absent caption yields an empty string.
    """
    sizes = []
    if image.get("width") is not None:
        sizes.append(f"width:{css_length(image['width'])}")
    if image.get("height") is not None:
        sizes.append(f"height:{css_length(image['height'])}")
    style = f' style="{";".join(sizes)}"' if sizes else ""
    img = (
        f'<img src="{escape_html(image["src"])}" '
        f'alt="{escape_html(image.get("alt") or "")}"{style}>'
    )
    caption = ""
    if image.get("caption"):
        align = image.get("caption_align") or "center"
        align_style = f' style="text-align:{align}"' if align != "center" else ""
        caption = (
            f"<figcaption{align_style}>"
            f"{inline_md(image['caption'])}</figcaption>"
        )
    return img, caption


def figure_classes(image: dict) -> str:
    """The lk-figure class list for an image: framed/sized modifiers."""
    classes = "lk-figure"
    if image.get("framed"):
        classes += " lk-figure--framed"
    if image.get("width") is not None or image.get("height") is not None:
        classes += " lk-figure--sized"
    return classes


def _side_image(block):
    # Drawn once per page by `side_column`, under the full-width title.
    return []


# Marp's background keywords that ask for the whole image, uncropped.
SIDE_FIT = frozenset({"contain", "fit"})
SIDE_KEYWORDS = SIDE_FIT | {"cover"}


def side_width_px(width, slide_width: int) -> int:
    """A ``side_image`` column's width in pixels on a slide ``slide_width`` wide.

    ``width`` is a percentage of the slide (``"40%"``), a pixel count
    (``"480px"`` or a bare number), or ``None`` for half the slide.
    """
    if isinstance(width, (int, float)):
        return round(width)
    text = str(width or "50%").strip()
    try:
        if text.endswith("%"):
            return round(slide_width * float(text[:-1]) / 100)
        return round(float(text.removesuffix("px")))
    except ValueError:
        return slide_width // 2


def side_column(
    sides: list, slide_width: int, texts: list = (), marks: tuple[int, ...] = ()
) -> list[str]:
    """The image column of a page with ``side_image`` blocks, on either side.

    Marp's split background moves the whole slide into the other column, the
    title with it: the title leaves the slide's edge on the left, and its rule
    stops at the column on either side. So the column is drawn by the theme
    instead (``section.lk-side-left`` / ``section.lk-side-right``): the title
    keeps the slide's width, and the images share one column that starts
    under it. The scoped style hands the theme that column's width, set by
    the first image.

    ``texts`` is the rendered markdown of the slide blocks set under the image
    (``under_side_image``), one list of lines per block. With any, the column
    is a stack (``lk-side-stack``): the images across its top at the column's
    width, the text below. Each text sits between blank lines so Marp parses it
    as markdown (an HTML block ends at a blank line).

    ``marks`` are the footnote numbers the images carry; their superscript is
    drawn at the column's inner top corner, next to the picture.
    """
    width = side_width_px(sides[0].content.get("width"), slide_width)
    images = []
    for block in sides:
        words = str(block.content.get("alt") or "").split()
        fit = ' class="lk-side-fit"' if SIDE_FIT & set(words) else ""
        alt = " ".join(word for word in words if word not in SIDE_KEYWORDS)
        images.append(
            f'<img src="{escape_html(block.content["src"])}" '
            f'alt="{escape_html(alt)}"{fit}>'
        )
    style = ["<style scoped>", f"section {{ --lk-side-width: {width}px; }}", "</style>", ""]
    if marks:
        images.append(marker_html(marks).replace(
            'class="footnote-ref"', 'class="footnote-ref footnote-ref--side"'
        ))
    if not texts:
        return [f'<figure class="lk-side">{"".join(images)}</figure>', "", *style]
    lines = [
        '<div class="lk-side lk-side-stack">',
        f'<div class="lk-side-images">{"".join(images)}</div>',
        "",
    ]
    for text in texts:
        lines += [*text, ""]
    return [*lines, "</div>", "", *style]


ALIGN_DELIMITERS = {"left": ":---", "center": ":---:", "right": "---:"}


def table_cell(text: str) -> str:
    # Keep cell markdown intact for Marp; only neutralize the two characters
    # that would break the single-line row syntax.
    return file_buttons(str(text)).replace("|", "\\|").replace("\n", " ")


def fixed_table(lines: list[str], widths: list[float]) -> list[str]:
    """Wrap GFM table lines so the table spans the slide at given column shares.

    A GFM table carries no widths, so the table goes in a div whose class names
    the percentages, and a <style> rule keyed on that class sets them (Marp
    honors rules in a <style> block; see the annotation bubbles below). The
    class is derived from the widths, so tables sharing widths share one rule
    and no deck-wide counter is needed. The <style> comes first: a footnote
    marker is tucked after the last table row, which must stay the last line
    with text in it. Blank lines keep Marp parsing the rows as markdown.
    """
    percents = [f"{100 * w:.2f}".rstrip("0").rstrip(".") for w in widths]
    cls = "lk-cols-" + "-".join(pc.replace(".", "_") for pc in percents)
    rules = [
        f"section .{cls} {{ align-self: stretch; }}",
        f"section .{cls} > table {{ table-layout: fixed; width: 100%; }}",
    ] + [
        f"section .{cls} > table th:nth-child({i}) {{ width: {pc}%; }}"
        for i, pc in enumerate(percents, start=1)
    ]
    return [
        "<style>", *rules, "</style>", "",
        f'<div class="lk-table {cls}">', "", *lines, "", "</div>", "",
    ]


SIDENOTE_WHEEL_SIZE = 6


def anchor_axes(at: str) -> tuple[str, str]:
    """Split an anchor name into (horizontal, vertical), each start/center/end."""
    horizontal, vertical = "center", "center"
    for part in at.split("-"):
        if part in ("left", "right"):
            horizontal = part
        elif part in ("top", "bottom"):
            vertical = part
    return horizontal, vertical


# Bubbles are placed with pixel left/top from the slide's top-left origin (scaled
# by the known slide size) plus an element-relative translate that aligns the
# bubble's matching edge to that point. The fraction of the slide per axis end:
# start=0, center=0.5, end=1.0; the matching self-translate is 0 / -50% / -100%.
# Positioning is emitted as a generated CSS *rule* (keyed by a unique class), not
# the aside's inline style: Marp strips positioning out of inline `style`
# attributes, but rules in a <style> block are honored — as the theme's working
# `.footnotes` rule shows.
AXIS = {"start": (0.0, 0), "center": (0.5, -50), "end": (1.0, -100)}


def annotation_decls(note, width: int, height: int) -> str:
    horizontal, vertical = anchor_axes(note.at)
    h = {"left": "start", "right": "end"}.get(horizontal, "center")
    v = {"top": "start", "bottom": "end"}.get(vertical, "center")
    hf, tx = AXIS[h]
    vf, ty = AXIS[v]
    left = round(width * hf) + note.dx
    top = round(height * vf) + note.dy
    return f"left:{left}px; top:{top}px; transform: translate({tx}%, {ty}%);"


def annotation_markup(note, uid: str, width: int, height: int) -> tuple[str, str]:
    """Return ``(aside_html, css_rule)`` for one annotation bubble.

    ``uid`` is a deck-unique token that ties the aside's ``ann-<uid>`` class to
    the returned rule. A bottom-anchored bubble sits at the slide's lower edge,
    so its tail flips up (toward content above) via ``annotation--tail-up``.
    """
    _, vertical = anchor_axes(note.at)
    extra = " annotation--tail-up" if vertical == "bottom" else ""
    aside = (
        f'<aside class="annotation ann-{uid}{extra}">'
        f"{inline_md(note.text)}</aside>"
    )
    rule = f"section .ann-{uid} {{ {annotation_decls(note, width, height)} }}"
    return aside, rule


# Math inside a chip: `$$…$$` (a whole row, set in display style) or `$…$`.
CHIP_MATH_RE = re.compile(r"\$\$(?P<display>.+?)\$\$|\$(?P<inline>[^$\n]+)\$")

# The characters markdown still reads as markup in text that is already HTML.
MD_INERT = {"*": "&#42;", "_": "&#95;", "`": "&#96;", "\\": "&#92;"}
MD_ACTIVE_RE = re.compile("[*_`\\\\]")


def inert_md(html: str) -> str:
    """Neutralize what markdown would still read as markup in rendered HTML.

    For HTML that is emitted on a markdown line rather than as a raw HTML block
    — the math form of the chip, a file button in a sentence — where markdown-it
    reads the text a second time, and a stray `_` in it is not emphasis the
    author asked for. Entities are the safe spelling — they survive inside an
    attribute (a link's href) as well as in text.
    """
    return MD_ACTIVE_RE.sub(lambda m: MD_INERT[m.group()], html)


def chip_math_md(line: str) -> str:
    """One chip row as markdown: the formulas verbatim, everything else inert.

    Marp typesets `$…$` while parsing markdown and never looks inside a raw HTML
    block, so a formula has to reach it as markdown source — which means it must
    not be escaped on the way (`&lt;` is not what MathJax should typeset), and
    the prose around it, already rendered to HTML, must not be read as markdown.
    A `$$…$$` row becomes `\\displaystyle` inline math: the chip is a line of a
    slide, so a display formula belongs *in* it rather than in a block of its
    own, and only the style has to carry over.
    """
    out: list[str] = []
    pos = 0
    for match in CHIP_MATH_RE.finditer(line):
        if match.start() > pos:
            out.append(inert_md(inline_md(line[pos:match.start()])))
        display = match.group("display")
        latex = display if display is not None else match.group("inline")
        prefix = "\\displaystyle " if display is not None else ""
        out.append(f"${prefix}{latex.strip()}$")
        pos = match.end()
    if pos < len(line):
        out.append(inert_md(inline_md(line[pos:])))
    return "".join(out)


def demo_output(output, bold):
    """The recorded output, escaped, with the lines ``bold=`` names in <strong>.

    The numbers count the output with its leading blank lines stripped (see
    ``Demo.check_content``); those lines stay in the text, so a demo without
    ``bold`` renders exactly as it did before the option existed.
    """
    if not bold:
        return escape_html(output)
    skip = len(output) - len(output.lstrip("\n"))
    wanted = {number + skip - 1 for number in bold}
    return "\n".join(
        f"<strong>{escape_html(line)}</strong>" if index in wanted else escape_html(line)
        for index, line in enumerate(output.split("\n"))
    )


def demo_files(files):
    """The file buttons of a demo block, or "" when it named none."""
    from lecturekit import source as source_module

    parts = []
    for path in source_module.normalize(files):
        parts.append(
            f'<button class="lk-demo-file" type="button" disabled'
            f' data-lk-source="{source_module.source_id(path)}"'
            f' aria-label="Show {escape_html(path)}">'
            f"{escape_html(source_module.label(path))}</button>"
        )
    return "".join(parts)


@dataclass
class MarpCtx:
    """What a block needs to know about the page it is drawn in."""
    # How many sidenotes the page drew before this block; the sidenote picks
    # its position in the margin column from it, and counts itself in.
    sidenote_index: int = 0


def render_block(
    block: model.Block,
    sidenote_index: int = 0,
    footnote_numbers: tuple[int, ...] = (),
    ctx: MarpCtx | None = None,
) -> list[str]:
    """The Marp lines of one block, with its footnote superscript attached.

    A page passes one ``ctx`` for all its blocks; a lone block gets its own.
    """
    lines = block.to_marp(ctx or MarpCtx(sidenote_index))
    if footnote_numbers and lines:
        lines = block.footnote_marker(lines, footnote_numbers)
    return lines


LONE_TAG_RE = re.compile(r"<[^>]+>")


def last_content_index(lines: list[str]) -> int:
    """Index of the last line a footnote marker can attach to inline.

    Skips trailing blank lines (renderers leave a trailing '') and lines that
    are a single HTML tag on their own — a float image's ``<img>`` or its
    wrapping ``<div>``/``</div>`` — so the marker lands on the slide's text, not
    on a structural tag. A rendered block whose body is one HTML element with
    inner text (e.g. a sidenote ``<aside>…</aside>``) is not a lone tag, so it
    still receives the marker inline as before.
    """
    for i in range(len(lines) - 1, -1, -1):
        line = lines[i].strip()
        if line and not LONE_TAG_RE.fullmatch(line):
            return i
    return len(lines) - 1


def marker_html(numbers: tuple[int, ...]) -> str:
    return f'<sup class="footnote-ref">{", ".join(str(n) for n in numbers)}</sup>'


def place_figure_marker(figure_html: str, marker: str, *, inside_image: bool) -> str:
    """Tuck a footnote marker inside a ``<figure>`` so it claims no extra line.

    A figure with its own trailing ``<figcaption>`` gets the marker on that
    caption. A captionless single image gets a corner superscript on an
    inline-block anchor wrapping the ``<img>``, so it rides the *image's*
    top-right corner (the image is centered and usually narrower than the
    figure). A captionless row or architecture diagram has no single image to
    hug, so the corner marker pins to the figure box instead.
    """
    if figure_html.endswith("</figcaption></figure>"):
        head = figure_html[: -len("</figcaption></figure>")]
        return f"{head} {marker}</figcaption></figure>"
    corner = marker.replace(
        'class="footnote-ref"', 'class="footnote-ref footnote-ref--corner"'
    )
    if inside_image:
        open_end = figure_html.index(">") + 1
        open_tag, body = figure_html[:open_end], figure_html[open_end:]
        img = body[: -len("</figure>")]
        return (
            f'{open_tag}<span class="lk-figure-anchor">{img}{corner}</span></figure>'
        )
    inner = figure_html[: -len("</figure>")]
    return f"{inner}{corner}</figure>"


def mark_text_end(lines: list[str], numbers: tuple[int, ...]) -> list[str]:
    """Text blocks: the marker rides the end of the last line.

    It is glued on with a word joiner and laid out at zero width (`--hang`), so
    a line that already fills the column cannot break before it and drop a lone
    number onto the next line — it hangs into the slide's right padding
    instead.
    """
    lines = list(lines)
    idx = last_content_index(lines)
    hang = marker_html(numbers).replace(
        'class="footnote-ref"', 'class="footnote-ref footnote-ref--hang"'
    )
    lines[idx] = f"{lines[idx]}&#8288;{hang}"
    return lines


def mark_figure(
    lines: list[str], numbers: tuple[int, ...], *, inside_image: bool = False
) -> list[str]:
    """A block rendered as one raw-HTML ``<figure>`` line (image, row,
    architecture). A marker appended *after* ``</figure>`` becomes its own Marp
    paragraph — a wasted line — so it is tucked *inside* the figure."""
    lines = list(lines)
    idx = last_content_index(lines)
    lines[idx] = place_figure_marker(
        lines[idx], marker_html(numbers), inside_image=inside_image
    )
    return lines


def mark_tucked(lines: list[str], numbers: tuple[int, ...]) -> list[str]:
    """Code and tables: the last content line is a syntactic row (closing ```
    fence or a `| … |` table row); appending inline would break it, so the
    marker is emitted just after the block. A zero-height, right-aligned
    wrapper parks it on the block's bottom-right corner, so — as on a figure —
    it claims no line of its own."""
    lines = list(lines)
    idx = last_content_index(lines)
    lines.insert(idx + 1, f'<div class="footnote-ref-tuck">{marker_html(numbers)}</div>')
    return lines


def mark_highlight(lines: list[str], numbers: tuple[int, ...]) -> list[str]:
    """One element on one line; a marker appended after it would become its own
    Marp paragraph. Tuck it inside, just outside the chip's <span> — so after
    the inner </span>, before the outer tag closes. That tag is a <p>, or a
    <span> on the math form of the chip (see `Highlight.to_marp`)."""
    lines = list(lines)
    idx = last_content_index(lines)
    tail = "</span>" if lines[idx].endswith("</span></span>") else "</p>"
    head = lines[idx][: -len(tail)]
    lines[idx] = f"{head} {marker_html(numbers)}{tail}"
    return lines


def render_footnotes(texts: list[str]) -> str:
    """A bottom-left aside listing a page's footnotes, numbered to match markers."""
    items = "".join(
        f'<p class="footnote-item">'
        f'<sup class="footnote-ref">{i}</sup> {inline_md(text)}</p>'
        for i, text in enumerate(texts, 1)
    )
    return f'<aside class="footnotes">{items}</aside>'
