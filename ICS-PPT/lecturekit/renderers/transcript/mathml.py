"""MathML as a browser's MathML Core reads it.

MathJax's MathML uses three things MathML Core dropped. A ``mathvariant``
becomes Unicode's mathematical letters, and a table's ``columnalign`` and
``columnspacing`` become the cells' own CSS. Only presentation MathML is
written: any other element an author put in a formula becomes an ``<mrow>``,
and an attribute that is private, acts or loads goes.
"""

from __future__ import annotations

import re
import unicodedata
import xml.etree.ElementTree as ET

ALPHABETS = {
    "bold": "BOLD", "italic": "ITALIC", "bold-italic": "BOLD ITALIC",
    "double-struck": "DOUBLE-STRUCK", "script": "SCRIPT", "bold-script": "BOLD SCRIPT",
    "fraktur": "FRAKTUR", "bold-fraktur": "BOLD FRAKTUR", "monospace": "MONOSPACE",
    "sans-serif": "SANS-SERIF", "bold-sans-serif": "SANS-SERIF BOLD",
    "sans-serif-italic": "SANS-SERIF ITALIC",
    "sans-serif-bold-italic": "SANS-SERIF BOLD ITALIC",
}
TOKENS = frozenset({"mi", "mn", "mo", "mtext", "ms"})
ELEMENTS = TOKENS | frozenset(
    "math mrow mstyle mpadded merror mspace mglyph mfrac msqrt mroot msub msup msubsup "
    "munder mover munderover mmultiscripts none mprescripts mtable mtr mlabeledtr mtd "
    "menclose mphantom mfenced semantics maction maligngroup malignmark annotation".split()
)
NOT_WRITTEN = frozenset({"id", "style", "title", "href", "src"})
NO_WIDTH = re.compile(r"0(\.0*)?[a-z]*")


def write(math: ET.Element) -> str:
    return ET.tostring(_copy(math), encoding="unicode", short_empty_elements=False)


def _copy(node: ET.Element, styles: tuple[str, ...] = ()) -> ET.Element:
    if node.tag not in ELEMENTS:
        out = ET.Element("mrow")
        out.extend([_copy(child) for child in node])
        return out
    attributes = {
        name: value for name, value in node.attrib.items()
        if not name.startswith(("data-", "on", "xmlns"))
        and ":" not in name and name not in NOT_WRITTEN
    }
    text = node.text
    variant = node.get("mathvariant")
    if node.tag in TOKENS and variant in ALPHABETS and text:
        text = "".join(_letter(character, ALPHABETS[variant]) for character in text)
        del attributes["mathvariant"]
    if styles:
        attributes["style"] = ";".join(styles)
    out = ET.Element(node.tag, attributes)
    out.text = text if node.tag in TOKENS or (text and text.strip()) else None
    if node.tag != "mtable":
        out.extend([_copy(child) for child in node])
        return out
    aligns = node.get("columnalign", "center").split() or ["center"]
    gaps = node.get("columnspacing", "").split()
    tight = bool(gaps) and all(NO_WIDTH.fullmatch(gap) for gap in gaps)
    for row in node:
        line = ET.SubElement(out, "mtr")
        cells = list(row)
        if row.tag == "mlabeledtr":
            cells = cells[1:] + cells[:1]   # an equation's number comes after it
        for index, cell in enumerate(cells):
            css = (f"text-align:{aligns[min(index, len(aligns) - 1)]}",)
            if tight:
                css += ("padding-left:0", "padding-right:0")
            line.append(_copy(cell, css))
    return out


def _letter(character: str, alphabet: str) -> str:
    """``character`` in a mathematical alphabet, or itself where Unicode has none."""
    name = unicodedata.name(character, "")
    for prefix, short in (
        ("LATIN CAPITAL LETTER ", "CAPITAL "), ("LATIN SMALL LETTER ", "SMALL "),
        ("GREEK CAPITAL LETTER ", "CAPITAL "), ("GREEK SMALL LETTER ", "SMALL "),
        ("DIGIT ", "DIGIT "),
    ):
        if name.startswith(prefix):
            letter = short + name[len(prefix):]
            break
    else:
        return character
    names = [f"MATHEMATICAL {alphabet} {letter}"]
    # The letters Unicode had before it had the alphabets keep their old names.
    if alphabet in ("SCRIPT", "DOUBLE-STRUCK"):
        names.append(f"{alphabet} {letter}")
    elif alphabet == "FRAKTUR":
        names.append(f"BLACK-LETTER {letter}")
    elif alphabet == "ITALIC" and letter == "SMALL H":
        names.append("PLANCK CONSTANT")
    for candidate in names:
        try:
            return unicodedata.lookup(candidate)
        except KeyError:
            continue
    return character
