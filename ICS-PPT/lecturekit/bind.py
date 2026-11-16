"""Bound figures: an SVG whose attributes and text are expressions over deck variables.

A figure script draws the SVG at each variable's default and leaves, on the
elements that depend on a variable, the expression they are computed from.
The file as written is an ordinary figure: the PDF, the PNG, PowerPoint, the
book and the transcript use it unchanged. The viewer evaluates the
expressions again whenever a variable moves (``bind.js``, the same language
in JavaScript) and shows the result.

Three attributes carry a binding:

``data-lk-vars`` (on one ``<metadata>`` element)
    The deck variables the figure reads and the values it is drawn at:
    ``T = 1``. Each is a ``lecture.variable``, and the value is its default.
``data-lk-let`` (on the same element)
    Values derived once for the whole figure, in order:
    ``x = [6, -5, 4]; p = softmax(x, T)``.
``data-lk-attr`` and ``data-lk-text`` (on any element)
    ``width: max(1.5, p[4] * 290); x: 698`` sets attributes, and
    ``fmt(p[4], 2)`` sets the element's text.

The expressions are a small language, evaluated here and in ``bind.js``
alike, with no access to anything but the values above:

* numbers, ``'strings'``, lists ``[a, b]``, indexing ``p[4]`` from 0;
* ``+ - * / %`` and ``^`` (power), element by element over lists, a number
  against a list applying to each item; ``+`` also joins two strings;
* ``< <= > >= == !=``, ``&& || !``, ``c ? a : b``;
* the functions in :data:`FUNCTIONS`, among them ``softmax(x, t)`` (with
  ``t = 0`` the limit, all weight on the largest entries) and ``fmt(v, nd)``.

A figure script writes these through :class:`Bindings`, which also puts the
default into the attribute or text, so a binding is never written by hand.
:func:`check_figure` reads a file back: every name defined, every
expression valid, and every attribute and text as written equal to what the
expressions give at the defaults.
"""
from __future__ import annotations

import math
import re
import xml.etree.ElementTree as ET
from decimal import ROUND_HALF_UP, Decimal, localcontext
from pathlib import Path
from typing import Any, Callable

ATTR_VARS = "data-lk-vars"
ATTR_LET = "data-lk-let"
ATTR_SET = "data-lk-attr"
ATTR_TEXT = "data-lk-text"

NAME = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
# An attribute a binding may set: an SVG attribute name, hyphens included.
ATTR_NAME = re.compile(r"[A-Za-z_][-A-Za-z0-9_.]*")

MINUS = "−"


class BindError(ValueError):
    """A binding that does not parse or does not evaluate."""


# --------------------------------------------------------------------------
# Numbers as text
# --------------------------------------------------------------------------

