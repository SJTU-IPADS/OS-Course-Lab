from __future__ import annotations

import json
import re
import shutil
from dataclasses import replace
from pathlib import Path

from lecturekit import demo, i18n, model, references
from .blocks import BLOCK_RENDERERS, annotation_markup, render_block, render_footnotes
from .marp import CJK_FONT
from .pdf import SLIDE_LINK_PREFIX

ASSETS = Path(__file__).resolve().parent / "assets"
REFERENCES_PAGE_ID = "__references__"
VIEWER_KINDS = set(BLOCK_RENDERERS)
OUTLINE_CSS = (ASSETS / "outline.css").read_text(encoding="utf-8")

# A Marp background directive (``![bg …](…)``) must stay a top-level image, so a
# side_image is never wrapped: it is always visible and consumes no reveal step.
# A spacer is pure layout whitespace, so it is likewise always visible and never
# claims a reveal step of its own.
_REVEAL_SKIP = {"side_image", "spacer"}
_GAP_FLOW_SKIP = {"side_image"}
_TITLE_MARK_RE = re.compile(r"\\emph\{([^{}]+)\}")


def _wrap_untranslated(lines: list[str]) -> list[str]:
    """Wrap a block that fell back to the baseline in a faintly washed div.

    Not applied to the kinds `_REVEAL_SKIP` names: a `side_image` is a Marp
    background directive and has to stay a top-level image, and a `spacer` is
    whitespace with nothing to wash.

    A missing translation is otherwise invisible — the slide simply shows the
    original language, which reads as a deliberate choice. The wash is a
    rehearsal aid, so only the deck draws it; the book, PPTX and the transcript
    sheet ignore `Block.untranslated` entirely.
    """
    body = list(lines)
    while body and body[-1] == "":
        body.pop()
    return ['<div class="lk-untranslated">', "", *body, "", "</div>", ""]


def _reveal_attrs(index: int, items: bool, *, extra_class: str = "") -> str:
    """The wrapper's attributes for one reveal step.

    ``items`` marks a block that steps through its own list items rather than
    lighting whole (`p.slide(..., reveal="items")`). The split itself happens in
    the browser — `reveal.js` finds the rendered `<li>`s, which only exist after
    Marp has turned this markdown into HTML — so all the build side owes it is
    the flag.
    """
    classes = f"{extra_class} reveal-block" if extra_class else "reveal-block"
    attrs = f'class="{classes}" data-reveal="{index}"'
    return f'{attrs} data-reveal-items="1"' if items else attrs


def _wrap_reveal(lines: list[str], index: int, *, items: bool = False) -> list[str]:
    """Wrap a block's rendered lines as one reveal step.

    Blank lines around the inner content keep Marp parsing it as markdown rather
    than raw HTML (a CommonMark type-6 HTML block ends at a blank line).
    """
    body = list(lines)
    while body and body[-1] == "":
        body.pop()
    return [f"<div {_reveal_attrs(index, items)}>", "", *body, "", "</div>", ""]


def _presenter_note(text: str) -> str:
    """Render a ``notes`` block as a Marp presenter note.

    Marp's bespoke deck turns an HTML comment into a speaker-only note, shown in
    the presenter view (press ``p``) but never on the slide the audience sees.
    ``-->`` in the text is defused so it can't close the comment early.
    """
    return f"<!--\n{text.replace('-->', '--&gt;')}\n-->"


def _tag_reveal(aside_html: str, index: int) -> str:
    """Add ``data-reveal`` to an annotation bubble so it reveals with its block.

    The bubble is positioned by a generated CSS rule keyed on its class, so it
    must not be wrapped in a div; we add the attribute to the <aside> directly.
    """
    return aside_html.replace(
        '<aside class="annotation', f'<aside data-reveal="{index}" class="annotation', 1
    )


