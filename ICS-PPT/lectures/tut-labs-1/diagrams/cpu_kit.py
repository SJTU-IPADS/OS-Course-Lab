"""Shared colours and shapes of the mini-cpu figures (cpu_*.py).

One colour per segment of an instruction, kept across the whole part, and
one shade per field of the ModRM and SIB bytes. The opcode, operand-byte
and constant colours continue lecture 3 (diagrams/insn_bytes.py there):
opcode orange, operand bytes green, a constant blue. Running this file
does nothing; the figure scripts import it.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,  # noqa: F401
                    INK, LINE, MONO, MUTED, ORANGE, WHITE, cells, mono, rect, text)

PURPLE, FILL_PURPLE = "#7b4fa6", "#efe7f7"
TEAL, FILL_TEAL = "#1f8a8a", "#e3f3f3"

# (fill, stroke) of each segment of an instruction
PREFIX = (FILL_GREY, MUTED)         # 66, REX, VEX
OPCODE = (FILL_ORANGE, ORANGE)      # one byte, 0f xx, or the VEX opcode byte
MODRM = (FILL_GREEN, GREEN)         # the ModRM byte
SIB = (FILL_PURPLE, PURPLE)         # the SIB byte
DISP = (FILL_TEAL, TEAL)            # the displacement of a memory operand
IMM = (FILL_BLUE, BLUE)             # an immediate, or the offset of a jump

# the three fields of ModRM, three shades of its green; of SIB, of its purple
MOD = (WHITE, GREEN)
REG = (FILL_GREEN, GREEN)
RM = ("#bfdfc9", GREEN)
SCALE = (WHITE, PURPLE)
INDEX = (FILL_PURPLE, PURPLE)
BASE = ("#d9c8ec", PURPLE)

# the student's code and the framework's, where a figure tells them apart
TODO = (FILL_ORANGE, ORANGE)
GIVEN = (FILL_GREY, MUTED)

CW, CH = 38, 32                     # one byte cell


def byte_row(x, y, groups, cw=CW, ch=CH, size=15):
    """Byte cells coloured by segment: groups = [(["4c"], PREFIX), ...].

    Returns (shapes, right edge)."""
    out = []
    for values, (fill, stroke) in groups:
        shapes, x = cells(x, y, values, cw, ch, fill, stroke, size)
        out += shapes
    return out, x


def bit_fields(x, y, fields, bw=26, bh=30, size=15, label_size=14, labels=True):
    """One byte as bit cells grouped into fields.

    fields = [("00", MOD, "mod"), ("111", REG, "reg"), ("100", RM, "rm")].
    Each field is one cell per bit; its name is written under it.
    Returns (shapes, right edge)."""
    out = []
    for bits, (fill, stroke), name in fields:
        w = bw * len(bits)
        out.append(rect(x, y, w, bh, fill, stroke, rx=3, width=1.4))
        for i, b in enumerate(bits):
            out.append(text(x + bw * i + bw / 2, y + bh / 2 + size * 0.36, b, size,
                            INK, "bold", font=MONO))
        if labels:
            out.append(text(x + w / 2, y + bh + label_size + 4, name, label_size,
                            stroke, "bold"))
        x += w
    return out, x


def legend(x, y, items, size=14, gap=18):
    """Coloured swatches with labels in one line: items = [(label, (fill, stroke))]."""
    out = []
    for label, (fill, stroke) in items:
        out.append(rect(x, y - size + 2, 22, size + 2, fill, stroke, rx=3, width=1.2))
        out.append(text(x + 28, y, label, size, INK, anchor="start"))
        x += 28 + sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in label) * size + gap
    return out, x


def note(x, y, s, size=14, fill=MUTED, anchor="start"):
    return text(x, y, s, size, fill, anchor=anchor)


def code(x, y, s, size=15, fill=INK, weight="normal", anchor="start"):
    return mono(x, y, s, size, fill, weight, anchor)
