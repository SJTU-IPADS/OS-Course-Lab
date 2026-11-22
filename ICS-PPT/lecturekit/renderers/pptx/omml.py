"""MathML into OMML, the equation markup PowerPoint edits natively.

One function for each MathML element, found by the element's tag (``DRAW``).
What stretches, what is an accent and what carries limits is read from the
MathML (``stretchy``, ``accent``, ``largeop``, the atom's class), which
MathJax has already worked out; no formula language is known here. The
fallback text and the height estimate come from the same tree.
"""

from __future__ import annotations

import re
import unicodedata
from collections.abc import Callable
from contextlib import contextmanager
from xml.etree.ElementTree import Element

from lxml import etree

M = "http://schemas.openxmlformats.org/officeDocument/2006/math"
A = "http://schemas.openxmlformats.org/drawingml/2006/main"
A14 = "http://schemas.microsoft.com/office/drawing/2010/main"
MC = "http://schemas.openxmlformats.org/markup-compatibility/2006"
XML_SPACE = "{http://www.w3.org/XML/1998/namespace}space"

# An empty required slot shows as a dotted box in PowerPoint; this fills one unseen.
EMPTY = "\u200b"
INVISIBLE = frozenset("\u2061\u2062\u2063\u2064")
# The delimiters that may divide the inside of a pair of fences.
SEPARATORS = frozenset("|∣‖∥/\\")
LINES = frozenset("―‾¯ˉ_\u0332")
# An accent as MathML writes it (a spacing character), as OMML does (combining).
ACCENTS = {
    "^": "\u0302", "ˆ": "\u0302", "~": "\u0303", "˜": "\u0303",
    "¯": "\u0304", "ˉ": "\u0304", "˘": "\u0306", "˙": "\u0307",
    ".": "\u0307", "¨": "\u0308", "˚": "\u030a", "ˇ": "\u030c",
    "`": "\u0300", "ˋ": "\u0300", "´": "\u0301", "ˊ": "\u0301",
    "→": "\u20d7", "←": "\u20d6", "↔": "\u20e1",
}
SCRIPTS = {"double-struck": "double-struck", "script": "script", "fraktur": "fraktur",
           "sans": "sans-serif", "monospace": "monospace"}
# The colour names CSS and DrawingML's presets share.
COLOR_NAMES = frozenset(
    "black silver gray white maroon red purple fuchsia green lime olive yellow "
    "navy blue teal aqua orange pink brown cyan magenta violet gold".split()
)
NAMED_WIDTHS = {"thinmathspace": 1 / 6, "mediummathspace": 2 / 9, "thickmathspace": 5 / 18}
SPACES = ((0.4, "\u2002"), (0.3, "\u2004"), (0.2, "\u2005"), (0.01, "\u2009"))


# --- the public functions ---


def to_omml(math: Element, *, size_pt: float, color: str, display: bool = False):
    """``math`` as a child of a DrawingML paragraph.

    PowerPoint reads the equation in ``mc:Choice``. A reader without the 2010
    math extension shows the text in ``mc:Fallback``, so none loses the formula.
    """
    writer = _Writer(size_pt, color)
    body = _m("oMath", *writer.node(math))
    if display:
        body = _m("oMathPara", _m("oMathParaPr", _m("jc", val="center")), body)
    root = etree.Element(f"{{{MC}}}AlternateContent",
                         nsmap={"mc": MC, "a": A, "a14": A14, "m": M})
    choice = etree.SubElement(root, f"{{{MC}}}Choice")
    choice.set("Requires", "a14")
    etree.SubElement(choice, f"{{{A14}}}m").append(body)
    run = etree.SubElement(etree.SubElement(root, f"{{{MC}}}Fallback"), f"{{{A}}}r")
    run.append(writer.run_properties())
    etree.SubElement(run, f"{{{A}}}t").text = plain_text(math)
    return root


