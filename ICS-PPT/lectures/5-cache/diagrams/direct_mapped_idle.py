#!/usr/bin/env python3
"""The direct-mapped structure with sets 1, 2 and 3 explicitly idle.

Run this script to refresh ../assets/direct-mapped-idle.svg.
"""

from direct_mapped import H, W, frame
from cachekit import save


if __name__ == "__main__":
    save("direct-mapped-idle", W, H, frame(idle_sets=(1, 2, 3)))
