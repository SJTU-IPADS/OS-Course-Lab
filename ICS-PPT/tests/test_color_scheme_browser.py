"""What a browser paints from a colour scheme, where the CSS alone does not tell:
the page number's ground, and the dev server's error page.

One headless Chrome, under every scheme and both systems.
"""

import json
import shutil
import subprocess
import threading
from pathlib import Path

import pytest

from lecturekit import cli, dev_server
from lecturekit.renderers.viewer import colorscheme, marp
from lecturekit.renderers.viewer.pdf import find_chrome, print_command

MODULES = marp.PKG_ROOT / "node_modules"
DRIVER = Path(__file__).parent / "fixtures" / "browser" / "computed.cjs"
CHOICES = colorscheme.choices()
SYSTEMS = ("light", "dark")
SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="36"/>'
GROUNDS = {
    "plain": "",
    "picture": "<!-- _backgroundImage: url('assets/plain.svg') -->",
    "with-variant": "<!-- _backgroundImage: url('assets/drawn.svg') -->",
    "gradient": "<!-- _backgroundImage: linear-gradient(#f00, #00f) -->",
    "colour": "<!-- _backgroundColor: #cfe8cf -->",
    "inverted-picture": "<!-- _class: invert -->\n<!-- _backgroundImage: url('assets/plain.svg') -->",
    "inverted-variant": "<!-- _class: invert -->\n<!-- _backgroundImage: url('assets/drawn.svg') -->",
    "bg-syntax": "![bg](assets/plain.svg)",
}
NUMBER_GROUND = """[...document.querySelectorAll('section')].map(section => {
  const own = getComputedStyle(section), number = getComputedStyle(section, '::after');
  return {ground: own.backgroundColor, adapt: own.getPropertyValue('--adapt-page-number').trim(),
          image: number.backgroundImage, colour: number.backgroundColor, ink: number.color};
})"""
PAGE_GROUND = """({
  scheme: getComputedStyle(document.documentElement).colorScheme,
  root: getComputedStyle(document.documentElement).backgroundColor,
  body: getComputedStyle(document.body).backgroundColor,
  ink: getComputedStyle(document.body).color,
  said: document.body.innerText,
})"""


def _missing() -> str | None:
    if shutil.which("node") is None:
        return "needs node"
    for package in ("puppeteer-core", "@marp-team/marp-cli"):
        if not (MODULES / package).exists():
            return f"needs {package} (scripts/prepare.sh)"
    try:
        find_chrome()
    except RuntimeError:
        return "needs Chrome"
    return None


@pytest.fixture(scope="module")
def painted(tmp_path_factory):
    missing = _missing()
    if missing:
        pytest.skip(missing)
    work = tmp_path_factory.mktemp("painted")
    servers = []
    try:
        source = work / "lecture"
        (source / "assets").mkdir(parents=True)
        for name in ("plain.svg", "drawn.svg", "drawn.dark.svg"):
            (source / "assets" / name).write_text(SVG, encoding="utf-8")
        pages = "".join(
            f"lecture.page({name!r}, body=lambda p: (p.title('T'), p.slide({(css + chr(10) * 2 + 'x')!r})))\n"
            for name, css in GROUNDS.items()
        )
        (source / "lecture.py").write_text(
            "from lecturekit.dsl import Lecture\nlecture = Lecture(id='g', title='G')\n" + pages,
            encoding="utf-8",
        )
        jobs, keys = [], []
        for choice in CHOICES:
            out = work / choice
            assert cli.main(["render", str(source), "--out", str(out), "--color-scheme", choice]) == 0
            handler = dev_server.make_handler(out, dev_server.ReloadBroadcaster(), reveal=False)
            server = dev_server.QuietHTTPServer(("127.0.0.1", 0), handler)
            threading.Thread(target=server.serve_forever, daemon=True).start()
            servers.append(server)
            base = f"http://127.0.0.1:{server.server_address[1]}"
            for system in SYSTEMS:
                for what, url, read in (
                    ("deck", f"{base}/slides.html", NUMBER_GROUND),
                    ("viewer", f"{base}/", PAGE_GROUND),
                    ("error", f"{base}/none.html", PAGE_GROUND),
                ):
                    jobs.append({"url": url, "system": system, "read": read})
                    keys.append((what, choice, system))
        chrome, *args = print_command()
        # Puppeteer has flags of its own; all it lacks is what root needs.
        args = [arg for arg in args if arg == "--no-sandbox"]
        done = subprocess.run(
            ["node", str(DRIVER), str(MODULES), chrome, *args],
            input=json.dumps(jobs), capture_output=True, text=True, timeout=120,
        )
        assert done.returncode == 0, done.stderr
    finally:
        for server in servers:
            server.shutdown()
            server.server_close()
    return dict(zip(keys, json.loads(done.stdout)))


@pytest.mark.parametrize("system", SYSTEMS)
@pytest.mark.parametrize("choice", CHOICES)
def test_the_page_number_takes_the_slides_colour_and_no_picture(painted, choice, system):
    sections = painted["deck", choice, system]["value"]
    assert len(sections) >= len(GROUNDS)
    for section in sections:
        assert section["image"] == "none", section
        assert section["colour"] != section["ink"], "the number cannot be read"
        if not section["adapt"]:  # `initial`: the slide's own colour
            assert section["colour"] == section["ground"], section


@pytest.mark.parametrize("system", SYSTEMS)
@pytest.mark.parametrize("choice", CHOICES)
def test_the_error_page_stands_on_the_viewers_ground(painted, choice, system):
    error, viewer = painted["error", choice, system], painted["viewer", choice, system]
    assert (error["status"], viewer["status"]) == (404, 200)
    for what in ("scheme", "root", "body", "ink"):
        assert error["value"][what] == viewer["value"][what], what
    assert "404" in error["value"]["said"] and "File not found" in error["value"]["said"]
