from __future__ import annotations

import re
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any

from . import marks


# Deck-level aspect-ratio presets, anchored on a fixed 720px height so the pixel
# sizes match PowerPoint conventions (e.g. 4:3 == 10in x 7.5in == 960 x 720 at
# 96dpi). The whole deck shares one ratio; Marp's slide canvas is per-deck, not
# per-slide.
RATIOS = {
    "16:9": (1280, 720),
    "4:3": (960, 720),
    "16:10": (1152, 720),
    "3:2": (1080, 720),
}
DEFAULT_RATIO = "16:9"

BLOCK_KINDS = frozenset({
    "cover", "slide", "notes", "prose", "code", "link",
    "image", "side_image", "sidenote", "demo", "aside",
    "table", "architecture", "row", "spacer", "highlight",
    "bridge",
})

# A bridge (衔接页) is one or two short lines — three is the hard ceiling, so it
# cannot grow into a title-less free-form page. See dsl.Lecture.bridge.
BRIDGE_MAX_LINES = 3

NEWS_KINDS = frozenset({"news", "paper", "blog", "video", "doc"})

# How finely a block steps in the live preview's reveal-on-Enter mode.
#   "block" — one Enter lights the whole block (default)
#   "items" — one Enter per list item, so the bullets arrive one at a time
# Only a `slide` block may ask for "items", and only the deck's live preview
# reads it: every other target renders the block whole. See dsl.PageBuilder.slide.
REVEAL_MODES = frozenset({"block", "items"})
DEFAULT_REVEAL_MODE = "block"

# A markdown list item at the top level of a block: `- x`, `* x`, `+ x`, `1. x`,
# `1) x`, indented no more than the three spaces markdown allows before the
# marker. What `reveal="items"` steps through.
LIST_ITEM_RE = re.compile(r"(?m)^ {0,3}(?:[-*+]|\d+[.)])\s+\S")

# How the book (LaTeX) target treats a page. The deck never looks at this.
#   "page"  — its own heading (default)
#   "merge" — fold its book-visible blocks into the previous page's section
#   "skip"  — leave it out of the book entirely
BOOK_MODES = frozenset({"page", "merge", "skip"})

# Block kinds that may carry a figure `ref` (an author-chosen anchor that prose
# cites with `[@name]`). All render as numbered LaTeX figures, which is where
# the `\label` lands — so a ref requires a caption.
REF_KINDS = frozenset({"image", "row", "architecture"})

_REF_NAME_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9_-]*")

# Directional flow drawn between adjacent layers of an architecture diagram.
# None renders plain stacked bands; the others draw a chevron between each pair.
ARCH_FLOWS = frozenset({"down", "up", "both"})

# A module box meaning "more modules here, not drawn". Authored as the Python
# ellipsis literal ``...`` (or the string ``"..."``/``"…"``) and normalized to
# this single character so the renderer can fade it.
ARCH_ELLIPSIS = "…"

# Wash colors a `highlight` chip can carry. A closed set, so a deck stays
# consistent — the hex values live in the themes, not here. It is literally the
# inline `<mark>` set (see marks.TONES): a chip and a marked keyword are the
# same gesture at two scales, so they cannot be allowed to drift apart.
HIGHLIGHT_TONES = marks.TONES
DEFAULT_HIGHLIGHT_TONE = marks.DEFAULT_TONE

# Nine slide-relative reference points an annotation bubble can attach to. The
# bubble floats over the slide (nothing measures a block's laid-out position at
# build time), so the author names where it lands; dx/dy nudge it from there.
ANNOTATION_ANCHORS = frozenset({
    "top-left", "top", "top-right",
    "left", "center", "right",
    "bottom-left", "bottom", "bottom-right",
})

class ValidationError(ValueError):
    """Raised when a lecture AST violates renderer-neutral invariants."""


@dataclass(frozen=True)
class Annotation:
    """A callout bubble floating over the slide, attached to a block.

    Placed by author coordinates rather than measured geometry: ``at`` is one of
    ``ANNOTATION_ANCHORS`` and ``dx``/``dy`` are pixel nudges from that anchor.
    A page may hide all of its bubbles via ``Page.show_annotations``.
    """
    text: str
    at: str = "top-right"
    dx: int = 0
    dy: int = 0


@dataclass(frozen=True)
class NewsItem:
    title: str
    url: str
    source: str | None = None
    date: str | None = None
    kind: str = "news"
    why: str | None = None
    tags: tuple[str, ...] = ()
    image: str | None = None
    archived_url: str | None = None


@dataclass(frozen=True)
class Citation:
    title: str
    author: str | None = None
    year: str | None = None
    venue: str | None = None
    url: str | None = None
    # Dedup identity across pages; when None, references.collect_citations falls
    # back to a slug of title+year. See dsl.PageBuilder.cite.
    key: str | None = None


