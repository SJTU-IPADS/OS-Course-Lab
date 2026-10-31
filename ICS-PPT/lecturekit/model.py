from __future__ import annotations

import re
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any, ClassVar, Iterable, Self

from . import filelinks, marks


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

# A container is written into a page's flat block list the way HTML writes an
# element: an opening `flex` block, its children, a closing `end` block. They
# are layout, not content — `layout(...)` reads them back into a tree, and a
# renderer that does not lay a slide out (the book, the transcript) never asks
# for either kind, so it sees the children in order. See docs/layout.md.
FLEX_KIND = "flex"
END_KIND = "end"
MARKER_KINDS = frozenset({FLEX_KIND, END_KIND})

# ``BLOCK_KINDS``, the kinds that exist, is read off the registered classes
# (see ``__getattr__`` below and ``lecturekit.elements``).

# The two axes of alignment, and where each value puts an element's content in
# its box as a fraction of the room left over: 0 at the start, 1 at the end.
HALIGNS = ("left", "center", "right")
VALIGNS = ("top", "middle", "bottom")
ALIGN_FRACTION = {
    "left": 0.0, "center": 0.5, "right": 1.0,
    "top": 0.0, "middle": 0.5, "bottom": 1.0,
}
FLEX_DIRECTIONS = ("column", "row")

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

# The kinds that may carry a figure `ref` (an author-chosen anchor that prose
# cites with `[@name]`) say so with `REF`. All render as numbered LaTeX figures,
# which is where the `\label` lands — so a ref requires a caption.

_REF_NAME_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9_-]*")

# Directional flow drawn between adjacent layers of an architecture diagram, and
# the chevron each direction draws between one pair of bands. The glyph lives
# here rather than in a renderer so a direction reads the same in every output.
# None renders plain stacked bands.
ARCH_FLOW_GLYPH = {"down": "▼", "up": "▲", "both": "⇅"}
ARCH_FLOWS = frozenset(ARCH_FLOW_GLYPH)

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

# Where a ``side_image`` page's columns sit in the band under the title. It is
# the page's own ``valign`` (see ``page_valign``), so it takes the same values.
# See dsl.PageBuilder.side_image.
SIDE_VALIGNS = VALIGNS
DEFAULT_SIDE_VALIGN = "top"
# Where an ``image_right`` picture sits against its text: "top" floats it and
# the text flows round it, "middle" sets the two side by side, each centred
# against the other. See dsl.BlockHandle.image_right.
FLOAT_VALIGNS = ("top", "middle")


def side_valign(blocks: Iterable[Block]) -> str:
    """The vertical alignment the ``side_image`` blocks among ``blocks`` ask for.

    A block without the key (one built by hand, or serialized before the key
    existed) asks for the default. Validation rejects a page whose side images
    disagree, so the first one's answer is the page's.
    """
    for block in blocks:
        if block.kind == "side_image":
            return block.content.get("valign", DEFAULT_SIDE_VALIGN)
    return DEFAULT_SIDE_VALIGN


