"""--color-scheme: a scheme is a file, the web pages follow it, and paper never does."""

import re
import shutil
import subprocess
import threading
import urllib.error
import urllib.request
from pathlib import Path
from unittest.mock import patch

import pytest

from lecturekit import cli, dev_server, tokens
from lecturekit.renderers.viewer import StaticViewerRenderer, colorscheme, marp

SAMPLE = "tests/fixtures/sample"
ASSETS = Path(marp.__file__).parent / "assets"
DEFAULT = tokens.DEFAULT_SCHEME
# Every scheme there is: a file added to themes/schemes/ is tested by all below.
SCHEMES = tokens.schemes()
OTHERS = tuple(name for name in SCHEMES if name != DEFAULT)
CHOICES = colorscheme.choices()
DARK_SYSTEM = "screen and (prefers-color-scheme: dark)"
SCREEN_ONLY = re.compile(r"<style[^>]* media=.*?</style>", re.S)
LAYER = re.compile(r'<style data-lk-scheme media="([^"]*)">(.*?)</style>', re.S)
# What can be painted from a scheme without moving a box.
PAINT = re.compile(
    r"--.*|color|opacity|fill|stroke|(caret|accent|scrollbar)-color|box-shadow|text-shadow"
    r"|background(-color|-image)?|border(-(top|right|bottom|left|block|inline))?(-color)?"
    r"|outline(-color)?|text-decoration(-color)?"
)


def render(out: Path, *args: str) -> Path:
    assert cli.main(["render", SAMPLE, "--out", str(out), "--no-build", *args]) == 0
    return out


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def meta(choice: str) -> str:
    grounds = "light dark" if choice == "auto" else tokens.scheme(choice).base
    return f'<meta name="color-scheme" content="{grounds}">'


def layers(html: str) -> dict[str, str]:
    """The schemes laid over the default in ``html``, by media query."""
    return dict(LAYER.findall(html))


def declared(colours: dict[str, str]) -> str:
    return "".join(f"  {name}: {value};\n" for name, value in colours.items())


@pytest.fixture
def scheme_dir(tmp_path, monkeypatch):
    """A copy of themes/schemes/ to add to and to break."""
    directory = tmp_path / "schemes"
    shutil.copytree(tokens.SCHEME_DIR, directory)
    monkeypatch.setattr(tokens, "SCHEME_DIR", directory)
    tokens.scheme.cache_clear()
    yield directory
    tokens.scheme.cache_clear()


def add_scheme(directory: Path, name: str, base: str, **change: str) -> Path:
    css = read(directory / f"{DEFAULT}.css")
    css = re.sub(r"color-scheme: \w+;", f"color-scheme: {base};", css)
    for token, value in change.items():
        token = "--" + token.replace("_", "-")
        css = re.sub(f"{token}: [^;]+;", f"{token}: {value};", css)
    path = directory / f"{name}.css"
    path.write_text(css, encoding="utf-8")
    return path


# --- the files ---


@pytest.mark.parametrize("name", SCHEMES)
def test_every_scheme_defines_the_defaults_tokens_and_no_others(name):
    scheme = tokens.scheme(name)
    assert scheme.base in ("light", "dark")
    assert scheme.colours.keys() == tokens.scheme(DEFAULT).colours.keys()


def test_one_file_is_a_whole_scheme(scheme_dir):
    add_scheme(scheme_dir, "second-dark", "dark", color_bg="#010203")
    assert "second-dark" in colorscheme.choices()
    html = colorscheme.head("second-dark")
    assert '<meta name="color-scheme" content="dark">' in html
    assert "--color-bg: #010203;" in layers(html)["screen"]


@pytest.mark.parametrize(
    "old, new, said",
    [
        ("--color-bg: ", "--color-gb: ", "lacks --color-bg and has unknown --color-gb"),
        ("--color-fg: ", "--color-fg: var(--color-typo); --x: ", "--color-typo"),
        ("color-scheme: ", "--no-base: ", "needs color-scheme: light or dark"),
        (":root {", "section { color: red }\n:root {", "is not one :root block"),
    ],
)
def test_a_broken_scheme_is_named_with_what_is_wrong(scheme_dir, old, new, said):
    path = add_scheme(scheme_dir, "broken", "dark")
    path.write_text(read(path).replace(old, new, 1), encoding="utf-8")
    with pytest.raises(ValueError) as caught:
        tokens.scheme("broken")
    assert "themes/schemes/broken.css" in str(caught.value)
    assert said in str(caught.value)