# The cap a bare ``p.gap()`` takes. The legacy ``p.gap("auto")`` spelling keeps
# the smaller cap it has always carried, so old pages render unchanged.
DEFAULT_GAP_MIN_PX = 8
DEFAULT_GAP_MAX_PX = 52
LEGACY_GAP_MAX_PX = 28
# The word ``p.gap(...)`` takes instead of a cap to lift it entirely.
GAP_FILL = "fill"


@dataclass(frozen=True)
class PageGap:
    mode: str = "auto"
    min_px: int = DEFAULT_GAP_MIN_PX
    # None is ``p.gap("fill")``: no cap, so the seams split whatever is left.
    max_px: int | None = DEFAULT_GAP_MAX_PX


@dataclass(frozen=True)
class FrameGroup:
    """Marks one slide of an animation: same page, one figure per frame.

    A page whose body calls ``p.frames(a, b, c)`` is expanded at build time into
    ``total`` sibling pages that share every block but that one figure. ``id`` is
    the page id the author wrote — the frames themselves are ``<id>-1`` …
    ``<id>-N`` — and ``index`` is 1-based. Downstream everything is an ordinary
    page; only three places read this: the outline (folds a group into one
    entry), the deck's reveal mode (only the first frame steps), and a citation
    backref (reported once, at the group's first slide). See dsl.PageBuilder.frames.
    """
    id: str
    index: int
    total: int


@dataclass(frozen=True)
class Block:
    kind: str
    content: Any
    only: set[str] | None = None
    except_: set[str] | None = None
    # An author-pinned name for this block in a translation overlay, replacing
    # the positional `<kind>.<n>` an auto key would use — so reordering a page
    # does not renumber the entry a translator already filled in. Local to the
    # page (the full key is `<page id>.<key>`), so it carries no dot. See
    # `i18n.block_key`.
    key: str | None = None
    # Set by `i18n.apply` when one of this block's strings had no overlay entry
    # and fell back to the baseline. Only the viewer reads it, to wash the block
    # so a missing translation is visible during rehearsal.
    untranslated: bool = False
    # When True, this block renders in no target — not the deck, not the book,
    # not PPTX. `select_blocks` drops it before any only/except/table logic, so
    # it is an unconditional off switch. See dsl.BlockHandle.disable. On a prose
    # block this is exactly "written but held back from the book".
    disabled: bool = False
    # Footnotes annotate the block; each renders as small bottom-left text with a
    # matching superscript on the block. Not a block kind — any block may carry
    # several. See dsl.BlockHandle.footnote.
    footnotes: tuple[str, ...] = ()
    # Callout bubbles floating over the slide near this block. See
    # dsl.BlockHandle.annotate and Annotation.
    annotations: tuple[Annotation, ...] = ()
    # A small image floated to the right of a slide block's text; the text
    # wraps around it. Only set on slide blocks. See dsl.BlockHandle.image_right.
    float_image: dict | None = None
    # How finely this block steps in the live preview's reveal-on-Enter mode;
    # one of REVEAL_MODES. Only a slide block may carry "items". Live preview
    # only — no exported target reads it. See dsl.PageBuilder.slide.
    reveal: str = DEFAULT_REVEAL_MODE
    # Whether this block's flush-left prose lines were auto-bolded. Only a
    # slide block sets it, and only at authoring time — the content stored here
    # already carries (or deliberately lacks) the `**...**`. It is kept so that
    # a translation overlay can put a replacement through the *same* rule the
    # author chose, rather than re-bolding a block written with
    # `p.slide(..., autobold=False)`. See dsl.PageBuilder.slide.
    autobold: bool = True
    # Set on the blocks an author wrote *after* ``p.frames(...)``: they belong
    # to the animation's finished picture, so only its last frame shows them —
    # the punchline lands when the animation has played, not before. Earlier
    # frames keep the blocks' space blank (see ``block_held``), so nothing above
    # shifts between frames. Derived at expansion time, never authored; always
    # False on a page without an animation.
    after_frames: bool = False


@dataclass(frozen=True)
class Page:
    id: str
    title: str
    blocks: list[Block] = field(default_factory=list)
    tags: set[str] = field(default_factory=set)
    # When False, this page renders none of its blocks' annotation bubbles. Lets
    # an author duplicate a page (same body) with the bubbles off then on to
    # reveal them as a build step. See dsl.PageBuilder via Lecture.page.
    show_annotations: bool = True
    # Student-facing companion reading material. Renderers ignore this unless
    # they explicitly implement a companion surface such as notebook panels.
    news: tuple[NewsItem, ...] = ()
    # References cited on this page. Not slide blocks: they surface only in the
    # deck's trailing 参考文献 page and the book's chapter-end 参考文献 section.
    # See dsl.PageBuilder.cite and references.collect_citations.
    citations: tuple[Citation, ...] = ()
    # Optional page-level vertical spacing policy. Viewer/Marp may honor this;
    # renderers without adaptive layout support can ignore it.
    gap: PageGap | None = None
    # Book-side treatment of this page; one of BOOK_MODES. Deck-only concern
    # stays out of this: the viewer/Marp/PPTX renderers never read it.
    book: str = "page"
    # Book-only heading override. The deck and the outline keep `title`.
    book_title: str | None = None
    # Set when this page is one frame of an animation; None for a normal page.
    frame_group: FrameGroup | None = None