def plain_text(math: Element) -> str:
    """The formula on one line of text: the fallback, and what width is measured on."""
    def group(node: Element) -> str:
        text = plain_text(node)
        return f"({text})" if len(text) > 1 and not text.isalnum() else text

    tag, parts = math.tag, list(math)
    if tag in TOKENS:
        return "".join(" " if c == "\u2061" else c
                       for c in _text(math) if c == "\u2061" or c not in INVISIBLE)
    if tag == "mphantom" or tag in EMPTY_ELEMENTS:
        return ""
    if tag == "mspace":
        return _space(math)
    if tag in ("semantics", "maction"):
        return plain_text(parts[0]) if parts else ""
    if tag == "mfrac":
        return f"{group(parts[0])}/{group(parts[1])}"
    if tag == "msqrt":
        return "√(%s)" % "".join(plain_text(part) for part in parts)
    if tag == "mroot":
        return f"{plain_text(parts[1])}√({plain_text(parts[0])})"
    if tag in LIMITS:
        out = plain_text(parts[0])
        for sign, script in zip(LIMITS[tag], parts[1:]):
            mark = _single(script)
            flag = mark.tag == "mo" and mark.get("accent") == "true"
            out += _text(mark) if flag else f"{sign}{group(script)}"
        return out
    if tag == "mtable":
        return "; ".join(plain_text(row) for row in parts)
    if tag in ("mtr", "mlabeledtr"):
        return " ".join(plain_text(cell) for cell in _cells(math))
    return "".join(plain_text(part) for part in parts)


def visual_lines(math: Element) -> float:
    """About how many lines of text tall the typeset formula is."""
    tag, parts = math.tag, list(math)
    if not parts:
        return 1.4 if math.get("largeop") == "true" else 1.0
    if tag == "mfrac":
        return sum(visual_lines(part) for part in parts) + 0.2
    if tag == "mtable":
        return sum(visual_lines(row) for row in parts)
    tallest = max(visual_lines(part) for part in parts)
    if tag in ("msqrt", "mroot"):
        return tallest + 0.2
    if tag in LIMITS:
        each = 0.2 if tag.startswith("ms") else 0.5
        return visual_lines(parts[0]) + each * (len(parts) - 1)
    return tallest


# --- reading MathML ---

TOKENS = frozenset({"mi", "mn", "mo", "mtext", "ms"})
EMPTY_ELEMENTS = frozenset({
    "none", "mprescripts", "maligngroup", "malignmark", "annotation", "annotation-xml",
})
# The elements that hang scripts on a base, and the sign of each script in text.
LIMITS = {"msub": "_", "msup": "^", "msubsup": "_^",
          "munder": "_", "mover": "^", "munderover": "_^"}


def _text(node: Element) -> str:
    return "".join(node.itertext())


def _single(node: Element) -> Element:
    """The one element a chain of wrappers holds."""
    while node.tag in ("mrow", "mstyle", "math") and len(node) == 1:
        node = node[0]
    return node


def _class(node: Element) -> str | None:
    return node.get("data-mjx-texclass") or _single(node).get("data-mjx-texclass")


def _fence(node: Element, side: str) -> bool:
    """A delimiter that grows with what it encloses, on ``side`` (OPEN or CLOSE)."""
    mark = _single(node)
    return (mark.tag == "mo" and mark.get("data-mjx-texclass") == side
            and (mark.get("stretchy") == "true" or mark.get("minsize") is not None))


def _pairs(nodes: list[Element]) -> dict[int, int]:
    """Where each growing delimiter of a row closes."""
    waiting, pairs = [], {}
    for index, node in enumerate(nodes):
        if _fence(node, "OPEN"):
            waiting.append(index)
        elif _fence(node, "CLOSE") and waiting:
            pairs[waiting.pop()] = index
    return pairs


def _separator(node: Element) -> str:
    mark = _single(node)
    text = _text(mark) if mark.tag == "mo" and mark.get("stretchy") == "true" else ""
    return text if text in SEPARATORS else ""


def _large_operator(node: Element) -> tuple[Element, Element | None, Element | None, str] | None:
    """``(operator, lower, upper, limit location)`` when ``node`` is an n-ary operator."""
    node = _single(node)
    base, lower, upper = node, None, None
    if node.tag in LIMITS:
        base, *scripts = node
        signs = dict(zip(LIMITS[node.tag], scripts))
        lower, upper = signs.get("_"), signs.get("^")
    operator = _single(base)
    if operator.tag != "mo" or operator.get("largeop") != "true":
        return None
    location = "undOvr" if node.tag in ("munder", "mover", "munderover") else "subSup"
    return operator, lower, upper, location