@pytest.mark.parametrize("name", SCHEMES)
def test_the_shell_stands_on_the_slides_ground(name):
    colours = tokens.scheme(name).colours
    assert colours["--shell-bg"] in ("var(--color-bg)", colours["--color-bg"])


def stylesheets() -> list[Path]:
    return [marp.THEME_DIR / "basic-office.css", *sorted(ASSETS.glob("*.css"))]


@pytest.mark.parametrize("path", stylesheets(), ids=lambda path: path.name)
def test_a_scheme_changes_paint_only(path):
    css = re.sub(r"/\*.*?\*/", "", read(path), flags=re.S)
    names = set(tokens.scheme(DEFAULT).colours)
    moved = set()
    for block in re.findall(r"\{([^{}]*)\}", css):
        for declaration in block.split(";"):
            prop, _, value = declaration.partition(":")
            if names & set(re.findall(r"var\(\s*(--[\w-]+)", value)):
                if not PAINT.fullmatch(prop.strip()):
                    moved.add(prop.strip())
    assert moved == set()


# --- the deck's theme ---


def test_the_decks_theme_has_the_default_colours_whatever_is_chosen(tmp_path):
    marp._copy_theme(tmp_path, marp.THEME_DIR)
    css = read(tmp_path / "theme.css")
    assert css.startswith(read(marp.THEME_DIR / "basic-office.css"))
    layer = css[css.index("@layer lk-scheme"):]
    assert declared(tokens.scheme(DEFAULT).colours) in layer
    assert "prefers-color-scheme" not in css


def test_marp_watch_rebuilds_from_that_same_theme(tmp_path):
    marp.watch_command(tmp_path)
    css = colorscheme.theme(read(marp.THEME_DIR / "basic-office.css"))
    assert read(tmp_path / "theme.css") == css


def test_pdf_and_png_have_the_default_colours_alone(tmp_path):
    name = marp._write_pdf_theme(tmp_path, marp.THEME_DIR)
    if name is None:
        pytest.skip("no CJK font here to write the PDF theme with")
    css = read(tmp_path / name)
    assert css.startswith(colorscheme.theme(read(marp.THEME_DIR / "basic-office.css")))
    assert "data-lk-scheme" not in css
    assert "prefers-color-scheme" not in css


# --- the head of a page ---


@pytest.mark.parametrize("choice", CHOICES)
def test_the_head_opens_with_the_charset_and_the_grounds(choice):
    assert colorscheme.head(choice).startswith('<meta charset="utf-8">' + meta(choice))


@pytest.mark.parametrize("choice", CHOICES)
def test_the_document_is_the_default_scheme_outside_the_screen(choice):
    html = colorscheme.head(choice)
    always = SCREEN_ONLY.sub("", html)
    assert f"color-scheme: {tokens.scheme(DEFAULT).base};" in always
    assert declared(tokens.scheme(DEFAULT).colours) in always
    assert all(media.startswith("screen") for media in re.findall(r'media="([^"]*)"', html))


def test_the_default_scheme_lays_nothing_over_itself():
    html = colorscheme.head(DEFAULT)
    assert layers(html) == {}
    assert "prefers-color-scheme" not in html


@pytest.mark.parametrize("name", OTHERS)
def test_a_pinned_scheme_holds_whatever_the_system_is(name):
    html = colorscheme.head(name)
    assert list(layers(html)) == ["screen"]
    assert "prefers-color-scheme" not in html
    assert declared(tokens.scheme(name).colours) in layers(html)["screen"]


def test_auto_is_the_default_and_the_dark_scheme_on_a_dark_system():
    laid = layers(colorscheme.head("auto"))
    assert list(laid) == [DARK_SYSTEM]
    assert declared(tokens.scheme("dark").colours) in laid[DARK_SYSTEM]


@pytest.mark.parametrize("name", OTHERS)
def test_a_head_cut_short_has_the_scheme_before_the_default(name):
    # Firefox paints what has arrived, and the default alone would flash.
    html = colorscheme.head(name)
    default = html.index("@layer lk-scheme")
    assert html.index(declared(tokens.scheme(name).colours)) < default
    assert "color-scheme: " not in html[html.index("<style>"):html.index("<style data-lk-scheme")]


def test_the_head_goes_first_into_the_documents():
    html = colorscheme.insert("<html><head><script></script></head></html>", "HEAD")
    assert html == "<html><head>HEAD<script></script></head></html>"


