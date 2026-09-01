"""Per-lecture language overlays: `i18n/<lang>.toml` substituted into the AST.

The Python source (`lecture.py` and its modules) is the **baseline** text, in
whichever language the author wrote it; nothing declares that language. An
overlay is a map from stable keys to replacement strings, applied **once, on the
model** — after `cli.load_lecture`, before any renderer — so viewer/Marp,
PDF/PNG, PPTX, transcript and book never learn that languages exist.

Keys are derived positionally (`<page id>.<kind>.<n>`, sub-strings hanging off
that: `intro.slide.2.footnote.1`), or pinned by the author with
`p.slide(..., key="why")` → `intro.why`. A pinned block does not consume a
number in its kind's sequence, so pinning one block never renumbers its
neighbours.

One walker serves all three commands. `transform` rewrites a lecture through a
`lookup(scope, key, text)` callback that answers with a replacement or `None`
for "no entry"; `apply` passes a lookup that substitutes, `collect` one that
records the key and always answers `None`. They therefore cannot disagree about
what a key is.

A missing entry falls back to the baseline text — never dropped — and is
recorded in `Lecture.untranslated` (and washed on the slide by the viewer).
An entry whose baseline has since changed is **still used**: the author polishes
wording daily, and silently reverting a translation to Chinese mid-lecture is
worse than a slightly stale English line. `check` is what reports it.
"""

from __future__ import annotations

import hashlib
import tomllib
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Any, Callable

from . import marks, model
from .autobold import autobold

# The directory an overlay lives in, under the lecture (or book) directory.
OVERLAY_DIR = "i18n"

# Content keys that are never author prose — a path, a URL, an enum. The walker
# is driven by the per-kind table below rather than by a generic scan, so this
# is only a second line of defence for the dict-shaped kinds.
_SKIP_VALUES = frozenset({model.ARCH_ELLIPSIS})

# lecturekit's own chrome, per language. Not part of an overlay: these strings
# belong to the framework, not to a lecture. An unknown language falls back to
# the Chinese table (today's behaviour) rather than blocking a render.
UI_STRINGS: dict[str, dict[str, str]] = {
    "zh": {
        "references": "参考文献",
        "outline": "大纲",
        "further_reading": "延伸阅读",
        "demo": "动手试试",
        "todo": "TODO",
        "figure": "图",
        "page": "P",
    },
    "en": {
        "references": "References",
        "outline": "Outline",
        "further_reading": "Further reading",
        "demo": "Try it",
        "todo": "TODO",
        "figure": "Figure",
        "page": "P",
    },
}
DEFAULT_UI_LANG = "zh"


def ui(lang: str | None, name: str) -> str:
    """One of lecturekit's own fixed strings, in ``lang`` where we have it."""
    table = UI_STRINGS.get(lang or DEFAULT_UI_LANG) or UI_STRINGS[DEFAULT_UI_LANG]
    return table.get(name, UI_STRINGS[DEFAULT_UI_LANG][name])


def ui_table(lang: str | None) -> dict[str, str]:
    """The whole fixed-string table for ``lang`` (the viewer ships it to JS)."""
    return {name: ui(lang, name) for name in UI_STRINGS[DEFAULT_UI_LANG]}


# --------------------------------------------------------------------------
# What a block's translatable strings are, and what they are called
# --------------------------------------------------------------------------