def page_valign(page: Page, blocks: Iterable[Block] | None = None) -> str | None:
    """The page's own ``valign``: where its elements sit in the band under the title.

    ``p.valign(...)`` sets it; ``p.side_image(..., valign=...)`` says the same
    thing for a page with an image column, and is read when the page sets none.
    ``blocks`` names the blocks a renderer kept (a ``side_image`` filtered out of
    a target no longer speaks for the page there); it defaults to all of them.
    """
    if page.valign is not None:
        return page.valign
    blocks = page.blocks if blocks is None else blocks
    if any(block.kind == "side_image" for block in blocks):
        return side_valign(blocks)
    return None


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
    page; this is read where the frames act as one: the outline (folds a group
    into one entry) and the deck's reveal mode (only the first frame steps).
    Each frame still prints its own slide number. See dsl.PageBuilder.frames.
    """
    id: str
    index: int
    total: int


@dataclass(frozen=True, kw_only=True)
class LecturekitElement:
    """Anything a page lays out: a block, a container, the page itself.

    Each element has a box — the space its container gives it — and these two
    fields place its content in that box: ``halign`` is one of ``HALIGNS``,
    ``valign`` one of ``VALIGNS``, and ``None`` keeps the theme's default for
    the element. A container's content is its children, moved as one group
    along its direction; an unset field takes the container's value. The rules
    are in docs/layout.md, and each renderer implements them once, for every
    element.

    Footnotes and callouts hang off any element, so they are fields of this
    class: each footnote renders as small bottom-left text with a matching
    superscript on the element, and a callout is a bubble floating over the
    slide next to it (see dsl.BlockHandle.footnote / annotate and Annotation).
    A container draws its footnote marker on its last leaf.
    """
    halign: str | None = None
    valign: str | None = None
    # The share of its row's width this element takes, among the children that
    # carry no width of their own (see ``p.flex("row", widths=...)``). ``None``
    # leaves it to the equal share.
    weight: float | None = None
    footnotes: tuple[str, ...] = ()
    annotations: tuple[Annotation, ...] = ()

    def aligned(self, **axes: str | None) -> Self:
        """An immutable copy with alignment set, shared by all element types."""
        if axes.keys() - {"halign", "valign"}:
            raise ValidationError(f"unknown alignment axis: {', '.join(axes)}")
        element = replace(self, **axes)
        check_alignment(element, type(self).__name__)
        return element


@dataclass(frozen=True, kw_only=True)
class Flex(LecturekitElement):
    """A container: its children laid out in a column or in a row.

    Not stored on a page as such — a page's ``blocks`` hold it as an opening
    ``flex`` block and a closing ``end`` block around its children, and
    :func:`layout` builds this node from them. ``block`` is that opening block.
    """
    direction: str = "column"
    children: tuple[LecturekitElement, ...] = ()
    block: Block | None = None


@dataclass(frozen=True)
class Block(LecturekitElement):
    """One leaf element of a page, and the base of every kind of leaf.

    A block is a ``kind`` and its ``content``; the class a ``Block`` actually
    is follows from its kind. ``Block(kind="table", content=...)`` builds a
    ``lecturekit.elements.Table``, and a kind nobody registered stays a plain
    ``Block`` that validation refuses. Everything one kind knows about itself
    lives on its class:

    * which targets draw it by default (``TARGETS``) and what it may carry
      (``REF``, ``MARKS``, ``FILE_LINKS``, ``VERBATIM``);
    * how the viewer lays it out (``REVEAL_STEP``, ``IN_FLOW``, ``CENTERED``,
      ``HOLDABLE``, ``IN_CONTAINER``);
    * how it checks itself (:meth:`check_content`);
    * how each target draws it: :meth:`to_marp` (the deck: HTML, PDF, PNG),
      :meth:`to_pptx`, :meth:`to_latex`, :meth:`to_transcript`, and
      :meth:`footnote_marker` for the superscript a footnote leaves on it.

    A renderer walks the page and asks each block to draw itself; it holds no
    table of kinds. The fields below are shared by every kind.
    """
    # Registration: the kind string this class answers to. ``Block`` itself
    # has none, so only an unregistered kind validates as unknown.
    KIND: ClassVar[str] = ""
    # The targets that draw this kind unless a block says otherwise with
    # ``only`` / ``except_``: any of "viewer", "pptx", "latex", "transcript".
    TARGETS: ClassVar[frozenset[str]] = frozenset()
    # May carry a figure ref (``ref=``, cited from prose as ``[@name]``).
    REF: ClassVar[bool] = False
    # Its text may carry ``<mark>`` (slide text) / a file link / is verbatim
    # and so scanned for neither. ``LINK_FREE_KEYS`` names content keys the
    # file-link scan leaves out (a demo's recorded output).
    MARKS: ClassVar[bool] = False
    FILE_LINKS: ClassVar[bool] = False
    VERBATIM: ClassVar[bool] = False
    LINK_FREE_KEYS: ClassVar[frozenset[str]] = frozenset()
    # Its text stands in as the book's prose (a `slide` or `columns` does so only
    # when the author forces it in with ``only=["latex"]``).
    BOOK_TEXT: ClassVar[bool] = False
    # Held back on the early frames of an animation (``after_frames``) / may sit
    # inside a ``flex`` container.
    HOLDABLE: ClassVar[bool] = True
    IN_CONTAINER: ClassVar[bool] = True
    # Viewer: takes a reveal step / is set in the page's flow (in the gap split,
    # in the footnote numbering) / sits centred in its box unless aligned.
    REVEAL_STEP: ClassVar[bool] = True
    IN_FLOW: ClassVar[bool] = True
    CENTERED: ClassVar[bool] = False

    def __new__(cls, *args: Any, **kwargs: Any):
        if cls is Block:
            kind = kwargs["kind"] if "kind" in kwargs else (args[0] if args else None)
            cls = element_class(kind)
        return object.__new__(cls)

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
    # A small image floated to the right of a slide block's text; the text
    # wraps around it. Only set on slide blocks. See dsl.BlockHandle.image_right.
    float_image: dict | None = None
    # When True, this slide block is set in the page's image column, under
    # the ``side_image``, instead of in the text column beside it. Only set on
    # slide blocks, and only on a page with a ``side_image``. See
    # dsl.BlockHandle.under_side_image.
    in_side: bool = False
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

    # --- validation ---

    def validate(
        self,
        page_id: str,
        refs: set[str] | None = None,
        keys: set[str] | None = None,
    ) -> None:
        """Refuse a block no renderer could draw; ``refs`` / ``keys`` collect the
        page's figure refs and pinned translation keys to keep them unique."""
        if type(self).KIND != self.kind:
            raise ValidationError(f"Unknown block kind on {page_id}: {self.kind}")
        if self.key is not None:
            check_block_key(self, page_id, keys)
        if self.REF:
            check_ref(self, page_id, refs)
        for footnote in self.footnotes:
            if not isinstance(footnote, str) or not footnote.strip():
                raise ValidationError(f"Empty footnote on {page_id}: {self.kind}")
        for note in self.annotations:
            if not isinstance(note.text, str) or not note.text.strip():
                raise ValidationError(f"Empty annotation on {page_id}: {self.kind}")
            if note.at not in ANNOTATION_ANCHORS:
                raise ValidationError(
                    f"Annotation anchor on {page_id} must be one of "
                    f"{sorted(ANNOTATION_ANCHORS)}, got {note.at!r}"
                )
        if self.float_image is not None:
            src = self.float_image.get("src")
            if not isinstance(src, str) or not src.strip():
                raise ValidationError(
                    f"Empty float image src on {page_id}: {self.kind}"
                )
            valign = self.float_image.get("valign", DEFAULT_SIDE_VALIGN)
            if valign not in FLOAT_VALIGNS:
                raise ValidationError(
                    f"Unknown image_right valign on {page_id}: {valign!r}"
                )
        self.check_content(page_id)
        check_marks(self, page_id)
        check_file_links(self, page_id)

    def check_content(self, page_id: str) -> None:
        """The kind's own rules for its ``content``; a kind with none keeps this."""

    # --- drawing: a kind overrides the methods of the targets that draw it ---

    def to_marp(self, ctx: Any) -> list[str]:
        """The deck (HTML, PDF and PNG are all this one page): Marp markdown lines.

        ``ctx`` is the viewer's ``MarpCtx``. A kind the deck draws elsewhere
        (a ``side_image`` is the page's image column) returns no lines.
        """
        return []

    def sized_in_box(self, in_row: bool) -> tuple[Block, str]:
        """Viewer, in an aligned layout: ``(block to draw, style for its content
        box)``. A kind that carries its own width takes it from the box instead
        of resolving it a second time; the rest are drawn as they are."""
        return self, ""

    def row_basis(self) -> str | None:
        """Viewer: the width this block keeps in a ``flex`` row, or ``None`` to
        share what the other children leave."""
        return None

    # PPTX: a block that can be drawn again in less height when its page ran past
    # the slide (an architecture diagram), and how.
    SHRINKABLE: ClassVar[bool] = False

    def redraw_pptx(self, ctx: Any, top: int, max_height: int) -> int:
        """Draw again from ``top`` within ``max_height``; the height it took."""
        raise NotImplementedError(self.kind)

    def footnote_marker(self, lines: list[str], numbers: tuple[int, ...]) -> list[str]:
        """``lines`` (what :meth:`to_marp` drew) with the footnote superscript
        for ``numbers`` attached. The default rides the end of the last line;
        a kind whose last line is not text overrides it."""
        from .renderers.viewer import blocks
        return blocks.mark_text_end(lines, numbers)

    def to_pptx(self, ctx: Any) -> None:
        """Draw onto ``ctx.slide`` (the pptx renderer's ``Ctx``), advancing its cursor."""

    def to_latex(self, ctx: Any) -> str:
        """The LaTeX the book sets for it (``ctx`` is the latex renderer's ``Ctx``)."""
        raise ValidationError(f"LaTeX renderer cannot emit block kind: {self.kind}")

    def to_transcript(self, embedder: Any) -> str:
        """HTML for the printable sheet. A kind the sheet does not normally draw,
        forced in with ``only=["transcript"]``, is set as body text if it is
        markdown, and dropped otherwise."""
        if isinstance(self.content, str):
            from .renderers.transcript.text import markdown
            return f'<div class="tx-slide">{markdown(self.content)}</div>'
        return ""


