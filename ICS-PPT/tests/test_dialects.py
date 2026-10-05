"""Code dialects: a base language plus the words it adds (`lecturekit.dialects`).

`p.code("cuda", ...)` is the one that exists. The deck colours it through the
engine marp-cli is run with, the book through a `listings` language the
preamble defines; both read ``dialects.json``.
"""

import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest

from lecturekit import dialects
from lecturekit.dsl import Lecture
from lecturekit.renderers.latex import preamble
from lecturekit.renderers.viewer import marp
from lecturekit.renderers.viewer.blocks import _code

VENDORED_MARP = marp.PKG_ROOT / "node_modules" / "@marp-team" / "marp-cli" / "marp-cli.js"

KERNEL = """__global__ void scale(int *v, int n) {
    __shared__ int cache[256];
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    __syncthreads();
    dim3 grid(1);
}"""


def _block(language, content):
    def body(p):
        p.title("T")
        p.code(language, content)

    lec = Lecture(id="t", title="T")
    lec.page(id="p", body=body)
    return lec.build().children[0].blocks[0]


def test_every_dialect_names_its_base_and_adds_identifiers():
    assert "cuda" in dialects.DIALECTS
    for name, dialect in dialects.DIALECTS.items():
        assert dialect["hljs"] and dialect["listings"], name
        assert set(dialect) <= {"hljs", "listings", *dialects.ROLES}, name
        added = dialects.words(name)
        assert added and len(added) == len(set(added)), name
        for word in added:
            # A word goes into a JS array and into a LaTeX key list as written.
            assert re.fullmatch(r"[A-Za-z_]\w*", word), (name, word)


def test_cuda_adds_what_c_plus_plus_lacks():
    added = set(dialects.words("cuda"))
    assert {"__global__", "__shared__", "threadIdx", "blockIdx", "__syncthreads", "dim3"} <= added
    assert "int" not in added  # the base's own words stay the base's


def test_the_deck_hands_marp_the_dialect_name():
    assert _code(_block("cuda", KERNEL))[0] == "```cuda"


def test_every_marp_run_goes_through_the_engine():
    assert marp.ENGINE.exists()
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        with patch("lecturekit.renderers.viewer.marp.subprocess.run") as run, \
                patch("lecturekit.renderers.viewer.marp.pdf.render_outline_pdf"), \
                patch("lecturekit.renderers.viewer.marp.pdf.merge_pdfs"), \
                patch("lecturekit.renderers.viewer.marp.pdf.link_outline_to_slides"):
            marp.build_deck(out, formats=("html", "png", "pdf"))
        commands = [call.args[0] for call in run.call_args_list]
        commands.append(marp.watch_command(out))
    assert len(commands) == 4
    for command in commands:
        assert command[command.index("--engine") + 1] == str(marp.ENGINE), command


def test_the_book_defines_each_dialect_for_listings():
    line = next(l for l in preamble._DIALECTS.splitlines() if "{cuda}" in l)
    assert line.startswith("\\lstdefinelanguage{cuda}[]{C++}{morekeywords={")
    for word in dialects.words("cuda"):
        assert re.search(r"[{,]%s[,}]" % re.escape(word), line), word


@pytest.mark.skipif(
    shutil.which("node") is None or not VENDORED_MARP.exists(),
    reason="needs node and the vendored marp-cli (scripts/prepare.sh)",
)
def test_marp_colours_a_dialect_and_leaves_its_base_alone(tmp_path):
    fence = "```%s\n" + KERNEL + "\n```\n"
    (tmp_path / "slides.md").write_text(
        "---\nmarp: true\n---\n\n" + "\n---\n\n".join(fence % l for l in ("cuda", "cpp", "c++")),
        encoding="utf-8",
    )
    subprocess.run(
        [shutil.which("node"), str(VENDORED_MARP), "slides.md", "-o", "slides.html", "--html"]
        + marp.ENGINE_ARGS,
        cwd=tmp_path, check=True, stdin=subprocess.DEVNULL, capture_output=True,
    )
    html = (tmp_path / "slides.html").read_text(encoding="utf-8")
    blocks = re.findall(r'<code class="language-([^"]+)">(.*?)</code>', html, re.S)
    assert [language for language, _ in blocks] == ["cuda", "cpp", "c++"]
    cuda, cpp, alias = (body for _, body in blocks)

    assert '<span class="hljs-keyword">__global__</span>' in cuda
    assert '<span class="hljs-keyword">__shared__</span>' in cuda
    assert '<span class="hljs-built_in">threadIdx</span>' in cuda
    assert '<span class="hljs-built_in">__syncthreads</span>' in cuda
    assert '<span class="hljs-type">dim3</span>' in cuda
    assert '<span class="hljs-type">int</span>' in cuda  # still C++ underneath

    # C++ itself, under its name and under an alias, learned nothing.
    for body in (cpp, alias):
        assert '<span class="hljs-type">int</span>' in body
        assert not re.search(r'<span class="hljs-[a-z_]+">(__global__|__shared__|threadIdx)</span>', body)
