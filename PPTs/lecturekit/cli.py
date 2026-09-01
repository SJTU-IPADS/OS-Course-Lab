from __future__ import annotations

import argparse
import importlib.util
import json
import os
import platform
import subprocess
import sys
import webbrowser
from pathlib import Path
from types import ModuleType

from .demo import DEFAULT_TIMEOUT_S as DEFAULT_DEMO_TIMEOUT_S
from .dev_server import DEFAULT_DEBOUNCE_MS, serve
from .dsl import Lecture as LectureBuilder, slugify
from .model import (
    Lecture,
    Page,
    Section,
    is_bridge_page,
    outline_folds,
    select_pages,
)
from .renderers import get_renderer
from .renderers.viewer.marp import build_deck
from .serialize import lecture_to_dict


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        if args.command == "inspect":
            print(format_tree(load_lecture_for(args)))
            return 0
        if args.command == "build":
            lecture = load_lecture_for(args)
            print(json.dumps(lecture_to_dict(lecture), ensure_ascii=False, indent=2))
            return 0
        if args.command == "i18n":
            return run_i18n(args)
        if args.command == "render":
            lecture_dir = Path(args.lecture_dir)
            lecture = load_lecture_for(args)
            if args.pages:
                lecture = select_pages(lecture, args.pages)
            output_dir = Path(args.out) if args.out else default_out_dir(lecture, args.to, args.lang)
            entry = render_lecture(lecture, lecture_dir, args.to, output_dir)
            # Only the viewer target needs the Marp build; pptx writes its deck
            # directly (no Marp/Chrome).
            if args.to == "viewer" and not args.no_build:
                build_deck(output_dir, deck_formats(args.pdf, args.png), name=slugify(lecture.title))
            if args.to == "transcript" and args.pdf:
                # The sheet is already a paged A4 document, so its PDF is one
                # headless-Chrome print of the file itself.
                from .renderers.transcript.pdf import print_pdf

                print(print_pdf(entry))
            print(entry)
            return 0
        if args.command == "view":
            lecture_dir = Path(args.lecture_dir)
            if args.watch and args.pages:
                raise ValueError(
                    "--pages cannot be combined with --watch: a watch session "
                    "re-resolves the selection on every save, so renaming or "
                    "deleting the selected page would wedge it. Watch the whole "
                    "lecture -- the viewer keeps you on the page you are editing."
                )
            if args.demo and not args.watch:
                raise ValueError(
                    "--demo needs --watch: running a demo takes a live server, "
                    "and a plain `view` only writes files and opens them."
                )
            lecture = load_lecture_for(args)
            if args.pages:
                lecture = select_pages(lecture, args.pages)
            output_dir = Path(args.out) if args.out else default_out_dir(lecture, "viewer", args.lang)
            if args.watch:
                serve(
                    lecture_dir, output_dir,
                    port=args.port, reveal=not args.no_reveal,
                    debounce_ms=args.debounce,
                    lang=args.lang, strict=args.strict,
                    demo=args.demo, demo_timeout_s=args.demo_timeout,
                )
                return 0
            entry = render_lecture(lecture, lecture_dir, "viewer", output_dir)
            if not args.no_build:
                build_deck(output_dir, deck_formats(args.pdf, args.png), name=slugify(lecture.title))
            open_in_browser(entry)
            return 0
        if args.command == "book":
            return render_book(args)
    except Exception as exc:
        print(f"lecturekit: {exc}", file=sys.stderr)
        return 1

    parser.print_help()
    return 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="lecture")
    subparsers = parser.add_subparsers(dest="command", required=True)

    inspect_parser = subparsers.add_parser("inspect", help="print and validate lecture tree")
    inspect_parser.add_argument("lecture_dir")
    add_lang_flag(inspect_parser)

    build_parser_ = subparsers.add_parser("build", help="emit the lecture AST as JSON to stdout")
    build_parser_.add_argument("lecture_dir")
    add_lang_flag(build_parser_)

    render_parser = subparsers.add_parser("render", help="render a complete viewer bundle")
    render_parser.add_argument("lecture_dir")
    add_lang_flag(render_parser, strict=True)
    render_parser.add_argument(
        "--to", default="viewer", choices=["viewer", "pptx", "transcript"]
    )
    render_parser.add_argument("--out")
    render_parser.add_argument(
        "--pages",
        help="render only a subset, e.g. '3-7', '2,4,6', or a page id",
    )
    render_parser.add_argument(
        "--no-build",
        action="store_true",
        help="skip running Marp to build slides.html",
    )
    render_parser.add_argument(
        "--pdf",
        action="store_true",
        help=(
            "also export a PDF: the deck for --to viewer, the sheet itself for "
            "--to transcript (needs local Chrome/Chromium)"
        ),
    )
    render_parser.add_argument(
        "--png",
        action="store_true",
        help="also export one PNG per page, slides.NNN.png (needs local Chrome/Chromium)",
    )

    view_parser = subparsers.add_parser("view", help="render, build the deck, and open a preview")
    view_parser.add_argument("lecture_dir")
    add_lang_flag(view_parser, strict=True)
    view_parser.add_argument("--out")
    view_parser.add_argument("--port", type=int, default=3030)
    view_parser.add_argument(
        "--pages",
        help="view only a subset, e.g. '3-7', '2,4,6', or a page id (not with --watch)",
    )
    view_parser.add_argument(
        "--watch",
        action="store_true",
        help="live-reload: re-render and refresh the browser on save",
    )
    view_parser.add_argument(
        "--no-reveal",
        action="store_true",
        help="with --watch, disable reveal-on-Enter (plain live preview)",
    )
    view_parser.add_argument(
        "--debounce",
        type=int,
        default=DEFAULT_DEBOUNCE_MS,
        metavar="MS",
        help=(
            "with --watch, coalesce a burst of file changes into one render, "
            f"firing MS ms after the last change (default {DEFAULT_DEBOUNCE_MS}); "
            "raise it for bulk/automated edits, lower it (e.g. 50) for snappier "
            "single-file reloads"
        ),
    )
    view_parser.add_argument(
        "--demo",
        action="store_true",
        help=(
            "with --watch, arm the lecture's p.demo(...) buttons: a press runs "
            "that command in the lecture directory and streams its output"
        ),
    )
    view_parser.add_argument(
        "--demo-timeout",
        type=float,
        default=DEFAULT_DEMO_TIMEOUT_S,
        metavar="S",
        help=(
            "with --demo, kill a demo that runs longer than S seconds; 0 for "
            f"no limit, and a block's own timeout= wins over this "
            f"(default {DEFAULT_DEMO_TIMEOUT_S:g})"
        ),
    )
    view_parser.add_argument(
        "--no-build",
        action="store_true",
        help="skip running Marp to build slides.html",
    )
    view_parser.add_argument(
        "--pdf",
        action="store_true",
        help="also export the deck as <lecture-title>.pdf (needs local Chrome/Chromium)",
    )
    view_parser.add_argument(
        "--png",
        action="store_true",
        help="also export one PNG per page, slides.NNN.png (needs local Chrome/Chromium)",
    )

    book_parser = subparsers.add_parser(
        "book", help="render a book.py of lectures into a LaTeX tree"
    )
    book_parser.add_argument("book_dir")
    add_lang_flag(book_parser, strict=True)
    book_parser.add_argument("--out")
    book_parser.add_argument(
        "--lectures",
        help="render only these lecture ids, comma-separated (e.g. 'lec02')",
    )
    book_parser.add_argument(
        "--stats",
        action="store_true",
        help="print prose coverage per lecture and exit without rendering",
    )
    book_parser.add_argument(
        "--compile",
        action="store_true",
        help="also run latexmk to build book.pdf (needs XeLaTeX)",
    )

    i18n_parser = subparsers.add_parser(
        "i18n", help="manage a lecture's translation overlays (i18n/<lang>.toml)"
    )
    i18n_sub = i18n_parser.add_subparsers(dest="i18n_command", required=True)

    extract_parser = i18n_sub.add_parser(
        "extract",
        help="write/refresh i18n/<lang>.toml from the lecture's baseline text",
    )
    extract_parser.add_argument("source_dir", help="a lecture dir, or a dir with book.py")
    extract_parser.add_argument("--lang", required=True)

    check_parser = i18n_sub.add_parser(
        "check", help="report overlay entries that are missing, changed, or orphaned"
    )
    check_parser.add_argument("source_dir", help="a lecture dir, or a dir with book.py")
    check_parser.add_argument("--lang", required=True)
    check_parser.add_argument(
        "--allow-changed",
        action="store_true",
        help="exit 0 when the only problem is a baseline that has since changed",
    )
    return parser


