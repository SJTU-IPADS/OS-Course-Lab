"""Render a BookModel to a compilable LaTeX tree.

One chapter per lecture. The lecture's own tree steps the sectioning depth
down: a Section is a `\\section`, a Section inside it a `\\subsection`, and a
Page renders one level below its parent section. Only `book.tex` carries the
preamble, so a single chapter can be compiled alone with `\\includeonly`.
"""

from __future__ import annotations

import re
import sys
from dataclasses import replace
from pathlib import Path

from lecturekit import model, references
from lecturekit.book import BookModel

from .assets import AssetCopier
from .blocks import LATEX_KINDS, Ctx, emit_block, emit_citations, emit_news
from .preamble import MAKEFILE, document_preamble
from .text import blocks as md_blocks
from .text import escape, inline

# Depth 0 is the chapter (the lecture itself); a Section starts at depth 1.
_HEADINGS = ["chapter", "section", "subsection", "subsubsection", "paragraph"]


class LatexRenderer:
    name = "latex"

    def render(self, book: BookModel, output_dir: Path) -> Path:
        output_dir = Path(output_dir)
        (output_dir / "chapters").mkdir(parents=True, exist_ok=True)
        assets = AssetCopier(output_dir)

        includes = []
        for lecture in book.lectures:
            chapter = _chapter(lecture, book.asset_roots[lecture.id], assets)
            (output_dir / "chapters" / f"{lecture.id}.tex").write_text(
                chapter, encoding="utf-8"
            )
            includes.append(f"\\include{{chapters/{lecture.id}}}")

        (output_dir / "Makefile").write_text(MAKEFILE, encoding="utf-8")

        entry = output_dir / "book.tex"
        entry.write_text(_document(book, includes), encoding="utf-8")
        return entry


def coverage(book: BookModel) -> list[tuple[str, int, int]]:
    """``(lecture id, sections carrying prose, total sections)`` per lecture.

    Counted over the book's own units: a run of merged pages is one section,
    and a ``book="skip"`` page is not in the denominator at all.
    """
    rows = []
    for lecture in book.lectures:
        pages = model.flatten_pages(_book_children(lecture.children))
        # A page that disabled its prose opted out of carrying book text — it is
        # neither written nor a TODO, so it drops out of the denominator, like a
        # book="skip" page.
        units = [page for page in pages if not _opted_out_of_prose(page)]
        written = sum(1 for page in units if _has_prose(page))
        rows.append((lecture.id, written, len(units)))
    return rows


def _book_children(
    children: list[model.Section | model.Page],
) -> list[model.Section | model.Page]:
    """The tree as the book sees it: pages skipped, merged, and retitled.

    A ``book="skip"`` page vanishes. A ``book="merge"`` page folds its blocks
    and news into the nearest earlier page sibling (validation guarantees one
    exists), so the group becomes a single Page — one heading, one TODO check.
    ``book_title``, when set, replaces the heading text.
    """
    out: list[model.Section | model.Page] = []
    for child in children:
        if isinstance(child, model.Section):
            out.append(replace(child, children=_book_children(child.children)))
            continue
        if child.book == "skip":
            continue
        if child.book == "merge" and out and isinstance(out[-1], model.Page):
            host = out[-1]
            out[-1] = replace(
                host,
                blocks=list(host.blocks) + list(child.blocks),
                news=host.news + child.news,
                citations=host.citations + child.citations,
            )
            continue
        out.append(replace(child, title=child.book_title or child.title))
    return out


_REF_TOKEN_RE = re.compile(r"\[@([A-Za-z0-9:_-]+)\]")


def _warn_dangling_refs(lecture: model.Lecture) -> None:
    """Warn about ``[@name]`` tokens that no figure in this lecture defines.

    Only bare names are checked — a ``lecid:name`` token points at another
    chapter, which this render cannot see. A dangling ref still compiles (LaTeX
    prints ``??``), so this is a warning, not an error.
    """
    defined: set[str] = set()
    used: set[str] = set()
    for page in model.flatten_pages(_book_children(lecture.children)):
        for block in page.blocks:
            if block.disabled:  # a disabled block neither defines nor cites a ref
                continue
            if isinstance(block.content, dict) and block.content.get("ref"):
                defined.add(block.content["ref"])
            used.update(_REF_TOKEN_RE.findall(repr(block.content)))
            used.update(_REF_TOKEN_RE.findall(repr(block.footnotes)))
    for name in sorted(used - defined):
        if ":" in name:
            continue
        print(
            f"lecturekit: {lecture.id}: [@{name}] does not match any figure ref",
            file=sys.stderr,
        )


