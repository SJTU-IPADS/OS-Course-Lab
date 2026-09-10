"""LaTeX math → plain HTML, for a sheet that has to stand on its own.

The viewer hands `$…$` to MathJax, the book to TeX, and PowerPoint gets OMML.
The transcript is a single self-contained file a student prints, so it can
neither load a script nor ship a megabyte of typesetting engine: it renders the
vocabulary lecture math actually uses — scripts, fractions, roots, big
operators, delimiters, upright text and the usual symbols — with `<sup>`,
`<sub>`, a two-row fraction, and Unicode.

What it does not recognize keeps its own name (`\\foo` renders as `foo`) rather
than taking the sheet down with it, the same bargain the OMML translator makes.
"""

from __future__ import annotations

import html
import re

_SYMBOLS = {
    # Greek
    "\\alpha": "α", "\\beta": "β", "\\gamma": "γ", "\\delta": "δ",
    "\\epsilon": "ε", "\\varepsilon": "ε", "\\zeta": "ζ", "\\eta": "η",
    "\\theta": "θ", "\\vartheta": "ϑ", "\\iota": "ι", "\\kappa": "κ",
    "\\lambda": "λ", "\\mu": "μ", "\\nu": "ν", "\\xi": "ξ", "\\pi": "π",
    "\\rho": "ρ", "\\sigma": "σ", "\\tau": "τ", "\\upsilon": "υ",
    "\\phi": "φ", "\\varphi": "φ", "\\chi": "χ", "\\psi": "ψ", "\\omega": "ω",
    "\\Gamma": "Γ", "\\Delta": "Δ", "\\Theta": "Θ", "\\Lambda": "Λ",
    "\\Xi": "Ξ", "\\Pi": "Π", "\\Sigma": "Σ", "\\Upsilon": "Υ", "\\Phi": "Φ",
    "\\Psi": "Ψ", "\\Omega": "Ω",
    # Operators and relations
    "\\times": "×", "\\cdot": "·", "\\div": "÷", "\\pm": "±", "\\mp": "∓",
    "\\ast": "∗", "\\star": "⋆", "\\oplus": "⊕", "\\otimes": "⊗",
    "\\approx": "≈", "\\sim": "∼", "\\simeq": "≃", "\\equiv": "≡",
    "\\neq": "≠", "\\ne": "≠", "\\leq": "≤", "\\le": "≤", "\\geq": "≥",
    "\\ge": "≥", "\\ll": "≪", "\\gg": "≫", "\\propto": "∝",
    "\\in": "∈", "\\notin": "∉", "\\subset": "⊂", "\\subseteq": "⊆",
    "\\cup": "∪", "\\cap": "∩", "\\forall": "∀", "\\exists": "∃",
    "\\land": "∧", "\\lor": "∨", "\\neg": "¬",
    # Arrows and misc
    "\\to": "→", "\\rightarrow": "→", "\\leftarrow": "←", "\\Rightarrow": "⇒",
    "\\Leftarrow": "⇐", "\\leftrightarrow": "↔", "\\uparrow": "↑",
    "\\downarrow": "↓", "\\mapsto": "↦",
    "\\infty": "∞", "\\partial": "∂", "\\nabla": "∇", "\\emptyset": "∅",
    "\\ldots": "…", "\\dots": "…", "\\cdots": "⋯", "\\vdots": "⋮",
    "\\degree": "°", "\\percent": "%", "\\angle": "∠",
    # Escaped literals
    "\\{": "{", "\\}": "}", "\\%": "%", "\\&": "&", "\\#": "#", "\\$": "$",
    "\\_": "_",
}

_NARY = {
    "\\sum": "∑", "\\prod": "∏", "\\coprod": "∐", "\\int": "∫",
    "\\iint": "∬", "\\oint": "∮", "\\bigcup": "⋃", "\\bigcap": "⋂",
}

_SPACES = {
    "\\,": " ", "\\;": " ", "\\:": " ", "\\!": "",
    "\\quad": " ", "\\qquad": "  ", "\\ ": " ", "~": " ",
}

# Commands whose single argument is set upright rather than italic.
_UPRIGHT = {
    "\\text", "\\mathrm", "\\operatorname", "\\mathsf", "\\mathbf", "\\mathtt",
    "\\textrm", "\\textbf", "\\mathbb", "\\mathcal", "\\mathfrak",
}

