"""LaTeX math → OMML, the equation markup PowerPoint typesets natively.

The viewer hands `$…$` to MathJax and the book hands it to TeX; PowerPoint has
its own equation model (OMML), so the pptx renderer translates into that rather
than shipping a picture. The result is a real PowerPoint equation: it typesets
with proper spacing and stacked limits, and stays editable in the equation
editor.

This covers the vocabulary lecture math actually uses — sub/superscripts,
fractions, roots, big operators with limits, sized delimiters, upright text,
and the usual symbols. What it does not recognize (matrices, alignment,
accents) is emitted as literal text: a formula that reads slightly wrong on
one slide beats an export that fails.
"""

from __future__ import annotations

import re

from lxml import etree

M = "http://schemas.openxmlformats.org/officeDocument/2006/math"
A = "http://schemas.openxmlformats.org/drawingml/2006/main"
A14 = "http://schemas.microsoft.com/office/drawing/2010/main"
MC = "http://schemas.openxmlformats.org/markup-compatibility/2006"

_NS = {"m": M, "a": A, "a14": A14, "mc": MC}

# Commands that set the following group upright (a function name or a word),
# rather than the italic that math runs default to.
_UPRIGHT = ("\\mathrm", "\\operatorname", "\\text", "\\textrm", "\\mathsf",
            "\\mathbf", "\\mathtt")
# Blackboard bold: \mathbb{R} -> ℝ, via OMML's own double-struck script.
_SCRIPTS = {"\\mathbb": "double-struck", "\\mathcal": "script",
            "\\mathfrak": "fraktur"}

# Big operators: the glyph, and whether limits stack above/below (as in a sum)
# or ride the corner (as in an integral).
_NARY = {
    "\\sum": ("∑", "undOvr"), "\\prod": ("∏", "undOvr"),
    "\\coprod": ("∐", "undOvr"), "\\bigcup": ("⋃", "undOvr"),
    "\\bigcap": ("⋂", "undOvr"), "\\bigoplus": ("⨁", "undOvr"),
    "\\int": ("∫", "subSup"), "\\iint": ("∬", "subSup"),
    "\\oint": ("∮", "subSup"),
}

_SYMBOLS = {
    # Greek
    "\\alpha": "α", "\\beta": "β", "\\gamma": "γ", "\\delta": "δ",
    "\\epsilon": "ϵ", "\\varepsilon": "ε", "\\zeta": "ζ", "\\eta": "η",
    "\\theta": "θ", "\\iota": "ι", "\\kappa": "κ", "\\lambda": "λ",
    "\\mu": "μ", "\\nu": "ν", "\\xi": "ξ", "\\pi": "π", "\\rho": "ρ",
    "\\sigma": "σ", "\\tau": "τ", "\\upsilon": "υ", "\\phi": "ϕ",
    "\\varphi": "φ", "\\chi": "χ", "\\psi": "ψ", "\\omega": "ω",
    "\\Gamma": "Γ", "\\Delta": "Δ", "\\Theta": "Θ", "\\Lambda": "Λ",
    "\\Xi": "Ξ", "\\Pi": "Π", "\\Sigma": "Σ", "\\Upsilon": "Υ",
    "\\Phi": "Φ", "\\Psi": "Ψ", "\\Omega": "Ω",
    # Relations and operators
    "\\times": "×", "\\div": "÷", "\\pm": "±", "\\mp": "∓", "\\cdot": "·",
    "\\ast": "∗", "\\approx": "≈", "\\sim": "∼", "\\simeq": "≃",
    "\\equiv": "≡", "\\propto": "∝", "\\neq": "≠", "\\ne": "≠",
    "\\leq": "≤", "\\le": "≤", "\\geq": "≥", "\\ge": "≥",
    "\\ll": "≪", "\\gg": "≫", "\\in": "∈", "\\notin": "∉",
    "\\subset": "⊂", "\\subseteq": "⊆", "\\supset": "⊃", "\\cup": "∪",
    "\\cap": "∩", "\\emptyset": "∅", "\\forall": "∀", "\\exists": "∃",
    "\\neg": "¬", "\\land": "∧", "\\lor": "∨",
    # Arrows and misc
    "\\to": "→", "\\rightarrow": "→", "\\leftarrow": "←",
    "\\Rightarrow": "⇒", "\\Leftarrow": "⇐", "\\leftrightarrow": "↔",
    "\\mapsto": "↦", "\\infty": "∞", "\\partial": "∂", "\\nabla": "∇",
    "\\cdots": "⋯", "\\ldots": "…", "\\dots": "…", "\\vdots": "⋮",
    "\\prime": "′", "\\circ": "∘", "\\bullet": "∙", "\\star": "⋆",
    "\\angle": "∠", "\\perp": "⊥", "\\parallel": "∥",
}

