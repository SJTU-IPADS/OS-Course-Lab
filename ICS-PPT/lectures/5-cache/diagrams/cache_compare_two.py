#!/usr/bin/env python3
"""The same 8 cache lines in direct-mapped and fully associative caches.

Run it to refresh ../assets/cache-compare-two.svg.
"""

from cache_compare import build as build_panels
from cachekit import save

W, H = 720, 290
PANELS = [("直接映射", 8, 1, 75), ("全相联", 1, 8, 435)]


def build():
    return build_panels(PANELS)


if __name__ == "__main__":
    save("cache-compare-two", W, H, build())
