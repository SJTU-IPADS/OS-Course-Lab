# Shared by tests/labN/run.sh, which source it; run from the top directory.
#
#   sh tests/labN/run.sh [DIR]     DIR holds mini-cpu, mini-unit and mini-jit
#                                  (default .)
#
# A run.sh sets LABNO and DIR before it sources this file.
#
# Every item prints one line, PASS or FAIL with its points, and on FAIL the
# first lines of what went wrong. The last line is
#
#   lab N score GOT/MAX
#
# and when the environment variable SCORES names a file, "N GOT MAX" is
# appended to it (tests/grade.sh adds these up).
#
# Random arguments come from the environment variable SEED (tests/grade.sh
# sets and prints it); with the same SEED a lab draws the same arguments.

set -u
CPU=$DIR/mini-cpu
JIT=$DIR/mini-jit
UNIT=$DIR/mini-unit
EXP=tests/lab$LABNO/expected.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
got=0
max=0

# A run that does not finish in this many seconds fails.
LIMIT=${LIMIT:-120}
if command -v timeout >/dev/null 2>&1; then T="timeout $LIMIT"; else T=; fi

# No file a test writes may grow beyond FSIZE blocks (256 MiB with the
# 512-byte blocks of dash, four times the physical memory): a program that
# prints without end is stopped by the system before it fills the disk.
FSIZE=${FSIZE:-524288}

# The emulators, with physical memory in a scratch file.
cpu() { ( ulimit -f $FSIZE 2>/dev/null; exec $T "$CPU" --ram "$TMP/dram.img" "$@" ); }
jit() { ( ulimit -f $FSIZE 2>/dev/null; exec $T "$JIT" --ram "$TMP/dram.img" "$@" ); }

# rand N: RND becomes a number in 0 .. N-1, the next one drawn from SEED.
SEED=${SEED:-$(od -An -N2 -tu2 /dev/urandom | tr -d ' ')}
_r=$(( SEED * 8 + LABNO ))
rand() {
    _r=$(( (_r * 1103515245 + 12345) % 2147483648 ))
    RND=$(( _r / 65536 % $1 ))
}

# Options that exist only in later builds.
has() { "$CPU" -h 2>&1 | grep -q -- "$1"; }

