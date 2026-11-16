"""Deck variables: a number the room can change, and what is drawn from it.

A lecture declares a variable once::

    lecture.variable("T", 1, min=0, max=10)

It takes any value from ``min`` to ``max``, or only ``min + k * step`` when a
``step`` is given. A page binds a control to it (``p.slider("T")``,
``p.input("T")``) and uses it in two ways:

* where a number is taken: an image's size (``width_px="$size"`` and the other
  three) and a spacer (``p.spacer("$gap")``). The reference is the whole value.
* in a bound figure: an SVG whose attributes and text are expressions over the
  variables (:mod:`lecturekit.bind`). The figure script writes the file at the
  defaults, and the page shows it with ``p.image`` like any other.

The model keeps every reference as written, and each target spells it once, on
the way in:

* :func:`for_static` gives PowerPoint, the book and the transcript sheet the
  default, since nothing moves there. A bound figure is already drawn at it.
* :func:`for_deck` gives the Marp deck a CSS custom property with the default
  as its fallback (``calc(var(--lkv-size, 480) * 1px)``) and marks each bound
  figure; :func:`page_table` adds the definitions each page needs and the
  source of its bound figures. A page without script (the PDF, the PNG) shows
  the defaults, and the viewer's ``vars.js`` moves every use of a variable at
  once.

A value is printed with as few decimals as it needs (:func:`number_text`); a
slider's readout keeps at least the decimals of the slider's step, so it does
not change width as the thumb moves.

``model`` imports this module, so this one reaches ``model`` at call time only.
"""
from __future__ import annotations

import json
import math
import re
from dataclasses import dataclass, replace
from decimal import Decimal
from pathlib import Path
from typing import Any, Callable, Iterator

from . import bind, model

NAME = r"[A-Za-z_][A-Za-z0-9_]*"
NAME_RE = re.compile(NAME)
# `$name` or `${name}`.
REF_RE = re.compile(rf"\$(?:\{{({NAME})\}}|({NAME}))")
# A size as the DSL stores it: `${name}px` or `${name}%`.
SIZE_RE = re.compile(rf"\$\{{({NAME})\}}(px|%)")

# The CSS custom property a variable is held in on the deck: `--lkv-<name>`.
CSS_PREFIX = "--lkv-"

# A float that is a grid value up to rounding: (value - min) / step within this
# of a whole number.
_GRID_SLACK = 1e-6

# The keys of an `image_right` picture (``Block.float_image``) that may hold a
# reference, as an image's content does.
FLOAT_IMAGE_KEYS = frozenset({"width", "height"})

# The blocks whose content names a picture in ``src`` (a row, in its items).
IMAGE_KINDS = frozenset({"image", "row", "side_image"})


# --------------------------------------------------------------------------
# Spelling a reference
# --------------------------------------------------------------------------

def ref_name(value: Any) -> str | None:
    """The variable a whole value names (``"$T"`` or ``"${T}"``), else None."""
    if not isinstance(value, str):
        return None
    match = REF_RE.fullmatch(value.strip())
    return (match.group(1) or match.group(2)) if match else None


def names_in(text: str) -> list[str]:
    """Every variable a string names, in order, repeats included."""
    return [m.group(1) or m.group(2) for m in REF_RE.finditer(text)]


def number_or_ref(value: Any, what: str) -> Any:
    """A number as given, or a reference in the stored spelling ``${name}``.

    The DSL calls this on every parameter that takes a variable, so a string
    that is not a reference is refused where it was written.
    """
    if isinstance(value, str):
        name = ref_name(value)
        if name is None:
            raise model.ValidationError(
                f'{what} takes a number or a variable reference such as "$size", '
                f"got {value!r}"
            )
        return "${" + name + "}"
    return value


def refuse_ref(value: Any, what: str) -> None:
    """Raise on a variable reference where a number is consumed at build time."""
    if isinstance(value, str) and "$" in value:
        raise model.ValidationError(
            f"{what} does not take a variable ({value!r}): its value is laid out "
            "when the deck is built, before any control can change it"
        )


# --------------------------------------------------------------------------
# A variable's values
# --------------------------------------------------------------------------

def _is_number(value: Any) -> bool:
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(value))


def decimals(value: float) -> int:
    """How many decimals ``value`` is written with: 0.1 -> 1, 10.0 -> 0."""
    exponent = Decimal(repr(float(value))).normalize().as_tuple().exponent
    return max(0, -int(exponent))


