"""Links in slide text: a web link opens in a new tab, as `p.link` does.

The deck is one page; a link followed in place would take it away. The
engine marp-cli is run with (``marp-engine.cjs``) marks every http(s) link
of the slide text; other addresses keep Marp's rendering.
"""

import re
import shutil
import subprocess

import pytest

from lecturekit.renderers.viewer import marp

VENDORED_MARP = marp.PKG_ROOT / "node_modules" / "@marp-team" / "marp-cli" / "marp-cli.js"


@pytest.mark.skipif(
    shutil.which("node") is None or not VENDORED_MARP.exists(),
    reason="needs node and the vendored marp-cli (scripts/prepare.sh)",
)
def test_a_web_link_opens_in_a_new_tab_and_others_stay(tmp_path):
    (tmp_path / "slides.md").write_text(
        "---\nmarp: true\n---\n\n"
        "- 提交页面：[https://example.org:8443/submit](https://example.org:8443/submit)\n"
        "- [plain](http://example.org/)\n"
        "- [relative](notes.html) and [anchor](#2)\n",
        encoding="utf-8",
    )
    subprocess.run(
        [shutil.which("node"), str(VENDORED_MARP), "slides.md", "-o", "slides.html", "--html"]
        + marp.ENGINE_ARGS,
        cwd=tmp_path, check=True, stdin=subprocess.DEVNULL, capture_output=True,
    )
    html = (tmp_path / "slides.html").read_text(encoding="utf-8")
    links = dict((m.group(2), m.group(1)) for m in re.finditer(r"<a ([^>]*)>([^<]*)</a>", html))

    for label in ("https://example.org:8443/submit", "plain"):
        assert 'target="_blank"' in links[label]
        assert 'rel="noopener noreferrer"' in links[label]
    # The text before the link stays outside it.
    assert "提交页面：<a " in html
    for label in ("relative", "anchor"):
        assert "target=" not in links[label]
