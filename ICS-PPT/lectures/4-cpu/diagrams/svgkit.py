"""Primitives shared by this lecture's figure generators.

The figures are boxes, arrows, rows of cells and short code listings. Keeping
the shapes and the palette in one place is what makes them look like one set.
Every figure is drawn at the width it is shown on the slide, so a font size
here is a size on the 1280x720 slide.
"""

import pathlib

FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"
MONO = "SFMono-Regular, Menlo, DejaVu Sans Mono, Consolas, monospace"
MONO_EM = 0.6                     # advance of one monospace character, in em

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
GREEN, RED = "#2e7d4f", "#c0392b"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE, FILL_GREY = "#eaf3f7", "#fce9df", "#eef2f4"
FILL_GREEN, FILL_RED = "#e5f3ea", "#fbe7e5"
WHITE = "#ffffff"

ARROW_COLORS = (BLUE, ORANGE, INK, GREEN, RED, MUTED, LINE)

ASSETS = pathlib.Path(__file__).resolve().parent.parent / "assets"


def esc(s):
    """SVG text is XML: an unescaped & or < silently voids the whole file."""
    return str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _marker(color):
    return (f'<marker id="ah-{color[1:]}" viewBox="0 0 10 10" refX="9" refY="5" '
            f'markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
            f'<path d="M0,0 L10,5 L0,10 z" fill="{color}"/></marker>')


def svg(w, h, body, defs=(), left=0):
    """The canvas is the window x in [left, left + w], y in [0, h] over the drawing.

    A figure shown across the page is centred on the slide by its canvas, so
    the drawing has to sit in the middle of it. When labels hang off one side
    and the ink ends up off-centre, `left` moves the window instead of every
    coordinate: left=-20 shows the drawing 20 px further right. margins.py
    prints the blank margin on each side of every figure.
    """
    head = "<defs>" + "".join(_marker(c) for c in ARROW_COLORS) + "".join(defs) + "</defs>"
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{left:g} 0 {w} {h}" '
            f'width="{w}" height="{h}">\n{head}\n' + "\n".join(body) + "\n</svg>\n")


def save(name, w, h, body, defs=(), left=0):
    path = ASSETS / f"{name}.svg"
    path.write_text(svg(w, h, body, defs, left), encoding="utf-8")
    print(path)


def text(x, y, s, size=17, fill=INK, weight="normal", anchor="middle", font=FONT):
    # Runs of spaces align listing columns; SVG collapses them unless told not
    # to, and Chrome honors xml:space only on the <text> element itself.
    keep = ' xml:space="preserve"' if "  " in str(s) or str(s).startswith(" ") else ""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{font}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}"{keep}>{esc(s)}</text>')


def mono(x, y, s, size=16, fill=INK, weight="normal", anchor="start"):
    return text(x, y, s, size, fill, weight, anchor, MONO)


def mono_width(s, size):
    return len(s) * size * MONO_EM