@dataclass(frozen=True)
class Section:
    id: str
    title: str
    children: list[Section | Page] = field(default_factory=list)
    collapsed: bool = False


@dataclass(frozen=True)
class Borrowed:
    """A lecture this one borrows review pages from (see ``dsl.review_section``).

    ``lecture_id`` is the *source* lecture's own id, which namespaces both the
    borrowed page ids (``<lecture_id>/<page id>``) and their assets
    (``assets/<lecture_id>/…``). ``directory`` is where that lecture was loaded
    from — renderers copy its ``assets/`` in, and the dev server watches it.
    """
    lecture_id: str
    directory: str


@dataclass(frozen=True)
class Lecture:
    id: str
    title: str
    subtitle: str | None = None
    children: list[Section | Page] = field(default_factory=list)
    ratio: str = DEFAULT_RATIO
    # Source lectures whose pages this one replays as review. Empty for the
    # common case; see `Borrowed` and `dsl.review_section`.
    borrowed: tuple[Borrowed, ...] = ()
    # The translation overlay actually applied (`i18n/<lang>.toml`), or None for
    # the baseline Python. Renderers read it only to pick their own fixed
    # strings; nothing else in the tree remembers that languages exist.
    lang: str | None = None
    # Full i18n keys whose overlay entry was missing, so the baseline text is
    # what renders. `--strict` refuses to render a lecture with any. See `i18n`.
    untranslated: tuple[str, ...] = ()


def resolve_asset(src: str, asset_root: Path | None, borrowed: tuple[Borrowed, ...] = ()) -> Path:
    """The on-disk path an image ``src`` names, honoring borrowed namespaces.

    A borrowed page's srcs were rewritten to ``assets/<source id>/…`` so the
    output bundle can hold two lectures' figures side by side; on disk that
    prefix means ``<source dir>/assets/…`` instead. Renderers that read the
    original file (pptx, latex) go through here; the viewer copies whole trees
    and needs no lookup.
    """
    path = Path(src)
    if path.is_absolute():
        return path
    for entry in borrowed:
        prefix = f"assets/{entry.lecture_id}/"
        if src.startswith(prefix):
            return Path(entry.directory, "assets", src[len(prefix):])
    return Path(asset_root, src) if asset_root is not None else path


def select_blocks(page: Page, target: str, kinds: set[str]) -> list[Block]:
    """Blocks of `page` that renderer `target` (handling `kinds`) should emit.

    The author's `only`/`except_` override sits *on top of* the renderer's own
    table: `only={target}` forces a block in even when the renderer does not
    normally render its kind (a `slide` pulled into the book, say), and
    `except_` forces one out. Absent an override, the table decides.

    A `disabled` block is dropped first of all, ahead of every override — it
    renders in no target at all.
    """
    selected: list[Block] = []
    for block in page.blocks:
        if block.disabled:
            continue
        if block.except_ and target in block.except_:
            continue
        if block.only is not None:
            if target in block.only:
                selected.append(block)
            continue
        if block.kind not in kinds:
            continue
        selected.append(block)
    return selected


def is_bridge_page(page: Page) -> bool:
    """A transition page: one ``bridge`` block and nothing else.

    Only ``dsl.Lecture.bridge`` / ``SectionBuilder.bridge`` build such a page,
    and validation refuses a ``bridge`` block with company — so the single-block
    test is the page's identity, no page-level flag needed (the same inference
    the cover page uses).
    """
    return len(page.blocks) == 1 and page.blocks[0].kind == "bridge"


def flatten_pages(children: list[Section | Page]) -> list[Page]:
    """The lecture's pages in deck order (depth-first, left to right)."""
    pages: list[Page] = []
    for child in children:
        if isinstance(child, Page):
            pages.append(child)
        else:
            pages.extend(flatten_pages(child.children))
    return pages


def frame_groups(pages: list[Page]) -> dict[str, list[int]]:
    """``group id -> the 0-based positions in ``pages`` belonging to it``.

    The one primitive every group-aware surface is built on: the outline folds a
    group into its first position, a citation backref reports that position, and
    ``--pages <group id>`` selects them all. Pages carrying no group are absent.
    Positions are whatever is in ``pages``, so a pruned deck (``--pages``) yields
    the frames that survived, not the ones the author wrote.
    """
    groups: dict[str, list[int]] = {}
    for position, page in enumerate(pages):
        if page.frame_group is not None:
            groups.setdefault(page.frame_group.id, []).append(position)
    return groups