def content_slots(kind: str, content: Any) -> list[tuple[str, tuple]]:
    """``(sub-key, path)`` for every translatable string in a block's content.

    ``path`` indexes into ``content`` (dict keys and list indices), so one table
    serves both reading and rewriting. A sub-key of ``""`` means the block key
    itself — a `slide`'s text is the block, not a field of it.
    """
    if kind in ("slide", "notes", "prose", "aside"):
        # Content is the string itself, so the block key names it directly.
        return [("", ())] if isinstance(content, str) and content.strip() else []
    if not isinstance(content, dict):
        return []
    slots: list[tuple[str, tuple]] = []

    def add(sub: str, path: tuple) -> None:
        value = _at(content, path)
        if isinstance(value, str) and value.strip() and value not in _SKIP_VALUES:
            slots.append((sub, path))

    if kind in ("highlight", "bridge"):
        add("", ("text",))
    elif kind == "sidenote":
        add("title", ("title",))
        add("text", ("text",))
    elif kind == "link":
        add("label", ("label",))
    elif kind == "demo":
        add("name", ("name",))
        add("description", ("description",))
    elif kind == "cover":
        add("author", ("author",))
        add("time", ("time",))
    elif kind in ("image", "side_image"):
        add("caption", ("caption",))
        add("alt", ("alt",))
    elif kind == "row":
        add("caption", ("caption",))
        for i, _ in enumerate(content.get("items") or [], start=1):
            add(f"item.{i}.caption", ("items", i - 1, "caption"))
            add(f"item.{i}.alt", ("items", i - 1, "alt"))
    elif kind == "architecture":
        add("caption", ("caption",))
        for i, layer in enumerate(content.get("layers") or [], start=1):
            add(f"layer.{i}.title", ("layers", i - 1, "title"))
            for j, _ in enumerate(layer.get("modules") or [], start=1):
                add(f"layer.{i}.module.{j}", ("layers", i - 1, "modules", j - 1))
    elif kind == "table":
        # The header is row 0, so a cell's key reads as its position in the
        # table as printed.
        for c, _ in enumerate(content.get("headers") or []):
            add(f"cell.0.{c}", ("headers", c))
        for r, row in enumerate(content.get("rows") or [], start=1):
            for c, _ in enumerate(row):
                add(f"cell.{r}.{c}", ("rows", r - 1, c))
    return slots


def rewrite_replacement(
    kind: str, sub: str, text: str, *, autobold_lines: bool = True
) -> str:
    """A replacement string, put through its slot's own authoring rules.

    Slide text is the one slot with any: `p.slide(...)` expands the `==mark==`
    shorthand and bolds flush-left prose lines, and a translator writing that
    slot is writing slide text — having to hand-write `**…**` on every headline
    would be a trap the DSL exists to remove. Both rewrites are idempotent on
    text already carrying them, so a translation copied from the `# src:`
    comment (which quotes the baseline *after* the same pass) comes out
    unchanged.

    `autobold_lines` is the block's own `p.slide(..., autobold=...)` choice: a
    block the author kept unbolded must not come back bolded in another
    language.
    """
    if kind == "slide" and sub == "":
        text = marks.expand(text)
        return autobold(text) if autobold_lines else text
    return text


def _at(container: Any, path: tuple) -> Any:
    for step in path:
        if isinstance(container, dict):
            container = container.get(step)
        elif isinstance(container, (list, tuple)) and isinstance(step, int):
            if step >= len(container):
                return None
            container = container[step]
        else:
            return None
    return container


def _set_at(container: Any, path: tuple, value: str) -> Any:
    """A copy of ``container`` with ``path`` replaced — nothing is mutated."""
    step, rest = path[0], path[1:]
    if isinstance(container, dict):
        out = dict(container)
        out[step] = value if not rest else _set_at(out.get(step), rest, value)
        return out
    out = list(container)
    out[step] = value if not rest else _set_at(out[step], rest, value)
    return out


class _Counter:
    """Per-page auto numbering: `<kind>.<n>`, counted per kind, in author order.

    A pinned block is skipped entirely, so it does not consume a number — which
    is what keeps pinning one block from renumbering its neighbours.
    """

    def __init__(self) -> None:
        self._counts: dict[str, int] = {}

    def next(self, kind: str) -> str:
        self._counts[kind] = self._counts.get(kind, 0) + 1
        return f"{kind}.{self._counts[kind]}"


def block_key(page_key: str, block: model.Block, counter: _Counter) -> str:
    """The key naming ``block`` — the author's pin, or its positional number."""
    local = block.key if block.key else counter.next(block.kind)
    return f"{page_key}.{local}"


# --------------------------------------------------------------------------
# The walker
# --------------------------------------------------------------------------

