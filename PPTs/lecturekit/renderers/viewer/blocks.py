from __future__ import annotations

import re

from lecturekit import model, pseudo

_IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp")


def _escape(text: str) -> str:
    return (
        str(text)
        .replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace('"', "&quot;")
    )


# Inline HTML tags allowed verbatim inside a sidenote body. Everything is
# escaped first; these are reintroduced afterwards so untrusted-looking input
# can't smuggle in arbitrary markup.
_INLINE_TAGS = "u|b|i|em|strong|code|sup|sub|br"
_TAG_RE = re.compile(rf"&lt;(/?(?:{_INLINE_TAGS})\s*/?)&gt;")
_CODE_RE = re.compile(r"`([^`]+)`")
_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\s]+)\)")
_BOLD_RE = re.compile(r"\*\*(.+?)\*\*")
_ITALIC_RE = re.compile(r"\*([^*]+?)\*")


def _inline_md(text: str) -> str:
    """Render a small subset of inline markdown inside a sidenote body.

    Marp does not parse markdown within raw HTML blocks, and a sidenote is
    emitted as raw HTML, so its body would otherwise show ``**bold**`` and
    ``<u>`` literally. Escape first, then reintroduce whitelisted inline tags,
    ``**bold**``/``*italic*``/``` `code` ```, ``[label](url)`` links, and turn
    newlines into line breaks.
    """
    html = _escape(text)
    html = _TAG_RE.sub(r"<\1>", html)
    html = _CODE_RE.sub(r"<code>\1</code>", html)
    html = _LINK_RE.sub(
        r'<a href="\2" target="_blank" rel="noopener noreferrer">\1</a>', html
    )
    html = _BOLD_RE.sub(r"<strong>\1</strong>", html)
    html = _ITALIC_RE.sub(r"<em>\1</em>", html)
    return html.replace("\n", "<br>")


def _looks_like_image(logo: str) -> bool:
    lowered = logo.lower()
    return (
        logo.startswith(("http://", "https://"))
        or "/" in logo
        or lowered.endswith(_IMAGE_EXTENSIONS)
    )


def _sidenote_logo_html(logo: str | None) -> str:
    if logo is None:
        return '<span class="sidenote-logo">📖</span>'
    if _looks_like_image(logo):
        return f'<img class="sidenote-logo" src="{_escape(logo)}" alt="" />'
    return f'<span class="sidenote-logo">{_escape(logo)}</span>'


def _sidenote_classes(note: dict) -> str:
    classes = ["sidenote"]
    if "\n" not in str(note["title"]) and "\n" not in str(note["text"]):
        classes.append("sidenote--single-line")
    return " ".join(classes)


def _float_image_html(image: dict) -> str:
    """A right-floated <img> emitted before a slide's text so the text wraps it.

    Marp's --html mode passes this through; absent width/height are omitted. The
    margin is a fixed cosmetic gutter between the image and the wrapped text.
    """
    decls = ["float: right"]
    if image.get("width") is not None:
        decls.append(f"width: {image['width']}")
    if image.get("height") is not None:
        decls.append(f"height: {image['height']}")
    decls.append("margin: 0 0 .4em 1em")
    style = "; ".join(decls)
    src = _escape(image["src"])
    alt = _escape(image.get("alt") or "")
    return f'<img src="{src}" alt="{alt}" style="{style};" />'


def _slide(block):
    content = str(block.content).strip()
    image = block.float_image
    if image:
        # The theme lays the slide body out as a flex column, where a bare
        # floated <img> becomes a flex item and `float` is ignored (it would
        # stack above the text). Wrapping the image and the text in one
        # block-flow container — `.lk-float`, a `flow-root` in the theme —
        # restores normal flow so the text wraps to the image's left. The float
        # must precede the text in document order; blank lines around the inner
        # content keep Marp parsing it as markdown rather than raw HTML.
        return [
            '<div class="lk-float">',
            "",
            _float_image_html(image),
            "",
            content,
            "",
            "</div>",
            "",
        ]
    return [content, ""]


def _cover(block):
    return []


# `pseudo` token kind -> the class suffix its <span> carries.
_PSEUDO_CLASSES = {
    "keyword": "kw", "message": "msg", "state": "st", "comment": "cm",
}


def _pseudo_html(content: str, marked: set[int], tone: str) -> str:
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
            f'<span class="tok-{cls}">{_escape(text)}</span>' if cls else _escape(text)
            for cls, text in (
                (_PSEUDO_CLASSES.get(kind), text) for kind, text in tokens
            )
        )
        if number in marked:
            line = f'<span class="lk-codemark lk-codemark--{tone}">{line}</span>'
        out.append(line)
    return '<pre class="lk-pseudo"><code>%s</code></pre>' % "\n".join(out)


