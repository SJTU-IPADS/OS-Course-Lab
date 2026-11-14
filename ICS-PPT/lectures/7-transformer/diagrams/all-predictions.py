#!/usr/bin/env python3
"""all-predictions: every final-layer vector predicts the token after its own.

One frame. Bottom: the input tokens; middle: their final-layer vectors;
top: the token each vector predicts (the next one in the text). In training
all of these predictions are made at once; in generation only the last
vector's prediction (orange) is used.
"""
from tk import *

H = 380
WORDS = ["a", "fluffy", "blue", "creature", "roamed", "the", "verdant", "forest"]
NEXT = WORDS[1:] + ["…"]
X0, PITCH = 150, 108
PY, VY, IY = 44, 142, 300      # tops of the predicted chips, vectors, input chips
CW, CH, NR = 34, 20, 5


def grey(i, j):
    s = 0.15 + 0.6 * (pseudo("final", i, j) + 1) / 2
    g = round(255 - s * 80)
    return f"#{g - 8:02x}{g - 3:02x}{g:02x}"


def main():
    b = []
    last = len(WORDS) - 1
    for j, (w, nxt) in enumerate(zip(WORDS, NEXT)):
        cx = X0 + j * PITCH
        hl = j == last
        c = ORANGE if hl else DATA
        # input token
        els, _ = chip(cx, IY, w, "data", size=19, anchor="middle")
        b += els
        b.append(arrow(cx, IY - 4, cx, VY + NR * CH + 8, LINE, 1.6))
        # its final-layer vector
        for i in range(NR):
            b.append(rect(cx - CW / 2, VY + i * CH, CW, CH, grey(i, j), WHITE, rx=0, width=0.8))
        b.append(rect(cx - CW / 2 - 2, VY - 2, CW + 4, NR * CH + 4, "none", c, rx=2,
                      width=2.2 if hl else 1.2))
        # the token it predicts
        b.append(arrow(cx, VY - 6, cx, PY + 42, c, 2 if hl else 1.6))
        els, _ = chip(cx, PY, nxt, "hl" if hl else "data", size=19, anchor="middle",
                      dash="4 3" if hl else None)
        b += els
    left = X0 - (width(WORDS[0], 19) + 20) / 2
    right = X0 + last * PITCH + (width(NEXT[-1], 19) + 20) / 2
    b += brace(left, right, PY - 6, INK, "训练时同时预测", size=18, below=False)
    for y, s in ((PY + 24, "预测"), (VY + NR * CH / 2 + 6, "最后一层"), (IY + 24, "输入")):
        b.append(note(24, y, s, anchor="start"))
    b.append(text(X0 + last * PITCH + (width("forest", 19) + 20) / 2, IY + 64,
                  "生成时只用这一个", 19, ORANGE, "bold", "end"))
    save("all-predictions", b, H, left=-30)


main()
