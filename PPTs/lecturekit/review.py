"""Replaying another lecture's pages as review.

A lecture often opens by looking back at pages taught earlier. Rather than
copying those slides (which then drift from the original), a lecture *borrows*
them: `dsl.review_section` names a source lecture directory and the page ids to
replay, and this module loads that lecture and hands back its built pages,
re-branded for their new home.

Re-branding is three rewrites, all driven by the source lecture's own
``Lecture(id=...)`` -- never by the path the author typed, so moving a lecture
directory changes nothing downstream:

- ids gain a ``<source id>/`` prefix, so they cannot collide with the host's
  own pages and ``--pages`` can still name one;
- image sources gain an ``assets/<source id>/`` prefix, matching where the
  renderers copy the source lecture's assets to;
- the page is forced to ``book="skip"``, because a chapter must not reprint
  another chapter, and its figure ``ref``s are dropped along with it (they
  anchor figures in the source chapter, and would otherwise collide).

Everything else -- title, blocks, tags, annotations, notes, citations, and the
frames of an animation -- rides along untouched: a review slide is the original
slide, not a variation on it.
"""

from __future__ import annotations

from dataclasses import replace
from pathlib import Path

from . import model
from .model import ValidationError

# Image sources are lecture-relative, and only the lecture's `assets/` tree is
# ever copied into a bundle, so that prefix is exactly the set of paths worth
# rewriting. An emoji sidenote logo, an http URL, or an absolute path is left
# alone by the same rule.
_ASSET_PREFIX = "assets/"


def borrow(base_dir: Path, source_spec: str, page_ids: list[str]) -> tuple[list[model.Page], model.Borrowed]:
    """Load `source_spec` and return its `page_ids`, re-branded for a host lecture.

    `base_dir` is what a relative `source_spec` resolves against; `~` and
    absolute paths are taken as given. Returns the pages in the order asked for,
    plus the `Borrowed` record naming the source for the renderers.
    """
    directory = resolve_dir(base_dir, source_spec)
    source = load_source(directory, source_spec)
    entry = model.Borrowed(lecture_id=source.id, directory=str(directory))
    pages = [
        rebrand(page, source.id)
        for page in take_pages(source, page_ids, source_spec)
    ]
    return pages, entry


def resolve_dir(base_dir: Path, source_spec: str) -> Path:
    """Where `source_spec` points. Relative paths hang off `base_dir`."""
    path = Path(source_spec).expanduser()
    if not path.is_absolute():
        path = Path(base_dir, path)
    return path.resolve()


def load_source(directory: Path, source_spec: str) -> model.Lecture:
    if not directory.is_dir():
        raise ValidationError(
            f"review source {source_spec!r} is not a directory: {directory}"
        )
    # Imported here, not at module scope: `cli` imports `dsl`, which imports
    # this module, so a top-level import would close the cycle.
    from .cli import load_lecture

    try:
        return load_lecture(directory)
    except Exception as exc:
        raise ValidationError(
            f"review source {source_spec!r} ({directory}) failed to load: {exc}"
        ) from exc


def take_pages(source: model.Lecture, page_ids: list[str], source_spec: str) -> list[model.Page]:
    """The source pages named by `page_ids`, in that order.

    An animation's authored id selects every one of its frames -- a `p.frames`
    page is one slide to everyone else, and replaying half of it would replay a
    half-drawn figure.
    """
    if not page_ids:
        raise ValidationError(f"review source {source_spec!r} names no pages")

    order = model.flatten_pages(source.children)
    by_id = {page.id: page for page in order}
    groups = {
        gid: [order[position] for position in positions]
        for gid, positions in model.frame_groups(order).items()
    }

    taken: list[model.Page] = []
    for page_id in page_ids:
        if page_id in groups:
            taken.extend(groups[page_id])
        elif page_id in by_id:
            taken.append(by_id[page_id])
        else:
            raise ValidationError(
                f"review: {source.id} has no page {page_id!r}. "
                f"Available: {', '.join(available_ids(order, groups))}"
            )
    return taken


def available_ids(order: list[model.Page], groups: dict[str, list[model.Page]]) -> list[str]:
    """Ids an author may ask for: real pages, with each animation folded to one."""
    frames = {page.id for pages in groups.values() for page in pages}
    listed = [page.id for page in order if page.id not in frames]
    return sorted(listed + list(groups))


def rebrand(page: model.Page, source_id: str) -> model.Page:
    """One source page, renamed and re-rooted for the borrowing lecture."""
    group = page.frame_group
    return replace(
        page,
        id=f"{source_id}/{page.id}",
        blocks=[_block(block, source_id) for block in page.blocks],
        book="skip",
        frame_group=(
            None if group is None
            else replace(group, id=f"{source_id}/{group.id}")
        ),
    )


def _block(block: model.Block, source_id: str) -> model.Block:
    return replace(
        block,
        content=_content(block.kind, block.content, source_id),
        float_image=_srcs(block.float_image, source_id),
    )


def _content(kind: str, content: object, source_id: str) -> object:
    if not isinstance(content, dict):
        return content
    out = _srcs(content, source_id)
    # A ref anchors a figure in the source *chapter*; a review page is not in
    # the book at all, so carrying it over would only collide with the host's
    # own refs (they are unique per lecture).
    out.pop("ref", None)
    if kind == "row":
        images = out.get("images")
        if isinstance(images, list):
            out["images"] = [_srcs(image, source_id) for image in images]
    if kind == "cover":
        out["logo"] = _logo(out.get("logo"), source_id)
    if kind == "sidenote":
        out["logo"] = _prefix(out.get("logo"), source_id)
    return out


def _srcs(content: dict | None, source_id: str) -> dict | None:
    """A copy of `content` with its `src` re-rooted, if it has one."""
    if not isinstance(content, dict):
        return content
    out = dict(content)
    if "src" in out:
        out["src"] = _prefix(out["src"], source_id)
    return out


def _logo(logo: object, source_id: str) -> object:
    if isinstance(logo, dict):
        return {slot: _prefix(value, source_id) for slot, value in logo.items()}
    return _prefix(logo, source_id)


def _prefix(src: object, source_id: str) -> object:
    if isinstance(src, str) and src.startswith(_ASSET_PREFIX):
        return f"{_ASSET_PREFIX}{source_id}/{src[len(_ASSET_PREFIX):]}"
    return src
