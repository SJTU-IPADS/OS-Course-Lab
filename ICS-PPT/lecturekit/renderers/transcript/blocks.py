"""Blocks → the transcript's HTML.

One function per block kind, the same shape the other renderers use. What is
*not* here is the point of the target: `notes`, `prose`, `demo`, `cover` and
`spacer` have no entry, and footnotes are dropped by the renderer — a printed
cheat sheet has no room for a source line it will never follow.
"""

from __future__ import annotations

import html

from lecturekit import model, pseudo

from .images import DEFAULT_WIDTH_PX, Embedder
from .text import escape, inline, markdown

#: Every kind that reaches the sheet. `only=["transcript"]` can still force one
#: in, and `except_=["transcript"]` takes any of them out.
TRANSCRIPT_KINDS = {
    "slide", "code", "link", "image", "side_image", "sidenote", "aside",
    "table", "architecture", "row", "highlight",
}

_PSEUDO_CLASSES = {"keyword": "kw", "message": "msg", "state": "st", "comment": "cm"}

# A figure prints at one uniform scale — the intrinsic width that fills a
# column — so two figures drawn at the same font size come out the same size on
# paper. Anything at or above the reference width takes the full column; a small
# icon keeps its proportion, down to a floor that stays legible.
_FULL_WIDTH_PX = DEFAULT_WIDTH_PX
_MIN_FIGURE_PCT = 22


def render_block(block: model.Block, embedder: Embedder) -> str:
    render = _RENDERERS.get(block.kind)
    if render is not None:
        return render(block, embedder)
    # A kind this target does not normally draw, forced in with
    # `only=["transcript"]` — the author asked for that text on the sheet, so a
    # markdown body (`prose`, `notes`) is set as body text rather than dropped.
    if isinstance(block.content, str):
        return f'<div class="tx-slide">{markdown(block.content)}</div>'
    return ""


def render_annotations(block: model.Block) -> str:
    """A block's callout bubbles, as the lines under it a sheet can carry.

    On the slide these float over the figure, positioned by author coordinates;
    paper has nothing to float over, so each becomes one marked line below the
    block it was attached to.
    """
    return "".join(
        f'<p class="tx-anno">{inline(note.text)}</p>' for note in block.annotations
    )


def _figure_pct(width_px: int) -> int:
    return max(_MIN_FIGURE_PCT, min(100, round(100 * width_px / _FULL_WIDTH_PX)))


def _img(src: str, alt: str, embedder: Embedder, *, pct: int | None = None) -> str:
    figure = embedder.embed(src)
    if figure is None:
        return f'<p class="tx-missing">[缺图：{escape(src)}]</p>'
    width = _figure_pct(figure.width_px) if pct is None else pct
    return (
        f'<img src="{figure.uri}" alt="{html.escape(alt, quote=True)}" '
        f'style="width:{width}%">'
    )


def _caption(text: str | None) -> str:
    return f'<figcaption>{inline(text)}</figcaption>' if text else ""


def _slide(block: model.Block, embedder: Embedder) -> str:
    body = markdown(block.content)
    if block.float_image:
        floated = _img(
            block.float_image["src"], block.float_image.get("alt", ""), embedder,
            pct=38,
        )
        body = f'<span class="tx-float">{floated}</span>{body}'
    return f'<div class="tx-slide">{body}</div>'


def _aside(block: model.Block, embedder: Embedder) -> str:
    return f'<div class="tx-aside">{markdown(block.content)}</div>'


def _sidenote(block: model.Block, embedder: Embedder) -> str:
    content = block.content
    logo = content.get("logo") or "📖"
    title = inline(content["title"])
    if content.get("link"):
        url = html.escape(content["link"], quote=True)
        title = f'<a href="{url}">{title}</a>'
    # The logo may be an image path rather than a glyph; on a sheet that is one
    # more figure than the line is worth, so a path falls back to the default.
    if "/" in str(logo) or str(logo).endswith((".svg", ".png", ".jpg")):
        logo = "📖"
    return (
        f'<div class="tx-sidenote"><span class="tx-logo">{escape(str(logo))}</span>'
        f'<strong>{title}</strong> {markdown(content["text"])}</div>'
    )


def _highlight(block: model.Block, embedder: Embedder) -> str:
    tone = block.content.get("tone", "yellow")
    lines = model.highlight_lines(block.content["text"])
    body = "<br>".join(inline(line) for line in lines if line)
    return f'<p class="tx-highlight tx-{tone}"><span>{body}</span></p>'