def _code(block):
    language = block.content["language"]
    content = block.content["content"].strip("\n")
    if language == "pseudo":
        marked = set(block.content.get("mark") or ())
        tone = block.content.get("tone") or model.DEFAULT_HIGHLIGHT_TONE
        return [_pseudo_html(content, marked, tone), ""]
    return [f"```{language}", content, "```", ""]


def _link(block):
    label = _escape(block.content["label"])
    url = _escape(block.content["url"])
    return [f'- <a href="{url}" target="_blank" rel="noopener noreferrer">{label}</a>', ""]


def _css_length(value) -> str:
    """A unit-bearing CSS length string, treating a bare number as pixels."""
    return value if isinstance(value, str) else f"{value}px"


def _figure_inner(image: dict) -> tuple[str, str]:
    """Build ``(img_html, caption_html)`` for one image dict.

    Shared by a standalone image (`_image`) and a row item (`_row`) so both
    render the <img> and caption identically. Caption runs through `_inline_md`
    and honors `caption_align`; an absent caption yields an empty string.
    """
    sizes = []
    if image.get("width") is not None:
        sizes.append(f"width:{_css_length(image['width'])}")
    if image.get("height") is not None:
        sizes.append(f"height:{_css_length(image['height'])}")
    style = f' style="{";".join(sizes)}"' if sizes else ""
    img = (
        f'<img src="{_escape(image["src"])}" '
        f'alt="{_escape(image.get("alt") or "")}"{style}>'
    )
    caption = ""
    if image.get("caption"):
        align = image.get("caption_align") or "center"
        align_style = f' style="text-align:{align}"' if align != "center" else ""
        caption = (
            f"<figcaption{align_style}>"
            f"{_inline_md(image['caption'])}</figcaption>"
        )
    return img, caption


def _figure_classes(image: dict) -> str:
    """The lk-figure class list for an image: framed/sized modifiers."""
    classes = "lk-figure"
    if image.get("framed"):
        classes += " lk-figure--framed"
    if image.get("width") is not None or image.get("height") is not None:
        classes += " lk-figure--sized"
    return classes


def _image(block):
    # Emitted as a semantic <figure> (not Marp image markdown) so the caption can
    # be styled and centered and an opt-in frame can hang off a modifier class.
    image = block.content
    img, caption = _figure_inner(image)
    return [f'<figure class="{_figure_classes(image)}">{img}{caption}</figure>', ""]


def _side_image(block):
    image = block.content
    src = image["src"]
    side = image.get("side") or "right"
    width = image.get("width")
    alt = image.get("alt") or ""
    directive = f"bg {side}"
    if width is not None:
        directive += f":{width}"
    if alt:
        directive += f" {alt}"
    return [f"![{directive}]({src})", ""]


def _aside(block):
    return [f"> {str(block.content).strip()}", ""]


_ALIGN_DELIMITERS = {"left": ":---", "center": ":---:", "right": "---:"}


def _table_cell(text: str) -> str:
    # Keep cell markdown intact for Marp; only neutralize the two characters
    # that would break the single-line row syntax.
    return str(text).replace("|", "\\|").replace("\n", " ")


def _table(block):
    table = block.content
    headers = table["headers"]
    align = table["align"]
    if align is None:
        delimiters = ["---"] * len(headers)
    else:
        delimiters = [_ALIGN_DELIMITERS[a] for a in align]
    lines = [
        "| " + " | ".join(_table_cell(h) for h in headers) + " |",
        "| " + " | ".join(delimiters) + " |",
    ]
    for row in table["rows"]:
        lines.append("| " + " | ".join(_table_cell(c) for c in row) + " |")
    return [*lines, ""]


_ARCH_FLOW_GLYPH = {"down": "▼", "up": "▲", "both": "⇅"}


