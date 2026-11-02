#!/usr/bin/env python3
"""LRU replacement in one set of a 2-way cache: the blocks A, B, A, C, B, all mapped to
the same set, one frame per access. The lower table lists the accesses so far with the
result and the line replaced; above it is the set after the access, each line
with the number of the access that last used it. The line with the smaller number is the
least recently used one and is the next to be replaced. Hits, misses and replacements
come from the simulation below. Run it to refresh ../assets/sa-lru-1.svg ... sa-lru-5.svg.
"""

from cachekit import *

W, H = 520, 440
SEQ = ["A", "B", "A", "C", "B"]
GEO = Geo(vw=38, tw=64, bw=112, lh=40, size=19)
TX, CX, Y0 = 40, 136, 48
COL = {"n": 30, "blk": 132, "res": 250, "rep": 340}


def simulate(seq, ways=2):
    """LRU in one set; returns one dict per access."""
    lines = [None] * ways            # block per line
    used = [0] * ways                # number of the access that last used the line
    out = []
    for now, blk in enumerate(seq, 1):
        hit = blk in lines
        old = None
        if hit:
            k = lines.index(blk)
        else:
            free = [j for j, b in enumerate(lines) if b is None]
            k = free[0] if free else min(range(ways), key=lambda j: used[j])
            old = lines[k]
            lines[k] = blk
        used[k] = now
        out.append(dict(blk=blk, hit=hit, line=k, old=old, lines=list(lines),
                        used=list(used)))
    return out


RES = simulate(SEQ)


def note(n):
    r = RES[n - 1]
    blk = r["blk"]
    if r["hit"]:
        other = r["lines"][1 - r["line"]]
        return f"访问 {blk}：hit，{blk} 成为最近使用的行，{other} 成为最久未用的行"
    if r["old"] is None:
        return f"访问 {blk}：miss，组内有无效的行，{blk} 放入第 {r['line'] + 1} 行"
    return f"访问 {blk}：miss，组内各行都有效，替换最久未用的行 {r['old']}"


def frame(n):
    out = []
    for name, key in (("次序", "n"), ("同组的块", "blk"), ("结果", "res"), ("替换", "rep")):
        out.append(text(TX + COL[key], 188, name, 17, MUTED, "bold"))
    out.append(line(TX - 10, 200, TX + 450, 200, LINE, 1.4))
    for i in range(n):
        r = RES[i]
        y = 208 + i * 36
        if i == n - 1:
            out.append(rect(TX - 10, y - 2, 460, 34, FILL_GREY, LINE, rx=4, width=1))
        base = y + 22
        out.append(text(TX + COL["n"], base, str(i + 1), 18, INK, "bold"))
        out.append(text(TX + COL["blk"], base, r["blk"], 20, INK, "bold", font=MONO))
        out += badge(TX + COL["res"], base - 7, "hit" if r["hit"] else "miss",
                     "hit" if r["hit"] else "miss", size=16)
        if r["old"]:
            out.append(text(TX + COL["rep"] - 20, base, f"替换 {r['old']}", 17, RED, "bold",
                            "start"))
    r = RES[n - 1]
    both = None not in r["lines"]
    lru = min(range(2), key=lambda j: r["used"][j])
    out.append(text(CX + GEO.vw / 2, Y0 - 12, "有效位", 17, MUTED))
    out.append(text(CX + GEO.vw + GEO.tw / 2, Y0 - 12, "块", 17, TAG, "bold"))
    out.append(text(CX + GEO.vw + GEO.tw + GEO.bw / 2, Y0 - 12, "上次使用", 17, MUTED))
    sets = [[]]
    for k in range(2):
        blk = r["lines"][k]
        kw = dict(v=0) if blk is None else dict(v=1, tag=blk, data=[f"第 {r['used'][k]} 次"])
        if k == r["line"]:
            kw["halo"] = GREEN if r["hit"] else RED
        sets[0].append(kw)
    shapes, pos, bottom = cache_sets(CX, Y0, GEO, 1, sets, set_gap=0, headers=False,
                                     group=True, labels=False, label_dx=16)
    out += shapes
    out.append(text(CX - 22, (Y0 + bottom) / 2 + 6, "组 0", 19, IDX, "bold", "end"))
    for k in range(2):
        y = pos[(0, k)][1] + GEO.lh / 2 + 6
        x = CX + GEO.width(1) + 22
        if r["lines"][k] is None:
            out.append(text(x, y, "无效", 17, MUTED, anchor="start"))
        elif both and k == lru:
            out.append(text(x, y, "最久未用", 18, RED, "bold", "start"))
        elif both:
            out.append(text(x, y, "最近使用", 18, GREEN, "bold", "start"))
        else:
            out.append(text(x, y, "最近使用", 18, GREEN, "bold", "start"))
    out.append(text(CX + GEO.width(1) / 2, bottom + 30, "同一组的 2 行（E = 2），LRU 替换", 17, MUTED))
    caption_lines = []
    current = ""
    parts = note(n).split("，")
    for index, part in enumerate(parts):
        piece = part + ("，" if index < len(parts) - 1 else "")
        candidate = current + piece
        if current and svgkit._advance(candidate, 18, FONT) > 470:
            caption_lines.append(current)
            current = piece
        else:
            current = candidate
    caption_lines.append(current)
    assert len(caption_lines) <= 2
    for index, caption in enumerate(caption_lines):
        out.append(text(W / 2, 424 + index * 26, caption, 18, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, len(SEQ) + 1):
        save(f"sa-lru-{n}", W, H, frame(n))