def _wrap_gap_block(
    lines: list[str],
    reveal_index: int | None = None,
    *,
    items: bool = False,
    held: bool = False,
) -> list[str]:
    body = list(lines)
    while body and body[-1] == "":
        body.pop()
    attrs = 'class="lk-gap-block"'
    if held:
        # An after_frames block before the last frame: it keeps its seam and its
        # space in the flow (so the gap split matches the last frame's), shown
        # on none of them.
        attrs = 'class="lk-gap-block lk-held" aria-hidden="true"'
    elif reveal_index is not None:
        attrs = _reveal_attrs(reveal_index, items, extra_class="lk-gap-block")
    return [f"<div {attrs}>", "", *body, "", "</div>"]


def _render_gap_flow(
    page: model.Page,
    visible_blocks: list[model.Block],
    slide_size: tuple[int, int],
    *,
    reveal: bool,
    footnotes: list[str],
    annotation_rules: list[str],
) -> list[str]:
    width, height = slide_size
    gap = page.gap
    if gap is None:
        return []

    out: list[str] = []
    flow_blocks: list[model.Block] = []
    sidenote_index = 0
    reveal_index = 0

    for block in visible_blocks:
        if block.kind in _GAP_FLOW_SKIP:
            out.extend(render_block(block, sidenote_index=sidenote_index))
            if page.show_annotations:
                for note in block.annotations:
                    uid = f"{page.id}-{len(annotation_rules)}"
                    aside, rule = annotation_markup(note, uid, width, height)
                    out.extend([aside, ""])
                    annotation_rules.append(rule)
            continue
        flow_blocks.append(block)

    # `p.gap("fill")` lifts the cap: `max-height: none` lets a seam take
    # whatever the flexbox has left over.
    cap = "none" if gap.max_px is None else f"{gap.max_px}px"
    out.append(
        '<div class="lk-gap-flow" '
        f'style="--lk-gap-min: {gap.min_px}px; --lk-gap-max: {cap};">'
    )
    first_visual = True
    for block in flow_blocks:
        if not first_visual:
            out.append('<div class="lk-gap-spacer" aria-hidden="true"></div>')
        first_visual = False

        if model.block_held(page, block):
            out.extend(_wrap_gap_block(
                render_block(block, sidenote_index=sidenote_index), held=True
            ))
            if block.kind == "sidenote":
                sidenote_index += 1
            continue
        numbers: tuple[int, ...] = ()
        if block.footnotes:
            start = len(footnotes) + 1
            numbers = tuple(range(start, start + len(block.footnotes)))
            footnotes.extend(block.footnotes)
        lines = render_block(
            block, sidenote_index=sidenote_index, footnote_numbers=numbers
        )
        if block.untranslated and block.kind not in _REVEAL_SKIP:
            lines = _wrap_untranslated(lines)
        wrap = (
            reveal and block.kind not in _REVEAL_SKIP and _block_steps(page, block)
        )
        idx = reveal_index if wrap else None
        out.extend(_wrap_gap_block(lines, idx, items=block.reveal == "items"))
        if idx is not None:
            reveal_index += 1
        if page.show_annotations:
            for note in block.annotations:
                uid = f"{page.id}-{len(annotation_rules)}"
                aside, rule = annotation_markup(note, uid, width, height)
                if idx is not None:
                    aside = _tag_reveal(aside, idx)
                out.extend([aside, ""])
                annotation_rules.append(rule)
        if block.kind == "sidenote":
            sidenote_index += 1
    out.extend(["</div>", ""])
    return out