def _architecture(block):
    """A layered + modular diagram: stacked layer bands of module boxes.

    Pure HTML styled by the theme (`.lk-arch*`) — layers stack and modules sit
    side by side via CSS, no measured geometry. A `flow` chevron is emitted
    between adjacent layers only. Emitted as one line so Marp passes the raw
    HTML through untouched rather than reparsing it as markdown.
    """
    diagram = block.content
    flow = diagram.get("flow")
    parts = ['<figure class="lk-arch">']
    layers = diagram["layers"]
    for i, layer in enumerate(layers):
        if i and flow:
            glyph = _ARCH_FLOW_GLYPH.get(flow, "")
            parts.append(
                f'<div class="lk-arch-flow lk-arch-flow--{flow}">{glyph}</div>'
            )
        parts.append('<div class="lk-arch-layer">')
        title = layer.get("title")
        if title:
            parts.append(f'<div class="lk-arch-label">{_escape(title)}</div>')
        parts.append('<div class="lk-arch-modules">')
        for module in layer["modules"]:
            cls = "lk-arch-module"
            if module == model.ARCH_ELLIPSIS:
                cls += " lk-arch-module--more"
            parts.append(f'<div class="{cls}">{_escape(module)}</div>')
        parts.append("</div></div>")
    if diagram.get("caption"):
        parts.append(f"<figcaption>{_inline_md(diagram['caption'])}</figcaption>")
    parts.append("</figure>")
    return ["".join(parts), ""]


SIDENOTE_WHEEL_SIZE = 6


def _sidenote(block, index=0):
    note = block.content
    title = f'{_escape(note["title"])}：'
    if note.get("link"):
        title_html = (
            f'<a class="sidenote-title" href="{_escape(note["link"])}"'
            f' target="_blank" rel="noopener noreferrer">{title}</a>'
        )
    else:
        title_html = f'<span class="sidenote-title">{title}</span>'
    logo_html = _sidenote_logo_html(note.get("logo"))
    # Cycle through the theme's color wheel so several sidenotes on one slide
    # don't read as a monotonous block. The first (index 0) keeps the bare
    # markup and inherits --sidenote-1 from .sidenote; later ones pick the next
    # wheel slot via an inline background (positional CSS can't reach into
    # Marp's svg>foreignObject>section DOM, so the color is assigned here).
    style = ""
    if index:
        slot = index % SIDENOTE_WHEEL_SIZE + 1
        style = f' style="background: var(--sidenote-{slot})"'
    html = (
        f'<aside class="{_sidenote_classes(note)}"{style}>{logo_html}'
        f'<p class="sidenote-body">{title_html}{_inline_md(note["text"])}</p>'
        f'</aside>'
    )
    return [html, ""]


def _anchor_axes(at: str) -> tuple[str, str]:
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
_AXIS = {"start": (0.0, 0), "center": (0.5, -50), "end": (1.0, -100)}


def _annotation_decls(note, width: int, height: int) -> str:
    horizontal, vertical = _anchor_axes(note.at)
    h = {"left": "start", "right": "end"}.get(horizontal, "center")
    v = {"top": "start", "bottom": "end"}.get(vertical, "center")
    hf, tx = _AXIS[h]
    vf, ty = _AXIS[v]
    left = round(width * hf) + note.dx
    top = round(height * vf) + note.dy
    return f"left:{left}px; top:{top}px; transform: translate({tx}%, {ty}%);"


def annotation_markup(note, uid: str, width: int, height: int) -> tuple[str, str]:
    """Return ``(aside_html, css_rule)`` for one annotation bubble.

    ``uid`` is a deck-unique token that ties the aside's ``ann-<uid>`` class to
    the returned rule. A bottom-anchored bubble sits at the slide's lower edge,
    so its tail flips up (toward content above) via ``annotation--tail-up``.
    """
    _, vertical = _anchor_axes(note.at)
    extra = " annotation--tail-up" if vertical == "bottom" else ""
    aside = (
        f'<aside class="annotation ann-{uid}{extra}">'
        f"{_inline_md(note.text)}</aside>"
    )
    rule = f"section .ann-{uid} {{ {_annotation_decls(note, width, height)} }}"
    return aside, rule


def _row(block):
    """A row of side-by-side image figures.

    One raw-HTML line (like `_architecture`) so Marp passes it through. Each
    item is an `lk-figure` reusing the standalone-image markup; an unsized item
    shares the track evenly (`flex:1 1 0`), a sized item takes its width
    (`flex:0 0 <width>`) and the rest split the remainder. A row-level caption
    renders as a trailing <figcaption>.
    """
    content = block.content
    parts = ['<figure class="lk-row"><div class="lk-row-track">']
    for image in content["items"]:
        img, caption = _figure_inner(image)
        width = image.get("width")
        basis = f"flex:0 0 {_css_length(width)}" if width is not None else "flex:1 1 0"
        parts.append(
            f'<figure class="{_figure_classes(image)} lk-row-item" '
            f'style="{basis}">{img}{caption}</figure>'
        )
    parts.append("</div>")
    if content.get("caption"):
        parts.append(f'<figcaption>{_inline_md(content["caption"])}</figcaption>')
    parts.append("</figure>")
    return ["".join(parts), ""]


