"""The attention of one head drawn as matrices, shared by kv_attention.py, kv_naive.py and
kv_cache.py, so that the three animations put every matrix at the same place.

Every matrix has one row per token. X (the token vectors) is multiplied by the parameters
W_Q, W_K, W_V into Q, K and V, Q and K give the weight matrix A, A and V give the output O,
and the last row of O produces the next token. The layout is a data flow from left to
right: X on the left, the three products stacked in one column, A to the right of Q and K,
O to the right of A, the next token at the far right.

Modes:
  prefill  every row is grey: N tokens processed at once;
  naive    a decoding step without a cache: the new row is orange, the rows already
           computed in the previous step are computed again (red); stage 2 greys out the
           rows the next token does not need;
  cache    a decoding step with a KV cache: only the new row of X, Q, A and O exists,
           the old rows of K and V are read from the cache (blue).
"""

from kv_kit import *

W, H, LEFT = 916, 432, 28                # the canvas is x in [LEFT, LEFT + W]
WORDS = ["今天", "天气", "很", "好", "适合", "出门", "散步"]
NMAX = 6                                  # most rows in any frame
RH = 15                                   # row height
XX, XW = 84, 100                          # X: left edge, width
WX, WS = 234, 50                          # W squares: left edge, size
MX, MW = 310, 100                         # Q, K, V: left edge, width
QT, KT, VT = 24, 152, 272                 # tops of Q, K, V
AX = 500                                  # A: left edge (square cells of RH)
AT = 88                                   # A: top
OX, OW = 700, 100                         # O: left edge, width
NX, NWD = 856, 72                         # the next token: left edge, width
CAPTION_Y, LEGEND_Y = 396, 422


def centre(top):
    """The vertical centre of a block of NMAX rows starting at top."""
    return top + NMAX * RH / 2


def drawn(top, kinds, only=None):
    """The vertical centre of the rows of a matrix that are drawn (or of kind `only`)."""
    idx = [i for i, k in enumerate(kinds) if k is not None and only in (None, k)]
    return top + (idx[0] + idx[-1] + 1) * RH / 2


def first(top, kinds):
    """The top of the first row that is drawn: a matrix label sits just above it."""
    return top + next(i for i, k in enumerate(kinds) if k is not None) * RH


def sub_label(x, y, base, sub, size=17, color=INK, anchor="middle"):
    """A name with a subscript, W_Q: the base letter and a smaller subscript."""
    bw, sw = size * 0.92, size * 0.5
    left = x - (bw + sw) / 2 if anchor == "middle" else x
    return [text(left, y, base, size, color, "bold", "start"),
            text(left + bw, y + size * 0.28, sub, size * 0.68, color, "bold", "start")]


def rows(x, top, w, kinds, prefix):
    """A matrix drawn as stacked rows; kinds[i] is the kind of row i + 1, None = absent."""
    out = []
    for i, kind in enumerate(kinds):
        if kind is None:
            continue
        y = top + i * RH
        fill, stroke = KINDS[kind]
        dash = "4 3" if kind in ("empty", "gone") else None
        out.append(rect(x, y, w, RH, fill, stroke, rx=2, width=1.3, dash=dash))
        tone = MUTED if kind in ("empty", "gone") else INK
        out.append(mono(x + w / 2, y + RH / 2 + 4.3, f"{prefix}{i + 1}", 12, tone,
                        "bold", "middle"))
    return out


def grid(x, top, kinds, n):
    """The weight matrix A: row i keeps columns 1..i (causal mask), the rest is masked."""
    out = []
    for i, kind in enumerate(kinds):
        if kind is None:
            continue
        for j in range(n):
            k = kind if j <= i else "empty"
            fill, stroke = KINDS[k]
            dash = "3 3" if k in ("empty", "gone") else None
            out.append(rect(x + j * RH, top + i * RH, RH, RH, fill, stroke, rx=1,
                            width=1.1, dash=dash))
    return out


def label(x, top, kinds, name, dims):
    return [text(x, first(top, kinds) - 9, f"{name}：{dims}", 15, INK, "bold", "start")]