def rect(x, y, w, h, fill, stroke, rx=4, width=1.4, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{width}"{d}/>')


def line(x1, y1, x2, y2, stroke=LINE, width=1.6, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{stroke}" stroke-width="{width}"{d}/>')


def arrow(x1, y1, x2, y2, color=INK, width=2, dash=None, both=False):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    m = f"url(#ah-{color[1:]})"
    start = f' marker-start="{m}"' if both else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{color}" stroke-width="{width}"{d}{start} marker-end="{m}"/>')


def path(d, stroke=INK, width=2, fill="none", dash=None, head=True):
    ds = f' stroke-dasharray="{dash}"' if dash else ""
    m = f' marker-end="url(#ah-{stroke[1:]})"' if head else ""
    return f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{width}"{ds}{m}/>'


def elbow(points, color=INK, width=2, dash=None, head=True):
    """An arrow through a list of (x, y) corners."""
    d = "M " + " L ".join(f"{x:.1f} {y:.1f}" for x, y in points)
    return path(d, color, width, dash=dash, head=head)


def circle(cx, cy, r, fill, stroke, width=1.4):
    return (f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="{fill}" '
            f'stroke="{stroke}" stroke-width="{width}"/>')


def box(x, y, w, h, label, fill=FILL_BLUE, stroke=BLUE, size=17, weight="bold",
        color=INK, font=FONT, sub=None, sub_size=14, rx=6):
    """A rounded box with a centred label and an optional second line."""
    out = [rect(x, y, w, h, fill, stroke, rx=rx, width=1.6)]
    if sub is None:
        out.append(text(x + w / 2, y + h / 2 + size * 0.36, label, size, color, weight,
                        font=font))
    else:
        out.append(text(x + w / 2, y + h / 2 - 3, label, size, color, weight, font=font))
        out.append(text(x + w / 2, y + h / 2 + sub_size + 4, sub, sub_size, MUTED))
    return out


def cells(x, y, values, cw, ch, fill, stroke, size=15, font=MONO, tone=INK):
    """A run of equal cells, one value each. Returns (shapes, right edge)."""
    out = []
    fills = fill if isinstance(fill, (list, tuple)) else [fill] * len(values)
    for i, v in enumerate(values):
        cx = x + i * cw
        out.append(rect(cx, y, cw, ch, fills[i], stroke, rx=2, width=1.2))
        if v != "":
            out.append(text(cx + cw / 2, y + ch / 2 + size * 0.36, v, size, tone,
                            font=font))
    return out, x + len(values) * cw


def brace(x0, x1, y, color, label, size=15, depth=8, below=True):
    """A square bracket spanning [x0, x1] with a label outside it."""
    s = 1 if below else -1
    d = (f"M {x0:.1f} {y:.1f} L {x0:.1f} {y + s * depth:.1f} "
         f"L {x1:.1f} {y + s * depth:.1f} L {x1:.1f} {y:.1f}")
    ty = y + s * depth + (size + 4 if below else -6)
    return [f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.6"/>',
            text((x0 + x1) / 2, ty, label, size, color, "bold")]


def vbrace(x, y0, y1, color, label, size=15, depth=8, right=True):
    """A vertical bracket spanning [y0, y1] with a label beside it."""
    s = 1 if right else -1
    d = (f"M {x:.1f} {y0:.1f} L {x + s * depth:.1f} {y0:.1f} "
         f"L {x + s * depth:.1f} {y1:.1f} L {x:.1f} {y1:.1f}")
    tx = x + s * (depth + 8)
    return [f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.6"/>',
            text(tx, (y0 + y1) / 2 + size * 0.36, label, size, color, "bold",
                 anchor="start" if right else "end")]


def listing(x, y, lines, size=16, lh=25, fill=INK, marks=None, width=None,
            mark_fill=FILL_ORANGE):
    """A code listing, one mono line per row; marks = {row: fill} washes rows.

    Tabs are expanded to the column the assembler listing would use, so
    `\tmovl\t(%rsi), %eax` lines up the way gcc -S prints it.
    """
    out = []
    marks = marks or {}
    w = width or max(mono_width(_expand(s), size) for s in lines) + 16
    for i, s in enumerate(lines):
        top = y + i * lh
        if i in marks:
            out.append(rect(x - 6, top + 2, w, lh - 2, marks[i] or mark_fill, "none",
                            rx=3, width=0))
        out.append(mono(x, top + lh * 0.72, _expand(s), size, fill))
    return out


def _expand(s):
    return s.expandtabs(8)


def label_line(x, y, parts, size=17):
    """One line built from (text, font, fill, weight) pieces laid left to right.

    CJK pieces are one em wide per character, latin about 0.55 em; mono pieces
    use MONO_EM. Good enough to butt a code token against a CJK word.
    """
    out = []
    for s, font, fill, weight in parts:
        out.append(text(x, y, s, size, fill, weight, "start", font))
        x += _advance(s, size, font)
    return out


def _advance(s, size, font):
    if font == MONO:
        return len(s) * size * MONO_EM
    return sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in s) * size