def test_a_document_without_a_head_is_led_by_it():
    assert colorscheme.insert("<body>half", "HEAD") == "HEAD<body>half"


# --- the bundle ---


def test_render_follows_the_system_by_default(tmp_path):
    out = render(tmp_path)
    assert meta("auto") in read(out / "index.html")
    assert read(out / colorscheme.HEAD_FILE) == colorscheme.head("auto")


@pytest.mark.parametrize("choice", CHOICES)
def test_render_puts_the_choice_in_the_two_heads(tmp_path, choice):
    out = render(tmp_path, "--color-scheme", choice)
    assert colorscheme.head(choice) in read(out / "index.html")
    assert read(out / colorscheme.HEAD_FILE) == colorscheme.head(choice)


def test_nothing_else_in_the_bundle_differs_by_scheme(tmp_path):
    chosen = {"index.html", colorscheme.HEAD_FILE}
    bundles = []
    for choice in CHOICES:
        out = render(tmp_path / choice, "--color-scheme", choice)
        bundles.append({
            path.relative_to(out): path.read_bytes()
            for path in out.rglob("*") if path.is_file() and path.name not in chosen
        })
    # slides.md and outline.html, which the PDF and the PNG are made of, included.
    assert all(bundle == bundles[0] for bundle in bundles)


def test_the_printed_outline_is_in_the_default_colours(tmp_path):
    html = read(render(tmp_path, "--color-scheme", OTHERS[0]) / "outline.html")
    assert colorscheme.default_colours() in html
    assert "data-lk-scheme" not in html


def test_the_python_api_stays_with_the_default(tmp_path):
    StaticViewerRenderer().render(cli.load_lecture(Path(SAMPLE)), tmp_path)
    assert read(tmp_path / colorscheme.HEAD_FILE) == colorscheme.head(DEFAULT)


@pytest.mark.parametrize("choice", CHOICES)
def test_build_deck_dresses_the_deck(tmp_path, choice):
    def built(command, cwd, **kwargs):
        Path(cwd, "slides.html").write_text(
            "<html><head></head><body></body></html>", encoding="utf-8"
        )
        return subprocess.CompletedProcess(command, 0)

    head = colorscheme.head(choice)
    (tmp_path / colorscheme.HEAD_FILE).write_text(head, encoding="utf-8")
    with patch.object(marp.subprocess, "run", built):
        marp.build_deck(tmp_path)
    assert f"<head>{head}" in read(tmp_path / "slides.html")


# --- the command line ---


@pytest.mark.parametrize("args, expected", [([], "auto"), (["--color-scheme", DEFAULT], DEFAULT)])
def test_render_has_the_head_ready_for_the_deck_build(tmp_path, args, expected):
    def build(output_dir, *_args, **_options):
        assert read(output_dir / colorscheme.HEAD_FILE) == colorscheme.head(expected)

    with patch.object(cli, "build_deck", side_effect=build) as built:
        assert cli.main(["render", SAMPLE, "--out", str(tmp_path), *args]) == 0
    built.assert_called_once()


@pytest.mark.parametrize("name", SCHEMES)
def test_view_has_the_head_ready_for_the_deck_build(tmp_path, name):
    def build(output_dir, *_args, **_options):
        assert read(output_dir / colorscheme.HEAD_FILE) == colorscheme.head(name)

    with patch.object(cli, "build_deck", side_effect=build) as built:
        with patch.object(cli, "open_in_browser"):
            assert cli.main(["view", SAMPLE, "--out", str(tmp_path), "--color-scheme", name]) == 0
    built.assert_called_once()


@pytest.mark.parametrize(
    "args, expected",
    [([], "auto"), *((["--color-scheme", name], name) for name in SCHEMES)],
)
def test_watch_hands_the_scheme_to_the_server(tmp_path, args, expected):
    with patch.object(cli, "serve") as serve:
        assert cli.main(["view", SAMPLE, "--watch", "--out", str(tmp_path), *args]) == 0
    assert serve.call_args.kwargs["color_scheme"] == expected


def test_a_scheme_without_a_web_page_is_refused(tmp_path, capsys):
    code = cli.main(
        ["render", SAMPLE, "--out", str(tmp_path), "--to", "transcript", "--color-scheme", "dark"]
    )
    assert code == 1
    assert "--color-scheme" in capsys.readouterr().err


def test_the_default_scheme_is_no_error_without_a_web_page(tmp_path):
    assert cli.main(["render", SAMPLE, "--out", str(tmp_path), "--to", "transcript"]) == 0


