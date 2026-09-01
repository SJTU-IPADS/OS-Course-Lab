"""Guard the reveal.css <-> theme coupling.

reveal.css (live-preview only) re-applies the theme's `section > X` direct-child
rules one level deeper (`section > .reveal-block > X`), because wrapping a block
in `.reveal-block` would otherwise make it a grandchild of <section> and drop
that styling. If someone adds a new `section > X` rule to the theme and forgets
reveal.css, wrapped blocks in the live preview silently lose it. This test fails
when the two drift apart.
"""

import re
from pathlib import Path

THEME = Path(__file__).resolve().parents[1] / "themes" / "basic-office.css"
REVEAL_CSS = (
    Path(__file__).resolve().parents[1]
    / "lecturekit" / "renderers" / "viewer" / "assets" / "reveal.css"
)

# `section > <selector> {` — capture the whole direct-child chain after the
# first `section >`, up to the rule's `{`. The chain may itself contain `>`
# combinators (e.g. "figure.lk-row > figcaption"), so the capture must allow
# `>` — a `[^{>]` class would silently skip every two-level `section > A > B`
# rule and let it drift unmirrored. Examples captured: "*", "table",
# "figure.lk-figure", "figure.lk-row > figcaption".
_SECTION_CHILD = re.compile(r"section\s*>\s*([^{]+?)\s*\{")


def _theme_direct_child_selectors() -> set[str]:
    selectors = set()
    for match in _SECTION_CHILD.finditer(THEME.read_text()):
        for selector in match.group(1).split(","):
            selector = re.sub(r"^section\s*>\s*", "", selector.strip())
            selectors.add(selector)
    return selectors


def test_reveal_css_mirrors_every_theme_section_child_rule():
    theme_selectors = _theme_direct_child_selectors()
    assert theme_selectors, "expected `section > X` rules in the theme"

    reveal_text = REVEAL_CSS.read_text()
    missing = [
        sel for sel in theme_selectors
        if f"section > .reveal-block > {sel}" not in reveal_text
    ]
    assert not missing, (
        "reveal.css must mirror the theme's `section > X` rules as "
        f"`section > .reveal-block > X`; missing: {sorted(missing)}"
    )


REVEAL_JS = REVEAL_CSS.with_name("reveal.js")
VIEWER = REVEAL_CSS.parents[1] / "__init__.py"


def test_item_reveal_contract_holds_across_the_three_files():
    """`reveal="items"` is one feature split over a build step and two assets.

    The renderer only writes a flag; reveal.js finds the rendered <li>s and
    dims them by a class reveal.css styles. Rename any of the three names in
    one file and the split silently stops working, so pin them together.
    """
    js, css, viewer = (
        REVEAL_JS.read_text(), REVEAL_CSS.read_text(), VIEWER.read_text()
    )
    assert "data-reveal-items" in viewer, "the renderer must emit the flag"
    assert "data-reveal-items" in js, "reveal.js must read the flag it is sent"
    assert 'ITEM = "reveal-item"' in js, "reveal.js must dim items by this class"
    assert ".reveal-item.reveal-dim" in css, "reveal.css must style dimmed items"
