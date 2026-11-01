#!/usr/bin/env python3
"""One token through the model: where matvec_q4_0 is called, and with which
matrices.

Every matrix is drawn to one scale (1 px per 40 rows or columns), so the
widths of ffn_down and token_embd and the heights of attn_k and attn_v are
proportions, and the output matrix is cut by a break: 151936 rows would be
74 times the height of attn_q. At this scale every matrix, with its name
above and its shape below, stays inside its dashed box. The 28 layers are one box with a loop. The
shapes are those main.c finds through model.c (find_tensors): n_embd 2048,
n_ff 6144, 16 query heads and 8 key heads of 128.
Run it to refresh ../assets/pd-frame-token.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, LINE,
                    MUTED, ORANGE, WHITE, arrow, elbow, line, mono, rect, save,
                    text)

W, H = 1120, 360
S = 1 / 40                            # px per row or column
TOP, BOTTOM = 52, 320                 # the layer box, and the output matrix beside it
MID = (TOP + BOTTOM) / 2              # the line x travels along


def matrix(x, y, rows, cols, name, dims_below=True, label_right=False):
    """A Q4_0 matrix of rows x cols, drawn to scale, named."""
    w, h = cols * S, rows * S
    out = [rect(x, y, w, h, FILL_BLUE, BLUE, rx=2, width=1.4)]
    if label_right:
        out.append(mono(x + w + 8, y + h / 2 + 1, name, 12.5, INK, "bold"))
        out.append(text(x + w + 8, y + h / 2 + 16, f"{rows} × {cols}", 11.5, MUTED,
                        anchor="start"))
    else:
        out.append(mono(x + w / 2, y - 6, name, 12.5, INK, "bold", anchor="middle"))
        out.append(text(x + w / 2, y + h + 15, f"{rows} × {cols}", 11.5, MUTED))
    return out


def layer():
    out = [rect(150, TOP, 740, BOTTOM - TOP, WHITE, INK, rx=8, width=1.8),
           text(520, TOP + 24, "第 l 层（共 28 层）：7 次矩阵向量乘", 15, INK, "bold")]
    box_top, box_h, y = TOP + 36, BOTTOM - TOP - 50, TOP + 76
    # attention: attn_q, attn_k, attn_v, attn_output
    out.append(rect(170, box_top, 300, box_h, WHITE, LINE, rx=6, width=1.2, dash="4 3"))
    out.append(text(320, box_top + 18, "注意力部分：4 次", 13, MUTED))
    out += matrix(200, y, 2048, 2048, "attn_q")
    out += matrix(290, y, 1024, 2048, "attn_k", label_right=True)
    out += matrix(290, y + 46, 1024, 2048, "attn_v", label_right=True)
    out += matrix(200, y + 98, 2048, 2048, "attn_output")
    # feed-forward: ffn_gate, ffn_up, ffn_down
    out.append(rect(490, box_top, 380, box_h, WHITE, LINE, rx=6, width=1.2, dash="4 3"))
    out.append(text(680, box_top + 18, "前馈部分：3 次", 13, MUTED))
    out += matrix(510, y, 6144, 2048, "ffn_gate")
    out += matrix(592, y, 6144, 2048, "ffn_up")
    out += matrix(680, y, 2048, 6144, "ffn_down")
    return out


def output():
    x, y, w, h = 930, TOP, 2048 * S, BOTTOM - TOP
    cut = y + 115
    out = [rect(x, y, w, h, FILL_BLUE, BLUE, rx=2, width=1.4),
           rect(x - 4, cut, w + 8, 14, WHITE, "none", rx=0, width=0),
           line(x - 4, cut, x + w + 4, cut + 6, BLUE, 1.4),
           line(x - 4, cut + 8, x + w + 4, cut + 14, BLUE, 1.4),
           mono(x + w / 2, y - 6, "token_embd", 12.5, INK, "bold", anchor="middle"),
           text(x + w / 2, y + h + 15, "151936 × 2048", 11.5, MUTED),
           text(x + w / 2, y + h + 31, "输出层，1 次", 11.5, MUTED)]
    return out


def build():
    out = []
    # input: the token's row of token_embd, decoded by embed
    out.append(text(60, MID - 65, "Token", 14, INK, "bold"))
    out.append(text(60, MID - 45, "embed：", 12, MUTED))
    out.append(text(60, MID - 29, "取 token_embd", 12, MUTED))
    out.append(text(60, MID - 13, "的一行为 x", 12, MUTED))
    out.append(arrow(104, MID, 146, MID, INK))
    out += layer()
    # the loop over the 28 layers
    out.append(elbow([(890, MID), (910, MID), (910, 30), (130, 30), (130, MID), (146, MID)],
                     ORANGE, 2, head=False))
    out.append(text(520, 22, "x 经过 28 层，每层 7 次：28 × 7 = 196 次", 14, ORANGE, "bold"))
    # the output layer and argmax
    out += output()
    out.append(arrow(910, MID, 926, MID, INK))
    out.append(arrow(930 + 2048 * S, MID, 1016, MID, INK))
    out.append(text(1068, MID - 15, "argmax", 14, INK, "bold"))
    out.append(text(1068, MID + 5, "分数最高的", 12, MUTED))
    out.append(text(1068, MID + 21, "下一个 Token", 12, MUTED))
    # the summary, level with the labels of the output matrix
    out.append(text(520, BOTTOM + 31, "生成一个 Token：196 + 1 = 197 次 matvec_q4_0，矩阵的每个权重各参与一次乘加",
                    15, INK, "bold"))
    return out


if __name__ == "__main__":
    save("pd-frame-token", W, H, build())
