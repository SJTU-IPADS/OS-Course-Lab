#!/usr/bin/env python3
"""dot_product on w[8] and x[8] (int, 32 bytes each) in a direct-mapped cache with
S = 2 sets and B = 16 bytes (4 ints), C = 32 bytes. Frame 1: w at address 0 and x
at address 32; w[i] and x[i] map to the same set, every read evicts the block the
next read needs, 16 reads and 16 misses. Frame 2: x starts at address 48 after a
16-byte pad; w[i] and x[i] map to different sets, 4 misses. The hits and misses
come from the simulation below. Run it to refresh ../assets/dm-conflict-1.svg and
dm-conflict-2.svg.
"""

from cachekit import *

W, H = 1000, 316
BLOCK = 16
SETS = 2
MX0, MW, MY = 60, 150, 52


def simulate(x_base):
    """Reads w[0], x[0], w[1], x[1], ...; returns a list of (name, hit)."""
    state, out = {}, []
    for i in range(8):
        for name, addr in (("w", 4 * i), ("x", x_base + 4 * i)):
            blk = addr // BLOCK
            st = blk % SETS
            hit = state.get(st) == blk
            state[st] = blk
            out.append((f"{name}[{i}]", hit))
    return out


def frame(n):
    out = []
    base = 32 if n == 1 else 48
    # memory strip: blocks of 16 bytes, washed by set
    blocks = [("w[0..3]", 0), ("w[4..7]", 16), ("x[0..3]", base), ("x[4..7]", base + 16)]
    if n == 2:
        blocks.insert(2, ("填充", 32))
    for k, (name, addr) in enumerate(blocks):
        st = (addr // BLOCK) % SETS
        x = MX0 + k * MW
        pad = name == "填充"
        out.append(rect(x, MY, MW - 6, 62, WHITE if pad else SET_FILLS[st * 2],
                        MUTED if pad else GREEN, rx=2, width=1.6, dash="5 3" if pad else None))
        out.append(text(x + (MW - 6) / 2, MY + 26, name, 18, MUTED if pad else INK, "bold",
                        font=MONO if not pad else FONT))
        out.append(text(x + (MW - 6) / 2, MY + 50, f"地址 {addr}–{addr + 15}", 14, MUTED))
        if not pad:
            out.append(text(x + (MW - 6) / 2, MY + 86, f"组 {st}", 17, IDX, "bold"))
    out.append(text(MX0 - 12, MY + 36, "内存", 17, GREEN, "bold", "end"))
    # reads
    res = simulate(base)
    cw = 56
    x0 = 30
    for row in range(2):
        for k in range(8):
            name, hit = res[row * 8 + k]
            x = x0 + (row * 8 + k) % 8 * 120
            y = 180 + row * 46
            col, fill = (GREEN, FILL_GREEN) if hit else (RED, FILL_RED)
            out.append(rect(x, y, 112, 38, fill, col, rx=6, width=1.6))
            out.append(text(x + 8, y + 25, name, 16, INK, "bold", "start", font=MONO))
            out.append(text(x + 106, y + 25, "hit" if hit else "miss", 14, col, "bold", "end"))
    misses = sum(1 for _, h in res if not h)
    out.append(text(W / 2, 298, f"16 次读取，{misses} 次 miss", 20, RED if n == 1 else GREEN,
                    "bold"))
    return out


if __name__ == "__main__":
    for n in (1, 2):
        save(f"dm-conflict-{n}", W, H, frame(n))