def number_text(value: float, places: int = 0) -> str:
    """``value`` with as few decimals as it needs, and at least ``places``.

    ``1`` -> ``"1"``, ``0.37`` -> ``"0.37"``; with ``places=1``, ``1`` ->
    ``"1.0"``. ASCII minus, and never ``-0``. ``vars.js`` prints the same.
    """
    text = bind._unsigned_zero(bind._positional(float(value)))
    have = len(text) - text.index(".") - 1 if "." in text else 0
    if have < places:
        text += ("" if "." in text else ".") + "0" * (places - have)
    return text


def on_grid(var: model.Variable, value: float) -> bool:
    """Is ``value`` one of the values ``var`` takes?"""
    if not _is_number(value) or not var.min <= value <= var.max:
        return False
    if var.step is None:
        return True
    k = (value - var.min) / var.step
    return abs(k - round(k)) < _GRID_SLACK


def slider_step(var: model.Variable, step: float | None = None) -> float:
    """The step a slider moves ``var`` by.

    The slider's own ``step``, else the variable's, else the power of ten
    nearest below a hundredth of the range (0 to 10: 0.1).
    """
    if step is not None:
        return step
    if var.step is not None:
        return var.step
    exponent = Decimal((var.max - var.min) / 100).adjusted()
    return float(Decimal(1).scaleb(exponent))


def definition(var: model.Variable) -> dict:
    """What a target needs to know about ``var``, as plain data."""
    return {"name": var.name, "default": var.default, "min": var.min,
            "max": var.max, "step": var.step}


def describe(var: model.Variable) -> str:
    """``$T = 1 in [0, 10]`` (and ``step 0.1``): one line for ``inspect``."""
    step = f" step {number_text(var.step)}" if var.step is not None else ""
    return (f"${var.name} = {number_text(var.default)} in "
            f"[{number_text(var.min)}, {number_text(var.max)}]{step}")


# --------------------------------------------------------------------------
# Validation
# --------------------------------------------------------------------------

def check_definitions(variables: tuple[model.Variable, ...]) -> dict[str, model.Variable]:
    """Refuse a malformed or repeated declaration; return them by name."""
    declared: dict[str, model.Variable] = {}
    for var in variables:
        if not isinstance(var.name, str) or not NAME_RE.fullmatch(var.name):
            raise model.ValidationError(
                "a variable name is letters, digits and _, not starting with a "
                f"digit; got {var.name!r}"
            )
        if var.name in declared:
            raise model.ValidationError(f"variable {var.name!r} is declared twice")
        for field in ("default", "min", "max") + (("step",) if var.step is not None else ()):
            if not _is_number(getattr(var, field)):
                raise model.ValidationError(
                    f"variable {var.name!r}: {field} must be a finite number, "
                    f"got {getattr(var, field)!r}"
                )
        if var.min >= var.max:
            raise model.ValidationError(
                f"variable {var.name!r}: min {var.min!r} must be below max {var.max!r}"
            )
        if not var.min <= var.default <= var.max:
            raise model.ValidationError(
                f"variable {var.name!r}: default {var.default!r} is outside "
                f"[{var.min!r}, {var.max!r}]"
            )
        if var.step is not None:
            if var.step <= 0:
                raise model.ValidationError(
                    f"variable {var.name!r}: step must be positive, got {var.step!r}"
                )
            span = (var.max - var.min) / var.step
            if abs(span - round(span)) >= _GRID_SLACK:
                raise model.ValidationError(
                    f"variable {var.name!r}: max {var.max!r} is not min + k * step "
                    f"(min {var.min!r}, step {var.step!r})"
                )
            if not on_grid(var, var.default):
                raise model.ValidationError(
                    f"variable {var.name!r}: default {var.default!r} is not min + k * "
                    f"step (min {var.min!r}, step {var.step!r})"
                )
        declared[var.name] = var
    return declared


def references(block: model.Block) -> list[tuple[str, str]]:
    """``(key, value)`` for every value of ``block`` that names a variable.

    Only the keys the block's class lists in ``VARIABLE_KEYS`` are read, at any
    depth (a row's items), and the ``image_right`` picture's.
    """
    found: list[tuple[str, str]] = []

    def visit(value: Any, keys: frozenset[str]) -> None:
        if isinstance(value, dict):
            for key, item in value.items():
                if key in keys and isinstance(item, str) and "$" in item:
                    found.append((key, item))
                else:
                    visit(item, keys)
        elif isinstance(value, (list, tuple)):
            for item in value:
                visit(item, keys)

    if block.VARIABLE_KEYS:
        visit(block.content, block.VARIABLE_KEYS)
    if block.float_image is not None:
        visit(block.float_image, FLOAT_IMAGE_KEYS)
    return found


