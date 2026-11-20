from __future__ import annotations

import json
import re
import shutil
from dataclasses import dataclass, replace
from pathlib import Path

from lecturekit import demo, i18n, model, references, source, tokens, variables
from .blocks import (
    MarpCtx, annotation_markup, escape_html, render_block, render_footnotes,
)
from .marp import CJK_FONT
from .pdf import SLIDE_LINK_PREFIX

ASSETS = Path(__file__).resolve().parent / "assets"
REFERENCES_PAGE_ID = "__references__"
OUTLINE_CSS = (ASSETS / "outline.css").read_text(encoding="utf-8")

_TITLE_MARK_RE = re.compile(r"\\emph\{([^{}]+)\}")

# Where an alignment puts content in a flex box.
_CSS_ALIGN = {"left": "flex-start", "center": "center", "right": "flex-end",
              "top": "flex-start", "middle": "center", "bottom": "flex-end"}
# Where a column of the body puts its blocks under the page's valign. `safe`
# keeps a column taller than the body at its top, overflowing downward, as a
# top-aligned column does.
_COLUMN_JUSTIFY = {"middle": "safe center", "bottom": "safe flex-end"}


@dataclass
class _Drawn:
    """One block drawn, and the state the box around it carries."""
    lines: list[str]
    held: bool = False
    step: int | None = None
    items: bool = False
    wash: bool = False


def _state_attrs(drawn: _Drawn, base: str = "") -> str:
    """The attributes of the box that carries a drawn block's state.

    A held block (an ``after_frames`` block before the last frame) keeps its
    space but is invisible, and ``aria-hidden`` keeps a screen reader from
    announcing it on every frame. A reveal step carries its index, and
    ``data-reveal-items`` when the block steps through its own list items
    (`p.slide(..., reveal="items")`): `reveal.js` splits the rendered `<li>`s
    in the browser, so the flag is all the build owes it.
    """
    classes = [base] if base else []
    extra = ""
    if drawn.held:
        classes.append("lk-held")
        extra = ' aria-hidden="true"'
    elif drawn.step is not None:
        classes.append("reveal-block")
        extra = f' data-reveal="{drawn.step}"'
        if drawn.items:
            extra += ' data-reveal-items="1"'
    return f'class="{" ".join(classes)}"{extra}'


def _wrap(lines: list[str], opening: str) -> list[str]:
    """``lines`` inside the ``<div>`` that ``opening`` opens.

    Blank lines around the inner content keep Marp parsing it as markdown rather
    than raw HTML (a CommonMark type-6 HTML block ends at a blank line).
    """
    body = list(lines)
    while body and body[-1] == "":
        body.pop()
    return [opening, "", *body, "", "</div>", ""]


def _style(declarations: list[str]) -> str:
    return f' style="{"; ".join(declarations)};"' if declarations else ""