def add_lang_flag(parser: argparse.ArgumentParser, *, strict: bool = False) -> None:
    """``--lang`` (and optionally ``--strict``) on a command that loads a lecture."""
    parser.add_argument(
        "--lang",
        help=(
            "apply the i18n/<lang>.toml translation overlay; omit for the "
            "baseline text as written in Python"
        ),
    )
    if strict:
        parser.add_argument(
            "--strict",
            action="store_true",
            help="with --lang, refuse to render when any string is untranslated",
        )


def deck_formats(pdf: bool, png: bool = False) -> tuple[str, ...]:
    """Marp output formats for a build; html always (the viewer iframes it)."""
    formats = ["html"]
    if pdf:
        formats.append("pdf")
    if png:
        formats.append("png")
    return tuple(formats)


def render_lecture(lecture: Lecture, lecture_dir: Path, to: str, output_dir: Path) -> Path:
    """Render a lecture bundle to ``output_dir`` and return the entry file to open.

    A renderer may return its primary output path (the pptx renderer returns the
    ``.pptx``); the viewer renderer returns ``None``, so its entry is ``index.html``.
    """
    renderer_cls = get_renderer(to)
    entry = renderer_cls(asset_root=lecture_dir).render(lecture, output_dir)
    return entry or Path(output_dir, "index.html")


