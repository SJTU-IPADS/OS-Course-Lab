#!/bin/sh
# Lab 2: the set-associative cache.
#
#   sh tests/lab2/run.sh [DIR]
#
# unit: mini-unit (tests/unit.c) calls cache_init, cache_read, cache_write
# and cache_flush directly on a few lines of memory, one call per line:
# the end of a line and the start of the next, write-back on eviction and
# on a flush only, the least recently used way, a second cache_init, a
# number of sets that is not a power of two.
#
# For a given sequence of accesses the counts are fixed by the rules of
# lab 2 (write-back, write allocation, LRU in a set). So every run writes
# its accesses with --memtrace (frame/memtrace.c), one line per access,
# and tools/cachemodel.py replays that trace; the lines "cache ..." and "dram ..." of --stats must
# equal its output. The random test draws the cache and the seed of
# guest/tinytf from SEED (see tests/lib.sh).
#
# The last three items measure what the cache saves. DRAM is the file
# dram.img, and every line read or written in DRAM bypasses the caches of
# the host (frame/cpu/dram.c). --cache 0 runs the same program without a
# cache, where every access is a line read or written in DRAM.
# Two items compare the line reads plus line writes of --stats. The
# wall-clock item compares the time of whole runs and takes
# the fastest of three runs of each kind.
LABNO=2 DIR=${1:-.}
. tests/lib.sh

# stats_of FILE: the lines of --stats that tools/cachemodel.py prints
stats_of() { grep -E '^(cache|dram) ' "$1"; }

# replay SETS WAYS PROGRAM ARGS...: the counts of mini-cpu --cache SETS,WAYS
# equal those of the model for the same accesses. The run must end normally
# and must have written a trace: a cache_read that is not implemented
# yet leaves both the trace and the counts empty, and empty equals empty.
replay() {
    _sets=$1; _ways=$2; shift 2
    cpu --stats --cache $_sets,$_ways --memtrace "$TMP/trace" "$@" > /dev/null 2> "$TMP/err" ||
        { echo "(--cache $_sets,$_ways $*: mini-cpu failed)"; head -n 3 "$TMP/err"; return 1; }
    [ -s "$TMP/trace" ] ||
        { echo "(--cache $_sets,$_ways $*: --memtrace wrote nothing)"; return 1; }
    is "$(python3 tools/cachemodel.py $_sets $_ways < "$TMP/trace")" "$(stats_of "$TMP/err")" &&
        return 0
    echo "(--cache $_sets,$_ways $*; expected: tools/cachemodel.py on the trace of --memtrace)"
    return 1
}
small()   { replay 64 1 $G/matmul && replay 16 4 $G/matmul; }
block()   { replay 16 1 $G/block 8 && out "sum = 501590016" cpu --cache 16,1 $G/block 8; }
# the memory is read and written a line at a time, once per miss and writeback
lines() {
    for cache in 64,4 16,1 4,2; do
        cpu --stats --cache $cache $G/matmul > /dev/null 2> "$TMP/err" ||
            { echo "(--cache $cache $G/matmul: mini-cpu failed)"; head -n 3 "$TMP/err"; return 1; }
        sed -n 's/^cache .* misses \([0-9]*\) .* writebacks \([0-9]*\)$/\1 \2/p;
                s/^dram  line reads \([0-9]*\)  line writes \([0-9]*\)$/\1 \2/p' "$TMP/err" > "$TMP/l"
        [ "$(wc -l < "$TMP/l")" -eq 2 ] && [ "$(sed -n 1p "$TMP/l")" = "$(sed -n 2p "$TMP/l")" ] ||
            { echo "--cache $cache: misses/writebacks and line reads/writes differ:"; cat "$TMP/l"; return 1; }
    done
}
same() {
    for cache in 1,1 8,1 16,2 256,8; do
        out "sum = 15420672" cpu --cache $cache $G/matmul || { echo "(--cache $cache)"; return 1; }
        out "scalar = -28
simd   = -28" cpu --cache $cache $G/simd || { echo "(--cache $cache)"; return 1; }
    done
}

# random: two random caches of up to 256 sets (a power of two) and up to 8 ways
random_() {
    for round in 1 2; do
        rand 9; sets=$((1 << RND)); rand 8; ways=$((RND + 1))
        rand 1000000; seed=$RND; rand 12; ntokens=$((RND + 1))
        out "$(python3 tests/reference.py tinytf $seed $ntokens)" \
            cpu --stats --cache $sets,$ways --memtrace "$TMP/trace" $G/tinytf $seed $ntokens &&
        is "$(python3 tools/cachemodel.py $sets $ways < "$TMP/trace")" "$(stats_of "$TMP/err")" ||
            { echo "(--cache $sets,$ways $G/tinytf $seed $ntokens)"; return 1; }
    done
}