class _PageDraw:
    """The body of one content page and what it leaves to the footnote band.

    A content page is three bands, top to bottom: the title, the body and the
    footnote band. The body is the main column, and with a ``side_image`` the
    image column beside it; each column sets its blocks at the page's valign.
    Every block passes through :meth:`leaf`, the one place that numbers its
    footnotes, holds it back on an animation's early frames, gives it a reveal
    step, marks it untranslated and collects its callouts.
    """

    def __init__(self, page: model.Page, slide_size: tuple[int, int], reveal: bool):
        self.page = page
        self.width, self.height = slide_size
        self.reveal = reveal
        self.ctx = MarpCtx()
        self.footnotes: list[str] = []
        # Callout bubbles are placed in slide coordinates, so they are drawn
        # outside the body, with the rules that place them.
        self.overlays: list[str] = []
        self.rules: list[str] = []
        self.steps = 0

    def number(self, texts: tuple[str, ...]) -> tuple[int, ...]:
        """Append ``texts`` to the page's footnotes; the numbers they now carry."""
        start = len(self.footnotes) + 1
        self.footnotes.extend(texts)
        return tuple(range(start, start + len(texts)))

    def leaf(self, block: model.Block, closers=(), *, steps: bool = True) -> _Drawn:
        """Draw one block. ``closers`` are the containers it ends: a container's
        footnote superscript rides the last leaf drawn inside it. ``steps`` is
        False in the image column, which is never a reveal step."""
        page = self.page
        held = model.block_held(page, block)
        numbers: tuple[int, ...] = ()
        if not held:
            # A held block's footnotes and bubbles wait with it.
            numbers = self.number(block.footnotes)
            for container in closers:
                numbers += self.number(container.footnotes)
        lines = render_block(block, footnote_numbers=numbers, ctx=self.ctx)
        step = None
        if (steps and not held and self.reveal and block.REVEAL_STEP
                and _block_steps(page, block)):
            step = self.steps
            self.steps += 1
        if not held and page.show_annotations:
            for note in block.annotations:
                uid = f"{page.id}-{len(self.rules)}"
                aside, rule = annotation_markup(note, uid, self.width, self.height)
                self.overlays.extend([aside if step is None else _tag_reveal(aside, step), ""])
                self.rules.append(rule)
        # The wash says a translation fell back to the baseline. A rehearsal
        # aid, so only the deck draws it; a `side_image` or a `spacer` (no
        # reveal step) has no text to wash.
        return _Drawn(lines, held, step, block.reveal == "items",
                      block.untranslated and block.REVEAL_STEP)

    @staticmethod
    def washed(drawn: _Drawn) -> list[str]:
        if drawn.wash:
            return _wrap(drawn.lines, '<div class="lk-untranslated">')
        return drawn.lines

    def bare(self, drawn: _Drawn) -> list[str]:
        """A block set directly in its column: a wrapper only for its state."""
        lines = self.washed(drawn)
        if drawn.held or drawn.step is not None:
            # A reveal wrapper is layout-transparent (`display: contents`).
            lines = _wrap(lines, f"<div {_state_attrs(drawn)}>")
        return lines

    def box(self, node, inherited, *, in_row=False, share=None, closers=()) -> list[str]:
        """An element in a box: the box carries its alignment and its state.

        A box owns alignment, not its drawer: text, code and figures all travel
        through this same path, and a container is a box of boxes.
        """
        halign, valign = model.resolve_align(node, inherited)
        attrs = 'class="lk-box"'
        if isinstance(node, model.Flex):
            body = []
            last = len(node.children) - 1
            row = node.direction == "row"
            shares = model.row_shares(node) if row else [None] * len(node.children)
            for i, child in enumerate(node.children):
                body.extend(self.box(
                    child, (halign, valign), in_row=row, share=shares[i],
                    closers=(node, *closers) if i == last else (),
                ))
            justify = _CSS_ALIGN.get(halign if row else valign, "flex-start")
            content = [f'<div class="lk-content lk-flex lk-flex-{node.direction}" '
                       f'style="justify-content:{justify}">', "", *body, "</div>", ""]
        else:
            block, content_style = node.sized_in_box(in_row)
            drawn = self.leaf(block, closers)
            if drawn.held or drawn.step is not None:
                attrs = _state_attrs(drawn, "lk-box")
            wash = " lk-untranslated" if drawn.wash else ""
            content = [f'<div class="lk-content{wash}"{content_style}>', "",
                       *drawn.lines, "</div>", ""]
        default_halign = ("center" if isinstance(node, model.Block) and node.CENTERED
                          else "stretch")
        style = (f'align-items:{_CSS_ALIGN.get(halign, default_halign)};'
                 f'justify-content:{_CSS_ALIGN.get(valign, "flex-start")}')
        # In a row a sized image keeps its requested width. Every other child
        # shares the remaining width; this works for nested containers too.
        if in_row and isinstance(node, model.Block) and node.row_basis():
            style += f';--lk-basis:{node.row_basis()}'
        elif share is not None:
            style += f';--lk-grow:{100 * share:.2f}'
        return [f'<div {attrs} style="{style}">', "", *content, "</div>", ""]

    def main(self, blocks: list[model.Block], valign: str | None) -> list[str]:
        """The main column: the page's blocks, at its valign and halign.

        A page that aligns anything or holds a container draws every element
        in a box; any other page sets its blocks directly in the column.
        ``p.gap(...)`` puts a spacer between each two top-level elements, each
        element one unit (`.lk-gap-block`) that leaves the spacing to them.
        """
        page = self.page
        boxed = page.halign is not None or page.valign is not None or any(
            block.kind in model.MARKER_KINDS or block.halign is not None
            or block.valign is not None for block in blocks
        )
        declarations = []
        if valign in _COLUMN_JUSTIFY:
            declarations.append(f"justify-content: {_COLUMN_JUSTIFY[valign]}")
        gap = page.gap
        if gap is not None:
            # `p.gap("fill")` lifts the cap: a seam takes whatever is left.
            cap = "none" if gap.max_px is None else f"{gap.max_px}px"
            declarations += [f"--lk-gap-min: {gap.min_px}px", f"--lk-gap-max: {cap}"]
        out = [f'<div class="lk-main"{_style(declarations)}>', ""]
        aligned = (page.halign, valign)
        for index, node in enumerate(model.layout(blocks)):
            if gap is None:
                out.extend(self.box(node, aligned) if boxed else self.bare(self.leaf(node)))
                continue
            if boxed:
                # The box carries the state; a gap unit only groups it.
                lines = self.box(node, aligned)
                unit = '<div class="lk-gap-block">'
            else:
                # The gap unit is the block's box, so it carries the state.
                drawn = self.leaf(node)
                lines = self.washed(drawn)
                unit = f"<div {_state_attrs(drawn, 'lk-gap-block')}>"
            if index:
                out.extend(['<div class="lk-gap-spacer" aria-hidden="true"></div>', ""])
            out.extend(_wrap(lines, unit))
        return [*out, "</div>", ""]

    def side(self, sides: list[model.Block], texts: list[model.Block],
             valign: str | None) -> list[str]:
        """The image column: the side images, top to bottom, then the slide text
        set under them (``under_side_image``), at the column's width.

        The first image sets the column's width, its share of the body.
        """
        body_width = self.width - 2 * tokens.px("--slide-padding-x")
        width = model.side_column_px(sides[0].content.get("width"), body_width)
        declarations = [f"--lk-side-width: {width}px"]
        if valign in _COLUMN_JUSTIFY:
            declarations.append(f"justify-content: {_COLUMN_JUSTIFY[valign]}")
        out = [f'<div class="lk-side"{_style(declarations)}>', ""]
        for block in (*sides, *texts):
            out.extend(self.bare(self.leaf(block, steps=False)))
        return [*out, "</div>", ""]

    def body(self, blocks: list[model.Block]) -> list[str]:
        """The body: the main column, and the image column on the side the
        first ``side_image`` names. The image column's footnotes come first."""
        sides = [block for block in blocks if block.kind == "side_image"]
        # Slide text set under the image leaves the main column. With no image
        # column in this target, it stays where it was written.
        texts = [block for block in blocks if block.in_side] if sides else []
        main = [block for block in blocks
                if block.IN_FLOW and not (texts and block.in_side)]
        valign = model.page_valign(self.page, blocks)
        side = self.side(sides, texts, valign) if sides else []
        column = self.main(main, valign)
        left = bool(sides) and sides[0].content.get("side") == "left"
        return ['<div class="lk-body">', *(side + column if left else column + side),
                "</div>", ""]


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
        # deleted demo from staying runnable in a live session. The file table
        # beside it is rewritten on the same terms.
        demo.write(lecture, output_dir)
        source.write(lecture, output_dir)

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
            build_marp_markdown(lecture, reveal=self.reveal, paginate=self.paginate,
                                asset_root=self.asset_root),
            encoding="utf-8",
        )
        Path(output_dir, "outline.html").write_text(
            build_outline_print_html(lecture), encoding="utf-8"
        )


