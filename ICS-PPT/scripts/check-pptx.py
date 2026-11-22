#!/usr/bin/env python3
"""Whether a PowerPoint export says what the web deck says.

    scripts/check-pptx.py build/X-viewer/slides.html build/X-pptx/X.pptx

Takes what is visible on each slide of both files, in reading order, and
prints where they differ; silent when they agree. Exit status 1 for a
difference, 2 for bad usage. Only the two outputs are read: nothing here
shares code with the parser or with a renderer.

* Text is compared without its white space, list markers included.
* A formula is compared as the sequence of its symbols: scripts below before
  above, a root's sign before its content.
* A picture is compared by its file's name, a listing line by line, a table
  cell by cell, with a cell's pictures read after the table.

What differs by design, as docs/usage.md lists it, is stated once here
(``INTERFACE``, ``HELD``, ``match_controls``) and applies to every slide:

* a run button and a reset button are interface, and no content;
* a block held back on an animation's frame is no content of that frame;
* a control is label = value, and the PPTX adds its range in a parenthesis.
"""

from __future__ import annotations

import re
import sys
import unicodedata
from pathlib import Path, PurePosixPath
from urllib.parse import unquote
from zipfile import BadZipFile

from lxml import etree
from lxml import html as lxml_html
from pptx import Presentation
from pptx.exc import PackageNotFoundError

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from lecturekit import tokens  # noqa: E402  (the theme names the code face)

INTERFACE = {"lk-demo-run", "lk-control-reset"}
HELD = "lk-held"

A = "{http://schemas.openxmlformats.org/drawingml/2006/main}"
M = "{http://schemas.openxmlformats.org/officeDocument/2006/math}"
P = "{http://schemas.openxmlformats.org/presentationml/2006/main}"
MC = "{http://schemas.openxmlformats.org/markup-compatibility/2006}"

UNSEEN = re.compile("[\\s\\u00ad\\u200b\\u200c\\u2060-\\u2064\\ufe0e\\ufe0f\\ufeff]+")


def squeeze(text: str) -> str:
    return UNSEEN.sub("", text)


def picture(src: str) -> str:
    stem = PurePosixPath(unquote(src.split("?")[0].split("#")[0])).stem
    return f"⟦image:{stem}⟧"


def listing(sections: list[str]) -> str:
    """Code, the blank lines at either end of each part left out."""
    lines: list[str] = []
    for section in sections:
        part = [line.replace("\u00a0", " ").rstrip() for line in section.split("\n")]
        while part and not part[0]:
            part.pop(0)
        while part and not part[-1]:
            part.pop()
        lines.extend(part)
    return "⟦code:%s⟧" % "↵".join(lines)


def table(rows: list[list[str]]) -> str:
    across = max((len(row) for row in rows), default=0)
    body = "↵".join("│".join(row + [""] * (across - len(row))) for row in rows)
    return f"⟦table:{body}⟧"


# --- the symbols of a formula ---

# One spelling for what the two sides draw with different characters.
SAME = {
    **dict.fromkeys("–—―¯‾ˉ\u0304\u0305\u0332", "¯"),
    **dict.fromkeys("^ˆ\u0302", "^"),
    **dict.fromkeys("~˜\u0303", "~"),
    **dict.fromkeys("˙\u0307", "˙"),
    **dict.fromkeys("˘\u0306", "˘"),
    **dict.fromkeys("ˇ\u030c", "ˇ"),
    **dict.fromkeys("¨\u0308", "¨"),
    **dict.fromkeys("´ˊ\u0301", "´"),
    **dict.fromkeys("`ˋ\u0300", "`"),
    **dict.fromkeys("˚\u030a", "˚"),
    "\u20d7": "→", "\u20d6": "←", "\u20e1": "↔",
    "∣": "|", "∥": "‖", "⏞": "⏟",
}
# The pieces a stretched character is built of.
PIECES = {
    **dict.fromkeys(range(0xE150, 0xE158), "⏟"),
    **dict.fromkeys(range(0x239B, 0x239E), "("), **dict.fromkeys(range(0x239E, 0x23A1), ")"),
    **dict.fromkeys(range(0x23A1, 0x23A4), "["), **dict.fromkeys(range(0x23A4, 0x23A7), "]"),
    **dict.fromkeys(range(0x23A7, 0x23AA), "{"), **dict.fromkeys(range(0x23AB, 0x23AE), "}"),
    0x23AA: "", 0x23D0: "|",
}
SPELLINGS = set(SAME.values())
NUMBER = re.compile(r"[-−]?\d+(?:\.\d+)?")
STRETCHED = set("¯⏟()[]{}|‖")
ARROWS = {chr(code) for code in range(0x2190, 0x2200)}
ARROW_EXTENDERS = set("−=¯")