def render_book(args) -> int:
    """The `book` command: a book.py of lectures → a LaTeX tree (optionally compiled)."""
    from .book import load_book
    from .renderers.latex import LatexRenderer, coverage

    book = load_book(Path(args.book_dir))
    if args.lectures:
        book = select_lectures(book, args.lectures)
    if args.lang:
        from . import i18n

        # After the selection, so a one-chapter draft build only loads that
        # chapter's overlay.
        book = i18n.apply_book(book, Path(args.book_dir), args.lang, strict=args.strict)

    if args.stats:
        print(format_coverage(coverage(book)))
        return 0

    output_dir = Path(args.out) if args.out else Path(
        "build", "book" if not args.lang else f"book-{args.lang}"
    )
    entry = LatexRenderer().render(book, output_dir)
    print(entry)

    if args.compile:
        subprocess.run(
            ["latexmk", "-xelatex", "-interaction=nonstopmode", "book.tex"],
            cwd=output_dir,
            check=True,
        )
    return 0


def select_lectures(book, spec: str):
    """A copy of ``book`` holding only the lectures named by ``spec`` (ids, comma-separated)."""
    from dataclasses import replace

    from .model import ValidationError

    wanted = [token.strip() for token in spec.split(",") if token.strip()]
    known = {lecture.id: lecture for lecture in book.lectures}
    for lecture_id in wanted:
        if lecture_id not in known:
            raise ValidationError(
                f"unknown lecture id: {lecture_id!r} (have {', '.join(known)})"
            )
    kept = [lecture for lecture in book.lectures if lecture.id in set(wanted)]
    roots = {lecture.id: book.asset_roots[lecture.id] for lecture in kept}
    return replace(book, lectures=tuple(kept), asset_roots=roots)


def format_coverage(rows: list[tuple[str, int, int]]) -> str:
    """A per-lecture table of how much of the book is written."""
    lines = []
    width = max((len(lecture_id) for lecture_id, _, _ in rows), default=5)
    written_total = pages_total = 0
    for lecture_id, written, pages in rows:
        written_total += written
        pages_total += pages
        lines.append(_coverage_line(lecture_id, written, pages, width))
    lines.append("-" * (width + 22))
    lines.append(_coverage_line("total", written_total, pages_total, width))
    return "\n".join(lines)