# dram_lines OPTIONS... PROGRAM ARGS...: NLINES becomes the DRAM line
# reads plus line writes that mini-cpu --stats prints. The run must end
# normally.
dram_lines() {
    NLINES=
    cpu --stats "$@" > /dev/null 2> "$TMP/err" ||
        { echo "(mini-cpu --stats $*: failed)"; head -n 3 "$TMP/err"; return 1; }
    NLINES=$(sed -n 's/^dram  line reads \([0-9]*\)  line writes \([0-9]*\)$/\1 + \2/p' "$TMP/err")
    [ -n "$NLINES" ] && NLINES=$(($NLINES)) && return 0
    echo "(mini-cpu --stats $*: no line \"dram\")"
    return 1
}
# speedup: with the default cache the DRAM line reads and writes are at
# most 1/10 of those without a cache
speedup() {
    for program in "$G/matmul" "$G/rowcol rows"; do
        dram_lines $program || return 1
        cached=$NLINES
        dram_lines --cache 0 $program || return 1
        [ $((cached * 10)) -le "$NLINES" ] && continue
        echo "$program: $cached DRAM line reads and writes with --cache 64,4 and $NLINES with --cache 0;"
        echo "expected at most 1/10 with the cache"
        return 1
    done
}
# locality: summed by columns, rowcol misses on almost every element; by
# rows, on one element in 16
locality() {
    dram_lines $G/rowcol rows || return 1
    rows=$NLINES
    dram_lines $G/rowcol cols || return 1
    [ "$NLINES" -ge $((3 * rows)) ] && return 0
    echo "$G/rowcol: $rows DRAM line reads and writes by rows and $NLINES by columns, --cache 64,4;"
    echo "expected at least 3 times as many by columns"
    return 1
}
# fastest COMMAND...: USEC becomes the wall-clock time of the fastest of
# three runs of COMMAND, in microseconds
fastest() {
    USEC=
    for _run in 1 2 3; do
        _start=$(date +%s%N)
        "$@" > /dev/null 2> "$TMP/err" || { echo "($*: failed)"; head -n 3 "$TMP/err"; return 1; }
        _took=$(( ($(date +%s%N) - _start) / 1000 ))
        if [ -z "$USEC" ] || [ "$_took" -lt "$USEC" ]; then USEC=$_took; fi
    done
}
# wallclock: a hit is served from memory the caches of the host hold, a
# DRAM line read or write goes to the DRAM of the host (frame/cpu/dram.c,
# flush_line), so the run with the cache ends sooner. guest/rowcol rows
# makes about 1.6 million line accesses. Without a cache each one reads or
# writes DRAM, with the cache about 10 thousand do. The run without a cache takes about 5 times
# as long, and 3 times when every core of the host is busy with other
# work, which slows the interpreter more than the DRAM. Hence the bound
# of 2.
#
# In an emulator, QEMU for one, clflush does nothing, a DRAM line read or
# write costs no more than a hit, and the two times say nothing about the cache. The run
# without a cache is timed once more with MCPU_NOFLUSH set, which turns
# flush_line off. When the run with the flush takes less than 1.5 times as
# long as the run without it (on hardware 3 to 6 times, in QEMU 1), the
# host is taken to be such an emulator and the item passes with a note.
wallclock() {
    fastest cpu --cache 0 $G/rowcol rows || return 1
    uncached=$USEC
    export MCPU_NOFLUSH=1
    fastest cpu --cache 0 $G/rowcol rows
    _status=$?
    unset MCPU_NOFLUSH
    [ $_status -eq 0 ] || return 1
    if [ $((2 * uncached)) -lt $((3 * USEC)) ]; then
        echo "not measured: on this host the flush of frame/cpu/dram.c does not slow a DRAM line read or write" > "$TMP/note"
        echo "(rowcol rows with --cache 0: $((uncached / 1000)) ms, $((USEC / 1000)) ms without the flush)," >> "$TMP/note"
        echo "as in an emulator; on x86-64 hardware the item is measured" >> "$TMP/note"
        return 0
    fi
    fastest cpu $G/rowcol rows || return 1
    cached=$USEC
    [ "$uncached" -ge $((2 * cached)) ] && return 0
    echo "$G/rowcol rows, the fastest of three runs: $((cached / 1000)) ms with --cache 64,4 and"
    echo "$((uncached / 1000)) ms with --cache 0; expected the run with the cache at least 2 times faster"
    return 1
}

item 20 "unit: cache_init, cache_read, cache_write, cache_flush"  unit cache
item 10 "matmul: counts of the model, --cache 64,4"     replay 64 4 $G/matmul
item  5 "sum: counts of the model, --cache 64,4"        replay 64 4 $G/sum
item 10 "matmul: --cache 64,1 and 16,4"                 small
item  5 "block 8: --cache 16,1"                         block
item  5 "dram: one line per miss and per writeback"     lines
item  5 "programs: same results for four other caches"  same
item 15 "random caches: counts of tools/cachemodel.py"  random_
item 10 "speedup: DRAM line reads and writes with the cache at most 1/10 of --cache 0"  speedup
item  5 "locality: rowcol by columns at least 3 times the DRAM line reads and writes by rows"  locality
item 10 "wall clock: rowcol with the cache at least 2 times faster than --cache 0"  wallclock
finish
