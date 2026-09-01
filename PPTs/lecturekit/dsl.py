from __future__ import annotations

import inspect
import linecache
import os
from collections.abc import Callable, Iterable
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any

from . import bibtex, marks, model
from .autobold import autobold as _autobold

# Absolute path of the lecturekit package, used to tell the framework's own
# stack frames from the author's ``lecture.py`` / ``pages.py`` when a page body
# blows up. See ``_page_body_error``.
_PKG_DIR = os.path.dirname(os.path.abspath(__file__))


PageBody = Callable[["PageBuilder"], None]
CoverLogo = str | tuple[str | None, str | None] | dict[str, str | None]

# The block ``p.frames(...)`` leaves behind. It lives only between the page body
# running and the build finishing: ``build_child`` expands it into one ordinary
# ``image`` block per frame, one page each. It is deliberately absent from
# ``model.BLOCK_KINDS`` — no renderer ever sees it, and validation rejects it if
# an expansion is ever missed.
FRAMES_KIND = "frames"


class Lecture:
    def __init__(
        self,
        *,
        id: str,
        title: str,
        subtitle: str | None = None,
        ratio: str = model.DEFAULT_RATIO,
        assets: str | None = None,
        lang: str | None = None,
    ):
        self.id = id
        self.lang = lang
        self.title = title
        self.subtitle = subtitle
        self.ratio = ratio
        self.assets = assets
        self._children: list[SectionBuilder | PageSpec | BorrowedPages] = []
        # Source lectures this one replays review pages from, in the order
        # `review_section` first reached each. See `review_section`.
        self._borrowed: list[model.Borrowed] = []
        # Auto-numbered ids for `bridge(...)` pages, lecture-wide — bridges in
        # different sections must not collide.
        self._bridge_count = 0

    def section(self, title: str, *, id: str | None = None, collapsed: bool = False) -> "SectionBuilder":
        section = SectionBuilder(
            id=id or slugify(title), title=title, collapsed=collapsed, lecture=self
        )
        self._children.append(section)
        return section

    def page(
        self,
        id: str,
        *,
        body: PageBody,
        tags: Iterable[str] = (),
        annotation: bool = True,
        book: str = "page",
        book_title: str | None = None,
    ) -> "PageHandle":
        """Add a page.

        ``book`` and ``book_title`` are book-side knobs the deck never sees:
        ``book="merge"`` folds this page's book-visible blocks into the previous
        page's section, ``book="skip"`` drops it from the book, and
        ``book_title`` overrides the heading the book prints (the deck and the
        outline keep ``p.title(...)``).
        """
        spec = PageSpec(
            id=id, body=body, tags=set(tags), annotation=annotation,
            book=book, book_title=book_title,
        )
        self._children.append(spec)
        return PageHandle(lecture=self, spec=spec)

    def cover(
        self,
        title: str,
        *,
        id: str = "cover",
        author: str | None = None,
        time: str | None = None,
        logo: CoverLogo | None = None,
        tags: Iterable[str] = (),
    ) -> "PageHandle":
        """Append a title slide.

        ``title`` is the page title and is required. ``author``, ``time``, and
        ``logo`` are optional. ``logo`` may be one image path, a ``(left,
        right)`` tuple, or ``{"left": ..., "right": ...}`` for dual logos.
        """
        def body(p: PageBuilder) -> None:
            p.title(title)
            p.cover(author=author, time=time, logo=logo)

        return self.page(id, body=body, tags=tags, annotation=False)

    def bridge(self, text: str, *, id: str | None = None) -> None:
        """Add a transition page (衔接页): a line or two, centered, nothing else.

        A bridge marks the turn between topics without being a knowledge point:
        it has no title, appears in no outline, prints no slide number (and does
        not advance the count), and never reaches the book or the transcript
        sheet. ``text`` is plain text — no inline markdown, no ``<mark>`` — of at
        most :data:`model.BRIDGE_MAX_LINES` lines; blank lines are dropped.

        Returns ``None``: nothing chains off a bridge — a transition with a
        footnote to carry has outgrown being one. ``id`` defaults to
        ``bridge-1``, ``bridge-2``, …; pass one to name the page for
        ``--pages``.
        """
        self.page(id or self._next_bridge_id(), body=_bridge_body(text), book="skip")
        return None

    def _next_bridge_id(self) -> str:
        self._bridge_count += 1
        return f"bridge-{self._bridge_count}"

    def close(
        self,
        id: str,
        *,
        body: PageBody,
        tags: Iterable[str] = (),
        annotation: bool = True,
        book: str = "page",
        book_title: str | None = None,
    ) -> "PageHandle":
        """Append a closing conclusion as a single top-level page.

        A one-page conclusion is a page, not a section, so it lands as a leaf in
        the outline with nothing to expand. Like any page, its title (the outline
        label) comes from ``p.title(...)`` in ``body``. Call it last to put the
        conclusion at the end.
        """
        return self.page(
            id, body=body, tags=tags, annotation=annotation,
            book=book, book_title=book_title,
        )

    def build(self) -> model.Lecture:
        lecture = model.Lecture(
            id=self.id,
            title=self.title,
            subtitle=self.subtitle,
            ratio=self.ratio,
            lang=self.lang,
            children=build_children(self._children),
            borrowed=tuple(self._borrowed),
        )
        model.validate_lecture(lecture)
        return lecture