_ELEMENTS: dict[str, type[Block]] = {}


def element(cls: type[Block]) -> type[Block]:
    """Class decorator: register ``cls`` as the class of blocks of ``cls.KIND``."""
    _ELEMENTS[cls.KIND] = cls
    return cls


def element_class(kind: str | None) -> type[Block]:
    """The class that blocks of ``kind`` are; ``Block`` for a kind nobody registered."""
    from . import elements  # noqa: F401  (registers the classes on first use)
    return _ELEMENTS.get(kind, Block)


def kinds_drawn_by(target: str) -> frozenset[str]:
    """Every kind that ``target`` draws by default."""
    element_class(None)
    return frozenset(k for k, cls in _ELEMENTS.items() if target in cls.TARGETS)


@dataclass(frozen=True)
class Page(LecturekitElement):
    """One slide's worth of content: the root container of its elements.

    As an element, the page is a column whose box is the band under its title;
    its ``halign`` / ``valign`` place its elements there (see ``page_valign``
    for the ``side_image`` spelling of the latter).
    """
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


def select_blocks(
    page: Page, target: str, kinds: Iterable[str] | None = None
) -> list[Block]:
    """Blocks of `page` that renderer `target` should emit.

    The author's `only`/`except_` override sits *on top of* the kind's own
    default: `only={target}` forces a block in even when its kind is not
    normally drawn by `target` (a `slide` pulled into the book, say), and
    `except_` forces one out. Absent an override, the block's class decides
    (its `TARGETS`); `kinds`, when given, decides instead.

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
        if kinds is None:
            if target not in block.TARGETS:
                continue
        elif block.kind not in kinds:
            continue
        selected.append(block)
    if kinds is None and target not in element_class(FLEX_KIND).TARGETS:
        selected = _with_container_footnotes(page, selected)
    return selected


def _with_container_footnotes(page: Page, selected: list[Block]) -> list[Block]:
    """Hand each container's footnotes to its last leaf.

    A target without containers (the book) draws no ``flex`` marker, which is
    where the footnotes of a container are stored, so its last leaf carries them
    instead: the superscript lands where the viewer puts it.
    """
    extra: dict[int, tuple[str, ...]] = {}
    stack: list[tuple[tuple[str, ...], int | None]] = []
    for index, block in enumerate(page.blocks):
        if block.kind == FLEX_KIND:
            stack.append((block.footnotes, None))
        elif block.kind == END_KIND:
            if stack:
                notes, last = stack.pop()
                if notes and last is not None:
                    extra[last] = extra.get(last, ()) + notes
        elif stack:
            stack = [(notes, index) for notes, _ in stack]
    if not extra:
        return selected
    positions = {id(block): index for index, block in enumerate(page.blocks)}
    return [
        replace(block, footnotes=block.footnotes + extra[positions[id(block)]])
        if positions.get(id(block)) in extra else block
        for block in selected
    ]


def layout(blocks: Iterable[Block]) -> list[LecturekitElement]:
    """Read a flat run of blocks back into the tree its markers spell.

    Returns the top-level elements in order: a ``Block`` for each leaf and a
    ``Flex`` for each ``flex`` … ``end`` pair, holding its children. A container
    left without children (every one filtered out of a target) is dropped, so
    a renderer never draws an empty box. Validation has already refused an
    unbalanced page; a stray ``end`` here closes nothing, and a container still
    open at the end of the run is closed there.
    """
    stack: list[tuple[Block, list[LecturekitElement]]] = []
    top: list[LecturekitElement] = []

    def close() -> None:
        opener, children = stack.pop()
        if not children:
            return
        node = Flex(
            direction=(opener.content or {}).get("direction", "column"),
            children=tuple(children),
            block=opener,
            halign=opener.halign,
            valign=opener.valign,
            weight=opener.weight,
            footnotes=opener.footnotes,
        )
        (stack[-1][1] if stack else top).append(node)

    for block in blocks:
        if block.kind == FLEX_KIND:
            stack.append((block, []))
        elif block.kind == END_KIND:
            if stack:
                close()
        else:
            (stack[-1][1] if stack else top).append(block)
    while stack:
        close()
    return top


def row_shares(row: Flex) -> list[float | None]:
    """The fraction of the room a row leaves each child, in order.

    The children that carry no width of their own share that room; their
    weights, renormalised over the ones the target kept, give the fractions.
    A child with a width of its own, and every child of a row without weights,
    has ``None``: the equal share.
    """
    own = [isinstance(child, Block) and bool(child.row_basis()) for child in row.children]
    weights = [None if has_own else child.weight
               for child, has_own in zip(row.children, own)]
    shared = [w for w in weights if w is not None]
    if not shared or len(shared) != own.count(False):
        return [None] * len(row.children)
    total = float(sum(shared))
    return [None if w is None else w / total for w in weights]


def resolve_align(
    element: LecturekitElement, inherited: tuple[str | None, str | None]
) -> tuple[str | None, str | None]:
    """``(halign, valign)`` of ``element``: its own, else its container's."""
    return (element.halign or inherited[0], element.valign or inherited[1])


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
    reveal pair — so they get one outline row, the same treatment an animation's
    frames already get. An animation counts as one unit here: its frames share a
    title, and it is the group as a whole that folds into its neighbour (or is
    folded into by it). Only the outline folds: every page still prints its own
    slide number, its position in the deck.

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
        and block.HOLDABLE
    )


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
            check_side_text(node)
            check_side_valign(node)
            check_layout(node)
            for item in node.news:
                check_news_item(item, page_id=node.id)
            return

        check_book_flow(node.children, owner_id=node.id)
        for child in node.children:
            check_node(child)

    check_node(lecture)