# Math inside a chip: `$$…$$` (a whole row, set in display style) or `$…$`.
_CHIP_MATH_RE = re.compile(r"\$\$(?P<display>.+?)\$\$|\$(?P<inline>[^$\n]+)\$")

# The characters markdown still reads as markup in text that is already HTML.
_MD_INERT = {"*": "&#42;", "_": "&#95;", "`": "&#96;", "\\": "&#92;"}
_MD_ACTIVE_RE = re.compile("[*_`\\\\]")


def _inert_md(html: str) -> str:
    """Neutralize what markdown would still read as markup in rendered HTML.

    Only the math form of the chip needs this: that one is emitted on a markdown
    line rather than as a raw HTML block, so markdown-it reads the text a second
    time, and a stray `_` in it is not emphasis the author asked for. Entities
    are the safe spelling — they survive inside an attribute (a link's href) as
    well as in text.
    """
    return _MD_ACTIVE_RE.sub(lambda m: _MD_INERT[m.group()], html)


def _chip_math_md(line: str) -> str:
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
    for match in _CHIP_MATH_RE.finditer(line):
        if match.start() > pos:
            out.append(_inert_md(_inline_md(line[pos:match.start()])))
        display = match.group("display")
        latex = display if display is not None else match.group("inline")
        prefix = "\\displaystyle " if display is not None else ""
        out.append(f"${prefix}{latex.strip()}$")
        pos = match.end()
    if pos < len(line):
        out.append(_inert_md(_inline_md(line[pos:])))
    return "".join(out)


def _highlight(block):
    """A centered chip: the outer element centers, the inner <span> is the wash.

    The split is the point — an inline-block span hugs the widest line instead
    of becoming a full-width banner. `_inline_md` already turns newlines into
    <br>, so a multi-line highlight grows the same chip taller.

    A chip carrying math swaps the outer <p> for a blocked-out <span>, because
    Marp renders math while parsing markdown and a line starting with `<p` is a
    raw HTML block it never parses — a `<span>` with text after it on the line
    is not, so the row arrives as a paragraph and the formula is typeset. The
    box is the same one; only the tag it is drawn on differs.
    """
    content = block.content
    lines = model.highlight_lines(content["text"])
    tone = content["tone"]
    classes = f"lk-highlight lk-highlight--{tone}"
    if any(_CHIP_MATH_RE.search(line) for line in lines):
        body = "<br>".join(_chip_math_md(line) for line in lines)
        return [f'<span class="{classes}"><span>{body}</span></span>', ""]
    body = _inline_md("\n".join(lines))
    return [f'<p class="{classes}"><span>{body}</span></p>', ""]


def _demo(block):
    """A command, drawn as the transcript it is, with a button that runs it.

    The button carries the command's id, never the command — see
    ``lecturekit.demo`` for why. It ships ``disabled``: a demo is only runnable
    against a live dev server, so the static deck shows the same block as an
    inert transcript and ``demo.js`` arms it when a server is actually behind
    the page.

    Two shapes, because two things get written as demos. A bare one-liner stays
    a chip on one row, the way it always did; a command with several lines or a
    recorded output becomes a bordered transcript, so that a page which used to
    hold a ``p.code(...)`` of the same thing keeps its proportions.
    """
    from lecturekit import demo as demo_module

    content = block.content
    command = str(content["command"])
    output = content.get("output")
    lines = demo_module.prompt_lines(command)
    block_form = len(lines) > 1 or bool(output)
    button = (
        '<button class="lk-demo-run" type="button" disabled'
        ' aria-label="Run this demo">▶</button>'
    )
    name = f'<span class="lk-demo-name">{_escape(content["name"])}</span>'
    desc = ""
    if content.get("description"):
        desc = f'<span class="lk-demo-desc">{_inline_md(content["description"])}</span>'
    # `lk-demo-cmd` holds the command lines and nothing else — `demo.js` reads
    # its text to label the drawer.
    cmd = f'<code class="lk-demo-cmd">{_escape(chr(10).join(lines))}</code>'
    open_tag = (
        f'<div class="lk-demo" data-lk-demo="{demo_module.demo_id(command)}"'
        f' data-lk-demo-form="{"block" if block_form else "inline"}">'
    )
    if not block_form:
        return ["".join([open_tag, button, name, cmd, desc, "</div>"]), ""]
    parts = [open_tag, f'<div class="lk-demo-head">{button}{name}{desc}</div>',
             f'<pre class="lk-demo-term">{cmd}']
    if output:
        # No newline between the two: inside a `pre` that would be a blank row,
        # and a slide cannot spare one. The gap is CSS's to draw.
        parts.append(f'<code class="lk-demo-out">{_escape(str(output))}</code>')
    parts.append("</pre></div>")
    return ["".join(parts), ""]