class SectionBuilder:
    def __init__(
        self,
        *,
        id: str,
        title: str,
        collapsed: bool = False,
        lecture: "Lecture | None" = None,
    ):
        self.id = id
        self.title = title
        self.collapsed = collapsed
        self._lecture = lecture
        self._children: list[SectionBuilder | PageSpec | BorrowedPages] = []

    def __enter__(self) -> "SectionBuilder":
        return self

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        return None

    def section(self, title: str, *, id: str | None = None, collapsed: bool = False) -> "SectionBuilder":
        section = SectionBuilder(
            id=id or slugify(title), title=title, collapsed=collapsed, lecture=self._lecture
        )
        self._children.append(section)
        return section

    def page(
        self,
        id: str,
        *,
        body: PageBody,
        tags: Iterable[str] = (),
        annotation: bool = True,
        book: str = "page",
        book_title: str | None = None,
    ) -> "PageHandle":
        spec = PageSpec(
            id=id, body=body, tags=set(tags), annotation=annotation,
            book=book, book_title=book_title,
        )
        self._children.append(spec)
        return PageHandle(lecture=self._lecture, spec=spec)

    def bridge(self, text: str, *, id: str | None = None) -> None:
        """Add a transition page inside this section. See :meth:`Lecture.bridge`."""
        if id is None:
            if self._lecture is None:
                raise model.ValidationError(
                    "bridge on a standalone section needs an explicit id "
                    "(auto ids are numbered lecture-wide)"
                )
            id = self._lecture._next_bridge_id()
        self.page(id, body=_bridge_body(text), book="skip")
        return None


@dataclass
class PageSpec:
    id: str
    body: PageBody
    tags: set[str] = field(default_factory=set)
    annotation: bool = True
    book: str = "page"
    book_title: str | None = None


@dataclass
class BorrowedPages:
    """Pages built by another lecture, dropped into this tree as-is.

    The one child kind with no body to run: `review.borrow` already built and
    re-branded these, so `build_child` just hands them over.
    """

    pages: list[model.Page]


def _bridge_lines(text: str) -> list[str]:
    """Normalize bridge text into its lines, refusing what a bridge is not.

    Each line is stripped and blank lines are dropped (they would render as
    stray gaps in a centered stack). Validated eagerly — at the ``bridge(...)``
    call — so the error points at the author's line, not at build time.
    """
    if not isinstance(text, str):
        raise model.ValidationError(f"bridge text must be a string, got {text!r}")
    lines = [line for line in (l.strip() for l in text.splitlines()) if line]
    if not lines:
        raise model.ValidationError("bridge text is empty")
    if len(lines) > model.BRIDGE_MAX_LINES:
        raise model.ValidationError(
            f"a bridge holds at most {model.BRIDGE_MAX_LINES} lines, got "
            f"{len(lines)} — a longer transition wants a normal page"
        )
    return lines


def _bridge_body(text: str) -> PageBody:
    lines = _bridge_lines(text)

    def body(p: PageBuilder) -> None:
        # The first line doubles as the page title for surfaces that need a
        # label (inspect, error messages); the deck never prints it as one.
        p.title(lines[0])
        p._block("bridge", {"text": "\n".join(lines)}, only=None, except_=None)

    return body


def review_section(
    lecture: Lecture,
    title: str,
    sources: dict[str, Iterable[str]],
    *,
    id: str | None = None,
    collapsed: bool = False,
) -> "SectionBuilder":
    """Add a section that replays pages taught in other lectures.

    ``sources`` maps a source lecture **directory** to the page ids to replay
    from it, so one review section may span several lectures:

    ```python
    review_section(lecture, "回顾：一致性协议", {
        "../two-phase-commit-replication": ["2pc-block"],
        "~/lab/other-course/consensus": ["quorum-intro"],
    })
    ```

    A relative directory resolves against **the file that calls this** — for the
    usual ``review.py`` sitting beside ``lecture.py``, that is the lecture
    directory. There is no implicit sibling lookup: a review source is always
    spelled out, because it need not live next door.

    The pages arrive as they were authored (same title, same figures, same
    bubbles) but re-branded for their new home — see ``review`` for what that
    means. Returns the section, so an author can add their own transition pages
    after the borrowed ones.
    """
    from . import review

    if not isinstance(sources, dict) or not sources:
        raise model.ValidationError(
            "review_section needs a non-empty {directory: [page ids]} mapping"
        )

    base_dir = _caller_dir()
    section = lecture.section(title, id=id, collapsed=collapsed)
    for source_spec, page_ids in sources.items():
        pages, entry = review.borrow(base_dir, source_spec, list(page_ids))
        _record_borrowed(lecture, entry, source_spec)
        section._children.append(BorrowedPages(pages))
    return section


def _record_borrowed(lecture: Lecture, entry: model.Borrowed, source_spec: str) -> None:
    """Register a source lecture, rejecting an id that is already spoken for.

    The source's id namespaces both the borrowed page ids and their assets, so
    two sources sharing one id would silently overwrite each other's figures.
    """
    if entry.lecture_id == lecture.id:
        raise model.ValidationError(
            f"review source {source_spec!r} has the same id as this lecture "
            f"({lecture.id!r}); a lecture cannot borrow from itself"
        )
    for seen in lecture._borrowed:
        if seen.lecture_id != entry.lecture_id:
            continue
        if seen.directory == entry.directory:
            return          # same lecture named twice; one record is enough
        raise model.ValidationError(
            f"two review sources share the lecture id {entry.lecture_id!r}:\n"
            f"  {seen.directory}\n  {entry.directory}"
        )
    lecture._borrowed.append(entry)


def _caller_dir() -> Path:
    """The directory of the nearest frame outside lecturekit itself.

    A review path is written in the author's file, so it should mean what it
    means *there* — not relative to wherever the CLI happened to be started.
    """
    for frame in inspect.stack():
        filename = os.path.abspath(frame.filename)
        if not filename.startswith(_PKG_DIR):
            return Path(filename).parent
    return Path.cwd()