def outline_folds(children: list[Section | Page]) -> frozenset[str]:
    """Page ids whose outline row folds into the page before it.

    Two consecutive sibling pages titled the same are one idea told over two
    slides — a static page and the animation that redraws it, or an annotation
    reveal pair — so they get one outline row and one shown slide number, the
    same treatment an animation's frames already get. An animation counts as one
    unit here: its frames share a title, and it is the group as a whole that
    folds into its neighbour (or is folded into by it).

    Siblings only, and adjacent in their own list: a run reaching across a
    section boundary would fold the next section's first page into the previous
    section's last row, leaving that section opening with no row at all. A bridge
    between two pages breaks the run too — a breath between topics says they are
    not one idea.

    An author who wants two rows gives the two pages two titles; that is the
    whole escape hatch, because a shared title is the claim being read here.
    """
    folded: set[str] = set()
    _collect_folds(children, folded)
    return frozenset(folded)


def _collect_folds(children: list[Section | Page], folded: set[str]) -> None:
    previous: tuple[str, str | None] | None = None   # (title, frame group id)
    for child in children:
        if isinstance(child, Section):
            _collect_folds(child.children, folded)
            previous = None
            continue
        if is_bridge_page(child):
            previous = None
            continue
        group = child.frame_group.id if child.frame_group is not None else None
        if group is not None and previous is not None and group == previous[1]:
            continue          # another frame of the unit already on the row
        if previous is not None and child.title == previous[0]:
            folded.add(child.id)
        previous = (child.title, group)


def tail_visible(page: Page) -> bool:
    """Does this page show its ``after_frames`` blocks?

    Blocks written after ``p.frames(...)`` belong to the finished picture, so
    only an animation's last frame shows them. A page without a frame group has
    no held blocks, and is trivially visible.
    """
    group = page.frame_group
    return group is None or group.index == group.total


# Kinds ``block_held`` never holds. A ``side_image`` is a Marp split background
# claiming the whole slide: hiding it would reflow every frame's text to full
# width, which is exactly the between-frame shift holding exists to prevent.
HELD_EXEMPT_KINDS = frozenset({"side_image"})


def block_held(page: Page, block: Block) -> bool:
    """Is this block held back on this page — reserving its space, not shown?

    True on an animation's earlier frames for the blocks written after
    ``p.frames(...)``: until the last frame they only claim their layout space
    (the viewer renders them invisible; PPTX, with no shared geometry between
    slides, drops them outright).
    """
    return (
        block.after_frames
        and not tail_visible(page)
        and block.kind not in HELD_EXEMPT_KINDS
    )


def slide_numbers(pages: list[Page], folds: frozenset[str] = frozenset()) -> list[int]:
    """The number each page is *shown* as, 1-based, one entry per page.

    An animation is one slide to a reader: every frame carries the group's
    number and the count advances once for the whole group, so a three-frame
    animation at 12 leaves the next page at 13, not 15. This is the number the
    deck prints, the outline labels a row with, and a citation backrefs — the
    physical position in ``pages`` stays the addressing scheme for navigation,
    ``--pages``, and exported image filenames.

    ``folds`` is :func:`outline_folds` — pages folded into the previous outline
    row, which take its number the same way, so one row stays one number. It is
    computed from the tree (siblings, not deck order), which is why it arrives
    as an argument rather than being read off ``pages`` here.
    """
    numbers = [0] * len(pages)
    seen: set[str] = set()
    count = 0
    for position, page in enumerate(pages):
        if is_bridge_page(page):
            # A bridge is a breath between topics, not an idea: it prints no
            # number and the count does not advance, so the pages around it
            # stay consecutively numbered.
            numbers[position] = count or 1
            continue
        group = page.frame_group.id if page.frame_group is not None else None
        if page.id in folds and count:
            # Same title as the page before: one row, so one number. Registering
            # the group keeps this page's own frames on that number too.
            if group is not None:
                seen.add(group)
            numbers[position] = count
            continue
        if group is None or group not in seen:
            count += 1
            if group is not None:
                seen.add(group)
        numbers[position] = count
    return numbers


def select_pages(lecture: Lecture, spec: str) -> Lecture:
    """Return a copy of ``lecture`` pruned to the pages named by ``spec``.

    ``spec`` is a comma-separated list of selectors, each one of:
      - a page id (e.g. ``motivation``),
      - the id of a page carrying ``p.frames(...)``, which selects every frame
        of that animation (e.g. ``commit-logging`` -> ``commit-logging-1..-3``),
      - a 1-based deck index in flat deck order (e.g. ``4``), or
      - an inclusive index range (e.g. ``3-7``).

    Pure-number tokens are always read as deck indices. Sections survive iff
    they still hold a selected page; page and section order is preserved.
    Unknown ids, out-of-range indices, or an empty selection raise
    ``ValidationError``.
    """
    order = flatten_pages(lecture.children)
    by_index = {i + 1: page.id for i, page in enumerate(order)}
    known_ids = {page.id for page in order}
    groups = {
        gid: [order[position].id for position in positions]
        for gid, positions in frame_groups(order).items()
    }
    chosen = _resolve_page_spec(spec, by_index, known_ids, groups)

    pruned = _prune_to_pages(lecture.children, chosen)
    if not pruned:
        raise ValidationError(f"page selection matched no pages: {spec!r}")
    return replace(lecture, children=pruned)