def _operand_end(nodes: list[Element], pairs: dict[int, int], start: int) -> int:
    """An n-ary operator's operand runs to the next relation outside brackets."""
    index, depth = start, 0
    while index < len(nodes):
        if index in pairs:
            index = pairs[index] + 1
            continue
        kind = _class(nodes[index])
        if depth == 0 and _ends_operand(nodes[index], kind):
            break
        depth += (kind == "OPEN") - (kind == "CLOSE")
        index += 1
    return index


def _ends_operand(node: Element, kind: str | None) -> bool:
    """What an operand cannot contain unless brackets enclose it."""
    if kind in ("REL", "PUNCT", "CLOSE") or _separator(node):
        return True
    mark = _single(node)
    return mark.tag == "mspace" and _width(mark.get("width")) >= 1


def _named_operator(node: Element) -> bool:
    """An operator written as a word, which is spaced from its argument."""
    node = _single(node)
    if node.tag in LIMITS:
        node = _single(node[0])
    word = "".join(_text(node).split())
    if len(word) < 2 or not word.isalpha():
        return False
    if node.tag == "mi":
        return node.get("data-mjx-auto-op") != "false" and node.get("mathvariant") is None
    return node.get("data-mjx-texclass") == "OP"


def _needs_space(neighbour: Element, *spaced: str) -> bool:
    """Whether an operator written as a word needs a thin space beside ``neighbour``:
    not beside a space, or an atom of a class that OMML spaces by itself."""
    return bool(_text(neighbour).strip()) and _class(neighbour) not in spaced


def _width(value: str | None) -> float:
    """A MathML width in em."""
    if value in NAMED_WIDTHS:
        return NAMED_WIDTHS[value]
    found = re.fullmatch(r"([+-]?[\d.]+)(em|ex|pt|px)", (value or "").strip())
    if found is None:
        return 0.0
    return float(found[1]) / {"em": 1, "ex": 2, "pt": 10, "px": 16}[found[2]]


def _space(node: Element) -> str:
    width = _width(node.get("width"))
    if width >= 0.9:
        return "\u2003" * round(width)
    return next((space for least, space in SPACES if width >= least), "")


def _cells(row: Element) -> list[Element]:
    """A row's cells as shown: an equation's number comes after it."""
    cells = list(row)
    return cells[1:] + cells[:1] if row.tag == "mlabeledtr" else cells


# --- writing OMML ---


def _m(tag: str, *children, val: str | None = None):
    node = etree.Element(f"{{{M}}}{tag}", nsmap={"m": M})
    if val is not None:
        node.set(f"{{{M}}}val", val)
    node.extend([child for child in children if child is not None])
    return node


