#!/usr/bin/env python3
"""Report figure content that runs off its own canvas.

Right-aligned CJK labels hung off the left of an axis are the usual way this
happens, and the renderer clips them without a word.  A grid whose last row is
computed from a loop counter is the other way, and that one clips a box rather
than a label, so rectangles are checked as well.  Reads every SVG named on the
command line (default: all of ../assets), writes one line per offence.
"""

import pathlib
import re
import sys

PAT = re.compile(r'<text x="([-\d.]+)" y="([-\d.]+)"[^>]*font-size="([\d.]+)"'
                 r'[^>]*text-anchor="(\w+)"[^>]*>([^<]*)</text>')

RECT = re.compile(r'<rect x="([-\d.]+)" y="([-\d.]+)" '
                  r'width="([\d.]+)" height="([\d.]+)"')


def advance(s, size):
    """Rough text width: CJK is one em, latin about 0.55."""
    return sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in s) * size


def offences(path):
    src = path.read_text(encoding="utf-8")
    m = re.search(r'viewBox="0 0 ([\d.]+) ([\d.]+)"', src)
    if not m:
        return [(path.name, "no viewBox")]
    w, h = float(m.group(1)), float(m.group(2))
    out = []
    for x, y, size, anchor, body in PAT.findall(src):
        x, y, size = float(x), float(y), float(size)
        wide = advance(body, size)
        left = {"end": x - wide, "middle": x - wide / 2}.get(anchor, x)
        right = left + wide
        if left < 2 or right > w - 2 or y > h - 2 or y < size:
            out.append((path.name, f"x {left:.0f}..{right:.0f} y {y:.0f} "
                                   f"(canvas {w:.0f}x{h:.0f})  {body!r}"))
    for x, y, rw, rh in RECT.findall(src):
        x, y, rw, rh = float(x), float(y), float(rw), float(rh)
        if x < 0 or y < 0 or x + rw > w or y + rh > h:
            out.append((path.name, f"rect {x:.0f},{y:.0f} {rw:.0f}x{rh:.0f} "
                                   f"(canvas {w:.0f}x{h:.0f})"))
    return out


def main(argv):
    paths = [pathlib.Path(a) for a in argv[1:]]
    if not paths:
        paths = sorted((pathlib.Path(__file__).resolve().parent.parent
                        / "assets").glob("*.svg"))
    bad = [o for p in paths for o in offences(p)]
    for name, msg in bad:
        print(f"{name}: {msg}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