def _resolve_page_spec(
    spec: str,
    by_index: dict[int, str],
    known_ids: set[str],
    groups: dict[str, list[str]] | None = None,
) -> set[str]:
    chosen: set[str] = set()
    for raw in spec.split(","):
        token = raw.strip()
        if not token:
            continue
        if re.fullmatch(r"\d+", token):
            chosen.add(_index_to_id(int(token), by_index))
        elif re.fullmatch(r"\d+-\d+", token):
            lo, hi = (int(part) for part in token.split("-"))
            if lo > hi:
                raise ValidationError(f"invalid page range: {token!r}")
            for i in range(lo, hi + 1):
                chosen.add(_index_to_id(i, by_index))
        elif token in known_ids:
            chosen.add(token)
        elif groups and token in groups:
            # The id of a frames page: no slide of its own, so it means the
            # whole animation.
            chosen.update(groups[token])
        else:
            raise ValidationError(f"unknown page id: {token!r}")
    if not chosen:
        raise ValidationError(f"empty page selection: {spec!r}")
    return chosen


def _index_to_id(i: int, by_index: dict[int, str]) -> str:
    if i not in by_index:
        raise ValidationError(f"page index out of range: {i} (have 1..{len(by_index)})")
    return by_index[i]


def _prune_to_pages(
    children: list[Section | Page], chosen: set[str]
) -> list[Section | Page]:
    kept: list[Section | Page] = []
    for child in children:
        if isinstance(child, Page):
            if child.id in chosen:
                kept.append(child)
        else:
            sub = _prune_to_pages(child.children, chosen)
            if sub:
                kept.append(replace(child, children=sub))
    return kept


def validate_lecture(lecture: Lecture) -> None:
    if lecture.ratio not in RATIOS:
        raise ValidationError(
            f"Unknown lecture ratio: {lecture.ratio!r} (allowed: {sorted(RATIOS)})"
        )

    seen: set[str] = set()
    seen_refs: set[str] = set()

    def check_node(node: Lecture | Section | Page) -> None:
        if not node.id:
            raise ValidationError("Lecture, section, and page IDs must be non-empty")
        if node.id in seen:
            raise ValidationError(f"Duplicate node ID: {node.id}")
        seen.add(node.id)

        # A title is already the loudest thing on the page; marking a word in it
        # says this one matters more than the rest of the loudest thing.
        check_mark_text(node.title, allowed=False, where="a title", page_id=node.id)
        if isinstance(node, Lecture) and node.subtitle:
            check_mark_text(
                node.subtitle, allowed=False, where="a subtitle", page_id=node.id
            )

        if isinstance(node, Page):
            if not node.blocks:
                raise ValidationError(f"Page has no content blocks: {node.id}")
            check_bridge_page(node)
            if node.book_title:
                check_mark_text(
                    node.book_title, allowed=False,
                    where="a book_title", page_id=node.id,
                )
            check_page_book(node)
            if node.gap is not None:
                check_page_gap(node.gap, page_id=node.id)
            keys: set[str] = set()
            for block in node.blocks:
                check_block(block, page_id=node.id, refs=seen_refs, keys=keys)
            for item in node.news:
                check_news_item(item, page_id=node.id)
            return

        check_book_flow(node.children, owner_id=node.id)
        for child in node.children:
            check_node(child)

    check_node(lecture)


def check_bridge_page(page: Page) -> None:
    """A ``bridge`` block owns its page: alone, and out of the book.

    A bridge that could share a page with figures or bullets would just be a
    title-less free-form page, which is exactly what the feature refuses to be;
    and its text is projector rhetoric, so the book never prints it.
    """
    if not any(block.kind == "bridge" for block in page.blocks):
        return
    if len(page.blocks) != 1:
        raise ValidationError(
            f"a bridge block must be the only block on its page: {page.id}"
        )
    if page.book != "skip":
        raise ValidationError(
            f'a bridge page must carry book="skip": {page.id}'
        )


def check_page_book(page: Page) -> None:
    if page.book not in BOOK_MODES:
        raise ValidationError(
            f"Page book mode on {page.id} must be one of "
            f"{sorted(BOOK_MODES)}, got {page.book!r}"
        )
    if page.book_title is not None and (
        not isinstance(page.book_title, str) or not page.book_title.strip()
    ):
        raise ValidationError(f"Empty book_title on {page.id}")


def check_book_flow(children: list[Section | Page], owner_id: str) -> None:
    """A ``book="merge"`` page needs a preceding sibling page to merge into.

    The host is the nearest earlier ``book="page"`` sibling; ``skip`` pages in
    between are fine (they vanish from the book), but a Section breaks the run —
    merging *across* a heading would silently reorder the book's text.
    """
    has_host = False
    for child in children:
        if isinstance(child, Section):
            has_host = False
            continue
        if child.book == "merge" and not has_host:
            raise ValidationError(
                f"Page {child.id} has book=\"merge\" but no earlier page "
                f"in {owner_id} to merge into"
            )
        if child.book == "page":
            has_host = True


