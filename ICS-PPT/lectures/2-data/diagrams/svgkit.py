"""Primitives shared by this lecture's figure generators.

Every figure here is the same handful of shapes: a row of cells holding bit or
byte values, a bracket naming a span of them, a label hanging off the left.
Keeping the shapes in one place is what makes the figures look like one set.
"""

FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"
MONO = "SFMono-Regular, Menlo, Consolas, monospace"
SERIF = "Latin Modern Math, STIX Two Math, Cambria Math, Times New Roman, serif"

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE, FILL_GREY = "#eaf3f7", "#fce9df", "#eef2f4"


def esc(s):
    """SVG text is XML: an unescaped & or < silently voids the whole file."""
    return str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def svg(w, h, body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" '
            f'width="{w}" height="{h}">\n' + "\n".join(body) + "\n</svg>")


def text(x, y, s, size=13, fill=INK, weight="normal", anchor="middle", font=FONT):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{font}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{esc(s)}</text>')


def rect(x, y, w, h, fill, stroke, rx=3, width=1.3, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{width}"{d}/>')


def line(x1, y1, x2, y2, stroke=LINE, width=1.6, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{stroke}" stroke-width="{width}"{d}/>')


def cells(x, y, values, cw, ch, fill, stroke, size=11.5, font=MONO, tone=INK):
    """A run of equal cells, one value each. Returns (shapes, right edge)."""
    out = []
    for i, v in enumerate(values):
        cx = x + i * cw
        out.append(rect(cx, y, cw, ch, fill, stroke))
        if v != "":
            out.append(text(cx + cw / 2, y + ch / 2 + size * 0.36, v, size, tone,
                            font=font))
    return out, x + len(values) * cw


def brace(x0, x1, y, color, label, size=12.5, depth=7, below=True):
    """A square bracket spanning [x0, x1] with a label outside it."""
    s = 1 if below else -1
    d = f"M {x0:.1f} {y:.1f} L {x0:.1f} {y + s * depth:.1f} " \
        f"L {x1:.1f} {y + s * depth:.1f} L {x1:.1f} {y:.1f}"
    ty = y + s * depth + (size + 3 if below else -5)
    return [f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.4"/>',
            text((x0 + x1) / 2, ty, label, size, color, "bold")]


def rich(x, y, parts, size=14, fill=INK, weight="normal", anchor="middle"):
    """One text line mixing plain runs, italic variables and subscripts.

    parts: str for a plain run, ("v", s) for an italic variable, ("_", s) for
    a subscript of the variable before it.
    """
    spans, low = [], 0.0
    for p in parts:
        kind, s = ("", p) if isinstance(p, str) else p
        drop = size * 0.28 if kind == "_" else 0.0
        dy = f' dy="{drop - low:.1f}"' if drop != low else ""
        low = drop
        if kind == "v":
            spans.append(f'<tspan font-family="{SERIF}" font-style="italic" '
                         f'font-size="{size * 1.12:.1f}"{dy}>{esc(s)}</tspan>')
        elif kind == "_":
            spans.append(f'<tspan font-family="{SERIF}" font-size="{size * 0.75:.1f}"'
                         f'{dy}>{esc(s)}</tspan>')
        else:
            spans.append(f'<tspan{dy}>{esc(s)}</tspan>')
    # preserve: the spaces at the edges of a run are part of the text
    return (f'<text xml:space="preserve" x="{x:.1f}" y="{y:.1f}" '
            f'font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">'
            + "".join(spans) + "</text>")