class _Writer:
    """The state a conversion carries: the size and the colour in force."""

    def __init__(self, size_pt: float, color: str) -> None:
        self.size = str(round(size_pt * 100))
        self.color = ("srgbClr", color)

    def run_properties(self, *, bold: bool = False, italic: bool = False):
        node = etree.Element(f"{{{A}}}rPr", nsmap={"a": A})
        node.set("lang", "en-US")
        node.set("sz", self.size)
        if bold:
            node.set("b", "1")
        if italic:
            node.set("i", "1")
        fill = etree.SubElement(node, f"{{{A}}}solidFill")
        kind, value = self.color
        etree.SubElement(fill, f"{{{A}}}{kind}").set("val", value)
        return node

    def properties(self, name: str, *children):
        """A structure's properties, which also size and colour what it draws itself."""
        return _m(name, *children, _m("ctrlPr", self.run_properties()))

    @contextmanager
    def colored(self, value: str | None):
        saved = self.color
        if value and re.fullmatch("#[0-9A-Fa-f]{6}", value):
            self.color = ("srgbClr", value[1:].upper())
        elif value and value.lower() in COLOR_NAMES:
            self.color = ("prstClr", value.lower())
        try:
            yield
        finally:
            self.color = saved

    def node(self, node: Element) -> list:
        try:
            draw = DRAW[node.tag]
        except KeyError:
            raise ValueError(f"MathML element without an OMML form: <{node.tag}>") from None
        with self.colored(node.get("mathcolor")):
            return draw(self, node)

    def slot(self, name: str, content: list):
        return _m(name, *(content or self.run(EMPTY)))

    def of(self, node: Element | None) -> list:
        return [] if node is None else self.node(node)

    def run(self, text: str, *, style: str = "p", script: str | None = None,
            normal: bool = False) -> list:
        if not text:
            return []
        if normal:
            properties = _m("rPr", _m("nor"))
        else:
            properties = _m("rPr", script and _m("scr", val=script), _m("sty", val=style))
        content = _m("t")
        content.text = text
        content.set(XML_SPACE, "preserve")
        italic = not normal and style in ("i", "bi")
        bold = not normal and style in ("b", "bi")
        return [_m("r", properties, self.run_properties(bold=bold, italic=italic), content)]

    def row(self, nodes: list[Element]) -> list:
        """Siblings: fences pair up, and an n-ary operator takes its operand."""
        pairs = _pairs(nodes)
        out: list = []
        index = 0
        while index < len(nodes):
            node = nodes[index]
            if index in pairs:
                end = pairs[index]
                out += self.fenced(node, nodes[index + 1:end], nodes[end])
                index = end + 1
                continue
            operator = _large_operator(node)
            if operator:
                end = _operand_end(nodes, pairs, index + 1)
                out += self.nary(*operator, nodes[index + 1:end])
                index = end
                continue
            named = _named_operator(node)
            if named and index and _needs_space(nodes[index - 1], "OPEN", "REL", "BIN"):
                out += self.run("\u2009")
            out += self.node(node)
            index += 1
            if named:
                while index < len(nodes) and _text(nodes[index]) == "\u2061":
                    index += 1
                if index < len(nodes) and _needs_space(
                        nodes[index], "OPEN", "REL", "BIN", "CLOSE", "PUNCT"):
                    out += self.run("\u2009")
        return out

    def fenced(self, opening: Element, inside: list[Element], closing: Element) -> list:
        dividers = [i for i, node in enumerate(inside) if _separator(node)]
        characters = {_separator(inside[i]) for i in dividers}
        divider = characters.pop() if len(characters) == 1 else None
        if divider is None:
            parts = [inside]
        else:
            starts = [0, *(i + 1 for i in dividers)]
            parts = [inside[a:b] for a, b in zip(starts, [*dividers, len(inside)])]
        properties = self.properties(
            "dPr",
            _m("begChr", val=_text(_single(opening))),
            None if divider is None else _m("sepChr", val=divider),
            _m("endChr", val=_text(_single(closing))),
        )
        return [_m("d", properties, *(self.slot("e", self.row(part)) for part in parts))]

    def nary(self, operator: Element, lower: Element | None, upper: Element | None,
             location: str, operand: list[Element]) -> list:
        properties = self.properties(
            "naryPr",
            _m("chr", val=_text(operator)),
            _m("limLoc", val=location),
            _m("subHide", val="1") if lower is None else None,
            _m("supHide", val="1") if upper is None else None,
        )
        return [_m("nary", properties,
                   _m("sub") if lower is None else self.slot("sub", self.node(lower)),
                   _m("sup") if upper is None else self.slot("sup", self.node(upper)),
                   self.slot("e", self.row(operand)))]

    def scripts(self, base: list, lower: Element | None, upper: Element | None) -> list:
        if lower is not None and upper is not None:
            return [_m("sSubSup", self.properties("sSubSupPr"), self.slot("e", base),
                       self.slot("sub", self.node(lower)), self.slot("sup", self.node(upper)))]
        if lower is not None:
            return [_m("sSub", self.properties("sSubPr"), self.slot("e", base),
                       self.slot("sub", self.node(lower)))]
        if upper is not None:
            return [_m("sSup", self.properties("sSupPr"), self.slot("e", base),
                       self.slot("sup", self.node(upper)))]
        return base

    def limit(self, base: list, script: Element, *, over: bool) -> list:
        """What stands straight over or under ``base``."""
        mark = _single(script)
        stretchy = mark.tag == "mo" and mark.get("stretchy") == "true"
        accent = mark.tag == "mo" and mark.get("accent") == "true"
        character = _text(mark)
        position = _m("pos", val="top" if over else "bot")
        with self.colored(mark.get("mathcolor")):
            if stretchy and character in LINES:
                return [_m("bar", self.properties("barPr", position), self.slot("e", base))]
            combining = ACCENTS.get(character)
            if len(character) == 1 and unicodedata.combining(character):
                combining = character
            if over and accent and combining and (not stretchy or character in "^~ˆ˜"):
                properties = self.properties("accPr", _m("chr", val=combining))
                return [_m("acc", properties, self.slot("e", base))]
            if stretchy and character:
                properties = self.properties(
                    "groupChrPr", _m("chr", val=character), position,
                    _m("vertJc", val="bot" if over else "top"))
                return [_m("groupChr", properties, self.slot("e", base))]
        name = "limUpp" if over else "limLow"
        return [_m(name, self.properties(f"{name}Pr"), self.slot("e", base),
                   self.slot("lim", self.node(script)))]