# Function names that are upright even without \mathrm.
_FUNCTIONS = {
    "\\log", "\\ln", "\\exp", "\\max", "\\min", "\\sin", "\\cos", "\\tan",
    "\\lim", "\\arg", "\\det", "\\dim", "\\gcd", "\\sup", "\\inf",
}

_TOKEN_RE = re.compile(r"\\[A-Za-z]+|\\.|[{}^_]|\s+|.", re.DOTALL)


def math_html(latex: str) -> str:
    """Render one LaTeX formula as an inline `<span class="tx-mathspan">`."""
    tokens = [t for t in _TOKEN_RE.findall(latex) if not t.isspace() or t == " "]
    body, _ = _render(tokens, 0)
    return f'<span class="tx-mathspan">{body}</span>'


def _render(tokens: list[str], index: int, *, stop_at_group_end: bool = False) -> tuple[str, int]:
    out: list[str] = []
    while index < len(tokens):
        token = tokens[index]
        if token == "}":
            if stop_at_group_end:
                return "".join(out), index + 1
            index += 1
            continue
        if token == "{":
            body, index = _render(tokens, index + 1, stop_at_group_end=True)
            out.append(body)
            continue
        if token in ("^", "_"):
            tag = "sup" if token == "^" else "sub"
            atom, index = _atom(tokens, index + 1)
            out.append(f"<{tag}>{atom}</{tag}>")
            continue
        if token == "\\frac":
            numerator, index = _atom(tokens, index + 1)
            denominator, index = _atom(tokens, index)
            out.append(
                '<span class="tx-frac"><span class="tx-num">'
                f"{numerator}</span><span class=\"tx-den\">{denominator}</span></span>"
            )
            continue
        if token == "\\sqrt":
            if index + 1 < len(tokens) and tokens[index + 1] == "[":
                index += 2
                while index < len(tokens) and tokens[index] != "]":
                    index += 1
                index += 1
            body, index = _atom(tokens, index + 1)
            out.append(f'√<span class="tx-root">{body}</span>')
            continue
        if token == "\\color":
            spec, index = _group_text(tokens, index + 1)
            body, index = _atom(tokens, index)
            color = html.escape(spec, quote=True)
            out.append(f'<span style="color:{color}">{body}</span>')
            continue
        if token in _UPRIGHT:
            body, index = _atom(tokens, index + 1)
            out.append(f'<span class="tx-up">{body}</span>')
            continue
        if token in _FUNCTIONS:
            out.append(f'<span class="tx-up">{token[1:]}</span>')
            index += 1
            continue
        if token in ("\\left", "\\right", "\\big", "\\Big", "\\bigl", "\\bigr"):
            index += 1
            continue
        if token in _SPACES:
            out.append(_SPACES[token])
            index += 1
            continue
        if token in _NARY:
            out.append(f'<span class="tx-up tx-nary">{_NARY[token]}</span>')
            index += 1
            continue
        if token in _SYMBOLS:
            out.append(f'<span class="tx-up">{html.escape(_SYMBOLS[token])}</span>')
            index += 1
            continue
        if token.startswith("\\"):
            # An unknown command keeps its name, upright — better a formula that
            # reads slightly wrong than a sheet that fails to render.
            out.append(f'<span class="tx-up">{html.escape(token[1:])}</span>')
            index += 1
            continue
        out.append(_plain(token))
        index += 1
    return "".join(out), index


def _atom(tokens: list[str], index: int) -> tuple[str, int]:
    """The next argument: a braced group, or a single token."""
    if index >= len(tokens):
        return "", index
    if tokens[index] == "{":
        return _render(tokens, index + 1, stop_at_group_end=True)
    return _render([tokens[index]], 0)[0], index + 1


def _group_text(tokens: list[str], index: int) -> tuple[str, int]:
    """The raw text of a braced group — for `\\color{#rrggbb}{…}`."""
    if index < len(tokens) and tokens[index] == "{":
        index += 1
        parts = []
        while index < len(tokens) and tokens[index] != "}":
            parts.append(tokens[index])
            index += 1
        return "".join(parts), index + 1
    return (tokens[index], index + 1) if index < len(tokens) else ("", index)


def _plain(token: str) -> str:
    """A bare character: letters stay italic (the span's default), the rest upright."""
    escaped = html.escape(token, quote=False)
    if token.isalpha():
        return escaped
    return f'<span class="tx-up">{escaped}</span>'
