#!/usr/bin/env python3
"""generation: predict, sample, append, repeat.

1: the seed text goes into the Transformer, which returns a probability for
   every candidate next word (the twelve the video shows, then the rest).
2: a random sample picks "woodland", a word of low probability.
3: "woodland" is appended to the text.
4: the longer text goes back into the model, which returns a new
   distribution; the arrows close the loop.
"""
from tk import *

H = 400
SEED = "Behold, a wild pi creature, foraging in its native"
CANDS = [("land", .22), ("forest", .09), ("country", .05), ("habitat", .04),
         ("forests", .04), ("soil", .04), ("territory", .02), ("woods", .02),
         ("lands", .01), ("waters", .01), ("woodland", .01), ("grass", .01)]
NEW = [.18, .15, .07, .06, .05, .04, .03, .03, .02, .02, .02, .01]
PICK = "woodland"

TB_X, TB_Y, TB_W, TB_H = 40, 16, 640, 48      # the text box
MX, MY, MW, MH = 70, 196, 180, 68             # the Transformer box
BX, BY, BW, BH, GAP = 420, 92, 200, 18, 5     # the bars
PMAX = .22
ROWY = [BY + i * (BH + GAP) + BH / 2 for i in range(len(CANDS))]
DX, DY = 760, ROWY[10]                        # the die, level with woodland


def textline(appended):
    tail = (f'<tspan fill="{GREEN}" font-weight="bold"> {PICK}</tspan>' if appended
            else f'<tspan fill="{LINE}"> ________</tspan>')
    return (f'<text x="{TB_X + 18:.1f}" y="{TB_Y + 31:.1f}" font-family="{FONT}" '
            f'font-size="20" fill="{INK}" text-anchor="start">{esc(SEED)}{tail}</text>')


def die(cx, cy, s=40):
    out = [rect(cx - s / 2, cy - s / 2, s, s, WHITE, INK, rx=7, width=1.8)]
    for dx, dy in ((-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)):
        out.append(circle(cx + dx * s * 0.25, cy + dy * s * 0.25, 3.4, INK, INK, 0.5))
    return out


def frame(n):
    b = []
    b.append(rect(TB_X, TB_Y, TB_W, TB_H, FILL_DATA, DATA, rx=6, width=1.4))
    b.append(textline(n >= 3))
    b.append(rect(MX, MY, MW, MH, FILL_BLUE, BLUE, rx=6, width=1.6))
    b.append(label(MX + MW / 2, MY + MH / 2 + 7, "Transformer", 20, INK, "bold"))
    b.append(arrow(MX + 90, TB_Y + TB_H + 2, MX + 90, MY - 4, INK, 2))
    b.append(arrow(MX + MW + 6, MY + MH / 2, BX - 94 if n < 4 else BX + 4, MY + MH / 2, INK, 2))

    if n < 4:
        els, _ = hbars(BX, BY, CANDS, bar_w=BW, bar_h=BH, gap=GAP, size=17,
                       pick=PICK if n >= 2 else None, pmax=PMAX)
        b += els
        b.append(label(BX - 30, BY + 12 * (BH + GAP) + 18, "⋮", 20, MUTED))
    else:
        for y, p in zip(ROWY, NEW):
            b.append(rect(BX + 12, y - BH / 2, max(2.0, BW * p / PMAX), BH, FILL_DATA,
                          DATA, rx=2, width=1.4))
        b.append(label(BX + 12 + BW * NEW[0] / PMAX + 12, ROWY[0] + 6, "新的分布", 18,
                       INK, anchor="start"))
        b.append(label(BX + 18, BY + 12 * (BH + GAP) + 18, "⋮", 20, MUTED))

    if n >= 2:
        b += die(DX, DY)
        b.append(label(DX + 34, DY - 4, "随机采样", 19, INK, "bold", anchor="start"))
    if n in (2, 3):
        b.append(arrow(DX - 24, DY, BX + 60, DY, GREEN, 2))
        b.append(note(DX + 34, DY + 22, "低概率的词也可能被选中", anchor="start"))
    if n == 4:
        b.append(arrow(DX - 24, DY, BX + 60, DY, MUTED, 2, dash="6 5"))
    if n >= 3:
        b.append(elbow([(DX, DY - 24), (DX, TB_Y + TB_H / 2), (TB_X + TB_W + 6, TB_Y + TB_H / 2)],
                       GREEN, 2))
        b.append(label(DX + 12, 196, "追加到文本末尾", 17, GREEN, anchor="start"))
    if n == 4:
        b.append(label(MX + 102, 132, "更长的文本再次输入", 17, INK, anchor="start"))
    save(f"generation-{n}", b, H)


for n in (1, 2, 3, 4):
    frame(n)