# --- one function for each element ---


def _style_of(node: Element) -> tuple[str, str | None]:
    """The style and script of a token: an identifier of one letter is italic."""
    variant = node.get("mathvariant")
    if variant is None:
        return ("i" if node.tag == "mi" and len(_text(node)) == 1 else "p"), None
    bold, italic = "bold" in variant, "italic" in variant
    style = "bi" if bold and italic else "b" if bold else "i" if italic else "p"
    return style, next((value for key, value in SCRIPTS.items() if key in variant), None)


def token(w: _Writer, node: Element) -> list:
    style, script = _style_of(node)
    return w.run("".join(c for c in _text(node) if c not in INVISIBLE),
                 style=style, script=script)


def mtext(w: _Writer, node: Element) -> list:
    if node.get("mathvariant") not in (None, "normal"):
        return token(w, node)
    return w.run(_text(node), normal=True)


def mspace(w: _Writer, node: Element) -> list:
    return w.run(_space(node))


def mrow(w: _Writer, node: Element) -> list:
    return w.row(list(node))


def merror(w: _Writer, node: Element) -> list:
    with w.colored("red"):
        return w.row(list(node))


def first_child(w: _Writer, node: Element) -> list:
    return w.node(node[0]) if len(node) else []


def nothing(w: _Writer, node: Element) -> list:
    return []


def mglyph(w: _Writer, node: Element) -> list:
    return w.run(node.get("alt", ""), normal=True)


def mfrac(w: _Writer, node: Element) -> list:
    kind = None
    if node.get("bevelled") == "true":
        kind = _m("type", val="skw")
    elif re.match(r"0(\.0*)?([a-z%]*)$", node.get("linethickness", "")):
        kind = _m("type", val="noBar")
    top, bottom = node
    return [_m("f", w.properties("fPr", kind),
               w.slot("num", w.node(top)), w.slot("den", w.node(bottom)))]


def msqrt(w: _Writer, node: Element) -> list:
    return [_m("rad", w.properties("radPr", _m("degHide", val="1")),
               _m("deg"), w.slot("e", w.row(list(node))))]


def mroot(w: _Writer, node: Element) -> list:
    base, degree = node
    return [_m("rad", w.properties("radPr"),
               w.slot("deg", w.node(degree)), w.slot("e", w.node(base)))]


def scripted(w: _Writer, node: Element) -> list:
    base, *scripts = node
    signs = dict(zip(LIMITS[node.tag], scripts))
    return w.scripts(w.node(base), signs.get("_"), signs.get("^"))


def stacked(w: _Writer, node: Element) -> list:
    base, *scripts = node
    out = w.node(base)
    for sign, script in zip(LIMITS[node.tag], scripts):
        out = w.limit(out, script, over=sign == "^")
    return out