def _coverage_line(label: str, written: int, pages: int, width: int) -> str:
    percent = round(100 * written / pages) if pages else 0
    return f"{label:<{width}}  {written}/{pages} pages  {percent:>3}%"


def load_lecture_for(args) -> Lecture:
    """Load the lecture a command names, with its ``--lang`` overlay applied.

    The single seam every command goes through, so an overlay is substituted
    once, on the model, and no renderer ever learns that languages exist.
    """
    lecture_dir = Path(args.lecture_dir)
    lecture = load_lecture(lecture_dir)
    lang = getattr(args, "lang", None)
    if not lang:
        return lecture
    from . import i18n

    return i18n.apply(
        lecture, lecture_dir, lang, strict=getattr(args, "strict", False)
    )


def default_out_dir(lecture: Lecture, to: str, lang: str | None = None) -> Path:
    """``build/<id>-<to>``, with the language in the name when one is applied.

    Two languages of one lecture are two decks; without the name they would
    overwrite each other on every render.
    """
    stem = lecture.id if not lang else f"{lecture.id}-{lang}"
    return Path("build", f"{stem}-{to}")


def run_i18n(args) -> int:
    """The ``i18n`` subcommands: extract an overlay skeleton, or check one."""
    from . import i18n

    source_dir = Path(args.source_dir)
    entries = i18n_entries(source_dir)
    existing = i18n.try_load_overlay(source_dir, args.lang)

    if args.i18n_command == "extract":
        path = i18n.overlay_path(source_dir, args.lang)
        path.parent.mkdir(parents=True, exist_ok=True)
        report = i18n.check_entries(entries, existing)
        path.write_text(i18n.extract(entries, existing), encoding="utf-8")
        print(
            f"{path}: {len(entries)} keys, {len(report.missing)} new/untranslated, "
            f"{len(report.changed)} changed, {len(report.orphaned)} orphaned"
        )
        return 0

    report = i18n.check_entries(entries, existing)
    print(i18n.format_report(report, args.lang))
    if report.missing:
        return 1
    if report.changed and not args.allow_changed:
        return 1
    return 0


def i18n_entries(source_dir: Path):
    """The translatable strings of a lecture directory — or of a book's own text."""
    from . import i18n

    if Path(source_dir, "book.py").exists():
        from .book import load_book

        return i18n.collect_book(load_book(source_dir))
    return i18n.collect(load_lecture(source_dir))


def load_lecture(lecture_dir: Path) -> Lecture:
    lecture_file = lecture_dir / "lecture.py"
    if not lecture_file.exists():
        raise FileNotFoundError(f"missing lecture source: {lecture_file}")

    module = load_module(lecture_file)
    if not hasattr(module, "lecture"):
        raise ValueError(f"{lecture_file} must define a variable named 'lecture'")

    lecture = module.lecture
    if isinstance(lecture, Lecture):
        return lecture
    if isinstance(lecture, LectureBuilder):
        return lecture.build()
    if hasattr(lecture, "build"):
        return lecture.build()
    raise TypeError("'lecture' must be a lecturekit Lecture builder or model")


def load_module(path: Path) -> ModuleType:
    """Execute ``path`` as a throwaway module, always from the source on disk.

    A lecture directory is renderer input, reloaded on every keystroke under
    ``--watch``, not a library worth caching. CPython validates a
    ``__pycache__`` entry against the source's ``(mtime, size)`` at one-second
    resolution, and writes one even when the module body raises -- so an edit
    that keeps the byte count and lands in the same second as the last compile
    replays the previous bytecode, repeating an error the author has already
    fixed. We compile the source ourselves, and switch bytecode writing off
    while the module runs so the sibling files it imports never grow a cache to
    go stale either.

    Sibling modules are also *isolated*: they are imported under bare names
    (``import pages``), so with two lectures loaded in one process -- a book
    chapter after another, or a review source loaded from inside its host --
    whichever got there first would answer for every later ``import pages``.
    Each load hides the sibling modules of the loads around it and takes its own
    back out of ``sys.modules`` on the way out, so a name always resolves to a
    file in the lecture doing the importing.
    """
    lecture_dir = path.parent.resolve()
    _LECTURE_DIRS.add(lecture_dir)
    module_name = f"_lecturekit_{path.parent.name}_{path.stem}"
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot import {path}")

    module = importlib.util.module_from_spec(spec)
    before = set(sys.modules)
    stashed = {
        name: sys.modules.pop(name)
        for name, cached in list(sys.modules.items())
        if _sibling_of_another_lecture(cached, lecture_dir)
    }
    sys.modules[module_name] = module
    code = compile(importlib.util.decode_source(path.read_bytes()), str(path), "exec")
    cached_bytecode = sys.dont_write_bytecode
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(lecture_dir))
    try:
        exec(code, module.__dict__)
    finally:
        sys.path.pop(0)
        sys.dont_write_bytecode = cached_bytecode
        for name in set(sys.modules) - before:
            if _module_under(sys.modules.get(name), lecture_dir):
                del sys.modules[name]
        sys.modules.update(stashed)
    return module