# Spacing commands, in the width each stands for.
_SPACES = {
    "\\,": "\u2009", "\\:": "\u2005", "\\;": "\u2005", "\\ ": " ",
    "\\quad": "\u2003", "\\qquad": "\u2003\u2003", "\\!": "", "\\thinspace": "\u2009",
}

# `\left(` / `\right)` and friends; the value is the literal delimiter.
_DELIMS = {"(": "(", ")": ")", "[": "[", "]": "]", "\\{": "{", "\\}": "}",
           "|": "|", "\\|": "‖", "\\langle": "⟨", "\\rangle": "⟩", ".": ""}

_TOKEN_RE = re.compile(r"\\[a-zA-Z]+|\\.|[{}_^]|\s+|[^\\{}_^\s]")
_CONTROL_WORD_RE = re.compile(r"\\[a-zA-Z]+")

# Where a big operator's body ends: everything up to the next relation belongs
# to the sum, so `\sum_i O(i) = X` puts `O(i)` under the ∑ and `= X` after it.
_RELATIONS = frozenset({
    "=", "<", ">", ",", ";", "\\approx", "\\equiv", "\\sim", "\\simeq",
    "\\le", "\\leq", "\\ge", "\\geq", "\\ne", "\\neq", "\\to", "\\rightarrow",
    "\\Rightarrow", "\\quad", "\\qquad", "\\propto", "\\in",
})
_STRUCTURAL_STOPS = frozenset({"}", "]", "\\right"})


def tokenize(latex: str) -> list[str]:
    """Split LaTeX into commands, grouping characters, and single characters."""
    tokens: list[str] = []
    for token in _TOKEN_RE.findall(latex):
        if token.isspace():
            # TeX eats the space that ends a control word, so `\times D` is two
            # symbols, not two symbols with a gap between them.
            if token != " " or (tokens and _CONTROL_WORD_RE.fullmatch(tokens[-1])):
                continue
            token = " "
        tokens.append(token)
    return tokens


def _flatten(tokens: list[str]) -> str:
    """Tokens as literal text — the argument of \\mathrm and friends is a word,
    not an expression, so it needs no structure."""
    return "".join(
        _SYMBOLS.get(t, _SPACES.get(t, t.lstrip("\\") if t.startswith("\\") else t))
        for t in tokens
        if t not in ("{", "}")
    )