def mmultiscripts(w: _Writer, node: Element) -> list:
    base, *rest = node
    split = next((i for i, part in enumerate(rest) if part.tag == "mprescripts"), len(rest))

    def given(part: Element) -> Element | None:
        return None if part.tag == "none" else part

    after, before = rest[:split], rest[split + 1:]
    out = w.node(base)
    for lower, upper in zip(after[::2], after[1::2]):
        out = w.scripts(out, given(lower), given(upper))
    for lower, upper in zip(before[::2], before[1::2]):
        out = [_m("sPre", w.properties("sPrePr"), w.slot("sub", w.of(given(lower))),
                  w.slot("sup", w.of(given(upper))), w.slot("e", out))]
    return out


def mtable(w: _Writer, node: Element) -> list:
    rows = [_cells(row) if row.tag in ("mtr", "mlabeledtr") else [row] for row in node]
    across = max((len(row) for row in rows), default=0)
    if not across:
        return []
    aligns = node.get("columnalign", "center").split() or ["center"]
    columns = [
        _m("mc", _m("mcPr", _m("count", val="1"),
                    _m("mcJc", val=aligns[min(index, len(aligns) - 1)])))
        for index in range(across)
    ]
    gaps = node.get("columnspacing", "").split()
    tight = bool(gaps) and all(_width(gap) == 0 for gap in gaps)
    properties = w.properties(
        "mPr", _m("plcHide", val="1"),
        _m("cGpRule", val="3") if tight else None,
        _m("cGp", val="0") if tight else None,
        _m("mcs", *columns),
    )
    lines = [
        _m("mr", *(w.slot("e", w.node(row[i]) if i < len(row) else [])
                   for i in range(across)))
        for row in rows
    ]
    return [_m("m", properties, *lines)]


def menclose(w: _Writer, node: Element) -> list:
    notation = set(node.get("notation", "longdiv").split())
    content = w.row(list(node))
    if "radical" in notation:
        return [_m("rad", w.properties("radPr", _m("degHide", val="1")),
                   _m("deg"), w.slot("e", content))]
    strikes = {"horizontalstrike": "strikeH", "verticalstrike": "strikeV",
               "updiagonalstrike": "strikeBLTR", "downdiagonalstrike": "strikeTLBR"}
    drawn = [name for key, name in strikes.items() if key in notation]
    sides = {"top": "hideTop", "bottom": "hideBot", "left": "hideLeft", "right": "hideRight"}
    shown = notation & set(sides)
    boxed = notation & {"box", "roundedbox", "circle"}
    # All four sides for a box and for a notation OMML has no shape for.
    around = boxed or not (shown or drawn)
    hidden = [] if around else [name for key, name in sides.items() if key not in shown]
    properties = w.properties("borderBoxPr",
                              *(_m(name, val="1") for name in [*hidden, *drawn]))
    return [_m("borderBox", properties, w.slot("e", content))]


def mphantom(w: _Writer, node: Element) -> list:
    return [_m("phant", w.properties("phantPr", _m("show", val="0")),
               w.slot("e", w.row(list(node))))]


def mfenced(w: _Writer, node: Element) -> list:
    separators = node.get("separators", ",").strip()
    properties = w.properties(
        "dPr", _m("begChr", val=node.get("open", "(")),
        _m("sepChr", val=separators[:1]), _m("endChr", val=node.get("close", ")")))
    return [_m("d", properties, *(w.slot("e", w.node(part)) for part in node))]


DRAW: dict[str, Callable[[_Writer, Element], list]] = {
    "math": mrow, "mrow": mrow, "mstyle": mrow, "mpadded": mrow, "merror": merror,
    "mi": token, "mn": token, "mo": token, "ms": token, "mtext": mtext,
    "mspace": mspace, "mglyph": mglyph,
    "mfrac": mfrac, "msqrt": msqrt, "mroot": mroot,
    "msub": scripted, "msup": scripted, "msubsup": scripted,
    "munder": stacked, "mover": stacked, "munderover": stacked,
    "mmultiscripts": mmultiscripts,
    "mtable": mtable, "mtr": mrow, "mlabeledtr": mrow, "mtd": mrow,
    "menclose": menclose, "mphantom": mphantom, "mfenced": mfenced,
    "semantics": first_child, "maction": first_child,
    **{tag: nothing for tag in EMPTY_ELEMENTS},
}