def figure(mode, n, stage, caption, items):
    """One frame. n = rows (tokens so far, the new one included); stage counts the steps
    drawn so far (prefill 1..3) or picks the variant (naive 1 or 2)."""
    out = []
    old = n - 1
    if mode == "prefill":
        xk = qk = kk = ak = ok = ["mid"] * n
        dims = dict(x="N × d", q="N × d", a="N × N", o="N × d")
    elif mode == "naive":
        xk = ["mid"] * old + ["new"]
        kk = ["waste"] * old + ["new"]
        qk = ak = ok = kk if stage == 1 else ["gone"] * old + ["new"]
        dims = dict(x="(N+1) × d", q="(N+1) × d", a="(N+1) × (N+1)", o="(N+1) × d")
    else:
        xk = qk = ak = ok = [None] * old + ["new"]
        kk = ["cache"] * old + ["new"]
        size = "N+1" if n == 5 else f"N+{n - 4}"
        dims = dict(x="1 × d", q="1 × d", a=f"1 × ({size})", o="1 × d", k=f"({size}) × d")
    kdims = dims.get("k", dims["q"])
    show_a = mode != "prefill" or stage >= 2
    show_o = mode != "prefill" or stage >= 3

    # the tokens and X
    xt = KT
    out += label(XX, xt, xk, "X", dims["x"])
    for i in range(n):
        new = mode != "prefill" and i == n - 1
        tone = ORANGE if new else (MUTED if xk[i] is None else INK)
        out.append(text(XX - 10, xt + i * RH + RH / 2 + 5, WORDS[i], 13, tone,
                        "bold" if new else "normal", "end"))
    out += rows(XX, xt, XW, xk, "x")
    xc = drawn(xt, xk)
    only = "new" if mode == "cache" else None      # with a cache only the new row is computed

    # the parameters and the three products
    for top, name, kinds, d in ((QT, "Q", qk, dims["q"]), (KT, "K", kk, kdims),
                                (VT, "V", kk, kdims)):
        c = centre(top)
        out.append(arrow(XX + XW + 4, xc, WX - 4, c, MUTED, 1.6))
        out.append(rect(WX, c - WS / 2, WS, WS, *KINDS["param"], rx=4, width=1.6))
        out += sub_label(WX + WS / 2, c + 5, "W", name, 17)
        out.append(arrow(WX + WS + 2, c, MX - 4, drawn(top, kinds, only), MUTED, 1.6))
        out += label(MX, top, kinds, name, d)
        out += rows(MX, top, MW, kinds, name.lower())
    if mode == "cache":
        y0, y1 = KT - 28, VT + NMAX * RH + 6
        out.append(rect(MX - 8, y0, MW + 16, y1 - y0, "none", BLUE, rx=6, width=2.2,
                        dash="7 4"))
        out.append(text(MX + MW + 16, y1 - 4, "KV Cache", 16, BLUE, "bold", "start"))

    # A = softmax(mask(Q K^T) / sqrt(d))
    if show_a:
        ac = drawn(AT, ak)
        out.append(arrow(MX + MW + 4, drawn(QT, qk), AX - 6, ac - 6, MUTED, 1.6))
        out.append(arrow(MX + MW + 4, drawn(KT, kk), AX - 6, ac + 6, MUTED, 1.6))
        out.append(text(450, (drawn(QT, qk) + drawn(KT, kk)) / 2 + 5, "Q Kᵀ", 15, MUTED,
                        "bold"))
        out += label(AX, AT, ak, "A", dims["a"])
        out += grid(AX, AT, ak, n)

    # O = A V, and the next token from its last row
    if show_o:
        ac = drawn(AT, ak)
        out.append(arrow(AX + n * RH + 6, ac, OX - 6, ac, MUTED, 1.6))
        out.append(text((AX + n * RH + OX) / 2, ac - 8, "× V", 15, MUTED, "bold"))
        vx = OX + OW / 2
        vc = drawn(VT, kk)
        out.append(elbow([(MX + MW + 4, vc), (vx, vc),
                          (vx, AT + n * RH + 6)], MUTED, 1.6))
        out += label(OX, AT, ok, "O", dims["o"])
        out += rows(OX, AT, OW, ok, "o")
        ly = AT + (n - 1) * RH + RH / 2
        out.append(arrow(OX + OW + 4, ly, NX - 6, ly, ORANGE, 2.2))
        out += cell(NX, ly - 15, NWD, 30, "", "new")
        out.append(text(NX + NWD / 2, ly + 6, WORDS[n], 16, INK, "bold"))
        out.append(text(NX + NWD / 2, ly - 26, "下一个 token", 14, MUTED, "bold"))

    out.append(text(LEFT + W / 2, CAPTION_Y, caption, 19, INK, "bold"))
    out += legend(LEFT + (W - legend_width(items)) / 2, LEGEND_Y, items)
    return out