class StaticViewerRenderer:
    name = "viewer"

    def __init__(
        self,
        *,
        asset_root: Path | None = None,
        reveal: bool = False,
        paginate: bool = True,
    ):
        self.asset_root = asset_root
        self.reveal = reveal
        self.paginate = paginate

    def render(self, lecture: model.Lecture, output_dir: Path) -> None:
        output_dir.mkdir(parents=True, exist_ok=True)
        if self.asset_root is not None:
            copy_assets(self.asset_root, output_dir)
        # After the host's own copy: it clears `assets/` wholesale, which would
        # take the borrowed subtrees with it.
        copy_borrowed_assets(lecture.borrowed, output_dir)

        # Rewritten on every render, always — an emptied table is what stops a
        # deleted demo from staying runnable in a live session.
        demo.write(lecture, output_dir)

        data = build_data(lecture)
        Path(output_dir, "lecture.json").write_text(
            json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        Path(output_dir, "viewer.css").write_text(
            (ASSETS / "viewer.css").read_text(encoding="utf-8"), encoding="utf-8"
        )
        Path(output_dir, "outline.css").write_text(
            (ASSETS / "outline.css").read_text(encoding="utf-8"), encoding="utf-8"
        )
        Path(output_dir, "viewer.js").write_text(
            (ASSETS / "viewer.js").read_text(encoding="utf-8"), encoding="utf-8"
        )
        Path(output_dir, "index.html").write_text(
            render_index_html(
                lecture.title, json.dumps(data, ensure_ascii=False), lecture.lang
            ),
            encoding="utf-8",
        )
        Path(output_dir, "slides.md").write_text(
            build_marp_markdown(lecture, reveal=self.reveal, paginate=self.paginate),
            encoding="utf-8",
        )
        Path(output_dir, "outline.html").write_text(
            build_outline_print_html(lecture), encoding="utf-8"
        )


def build_data(lecture: model.Lecture) -> dict:
    lecture = with_references_page(lecture)
    pages = iter_pages(lecture.children)
    folds = model.outline_folds(lecture.children)
    spans = outline_spans(pages, folds)
    numbers = model.slide_numbers(pages, folds)
    return {
        "lecture": {"id": lecture.id, "title": lecture.title, "subtitle": lecture.subtitle},
        # lecturekit's own chrome, resolved here so `viewer.js` prints the deck's
        # language instead of hardcoding one. See `i18n.UI_STRINGS`.
        "ui": i18n.ui_table(lecture.lang),
        "tree": build_tree(lecture.children, spans),
        "pages": [build_page(page, number) for page, number in zip(pages, numbers)],
    }


def outline_spans(
    pages: list[model.Page], folds: frozenset[str] = frozenset()
) -> dict[str, int]:
    """``page id -> how many deck slides its outline row stands for``.

    One row per *idea*, not per slide: an animation folds into a single row on
    its first frame, which spans the whole group, and its other frames get no
    row at all (span 0). A page named by ``folds`` (:func:`model.outline_folds`
    — it repeats the previous page's title) folds the same way, handing its
    slides to the row above. A bridge page is no idea at all — it gets no row
    either (span 0; the print outline still steps its physical index, see
    ``_walk_outline``). Every other page spans itself. Computed over the pages
    actually in the deck, so a ``--pages`` selection that keeps two frames of
    three yields a span of two.
    """
    spans = {
        page.id: (0 if model.is_bridge_page(page) else 1) for page in pages
    }
    for positions in model.frame_groups(pages).values():
        spans[pages[positions[0]].id] = len(positions)
        for position in positions[1:]:
            spans[pages[position].id] = 0
    leader: str | None = None
    for page in pages:
        if model.is_bridge_page(page):
            continue
        if page.id in folds and leader is not None:
            spans[leader] += spans[page.id]
            spans[page.id] = 0
        elif spans[page.id] > 0:
            leader = page.id
    return spans


def build_tree(
    nodes: list[model.Section | model.Page], spans: dict[str, int]
) -> list[dict]:
    """The outline tree: one node per section and per outline row."""
    out = []
    for node in nodes:
        built = build_tree_node(node, spans)
        if built is not None:
            out.append(built)
    return out


def build_tree_node(
    node: model.Section | model.Page, spans: dict[str, int] | None = None
) -> dict | None:
    """One outline node, or None for a page folded into an earlier row.

    A page node carries ``frames`` when its row stands for more than one slide,
    so the viewer can label it with the range (``12–14``) and still map every
    slide back to the row it belongs to.
    """
    if isinstance(node, model.Page):
        if model.is_bridge_page(node):
            return None
        span = 1 if spans is None else spans.get(node.id, 1)
        if span == 0:
            return None
        built = {"type": "page", "id": node.id, "title": node.title}
        if span > 1:
            built["frames"] = span
        return built
    return {
        "type": "section", "id": node.id, "title": node.title,
        "collapsed": node.collapsed,
        "children": build_tree(node.children, spans or {}),
    }


def iter_pages(children: list[model.Section | model.Page]) -> list[model.Page]:
    return model.flatten_pages(children)


def with_references_page(lecture: model.Lecture) -> model.Lecture:
    """Return ``lecture`` with a trailing references page, if any page cited work.

    Pure: a new ``Lecture`` is returned (the input is never mutated), so every
    render entry point can call it independently and get the same deck. The page
    is a normal top-level page, so it flows through the outline, viewer
    navigation, slide numbering, and PDF like any other. No citations → the
    lecture is returned unchanged.
    """
    pages = iter_pages(lecture.children)
    if any(page.id == REFERENCES_PAGE_ID for page in pages):
        raise model.ValidationError(
            f"page id {REFERENCES_PAGE_ID!r} is reserved for the auto-generated "
            "references page"
        )
    entries = references.collect_citations(pages, model.outline_folds(lecture.children))
    if not entries:
        return lecture
    page = model.Page(
        id=REFERENCES_PAGE_ID,
        title=i18n.ui(lecture.lang, "references"),
        blocks=[model.Block(kind="slide", content=_references_markdown(entries))],
    )
    return replace(lecture, children=lecture.children + [page])


def _references_markdown(entries: list[references.CitationEntry]) -> str:
    """The reference list as a slide's markdown: one numbered entry per paragraph.

    Each entry keeps its ``**[n]**`` marker (so autobold leaves the line alone),
    links the title when a url is present, and ends with the deck pages that
    cited it, e.g. ``(P3, P7)``.
    """
    lines: list[str] = []
    for number, entry in enumerate(entries, start=1):
        citation = entry.citation
        title = f"[{citation.title}]({citation.url})" if citation.url else citation.title
        meta = _citation_meta(citation)
        pages = ", ".join(f"P{n}" for n in entry.pages)
        line = f"**[{number}]** {title}"
        if meta:
            line += f" — {meta}"
        line += f"  ({pages})"
        lines.append(line)
    return "\n\n".join(lines)


def _citation_meta(citation: model.Citation) -> str:
    """``Author, Year. Venue.`` — with the missing parts dropped."""
    head = ", ".join(part for part in (citation.author, citation.year) if part)
    parts = [p for p in (head, citation.venue) if p]
    return ". ".join(parts)


def build_page(page: model.Page, number: int) -> dict:
    return {
        "id": page.id,
        "title": page.title,
        # The number this slide is *shown* as; the position in ``pages`` is what
        # navigation addresses. They differ once an animation is in the deck.
        "number": number,
        "blocks": [
            {"kind": block.kind, "content": block.content}
            for block in model.select_blocks(page, "viewer", VIEWER_KINDS)
        ],
    }


def build_marp_markdown(
    lecture: model.Lecture,
    theme: str = "basic-office",
    *,
    reveal: bool = False,
    paginate: bool = True,
) -> str:
    lecture = with_references_page(lecture)
    paginate_value = "true" if paginate else "false"
    header = [
        "---",
        "marp: true",
        "math: mathjax",
        f"theme: {theme}",
        f"size: {lecture.ratio}",
        f"paginate: {paginate_value}",
        "---",
    ]
    slide_size = model.RATIOS[lecture.ratio]
    pages = iter_pages(lecture.children)
    numbers = model.slide_numbers(pages, model.outline_folds(lecture.children))
    slides = [
        render_marp_page(
            page,
            slide_size,
            reveal=reveal and _steps_through(page),
            hold_number=i > 0 and numbers[i] == numbers[i - 1],
        )
        for i, page in enumerate(pages)
    ]
    return "\n".join(header) + "\n\n" + "\n\n---\n\n".join(slides) + "\n"


def _steps_through(page: model.Page) -> bool:
    """Does this page reveal block by block, or arrive fully lit?

    An animation's middle frames repeat the first frame's text, so dimming them
    again would make the speaker press Enter through the same bullets N times;
    they arrive fully lit, one Enter apiece — which is what makes paging through
    them feel like the animation running. Two frames do step: the first, through
    the blocks written before the animation, and the last, through the blocks
    written after it (``_block_steps`` picks the half each frame dims).
    """
    group = page.frame_group
    return group is None or group.index in (1, group.total)


def _block_steps(page: model.Page, block: model.Block) -> bool:
    """Does this block claim a reveal step on this page?

    A block steps on the frame where it first appears: pre-animation blocks on
    the first frame, ``after_frames`` blocks on the last. Everywhere else it
    arrives already lit — or, for an ``after_frames`` block before the last
    frame, held invisible (see ``_wrap_held``). On a page without an animation
    every block steps. On a single-frame animation the first frame *is* the
    last, so both halves step there, in author order.
    """
    group = page.frame_group
    if group is None:
        return True
    if block.after_frames:
        return group.index == group.total
    return group.index == 1


def _wrap_held(lines: list[str]) -> list[str]:
    """Wrap an ``after_frames`` block on a frame before the last.

    The block belongs to the animation's finished picture, but its *space* is
    claimed on every frame — rendered invisible rather than dropped, so the
    figure and text above it sit at identical positions while the animation
    plays. ``aria-hidden`` keeps a screen reader from announcing it N times.
    """
    body = list(lines)
    while body and body[-1] == "":
        body.pop()
    return ['<div class="lk-held" aria-hidden="true">', "", *body, "", "</div>", ""]


def build_outline_html(lecture: model.Lecture) -> str:
    """Reproduce the viewer's navigation tree as a static, fully-expanded page.

    Each page row's title is an anchor to its slide (``SLIDE_LINK_PREFIX`` + the
    flat page index, in deck order); the merged-PDF step rewrites those URIs into
    internal page links. Section rows are not links — they have no slide. An
    animation is one row, linking to its first frame and labelled with the single
    number the deck shows on every one of its frames.
    """
    lecture = with_references_page(lecture)
    spans = outline_spans(
        iter_pages(lecture.children), model.outline_folds(lecture.children)
    )
    rows: list[str] = []
    if lecture.subtitle:
        rows.append(f'<p class="outline-subtitle">{escape_html(lecture.subtitle)}</p>')
    rows.append(
        '<div class="row root-row"><span class="label">'
        f'<span class="marker">▼</span>'
        f'<span class="node-title"> {_title_html(lecture.title)}</span></span></div>'
    )
    _walk_outline(lecture.children, [], rows, [0, 1], spans)
    return '<div class="outline">\n' + "\n".join(rows) + "\n</div>"


def build_outline_print_html(lecture: model.Lecture) -> str:
    """A standalone print page: the outline as one tall, content-sized PDF page.

    A fixed-size Marp slide clips a large outline, so the outline gets its own
    page instead. The page is the deck's width; an inline script grows its height
    to the laid-out content after the embedded CJK font loads, so headless Chrome
    prints exactly one page that never clips.
    """
    # The page is a fixed pixel width; the body lays out at this width both on
    # screen (where the fit script measures the height) and in print (the @page
    # size), so the measurement matches the printed layout. outline.css sizes
    # .outline with a vw-based clamp, which would resolve against a *different*
    # viewport at measure time and overflow onto a second page — so we pin the
    # outline font to a fixed px below (after OUTLINE_CSS, to win the cascade).
    width = model.RATIOS[lecture.ratio][0]
    style = (
        f'@font-face {{ font-family: "Noto Sans SC";'
        f' src: url("{CJK_FONT}") format("woff2"); }}\n'
        "@page { margin: 0; }\n"
        "html, body { margin: 0; padding: 0; }\n"
        f"body {{ width: {width}px; background: #fff;"
        ' font-family: "Lato", "Helvetica Neue", Helvetica, Arial,'
        ' "Noto Sans SC", sans-serif; }\n'
        f".print-wrap {{ box-sizing: border-box; width: {width}px;"
        " padding: 48px 56px; }\n"
        ".outline a { color: inherit; text-decoration: none; }\n"
        f"{OUTLINE_CSS}"
        "\n.outline { font-size: 18px; }\n"
    )
    script = (
        # getBoundingClientRect().height is fractional; scrollHeight floors it,
        # leaving the @page up to ~1px too short and bumping the last (unsplittable)
        # row onto a second page. Measure the fractional height and round up.
        "function fit(){"
        "var h=Math.ceil(document.body.getBoundingClientRect().height);"
        "var st=document.createElement('style');"
        f"st.textContent='@page{{size:{width}px '+h+'px;margin:0}}';"
        "document.head.appendChild(st);}"
        "if(document.fonts&&document.fonts.ready)document.fonts.ready.then(fit);"
        "else window.addEventListener('load',fit);"
    )
    return (
        f'<!doctype html>\n<html lang="{escape_html(lecture.lang or "zh")}">\n<head>\n'
        '<meta charset="utf-8" />\n'
        f"<style>\n{style}</style>\n</head>\n<body>\n"
        f'<div class="print-wrap">\n{build_outline_html(lecture)}\n</div>\n'
        f"<script>\n{script}\n</script>\n</body>\n</html>\n"
    )


def _walk_outline(
    nodes: list[model.Section | model.Page],
    ancestors_last: list[bool],
    rows: list[str],
    page_index: list[int],          # [physical slide index, shown number]
    spans: dict[str, int],
) -> None:
    # A folded animation frame or a bridge has no row, and so is not a candidate
    # for the last-child connector either.
    shown = [node for node in nodes
             if isinstance(node, model.Section) or spans.get(node.id, 1) > 0]
    shown_i = 0
    for node in nodes:
        if isinstance(node, model.Page) and spans.get(node.id, 1) == 0:
            # A bridge slide exists in the deck — only the outline omits it —
            # so the physical index later links address must step over it. A
            # folded animation frame is instead covered by its row's span.
            if model.is_bridge_page(node):
                page_index[0] += 1
            continue
        is_last = shown_i == len(shown) - 1
        rows.append(
            _outline_row(node, _outline_prefix(ancestors_last, is_last), page_index, spans)
        )
        if isinstance(node, model.Section) and node.children:
            _walk_outline(node.children, ancestors_last + [is_last], rows, page_index, spans)
        shown_i += 1


def _outline_prefix(ancestors_last: list[bool], is_last: bool) -> str:
    stem = "".join("    " if last else "│   " for last in ancestors_last)
    return stem + ("└── " if is_last else "├── ")


def _outline_row(
    node: model.Section | model.Page,
    prefix: str,
    page_index: list[int],
    spans: dict[str, int],
) -> str:
    title = f'<span class="node-title"> {_title_html(node.title)}</span>'
    page_number = ""
    if isinstance(node, model.Section):
        marker, marker_class, kind = "▼", "marker", "section-row"
        node_html = title
    else:
        marker, marker_class, kind = "✦", "icon", "page-row"
        node_html = f'<a class="node-link" href="{SLIDE_LINK_PREFIX}{page_index[0]}">{title}</a>'
        # The link addresses the physical slide; the label is the number the deck
        # shows, which counts a whole animation as one.
        page_number = f'<span class="page-number">{page_index[1]}</span>'
        page_index[0] += spans.get(node.id, 1)
        page_index[1] += 1
    return (
        f'<div class="row {kind}">'
        f'<span class="connector">{prefix}</span>'
        f'{page_number}'
        f'<span class="label"><span class="{marker_class}">{marker}</span>'
        f'{node_html}</span></div>'
    )


def render_marp_page(
    page: model.Page,
    slide_size: tuple[int, int] = model.RATIOS[model.DEFAULT_RATIO],
    *,
    reveal: bool = False,
    hold_number: bool = False,
) -> str:
    width, height = slide_size
    visible_blocks = model.select_blocks(page, "viewer", VIEWER_KINDS)
    if _is_cover_page(visible_blocks):
        return _render_cover_page(page, visible_blocks[0])
    if model.is_bridge_page(page):
        return _render_bridge_page(visible_blocks[0])
    out = []
    if hold_number:
        # The same slide, told over another page — a later frame of an animation,
        # or a page repeating the previous one's title: it shows the number that
        # page showed, and does not advance the count.
        out.extend(["<!-- _paginate: hold -->", ""])
    classes = []
    if page.gap is not None:
        classes.append("lk-gap-auto")
    if page.id == REFERENCES_PAGE_ID:
        classes.append("lk-references")
    if classes:
        out.extend([f"<!-- _class: {' '.join(classes)} -->", ""])
    out.extend([f"# {_title_html(page.title)}", ""])
    footnotes: list[str] = []
    annotation_rules: list[str] = []
    if page.gap is not None:
        out.extend(
            _render_gap_flow(
                page,
                visible_blocks,
                slide_size,
                reveal=reveal,
                footnotes=footnotes,
                annotation_rules=annotation_rules,
            )
        )
    else:
        sidenote_index = 0
        reveal_index = 0
        for block in visible_blocks:
            if model.block_held(page, block):
                # An after_frames block before the last frame: its space is
                # reserved, its footnotes and bubbles wait with it.
                out.extend(_wrap_held(render_block(block, sidenote_index=sidenote_index)))
                if block.kind == "sidenote":
                    sidenote_index += 1
                continue
            numbers: tuple[int, ...] = ()
            if block.footnotes:
                start = len(footnotes) + 1
                numbers = tuple(range(start, start + len(block.footnotes)))
                footnotes.extend(block.footnotes)
            lines = render_block(
                block, sidenote_index=sidenote_index, footnote_numbers=numbers
            )
            if block.untranslated and block.kind not in _REVEAL_SKIP:
                lines = _wrap_untranslated(lines)
            wrap = (
                reveal
                and block.kind not in _REVEAL_SKIP
                and _block_steps(page, block)
            )
            idx = reveal_index if wrap else None
            if wrap:
                out.extend(_wrap_reveal(lines, idx, items=block.reveal == "items"))
                reveal_index += 1
            else:
                out.extend(lines)
            if page.show_annotations:
                for note in block.annotations:
                    uid = f"{page.id}-{len(annotation_rules)}"
                    aside, rule = annotation_markup(note, uid, width, height)
                    if idx is not None:
                        aside = _tag_reveal(aside, idx)
                    out.extend([aside, ""])
                    annotation_rules.append(rule)
            if block.kind == "sidenote":
                sidenote_index += 1
    if footnotes:
        out.append(render_footnotes(footnotes))
    if annotation_rules:
        out.append("<style>\n" + "\n".join(annotation_rules) + "\n</style>")
    for block in model.select_blocks(page, "viewer", {"notes"}):
        # A blank line before the comment forces it to start its own HTML block.
        # Without it, markdown-it folds the ``<!--`` into a preceding block-level
        # HTML element (e.g. the footnotes ``<aside>``); a blank line inside the
        # note content then closes that block early and the rest of the note
        # spills onto the slide instead of staying a presenter note.
        out.append("")
        out.append(_presenter_note(block.content))
    return "\n".join(out).rstrip()


def _is_cover_page(blocks: list[model.Block]) -> bool:
    return len(blocks) == 1 and blocks[0].kind == "cover"


def _render_bridge_page(block: model.Block) -> str:
    """A transition slide: its lines centered, no title, no number.

    ``_paginate: skip`` both hides the page number and keeps Marp's count from
    advancing, matching :func:`model.slide_numbers` — the pages around a bridge
    stay consecutively numbered on the projector and in the outline alike. Like
    a cover, the page bypasses reveal mode entirely and arrives fully lit.
    """
    return "\n".join([
        "<!-- _class: lk-bridge -->",
        "<!-- _paginate: skip -->",
        "",
        *render_block(block),
    ]).rstrip()


def _render_cover_page(page: model.Page, block: model.Block) -> str:
    cover = block.content
    out = [
        "<!-- _class: cover -->",
        "<!-- _paginate: false -->",
        "",
        '<div class="lk-cover">',
    ]
    logos = _cover_logos_html(cover.get("logo"))
    if logos:
        out.append(logos)
    out.extend([
        '<div class="lk-cover-main">',
        f'<h1 class="lk-cover-title">{_title_html(page.title)}</h1>',
    ])
    meta = []
    if cover.get("author"):
        meta.append(
            f'<div class="lk-cover-author">{escape_html(cover["author"])}</div>'
        )
    if cover.get("time"):
        meta.append(
            f'<div class="lk-cover-time">{escape_html(cover["time"])}</div>'
        )
    if meta:
        out.append('<div class="lk-cover-meta">' + "".join(meta) + "</div>")
    out.extend(["</div>", "</div>"])
    return "\n".join(out)


def _cover_logos_html(logo) -> str:
    if logo is None:
        return ""
    if isinstance(logo, str):
        return (
            '<div class="lk-cover-logos lk-cover-logos--single">'
            f'<img class="lk-cover-logo lk-cover-logo--center" '
            f'src="{escape_html(logo)}" alt="" />'
            "</div>"
        )
    parts = []
    for slot in ("left", "right"):
        src = logo.get(slot)
        if src:
            parts.append(
                f'<img class="lk-cover-logo lk-cover-logo--{slot}" '
                f'src="{escape_html(src)}" alt="" />'
            )
    if not parts:
        return ""
    return '<div class="lk-cover-logos lk-cover-logos--dual">' + "".join(parts) + "</div>"


def copy_assets(asset_root: Path, output_dir: Path) -> None:
    source = Path(asset_root, "assets")
    if not source.exists():
        return
    destination = Path(output_dir, "assets")
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination)


