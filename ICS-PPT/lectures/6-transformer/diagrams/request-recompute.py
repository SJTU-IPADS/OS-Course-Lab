#!/usr/bin/env python3
"""request-recompute: every request of Claude Code starts with one system prompt.

Left to right, one row per user and request:
- three users, A, B, C, each a head-and-shoulders glyph with its name below
  and a speech bubble holding a different question;
- one box 「Claude Code 客户端」 that all three users' arrows enter: they
  run the same client. Inside it a code panel in the deck's code style (grey
  background, a bar down its left edge) holding the system prompt as one
  schematic line, its first sentence and an ellipsis;
- from the box, requests 1, 2, 3 (users A, B, C), each a prompt of
  prompt_bar.py: the system prompt, then that user's question written out.
  The system prompt has the same width in all three and is the longest
  segment;
- the three requests enter a box 「Anthropic 服务器」 with a small server
  glyph.
The system prompt of request 1 is grey: request 1 computes its K, V. In
requests 2 and 3 it is hatched: an earlier request already computed it and
this request computes it again. A legend under the requests names the
hatching. Nothing is reused here: the page asks whether the K, V can be
computed once.
"""
from prompt_bar import *

H = 304
MIDS = (60, 140, 220)              # centre line of each row
TAG_UP = 9                         # request name baseline above its segments
LEGEND_Y = 290                     # baseline of the legend
BOX_TOP, BOX_BOT = 12, 258         # the client and server boxes span the rows

UX = 40                            # centre of the user glyphs
BUB_X, BUB_SIZE = 74, 17           # left edge and text size of the bubbles
CLIENT_X = 228                     # left edge of the client box
ARROW_GAP = 28                     # length of an arrow between two parts
SERVER_W = 104

CODE_SIZE, CODE_LH, CODE_PAD, CODE_BAR = 15, 22, 10, 4
CODE = [
    "// system prompt",
    "systemPrompt =",
    '  "You are Claude Code,',
    "   Anthropic's official",
    '   CLI for Claude. …"',
]
COMMENT_ROWS = {0}

USERS = [
    ("用户 A", "修复登录报错", "请求 1"),
    ("用户 B", "解释这个函数", "请求 2"),
    ("用户 C", "补充单元测试", "请求 3"),
]


def person(cx, mid):
    """A head-and-shoulders glyph centred on the row's centre line."""
    base, top = mid + 15, mid + 1
    return [circle(cx, mid - 9, 8, MUTED, MUTED, 1),
            path(f"M {cx - 16:.1f} {base:.1f} C {cx - 16:.1f} {top - 3:.1f} "
                 f"{cx + 16:.1f} {top - 3:.1f} {cx + 16:.1f} {base:.1f} Z",
                 MUTED, 1, fill=MUTED, head=False)]


def bubble(x, mid, s):
    """A speech bubble whose tail points left, at the user. Returns
    (elements, right edge)."""
    w, h, r = width(s, BUB_SIZE) + 20, 34, 6
    t, b_, rt = mid - h / 2, mid + h / 2, x + w
    d = (f"M {x + r:.1f} {t:.1f} H {rt - r:.1f} Q {rt:.1f} {t:.1f} {rt:.1f} {t + r:.1f} "
         f"V {b_ - r:.1f} Q {rt:.1f} {b_:.1f} {rt - r:.1f} {b_:.1f} H {x + r:.1f} "
         f"Q {x:.1f} {b_:.1f} {x:.1f} {b_ - r:.1f} V {mid + 5:.1f} L {x - 8:.1f} {mid:.1f} "
         f"L {x:.1f} {mid - 5:.1f} V {t + r:.1f} Q {x:.1f} {t:.1f} {x + r:.1f} {t:.1f} Z")
    return [path(d, LINE, 1.4, fill=WHITE, head=False),
            label(x + w / 2, mid + BUB_SIZE * 0.36, s, BUB_SIZE)], rt