class _Parser:
    """Recursive-descent over the token list, emitting OMML elements."""

    def __init__(self, tokens: list[str]):
        self.tokens = tokens
        self.pos = 0

    # --- token helpers ---

    def peek(self) -> str | None:
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def next(self) -> str | None:
        token = self.peek()
        if token is not None:
            self.pos += 1
        return token

    # --- parsing ---

    def parse(self, stop: str | None = None) -> list:
        """Elements up to ``stop`` (or the end), with scripts already attached."""
        out: list = []
        while True:
            token = self.peek()
            if token is None or token == stop:
                if token == stop and stop is not None:
                    self.next()
                break
            if token in ("_", "^"):
                # A script with nothing before it: treat the base as empty.
                self._attach_script(out)
                continue
            self.pos += 1
            element = self._atom(token)
            if element is None:
                continue
            out.append(element)
            while self.peek() in ("_", "^"):
                self._attach_script(out)
        return _merge_runs(_splice(out))

    def parse_until(self, stops: frozenset) -> list:
        """Elements up to (not consuming) the first token in ``stops``.

        Structural tokens always stop it too: a big operator inside a group or
        a ``\\left…\\right`` pair must not eat the thing that closes it.
        """
        stops = stops | _STRUCTURAL_STOPS
        out: list = []
        while (token := self.peek()) is not None and token not in stops:
            self.pos += 1
            element = self._atom(token)
            if element is None:
                continue
            out.append(element)
            while self.peek() in ("_", "^"):
                self._attach_script(out)
        return _trim(_merge_runs(_splice(out)))

    def _atom(self, token: str):
        if token == "{":
            return _wrap(self.parse("}"))
        if token == "}":
            return None
        if token in _SPACES:
            return _run(_SPACES[token]) if _SPACES[token] else None
        if token == "\\frac" or token == "\\dfrac" or token == "\\tfrac":
            num, den = self._group(), self._group()
            return _fraction(num, den)
        if token == "\\sqrt":
            degree = self._optional_degree()
            return _radical(self._group(), degree)
        if token in _NARY:
            return _nary(token, self)
        if token == "\\left":
            return self._delimited()
        if token == "\\right":  # unmatched: the delimiter itself is enough
            closing = self.next()
            return _run(_DELIMS.get(closing, closing or ""))
        if token in _UPRIGHT:
            return _text_run(_flatten(self._group_tokens()), upright=True)
        if token in _SCRIPTS:
            return _text_run(_flatten(self._group_tokens()), script=_SCRIPTS[token])
        if token in _SYMBOLS:
            return _run(_SYMBOLS[token])
        if token.startswith("\\"):
            # Unknown command: show its name rather than dropping the formula.
            return _text_run(token.lstrip("\\"), upright=True)
        return _run(token)

    def _attach_script(self, out: list) -> None:
        kind = self.next()  # "_" or "^"
        base = out.pop() if out else _run("")
        script = self._group()
        out.append(_scripted(base, script, sub=(kind == "_")))

    def _group(self):
        """The next single atom or {...} group, as one element."""
        token = self.next()
        if token is None:
            return _run("")
        if token == "{":
            return _wrap(self.parse("}"))
        element = self._atom(token)
        return element if element is not None else _run("")

    def _group_tokens(self) -> list[str]:
        """The raw tokens of the next group — for commands that take text."""
        token = self.next()
        if token != "{":
            return [token] if token else []
        depth, collected = 1, []
        while depth and (token := self.next()) is not None:
            if token == "{":
                depth += 1
            elif token == "}":
                depth -= 1
                if not depth:
                    break
            collected.append(token)
        return collected

    def _optional_degree(self):
        """``\\sqrt[3]{x}``'s degree, if the bracket is there."""
        if self.peek() != "[":
            return None
        self.next()
        return _wrap(self.parse("]"))

    def _delimited(self):
        opening = _DELIMS.get(self.next() or "", "")
        body: list = []
        # Parse to the matching \right, keeping nested \left…\right intact.
        while (token := self.peek()) is not None and token != "\\right":
            self.pos += 1
            element = self._atom(token)
            if element is not None:
                body.append(element)
                while self.peek() in ("_", "^"):
                    self._attach_script(body)
        closing = ""
        if self.peek() == "\\right":
            self.next()
            closing = _DELIMS.get(self.next() or "", "")
        return _delimiter(opening, closing, _merge_runs(body))


# --- OMML element builders ---


def _el(tag: str, *children):
    node = etree.Element(f"{{{M}}}{tag}", nsmap={"m": M})
    for child in children:
        if child is not None:
            node.append(child)
    return node