# A lookup answers "what does this key say?", given the baseline text. It
# returns the replacement, or **None** for "no entry" — the one signal
# `transform` needs to fall back to the baseline and mark the block. `scope` is
# None for the lecture's own pages and the source lecture's id for a borrowed
# review page, whose overlay lives with the source (as its assets do).
Lookup = Callable[[str | None, str, str], str | None]


class _Fallbacks:
    """Resolves one lookup, falling back to the baseline and remembering that.

    The count is what a block reports as ``Block.untranslated``; for a title,
    where there is no block to mark, only the lookup's own record matters.
    """

    def __init__(self) -> None:
        self.count = 0

    def get(self, lookup: Lookup, scope: str | None, key: str, text: str) -> str:
        out = lookup(scope, key, text)
        if out is None:
            self.count += 1
            return text
        return out


def transform(lecture: model.Lecture, lookup: Lookup) -> model.Lecture:
    """Rewrite every translatable string in ``lecture`` through ``lookup``."""
    misses = _Fallbacks()
    title = misses.get(lookup, None, "lecture.title", lecture.title)
    subtitle = (
        misses.get(lookup, None, "lecture.subtitle", lecture.subtitle)
        if lecture.subtitle else lecture.subtitle
    )
    children = [_node(child, lecture.borrowed, lookup) for child in lecture.children]
    return replace(lecture, title=title, subtitle=subtitle, children=children)


def _node(
    node: model.Section | model.Page,
    borrowed: tuple[model.Borrowed, ...],
    lookup: Lookup,
) -> model.Section | model.Page:
    if isinstance(node, model.Section):
        misses = _Fallbacks()
        return replace(
            node,
            title=misses.get(lookup, None, f"section.{node.id}.title", node.title),
            children=[_node(child, borrowed, lookup) for child in node.children],
        )
    return _page(node, borrowed, lookup)


def page_scope(
    page: model.Page, borrowed: tuple[model.Borrowed, ...]
) -> tuple[str | None, str]:
    """``(source lecture id | None, the page key)`` for one page.

    A borrowed review page was re-branded ``<source id>/<page id>``; its overlay
    lives in the **source** lecture, under the unprefixed id — the same rule its
    assets follow. An animation's frames are one slide, so every frame keys off
    the group id the author wrote, not its ``-1``/``-2`` frame id.
    """
    page_id = page.id
    if page.frame_group is not None:
        page_id = page.frame_group.id
    for entry in borrowed:
        prefix = f"{entry.lecture_id}/"
        if page_id.startswith(prefix):
            return entry.lecture_id, page_id[len(prefix):]
    return None, page_id


def _page(
    page: model.Page,
    borrowed: tuple[model.Borrowed, ...],
    lookup: Lookup,
) -> model.Page:
    scope, key = page_scope(page, borrowed)
    misses = _Fallbacks()
    title = misses.get(lookup, scope, f"{key}.title", page.title)
    book_title = (
        misses.get(lookup, scope, f"{key}.book_title", page.book_title)
        if page.book_title else page.book_title
    )
    counter = _Counter()
    blocks = [_block(block, key, scope, counter, lookup) for block in page.blocks]
    return replace(page, title=title, book_title=book_title, blocks=blocks)


def _block(
    block: model.Block,
    page_key: str,
    scope: str | None,
    counter: _Counter,
    lookup: Lookup,
) -> model.Block:
    key = block_key(page_key, block, counter)
    misses = _Fallbacks()
    content = block.content
    for sub, path in content_slots(block.kind, content):
        full = f"{key}.{sub}" if sub else key
        baseline = _at(content, path) if path else content
        translated = misses.get(lookup, scope, full, baseline)
        if translated != baseline:
            translated = rewrite_replacement(
                block.kind, sub, translated, autobold_lines=block.autobold
            )
            content = _set_at(content, path, translated) if path else translated
    footnotes = tuple(
        misses.get(lookup, scope, f"{key}.footnote.{i}", text)
        for i, text in enumerate(block.footnotes, start=1)
    )
    annotations = tuple(
        replace(note, text=misses.get(
            lookup, scope, f"{key}.annotation.{i}", note.text
        ))
        for i, note in enumerate(block.annotations, start=1)
    )
    return replace(
        block,
        content=content,
        footnotes=footnotes,
        annotations=annotations,
        untranslated=bool(misses.count) or block.untranslated,
    )


