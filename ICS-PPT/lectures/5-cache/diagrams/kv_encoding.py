#!/usr/bin/env python3
"""A computer stores text as character codes, and the codes say nothing about meaning.

Left: the sentence 今天天气很好 as six cells; under each character its three UTF-8 bytes,
computed with str.encode("utf-8"), so they are exact.
Right: a number line of Unicode code points. 佳 U+4F73 (20339), 她 U+5979 (22905) and
好 U+597D (22909) are placed to scale, with the unit of the axis fixed by 佳 and 好. 她 and
好 are 4 apart (under one pixel) and share one point: 好 is labelled above the axis and
她 below it. 好 and 佳 are close in meaning but 2570 apart; 她 is 4 from 好 and unrelated.
Only neutral colours are used. Run it to refresh ../assets/kv-encoding.svg.
"""

from kv_kit import *
from svgkit import circle

W, H = 1000, 226
SENTENCE = "今天天气很好"
CX, CW, CP = 30, 63, 68                   # cells: left edge, width, pitch
CY, CH = 80, 122                          # cell top and height
HEAD = 30                                 # baseline of the panel headers

AX0, AX1, AY = 490, 892, 144              # the number line
CODES = {"佳": ord("佳"), "她": ord("她"), "好": ord("好")}
X_LEFT, X_RIGHT = 520, 860                # x of 佳 and of 好
SCALE = (X_RIGHT - X_LEFT) / (CODES["好"] - CODES["佳"])


def xpos(ch):
    return X_LEFT + (CODES[ch] - CODES["佳"]) * SCALE


def utf8():
    out = [text(CX, HEAD, "UTF-8 编码", 17, INK, "bold", "start")]
    for i, ch in enumerate(SENTENCE):
        x = CX + i * CP
        out += cell(x, CY, CW, CH, "", "mid")
        out.append(text(x + CW / 2, CY + 30, ch, 17, INK, "bold"))
        out.append(line(x + 12, CY + 42, x + CW - 12, CY + 42, LINE, 1.2))
        for k, byte in enumerate(ch.encode("utf-8")):
            out.append(mono(x + CW / 2, CY + 66 + k * 22, f"{byte:02X}", 14, INK, anchor="middle"))
    return out


def label(ch, x, y):
    """Character above, code in mono below; y is the baseline of the character."""
    return [text(x, y, ch, 17, INK, "bold"),
            mono(x, y + 19, f"U+{CODES[ch]:04X}", 13, MUTED, anchor="middle")]


def numbers():
    out = [text(AX0, HEAD, "Unicode 编号", 17, INK, "bold", "start"),
           arrow(AX0, AY, AX1 - 4, AY, MUTED, 1.6)]
    xl, xr = xpos("佳"), xpos("好")
    # 佳 above the axis, 好 above and 她 below the shared point
    out += label("佳", xl, AY - 36) + label("好", xr, AY - 36)
    out.append(line(xl, AY - 12, xl, AY - 3, LINE, 1.4))
    out.append(line(xr, AY - 12, xr, AY - 3, LINE, 1.4))
    out += label("她", xr, AY + 44)
    out.append(line(xr, AY + 3, xr, AY + 28, LINE, 1.4))
    out += [circle(xl, AY, 5, INK, INK), circle(xr, AY, 5, INK, INK)]
    # 佳 to 好: a bracket over the two labels
    by = AY - 76
    d = f"M {xl:.1f} {by + 8} L {xl:.1f} {by} L {xr:.1f} {by} L {xr:.1f} {by + 8}"
    out.append(f'<path d="{d}" fill="none" stroke="{MUTED}" stroke-width="1.6"/>')
    out.append(text((xl + xr) / 2, by - 8, "相差 2570，含义相近", 15, INK, "bold"))
    # 好 to 她: a bracket on the right spanning the label above and the label below
    bx, y0, y1 = xr + 34, AY - 52, AY + 50
    d = f"M {bx - 8} {y0} L {bx} {y0} L {bx} {y1} L {bx - 8} {y1}"
    out.append(f'<path d="{d}" fill="none" stroke="{MUTED}" stroke-width="1.6"/>')
    out.append(text(bx + 12, AY - 4, "相差 4", 15, INK, "bold", "start"))
    out.append(text(bx + 12, AY + 17, "含义无关", 15, INK, "bold", "start"))
    return out


if __name__ == "__main__":
    save("kv-encoding", W, H, utf8() + numbers() + [line(462, 20, 462, H - 16, LINE, 1.2)])