def check_page_gap(gap: PageGap, page_id: str) -> None:
    if gap.mode != "auto":
        raise ValidationError(f"Unknown page gap mode on {page_id}: {gap.mode!r}")
    uncapped = gap.max_px is None
    if type(gap.min_px) is not int or not (uncapped or type(gap.max_px) is int):
        raise ValidationError(f"Page gap values must be integers on {page_id}")
    if gap.min_px < 0 or (not uncapped and gap.max_px < 0):
        raise ValidationError(f"Page gap values must be non-negative on {page_id}")
    if not uncapped and gap.min_px > gap.max_px:
        raise ValidationError(
            f"Page gap min_px must be <= max_px on {page_id}: "
            f"{gap.min_px} > {gap.max_px}"
        )


def check_news_item(item: NewsItem, page_id: str) -> None:
    if not isinstance(item.title, str) or not item.title.strip():
        raise ValidationError(f"Empty news title on {page_id}")
    if not isinstance(item.url, str) or not item.url.strip():
        raise ValidationError(f"Empty news url on {page_id}: {item.title!r}")
    if item.kind not in NEWS_KINDS:
        raise ValidationError(
            f"News kind on {page_id} must be one of "
            f"{sorted(NEWS_KINDS)}, got {item.kind!r}"
        )
    for field_name in ("source", "date", "why", "image", "archived_url"):
        value = getattr(item, field_name)
        if value is not None and (not isinstance(value, str) or not value.strip()):
            raise ValidationError(
                f"Empty news {field_name} on {page_id}: {item.title!r}"
            )
    for tag in item.tags:
        if not isinstance(tag, str) or not tag.strip():
            raise ValidationError(f"Empty news tag on {page_id}: {item.title!r}")


def check_block(
    block: Block,
    page_id: str,
    refs: set[str] | None = None,
    keys: set[str] | None = None,
) -> None:
    if block.kind not in BLOCK_KINDS:
        raise ValidationError(f"Unknown block kind on {page_id}: {block.kind}")
    if block.key is not None:
        check_block_key(block, page_id, keys)
    if block.kind in REF_KINDS:
        check_ref(block, page_id, refs)
    for footnote in block.footnotes:
        if not isinstance(footnote, str) or not footnote.strip():
            raise ValidationError(f"Empty footnote on {page_id}: {block.kind}")
    for note in block.annotations:
        if not isinstance(note.text, str) or not note.text.strip():
            raise ValidationError(f"Empty annotation on {page_id}: {block.kind}")
        if note.at not in ANNOTATION_ANCHORS:
            raise ValidationError(
                f"Annotation anchor on {page_id} must be one of "
                f"{sorted(ANNOTATION_ANCHORS)}, got {note.at!r}"
            )
    if block.float_image is not None:
        src = block.float_image.get("src")
        if not isinstance(src, str) or not src.strip():
            raise ValidationError(
                f"Empty float image src on {page_id}: {block.kind}"
            )
    if block.kind == "architecture":
        check_architecture(block.content, page_id)
    if block.kind == "row":
        check_row(block.content, page_id)
    if block.kind == "cover":
        check_cover(block.content, page_id)
    if block.kind == "spacer":
        check_spacer(block.content, page_id)
    if block.kind == "highlight":
        check_highlight(block.content, page_id)
    if block.kind == "code":
        check_code(block.content, page_id)
    if block.kind == "bridge":
        check_bridge(block, page_id)
    check_marks(block, page_id)


# Content values that are never author prose: a path, a URL, an enum, a shell
# command, a language name. `check_marks` walks a block's content generically —
# so a block kind added tomorrow is scanned the day it lands — and these are the
# keys it must not walk into.
_NON_PROSE_KEYS = frozenset({
    "src", "srcs", "url", "link", "logo", "ref", "language", "command",
    "side", "flow", "align", "caption_align", "tone", "width",
})


def _prose_strings(value: Any, key: str | None = None) -> list[str]:
    """Every author-written string inside a block's content, in author order."""
    if isinstance(value, str):
        return [] if key in _NON_PROSE_KEYS else [value]
    if isinstance(value, dict):
        found: list[str] = []
        for name, item in value.items():
            found.extend(_prose_strings(item, name))
        return found
    if isinstance(value, (list, tuple)):
        found = []
        for item in value:
            found.extend(_prose_strings(item, key))
        return found
    return []


def check_mark_text(text: str, *, allowed: bool, where: str, page_id: str) -> None:
    for problem in marks.errors(text, allowed=allowed):
        raise ValidationError(f"{problem} — on {page_id}, in {where}")