@dataclass(frozen=True)
class Entry:
    """One translatable string of a lecture, as `collect` found it."""
    key: str
    text: str


def collect(lecture: model.Lecture) -> list[Entry]:
    """Every translatable string of ``lecture``'s **own** pages, in walk order.

    Borrowed review pages are skipped: their text belongs to the source
    lecture's overlay, and duplicating it here would give one string two
    translations. An animation's frames all produce the same keys (same blocks,
    only the figure `src` differs), so they collapse to one entry each.
    """
    found: dict[str, str] = {}
    order: list[str] = []

    def lookup(scope: str | None, key: str, text: str) -> str | None:
        if scope is None and key not in found:
            found[key] = text
            order.append(key)
        return None

    transform(lecture, lookup)
    return [Entry(key, found[key]) for key in order]


# --------------------------------------------------------------------------
# Overlay files
# --------------------------------------------------------------------------

@dataclass(frozen=True)
class OverlayEntry:
    src_hash: str
    text: str


Overlay = dict[str, OverlayEntry]


def source_hash(text: str) -> str:
    """The baseline fingerprint an overlay entry records: 8 hex of sha256."""
    return hashlib.sha256(normalize(text).encode("utf-8")).hexdigest()[:8]


def normalize(text: str) -> str:
    """One normalization, used for both hashing and comparing overlay text.

    TOML drops the newline right after a ``'''`` opener but keeps the one before
    the closer, so a round-tripped string differs from the baseline by trailing
    newlines and nothing else.
    """
    return text.rstrip("\n")


def overlay_path(directory: Path, lang: str) -> Path:
    return Path(directory, OVERLAY_DIR, f"{lang}.toml")


def load_overlay(directory: Path, lang: str) -> Overlay:
    """Read ``<directory>/i18n/<lang>.toml``.

    A missing file is an error rather than an empty overlay: `--lang em` is a
    typo, and silently rendering the baseline hides it until the projector.
    """
    path = overlay_path(directory, lang)
    if not path.exists():
        raise model.ValidationError(
            f"no {lang} overlay: {path} (run `lecturekit i18n extract "
            f"{directory} --lang {lang}` to start one)"
        )
    return parse_overlay(path.read_text(encoding="utf-8"), path)


def parse_overlay(text: str, path: Path | None = None) -> Overlay:
    try:
        data = tomllib.loads(text)
    except tomllib.TOMLDecodeError as exc:
        where = f" in {path}" if path is not None else ""
        raise model.ValidationError(f"cannot parse overlay{where}: {exc}") from exc
    overlay: Overlay = {}
    for key, entry in data.items():
        if not isinstance(entry, dict):
            raise model.ValidationError(
                f"overlay entry {key!r} must be a table, got {type(entry).__name__}"
            )
        overlay[key] = OverlayEntry(
            src_hash=str(entry.get("src_hash") or ""),
            text=normalize(str(entry.get("text") or "")),
        )
    return overlay


def try_load_overlay(directory: Path, lang: str) -> Overlay:
    """``load_overlay``, but an absent file is an empty overlay.

    Used for a borrowed review source, which need not be translated just because
    the lecture borrowing from it is — the pages fall back and are marked.
    """
    path = overlay_path(directory, lang)
    if not path.exists():
        return {}
    return parse_overlay(path.read_text(encoding="utf-8"), path)


# --------------------------------------------------------------------------
# apply / check
# --------------------------------------------------------------------------