def _code(block: model.Block, embedder: Embedder) -> str:
    language = block.content["language"]
    content = block.content["content"].strip("\n")
    if language != "pseudo":
        return f'<pre class="tx-code">{escape(content)}</pre>'
    marked = set(block.content.get("mark") or ())
    tone = block.content.get("tone") or model.DEFAULT_HIGHLIGHT_TONE
    lines = []
    for number, tokens in enumerate(pseudo.tokenize(content), start=1):
        line = "".join(
            f'<span class="tok-{_PSEUDO_CLASSES[kind]}">{escape(text)}</span>'
            if kind in _PSEUDO_CLASSES else escape(text)
            for kind, text in tokens
        )
        if number in marked:
            line = f'<span class="tx-codemark tx-codemark--{tone}">{line}</span>'
        lines.append(line)
    return '<pre class="tx-code tx-pseudo">%s</pre>' % "\n".join(lines)


def _link(block: model.Block, embedder: Embedder) -> str:
    url = html.escape(block.content["url"], quote=True)
    return f'<p class="tx-link"><a href="{url}">{inline(block.content["label"])}</a></p>'


def _image(block: model.Block, embedder: Embedder) -> str:
    content = block.content
    figure = _img(content["src"], content.get("alt", ""), embedder)
    framed = " tx-framed" if content.get("framed") else ""
    return f'<figure class="tx-fig{framed}">{figure}{_caption(content.get("caption"))}</figure>'


def _side_image(block: model.Block, embedder: Embedder) -> str:
    # A split-background on the slide; on paper it is simply another figure.
    return (
        f'<figure class="tx-fig">'
        f'{_img(block.content["src"], block.content.get("alt", ""), embedder)}</figure>'
    )


def _row(block: model.Block, embedder: Embedder) -> str:
    items = block.content["items"]
    widths = []
    for item in items:
        figure = embedder.embed(item["src"])
        widths.append(figure.width_px if figure else DEFAULT_WIDTH_PX)
    total = sum(widths) or 1
    cells = []
    for item, width in zip(items, widths):
        share = max(15, round(100 * width / total) - 2)
        cells.append(
            f'<span class="tx-cell" style="width:{share}%">'
            f'{_img(item["src"], item.get("alt", ""), embedder, pct=100)}'
            f'{_caption(item.get("caption"))}</span>'
        )
    return (
        f'<figure class="tx-fig tx-row">{"".join(cells)}'
        f'{_caption(block.content.get("caption"))}</figure>'
    )


def _table(block: model.Block, embedder: Embedder) -> str:
    content = block.content
    align = content.get("align") or ["left"] * len(content["headers"])
    head = "".join(
        f'<th style="text-align:{a}">{inline(cell)}</th>'
        for cell, a in zip(content["headers"], align)
    )
    body = "".join(
        "<tr>%s</tr>" % "".join(
            f'<td style="text-align:{a}">{inline(cell)}</td>'
            for cell, a in zip(row, align)
        )
        for row in content["rows"]
    )
    return f'<table class="tx-table"><thead><tr>{head}</tr></thead><tbody>{body}</tbody></table>'


_FLOW_GLYPH = {"down": "↓", "up": "↑", "both": "↕"}


def _architecture(block: model.Block, embedder: Embedder) -> str:
    content = block.content
    flow = _FLOW_GLYPH.get(content.get("flow") or "")
    bands = []
    for index, layer in enumerate(content["layers"]):
        if index and flow:
            bands.append(f'<div class="tx-flow">{flow}</div>')
        title = (
            f'<div class="tx-layer-title">{escape(layer["title"])}</div>'
            if layer.get("title") else ""
        )
        modules = "".join(
            f'<span class="tx-mod{" tx-mod-more" if _is_more(m) else ""}">'
            f'{escape(str(m))}</span>'
            for m in layer["modules"]
        )
        bands.append(f'<div class="tx-layer">{title}<div class="tx-mods">{modules}</div></div>')
    return (
        f'<figure class="tx-fig tx-arch">{"".join(bands)}'
        f'{_caption(content.get("caption"))}</figure>'
    )


def _is_more(module) -> bool:
    return str(module) in ("...", "…")


_RENDERERS = {
    "slide": _slide,
    "aside": _aside,
    "sidenote": _sidenote,
    "highlight": _highlight,
    "code": _code,
    "link": _link,
    "image": _image,
    "side_image": _side_image,
    "row": _row,
    "table": _table,
    "architecture": _architecture,
}