def symbols(text: str) -> str:
    text = "".join(SAME.get(char, char) for char in text)
    text = "".join(
        char if char in SPELLINGS else unicodedata.normalize("NFKC", char) for char in text
    )
    return squeeze(text)


def drawn_operator(codes: list[int]) -> str:
    """One <mo> of the deck's SVG: a stretched character is one character."""
    chars = [PIECES[code] if code in PIECES else symbols(chr(code)) for code in codes]
    kinds = {char for char in chars if char}
    arrows = kinds & ARROWS
    if arrows and len(chars) > 1 and kinds <= arrows | ARROW_EXTENDERS:
        return "↔" if {"←", "→"} <= arrows else "".join(sorted(arrows))
    if len(kinds) == 1 and kinds <= STRETCHED:
        return kinds.pop()
    return "".join(chars)


def height_of(node) -> float:
    found = re.search(r"translate\(\s*[-\d.e]+[ ,]+([-\d.e]+)", node.get("transform", ""))
    return float(found[1]) if found else 0.0


def is_script(node) -> bool:
    return "scale(" in node.get("transform", "")


def mml_children(node) -> list:
    out = []
    for child in node:
        if isinstance(child.tag, str):
            out.extend([child] if child.get("data-mml-node") else mml_children(child))
    return out


def drawn_formula(node) -> str:
    """The symbols MathJax drew, from the SVG's own record of them."""
    kind = node.get("data-mml-node")
    parts = mml_children(node)
    if kind in ("msqrt", "mroot"):
        *inner, _surd = parts
        if kind == "mroot":
            inner.reverse()
        return "√" + "".join(drawn_formula(part) for part in inner)
    if kind in ("msubsup", "munderover") and len(parts) == 3:
        base, *scripts = parts
        parts = [base, *sorted(scripts, key=height_of)]
    elif kind == "mmultiscripts":
        ordered, run = [], []
        for part in parts:
            if is_script(part):
                run.append(part)
            else:
                ordered += sorted(run, key=height_of) + [part]
                run = []
        parts = ordered + sorted(run, key=height_of)
    if parts:
        return "".join(drawn_formula(part) for part in parts)
    codes = []
    for item in node.iter():
        if item.get("data-c"):
            codes.append(int(item.get("data-c"), 16))
        elif item.tag == "text":
            codes.extend(ord(char) for char in item.text or "")
    if kind == "mo":
        return drawn_operator(codes)
    return symbols("".join(map(chr, codes)))


def omml_formula(node) -> str:
    """The symbols of an OMML equation, in the same order."""
    tag = etree.QName(node).localname
    if tag == "t":
        return symbols(node.text or "")
    if tag.endswith("Pr"):
        return ""
    inner = [omml_formula(child) for child in node if child.tag.startswith(M)]

    def value(name: str, default: str) -> str:
        found = node.find(f"{M}{tag}Pr/{M}{name}")
        return default if found is None else found.get(f"{M}val", "")

    if tag == "phant" and value("show", "1") == "0":
        return ""
    if tag == "nary":
        return symbols(value("chr", "∫")) + "".join(inner)
    if tag == "d":
        bodies = [omml_formula(child) for child in node.findall(f"{M}e")]
        between = symbols(value("sepChr", "|"))
        return symbols(value("begChr", "(")) + between.join(bodies) + symbols(value("endChr", ")"))
    if tag == "acc":
        return "".join(inner) + symbols(value("chr", "\u0302"))
    if tag == "groupChr":
        return "".join(inner) + symbols(value("chr", "⏟"))
    if tag == "bar":
        return "".join(inner) + "¯"
    if tag == "rad":
        return "√" + "".join(inner)
    return "".join(inner)