def copy_borrowed_assets(borrowed: tuple[model.Borrowed, ...], output_dir: Path) -> None:
    """Each review source's ``assets/`` under ``assets/<its lecture id>/``.

    The namespace matches the src rewrite in ``review.rebrand``, and keeps the
    bundle self-contained: a borrowed figure is a file in this bundle, not a
    path back into another lecture's directory.
    """
    for entry in borrowed:
        source = Path(entry.directory, "assets")
        if not source.exists():
            continue
        destination = Path(output_dir, "assets", entry.lecture_id)
        if destination.exists():
            shutil.rmtree(destination)
        shutil.copytree(source, destination)


def render_index_html(title: str, embedded_json: str, lang: str | None = None) -> str:
    template = (ASSETS / "index.html").read_text(encoding="utf-8")
    return template.format(
        title=escape_html(title),
        # The shell's own language, so hyphenation and screen readers get an
        # English deck right. Falls back to the historical `zh`.
        lang=escape_html(lang or "zh"),
        data=embedded_json.replace("</", "<\\/"),
    )


def escape_html(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _title_html(text: str) -> str:
    parts: list[str] = []
    last = 0
    for match in _TITLE_MARK_RE.finditer(text):
        parts.append(escape_html(text[last:match.start()]))
        parts.append(
            '<span class="title-parenthetical">'
            f"{escape_html(match.group(1))}</span>"
        )
        last = match.end()
    parts.append(escape_html(text[last:]))
    return "".join(parts)