def _declared(name: Any, declared: dict[str, model.Variable], where: str) -> model.Variable:
    if name in declared:
        return declared[name]
    known = ", ".join(f"${n}" for n in declared) or "none"
    raise model.ValidationError(
        f"{where} uses ${name}, which the lecture does not declare "
        f'(lecture.variable("{name}", ...); declared: {known})'
    )


def _values(var: model.Variable) -> str:
    out = f"{number_text(var.min)} to {number_text(var.max)}"
    return out + (f" in steps of {number_text(var.step)}" if var.step is not None else "")


def _check_control(block: model.Block, var: model.Variable, where: str) -> None:
    content = block.content
    for mark in content.get("marks") or ():
        if not on_grid(var, mark):
            raise model.ValidationError(
                f"{where}: mark {mark!r} is not a value ${var.name} takes "
                f"({_values(var)})"
            )
    step = content.get("step")
    if step is None:
        return
    if not _is_number(step) or step <= 0:
        raise model.ValidationError(f"{where}: step must be a positive number, got {step!r}")
    if var.step is not None:
        k = step / var.step
        if abs(k - round(k)) >= _GRID_SLACK:
            raise model.ValidationError(
                f"{where}: step {step!r} is not a multiple of ${var.name}'s step "
                f"{number_text(var.step)}"
            )


def check_block(block: model.Block, declared: dict[str, model.Variable], *, page_id: str) -> None:
    """Every variable ``block`` uses is declared, and fits where it is used."""
    where = f"the {block.kind} block on {page_id}"
    if block.CONTROL:
        content = block.content if isinstance(block.content, dict) else {}
        _check_control(block, _declared(content.get("var"), declared, where), where)
    for key, value in references(block):
        if key == "px":
            name = ref_name(value)
            if name is None:
                raise model.ValidationError(
                    f'{where}: px {value!r} is neither a number nor "$name"'
                )
            var = _declared(name, declared, where)
            whole = var.step is not None and float(var.step).is_integer() \
                and float(var.min).is_integer()
            if not whole or var.min < 1:
                raise model.ValidationError(
                    f"{where}: px is a whole number of pixels from 1 up, and "
                    f"${name} takes {_values(var)}"
                )
        else:
            match = SIZE_RE.fullmatch(value)
            if match is None:
                raise model.ValidationError(
                    f'{where}: {key} {value!r} is not a size; write the '
                    f'reference as the whole value, {key}_px="$name"'
                )
            var = _declared(match.group(1), declared, where)
            if var.min < 0:
                raise model.ValidationError(
                    f"{where}: {key} takes ${var.name}, which goes down to "
                    f"{number_text(var.min)}; a size is not negative"
                )


# --------------------------------------------------------------------------
# Bound figures
# --------------------------------------------------------------------------

@dataclass(frozen=True)
class Figure:
    """A bound figure a page shows: its source, and the variables it reads."""
    src: str
    svg: str
    inputs: dict[str, float]


def image_sources(block: model.Block) -> Iterator[str]:
    """The ``src`` of every picture ``block`` shows."""
    if block.kind in IMAGE_KINDS and isinstance(block.content, dict):
        if isinstance(block.content.get("src"), str):
            yield block.content["src"]
        for item in block.content.get("items") or ():
            if isinstance(item, dict) and isinstance(item.get("src"), str):
                yield item["src"]
    if block.float_image is not None and isinstance(block.float_image.get("src"), str):
        yield block.float_image["src"]


def _figure(src: str, asset_root: Path | None, borrowed: tuple) -> Figure | None:
    if "://" in src or not src.lower().endswith(".svg"):
        return None
    path = model.resolve_asset(src, asset_root, borrowed)
    if not bind.is_bound(path):
        return None
    try:
        inputs = bind.read_inputs(path)
    except bind.BindError as error:
        raise model.ValidationError(f"{src}: {error}") from None
    return Figure(src=src, svg=path.read_text(encoding="utf-8"), inputs=inputs)


def figures(lecture: model.Lecture, asset_root: Path | None) -> dict[str, Figure]:
    """Every bound figure the lecture's pages show, by ``src``."""
    found: dict[str, Figure] = {}
    for page in model.flatten_pages(lecture.children):
        for block in page.blocks:
            for src in image_sources(block):
                if src not in found:
                    figure = _figure(src, asset_root, lecture.borrowed)
                    if figure is not None:
                        found[src] = figure
    return found