# --- the web deck ---


class Web:
    """What one <section> shows."""

    def __init__(self) -> None:
        self.out: list[str] = []
        self.controls: list[tuple[str, str, list[str]]] = []
        self.after_table: list[str] | None = None

    def shown(self, node) -> bool:
        if not isinstance(node.tag, str) or node.tag in ("script", "style", "template"):
            return False
        classes = set(node.get("class", "").split())
        style = node.get("style", "").replace(" ", "")
        return not (classes & INTERFACE or HELD in classes
                    or node.get("hidden") is not None or "display:none" in style)

    def content(self, node) -> None:
        self.out.append(squeeze(node.text or ""))
        for child in node:
            if self.shown(child):
                self.element(child)
            self.out.append(squeeze(child.tail or ""))

    def element(self, node) -> None:
        tag, classes = node.tag, node.get("class", "").split()
        if tag == "mjx-container":
            math = node.xpath(".//*[@data-mml-node='math']")
            self.out.append(f"⟦math:{''.join(drawn_formula(one) for one in math)}⟧")
        elif tag == "img":
            if "emoji" in classes:
                self.out.append(squeeze(node.get("alt", "")))
            elif self.after_table is not None:
                self.after_table.append(picture(node.get("src", "")))
            else:
                self.out.append(picture(node.get("src", "")))
        elif tag == "pre":
            self.out.append(listing(self.pre(node)))
        elif tag == "table":
            self.table(node)
        elif "lk-control" in classes:
            self.control(node)
        elif tag == "input":
            pass
        else:
            parent = node.getparent()
            if tag == "li" and parent.tag == "ol":
                items = [child for child in parent if child.tag == "li"]
                self.out.append(f"{int(parent.get('start', '1')) + items.index(node)}.")
            elif tag == "li":
                self.out.append("•")
            self.content(node)

    def pre(self, node) -> list[str]:
        sections = [""]

        def walk(part) -> None:
            sections[-1] += part.text or ""
            for child in part:
                if self.shown(child):
                    if child.tag == "code" and part is node and sections[-1]:
                        sections.append("")
                    if child.tag == "br":
                        sections[-1] += "\n"
                    walk(child)
                sections[-1] += child.tail or ""

        walk(node)
        return sections

    def table(self, node) -> None:
        saved, outer = self.out, self.after_table
        pictures = [] if outer is None else outer
        self.after_table = pictures
        rows = []
        for row in node.xpath("./tr | ./thead/tr | ./tbody/tr | ./tfoot/tr"):
            cells = []
            for cell in row:
                if self.shown(cell):
                    self.out = []
                    self.content(cell)
                    cells.append("".join(self.out))
            rows.append(cells)
        self.out, self.after_table = saved, outer
        self.out.append(table(rows))
        if outer is None:
            self.out.extend(pictures)

    def control(self, node) -> None:
        label = "".join(n.text_content() for n in node.find_class("lk-control-label"))
        boxes = node.find_class("lk-control-value")
        value = boxes[0].get("value", "") if boxes else ""
        ends = node.find_class("lk-slider-mark")
        numbers = [squeeze(mark.text_content()) for mark in ends]
        for span in node.find_class("lk-control-range"):
            numbers += NUMBER.findall(span.text_content())
        self.controls.append((squeeze(label), squeeze(value), numbers))
        self.out.append(f"{squeeze(label)}={squeeze(value)}")
        for marker in node.find_class("footnote-ref"):
            self.out.append(squeeze(marker.text_content()))


def web_slides(path: Path) -> list[tuple[str, list]]:
    document = lxml_html.document_fromstring(path.read_text(encoding="utf-8"))
    out = []
    for svg in document.xpath("//svg[@data-marpit-svg]"):
        reader = Web()
        for section in svg.xpath("./foreignobject/section"):
            reader.content(section)
        out.append(("".join(reader.out), reader.controls))
    return out


# --- the PowerPoint file ---