# Every lecture directory this process has loaded from. A cached module whose
# file sits directly in one of them is some lecture's sibling, which is exactly
# what a different lecture's load must not see. See load_module.
_LECTURE_DIRS: set[Path] = set()


def _sibling_of_another_lecture(module: ModuleType | None, lecture_dir: Path) -> bool:
    parent = _module_dir(module)
    return parent is not None and parent != lecture_dir and parent in _LECTURE_DIRS


def _module_under(module: ModuleType | None, directory: Path) -> bool:
    resolved = _module_path(module)
    return resolved is not None and directory in resolved.parents


def _module_dir(module: ModuleType | None) -> Path | None:
    resolved = _module_path(module)
    return None if resolved is None else resolved.parent


def _module_path(module: ModuleType | None) -> Path | None:
    filename = getattr(module, "__file__", None)
    if not filename:
        return None
    try:
        return Path(filename).resolve()
    except (OSError, ValueError):
        return None


def format_tree(lecture: Lecture) -> str:
    lines = [f"v {lecture.title} ({lecture.id})"]
    folds = outline_folds(lecture.children)
    for child in lecture.children:
        append_node(lines, child, depth=1, folds=folds)
    return "\n".join(lines)


def append_node(
    lines: list[str],
    node: Section | Page,
    *,
    depth: int,
    folds: frozenset[str] = frozenset(),
) -> None:
    indent = "  " * depth
    if isinstance(node, Page):
        group = node.frame_group
        # Every page keeps its own line here — this is the structural view, and
        # the page a title-run folds into the row above still has its own id for
        # `--pages`. The marker says the outline will print one row for both.
        folded = " [folded]" if node.id in folds else ""
        if is_bridge_page(node):
            lines.append(f"{indent}* {node.title} ({node.id}) [bridge]")
        elif group is None:
            lines.append(f"{indent}* {node.title} ({node.id}){folded}")
        elif group.index == 1:
            # One line per animation, under the id the author wrote — the same
            # id --pages takes for the whole group. Listing every frame would
            # repeat the title N times and say nothing new.
            lines.append(
                f"{indent}* {node.title} ({group.id}) [{group.total} frames]{folded}"
            )
        return
    marker = ">" if node.collapsed else "v"
    lines.append(f"{indent}{marker} {node.title} ({node.id})")
    for child in node.children:
        append_node(lines, child, depth=depth + 1, folds=folds)


def open_in_browser(entry: Path) -> None:
    """Open ``entry`` in the default browser, preferring the OS-native opener.

    ``webbrowser.open`` can silently fail to find a browser (notably on macOS),
    so try the platform opener first and fall back to ``webbrowser``.
    """
    path = str(entry.resolve())
    system = platform.system()
    try:
        if system == "Darwin":
            subprocess.run(["open", path], check=True)
            return
        if system == "Windows":
            os.startfile(path)  # type: ignore[attr-defined]
            return
        if system == "Linux":
            subprocess.run(["xdg-open", path], check=True)
            return
    except (OSError, subprocess.CalledProcessError):
        pass
    webbrowser.open(entry.resolve().as_uri())


if __name__ == "__main__":
    raise SystemExit(main())