def check_marks(block: Block, page_id: str) -> None:
    """`<mark>` is legal in `p.slide(...)` text and nowhere else.

    Refused rather than stripped: a highlight that silently vanishes looks
    exactly like a word the author never marked, so the mistake would surface on
    the projector instead of at build time. See :mod:`lecturekit.marks`.

    A `code` block is exempt from the scan entirely — it is verbatim, and a
    sample that *shows* `<mark>` as the HTML it is must not be rejected for it.
    """
    if block.kind == "code":
        texts: list[str] = []
    elif isinstance(block.content, str):
        texts = [block.content]
    else:
        texts = _prose_strings(block.content)
    for text in texts:
        check_mark_text(
            text, allowed=block.kind == "slide",
            where=f"the {block.kind} block", page_id=page_id,
        )
    # A footnote or a callout bubble is marginal prose hanging off a block — it
    # is never slide text, not even on a slide block.
    for footnote in block.footnotes:
        check_mark_text(footnote, allowed=False, where="a footnote", page_id=page_id)
    for note in block.annotations:
        check_mark_text(note.text, allowed=False, where="an annotation", page_id=page_id)
    if block.float_image is not None:
        check_mark_text(
            str(block.float_image.get("alt") or ""),
            allowed=False, where="an image_right alt", page_id=page_id,
        )


def check_block_key(block: Block, page_id: str, keys: set[str] | None) -> None:
    """A pinned translation key is a non-empty, dot-free name, unique per page.

    Dot-free because the full key joins the page id, the pin, and any sub-string
    with dots (``intro.why.footnote.1``): a dot inside the pin would make one
    key parse as another's sub-string. Unique per page because two blocks
    sharing a pin would share one translation.
    """
    key = block.key
    if not isinstance(key, str) or not key.strip():
        raise ValidationError(f"Empty block key on {page_id}: {block.kind}")
    if "." in key:
        raise ValidationError(
            f"Block key on {page_id} may not contain '.': {key!r}"
        )
    if keys is not None:
        if key in keys:
            raise ValidationError(f"Duplicate block key on {page_id}: {key!r}")
        keys.add(key)


def check_ref(block: Block, page_id: str, refs: set[str] | None) -> None:
    """A figure ref must be a well-formed, unique name on a captioned figure.

    The `\\label` the book emits for a ref rides the figure's `\\caption` — an
    uncaptioned figure has no number to reference — so a ref without a caption
    is rejected here rather than producing a dangling reference at render time.
    """
    ref = block.content.get("ref")
    if ref is None:
        return
    if not isinstance(ref, str) or not _REF_NAME_RE.fullmatch(ref):
        raise ValidationError(
            f"Figure ref on {page_id} must be letters/digits/dashes/underscores, "
            f"got {ref!r}"
        )
    if not (block.content.get("caption") or "").strip():
        raise ValidationError(
            f"Figure ref {ref!r} on {page_id} needs a caption "
            f"(the book's figure number rides the caption)"
        )
    if refs is not None:
        if ref in refs:
            raise ValidationError(f"Duplicate figure ref: {ref!r} (on {page_id})")
        refs.add(ref)


def check_cover(content: dict, page_id: str) -> None:
    for field_name in ("author", "time"):
        value = content.get(field_name)
        if value is not None and (not isinstance(value, str) or not value.strip()):
            raise ValidationError(
                f"Empty cover {field_name} on {page_id}"
            )
    logo = content.get("logo")
    if logo is None:
        return
    if isinstance(logo, str):
        if not logo.strip():
            raise ValidationError(f"Empty cover logo on {page_id}")
        return
    if not isinstance(logo, dict):
        raise ValidationError(
            f"Cover logo on {page_id} must be a string or left/right mapping"
        )
    for key in logo:
        if key not in ("left", "right"):
            raise ValidationError(
                f"Cover logo on {page_id} has unknown slot {key!r}"
            )
    for key in ("left", "right"):
        value = logo.get(key)
        if value is not None and (not isinstance(value, str) or not value.strip()):
            raise ValidationError(
                f"Empty cover logo {key} on {page_id}"
            )


def check_spacer(content: dict, page_id: str) -> None:
    px = content.get("px")
    # bool is an int subclass; reject it so True/False can't pass as a height.
    if type(px) is not int or px <= 0:
        raise ValidationError(
            f"Spacer px on {page_id} must be a positive integer, got {px!r}"
        )


# The fence a display formula is written between, in a highlight as in a slide.
DISPLAY_MATH_FENCE = "$$"