def paragraph_text(paragraph) -> str:
    out = []
    for node in paragraph:
        if node.tag in (f"{A}r", f"{A}fld"):
            out.append(squeeze(node.findtext(f"{A}t") or ""))
        elif node.tag == f"{MC}AlternateContent":
            choice = node.find(f"{MC}Choice")
            math = [] if choice is None else choice.iter(f"{M}oMath")
            out.extend(f"⟦math:{omml_formula(one)}⟧" for one in math)
    return "".join(out)


def is_listing(shape, mono: str) -> bool:
    """A filled text box set in the code face."""
    if shape.find(f"{P}nvSpPr/{P}cNvSpPr[@txBox='1']") is None:
        return False
    if shape.find(f"{P}spPr/{A}solidFill") is None:
        return False
    runs = shape.findall(f".//{A}r")
    return bool(runs) and all(
        (face := run.find(f"{A}rPr/{A}latin")) is not None and face.get("typeface") == mono
        for run in runs
    )


def listing_lines(shape) -> str:
    lines = []
    for paragraph in shape.iter(f"{A}p"):
        parts = []
        for node in paragraph:
            if node.tag == f"{A}br":
                parts.append("\n")
            elif node.tag == f"{A}r":
                parts.append(node.findtext(f"{A}t") or "")
        lines.append("".join(parts))
    return "\n".join(lines)


def pptx_slides(path: Path) -> list[str]:
    mono = tokens.face("--pptx-font-mono")
    out = []
    for slide in Presentation(str(path)).slides:
        parts = []
        for shape in slide.shapes:          # the order of the shape tree
            node = shape._element
            if node.tag == f"{P}pic":
                parts.append(picture(node.find(f".//{P}cNvPr").get("descr", "")))
            elif node.tag == f"{P}graphicFrame":
                parts.append(table([
                    ["".join(paragraph_text(p) for p in cell.iter(f"{A}p"))
                     for cell in row.findall(f"{A}tc")]
                    for row in node.iter(f"{A}tr")
                ]))
            elif is_listing(node, mono):
                parts.append(listing([listing_lines(node)]))
            else:
                parts.extend(paragraph_text(p) for p in node.iter(f"{A}p"))
        out.append("".join(parts))
    return out


# --- the comparison ---


def match_controls(shown: str, controls: list[tuple[str, str, list[str]]]) -> str:
    """The PPTX's line for each control, spelled as the deck's label = value."""
    for label, value, numbers in controls:
        name = re.escape(label)
        pattern = (rf"(?:⟦math:{re.escape(symbols(label))}⟧|{name})={re.escape(value)}"
                   r"[（(](?P<range>[^）)⟦]*)[）)]")

        def spell(found: re.Match) -> str:
            complete = set(numbers) <= set(NUMBER.findall(found["range"]))
            return f"{label}={value}" if complete else found[0]

        shown = re.sub(pattern, spell, shown, count=1)
    return shown


def first_difference(web: str, pptx: str) -> tuple[str, str]:
    at = 0
    while at < min(len(web), len(pptx)) and web[at] == pptx[at]:
        at += 1
    start = max(0, at - 30)

    def around(text: str) -> str:
        return ("…" if start else "") + text[start:at + 50] + ("…" if at + 50 < len(text) else "")

    return around(web), around(pptx)


def differences(html_path: Path, pptx_path: Path) -> list[str]:
    web, pptx = web_slides(html_path), pptx_slides(pptx_path)
    out = []
    if len(web) != len(pptx):
        out.append(f"{pptx_path}: {len(web)} slides on the web, {len(pptx)} in the PPTX")
    for number, ((wanted, controls), shown) in enumerate(zip(web, pptx), 1):
        shown = match_controls(shown, controls)
        if shown != wanted:
            on_web, in_pptx = first_difference(wanted, shown)
            out.append(f"{pptx_path}: slide {number}\n  web:  {on_web}\n  pptx: {in_pptx}")
    return out


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: check-pptx.py slides.html deck.pptx", file=sys.stderr)
        return 2
    try:
        found = differences(Path(argv[0]), Path(argv[1]))
    except (OSError, BadZipFile, PackageNotFoundError) as problem:
        print(f"check-pptx.py: {problem}", file=sys.stderr)
        return 2
    for line in found:
        print(line)
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