def apply(
    lecture: model.Lecture,
    directory: Path,
    lang: str,
    *,
    strict: bool = False,
) -> model.Lecture:
    """``lecture`` with its ``lang`` overlay substituted in.

    Borrowed review pages are looked up in their own source lecture's overlay,
    loaded lazily. Keys with no entry (or an empty one) keep the baseline text
    and are collected into ``Lecture.untranslated``; ``strict`` turns that into
    a refusal to render.
    """
    overlay = load_overlay(Path(directory), lang)
    sources = {entry.lecture_id: Path(entry.directory) for entry in lecture.borrowed}
    cache: dict[str, Overlay] = {}
    missing: list[str] = []
    seen: set[str] = set()

    def lookup(scope: str | None, key: str, text: str) -> str | None:
        table = overlay
        reported = key
        if scope is not None:
            if scope not in cache:
                cache[scope] = try_load_overlay(sources[scope], lang)
            table = cache[scope]
            # Reported with the source prefix the deck knows the page by, so a
            # `--strict` listing says which lecture to go and translate.
            reported = f"{scope}/{key}"
        entry = table.get(key)
        if entry is None or not entry.text:
            # An animation looks its keys up once per frame; a key is one
            # missing translation however many slides it renders on.
            if reported not in seen:
                seen.add(reported)
                missing.append(reported)
            return None
        return entry.text

    translated = transform(lecture, lookup)
    if strict and missing:
        listed = "\n  ".join(missing)
        raise model.ValidationError(
            f"{len(missing)} untranslated string(s) under --strict "
            f"({lang}):\n  {listed}"
        )
    return replace(translated, lang=lang, untranslated=tuple(missing))


@dataclass(frozen=True)
class Report:
    """What `i18n check` found, one list of keys per problem."""
    missing: tuple[str, ...] = ()
    changed: tuple[str, ...] = ()
    orphaned: tuple[str, ...] = ()

    @property
    def clean(self) -> bool:
        return not (self.missing or self.changed or self.orphaned)


def check(lecture: model.Lecture, overlay: Overlay) -> Report:
    """Compare a lecture's baseline strings against an overlay."""
    return check_entries(collect(lecture), overlay)


def check_entries(entries: list[Entry], overlay: Overlay) -> Report:
    """The same comparison, for strings already collected (a book's, say)."""
    known = {entry.key for entry in entries}
    missing: list[str] = []
    changed: list[str] = []
    for entry in entries:
        existing = overlay.get(entry.key)
        if existing is None or not existing.text:
            missing.append(entry.key)
        elif existing.src_hash and existing.src_hash != source_hash(entry.text):
            changed.append(entry.key)
    orphaned = [key for key in overlay if key not in known]
    return Report(tuple(missing), tuple(changed), tuple(orphaned))


def format_report(report: Report, lang: str) -> str:
    """One line per key, grouped — nothing when everything is in order."""
    lines: list[str] = []
    for label, keys in (
        ("missing", report.missing),
        ("changed", report.changed),
        ("orphaned", report.orphaned),
    ):
        for key in keys:
            lines.append(f"{label:<9}{key}")
    lines.append(
        f"{lang}: {len(report.missing)} missing, {len(report.changed)} changed, "
        f"{len(report.orphaned)} orphaned"
    )
    return "\n".join(lines)


# --------------------------------------------------------------------------
# The book's own front matter
# --------------------------------------------------------------------------

# A book is an *ordering*, not a document: the only text it owns is its front
# matter, so its overlay is three keys. Every chapter keeps translating itself
# through its own lecture overlay.
_BOOK_FIELDS = ("title", "subtitle", "preface")


def collect_book(book) -> list[Entry]:
    """The book's own translatable strings (``book.title`` …), in field order."""
    return [
        Entry(f"book.{name}", getattr(book, name))
        for name in _BOOK_FIELDS
        if getattr(book, name)
    ]


