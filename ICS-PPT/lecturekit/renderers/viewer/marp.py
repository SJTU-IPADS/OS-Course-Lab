from __future__ import annotations

import os
import re
import shutil
import subprocess
from pathlib import Path

from ... import tokens
from . import pdf

PKG_ROOT = tokens.PKG_ROOT
THEME_DIR = tokens.THEME_DIR
ASSETS_DIR = Path(__file__).resolve().parent / "assets"

# A CJK font bundled into the PDF. Marp renders PDFs with a headless Chromium
# that cannot see the host's system fonts, so Chinese silently drops out unless
# we embed a font via @font-face. The live viewer (a real browser) is unaffected.
CJK_FONT = "NotoSansSC-Regular.woff2"

# The Latin faces the theme @font-face's. They ship with the theme so a deck
# never reaches out to fonts.googleapis.com while rendering (see the comment on
# the @font-face block in themes/basic-office.css).
LATIN_FONTS = (
    "Lato-Regular.woff2",
    "Lato-Italic.woff2",
    "Lato-Bold.woff2",
    "Lato-BoldItalic.woff2",
    "Lato-Black.woff2",
)

# The marp-cli release this renderer is built against. Pinned rather than
# `@latest` because a dist-tag has to be resolved against the npm registry on
# every run: with a network that is up but unreachable, that resolution hangs
# for minutes (npm's own fetch timeout is 5 minutes, times three attempts) and
# nothing ever rebuilds the deck. See `marp_command`.
MARP_VERSION = "4.5.0"
MARP_PACKAGE = f"@marp-team/marp-cli@{MARP_VERSION}"


def marp_command() -> list[str]:
    """The argv prefix that runs marp-cli, preferring anything already local.

    A locally installed marp needs no network at all; `npx` is the last resort
    because it talks to the registry. `scripts/prepare.sh` vendors marp into
    `node_modules/` precisely so the offline path is the one taken.

    Order: ``$LECTUREKIT_MARP``, the vendored ``node_modules/.bin/marp``, a
    ``marp`` on ``PATH``, then ``npx``. The npx fallback pins the version and
    passes ``--prefer-offline`` so a cached copy is used without revalidating
    it against the registry.
    """
    override = os.environ.get("LECTUREKIT_MARP")
    if override:
        return [override]
    vendored = PKG_ROOT / "node_modules" / ".bin" / "marp"
    if vendored.exists():
        return [str(vendored)]
    on_path = shutil.which("marp")
    if on_path:
        return [on_path]
    return ["npx", "--yes", "--prefer-offline", MARP_PACKAGE]


def _copy_theme(output_dir: Path, theme_dir: Path) -> bool:
    theme_src = theme_dir / "basic-office.css"
    if not theme_src.exists():
        return False
    shutil.copyfile(theme_src, output_dir / "theme.css")
    _copy_latin_fonts(output_dir, theme_dir)
    return True


def _copy_latin_fonts(output_dir: Path, theme_dir: Path = THEME_DIR) -> None:
    """Copy the theme's bundled Latin faces next to ``theme.css``.

    Marp inlines the theme into ``slides.html``, so the ``url(...)`` in each
    @font-face resolves against the bundle directory.
    """
    for name in LATIN_FONTS:
        src = theme_dir / "fonts" / name
        if src.exists():
            shutil.copyfile(src, output_dir / name)


def _copy_cjk_font(output_dir: Path, theme_dir: Path = THEME_DIR) -> bool:
    """Copy the bundled CJK font into the bundle; return whether it was present."""
    font_src = theme_dir / "fonts" / CJK_FONT
    if not font_src.exists():
        return False
    shutil.copyfile(font_src, output_dir / CJK_FONT)
    return True


def _write_pdf_theme(output_dir: Path, theme_dir: Path) -> str | None:
    """Write a PDF-only theme that embeds a CJK font; return its filename.

    Returns ``None`` if the base theme or font is missing. The override drops
    the system CJK names from ``--font-base``: headless Chromium will not fall
    through unresolvable family names to reach the embedded ``@font-face``, so
    the embedded font has to lead the CJK part of the stack.
    """
    base_src = theme_dir / "basic-office.css"
    font_src = theme_dir / "fonts" / CJK_FONT
    if not base_src.exists() or not font_src.exists():
        return None

    _copy_cjk_font(output_dir, theme_dir)
    _copy_latin_fonts(output_dir, theme_dir)
    override = (
        "\n\n/* --- PDF-only CJK embedding (headless Chromium lacks system "
        "CJK fonts) --- */\n"
        f'@font-face {{\n  font-family: "Noto Sans SC";\n'
        f'  src: url("{CJK_FONT}") format("woff2");\n}}\n'
        ':root {\n  --font-base: "Lato", "Helvetica Neue", Helvetica, '
        'Arial, "Noto Sans SC", sans-serif;\n}\n'
        'code {\n  font-family: "Cascadia Code", "SFMono-Regular", "JetBrains Mono", Consolas, '
        '"Liberation Mono",\n    Menlo, "Noto Sans SC", monospace;\n}\n'
    )
    theme_name = "theme-pdf.css"
    (output_dir / theme_name).write_text(
        base_src.read_text(encoding="utf-8") + override, encoding="utf-8"
    )
    return theme_name