def build_data(lecture: model.Lecture) -> dict:
    lecture = with_references_page(lecture)
    pages = model.flatten_pages(lecture.children)
    spans = outline_spans(pages, model.outline_folds(lecture.children))
    return {
        "lecture": {"id": lecture.id, "title": lecture.title, "subtitle": lecture.subtitle},
        # lecturekit's own chrome, resolved here so `viewer.js` prints the deck's
        # language instead of hardcoding one. See `i18n.UI_STRINGS`.
        "ui": i18n.ui_table(lecture.lang),
        "tree": build_tree(lecture.children, spans),
        "pages": [build_page(page) for page in pages],
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


def with_references_page(lecture: model.Lecture) -> model.Lecture:
    """Return ``lecture`` with a trailing references page, if any page cited work.

    Pure: a new ``Lecture`` is returned (the input is never mutated), so every
    render entry point can call it independently and get the same deck. The page
    is a normal top-level page, so it flows through the outline, viewer
    navigation, slide numbering, and PDF like any other. No citations → the
    lecture is returned unchanged.
    """
    pages = model.flatten_pages(lecture.children)
    if any(page.id == REFERENCES_PAGE_ID for page in pages):
        raise model.ValidationError(
            f"page id {REFERENCES_PAGE_ID!r} is reserved for the auto-generated "
            "references page"
        )
    entries = references.collect_citations(pages)
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


def build_page(page: model.Page) -> dict:
    return {
        "id": page.id,
        "title": page.title,
        "blocks": [
            {"kind": block.kind, "content": block.content}
            for block in model.select_blocks(page, "viewer")
        ],
    }


def build_marp_markdown(
    lecture: model.Lecture,
    theme: str = "basic-office",
    *,
    reveal: bool = False,
    paginate: bool = True,
    asset_root: Path | None = None,
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
    # The deck spells each variable as a CSS custom property and marks the
    # pictures of bound figures (`variables.for_deck`); a page that uses a
    # variable also carries its definition and the source of its bound
    # figures, which `vars.js` reads (`variables.page_table`). Bound figures
    # are found under `asset_root`, the lecture directory.
    bound = variables.figures(lecture, asset_root) if lecture.variables else {}
    pages = model.flatten_pages(lecture.children)
    deck_pages = model.flatten_pages(variables.for_deck(lecture, bound).children)
    slides = []
    for page, deck_page in zip(pages, deck_pages):
        slide = render_marp_page(deck_page, slide_size,
                                 reveal=reveal and _steps_through(deck_page))
        table = variables.page_table(lecture, page, bound)
        slides.append(slide if table is None else f"{slide}\n\n{table}")
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
    frame, held invisible (see ``model.block_held``). On a page without an animation
    every block steps. On a single-frame animation the first frame *is* the
    last, so both halves step there, in author order.
    """
    group = page.frame_group
    if group is None:
        return True
    if block.after_frames:
        return group.index == group.total
    return group.index == 1


def build_outline_html(lecture: model.Lecture) -> str:
    """Reproduce the viewer's navigation tree as a static, fully-expanded page.

    Each page row's title is an anchor to its slide (``SLIDE_LINK_PREFIX`` + the
    flat page index, in deck order); the merged-PDF step rewrites those URIs into
    internal page links. Section rows are not links — they have no slide. An
    animation, or a run of same-titled pages, is one row, linking to its first
    slide and labelled with the number that slide prints.
    """
    lecture = with_references_page(lecture)
    spans = outline_spans(
        model.flatten_pages(lecture.children), model.outline_folds(lecture.children)
    )
    rows: list[str] = []
    if lecture.subtitle:
        rows.append(f'<p class="outline-subtitle">{escape_html(lecture.subtitle)}</p>')
    rows.append(
        '<div class="row root-row"><span class="label">'
        f'<span class="marker">▼</span>'
        f'<span class="node-title"> {_title_html(lecture.title)}</span></span></div>'
    )
    _walk_outline(lecture.children, [], rows, [0], spans)
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
    page_index: list[int],          # [0-based slide index]
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
        # The label is the number the row's first slide prints: its 1-based
        # position in the deck.
        page_number = f'<span class="page-number">{page_index[0] + 1}</span>'
        page_index[0] += spans.get(node.id, 1)
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
) -> str:
    """One page as Marp markdown.

    A content page is three bands, top to bottom: the title, the body and the
    footnote band (see `_PageDraw`). The cover and a bridge page own the whole
    slide and are drawn by their own blocks.
    """
    visible_blocks = model.select_blocks(page, "viewer")
    if _is_cover_page(visible_blocks):
        return _render_cover_page(page, visible_blocks[0])
    if model.is_bridge_page(page):
        return _render_bridge_page(visible_blocks[0])
    out = []
    if page.id == REFERENCES_PAGE_ID:
        out.extend(["<!-- _class: lk-references -->", ""])
    out.extend([f"# {_title_html(page.title)}", ""])
    draw = _PageDraw(page, slide_size, reveal)
    out.extend(draw.body(visible_blocks))
    out.extend([render_footnotes(draw.footnotes), ""])
    out.extend(draw.overlays)
    if draw.rules:
        out.append("<style>\n" + "\n".join(draw.rules) + "\n</style>")
    for block in model.select_blocks(page, "viewer", {"notes"}):
        # A blank line before the comment forces it to start its own HTML block.
        # Without it, markdown-it folds the ``<!--`` into a preceding block-level
        # HTML element (e.g. the footnote band); a blank line inside the note
        # content then closes that block early and the rest of the note spills
        # onto the slide instead of staying a presenter note.
        out.append("")
        out.append(_presenter_note(block.content))
    return "\n".join(out).rstrip()


def _is_cover_page(blocks: list[model.Block]) -> bool:
    return len(blocks) == 1 and blocks[0].kind == "cover"


def _render_bridge_page(block: model.Block) -> str:
    """A transition slide: its lines centered, no title, no number.

    ``_paginate: false`` hides the page number but, like a cover, still counts
    the slide, so every page after it prints its position in the deck. Like a
    cover, the page bypasses reveal mode entirely and arrives fully lit.
    """
    return "\n".join([
        "<!-- _class: lk-bridge -->",
        "<!-- _paginate: false -->",
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
    origin = Path(asset_root, "assets")
    if not origin.exists():
        return
    destination = Path(output_dir, "assets")
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(origin, destination)


def copy_borrowed_assets(borrowed: tuple[model.Borrowed, ...], output_dir: Path) -> None:
    """Each review source's ``assets/`` under ``assets/<its lecture id>/``.

    The namespace matches the src rewrite in ``review.rebrand``, and keeps the
    bundle self-contained: a borrowed figure is a file in this bundle, not a
    path back into another lecture's directory.
    """
    for entry in borrowed:
        origin = Path(entry.directory, "assets")
        if not origin.exists():
            continue
        destination = Path(output_dir, "assets", entry.lecture_id)
        if destination.exists():
            shutil.rmtree(destination)
        shutil.copytree(origin, destination)


def render_index_html(title: str, embedded_json: str, lang: str | None = None) -> str:
    template = (ASSETS / "index.html").read_text(encoding="utf-8")
    return template.format(
        title=escape_html(title),
        # The shell's own language, so hyphenation and screen readers get an
        # English deck right. Falls back to the historical `zh`.
        lang=escape_html(lang or "zh"),
        data=embedded_json.replace("</", "<\\/"),
    )


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