def apply_book(book, directory: Path, lang: str, *, strict: bool = False):
    """``book`` with its front matter overlaid and every chapter translated.

    A chapter's text lives with its lecture, so each is looked up in its own
    ``i18n/<lang>.toml`` — under ``book.asset_roots``, which already records
    where each lecture was loaded from.
    """
    overlay = try_load_overlay(Path(directory), lang)
    fields: dict[str, str] = {}
    missing: list[str] = []
    for name in _BOOK_FIELDS:
        baseline = getattr(book, name)
        if not baseline:
            continue
        entry = overlay.get(f"book.{name}")
        if entry is None or not entry.text:
            missing.append(f"book.{name}")
        else:
            fields[name] = entry.text
    lectures = tuple(
        apply(lecture, book.asset_roots[lecture.id], lang, strict=strict)
        for lecture in book.lectures
    )
    if strict and missing:
        listed = "\n  ".join(missing)
        raise model.ValidationError(
            f"{len(missing)} untranslated string(s) under --strict ({lang}):"
            f"\n  {listed}"
        )
    return replace(book, lectures=lectures, lang=lang, **fields)


# --------------------------------------------------------------------------
# extract: the overlay skeleton, merged with what is already translated
# --------------------------------------------------------------------------

def extract(entries: list[Entry], existing: Overlay) -> str:
    """The TOML text of an overlay for ``entries``, keeping existing translations.

    Merge rules, all of them conservative — a translator's work is never thrown
    away by a tool run:

    - an existing translation is kept verbatim;
    - a new key arrives with ``text = ''`` (grep for it to find what is left);
    - a key whose baseline changed keeps its text, takes the new hash, and is
      flagged with a ``# CHANGED`` comment;
    - a key the lecture no longer has is moved to an ``# orphaned`` section at
      the end rather than deleted.
    """
    out: list[str] = [
        "# Generated by `lecturekit i18n extract`; edit the `text` fields.",
        "# `src_hash` fingerprints the baseline a `text` was translated from, and",
        "# the `# src:` comment under each entry quotes it. Both are rewritten on",
        "# every extract; an empty `text` means untranslated.",
        "",
    ]
    known: set[str] = set()
    for entry in entries:
        known.add(entry.key)
        previous = existing.get(entry.key)
        digest = source_hash(entry.text)
        note = None
        if previous is not None and previous.src_hash and previous.src_hash != digest:
            note = f"# CHANGED: was {previous.src_hash}"
        out.extend(_emit_entry(
            entry.key, digest, previous.text if previous else "", entry.text, note
        ))
    orphans = [key for key in existing if key not in known]
    if orphans:
        out.extend([
            "# orphaned — no longer in the lecture. Kept so a rename does not",
            "# throw the translation away; delete by hand once you are sure.",
            "",
        ])
        for key in orphans:
            previous = existing[key]
            out.extend(_emit_entry(key, previous.src_hash, previous.text, None, None))
    return "\n".join(out)


def _emit_entry(
    key: str,
    src_hash: str,
    text: str,
    baseline: str | None,
    note: str | None,
) -> list[str]:
    lines = [f"[{toml_string(key)}]"]
    if note:
        lines.append(note)
    if src_hash:
        lines.append(f"src_hash = {toml_string(src_hash)}")
    lines.append(f"text = {toml_string(text)}")
    if baseline is not None:
        lines.extend(_src_comment(baseline))
    lines.append("")
    return lines


def _src_comment(baseline: str) -> list[str]:
    lines = normalize(baseline).split("\n")
    if len(lines) == 1:
        return [f"# src: {lines[0]}"]
    return ["# src:"] + [f"# {line}" for line in lines]


def toml_string(text: str) -> str:
    """``text`` as a TOML string, preserving it verbatim wherever possible.

    A literal string (single quotes) escapes nothing, which is what keeps
    ``==mark==``, ``$$``, a leading space and a backslash surviving the round
    trip. The multi-line form is used whenever the text has a newline; only a
    body containing ``'''`` forces the escaping basic-string form.
    """
    if "\n" not in text and "\r" not in text:
        # One line: a literal string, unless the text itself carries a quote.
        return f"'{text}'" if "'" not in text else _basic_string(text)
    if "'''" not in text and "\r" not in text:
        # TOML drops the newline directly after the opener, so the text starts
        # on its own line; `normalize` undoes the one before the closer.
        return f"'''\n{text}\n'''"
    return _basic_string(text)


def _basic_string(text: str) -> str:
    """The escaping form, for text no literal string can hold verbatim."""
    escaped = (
        text.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )
    return f'"{escaped}"'