@dataclass
class PageHandle:
    """A renderable reference to a page just added, for inline notebook display.

    Holds the owning ``Lecture`` builder (for ``id``/``title``/``ratio``/``assets``)
    and the page's ``PageSpec``. Jupyter calls ``_repr_html_`` to show the slide
    as the cell output. The notebook renderer is imported lazily so ``dsl`` stays
    free of any Marp/subprocess dependency at import time.
    """

    lecture: "Lecture"
    spec: PageSpec

    def _repr_html_(self) -> str:
        from . import notebook

        return notebook.render_page_html(self.lecture, self.spec)

    def news(self) -> "NewsPanel":
        return NewsPanel(lecture=self.lecture, spec=self.spec)


@dataclass
class NewsPanel:
    """A renderable notebook companion for a page's related reading."""

    lecture: "Lecture"
    spec: PageSpec

    def _repr_html_(self) -> str:
        from . import notebook

        return notebook.render_news_html(self.lecture, self.spec)


class PageBuilder:
    def __init__(self) -> None:
        self.blocks: list[model.Block] = []
        self.news_items: list[model.NewsItem] = []
        self.cite_items: list[model.Citation] = []
        self._title: str | None = None
        self._gap: model.PageGap | None = None

    def title(self, text: str) -> None:
        if self._title is not None:
            raise model.ValidationError("Page body sets its title more than once")
        self._title = text

    def gap(
        self,
        spread: int | str | None = None,
        *,
        min_px: int | None = None,
        max_px: int | None = None,
    ) -> None:
        """Spread this page's blocks: leftover vertical space is split evenly
        across the seams between them, each seam capped at ``spread`` pixels.

        ``p.gap()`` takes the default cap, ``p.gap(24)`` sets one, and
        ``p.gap("fill")`` lifts it so the blocks split the whole page. A cap
        under the default floor pulls the floor down with it, so ``p.gap(4)``
        means what it says; pass ``min_px`` to set that floor yourself.

        The original spelling — ``p.gap("auto", max_px=24)`` — keeps working and
        keeps its own smaller default cap, so pages written against it render
        exactly as before.
        """
        if self._gap is not None:
            raise model.ValidationError("Page body sets gap more than once")

        if spread == "auto":                      # the original spelling
            cap = model.LEGACY_GAP_MAX_PX if max_px is None else max_px
        elif spread is None:
            cap = model.DEFAULT_GAP_MAX_PX if max_px is None else max_px
        elif spread == model.GAP_FILL or type(spread) is int:
            if max_px is not None:
                raise model.ValidationError(
                    "Page gap takes a cap either positionally or as max_px, "
                    f"not both: gap({spread!r}, max_px={max_px!r})"
                )
            cap = None if spread == model.GAP_FILL else spread
        else:
            raise model.ValidationError(
                "Page gap takes a pixel cap, \"fill\", or nothing: "
                f"gap({spread!r})"
            )

        floor = min_px
        if floor is None:
            floor = model.DEFAULT_GAP_MIN_PX
            if type(cap) is int and cap < floor:
                floor = cap
        self._gap = model.PageGap(mode="auto", min_px=floor, max_px=cap)

    def spacer(
        self,
        px: int,
        *,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A fixed vertical gap (``px`` high) inserted between blocks.

        Unlike page-level ``gap(...)`` — which distributes leftover space
        evenly across *every* block seam — a spacer is a manual, exact gap at
        one position, and a page may carry several. Intended for normal (non
        ``gap(...)``) pages. Honored by the viewer/Marp deck; PPTX ignores it.
        """
        return self._block("spacer", {"px": px}, only=only, except_=except_, key=key)

    def highlight(
        self,
        text: str,
        *,
        tone: str = model.DEFAULT_HIGHLIGHT_TONE,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A centered, bold chip carrying the one line the slide wants to land.

        Newlines are kept — the chip grows taller and stays one rectangle. Each
        line takes the same inline markdown as a sidenote body, and is **not**
        autobolded (the block is already bold). ``tone`` is one of
        ``model.HIGHLIGHT_TONES``.
        """
        return self._block(
            "highlight", {"text": text, "tone": tone}, only=only, except_=except_, key=key
        )

    def slide(self, content: str, *, reveal: str = model.DEFAULT_REVEAL_MODE,
        autobold: bool = True,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None, key: str | None = None) -> "BlockHandle":
        """The deck's body text: a headline claim over its supporting bullets.

        Two rewrites happen here, so what is stored is one canonical spelling:
        the `==keyword==` shorthand becomes a `<mark>` tag, and each flush-left
        prose line is bolded. Marks expand first — a headline whose first word
        is marked must still reach `autobold` as the tag it special-cases.

        ``autobold=False`` turns the second rewrite off for the whole block —
        for the slide that is a few plain lines rather than a headline over
        bullets. Indenting a line by one space still opts *that* line out; this
        is the block-wide form of the same escape, so a paragraph does not have
        to carry a leading space on every line. Marks still expand, and an
        explicit ``**...**`` still means what it says.

        ``reveal="items"`` makes the live preview step through the block's list
        items one Enter at a time instead of lighting the whole block at once —
        for the summary slide whose bullets are meant to arrive one by one. It
        needs at least one list item to step through, so a block without one is
        a `ValidationError` rather than a step that quietly does nothing. Live
        preview only: every export renders the block whole.
        """
        if reveal not in model.REVEAL_MODES:
            raise model.ValidationError(
                f"slide reveal must be one of {sorted(model.REVEAL_MODES)}, got {reveal!r}"
            )
        content = marks.expand(content)
        if autobold:
            content = _autobold(content)
        if reveal == "items" and not model.LIST_ITEM_RE.search(content):
            raise model.ValidationError(
                'slide reveal="items" needs list items to step through, '
                "but this block has none"
            )
        return self._block(
            "slide", content, only=only, except_=except_, key=key, reveal=reveal,
            autobold=autobold,
        )

    def notes(self, content: str, *, only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None, key: str | None = None) -> "BlockHandle":
        return self._block("notes", content, only=only, except_=except_, key=key)

    def prose(self, content: str, *, only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None, key: str | None = None) -> "BlockHandle":
        """Textbook prose: rendered by the book, never by the deck.

        The counterpart of ``slide``. A page carries both bodies and each
        renderer sees only its own. Not autobolded — a paragraph has no
        headline.
        """
        return self._block("prose", content, only=only, except_=except_, key=key)

    def handout(self, content: str, *, only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None, key: str | None = None) -> "BlockHandle":
        """Deprecated alias for :meth:`prose`."""
        return self.prose(content, only=only, except_=except_, key=key)

    def code(
        self,
        language: str,
        content: str,
        *,
        mark: Iterable[int] | None = None,
        tone: str = model.DEFAULT_HIGHLIGHT_TONE,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A fenced code block. ``language="pseudo"`` is lexed by lecturekit
        itself (see :mod:`lecturekit.pseudo`).

        ``mark`` is the marker pen at line scale: a list of 1-based line numbers
        that render washed in ``tone`` (one of ``model.HIGHLIGHT_TONES``), the
        way ``<mark>`` washes a word in slide text. The numbers count the lines
        as the slide shows them — leading and trailing blank lines are stripped
        first — so the same shared listing can be marked differently on each
        page that shows it. ``pseudo`` only: another language is coloured by the
        renderer's own highlighter, which leaves nowhere to put the wash.
        """
        content_dict: dict = {"language": language, "content": content}
        if mark is not None:
            content_dict["mark"] = list(mark)
            content_dict["tone"] = tone
        return self._block("code", content_dict, only=only, except_=except_, key=key)

    def link(
        self,
        label: str,
        url: str,
        *,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        return self._block("link", {"label": label, "url": url}, only=only, except_=except_, key=key)

    def image(
        self,
        src: str,
        *,
        alt: str = "",
        caption: str | None = None,
        width_px: int | None = None,
        width_pct: int | None = None,
        height_px: int | None = None,
        height_pct: int | None = None,
        framed: bool = False,
        caption_align: str = "center",
        ref: str | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "ImageHandle":
        """Add an image. Size is unit-explicit: pass ``width_px``/``height_px``
        for pixels or ``width_pct``/``height_pct`` for a percentage of the slide.

        ``framed=True`` draws a bordered, shadowed card around the image (off by
        default, since transparent line diagrams read better unframed). The
        caption centers by default; ``caption_align`` re-aligns it to ``"left"``
        or ``"right"``. ``ref`` names the figure so book prose can cite it with
        ``[@name]``; it requires a caption.

        The same setters exist on the returned :class:`ImageHandle`, so options
        can also be chained: ``p.image(src).framed().width_px(480).footnote(...)``.
        """
        width = _size_value(width_px, width_pct, "width")
        height = _size_value(height_px, height_pct, "height")
        self._block(
            "image",
            {
                "src": src, "alt": alt, "caption": caption,
                "width": width, "height": height,
                "framed": framed, "caption_align": _caption_align(caption_align),
                "ref": ref,
            },
            only=only,
            except_=except_,
            key=key,
        )
        return ImageHandle(self, len(self.blocks) - 1)

    def frames(
        self,
        *srcs: str,
        alt: str = "",
        caption: str | None = None,
        width_px: int | None = None,
        width_pct: int | None = None,
        height_px: int | None = None,
        height_pct: int | None = None,
        framed: bool = False,
        caption_align: str = "center",
        ref: str | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """An animation: one page, one figure per frame.

        The page is expanded at build time into one slide per ``src`` — ids
        ``<page id>-1`` … ``<page id>-N``. Paging forward runs the animation;
        nothing is duplicated in the source, so the text cannot drift between
        frames. Where a block sits relative to this call decides when it shows:
        blocks written *before* it ride every frame, blocks written *after* it
        belong to the finished picture and show only on the last frame — the
        punchline lands when the animation has played. Earlier frames keep the
        held blocks' space blank, so nothing shifts between frames.

        Only the image *source* varies within the animation. Size, caption,
        frame, footnotes and bubbles are written once here and shared by every
        frame, which is what makes "same figure, drawn a step further"
        structural rather than a convention. Options mirror :meth:`image`.

        A page may carry at most one ``frames`` block. In the book the animation
        is one section printing its **last** frame — the finished picture — so a
        ``ref`` names that figure. In ``--watch`` reveal mode the first frame
        steps through the blocks before the animation, the last frame through
        the blocks after it; the frames in between arrive fully lit, one Enter
        apiece.
        """
        if not srcs:
            raise model.ValidationError("p.frames(...) needs at least one image")
        for src in srcs:
            if not isinstance(src, str) or not src.strip():
                raise model.ValidationError(f"Empty frames image src: {src!r}")
        if any(block.kind == FRAMES_KIND for block in self.blocks):
            raise model.ValidationError(
                "a page may carry at most one frames block "
                "(one page, one animation)"
            )
        width = _size_value(width_px, width_pct, "width")
        height = _size_value(height_px, height_pct, "height")
        return self._block(
            FRAMES_KIND,
            {
                "srcs": list(srcs), "alt": alt, "caption": caption,
                "width": width, "height": height,
                "framed": framed, "caption_align": _caption_align(caption_align),
                "ref": ref,
            },
            only=only,
            except_=except_,
            key=key,
        )

    def side_image(
        self,
        src: str,
        *,
        alt: str = "",
        width: str | int | None = None,
        side: str = "right",
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """Place an image in a side column; slide text reflows into the other column.

        ``side`` is "right" (default) or "left"; ``width`` sets the column width
        (e.g. "38%"), defaulting to a half-split. Renders via Marp's split
        background in the viewer/Marp deck and a floated image in slidev.
        """
        if side not in ("left", "right"):
            raise model.ValidationError(
                f"side_image side must be 'left' or 'right', got {side!r}"
            )
        return self._block(
            "side_image",
            {"src": src, "alt": alt, "width": width, "side": side},
            only=only,
            except_=except_,
            key=key,
        )

    def demo(
        self,
        name: str,
        command: str,
        *,
        output: str | None = None,
        description: str | None = None,
        timeout: float | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A command, shown as a transcript and runnable from the live preview.

        ``command`` is a shell command line, and may be several lines — they run
        in order, in the lecture's own directory. ``output`` is what it printed
        when the author ran it: the slide shows that, so the page carries the
        point whether or not anyone presses the button, and PDF and PPTX carry
        it too. Pressing the button under ``view --watch --demo`` runs the real
        thing and streams it into a drawer, leaving the slide as it is.

        ``timeout`` is in seconds; ``0`` means the command has no natural end
        (``ollama serve``) and is stopped by hand. Omitted, the session's
        ``--demo-timeout`` applies.
        """
        return self._block(
            "demo",
            {
                "name": name,
                "command": command,
                "output": output,
                "description": description,
                "timeout": timeout,
            },
            only=only,
            except_=except_,
            key=key,
        )

    def aside(self, content: str, *, only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None, key: str | None = None) -> "BlockHandle":
        return self._block("aside", content, only=only, except_=except_, key=key)

    def sidenote(
        self,
        title: str,
        text: str,
        *,
        link: str | None = None,
        logo: str | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A boxed callout pointing at external material.

        Renders a logo (default 📖, or a custom emoji/glyph or image path),
        a bold title (linked when ``link`` is given), and body text.
        """
        return self._block(
            "sidenote",
            {"title": title, "text": text, "link": link, "logo": logo},
            only=only,
            except_=except_,
            key=key,
        )

    def news(
        self,
        title: str,
        *,
        url: str,
        source: str | None = None,
        date: str | None = None,
        kind: str = "news",
        why: str | None = None,
        tags: Iterable[str] = (),
        image: str | None = None,
        archived_url: str | None = None,
    ) -> "NewsHandle":
        """Attach student-facing companion reading material to this page.

        News items are page metadata, not slide blocks, so normal deck renderers
        ignore them unless they explicitly implement a companion surface.
        """
        self.news_items.append(
            model.NewsItem(
                title=title,
                url=url,
                source=source,
                date=date,
                kind=kind,
                why=why,
                tags=tuple(str(tag).strip() for tag in tags),
                image=image,
                archived_url=archived_url,
            )
        )
        return NewsHandle(self, len(self.news_items) - 1)

    def cite(
        self,
        bibtex_str: str | None = None,
        *,
        title: str | None = None,
        author: str | None = None,
        year: str | None = None,
        venue: str | None = None,
        url: str | None = None,
        key: str | None = None,
    ) -> "CiteHandle":
        """Attach a reference to this page — off the slide, collected at the end.

        Citations never render in the slide body. They surface only in the deck's
        trailing 参考文献 page (each entry tagged with the pages that cited it)
        and the book's chapter-end 参考文献 section.

        Pass structured fields, or a BibTeX entry string as the first argument
        (``p.cite("@article{…}")``) to have its fields parsed. Explicit keyword
        fields always override the parsed ones. A ``title`` — given directly or
        via BibTeX — is required.
        """
        fields = bibtex.parse(bibtex_str) if bibtex_str is not None else {}
        title = title if title is not None else fields.get("title")
        if not title:
            raise model.ValidationError(
                "p.cite(...) needs a title (as a field or parsed from BibTeX)"
            )
        self.cite_items.append(
            model.Citation(
                title=title,
                author=author if author is not None else fields.get("author"),
                year=year if year is not None else fields.get("year"),
                venue=venue if venue is not None else fields.get("venue"),
                url=url if url is not None else fields.get("url"),
                key=key if key is not None else fields.get("key"),
            )
        )
        return CiteHandle(self, len(self.cite_items) - 1)

    def table(
        self,
        rows: Iterable[Iterable[str]],
        *,
        headers: Iterable[str],
        align: Iterable[str] | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """A GFM table from a header list plus rows.

        ``headers`` is required (a GFM table must have a header row). ``align``,
        when given, is one of ``"left"``/``"center"``/``"right"`` per column.
        Cells carry inline markdown, passed through to Marp.
        """
        header_list = [str(h) for h in headers]
        row_list = [[str(cell) for cell in row] for row in rows]
        width = len(header_list)
        for row in row_list:
            if len(row) != width:
                raise model.ValidationError(
                    f"table row has {len(row)} cells, expected {width}"
                )
        align_list = None
        if align is not None:
            align_list = [str(a) for a in align]
            if len(align_list) != width:
                raise model.ValidationError(
                    f"table align has {len(align_list)} entries, expected {width}"
                )
            for a in align_list:
                if a not in ("left", "center", "right"):
                    raise model.ValidationError(
                        f"table align must be left/center/right, got {a!r}"
                    )
        return self._block(
            "table",
            {"headers": header_list, "rows": row_list, "align": align_list},
            only=only,
            except_=except_,
            key=key,
        )

    def architecture(
        self,
        *,
        caption: str | None = None,
        flow: str | None = None,
        ref: str | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "ArchHandle":
        """Start a layered + modular architecture diagram.

        Returns an :class:`ArchHandle`; add bands with ``arch.layer(title,
        modules)``, each a list of module-box labels. ``flow`` ∈
        ``{None, "down", "up", "both"}`` draws a chevron between adjacent layers
        (``None`` = plain stacked bands). Pass ``title=None``/``""`` to ``layer``
        for an unlabeled band. Chains footnotes/annotations like any block.
        """
        self._block(
            "architecture",
            {"caption": caption, "flow": _arch_flow(flow), "layers": [], "ref": ref},
            only=only,
            except_=except_,
            key=key,
        )
        return ArchHandle(self, len(self.blocks) - 1)

    def row(
        self,
        *,
        caption: str | None = None,
        ref: str | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "RowHandle":
        """Start a row of side-by-side images.

        Returns a :class:`RowHandle`; add images with ``row.image(src, ...)``,
        each taking the same size/caption/frame options as
        :meth:`PageBuilder.image`. With no per-image width, images share the row
        width evenly; a sized image takes its width and the rest split what's
        left. ``caption`` renders below the whole row. Chains
        ``.footnote(...)`` / ``.annotate(...)`` like any block.
        """
        self._block(
            "row",
            {"caption": caption, "items": [], "ref": ref},
            only=only,
            except_=except_,
            key=key,
        )
        return RowHandle(self, len(self.blocks) - 1)

    def cover(
        self,
        *,
        author: str | None = None,
        time: str | None = None,
        logo: CoverLogo | None = None,
        only: Iterable[str] | None = None,
        except_: Iterable[str] | None = None,
        key: str | None = None,
    ) -> "BlockHandle":
        """Add cover-page metadata.

        The page title remains the cover title. ``logo`` accepts one path, a
        ``(left, right)`` tuple, or a ``{"left": ..., "right": ...}`` mapping.
        """
        return self._block(
            "cover",
            {"author": author, "time": time, "logo": _cover_logo(logo)},
            only=only,
            except_=except_,
            key=key,
        )

    def _block(
        self,
        kind: str,
        content: Any,
        *,
        only: Iterable[str] | None,
        except_: Iterable[str] | None,
        key: str | None = None,
        reveal: str = model.DEFAULT_REVEAL_MODE,
        autobold: bool = True,
    ) -> "BlockHandle":
        self.blocks.append(
            model.Block(
                kind=kind,
                content=content,
                only=set(only) if only is not None else None,
                except_=set(except_) if except_ is not None else None,
                key=key,
                reveal=reveal,
                autobold=autobold,
            )
        )
        return BlockHandle(self, len(self.blocks) - 1)


class BlockHandle:
    """A reference to the block just added, so the author can chain a footnote.

    The block is frozen, so ``footnote`` replaces it in place with a copy that
    carries one more footnote, and returns ``self`` for ``.footnote(a).footnote(b)``.
    """

    def __init__(self, builder: "PageBuilder", index: int):
        self._builder = builder
        self._index = index

    def footnote(self, text: str) -> "BlockHandle":
        block = self._builder.blocks[self._index]
        self._builder.blocks[self._index] = replace(
            block, footnotes=block.footnotes + (text,)
        )
        return self

    def disable(self) -> "BlockHandle":
        """Turn this block off: it then renders in no target at all.

        An unconditional off switch — the deck, the book, and PPTX all skip it,
        ahead of any ``only``/``except_`` override. The block stays in the source
        (and the serialized AST), so it is a way to hold finished text back rather
        than delete it. On a ``prose`` block (book-only) this means the paragraph
        is kept in the source but left out of the book; the page reverts to
        showing only its figures, with no ``TODO`` box and no dent in ``--stats``.
        Chains and returns ``self`` like :meth:`footnote`.
        """
        block = self._builder.blocks[self._index]
        self._builder.blocks[self._index] = replace(block, disabled=True)
        return self

    def ref(self, name: str) -> "BlockHandle":
        """Name this figure so book prose can cite it with ``[@name]``.

        Only figure blocks (``image``, ``row``, ``architecture``) can carry a
        ref, and the figure needs a caption — the book's figure number, which
        the reference resolves to, rides the caption. Mirrors the ``ref=``
        keyword on the corresponding builder methods; the last call wins.
        """
        block = self._builder.blocks[self._index]
        if block.kind not in model.REF_KINDS:
            raise model.ValidationError(
                f"ref may only be attached to a figure block "
                f"({', '.join(sorted(model.REF_KINDS))}), got {block.kind!r}"
            )
        block.content["ref"] = name
        return self

    def annotate(
        self, text: str, *, at: str = "top-right", dx: int = 0, dy: int = 0
    ) -> "BlockHandle":
        """Float a callout bubble over the slide, attached to this block.

        ``at`` is one of :data:`model.ANNOTATION_ANCHORS` (default ``top-right``)
        and ``dx``/``dy`` nudge the bubble from that anchor in pixels. Chains and
        accumulates like :meth:`footnote`: ``.annotate(a).annotate(b)`` adds two.
        """
        block = self._builder.blocks[self._index]
        note = model.Annotation(text=text, at=at, dx=dx, dy=dy)
        self._builder.blocks[self._index] = replace(
            block, annotations=block.annotations + (note,)
        )
        return self

    def image_right(
        self,
        src: str,
        *,
        alt: str = "",
        width_px: int | None = None,
        width_pct: int | None = None,
        height_px: int | None = None,
        height_pct: int | None = None,
    ) -> "BlockHandle":
        """Float a small image to the right of this slide block's text.

        Only valid on a ``slide`` block (text wraps around the image, which is
        meaningless on, e.g., an image or code block). Size is unit-explicit and
        mirrors :meth:`PageBuilder.image`; passing both units for one dimension
        raises ``ValidationError``. Returns ``self`` so it chains with
        ``footnote`` / ``annotate``; a later call replaces the prior image.
        """
        block = self._builder.blocks[self._index]
        if block.kind != "slide":
            raise model.ValidationError(
                f"image_right may only be attached to a slide block, got {block.kind!r}"
            )
        width = _size_value(width_px, width_pct, "width")
        height = _size_value(height_px, height_pct, "height")
        self._builder.blocks[self._index] = replace(
            block,
            float_image={
                "src": src,
                "alt": alt,
                "width": width,
                "height": height,
                "side": "right",
            },
        )
        return self


class NewsHandle:
    """A reference to a page-level news item.

    The first version has no chainable mutators; the handle exists so the DSL
    can grow later without changing ``p.news(...)``'s return shape.
    """

    def __init__(self, builder: "PageBuilder", index: int):
        self._builder = builder
        self._index = index


class CiteHandle:
    """A reference to a page-level citation.

    Like ``NewsHandle``, it carries no chainable mutators yet; it exists so the
    DSL can grow later without changing ``p.cite(...)``'s return shape.
    """

    def __init__(self, builder: "PageBuilder", index: int):
        self._builder = builder
        self._index = index


def _size_value(px: int | None, pct: int | None, dim: str) -> str | None:
    """Resolve a pixel/percentage pair into a single CSS length string.

    Raises ``ValidationError`` if both units are given for the same dimension.
    """
    if px is not None and pct is not None:
        raise model.ValidationError(
            f"image {dim}: pass {dim}_px or {dim}_pct, not both"
        )
    if px is not None:
        return f"{px}px"
    if pct is not None:
        return f"{pct}%"
    return None


_CAPTION_ALIGNMENTS = ("left", "center", "right")


def _caption_align(value: str) -> str:
    """Validate a caption alignment, defaulting to the centered look."""
    if value not in _CAPTION_ALIGNMENTS:
        raise model.ValidationError(
            f"image caption_align: expected one of {_CAPTION_ALIGNMENTS}, got {value!r}"
        )
    return value


def _arch_flow(value: str | None) -> str | None:
    """Validate an architecture flow direction, eagerly at authoring time."""
    if value is not None and value not in model.ARCH_FLOWS:
        raise model.ValidationError(
            f"architecture flow: expected one of {sorted(model.ARCH_FLOWS)} "
            f"or None, got {value!r}"
        )
    return value


def _cover_logo(logo: CoverLogo | None) -> str | dict[str, str | None] | None:
    if logo is None or isinstance(logo, str):
        return logo
    if isinstance(logo, tuple):
        if len(logo) != 2:
            raise model.ValidationError("cover logo tuple must be (left, right)")
        return {"left": logo[0], "right": logo[1]}
    if isinstance(logo, dict):
        unknown = set(logo) - {"left", "right"}
        if unknown:
            raise model.ValidationError(
                f"cover logo mapping accepts only left/right, got {sorted(unknown)}"
            )
        return {"left": logo.get("left"), "right": logo.get("right")}
    raise model.ValidationError(
        "cover logo must be a string, (left, right) tuple, or left/right mapping"
    )


def _arch_module(value) -> str:
    """Normalize one module entry, mapping the ellipsis to its canonical token.

    The Python ellipsis literal ``...`` (and the strings ``"..."``/``"…"``) mean
    "more modules here, not drawn" and collapse to :data:`model.ARCH_ELLIPSIS`.
    Anything else is coerced to ``str`` like a table cell.
    """
    if value is ... or (isinstance(value, str) and value.strip() in ("...", "…")):
        return model.ARCH_ELLIPSIS
    return str(value)


class ArchHandle(BlockHandle):
    """A :class:`BlockHandle` for an architecture block, adding ``layer``.

    Each ``layer(title, modules)`` appends one band to the diagram, top to
    bottom, and returns ``self`` so layers chain alongside a footnote:
    ``p.architecture().layer("App", [...]).layer("Kernel", [...]).footnote(...)``.
    """

    def layer(self, title: str | None, modules: Iterable[str]) -> "ArchHandle":
        module_list = [_arch_module(m) for m in modules]
        layers = self._builder.blocks[self._index].content["layers"]
        layers.append({"title": title, "modules": module_list})
        return self


class RowHandle(BlockHandle):
    """A :class:`BlockHandle` for a row block, adding ``image``.

    Each ``image(src, ...)`` appends one figure to the row and returns ``self``
    so images chain alongside a footnote:
    ``p.row().image("a.svg").image("b.svg").footnote(...)``. The per-image
    options mirror :meth:`PageBuilder.image`.
    """

    def image(
        self,
        src: str,
        *,
        alt: str = "",
        caption: str | None = None,
        width_px: int | None = None,
        width_pct: int | None = None,
        height_px: int | None = None,
        height_pct: int | None = None,
        framed: bool = False,
        caption_align: str = "center",
    ) -> "RowHandle":
        item = {
            "src": src,
            "alt": alt,
            "caption": caption,
            "width": _size_value(width_px, width_pct, "width"),
            "height": _size_value(height_px, height_pct, "height"),
            "framed": framed,
            "caption_align": _caption_align(caption_align),
        }
        self._builder.blocks[self._index].content["items"].append(item)
        return self


class ImageHandle(BlockHandle):
    """A :class:`BlockHandle` for image blocks, adding unit-explicit size setters.

    ``width_px``/``height_px`` set a pixel size; ``width_pct``/``height_pct`` set
    a percentage of the slide. ``framed()`` draws the bordered card; and
    ``caption_align()`` re-aligns the caption. Each returns ``self`` so options
    and footnotes chain: ``p.image(src).framed().width_px(480).footnote(...)``.
    The setters mirror the keyword arguments on :meth:`PageBuilder.image`; the
    last call for a given option wins.
    """

    def width_px(self, value: int) -> "ImageHandle":
        self._content()["width"] = f"{value}px"
        return self

    def width_pct(self, value: int) -> "ImageHandle":
        self._content()["width"] = f"{value}%"
        return self

    def height_px(self, value: int) -> "ImageHandle":
        self._content()["height"] = f"{value}px"
        return self

    def height_pct(self, value: int) -> "ImageHandle":
        self._content()["height"] = f"{value}%"
        return self

    def framed(self, value: bool = True) -> "ImageHandle":
        self._content()["framed"] = value
        return self

    def caption_align(self, value: str) -> "ImageHandle":
        self._content()["caption_align"] = _caption_align(value)
        return self

    def _content(self) -> dict:
        return self._builder.blocks[self._index].content


def _page_body_error(child: PageSpec, exc: Exception) -> model.ValidationError:
    """Wrap an exception raised inside a page body with author-facing location.

    A page ``body`` is arbitrary author code (``lecture.py`` / ``pages.py``), so a
    typo like a missing ``p.image(...)`` argument surfaces as a bare ``TypeError``
    with no hint of *which* page or *which* line. This walks the traceback to the
    deepest frame outside the lecturekit package — the author's own call site —
    and re-raises a ``ValidationError`` naming the page id, the ``file:line``, and
    the offending source line, so every path (inspect / render / watch) points
    straight at the mistake. The original exception is chained (``from exc``) so a
    full traceback is still available when wanted.
    """
    frame = None
    tb = exc.__traceback__
    while tb is not None:
        filename = os.path.abspath(tb.tb_frame.f_code.co_filename)
        if not filename.startswith(_PKG_DIR):
            frame = tb  # keep the deepest author frame
        tb = tb.tb_next

    location = ""
    if frame is not None:
        filename = frame.tb_frame.f_code.co_filename
        lineno = frame.tb_lineno
        func = frame.tb_frame.f_code.co_name
        source = linecache.getline(filename, lineno).strip()
        location = f"\n  at {os.path.basename(filename)}:{lineno} in {func}()"
        if source:
            location += f"\n    {source}"

    kind = type(exc).__name__
    return model.ValidationError(
        f"while building page '{child.id}':{location}\n  {kind}: {exc}"
    )


def build_children(
    children: Iterable[SectionBuilder | PageSpec | BorrowedPages],
) -> list[model.Section | model.Page]:
    """Build one level of the authored tree into model nodes, in order."""
    out: list[model.Section | model.Page] = []
    for child in children:
        out.extend(build_child(child))
    return out


def build_child(child: SectionBuilder | PageSpec | BorrowedPages) -> list[model.Section | model.Page]:
    """Build one authored child into the node(s) it becomes.

    Usually exactly one — but a page whose body called ``p.frames(...)`` becomes
    one page per frame, and a borrowed animation arrives as a run of pages,
    which is why this returns a list.
    """
    if isinstance(child, BorrowedPages):
        return list(child.pages)
    if isinstance(child, PageSpec):
        builder = PageBuilder()
        try:
            child.body(builder)
        except Exception as exc:
            # Author code (a missing arg, a bad DSL value, a typo) raised while
            # building this page. Re-raise with the page id and source line so
            # the failure points at the mistake instead of a bare message.
            raise _page_body_error(child, exc) from exc
        if builder._title is None:
            raise model.ValidationError(f"Page body never set a title: {child.id}")
        return _build_pages(child, builder)
    return [model.Section(
        id=child.id,
        title=child.title,
        children=build_children(child._children),
        collapsed=child.collapsed,
    )]


def _build_pages(spec: PageSpec, builder: PageBuilder) -> list[model.Page]:
    """The page(s) a built body becomes: one, or one per animation frame."""
    def page(
        *, id: str, blocks: list[model.Block], book: str,
        frame_group: model.FrameGroup | None = None,
    ) -> model.Page:
        return model.Page(
            id=id,
            title=builder._title,
            blocks=blocks,
            tags=spec.tags,
            show_annotations=spec.annotation,
            news=tuple(builder.news_items),
            citations=tuple(builder.cite_items),
            gap=builder._gap,
            book=book,
            book_title=spec.book_title,
            frame_group=frame_group,
        )

    index = next(
        (i for i, block in enumerate(builder.blocks) if block.kind == FRAMES_KIND),
        None,
    )
    if index is None:
        return [page(id=spec.id, blocks=builder.blocks, book=spec.book)]

    frames = builder.blocks[index]
    srcs = frames.content["srcs"]
    # Blocks written after the animation belong to its finished picture: the
    # mark makes every frame but the last hold them back (shown nowhere, their
    # space reserved), so a punchline lands when the animation has played.
    tail = [
        replace(block, after_frames=True) for block in builder.blocks[index + 1:]
    ]
    pages = []
    for number, src in enumerate(srcs, start=1):
        last = number == len(srcs)
        blocks = builder.blocks[:index] + [
            _frame_image(frames, src, ref=frames.content["ref"] if last else None)
        ] + tail
        pages.append(page(
            id=f"{spec.id}-{number}",
            blocks=blocks,
            # The book prints an animation as one section showing the finished
            # picture, so the last frame carries the author's book mode and the
            # earlier frames drop out exactly like a book="skip" page.
            book=spec.book if last else "skip",
            frame_group=model.FrameGroup(id=spec.id, index=number, total=len(srcs)),
        ))
    return pages


def _frame_image(frames: model.Block, src: str, *, ref: str | None) -> model.Block:
    """One frame as an ordinary image block: shared options, this frame's source.

    Footnotes, bubbles, and the only/except overrides ride along unchanged — they
    were authored on the animation, so they belong to every frame. The figure
    ``ref`` is the exception: it names the figure the book prints, and the book
    prints the last frame, so repeating it would define the same anchor N times.
    """
    content = dict(frames.content)
    del content["srcs"]
    content["src"] = src
    content["ref"] = ref
    return replace(frames, kind="image", content=content)


def slugify(text: str) -> str:
    lowered = text.strip().lower()
    slug = []
    previous_dash = False
    for char in lowered:
        if char.isalnum():
            slug.append(char)
            previous_dash = False
        elif not previous_dash:
            slug.append("-")
            previous_dash = True
    return "".join(slug).strip("-") or "section"