def _val(tag: str, value: str):
    node = etree.Element(f"{{{M}}}{tag}", nsmap={"m": M})
    node.set(f"{{{M}}}val", value)
    return node


def _run(text: str):
    return _text_run(text)


def _text_run(text: str, *, upright: bool = False, script: str | None = None):
    run = _el("r")
    if upright or script:
        props = _el("rPr")
        if script:
            props.append(_val("scr", script))
        # Without sty=p a math run italicizes its Latin letters, which is right
        # for a variable and wrong for a function name like "softmax" — or for
        # a blackboard-bold set like ℝ, which is upright by convention.
        props.append(_val("sty", "p"))
        run.append(props)
    node = _el("t")
    node.text = text
    # Leading/trailing spaces are meaningful between symbols.
    node.set("{http://www.w3.org/XML/1998/namespace}space", "preserve")
    run.append(node)
    return run


def _wrap(elements: list):
    """Several elements as one. OMML has no group node, so `{…}` becomes a
    placeholder that is either poured into a slot (`_unwrap`) or, if the group
    was just parenthesised source, spliced back into the flow (`_splice`)."""
    if len(elements) == 1:
        return elements[0]
    box = _el("e")
    for element in elements:
        box.append(element)
    return box


def _unwrap(element) -> list:
    """Elements to place inside a slot, undoing ``_wrap``'s placeholder."""
    if element is None:
        return []
    if element.tag == f"{{{M}}}e":
        return list(element)
    return [element]


def _trim(elements: list) -> list:
    """Drop the space a body picked up before the relation that ended it."""
    if elements and elements[-1].tag == f"{{{M}}}r":
        text = elements[-1].find(f"{{{M}}}t")
        if text is not None and text.text:
            text.text = text.text.rstrip()
            if not text.text:
                return elements[:-1]
    return elements


def _splice(elements: list) -> list:
    """Dissolve any group placeholder no script or slot claimed."""
    out: list = []
    for element in elements:
        out.extend(_unwrap(element))
    return out


def _slot(tag: str, element):
    return _el(tag, *_unwrap(element))


def _scripted(base, script, *, sub: bool):
    """``x_i`` / ``x^2``, folding a base that already has the other script."""
    other = f"{{{M}}}sSup" if sub else f"{{{M}}}sSub"
    if base is not None and base.tag == other:
        # C^{(N)} then _{FFN} (or the reverse): one base carrying both.
        node = _el("sSubSup", base.find(f"{{{M}}}e"))
        existing = base.find(f"{{{M}}}{'sup' if sub else 'sub'}")
        new = _slot("sub" if sub else "sup", script)
        node.append(new if sub else existing)
        node.append(existing if sub else new)
        return node
    node = _el("sSub" if sub else "sSup")
    node.append(_slot("e", base))
    node.append(_slot("sub" if sub else "sup", script))
    return node


def _fraction(num, den):
    return _el("f", _slot("num", num), _slot("den", den))


def _radical(body, degree):
    props = _el("radPr")
    if degree is None:
        props.append(_val("degHide", "1"))
    return _el("rad", props, _slot("deg", degree), _slot("e", body))


def _delimiter(opening: str, closing: str, body: list):
    props = _el("dPr")
    props.append(_val("begChr", opening))
    props.append(_val("endChr", closing))
    return _el("d", props, _el("e", *body))


def _nary(command: str, parser: _Parser):
    glyph, limits = _NARY[command]
    sub = sup = None
    while parser.peek() in ("_", "^"):
        kind = parser.next()
        if kind == "_":
            sub = parser._group()
        else:
            sup = parser._group()
    props = _el("naryPr")
    props.append(_val("chr", glyph))
    props.append(_val("limLoc", limits))
    if sub is None:
        props.append(_val("subHide", "1"))
    if sup is None:
        props.append(_val("supHide", "1"))
    body = parser.parse_until(_RELATIONS)
    return _el("nary", props, _slot("sub", sub), _slot("sup", sup),
               _el("e", *body))


