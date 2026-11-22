#!/usr/bin/env python3
"""Seven requests of three sessions in the order they arrive, and the text
each session has at the end, token by token.

Each request appends the message, in the chat format of the model, and an
answer of at most 8 tokens to the text of its session. The two counts stand
above each piece of text. They are those of the real model: the n_prompt of
the contract, and every answer reached 8 tokens.
Run it to refresh ../assets/kv-sessions.svg.
"""

from kv_kit import *

W, H = 1040, 340

WORKLOAD = [("A", "用一句话介绍你自己。"), ("A", "你能帮我做什么？"), ("B", "什么是缓存？"),
            ("C", "内存和硬盘有什么区别？"), ("B", "为什么缓存比内存快？"),
            ("A", "请推荐一本计算机系统的书。"), ("C", "哪一个更便宜？")]
# session -> [(turn, message tokens, answer tokens)]
TEXT = {"A": [(1, 13, 8), (2, 14, 8), (6, 17, 8)],
        "B": [(3, 12, 8), (5, 17, 8)],
        "C": [(4, 14, 8), (7, 14, 8)]}
CIRCLED = "①②③④⑤⑥⑦"

LX, LW = 20, 300                        # the listing of the requests
SX = 400                                # the session strips
UNIT = 7.6
GAP = 6                                 # between two turns
LANE_Y = {"A": 92, "B": 168, "C": 244}
LH = 24                                 # strip height


def listing():
    out = [rect(LX, 40, LW, 30 + 7 * 30, WHITE, MUTED, rx=6, width=1.4),
           text(LX + 14, 30, "依次到来的请求：会话与消息", 16, INK, "bold", "start")]
    for i, (session, message) in enumerate(WORKLOAD):
        y = 70 + i * 30
        out.append(text(LX + 16, y, CIRCLED[i], 16, MUTED, anchor="start"))
        out.append(mono(LX + 42, y, session, 16, OWNER[session], "bold"))
        out.append(text(LX + 64, y, message, 16, INK, anchor="start"))
    return out


def strips():
    out = [text(SX, 30, "每个会话的文本，一格一个 token。格上方：先后、消息 + 回答", 16, INK, "bold", "start")]
    for session, turns in TEXT.items():
        y = LANE_Y[session]
        out.append(mono(SX - 34, y + LH / 2 + 6, session, 18, OWNER[session], "bold"))
        x = SX
        for turn, n_msg, n_ans in turns:
            x0 = x
            for k in range(n_msg + n_ans):
                kind_fill = (LINE, MUTED) if k < n_msg else (WHITE, MUTED)
                out.append(rect(x, y, UNIT, LH, kind_fill[0], kind_fill[1], rx=1, width=0.9))
                x += UNIT
            out += tagged(f"text/{session}/{turn}",
                          [text((x0 + x) / 2, y - 8, f"{CIRCLED[turn - 1]} {n_msg} + {n_ans}", 16, INK)])
            x += GAP
        out.append(text(x + 6, y + LH / 2 + 6, f"{sum(m + a for _, m, a in turns)} 个 token",
                        15, MUTED, anchor="start"))
    return out


def figure():
    out = listing() + strips()
    items = [("mid", "消息（含对话格式的标记）：接在会话的文本之后"), ("empty", "回答：逐个生成，每次最多 8 个")]
    # The answer is drawn white with a solid outline; the legend swatch follows it.
    leg = centred_legend(W / 2, H - 14, items)
    leg[0] = leg[0].replace(f'fill="{FILL_GREY}"', f'fill="{LINE}"')
    leg = [s.replace('stroke-dasharray="5 4"', "").replace(f'stroke="{LINE}"', f'stroke="{MUTED}"')
           for s in leg]
    return out + leg


if __name__ == "__main__":
    save("kv-sessions", W, H, figure())