def _has_prose(page: model.Page) -> bool:
    return any(_is_body_text(block)
               for block in model.select_blocks(page, "latex", set(LATEX_KINDS)))


def _opted_out_of_prose(page: model.Page) -> bool:
    """The page authored book text, then disabled it — so it wants no prose.

    Distinguishes a deliberately figure-only page (`p.prose(...).disable()`) from
    one that was simply never written: the former has a disabled book-text block
    and no live one, so it earns no TODO box and leaves the coverage count. A
    plain disabled *deck* `slide` is not book text and does not count — only a
    `prose` block or a slide the author forced into the book with `only=["latex"]`.
    """
    return (not _has_prose(page)
            and any(block.disabled and _would_supply_prose(block)
                    for block in page.blocks))


def _would_supply_prose(block: model.Block) -> bool:
    if block.kind == "prose":
        return True
    return (block.kind == "slide"
            and block.only is not None and "latex" in block.only)


def _is_body_text(block: model.Block) -> bool:
    """Does this block supply the page's book prose?

    A `slide` reaches the book only when the author forced it in with
    `only=["latex"]`, which is a deliberate "use this text as the prose".
    """
    return block.kind in ("prose", "slide")


def _document(book: BookModel, includes: list[str]) -> str:
    parts = [document_preamble(book), "", r"\begin{document}", r"\maketitle"]
    if book.preface:
        parts += [
            r"\chapter*{前言}",
            r"\addcontentsline{toc}{chapter}{前言}",
            md_blocks(book.preface),
        ]
    parts.append(r"\tableofcontents")
    parts += ["", *includes, "", r"\end{document}", ""]
    return "\n".join(parts)


def _chapter(lecture: model.Lecture, asset_root: Path, assets: AssetCopier) -> str:
    width = model.RATIOS[lecture.ratio][0]
    parts = [f"\\chapter{{{inline(lecture.title)}}}", ""]
    news: list[model.NewsItem] = []

    _warn_dangling_refs(lecture)
    book_children = _book_children(lecture.children)
    for child in book_children:
        parts.append(_node(child, depth=1, lecture=lecture, asset_root=asset_root,
                           assets=assets, width=width, news=news))

    reading = emit_news(tuple(news), lecture.lang)
    if reading:
        parts += ["", reading]
    cited = [c for page in model.flatten_pages(book_children) for c in page.citations]
    refs = emit_citations(
        tuple(references.dedup_citations(cited)), lecture.lang
    )
    if refs:
        parts += ["", refs]
    return "\n".join(parts) + "\n"


def _node(
    node: model.Section | model.Page,
    *,
    depth: int,
    lecture: model.Lecture,
    asset_root: Path,
    assets: AssetCopier,
    width: int,
    news: list[model.NewsItem],
) -> str:
    if depth >= len(_HEADINGS):
        raise model.ValidationError(
            f"{lecture.id}: nesting is deeper than LaTeX can section "
            f"(past \\{_HEADINGS[-1]})"
        )
    if isinstance(node, model.Page):
        return _page(node, depth=depth, lecture=lecture, asset_root=asset_root,
                     assets=assets, width=width, news=news)

    parts = [f"\\{_HEADINGS[depth]}{{{inline(node.title)}}}", ""]
    for child in node.children:
        parts.append(_node(child, depth=depth + 1, lecture=lecture,
                           asset_root=asset_root, assets=assets, width=width, news=news))
    return "\n".join(parts)


def _page(
    page: model.Page,
    *,
    depth: int,
    lecture: model.Lecture,
    asset_root: Path,
    assets: AssetCopier,
    width: int,
    news: list[model.NewsItem],
) -> str:
    news.extend(page.news)
    parts = [f"\\{_HEADINGS[depth]}{{{inline(page.title)}}}", ""]

    selected = model.select_blocks(page, "latex", set(LATEX_KINDS))
    if not any(_is_body_text(block) for block in selected) and not _opted_out_of_prose(page):
        # The id is typeset by \booktodo, so it needs escaping like any text.
        parts += [f"\\booktodo{{{escape(page.id)}}}", ""]

    ctx = Ctx(lecture_id=lecture.id, page_id=page.id, slide_width=width,
              assets=assets, asset_root=asset_root, lang=lecture.lang)
    for block in selected:
        # A slide forced in with only=["latex"] renders as the prose it stands in for.
        kind = "prose" if block.kind == "slide" else block.kind
        parts += [emit_block(model.Block(
            kind=kind, content=block.content, footnotes=block.footnotes,
        ), ctx), ""]
    return "\n".join(parts)