def highlight_lines(text: str) -> list[str]:
    """A highlight's text as the rows the chip draws — one string per row.

    Each line is stripped, so a triple-quoted literal indents naturally. The one
    thing that is not one line per row is a display formula written as a ``$$``
    fence — the spelling a slide uses, so the spelling an author reaches for
    here. A formula is one row of the chip however many lines it was typed
    across, so the fence is folded onto a single ``$$…$$`` line — once, for
    every renderer, since each of the four then has to say what display math
    *is* in its own medium and none of them should also have to find it.
    """
    lines = [line.strip() for line in str(text).strip().splitlines()]
    rows: list[str] = []
    index = 0
    while index < len(lines):
        line = lines[index]
        index += 1
        if line != DISPLAY_MATH_FENCE:
            rows.append(line)
            continue
        body: list[str] = []
        while index < len(lines) and lines[index] != DISPLAY_MATH_FENCE:
            body.append(lines[index])
            index += 1
        index += 1                      # the closing fence
        latex = " ".join(part for part in body if part)
        if latex:
            rows.append(f"{DISPLAY_MATH_FENCE}{latex}{DISPLAY_MATH_FENCE}")
    return rows


def check_highlight(content: dict, page_id: str) -> None:
    text = content.get("text")
    if not isinstance(text, str) or not text.strip():
        raise ValidationError(f"Empty highlight text on {page_id}")
    tone = content.get("tone")
    if tone not in HIGHLIGHT_TONES:
        raise ValidationError(
            f"Highlight tone on {page_id} must be one of "
            f"{sorted(HIGHLIGHT_TONES)}, got {tone!r}"
        )


def check_code(content: dict, page_id: str) -> None:
    """A marked code line names a line that exists, and is not blank.

    Line numbers are 1-based over the block as the slide shows it — every
    renderer strips the leading and trailing blank lines a triple-quoted
    literal collects, so the numbers have to count what is left. A number past
    the end, or one landing on a blank line, is refused rather than ignored: an
    author who renumbers a listing and forgets the mark would otherwise find out
    on the projector, from a wash that quietly went missing.
    """
    if "mark" not in content:
        return
    language = content.get("language")
    if language != "pseudo":
        raise ValidationError(
            f"Code mark on {page_id} needs language 'pseudo', got {language!r} — "
            "another language is coloured by the renderer's own highlighter, "
            "which leaves nowhere to put the wash"
        )
    tone = content.get("tone")
    if tone not in HIGHLIGHT_TONES:
        raise ValidationError(
            f"Code mark tone on {page_id} must be one of "
            f"{sorted(HIGHLIGHT_TONES)}, got {tone!r}"
        )
    lines = str(content.get("content", "")).strip("\n").split("\n")
    for number in content["mark"]:
        # bool is an int subclass; reject it so True/False can't pass as a line.
        if type(number) is not int or number < 1:
            raise ValidationError(
                f"Code mark on {page_id} must be a 1-based line number, "
                f"got {number!r}"
            )
        if number > len(lines):
            raise ValidationError(
                f"Code mark on {page_id} names line {number}, but the block has "
                f"{len(lines)} line{'' if len(lines) == 1 else 's'}"
            )
        if not lines[number - 1].strip():
            raise ValidationError(
                f"Code mark on {page_id} names line {number}, which is blank"
            )


def check_bridge(block: Block, page_id: str) -> None:
    """Bridge text is plain, short, and carries nothing else.

    No footnotes or annotations: a bridge with something to source or point at
    has outgrown being a bridge and should be a normal page.
    """
    content = block.content if isinstance(block.content, dict) else {}
    text = content.get("text")
    if not isinstance(text, str) or not text.strip():
        raise ValidationError(f"Empty bridge text on {page_id}")
    lines = [line for line in (l.strip() for l in text.splitlines()) if line]
    if len(lines) > BRIDGE_MAX_LINES:
        raise ValidationError(
            f"a bridge holds at most {BRIDGE_MAX_LINES} lines on {page_id}, "
            f"got {len(lines)} — a longer transition wants a normal page"
        )
    if block.footnotes or block.annotations:
        raise ValidationError(
            f"a bridge carries no footnotes or annotations: {page_id}"
        )


def check_architecture(content: dict, page_id: str) -> None:
    flow = content.get("flow")
    if flow is not None and flow not in ARCH_FLOWS:
        raise ValidationError(
            f"Architecture flow on {page_id} must be one of "
            f"{sorted(ARCH_FLOWS)} or None, got {flow!r}"
        )
    layers = content.get("layers") or []
    if not layers:
        raise ValidationError(f"Architecture has no layers on {page_id}")
    for layer in layers:
        modules = layer.get("modules") or []
        if not modules:
            raise ValidationError(
                f"Architecture layer has no modules on {page_id}"
            )
        for module in modules:
            if not isinstance(module, str) or not module.strip():
                raise ValidationError(
                    f"Empty architecture module on {page_id}"
                )


def check_row(content: dict, page_id: str) -> None:
    items = content.get("items") or []
    if not items:
        raise ValidationError(f"Row has no images on {page_id}")
    for item in items:
        src = item.get("src")
        if not isinstance(src, str) or not src.strip():
            raise ValidationError(f"Empty row image src on {page_id}")
        align = item.get("caption_align")
        if align is not None and align not in ("left", "center", "right"):
            raise ValidationError(
                f"Row image caption_align on {page_id} must be "
                f"left/center/right, got {align!r}"
            )
