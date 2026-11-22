"""The example of the third subsection of part three.

Three sessions share a pool of 96 blocks, one token of the real model each,
and have cached A 42, B 19, C 21 tokens. B sends a request: 20 tokens of
text and a message of 17, at most 8 tokens of answer. A and C follow with a
request each. The numbers are those of the contract
(build/scratch-kvblocks/contract/expected.txt, from the state after its
fourth turn). kv_memory.py, kv_reserve.py and kv_policies.py share them,
and each page draws the setting its numbers rest on. The pool is drawn to
scale, one cell a block.
"""

from kv_kit import *

N_BLOCKS = 96
MAX_ANSWER = 8
B_TEXT, B_MESSAGE = 20, 17              # the text of B so far, its new message
NEED = 44                               # the tokens of B that serve as input
# the requests that arrive from START on: session, tokens of text, blocks it needs
REQUESTS = [("B", 37, NEED), ("A", 60, 67), ("C", 36, 43)]


def session(n_cached, last_use, *runs):
    """runs: (first block, blocks), in the order of the block table."""
    return dict(n_cached=n_cached, capacity=sum(count for _, count in runs), last_use=last_use,
                runs=list(runs))


# the state before the request of B, the same under both policies
START = {"A": session(42, 2, (0, 42)), "B": session(19, 3, (42, 19)), "C": session(21, 4, (61, 21))}


def in_use(sessions):
    return sum(entry["capacity"] for entry in sessions.values())


def num_free(sessions):
    return N_BLOCKS - in_use(sessions)


def block_list(entry):
    return [first + k for first, count in entry["runs"] for k in range(count)]


def ranges(entry):
    return "、".join(f"{first}–{first + count - 1}" for first, count in entry["runs"]) or "无"


class Scale:
    """96 blocks side by side, each UNIT wide."""

    def __init__(self, x0, unit):
        self.x0, self.unit = x0, unit

    def x(self, block):
        return self.x0 + block * self.unit

    @property
    def x1(self):
        return self.x(N_BLOCKS)

    def segments(self, sessions):
        """(first, blocks, session or None, valid blocks) over the pool, left to right."""
        out = []
        for who, entry in sessions.items():
            left = entry["n_cached"]
            for first, count in entry["runs"]:
                out.append((first, count, who, max(0, min(count, left))))
                left -= count
        out.sort()
        full, at = [], 0
        for part in out:
            if part[0] > at:
                full.append((at, part[0] - at, None, 0))
            full.append(part)
            at = part[0] + part[1]
        if at < N_BLOCKS:
            full.append((at, N_BLOCKS - at, None, 0))
        return full

    def pool(self, sessions, bit_y, y, h, before=None, freed=(), read=False, size=16, free_names=True):
        """The bitmap, one bit a block, the blocks, and whose they are."""
        was = {b for e in (before or sessions).values() for b in block_list(e)}
        busy = {b for e in sessions.values() for b in block_list(e)}
        valid = {b for e in sessions.values() for b in block_list(e)[:e["n_cached"]]}
        out = []
        for block in range(N_BLOCKS):
            x, bit = self.x(block), int(block in busy)
            changed = (block in was) != (block in busy)
            kind = "new" if changed else "reuse" if read and not bit else None
            fill, stroke = KINDS[kind] if kind else (WHITE, LINE)
            out.append(rect(x, bit_y, self.unit, 21, fill, stroke, rx=0, width=1.2 if kind else 0.8))
            out += tagged(f"bit/{block}", [mono(x + self.unit / 2, bit_y + 15.5, str(bit), 12,
                                                ORANGE if changed else INK, "bold" if changed else
                                                "normal", anchor="middle")])
            look = ("freed" if block in freed else "cache" if block in valid
                    else "reserved" if block in busy else "free")
            fill, stroke = KINDS[look]
            out.append(rect(x, y, self.unit, h, fill, stroke, rx=0, width=0.9,
                            dash="3 2" if look in DASHED else None))
        for first, count, who, n_valid in self.segments(sessions):
            x, w = self.x(first), count * self.unit
            if who:
                colour = OWNER[who]
                out.append(rect(x, y, w, h, "none", colour, rx=0, width=2.4))
                out.append(rect(x + 1, y + h + 3, w - 2, 22, colour if n_valid else WHITE, colour,
                                rx=3, width=1.6))
                label = f"{who}：{count} 块" if n_valid else f"{who} 预留 {count} 块"
                out += tagged(f"run/{first}/{count}",
                              [text(x + w / 2, y + h + 19.5, label, size,
                                    WHITE if n_valid else colour, "bold")])
            elif free_names and w >= 40:
                out += tagged(f"run/{first}/{count}",
                              [text(x + w / 2, y + h + 19.5, f"空闲 {count}", size, MUTED, "bold")])
        return out

    def ticks(self, blocks, base):
        return [mono(self.x(b) + self.unit / 2, base, str(b), 14, BLUE, "bold", anchor="middle")
                for b in blocks]
