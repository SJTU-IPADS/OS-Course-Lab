#!/usr/bin/env python3
"""The same matrix read column by column: addresses 0, 16, 32, 48, 4, 20, 36, 52 (stride 4).

Shares the drawing code of stride_1.py. Run it to refresh ../assets/stride-k-1.svg ... -8.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from stride_1 import H, W, frame
from svgkit import save

if __name__ == "__main__":
    for n in range(1, 9):
        save(f"stride-k-{n}", W, H, frame(False, n))