def test_an_unknown_scheme_is_refused(tmp_path, capsys):
    with pytest.raises(SystemExit):
        cli.main(["render", SAMPLE, "--out", str(tmp_path), "--color-scheme", "no-such-scheme"])
    said = capsys.readouterr().err
    assert all(name in said for name in CHOICES)


# --- a watch session ---


@pytest.fixture
def lecture_dir(tmp_path):
    directory = tmp_path / "lecture"
    directory.mkdir()
    (directory / "lecture.py").write_text(
        "from lecturekit.dsl import Lecture\n"
        "lecture = Lecture(id='t', title='T')\n"
        "lecture.page('p', body=lambda p: (p.title('P'), p.slide('x')))\n",
        encoding="utf-8",
    )
    return directory


@pytest.mark.parametrize("choice", CHOICES)
def test_every_render_of_a_session_holds_the_scheme(tmp_path, lecture_dir, choice):
    out = tmp_path / "out"
    source = lecture_dir / "lecture.py"
    changed = [(dev_server.Change.modified, str(source))]
    broadcaster = dev_server.ReloadBroadcaster()

    def held():
        assert read(out / colorscheme.HEAD_FILE) == colorscheme.head(choice)
        assert colorscheme.head(choice) in read(out / "index.html")

    dev_server.render_once(lecture_dir, out, color_scheme=choice)
    held()
    dev_server.handle_changes(changed, lecture_dir, out, broadcaster, color_scheme=choice)
    held()
    source.write_text("syntax ((( error", encoding="utf-8")
    dev_server.handle_changes(changed, lecture_dir, out, broadcaster, color_scheme=choice)
    assert "Lecture source error" in read(out / "lecture.json")
    held()


@pytest.mark.parametrize("choice", CHOICES)
def test_the_server_dresses_the_deck_and_its_404(tmp_path, choice):
    deck = tmp_path / "slides.html"
    deck.write_text("<html><head></head><body></body></html>", encoding="utf-8")
    on_disk = deck.read_bytes()
    # A percent sign, which the stdlib's error page would take for a place to fill in.
    head = colorscheme.head(choice) + "<!-- 100% -->"
    (tmp_path / colorscheme.HEAD_FILE).write_text(head, encoding="utf-8")
    handler = dev_server.make_handler(tmp_path, dev_server.ReloadBroadcaster(), reveal=False)
    server = dev_server.QuietHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    base = f"http://127.0.0.1:{server.server_address[1]}"
    try:
        with urllib.request.urlopen(f"{base}/slides.html", timeout=5) as response:
            served = response.read().decode("utf-8")
        with pytest.raises(urllib.error.HTTPError) as missing:
            urllib.request.urlopen(f"{base}/none.html", timeout=5)
        error_page = missing.value.read().decode("utf-8")
    finally:
        server.shutdown()
        server.server_close()
    assert f"<head>{head}" in served
    assert head in error_page
    # The head's declaration stands: nothing after it declares another.
    assert "color-scheme" not in error_page.replace(head, "")
    assert missing.value.code == 404
    assert "404" in error_page and "File not found" in error_page
    # `marp --watch` owns the file, so only the response is changed.
    assert deck.read_bytes() == on_disk


def test_serve_renders_in_the_scheme_before_marp_starts(tmp_path, lecture_dir):
    out = tmp_path / "out"

    class Server:
        def __init__(self, address, handler):
            pass

        def serve_forever(self):
            pass

        def shutdown(self):
            pass

        def server_close(self):
            pass

    def command(output_dir):
        assert read(output_dir / colorscheme.HEAD_FILE) == colorscheme.head("dark")
        return ["marp"]

    with (
        patch.object(dev_server.subprocess, "Popen") as popen,
        patch.object(dev_server, "watch_command", side_effect=command) as watched,
        patch.object(dev_server, "QuietHTTPServer", Server),
        patch.object(dev_server, "_terminate_process_group"),
        patch.object(dev_server, "watch", side_effect=KeyboardInterrupt),
    ):
        popen.return_value.poll.return_value = 0
        popen.return_value.stdout = []
        dev_server.serve(lecture_dir, out, open_browser=False, color_scheme="dark")
    watched.assert_called_once()


def test_a_framed_deck_leaves_reloading_to_its_shell():
    # Two reloads at once cut the inner one short, which shows as a white frame.
    client = dev_server.LIVERELOAD_CLIENT % (0,)
    assert client.index("parent.lkLiveReload") < client.index("function poll")