# val [FILE] KEY: the value of KEY in an expected.txt ("KEY value" lines).
val() {
    if [ $# -gt 1 ]; then _f=$1; shift; else _f=$EXP; fi
    sed -n "s/^$1 //p" "$_f"
}

# item POINTS NAME COMMAND...: PASS when COMMAND succeeds.
item() {
    _p=$1; _n=$2; shift 2
    max=$((max + _p))
    if "$@" > "$TMP/why" 2>&1; then
        got=$((got + _p))
        printf 'PASS %3d/%-3d %s\n' "$_p" "$_p" "$_n"
    else
        printf 'FAIL %3d/%-3d %s\n' 0 "$_p" "$_n"
        head -n 8 "$TMP/why" | sed 's/^/        | /'
    fi
}

# is WANT GOT: equal strings, otherwise print both.
is() {
    [ "$1" = "$2" ] && return 0
    printf 'expected: %s\ngot:      %s\n' "$1" "$2"
    return 1
}

# out WANT COMMAND...: the stdout of COMMAND is WANT (stderr is shown on failure).
out() {
    _w=$1; shift
    "$@" > "$TMP/out" 2> "$TMP/err"
    is "$_w" "$(cat "$TMP/out")" && return 0
    head -n 3 "$TMP/err"
    return 1
}

# hasline LINE COMMAND...: stdout and stderr of COMMAND contain the line LINE.
hasline() {
    _l=$1; shift
    "$@" > "$TMP/all" 2>&1
    grep -qxF -- "$_l" "$TMP/all" && return 0
    printf 'expected the line: %s\n' "$_l"
    grep -E '^(insns|pipe|tlb|cache|dram|disk|fault|faulting)|not implemented yet' "$TMP/all" | head -n 6
    return 1
}

# same PROGRAM ARGS...: the commit log of mini-cpu MODE PROGRAM equals that
# of mini-cpu --interp PROGRAM (from lab 2 on).
same() {
    rm -f "$TMP/log" "$TMP/ilog"
    cpu --interp --commit "$TMP/ilog" "$@" > /dev/null 2>&1
    cpu $MODE --commit "$TMP/log" "$@" > /dev/null 2> "$TMP/err"
    cmp -s "$TMP/ilog" "$TMP/log" && return 0
    echo "mini-cpu $MODE $*: the commit log differs from that of --interp"
    diff "$TMP/ilog" "$TMP/log" | head -n 4
    head -n 3 "$TMP/err"
    return 1
}

# log KEY PROGRAM ARGS...: the commit log of mini-cpu [MODE] PROGRAM has the
# checksum KEY of tests/lab1/expected.txt. MODE is empty, --interp or --pipe.
MODE=
log() {
    _k=$1; shift
    rm -f "$TMP/log"
    cpu $MODE --commit "$TMP/log" "$@" > /dev/null 2> "$TMP/err"
    [ -s "$TMP/log" ] || { echo "mini-cpu $MODE $*: no commit log"; head -n 3 "$TMP/err"; return 1; }
    is "$(val tests/lab1/expected.txt "$_k")" "$(cksum < "$TMP/log" | awk '{print $1, $2}')" ||
        { echo "(cksum of the commit log of $*: checksum and size in bytes)"
          [ "$MODE" != --interp ] && has --interp &&
              echo "(compare with: mini-cpu --interp --commit F $*)"
          return 1; }
}

# unit GROUP...: mini-unit GROUP (tests/unit.c) reports no failed check,
# for every GROUP. It calls the functions of a lab one at a time.
unit() {
    for _g in "$@"; do
        ( ulimit -f $FSIZE 2>/dev/null
          exec $T "$UNIT" --ram "$TMP/unit.img" --disk "$TMP/unit-disk.img" "$_g" < /dev/null )
        _s=$?
        [ $_s -eq 0 ] && continue
        [ $_s -eq 1 ] || echo "mini-unit $_g ended with status $_s"
        return 1
    done
}

# build NAME: assemble $TMP/NAME.s into the program $TMP/NAME, without libc.
build() { ${CC:-gcc} -nostdlib -no-pie -o "$TMP/$1" "$TMP/$1.s"; }

# Three generated programs, shared by the tests of labs 1 and 2. X_make
# writes the program $TMP/X and its expected output $TMP/X.want; X_run
# compares the output of mini-cpu $MODE with it and describes the first
# difference.
#
#   condops   every condition code under every combination of flags
#   randops   blocks of random integer instructions (seed RS)
#   vecops    blocks of random vector instructions (seed VS)
BLOCKS=60
condops_make() {
    python3 tests/lab1/condops.py > "$TMP/condops.s" && build condops &&
        "$TMP/condops" > "$TMP/condops.want" ||
        { echo "condops: does not run on this machine"; return 1; }
}
condops_run() {
    cpu $MODE "$TMP/condops" > "$TMP/got" 2> "$TMP/err"
    cmp -s "$TMP/condops.want" "$TMP/got" && return 0
    echo "(tests/lab1/condops.py)"
    head -n 3 "$TMP/err"
    python3 tests/lab1/condops.py --explain "$TMP/condops.want" "$TMP/got"
    return 1
}
randops_make() {
    rand 1000000; RS=$RND
    python3 tests/lab1/randops.py $RS $BLOCKS > "$TMP/randops.s" && build randops &&
        "$TMP/randops" > "$TMP/randops.want" ||
        { echo "randops $RS: does not run on this machine"; return 1; }
}
randops_run() {
    cpu $MODE "$TMP/randops" > "$TMP/got" 2> "$TMP/err"
    cmp -s "$TMP/randops.want" "$TMP/got" && return 0
    echo "(tests/lab1/randops.py $RS $BLOCKS)"
    head -n 3 "$TMP/err"
    python3 tests/lab1/randops.py --explain $RS $BLOCKS "$TMP/randops.want" "$TMP/got"
    return 1
}
vecops_make() {
    rand 1000000; VS=$RND
    python3 tests/lab1/vecops.py $VS $BLOCKS > "$TMP/vecops.s" && build vecops &&
        python3 tests/lab1/vecops.py --want $VS $BLOCKS > "$TMP/vecops.want"
}
vecops_run() {
    cpu $MODE "$TMP/vecops" > "$TMP/got" 2> "$TMP/err"
    cmp -s "$TMP/vecops.want" "$TMP/got" && return 0
    echo "(tests/lab1/vecops.py $VS $BLOCKS)"
    head -n 3 "$TMP/err"
    python3 tests/lab1/vecops.py --explain $VS $BLOCKS "$TMP/got"
    return 1
}

finish() {
    printf 'lab %s score %d/%d\n' "$LABNO" "$got" "$max"
    [ -n "${SCORES:-}" ] && echo "$LABNO $got $max" >> "$SCORES"
    exit 0
}