def check_figures(lecture: model.Lecture, asset_root: Path | None) -> None:
    """Every bound figure is well formed and drawn at its variables' defaults.

    Its expressions parse and evaluate, every name it reads is a declared
    variable, the values it is drawn at are their defaults, and each bound
    attribute and text in the file is what its expression gives there. The
    file is then what every static target shows, and the viewer starts from it.
    """
    declared = {var.name: var for var in lecture.variables}
    checked: set[str] = set()
    for page in model.flatten_pages(lecture.children):
        for block in page.blocks:
            for src in image_sources(block):
                if src in checked:
                    continue
                checked.add(src)
                figure = _figure(src, asset_root, lecture.borrowed)
                if figure is None:
                    continue
                path = model.resolve_asset(src, asset_root, lecture.borrowed)
                try:
                    bind.check_figure(path)
                except bind.BindError as error:
                    raise model.ValidationError(f"{page.id}: {error}") from None
                for name, drawn in figure.inputs.items():
                    var = _declared(name, declared, f"{page.id}: the figure {src}")
                    if drawn != var.default:
                        raise model.ValidationError(
                            f"{page.id}: {src} is drawn at {name} = "
                            f"{number_text(drawn)}, but ${name} defaults to "
                            f"{number_text(var.default)}; draw it at the default"
                        )


# --------------------------------------------------------------------------
# Spelling the lecture for a target
# --------------------------------------------------------------------------

Spell = Callable[[str, str], dict]


def _rewrite(value: Any, keys: frozenset[str], spell: Spell) -> Any:
    if isinstance(value, dict):
        out: dict = {}
        for key, item in value.items():
            if key in keys and isinstance(item, str) and "$" in item:
                out.update(spell(key, item))
            else:
                out[key] = _rewrite(item, keys, spell)
        return out
    if isinstance(value, list):
        return [_rewrite(item, keys, spell) for item in value]
    if isinstance(value, tuple):
        return tuple(_rewrite(item, keys, spell) for item in value)
    return value


def _mark(value: Any, bound: dict[str, Figure]) -> Any:
    """``value`` with ``"figure": src`` beside every ``src`` of a bound figure."""
    if isinstance(value, dict):
        out = {key: _mark(item, bound) for key, item in value.items()}
        if out.get("src") in bound:
            out["figure"] = out["src"]
        return out
    if isinstance(value, list):
        return [_mark(item, bound) for item in value]
    return value


def _spell_static(declared: dict[str, model.Variable]) -> Spell:
    def spell(key: str, value: str) -> dict:
        if key == "px":
            return {key: int(round(declared[ref_name(value)].default))}
        name, unit = SIZE_RE.fullmatch(value).groups()
        return {key: f"{number_text(declared[name].default)}{unit}"}
    return spell


def css_value(var: model.Variable) -> str:
    """The variable on the deck: its custom property, falling back to the default."""
    return f"var({CSS_PREFIX}{var.name}, {number_text(var.default)})"


def _spell_deck(declared: dict[str, model.Variable]) -> Spell:
    def spell(key: str, value: str) -> dict:
        if key == "px":
            var = declared[ref_name(value)]
            return {key: f"calc({css_value(var)} * 1px)"}
        name, unit = SIZE_RE.fullmatch(value).groups()
        return {key: f"calc({css_value(declared[name])} * 1{unit})"}
    return spell


def _resolve(lecture: model.Lecture, spell: Spell | None,
             bound: dict[str, Figure]) -> model.Lecture:
    declared = {var.name: var for var in lecture.variables}
    if not declared:
        return lecture

    def block(b: model.Block) -> model.Block:
        changes: dict[str, Any] = {}
        content = b.content
        if b.VARIABLE_KEYS and spell is not None:
            content = _rewrite(content, b.VARIABLE_KEYS, spell)
        if bound and b.kind in IMAGE_KINDS:
            content = _mark(content, bound)
        if b.CONTROL:
            var = declared[content["var"]]
            content = {**content, "variable": definition(var), "lang": lecture.lang}
            if b.kind == "slider":
                content["step_used"] = slider_step(var, content.get("step"))
        if content is not b.content:
            changes["content"] = content
        if b.float_image is not None:
            image = b.float_image
            if spell is not None:
                image = _rewrite(image, FLOAT_IMAGE_KEYS, spell)
            changes["float_image"] = _mark(image, bound)
        return replace(b, **changes) if changes else b

    def node(n: model.Section | model.Page) -> model.Section | model.Page:
        if isinstance(n, model.Page):
            return replace(n, blocks=[block(b) for b in n.blocks])
        return replace(n, children=[node(c) for c in n.children])

    return replace(lecture, children=[node(c) for c in lecture.children])