def _bridge(block):
    """A transition page's centered lines. Plain text — escaped, no inline md."""
    body = "<br>".join(
        _escape(line) for line in str(block.content["text"]).splitlines()
    )
    return [f'<div class="lk-bridge-body">{body}</div>', ""]


def _spacer(block):
    px = int(block.content["px"])
    return [f'<div class="lk-spacer" aria-hidden="true" style="height:{px}px"></div>', ""]


BLOCK_RENDERERS = {
    "cover": _cover,
    "bridge": _bridge,
    "slide": _slide,
    "spacer": _spacer,
    "highlight": _highlight,
    "demo": _demo,
    "code": _code,
    "link": _link,
    "image": _image,
    "side_image": _side_image,
    "aside": _aside,
    "sidenote": _sidenote,
    "table": _table,
    "architecture": _architecture,
    "row": _row,
}


def render_block(
    block: model.Block,
    sidenote_index: int = 0,
    footnote_numbers: tuple[int, ...] = (),
) -> list[str]:
    if block.kind == "sidenote":
        lines = _sidenote(block, sidenote_index)
    else:
        fn = BLOCK_RENDERERS.get(block.kind)
        lines = fn(block) if fn else []
    if footnote_numbers and lines:
        lines = _append_footnote_marker(lines, footnote_numbers, block.kind)
    return lines


_LONE_TAG_RE = re.compile(r"<[^>]+>")


def _last_content_index(lines: list[str]) -> int:
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
        if line and not _LONE_TAG_RE.fullmatch(line):
            return i
    return len(lines) - 1


# Blocks rendered as a single raw-HTML <figure> line (image, row, architecture).
# A marker appended *after* </figure> becomes its own Marp paragraph — a wasted
# line — so for these the marker is tucked *inside* the figure instead.
_FIGURE_KINDS = frozenset({"image", "row", "architecture"})


def _place_figure_marker(figure_html: str, marker: str, kind: str) -> str:
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
    if kind == "image":
        open_end = figure_html.index(">") + 1
        open_tag, body = figure_html[:open_end], figure_html[open_end:]
        img = body[: -len("</figure>")]
        return (
            f'{open_tag}<span class="lk-figure-anchor">{img}{corner}</span></figure>'
        )
    inner = figure_html[: -len("</figure>")]
    return f"{inner}{corner}</figure>"


def _append_footnote_marker(
    lines: list[str], numbers: tuple[int, ...], kind: str
) -> list[str]:
    marker = (
        f'<sup class="footnote-ref">{", ".join(str(n) for n in numbers)}</sup>'
    )
    lines = list(lines)
    idx = _last_content_index(lines)
    if kind in _FIGURE_KINDS:
        lines[idx] = _place_figure_marker(lines[idx], marker, kind)
    elif kind in ("code", "table"):
        # The last content line is a syntactic row (closing ``` fence or a
        # `| … |` table row); appending inline would break it, so the marker is
        # emitted just after the block. A zero-height, right-aligned wrapper
        # parks it on the block's bottom-right corner, so — as on a figure — it
        # claims no line of its own.
        lines.insert(idx + 1, f'<div class="footnote-ref-tuck">{marker}</div>')
    elif kind == "highlight":
        # One element on one line; a marker appended after it would become its
        # own Marp paragraph. Tuck it inside, just outside the chip's <span> —
        # so after the inner </span>, before the outer tag closes. That tag is a
        # <p>, or a <span> on the math form of the chip (see `_highlight`).
        tail = "</span>" if lines[idx].endswith("</span></span>") else "</p>"
        head = lines[idx][: -len(tail)]
        lines[idx] = f"{head} {marker}{tail}"
    else:
        # Text blocks: the marker rides the end of the last line. It is glued on
        # with a word joiner and laid out at zero width (`--hang`), so a line
        # that already fills the column cannot break before it and drop a lone
        # number onto the next line — it hangs into the slide's right padding
        # instead.
        hang = marker.replace(
            'class="footnote-ref"', 'class="footnote-ref footnote-ref--hang"'
        )
        lines[idx] = f"{lines[idx]}&#8288;{hang}"
    return lines


def render_footnotes(texts: list[str]) -> str:
    """A bottom-left aside listing a page's footnotes, numbered to match markers."""
    items = "".join(
        f'<p class="footnote-item">'
        f'<sup class="footnote-ref">{i}</sup> {_inline_md(text)}</p>'
        for i, text in enumerate(texts, 1)
    )
    return f'<aside class="footnotes">{items}</aside>'