def _positional(value: float) -> str:
    """The shortest digits that read back as ``value``, without an exponent."""
    text = format(Decimal(repr(value)), "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    return text


def _fixed(value: float, nd: int) -> str:
    """``value`` with ``nd`` decimals, a half rounded away from zero.

    The rounding of JavaScript's ``toFixed``, which works on the exact binary
    value as this does, so both print the same digits.
    """
    with localcontext() as ctx:
        ctx.prec = 400
        exact = Decimal(value).quantize(Decimal(1).scaleb(-nd), rounding=ROUND_HALF_UP)
    return format(exact, "f")


def _unsigned_zero(text: str) -> str:
    return text[1:] if text.startswith("-") and float(text) == 0 else text


def fmt(value: Any, nd: Any = None) -> str:
    """A number as a figure prints it: ``nd`` decimals, or as few as it needs.

    The minus sign is U+2212, as in typeset text, and a zero has no sign.
    """
    number = _number(value, "fmt")
    if not math.isfinite(number):
        return "NaN" if math.isnan(number) else ("∞" if number > 0 else MINUS + "∞")
    text = _positional(number) if nd is None else _fixed(number, _decimals(nd))
    return _unsigned_zero(text).replace("-", MINUS)


def attr_number(value: float) -> str:
    """A number as an attribute holds it: two decimals at most, ASCII minus."""
    text = _fixed(value, 2)
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    return _unsigned_zero(text)


def _decimals(nd: Any) -> int:
    nd = _number(nd, "fmt")
    if nd != int(nd) or not 0 <= nd <= 20:
        raise BindError(f"a number of decimals is a whole number from 0 to 20, got {nd!r}")
    return int(nd)


# --------------------------------------------------------------------------
# Parsing
# --------------------------------------------------------------------------

_TOKEN = re.compile(r"""
    \s*(?:
      (?P<num>(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)
    | (?P<str>'[^']*')
    | (?P<name>[A-Za-z_][A-Za-z0-9_]*)
    | (?P<op>&&|\|\||==|!=|<=|>=|[-+*/%^()\[\],?:<>!])
    )""", re.VERBOSE)


def _tokens(source: str) -> list[tuple[str, Any]]:
    out: list[tuple[str, Any]] = []
    pos = 0
    while True:
        while pos < len(source) and source[pos].isspace():
            pos += 1
        if pos == len(source):
            break
        match = _TOKEN.match(source, pos)
        if match is None:
            raise BindError(f"unexpected {source[pos]!r} at {pos} in {source!r}")
        kind = match.lastgroup
        text = match.group(kind)
        if kind == "num":
            out.append(("num", float(text)))
        elif kind == "str":
            out.append(("str", text[1:-1]))
        else:
            out.append((kind, text))
        pos = match.end()
    out.append(("end", None))
    return out


class _Parser:
    def __init__(self, source: str):
        self.source = source
        self.tokens = _tokens(source)
        self.at = 0

    def peek(self) -> tuple[str, Any]:
        return self.tokens[self.at]

    def take(self, op: str | None = None) -> tuple[str, Any]:
        token = self.tokens[self.at]
        if op is not None and token != ("op", op):
            found = "the end" if token[0] == "end" else repr(token[1])
            raise BindError(f"expected {op!r}, found {found} in {self.source!r}")
        self.at += 1
        return token

    def accept(self, *ops: str) -> str | None:
        kind, value = self.peek()
        if kind == "op" and value in ops:
            self.at += 1
            return value
        return None

    def whole(self) -> tuple:
        tree = self.expr()
        if self.peek()[0] != "end":
            raise BindError(f"unexpected {self.peek()[1]!r} in {self.source!r}")
        return tree

    def expr(self) -> tuple:
        cond = self.binary(0)
        if self.accept("?"):
            yes = self.expr()
            self.take(":")
            return ("cond", cond, yes, self.expr())
        return cond

    # Binary operators by precedence, loosest first; comparisons do not chain.
    LEVELS = (("||",), ("&&",), ("==", "!=", "<", "<=", ">", ">="), ("+", "-"), ("*", "/", "%"))

    def binary(self, level: int) -> tuple:
        if level == len(self.LEVELS):
            return self.unary()
        left = self.binary(level + 1)
        while True:
            op = self.accept(*self.LEVELS[level])
            if op is None:
                return left
            left = ("bin", op, left, self.binary(level + 1))
            if level == 2:
                if self.accept(*self.LEVELS[level]):
                    raise BindError(f"comparisons do not chain in {self.source!r}")
                return left

    def unary(self) -> tuple:
        op = self.accept("-", "!")
        if op:
            return ("neg" if op == "-" else "not", self.unary())
        base = self.postfix()
        if self.accept("^"):
            return ("bin", "^", base, self.unary())
        return base

    def postfix(self) -> tuple:
        tree = self.primary()
        while self.accept("["):
            index = self.expr()
            self.take("]")
            tree = ("index", tree, index)
        return tree

    def primary(self) -> tuple:
        kind, value = self.take()
        if kind == "num":
            return ("num", value)
        if kind == "str":
            return ("str", value)
        if kind == "name":
            if self.accept("("):
                return ("call", value, self.items(")"))
            return ("name", value)
        if (kind, value) == ("op", "("):
            tree = self.expr()
            self.take(")")
            return tree
        if (kind, value) == ("op", "["):
            return ("list", self.items("]"))
        found = "the end" if kind == "end" else repr(value)
        raise BindError(f"unexpected {found} in {self.source!r}")

    def items(self, close: str) -> list[tuple]:
        out: list[tuple] = []
        if self.accept(close):
            return out
        while True:
            out.append(self.expr())
            if self.accept(close):
                return out
            self.take(",")


def parse(source: str) -> tuple:
    """The syntax tree of one expression."""
    if not isinstance(source, str):
        raise BindError(f"an expression is a string, got {source!r}")
    return _Parser(source).whole()


def _split(source: str) -> list[str]:
    """``source`` cut at each ``;`` outside a string, empty pieces dropped."""
    pieces, current, quoted = [], [], False
    for ch in source:
        if ch == "'":
            quoted = not quoted
        if ch == ";" and not quoted:
            pieces.append("".join(current))
            current = []
        else:
            current.append(ch)
    pieces.append("".join(current))
    return [piece for piece in pieces if piece.strip()]


def parse_program(source: str) -> list[tuple[str, tuple]]:
    """``name = expr; ...`` as ``[(name, tree), ...]``, in order."""
    out = []
    for piece in _split(source):
        match = re.fullmatch(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)(.*)", piece, re.DOTALL)
        if match is None:
            raise BindError(f"expected 'name = expression', got {piece.strip()!r}")
        out.append((match.group(1), parse(match.group(2))))
    return out


def parse_attrs(source: str) -> list[tuple[str, tuple]]:
    """``attribute: expr; ...`` as ``[(attribute, tree), ...]``."""
    out = []
    for piece in _split(source):
        match = re.fullmatch(r"\s*([A-Za-z_][-A-Za-z0-9_.]*)\s*:(.*)", piece, re.DOTALL)
        if match is None:
            raise BindError(f"expected 'attribute: expression', got {piece.strip()!r}")
        out.append((match.group(1), parse(match.group(2))))
    return out


# --------------------------------------------------------------------------
# Evaluation
# --------------------------------------------------------------------------

def _number(value: Any, what: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise BindError(f"{what} takes a number, got {_kind(value)}")
    return float(value)


def _list(value: Any, what: str) -> list:
    if not isinstance(value, list):
        raise BindError(f"{what} takes a list, got {_kind(value)}")
    return value


def _kind(value: Any) -> str:
    if isinstance(value, bool):
        return "a truth value"
    if isinstance(value, str):
        return f"the string {value!r}"
    if isinstance(value, list):
        return "a list"
    return repr(value)


def _truth(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    number = _number(value, "a condition")
    return number != 0 and not math.isnan(number)


# IEEE arithmetic, as JavaScript does it: a division by zero is infinite, not
# an error.
def _div(a: float, b: float) -> float:
    if b == 0:
        if a == 0 or math.isnan(a):
            return math.nan
        return math.copysign(math.inf, a) * math.copysign(1.0, b)
    return a / b


def _mod(a: float, b: float) -> float:
    if b == 0 or math.isinf(a) or math.isnan(a) or math.isnan(b):
        return math.nan
    return math.fmod(a, b)


def _pow(a: float, b: float) -> float:
    if math.isnan(b) or (abs(a) == 1 and math.isinf(b)):
        return math.nan
    try:
        return math.pow(a, b)
    except OverflowError:
        return math.inf
    except (ValueError, ZeroDivisionError):
        if a == 0 and b < 0:
            return math.inf
        return math.nan


_ARITH: dict[str, Callable[[float, float], float]] = {
    "+": lambda a, b: a + b,
    "-": lambda a, b: a - b,
    "*": lambda a, b: a * b,
    "/": _div,
    "%": _mod,
    "^": _pow,
}

_COMPARE: dict[str, Callable[[float, float], bool]] = {
    "<": lambda a, b: a < b,
    "<=": lambda a, b: a <= b,
    ">": lambda a, b: a > b,
    ">=": lambda a, b: a >= b,
}


def _arith(op: str, a: Any, b: Any) -> Any:
    if isinstance(a, list) or isinstance(b, list):
        if isinstance(a, list) and isinstance(b, list):
            if len(a) != len(b):
                raise BindError(f"{op!r} over lists of {len(a)} and {len(b)} items")
            return [_arith(op, x, y) for x, y in zip(a, b)]
        if isinstance(a, list):
            return [_arith(op, x, b) for x in a]
        return [_arith(op, a, y) for y in b]
    if op == "+" and isinstance(a, str) and isinstance(b, str):
        return a + b
    return _ARITH[op](_number(a, repr(op)), _number(b, repr(op)))


def _map(fn: Callable[[float], float], name: str) -> Callable[..., Any]:
    def apply(value: Any) -> Any:
        if isinstance(value, list):
            return [apply(item) for item in value]
        return fn(_number(value, name))
    return apply


def _exp(x: float) -> float:
    try:
        return math.exp(x)
    except OverflowError:
        return math.inf


def _log(x: float) -> float:
    if x == 0:
        return -math.inf
    if x < 0 or math.isnan(x):
        return math.nan
    return math.log(x)


def _sqrt(x: float) -> float:
    return math.nan if x < 0 or math.isnan(x) else math.sqrt(x)


def _whole(fn: Callable[[float], float]) -> Callable[[float], float]:
    return lambda x: x if not math.isfinite(x) else float(fn(x))


def _extreme(pick: Callable, name: str) -> Callable[..., float]:
    def apply(*args: Any) -> float:
        items = args[0] if len(args) == 1 and isinstance(args[0], list) else list(args)
        if not items:
            raise BindError(f"{name}() of nothing")
        return pick(_number(item, name) for item in items)
    return apply


def _round(value: Any, nd: Any = 0.0) -> float:
    number = _number(value, "round")
    places = _decimals(nd)
    return float(_fixed(number, places)) if math.isfinite(number) else number


def _clamp(value: Any, low: Any, high: Any) -> float:
    return min(max(_number(value, "clamp"), _number(low, "clamp")), _number(high, "clamp"))


def _sum(values: Any) -> float:
    total = 0.0
    for item in _list(values, "sum"):
        total += _number(item, "sum")
    return total


def _len(values: Any) -> float:
    return float(len(_list(values, "len")))


def softmax(values: Any, t: Any = 1.0) -> list[float]:
    """``exp(x / t)`` normalised to sum to 1; at ``t = 0`` the limit.

    The limit puts all the weight on the largest entries, split evenly among
    them. The exponents are taken relative to the largest, so no entry
    overflows. ``bind.js`` does the same operations in the same order.
    """
    xs = [_number(item, "softmax") for item in _list(values, "softmax")]
    if not xs:
        raise BindError("softmax() of an empty list")
    t = _number(t, "softmax")
    if t == 0:
        top = max(xs)
        hits = [1.0 if x == top else 0.0 for x in xs]
        count = _sum(hits)
        return [hit / count for hit in hits]
    scaled = [x / t for x in xs]
    top = max(scaled)
    weights = [_exp(s - top) for s in scaled]
    total = _sum(weights)
    return [w / total for w in weights]


FUNCTIONS: dict[str, tuple[Callable[..., Any], int, int]] = {
    # name: (function, fewest arguments, most arguments)
    "abs": (_map(abs, "abs"), 1, 1),
    "sqrt": (_map(_sqrt, "sqrt"), 1, 1),
    "exp": (_map(_exp, "exp"), 1, 1),
    "log": (_map(_log, "log"), 1, 1),
    "floor": (_map(_whole(math.floor), "floor"), 1, 1),
    "ceil": (_map(_whole(math.ceil), "ceil"), 1, 1),
    "round": (_round, 1, 2),
    "min": (_extreme(min, "min"), 1, 64),
    "max": (_extreme(max, "max"), 1, 64),
    "pow": (lambda a, b: _arith("^", a, b), 2, 2),
    "clamp": (_clamp, 3, 3),
    "sum": (_sum, 1, 1),
    "len": (_len, 1, 1),
    "softmax": (softmax, 1, 2),
    "fmt": (fmt, 1, 2),
}


def evaluate(tree: tuple, env: dict[str, Any]) -> Any:
    """The value of ``tree`` with the names in ``env``."""
    kind = tree[0]
    if kind in ("num", "str"):
        return tree[1]
    if kind == "name":
        if tree[1] not in env:
            raise BindError(f"{tree[1]!r} is not defined")
        return env[tree[1]]
    if kind == "list":
        return [evaluate(item, env) for item in tree[1]]
    if kind == "index":
        items = _list(evaluate(tree[1], env), "indexing")
        index = _number(evaluate(tree[2], env), "an index")
        if index != int(index) or not 0 <= index < len(items):
            raise BindError(f"index {index:g} is outside a list of {len(items)}")
        return items[int(index)]
    if kind == "call":
        if tree[1] not in FUNCTIONS:
            raise BindError(f"no function {tree[1]!r}; there are {', '.join(FUNCTIONS)}")
        fn, fewest, most = FUNCTIONS[tree[1]]
        if not fewest <= len(tree[2]) <= most:
            raise BindError(f"{tree[1]}() takes {fewest} to {most} arguments, got {len(tree[2])}")
        return fn(*(evaluate(arg, env) for arg in tree[2]))
    if kind == "neg":
        return _arith("*", evaluate(tree[1], env), -1.0)
    if kind == "not":
        return not _truth(evaluate(tree[1], env))
    if kind == "cond":
        return evaluate(tree[2] if _truth(evaluate(tree[1], env)) else tree[3], env)
    op, a = tree[1], evaluate(tree[2], env)
    if op == "&&":
        return _truth(a) and _truth(evaluate(tree[3], env))
    if op == "||":
        return _truth(a) or _truth(evaluate(tree[3], env))
    b = evaluate(tree[3], env)
    if op in ("==", "!="):
        if isinstance(a, list) or isinstance(b, list):
            raise BindError(f"{op!r} does not compare lists")
        if isinstance(a, (str, bool)) or isinstance(b, (str, bool)):
            same = type(a) is type(b) and a == b
        else:
            same = _number(a, op) == _number(b, op)
        return same if op == "==" else not same
    if op in _COMPARE:
        return _COMPARE[op](_number(a, repr(op)), _number(b, repr(op)))
    return _arith(op, a, b)


def run(program: list[tuple[str, tuple]], env: dict[str, Any]) -> dict[str, Any]:
    """``env`` extended by each ``name = expr`` of ``program`` in turn."""
    env = dict(env)
    for name, tree in program:
        env[name] = evaluate(tree, env)
    return env


def names(tree: tuple) -> set[str]:
    """The names ``tree`` reads (not the functions it calls)."""
    if tree[0] == "name":
        return {tree[1]}
    found: set[str] = set()
    for part in tree[1:]:
        if isinstance(part, tuple):
            found |= names(part)
        elif isinstance(part, list):
            for item in part:
                found |= names(item)
    return found


def attr_value(value: Any, what: str) -> str:
    """What an attribute holds for ``value``: a number, or a string as is."""
    if isinstance(value, str):
        return value
    number = _number(value, what)
    if not math.isfinite(number):
        raise BindError(f"{what} is {number}, which no attribute can hold")
    return attr_number(number)


def text_value(value: Any, what: str) -> str:
    """What an element's text is for ``value``: a string as is, a number by fmt."""
    if isinstance(value, str):
        return value
    return fmt(_number(value, what))


def literal(value: Any) -> str:
    """``value`` (a number, a string, a list of them) written as an expression."""
    if isinstance(value, bool):
        raise BindError("a truth value has no literal; write a comparison")
    if isinstance(value, (int, float)):
        number = float(value)
        if not math.isfinite(number):
            raise BindError(f"{number} has no literal")
        return _positional(number)
    if isinstance(value, str):
        if "'" in value:
            raise BindError(f"a string literal cannot hold a quote: {value!r}")
        return f"'{value}'"
    if isinstance(value, (list, tuple)):
        return "[" + ", ".join(literal(item) for item in value) + "]"
    raise BindError(f"{value!r} has no literal")


# --------------------------------------------------------------------------
# Writing a bound figure (for figure scripts)
# --------------------------------------------------------------------------

_START_TAG = re.compile(r'\s*<([A-Za-z][-A-Za-z0-9_:.]*)((?:\s+[^\s=/>]+\s*=\s*"[^"]*")*)\s*(/?)>')
_ATTR = re.compile(r'([^\s=/>]+)\s*=\s*"([^"]*)"')


def _escape(text: str) -> str:
    return (text.replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


class Bindings:
    """The bindings of one figure, for the script that draws it.

    ``Bindings(T=1)`` names the deck variables the figure reads, at the values
    the file is drawn at: the variables' defaults (``inspect`` checks). Then::

        bind.let("p", "softmax(x, T)")          # a derived value
        b.append(bind.declare())                # the <metadata> element
        b.append(bind.attrs(rect(...), width="max(1.5, p[4] * 290)"))
        b.append(bind.text(text(...), "fmt(p[4], 2)"))

    :meth:`attrs` and :meth:`text` take the markup any drawing helper returns,
    put the value at the defaults into it and add the expression beside it.
    A keyword's underscores stand for hyphens (``stroke_width``).
    """

    def __init__(self, **inputs: float):
        if not inputs:
            raise BindError("a bound figure reads at least one deck variable")
        self.inputs = {name: _number(value, name) for name, value in inputs.items()}
        self.lets: list[tuple[str, str]] = []
        self.env: dict[str, Any] = dict(self.inputs)

    def let(self, name: str, value: Any) -> Any:
        """Define ``name`` from an expression (a string) or a constant; its value."""
        if not NAME.fullmatch(name) or name in self.env or name in FUNCTIONS:
            raise BindError(f"cannot define {name!r}: taken or not a name")
        source = value if isinstance(value, str) else literal(value)
        self.env[name] = evaluate(parse(source), self.env)
        self.lets.append((name, source))
        return self.env[name]

    def value(self, expr: str) -> Any:
        """``expr`` at the values the figure is drawn at."""
        return evaluate(parse(expr), self.env)

    def declare(self) -> str:
        """The ``<metadata>`` element that declares the inputs and derived values."""
        inputs = "; ".join(f"{n} = {literal(v)}" for n, v in self.inputs.items())
        lets = "; ".join(f"{n} = {source}" for n, source in self.lets)
        return (f'<metadata {ATTR_VARS}="{_escape(inputs)}" '
                f'{ATTR_LET}="{_escape(lets)}"/>')

    def attrs(self, markup: str, **exprs: str) -> str:
        """``markup`` with each attribute set from its expression."""
        match = _START_TAG.match(markup)
        if match is None:
            raise BindError(f"no start tag to bind in {markup[:60]!r}")
        attrs = dict(_ATTR.findall(match.group(2)))
        bound = [(name.replace("_", "-"), expr) for name, expr in exprs.items()]
        for name, expr in bound:
            attrs[name] = _escape(attr_value(self.value(expr), name))
        old = attrs.pop(ATTR_SET, "")
        spec = "; ".join(f"{name}: {expr}" for name, expr in bound)
        attrs[ATTR_SET] = (old + "; " if old else "") + _escape(spec)
        tag = match.group(1)
        head = f"<{tag}" + "".join(f' {k}="{v}"' for k, v in attrs.items()) + f"{match.group(3)}>"
        return markup[:match.start(1) - 1] + head + markup[match.end():]

    def text(self, markup: str, expr: str) -> str:
        """``markup`` (one element) with its text set from ``expr``."""
        match = _START_TAG.match(markup)
        if match is None or match.group(3):
            raise BindError(f"no element with text to bind in {markup[:60]!r}")
        tag = match.group(1)
        close = f"</{tag}>"
        body = markup.rstrip()
        if not body.endswith(close):
            raise BindError(f"{markup[:60]!r} is not one <{tag}> element")
        value = text_value(self.value(expr), "text")
        attrs = match.group(2) + f' {ATTR_TEXT}="{_escape(expr)}"'
        return (markup[:match.start(1) - 1] + f"<{tag}{attrs}>" + _escape(value)
                + close + markup[len(body):])


# --------------------------------------------------------------------------
# Reading a bound figure back
# --------------------------------------------------------------------------

def is_bound(path: Path) -> bool:
    """Does the SVG at ``path`` declare bindings? (A cheap test, no parse.)"""
    if path.suffix.lower() != ".svg" or not path.is_file():
        return False
    return ATTR_VARS.encode() in path.read_bytes()


def _local(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def read_inputs(path: Path) -> dict[str, float]:
    """The deck variables a bound figure reads, and the values it is drawn at."""
    root = _parse_file(path)
    declarations = [el for el in root.iter() if ATTR_VARS in el.attrib]
    if len(declarations) != 1:
        raise BindError(f"{path.name}: {len(declarations)} elements declare "
                        f"{ATTR_VARS}; a bound figure has one")
    inputs: dict[str, float] = {}
    for name, tree in parse_program(declarations[0].get(ATTR_VARS)):
        inputs[name] = _number(evaluate(tree, {}), f"{name} in {ATTR_VARS}")
    if not inputs:
        raise BindError(f"{path.name}: {ATTR_VARS} names no variable")
    return inputs


def _parse_file(path: Path) -> ET.Element:
    try:
        return ET.parse(path).getroot()
    except ET.ParseError as error:
        raise BindError(f"{path.name}: not well-formed XML ({error})") from None


def check_figure(path: Path) -> dict[str, float]:
    """Check a bound figure and return its inputs (see :func:`read_inputs`).

    Every expression parses and evaluates at the inputs, each name is an input
    or defined before it is used, and each bound attribute and text as written
    is what its expression gives: the file shows the figure at the inputs.
    """
    root = _parse_file(path)
    inputs = read_inputs(path)
    declaration = next(el for el in root.iter() if ATTR_VARS in el.attrib)
    env: dict[str, Any] = dict(inputs)
    for name, tree in parse_program(declaration.get(ATTR_LET, "")):
        if name in env or name in FUNCTIONS:
            raise BindError(f"{path.name}: {name!r} is defined twice")
        try:
            env[name] = evaluate(tree, env)
        except BindError as error:
            raise BindError(f"{path.name}: {name} = …: {error}") from None
    for el in root.iter():
        where = f"{path.name}: <{_local(el.tag)}>"
        try:
            for attr, tree in parse_attrs(el.get(ATTR_SET, "")):
                want = attr_value(evaluate(tree, env), attr)
                have = el.get(attr)
                if have is None or not _same(have, want):
                    raise BindError(f"{attr}={have!r}, but its binding gives {want!r}")
            if ATTR_TEXT in el.attrib:
                want = text_value(evaluate(parse(el.get(ATTR_TEXT)), env), "text")
                have = "".join(el.itertext())
                if have != want:
                    raise BindError(f"text {have!r}, but its binding gives {want!r}")
        except BindError as error:
            raise BindError(f"{where}: {error}") from None
    return inputs


def _same(have: str, want: str) -> bool:
    if have == want:
        return True
    try:
        return abs(float(have) - float(want)) <= 0.0051
    except ValueError:
        return False