def for_static(lecture: model.Lecture) -> model.Lecture:
    """``lecture`` with every reference replaced by its variable's default.

    For the targets nothing can move in: PowerPoint, the book, the transcript
    sheet. Each control learns its variable, so it can print itself as a line
    of text. A bound figure needs nothing: its file is drawn at the defaults.
    """
    declared = {var.name: var for var in lecture.variables}
    return _resolve(lecture, _spell_static(declared), {})


def for_deck(lecture: model.Lecture, bound: dict[str, Figure] | None = None) -> model.Lecture:
    """``lecture`` as the Marp deck draws it.

    Sizes become CSS custom properties with the default as fallback, each
    control is told its variable, and each picture of a bound figure (one of
    ``bound``, see :func:`figures`) is marked ``"figure": src`` so the viewer
    can find its source.
    """
    declared = {var.name: var for var in lecture.variables}
    return _resolve(lecture, _spell_deck(declared), bound or {})


def page_names(page: model.Page, bound: dict[str, Figure] | None = None) -> list[str]:
    """The variables ``page`` uses, in the order its blocks first use them.

    With ``bound`` (see :func:`figures`), the inputs of the bound figures it
    shows count too.
    """
    names: list[str] = []
    for block in page.blocks:
        if block.CONTROL and isinstance(block.content, dict):
            names.append(block.content.get("var"))
        for _, value in references(block):
            names.extend(names_in(value))
        for src in image_sources(block):
            if bound and src in bound:
                names.extend(bound[src].inputs)
    return list(dict.fromkeys(n for n in names if n))


def page_figures(page: model.Page, bound: dict[str, Figure]) -> list[Figure]:
    """The bound figures ``page`` shows, each once."""
    srcs = [src for block in page.blocks for src in image_sources(block) if src in bound]
    return [bound[src] for src in dict.fromkeys(srcs)]


def page_table(lecture: model.Lecture, page: model.Page,
               bound: dict[str, Figure] | None = None) -> str | None:
    """What ``page`` needs from ``vars.js``: definitions and figure sources.

    A ``<script type="application/json" data-lk-vars>`` with the definitions
    of the variables the page uses, and for each bound figure it shows a
    ``<script type="image/svg+xml" data-lk-figure="src">`` holding the SVG,
    which ``vars.js`` evaluates as the variables move. Per page, so a deck cut
    down with ``--pages`` still carries what its pages need, and inline, so a
    deck opened from disk works without a server. Never displayed; the PDF
    ignores it.
    """
    declared = {var.name: var for var in lecture.variables}
    bound = bound or {}
    names = [n for n in page_names(page, bound) if n in declared]
    if not names:
        return None
    table = {"lecture": lecture.id,
             "vars": {n: definition(declared[n]) for n in names}}
    body = json.dumps(table, ensure_ascii=False).replace("</", "<\\/")
    out = [f'<script type="application/json" data-lk-vars>{body}</script>']
    for figure in page_figures(page, bound):
        if "</script" in figure.svg.lower():
            raise model.ValidationError(
                f"{figure.src}: a bound figure cannot contain '</script'"
            )
        out.append(f'<script type="image/svg+xml" data-lk-figure="{_attr(figure.src)}">'
                   f"{figure.svg.strip()}</script>")
    return "\n".join(out)


def _attr(text: str) -> str:
    return text.replace("&", "&amp;").replace('"', "&quot;").replace("<", "&lt;")


def control_places(content: dict) -> int:
    """The decimals a control's value keeps at the least: a slider's step's."""
    step = content.get("step_used")
    return decimals(step) if step is not None else 0


def control_text(content: dict) -> str:
    """A control as one line of inline markdown, for a target that cannot draw it.

    ``$T$ = 1.0（取值 0 – 10，刻度 0、1、10）``: the label (the variable's name as
    a math symbol when none is given), the default the page is drawn at, the
    range, and the marks a slider shows, each as written. Nothing the deck
    shows is left out. ``content`` is a control's, after :func:`for_static`
    told it its variable.
    """
    from . import i18n

    var = content["variable"]
    lang = content.get("lang")
    fields = {
        "label": content.get("label") or f"${var['name']}$",
        "value": number_text(var["default"], control_places(content)),
        "min": number_text(var["min"]),
        "max": number_text(var["max"]),
    }
    marks = content.get("marks")
    if marks:
        fields["marks"] = i18n.ui(lang, "list_sep").join(number_text(m) for m in marks)
        return i18n.ui(lang, "control_marks").format(**fields)
    return i18n.ui(lang, "control_range").format(**fields)
