"""Guard the reveal.css <-> theme coupling.

reveal.css (live-preview only) re-applies the theme's rules for a column's
direct children (`section :where(.lk-main, .lk-side) > X`) one level deeper
(`… > .reveal-block > X`), because wrapping a block in `.reveal-block` would
otherwise make it a grandchild of its column and drop that styling. If someone
adds a new column-child rule to the theme and forgets reveal.css, wrapped blocks
in the live preview silently lose it. This test fails when the two drift apart.
"""

import re
from pathlib import Path

THEME = Path(__file__).resolve().parents[1] / "themes" / "basic-office.css"
REVEAL_CSS = (
    Path(__file__).resolve().parents[1]
    / "lecturekit" / "renderers" / "viewer" / "assets" / "reveal.css"
)

COLUMN = "section :where(.lk-main, .lk-side) > "
# `<COLUMN><selector>` up to the next `,` of a selector list or the rule's `{`.
# The selector may itself contain `>` combinators (e.g.
# "figure.lk-row > figcaption"). Examples captured: "*", "table",
# "figure.lk-figure", "figure.lk-row > figcaption".
_COLUMN_CHILD = re.compile(re.escape(COLUMN) + r"([^{,]+?)\s*[{,]")


def _theme_direct_child_selectors() -> set[str]:
    return {
        match.group(1).strip()
        for match in _COLUMN_CHILD.finditer(THEME.read_text())
        if not match.group(1).startswith(".reveal-block")
    }


def test_reveal_css_mirrors_every_theme_column_child_rule():
    theme_selectors = _theme_direct_child_selectors()
    assert {"*", "table", "figure.lk-figure"} <= theme_selectors

    reveal_text = REVEAL_CSS.read_text()
    missing = [
        sel for sel in theme_selectors
        if f"{COLUMN}.reveal-block > {sel}" not in reveal_text
    ]
    assert not missing, (
        "reveal.css must mirror the theme's column-child rules as "
        f"`{COLUMN}.reveal-block > X`; missing: {sorted(missing)}"
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


def test_dim_lands_once_on_a_gap_page_block():
    """A `p.gap` page keeps the wrapper's box, so a dim on the wrapper and on
    its children would multiply and blank the page. Only the children dim."""
    css = REVEAL_CSS.read_text()
    assert "[data-reveal]:not(.reveal-block).reveal-dim" in css
    assert "\n[data-reveal].reveal-dim" not in css