def code_panel(x, y):
    """The system prompt as source code. Returns (elements, width, height)."""
    tw = max(mono_width(s, CODE_SIZE) for s in CODE)
    w = CODE_BAR + 2 * CODE_PAD + tw
    h = len(CODE) * CODE_LH + 2 * CODE_PAD - 6
    out = [rect(x, y, w, h, FILL_GREY, "none", rx=4, width=0),
           rect(x, y, CODE_BAR, h, LINE, "none", rx=0, width=0)]
    for i, s in enumerate(CODE):
        out.append(mono(x + CODE_BAR + CODE_PAD, y + CODE_PAD + 13 + i * CODE_LH, s,
                        CODE_SIZE, MUTED if i in COMMENT_ROWS else INK))
    return out, w, h


def client():
    """The client box, its title above the code panel, both centred in the
    box. Returns (elements, right edge)."""
    title_h, gap, pad = 18, 16, 12
    panel_els, pw, ph = code_panel(0, 0)
    w = pw + 2 * pad
    top = BOX_TOP + (BOX_BOT - BOX_TOP - title_h - gap - ph) / 2
    out = [rect(CLIENT_X, BOX_TOP, w, BOX_BOT - BOX_TOP, WHITE, DATA, rx=8, width=1.6),
           label(CLIENT_X + w / 2, top + title_h * 0.85, "Claude Code 客户端", 18, INK,
                 "bold")]
    out.append(f'<g transform="translate({CLIENT_X + pad:.1f} {top + title_h + gap:.1f})">')
    out += panel_els
    out.append("</g>")
    return out, CLIENT_X + w


def server(x):
    """The server box: a rack of three units above the name."""
    w, cx = SERVER_W, x + SERVER_W / 2
    uw, uh, ug = 46, 12, 4
    name_h = 18 + 24
    rack_h = 3 * uh + 2 * ug
    top = BOX_TOP + (BOX_BOT - BOX_TOP - rack_h - 18 - name_h) / 2
    out = [rect(x, BOX_TOP, w, BOX_BOT - BOX_TOP, WHITE, DATA, rx=8, width=1.6)]
    for k in range(3):
        uy = top + k * (uh + ug)
        out += [rect(cx - uw / 2, uy, uw, uh, FILL_DATA, DATA, rx=2, width=1.2),
                line(cx - uw / 2 + 6, uy + uh / 2, cx - uw / 2 + 18, uy + uh / 2, DATA, 1.2),
                circle(cx + uw / 2 - 7, uy + uh / 2, 2, DATA, DATA, 1)]
    ny = top + rack_h + 18 + 15
    out += [label(cx, ny, "Anthropic", 18, INK, "bold"),
            label(cx, ny + 24, "服务器", 18, INK, "bold")]
    return out


def main():
    b = []
    box_els, client_right = client()
    b += box_els
    bar_x = client_right + ARROW_GAP
    ends = []
    for k, ((user, question, request), mid) in enumerate(zip(USERS, MIDS)):
        b += person(UX, mid)
        b.append(label(UX, mid + 36, user, 16, INK, "bold"))
        els, bub_right = bubble(BUB_X, mid, question)
        b += els
        b.append(arrow(bub_right + 2, mid, CLIENT_X - 2, mid, INK, 1.8))

        top = mid - SEG_H / 2
        b.append(label(bar_x, top - TAG_UP, request, 17, INK, "bold", "start"))
        b.append(arrow(client_right + 2, mid, bar_x - 2, mid, INK, 1.8))
        els, spans = bar(bar_x, top, ["系统提示词", question],
                         hatched={0} if k else set())
        b += els
        ends.append((spans[-1][1], mid))
    server_x = max(x for x, _ in ends) + ARROW_GAP
    for x, mid in ends:
        b.append(arrow(x + 2, mid, server_x - 2, mid, INK, 1.8))
    b += server(server_x)
    b += hatch_key(bar_x, LEGEND_Y, "前面的请求已经计算过")[0]
    save("request-recompute", b, H)


main()