def _merge_runs(elements: list) -> list:
    """Fold neighbouring plain runs together, so `O(i)` is one run not four."""
    merged: list = []
    for element in elements:
        if (merged and element.tag == f"{{{M}}}r" and merged[-1].tag == f"{{{M}}}r"
                and element.find(f"{{{M}}}rPr") is None
                and merged[-1].find(f"{{{M}}}rPr") is None):
            text = merged[-1].find(f"{{{M}}}t")
            text.text = (text.text or "") + (element.find(f"{{{M}}}t").text or "")
            continue
        merged.append(element)
    return merged


# --- public API ---


def to_omml(latex: str, *, display: bool = False):
    """The ``<m:oMath>`` (or ``<m:oMathPara>`` when display) for ``latex``."""
    body = _Parser(tokenize(latex)).parse()
    math = _el("oMath", *body)
    if not display:
        return math
    props = _el("oMathParaPr")
    props.append(_val("jc", "center"))
    return _el("oMathPara", props, math)


def to_alternate_content(latex: str, *, display: bool = False):
    """OMML wrapped for a DrawingML paragraph, with a plain-text fallback.

    PowerPoint reads the ``mc:Choice``; anything that does not understand the
    2010 math extension falls back to the text, so the formula is never lost.
    """
    root = etree.Element(f"{{{MC}}}AlternateContent", nsmap={"mc": MC, "a": A})
    choice = etree.SubElement(root, f"{{{MC}}}Choice", nsmap={"a14": A14})
    choice.set("Requires", "a14")
    holder = etree.SubElement(choice, f"{{{A14}}}m", nsmap={"a14": A14})
    holder.append(to_omml(latex, display=display))

    fallback = etree.SubElement(root, f"{{{MC}}}Fallback")
    run = etree.SubElement(fallback, f"{{{A}}}r", nsmap={"a": A})
    text = etree.SubElement(run, f"{{{A}}}t")
    text.text = plain_text(latex)
    return root


def apply_run_props(node, *, size_pt: float, color: str) -> None:
    """Give every math run the slide's body size and colour.

    A math run's own ``m:rPr`` holds only math properties (style, script), so
    size and colour ride an ``a:rPr`` from DrawingML, which the schema places
    right after it.
    """
    for run in node.iter(f"{{{M}}}r"):
        text = run.find(f"{{{M}}}t")
        if text is None:
            continue
        props = etree.Element(f"{{{A}}}rPr", nsmap={"a": A})
        props.set("lang", "en-US")
        props.set("sz", str(int(round(size_pt * 100))))
        fill = etree.SubElement(props, f"{{{A}}}solidFill")
        etree.SubElement(fill, f"{{{A}}}srgbClr").set("val", color)
        run.insert(list(run).index(text), props)  # a:rPr sits just before m:t


# Constructs that stack something above or below the baseline, so a display
# equation using one needs more than a line of vertical room.
_STACKING = ("\\frac", "\\dfrac", "\\tfrac", "\\sqrt", "\\binom", *_NARY)


def visual_lines(latex: str) -> float:
    """Roughly how many text lines tall the typeset formula will be."""
    return 2.4 if any(token in latex for token in _STACKING) else 1.2


def plain_text(latex: str) -> str:
    """A readable one-line rendering, for width estimates and the fallback."""
    out = []
    for token in tokenize(latex):
        if token in _SPACES:
            out.append(_SPACES[token] or "")
        elif token in _SYMBOLS:
            out.append(_SYMBOLS[token])
        elif token in _NARY:
            out.append(_NARY[token][0])
        elif token in ("\\left", "\\right") or token in ("{", "}"):
            continue
        elif token in _UPRIGHT or token in _SCRIPTS:
            continue
        elif token == "\\frac":
            continue
        elif token.startswith("\\"):
            out.append(token.lstrip("\\"))
        else:
            out.append(token)
    return "".join(out)
