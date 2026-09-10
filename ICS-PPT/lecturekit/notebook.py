from __future__ import annotations

import base64
import hashlib
import html
import json
import mimetypes
import re
import tempfile
from pathlib import Path

from . import dsl, model
from .serialize import lecture_to_dict
from .renderers.viewer import StaticViewerRenderer
from .renderers.viewer.marp import build_deck

# Module-level default asset root, overridable by the notebook author. Used only
# when neither an explicit ``assets=`` nor ``Lecture(assets=...)`` is given.
assets_dir = "."

# A local asset reference as authored (``p.image("assets/foo.png")``) and as it
# survives into the Marp HTML (``src="assets/foo.png"`` / ``url(assets/...)``).
_ASSET_REF = re.compile(r"assets/[\w./\-]+")

# Bump when the render pipeline changes in a way that alters output for the same
# AST (e.g. pagination turned off), so on-disk bundles cached by a prior version
# are invalidated rather than served stale.
_CACHE_VERSION = "2"


def build_one_page_lecture(lecture, page_spec) -> model.Lecture:
    """Project a single ``PageSpec`` into a one-page ``model.Lecture``.

    ``lecture`` is the owning ``dsl.Lecture`` builder; the projection carries its
    ``id``/``title``/``subtitle``/``ratio`` so the page renders exactly as it
    would in the full deck. Reuses ``dsl.build_child`` — the same call the CLI
    makes — so there is no second rendering path to drift. A page carrying
    ``p.frames(...)`` builds into one page per frame, so the cell shows the
    animation's slides and arrows through them.
    """
    one_page_lecture = model.Lecture(
        id=lecture.id,
        title=lecture.title,
        subtitle=lecture.subtitle,
        ratio=lecture.ratio,
        children=dsl.build_child(page_spec),
    )
    model.validate_lecture(one_page_lecture)
    return one_page_lecture


def _data_uri(path: Path) -> str:
    mime = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
    data = base64.b64encode(path.read_bytes()).decode("ascii")
    return f"data:{mime};base64,{data}"


def inline_assets(html: str, root: Path) -> str:
    """Rewrite local ``assets/…`` references in ``html`` to ``data:`` URIs.

    Covers both ``src="assets/…"`` and CSS ``url(assets/…)`` by replacing the
    literal reference token wherever it appears. References whose file is absent
    under ``root`` (and any non-local src like ``https://``) are left untouched,
    so the slide survives inside an ``iframe srcdoc`` with no file server.
    Longest tokens are replaced first so ``assets/a.png`` cannot clobber
    ``assets/a.png2``.
    """
    refs = sorted(set(_ASSET_REF.findall(html)), key=len, reverse=True)
    for ref in refs:
        target = Path(root) / ref
        if target.exists():
            html = html.replace(ref, _data_uri(target))
    return html


def wrap_iframe(html: str, ratio: str) -> str:
    """Wrap self-contained slide HTML in a ratio-sized ``<iframe srcdoc>``.

    Mirrors how the viewer already iframes ``slides.html`` so the slide's card
    framing renders correctly. ``&`` is escaped before ``"`` so the srcdoc
    attribute value is well-formed.
    """
    width, height = model.RATIOS[ratio]
    srcdoc = html.replace("&", "&amp;").replace('"', "&quot;")
    return (
        f'<iframe srcdoc="{srcdoc}" '
        f'style="width:100%; aspect-ratio:{width}/{height}; border:0;" '
        f'loading="lazy"></iframe>'
    )


def cache_key(one_page_lecture: model.Lecture, asset_root: Path) -> str:
    """Content hash of the page's serialized AST + referenced asset mtimes.

    Two identical pages hash the same (cache hit); editing a block, the title,
    the ratio, or a referenced asset file changes the hash (miss).
    """
    blob = json.dumps(
        lecture_to_dict(one_page_lecture), ensure_ascii=False, sort_keys=True
    )
    parts = [f"v{_CACHE_VERSION}", blob]
    for ref in sorted(set(_ASSET_REF.findall(blob))):
        path = Path(asset_root) / ref
        if path.exists():
            parts.append(f"{ref}:{path.stat().st_mtime_ns}")
    return hashlib.sha256("\n".join(parts).encode("utf-8")).hexdigest()


# In-process cache: content hash -> finished iframe HTML. Survives within a
# kernel session so re-running an unchanged cell is instant.
_MEM_CACHE: dict[str, str] = {}