def render_pages_pdf(output_dir: Path, *, theme_dir: Path = THEME_DIR) -> Path:
    """Marp-export the pages-only deck to ``pages.pdf`` (embedded CJK, local files)."""
    command = marp_command() + [
        "slides.md", "-o", "pages.pdf", "--html", "--allow-local-files",
    ]
    pdf_theme = _write_pdf_theme(output_dir, theme_dir)
    if pdf_theme is not None:
        command += ["--theme", pdf_theme]
    subprocess.run(command, cwd=output_dir, check=True)
    return output_dir / "pages.pdf"


def render_pages_png(output_dir: Path, *, theme_dir: Path = THEME_DIR) -> None:
    """Marp-export one PNG per page (``slides.NNN.png``), with embedded CJK fonts.

    Like the PDF pages, PNGs render in a headless Chromium that lacks the host's
    CJK fonts, so they use the same font-embedding theme and ``--allow-local-files``.
    With a one-page selection this yields exactly one image.
    """
    command = marp_command() + [
        "slides.md", "-o", "slides.png", "--images", "png",
        "--html", "--allow-local-files",
    ]
    pdf_theme = _write_pdf_theme(output_dir, theme_dir)
    if pdf_theme is not None:
        command += ["--theme", pdf_theme]
    subprocess.run(command, cwd=output_dir, check=True)


def inject_svg_scope(html: str) -> str:
    """Return ``html`` with the SVG-scoping controller inserted before </body>.

    marp-cli bakes marpit-svg-polyfill into every deck; on WebKit it re-runs over
    *every* slide on every animation frame, which costs a full-document layout
    apiece. The controller confines it to the slide on screen. See
    ``assets/svg-scope.js`` for the measurements and the why.

    Applied to every deck we build, not just the live preview: the cost lands on
    whoever opens the deck in Safari, which includes a rendered bundle opened
    long after the session that produced it.
    """
    bundle = f"<script>\n{(ASSETS_DIR / 'svg-scope.js').read_text(encoding='utf-8')}</script>"
    marker = "</body>"
    idx = html.rfind(marker)
    if idx == -1:
        return html + bundle
    return html[:idx] + bundle + html[idx:]


#: The script `marp --watch` appends to every deck it builds: its own WebSocket
#: live-reload client, independent of anything this package broadcasts.
_WATCH_CLIENT_RE = re.compile(
    r"<script>window\.__marpCliWatchWS=.*?</script>", re.DOTALL
)


def strip_watch_client(html: str) -> str:
    """Return ``html`` without marp's own live-reload client.

    ``marp --watch`` bakes a WebSocket client into the deck, so a rebuild
    refreshes the page over a channel this package does not own and cannot make
    exceptions to. That is fine while the only cause of a rebuild is an edit —
    and wrong once a demo's build artifacts can cause one, because the reload
    then wipes the output the presenter just asked for. The dev server strips it
    in that mode and puts the deck on its own SSE channel instead, which knows
    the difference (see ``dev_server.DemoQuiet``).
    """
    return _WATCH_CLIENT_RE.sub("", html)


def build_deck(
    output_dir: Path,
    formats: tuple[str, ...] = ("html",),
    *,
    name: str = "slides",
    theme_dir: Path = THEME_DIR,
) -> None:
    """Build the requested deck products from ``slides.md`` / ``outline.html``.

    HTML is a single Marp run (the viewer iframes it, so it stays ``slides.html``).
    PDF is assembled in three steps so a large outline never clips: the outline is
    printed to its own content-sized page, the pages are Marp-exported, and the two
    are merged into ``{name}.pdf`` (the intermediates are removed) — ``name``
    defaults to ``slides`` but the CLI passes the lecture's slugified title so the
    exported PDF is named after the lecture. PNG emits one image per page.
    """
    if "html" in formats:
        command = marp_command() + [
            "slides.md", "-o", "slides.html", "--html",
        ]
        if _copy_theme(output_dir, theme_dir):
            command += ["--theme", "theme.css"]
        subprocess.run(command, cwd=output_dir, check=True)
        slides_html = output_dir / "slides.html"
        if slides_html.exists():
            slides_html.write_text(
                inject_svg_scope(slides_html.read_text(encoding="utf-8")),
                encoding="utf-8",
            )

    if "png" in formats:
        render_pages_png(output_dir, theme_dir=theme_dir)

    if "pdf" in formats:
        _copy_cjk_font(output_dir, theme_dir)
        outline_pdf = pdf.render_outline_pdf(output_dir)
        pages_pdf = render_pages_pdf(output_dir, theme_dir=theme_dir)
        deck_pdf = output_dir / f"{name}.pdf"
        pdf.merge_pdfs([outline_pdf, pages_pdf], deck_pdf)
        pdf.link_outline_to_slides(deck_pdf)
        outline_pdf.unlink(missing_ok=True)
        pages_pdf.unlink(missing_ok=True)


def watch_command(output_dir: Path, *, theme_dir: Path = THEME_DIR) -> list[str]:
    """The persistent `marp --watch` command; copies the theme if present."""
    command = marp_command() + [
        "slides.md", "-o", "slides.html", "--html", "--watch",
    ]
    if _copy_theme(output_dir, theme_dir):
        command += ["--theme", "theme.css"]
    return command