def check_side_text(page: Page) -> None:
    """Slide text set under a side image needs that image on the same page."""
    if not any(block.in_side for block in page.blocks):
        return
    if not any(block.kind == "side_image" for block in page.blocks):
        raise ValidationError(
            f"under_side_image needs a p.side_image(...) on the same page: {page.id}"
        )


def check_side_valign(page: Page) -> None:
    """A page's side images agree on ``valign``, with each other and the page.

    A side image's ``valign`` is the page's (see ``page_valign``): one page has
    one band under its title, so its side images cannot ask for different
    places in it, nor for another than ``p.valign(...)`` set.
    """
    valigns = []
    for block in page.blocks:
        if block.kind != "side_image":
            continue
        valign = block.content.get("valign", DEFAULT_SIDE_VALIGN)
        if valign not in SIDE_VALIGNS:
            raise ValidationError(
                f"Unknown side_image valign on {page.id}: {valign!r}"
            )
        valigns.append(valign)
    if len(set(valigns)) > 1:
        raise ValidationError(
            f"The side images on {page.id} disagree on valign: "
            f"{', '.join(repr(v) for v in valigns)}"
        )
    if (page.valign is not None and valigns
            and valigns[0] != DEFAULT_SIDE_VALIGN and valigns[0] != page.valign):
        raise ValidationError(
            f"side_image valign={valigns[0]!r} and p.valign({page.valign!r}) "
            f"disagree on {page.id}; the side image's valign is the page's"
        )


