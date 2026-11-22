"""The face of Chinese inside a formula, where only a browser tells."""

import json
import shutil
import subprocess
from pathlib import Path

import pytest

from lecturekit import cli
from lecturekit.renderers.viewer import marp
from lecturekit.renderers.viewer.pdf import find_chrome, print_command

MODULES = marp.PKG_ROOT / "node_modules"
DRIVER = Path(__file__).parent / "fixtures" / "browser" / "computed.cjs"
FACES = """({
  body: getComputedStyle(document.querySelector('section')).fontFamily,
  math: [...document.querySelectorAll('mjx-container text')].map(t => getComputedStyle(t).fontFamily),
})"""


def test_chinese_in_a_formula_takes_the_face_of_the_body(tmp_path):
    if shutil.which("node") is None or not (MODULES / "puppeteer-core").exists():
        pytest.skip("needs node and puppeteer-core (scripts/prepare.sh)")
    try:
        find_chrome()
    except RuntimeError:
        pytest.skip("needs Chrome")
    source, out = tmp_path / "lecture", tmp_path / "out"
    source.mkdir()
    body = "$\\\\text{步长}$ and $$\\\\frac{\\\\text{位}}{G}$$"
    (source / "lecture.py").write_text(
        "from lecturekit.dsl import Lecture\n"
        "lecture = Lecture(id='m', title='M')\n"
        f"lecture.page('p', body=lambda p: (p.title('T'), p.slide('{body}')))\n",
        encoding="utf-8",
    )
    assert cli.main(["render", str(source), "--out", str(out)]) == 0
    chrome, *args = print_command()
    done = subprocess.run(
        ["node", str(DRIVER), str(MODULES), chrome, *[arg for arg in args if arg == "--no-sandbox"]],
        input=json.dumps([{"url": (out / "slides.html").as_uri(), "system": "light", "read": FACES}]),
        capture_output=True, text=True, timeout=60,
    )
    assert done.returncode == 0, done.stderr
    faces = json.loads(done.stdout)[0]["value"]
    assert faces["math"], "the formula has no text of its own"
    assert set(faces["math"]) == {faces["body"]}