# On-disk cache root: one built bundle per content hash, so an unchanged page
# skips Marp even across kernel restarts.
_CACHE_ROOT = Path(tempfile.gettempdir()) / "lecturekit-notebook"


def _resolve_assets(lecture, assets) -> str:
    """Asset root precedence: explicit arg > ``Lecture(assets=...)`` > module default."""
    if assets is not None:
        return assets
    lecture_assets = getattr(lecture, "assets", None)
    if lecture_assets is not None:
        return lecture_assets
    return assets_dir


def _render_bundle(one_page_lecture: model.Lecture, asset_root: Path, out_dir: Path) -> None:
    """Render the one-page deck into ``out_dir`` via the exact CLI pipeline.

    Pagination is off: a single inline slide has no meaningful page number.
    """
    StaticViewerRenderer(asset_root=asset_root, paginate=False).render(one_page_lecture, out_dir)
    build_deck(out_dir, ("html",))


def render_page_html(lecture, page_spec, *, assets=None, cache=True) -> str:
    """Render one page to a self-contained ``<iframe srcdoc>`` string.

    ``lecture`` is the owning ``dsl.Lecture`` builder. Reuses the deck's Marp
    pipeline so the inline slide is identical to the deck by construction, then
    inlines local assets and wraps the result in a ratio-sized iframe. Caches on
    a content hash (in-process + on-disk); ``cache=False`` forces a fresh render.
    """
    root = Path(_resolve_assets(lecture, assets))
    one_page = build_one_page_lecture(lecture, page_spec)
    key = cache_key(one_page, root)

    if cache and key in _MEM_CACHE:
        return _MEM_CACHE[key]

    out_dir = _CACHE_ROOT / key
    slides_html = out_dir / "slides.html"
    if (not cache) or (not slides_html.exists()):
        out_dir.mkdir(parents=True, exist_ok=True)
        _render_bundle(one_page, root, out_dir)

    html = inline_assets(slides_html.read_text(encoding="utf-8"), out_dir)
    result = wrap_iframe(html, one_page.ratio)
    if cache:
        _MEM_CACHE[key] = result
    return result


def render_news_html(lecture, page_spec) -> str:
    """Render one page's related reading as a compact notebook HTML panel."""
    one_page = build_one_page_lecture(lecture, page_spec)
    page = one_page.children[0]
    if not isinstance(page, model.Page):
        raise model.ValidationError("Notebook news rendering expected a page")
    if not page.news:
        return (
            '<section class="lecturekit-news lecturekit-news--empty">'
            "<h3>Related Reading</h3>"
            "<p>No related reading for this page.</p>"
            "</section>"
        )
    items = "\n".join(_news_item_html(item) for item in page.news)
    return f'<section class="lecturekit-news"><h3>Related Reading</h3><ul>{items}</ul></section>'


def _news_item_html(item: model.NewsItem) -> str:
    title = html.escape(item.title)
    url = html.escape(item.url, quote=True)
    bits = [value for value in (item.source, item.date, item.kind) if value]
    meta = " - ".join(html.escape(bit) for bit in bits)
    meta_html = f'<div class="lecturekit-news-meta">{meta}</div>' if meta else ""
    why_html = (
        f'<p class="lecturekit-news-why">{html.escape(item.why)}</p>'
        if item.why
        else ""
    )
    tags_html = ""
    if item.tags:
        tags = "".join(
            f'<span class="lecturekit-news-tag">{html.escape(tag)}</span>'
            for tag in item.tags
        )
        tags_html = f'<div class="lecturekit-news-tags">{tags}</div>'
    archive_html = ""
    if item.archived_url:
        archive = html.escape(item.archived_url, quote=True)
        archive_html = (
            f' <a class="lecturekit-news-archive" href="{archive}" target="_blank" '
            'rel="noopener noreferrer">Archive</a>'
        )
    image_html = ""
    if item.image:
        image = html.escape(item.image, quote=True)
        image_html = f'<img class="lecturekit-news-image" src="{image}" alt="" />'
    return (
        '<li class="lecturekit-news-item">'
        f"{image_html}"
        f'<a href="{url}" target="_blank" rel="noopener noreferrer">{title}</a>'
        f"{archive_html}{meta_html}{why_html}{tags_html}"
        "</li>"
    )
