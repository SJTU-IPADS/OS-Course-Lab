#!/usr/bin/env python3
"""Replay a memory trace through the cache of lab 2 and print the lines
mini-cpu --stats would print for it.

    mini-cpu --cache S,W --memtrace t.txt program
    python3 tools/cachemodel.py S W < t.txt
    python3 tools/cachemodel.py 0 < t.txt       for mini-cpu --cache 0

The trace comes from frame/memtrace.c: r PA N and w PA N for cache_read
and cache_write, f for cache_flush, t VA for translations (ignored here).
Every r and w record lies within one line and is one line access; a
record that crosses a line boundary is an error. The rules are those of
src/lab2/README.md: 64-byte lines, set = line number mod S, write-back,
write allocation, least recently used way evicted. S = 0 is the machine
without a cache (mini-cpu --cache 0): every access is one DRAM transfer,
a read or a write.
"""
import sys
from collections import OrderedDict

LINE = 64


def line_of(fields):
    """The line number of an r or w record, which lies within one line."""
    pa, nbytes = int(fields[1], 16), int(fields[2])
    if nbytes < 1 or pa % LINE + nbytes > LINE:
        sys.exit("cachemodel.py: %s %s %s does not lie within one line" % tuple(fields[:3]))
    return pa // LINE


def uncached(trace):
    reads = writes = 0
    for record in trace:
        fields = record.split()
        if fields and fields[0] == "r":
            line_of(fields)
            reads += 1
        elif fields and fields[0] == "w":
            line_of(fields)
            writes += 1
    print("cache off")
    print("dram  line reads %d  line writes %d" % (reads, writes))


def cached(trace, set_count, way_count):
    sets = [OrderedDict() for set_index in range(set_count)]  # tag -> dirty, oldest first
    hits = misses = writebacks = 0

    def touch(line, write):
        nonlocal hits, misses, writebacks
        ways = sets[line % set_count]
        tag = line // set_count
        if tag in ways:
            hits += 1
            ways.move_to_end(tag)
        else:
            misses += 1
            if len(ways) == way_count:
                evicted, dirty = ways.popitem(last=False)
                writebacks += dirty
            ways[tag] = False
        if write:
            ways[tag] = True

    for record in trace:
        fields = record.split()
        if fields and fields[0] in ("r", "w"):
            touch(line_of(fields), fields[0] == "w")
        elif fields and fields[0] == "f":
            for ways in sets:
                for tag, dirty in ways.items():
                    writebacks += dirty
                    ways[tag] = False

    accesses = hits + misses
    print("cache accesses %d  hits %d  misses %d  hit rate %.2f%%  writebacks %d"
          % (accesses, hits, misses, 100.0 * hits / accesses if accesses else 0.0, writebacks))
    print("dram  line reads %d  line writes %d" % (misses, writebacks))


def main(argv):
    if argv == ["0"]:
        uncached(sys.stdin)
    elif len(argv) == 2 and int(argv[0]) > 0:
        cached(sys.stdin, int(argv[0]), max(int(argv[1]), 1))
    else:
        sys.exit("usage: cachemodel.py SETS WAYS < trace, or cachemodel.py 0 < trace (no cache)")


if __name__ == "__main__":
    main(sys.argv[1:])
