#!/usr/bin/env python3
"""The roles of the query, the key and the value, for the last token of the sentence.

The query of the last token is compared with the key of every token of the input. A label
beside each key says what the key stands for in this example, a bar shows how well the key
matches (the weights of the last row of kv_qkv.py), and the values are merged into the
information the last token takes, a thicker line for a larger share. The best match is
outlined. The labels under the result say where it goes: into the vector of the last
token, which predicts the next token. Run it to refresh ../assets/kv-qkv-roles.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from kv_qkv import WEIGHTS

W, H = 900, 220
N = 4
CW, CH = 64, 24                           # a vector cell
RY, RP = 44, 34                           # top of the first row, row pitch
QX = 92                                   # the query cell
KX = 372                                  # the key cells
BX, BAR = KX + CW + 50, 200               # the match bars: left edge, length of weight 1
VX = 604                                  # the value cells
JX = 752                                  # where the values join
OX = 790                                  # the result cell
MID = RY + CH / 2 + (N - 1) * RP / 2      # the vertical centre of the rows
HEAD = RY - 12
SHARE = WEIGHTS[-1]
BEST = max(range(N), key=lambda j: SHARE[j])
WANTED = "它形容的事物"                    # what the query of the last token looks for
TRAITS = ["时间", "事物", "程度", "性质"]    # what each key stands for


def vec(x, cy, label, strong=False):
    fill, stroke = KINDS["mid"]
    return [rect(x, cy - CH / 2, CW, CH, fill, INK if strong else stroke, rx=3,
                 width=2.2 if strong else 1.4),
            mono(x + CW / 2, cy + 4.6, label, 13, INK, "bold", "middle")]


def figure():
    last = WORDS[N - 1]
    out = [text(QX + CW / 2, MID - CH / 2 - 9, f"「{last}」的查询", 15, INK, "bold"),
           text(QX + CW / 2, MID + CH / 2 + 19, f"要找：{WANTED}", 14, MUTED, "bold"),
           text(KX - 10, HEAD, "输入的 token", 14, MUTED, "bold", "end"),
           text(KX + CW / 2, HEAD, "键", 15, INK, "bold"),
           text(BX + 50, HEAD, "匹配的程度", 14, MUTED, "bold"),
           text(VX + CW / 2, HEAD, "值", 15, INK, "bold"),
           text(JX - 12, HEAD, "按匹配的程度相加", 14, MUTED, "bold"),
           text(OX + CW / 2, MID - CH / 2 - 9, f"「{last}」的输出", 15, INK, "bold"),
           text(OX + CW / 2, MID + CH / 2 + 19, f"加入「{last}」的向量，", 14, MUTED, "bold"),
           text(OX + CW / 2, MID + CH / 2 + 37, "用于预测下一个 token", 14, MUTED, "bold"),
           text(222, 66, "比较", 14, MUTED, "bold")]
    out += vec(QX, MID, f"q{N}")
    for j in range(N):
        cy = RY + CH / 2 + j * RP
        best = j == BEST
        tone = INK if best else MUTED
        out.append(arrow(QX + CW + 5, MID, KX - 52, cy, tone, 2.2 if best else 1.4))
        out.append(text(KX - 10, cy + 5, WORDS[j], 15, INK, "bold" if best else "normal",
                        "end"))
        out += vec(KX, cy, f"k{j + 1}", best)
        out.append(text(KX + CW + 8, cy + 5, TRAITS[j], 14, tone,
                        "bold" if best else "normal", "start"))
        out.append(rect(BX, cy - 6, BAR * SHARE[j], 12, FILL_GREY, tone, rx=2,
                        width=2 if best else 1.2))
        out += vec(VX, cy, f"v{j + 1}", best)
        out.append(line(VX + CW + 5, cy, JX, MID, tone, 1 + 6 * SHARE[j]))
    out.append(arrow(JX, MID, OX - 6, MID, INK, 2.2))
    out += vec(OX, MID, f"o{N}")
    out.append(text(W / 2, H - 10,
                    f"「{last}」的查询与输入中每个 token 的键比较，键越匹配，从这个 token 取得的值越多",
                    17, INK, "bold"))
    return out


if __name__ == "__main__":
    save("kv-qkv-roles", W, H, figure())
