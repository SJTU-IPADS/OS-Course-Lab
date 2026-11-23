#!/usr/bin/env python3
"""The five turns of one conversation, and the text it has at the end of each.

Each turn appends the message, in the chat format of the model, and an
answer of at most 8 tokens to the text of the conversation. Row n is the
text at the end of turn n: the text of the earlier turns as one bar, then
the message and the answer of this turn, one cell a token. The counts are
those of the real model: every answer reached 8 tokens, and turn n computes
up to its n_prompt, 13, 35, 57, 81 and 103 tokens of text.
Run it to refresh ../assets/kv-turns.svg.
"""

from kv_kit import *

W, H = 979, 330

MESSAGES = ["用一句话介绍你自己。", "什么是缓存？", "它为什么快？", "内存和硬盘有什么区别？",
            "哪一个更便宜？"]
TURNS = [(13, 8), (14, 8), (14, 8), (16, 8), (14, 8)]       # message tokens, answer tokens
CIRCLED = "①②③④⑤"

LX = 8                                  # the messages
SX = 280                                # the text
UNIT = 5.6                              # one token
ROW_Y, PITCH, LH = 70, 46, 22
LEGEND_Y = 316


def row_y(turn):
    return ROW_Y + turn * PITCH


def messages():
    out = [text(LX, 30, "一段对话的 5 轮：用户的消息", 16, INK, "bold", "start")]
    for turn, message in enumerate(MESSAGES):
        base = row_y(turn) + LH / 2 + 6
        out.append(text(LX, base, CIRCLED[turn], 16, MUTED, anchor="start"))
        out.append(text(LX + 24, base, message, 16, INK, anchor="start"))
    return out


def texts():
    out = [text(SX, 30, "每轮结束时对话的文本，一格是千问模型的一个 token。格上方：消息 + 回答", 16, INK, "bold",
                "start")]
    earlier = 0
    for turn, (n_msg, n_ans) in enumerate(TURNS):
        y = row_y(turn)
        if earlier:
            width = earlier * UNIT
            out.append(rect(SX, y, width, LH, FILL_GREY, LINE, rx=2, width=1))
            out += tagged(f"earlier/{turn + 1}", [text(SX + width / 2, y + LH / 2 + 5,
                                                       f"之前 {earlier} 个", 13, MUTED)])
        x0 = x = SX + earlier * UNIT
        for k in range(n_msg + n_ans):
            fill = LINE if k < n_msg else WHITE
            out.append(rect(x, y, UNIT, LH, fill, MUTED, rx=0.5, width=0.7))
            x += UNIT
        out += tagged(f"turn/{turn + 1}", [text((x0 + x) / 2, y - 6, f"{n_msg} + {n_ans}", 15, INK)])
        earlier += n_msg + n_ans
        out += tagged(f"total/{turn + 1}", [text(x + 8, y + LH / 2 + 5, f"共 {earlier} 个", 14, MUTED,
                                                 anchor="start")])
    return out


def figure():
    items = [("unused", "之前各轮的文本"), ("mid", "这一轮的消息（含对话格式的标记）"),
             ("empty", "这一轮的回答：逐个生成，最多 8 个")]
    leg = centred_legend(W / 2, LEGEND_Y, items)
    # The swatches follow the cells: no dashes, the message filled as drawn.
    leg = [s.replace('stroke-dasharray="5 4"', "") for s in leg]
    leg[2] = leg[2].replace(f'fill="{FILL_GREY}"', f'fill="{LINE}"').replace(
        f'stroke="{INK}"', f'stroke="{MUTED}"')
    leg[4] = leg[4].replace(f'stroke="{LINE}"', f'stroke="{MUTED}"')
    return messages() + texts() + leg


if __name__ == "__main__":
    save("kv-turns", W, H, figure())