def check_alignment(element: LecturekitElement, where: str) -> None:
    """``halign`` / ``valign`` name a place on their own axis, or are unset."""
    if element.halign is not None and element.halign not in HALIGNS:
        raise ValidationError(
            f"halign on {where} must be one of {', '.join(HALIGNS)}, "
            f"got {element.halign!r}"
        )
    if element.valign is not None and element.valign not in VALIGNS:
        raise ValidationError(
            f"valign on {where} must be one of {', '.join(VALIGNS)}, "
            f"got {element.valign!r}"
        )


def check_weight(element: LecturekitElement, container: str | None, where: str) -> None:
    """A weight is a positive number on a child of a row that gives no width of
    its own."""
    weight = element.weight
    if weight is None:
        return
    if isinstance(weight, bool) or not isinstance(weight, (int, float)) \
            or not 0 < weight < float("inf"):
        raise ValidationError(f"the weight of {where} must be a positive number, got {weight!r}")
    if container != "row":
        raise ValidationError(f"{where} has a weight but is not a child of a row")
    if isinstance(element, Block) and element.row_basis():
        raise ValidationError(
            f"{where} has a width of its own and cannot also take a share of the row"
        )


# What a container may not hold: the blocks that belong to the page's image
# column, or own a page of their own.
def check_layout(page: Page) -> None:
    """The page's alignment, and its containers: well formed and well placed.

    Every ``flex`` block is closed by an ``end`` block, holds at least one
    block, and names a direction; a marker may carry a footnote (drawn on the
    container's last leaf) but no callouts, filters or key, since it draws
    nothing of its own. A container
    holds no ``side_image`` and no text set under one, which belong to the
    page's image column. A page set in the middle or at the bottom of its band
    cannot also share that band out with ``p.gap(...)``.
    """
    check_alignment(page, f"page {page.id}")
    valign = page_valign(page)
    if valign in ("middle", "bottom") and page.gap is not None:
        raise ValidationError(
            f"valign={valign!r} and p.gap(...) both claim the page's leftover "
            f"vertical space; {page.id} sets both"
        )
    depth = 0
    counts: list[int] = []
    directions: list[str] = []
    for block in page.blocks:
        where = f"the {block.kind} block on {page.id}"
        check_alignment(block, where)
        if block.kind != END_KIND:
            check_weight(block, directions[-1] if directions else None, where)
        if block.kind in MARKER_KINDS:
            if (block.annotations or block.only is not None
                    or block.except_ is not None or block.disabled
                    or block.key is not None):
                raise ValidationError(
                    f"a flex container carries no callouts, filters or key: "
                    f"{page.id}"
                )
        if block.kind == FLEX_KIND:
            direction = (block.content or {}).get("direction")
            if direction not in FLEX_DIRECTIONS:
                raise ValidationError(
                    f"flex direction on {page.id} must be one of "
                    f"{', '.join(FLEX_DIRECTIONS)}, got {direction!r}"
                )
            if counts:
                counts[-1] += 1
            depth += 1
            counts.append(0)
            directions.append(direction)
            continue
        if block.kind == END_KIND:
            if depth:
                directions.pop()
            if not depth:
                raise ValidationError(
                    f"an end block closes no flex container on {page.id}"
                )
            if not counts.pop():
                raise ValidationError(f"an empty flex container on {page.id}")
            depth -= 1
            continue
        if counts:
            counts[-1] += 1
        if block.kind == "side_image" and (block.halign or block.valign):
            raise ValidationError(
                f"a side_image follows the page's alignment and has none of "
                f"its own: {page.id} (use p.valign(...) or side_image(valign=...))"
            )
        if depth and (not block.IN_CONTAINER or block.in_side):
            what = "under_side_image text" if block.in_side else block.kind
            raise ValidationError(
                f"a flex container cannot hold {what}: {page.id}"
            )
    if depth:
        raise ValidationError(
            f"a flex container on {page.id} is never closed; write it as "
            f"`with p.flex(...):`"
        )


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
    block.validate(page_id, refs, keys)


# Content values that are never author prose: a path, a URL, an enum, a shell
# command, a language name. `check_marks` walks a block's content generically —
# so a block kind added tomorrow is scanned the day it lands — and these are the
# keys it must not walk into.
_NON_PROSE_KEYS = frozenset({
    "src", "srcs", "url", "link", "logo", "ref", "language", "command",
    "side", "flow", "align", "caption_align", "tone", "width", "direction",
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
    """`<mark>` is legal in slide text (`p.slide`, `p.columns`) and nowhere else.

    Refused rather than stripped: a highlight that silently vanishes looks
    exactly like a word the author never marked, so the mistake would surface on
    the projector instead of at build time. See :mod:`lecturekit.marks`.

    A `code` block is exempt from the scan entirely — it is verbatim, and a
    sample that *shows* `<mark>` as the HTML it is must not be rejected for it.
    """
    if block.VERBATIM:
        texts: list[str] = []
    elif isinstance(block.content, str):
        texts = [block.content]
    else:
        texts = _prose_strings(block.content)
    for text in texts:
        check_mark_text(
            text, allowed=block.MARKS,
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


def file_link_texts(block: Block) -> list[str]:
    """The strings of ``block`` that may carry a file link; none for most kinds."""
    if not block.FILE_LINKS:
        return []
    if isinstance(block.content, str):
        return [block.content]
    return _prose_strings(block.content)


def check_file_link_text(text: str, *, allowed: bool, where: str, page_id: str) -> None:
    for problem in filelinks.errors(text, allowed=allowed):
        raise ValidationError(f"{problem} — on {page_id}, in {where}")


def check_file_links(block: Block, page_id: str) -> None:
    """A file link is legal in slide text and table cells, and names a file
    inside the lecture directory.

    Refused elsewhere on `check_marks`' reasoning: the targets that cannot draw
    the button print the label, and a surface no renderer reads as inline
    markdown would print the markup itself. A `code` block is verbatim and is
    not scanned, and neither is a demo's recorded ``output``.
    """
    where = f"the {block.kind} block"
    for text in file_link_texts(block):
        check_file_link_text(text, allowed=True, where=where, page_id=page_id)
    if block.VERBATIM or block.FILE_LINKS:
        texts: list[str] = []
    elif isinstance(block.content, str):
        texts = [block.content]
    elif block.LINK_FREE_KEYS:
        texts = _prose_strings({
            name: value for name, value in block.content.items()
            if name not in block.LINK_FREE_KEYS
        })
    else:
        texts = _prose_strings(block.content)
    for text in texts:
        check_file_link_text(text, allowed=False, where=where, page_id=page_id)
    for footnote in block.footnotes:
        check_file_link_text(footnote, allowed=False, where="a footnote", page_id=page_id)
    for note in block.annotations:
        check_file_link_text(note.text, allowed=False, where="an annotation", page_id=page_id)


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


def logo_is_image(logo: str | None) -> bool:
    """Whether a sidenote's ``logo`` names an image (a path or a URL) rather
    than a glyph such as an emoji. Asked here, once, so that every renderer
    reads a given logo the same way."""
    if not logo:
        return False
    logo = str(logo)
    return (
        logo.startswith(("http://", "https://"))
        or "/" in logo
        or logo.lower().endswith(
            (".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp", ".bmp", ".tif", ".tiff")
        )
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


def __getattr__(name: str) -> Any:
    # ``BLOCK_KINDS`` is read off the registry, which the classes in
    # ``lecturekit.elements`` fill when that package is first imported.
    if name == "BLOCK_KINDS":
        element_class(None)
        return frozenset(_ELEMENTS)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
