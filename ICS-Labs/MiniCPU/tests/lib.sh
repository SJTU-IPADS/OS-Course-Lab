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
G=tests/binary
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
got=0
max=0

# A run that does not finish in this many seconds fails.
LIMIT=${LIMIT:-120}
TIMEOUT=
if command -v timeout >/dev/null 2>&1; then TIMEOUT="timeout $LIMIT"; fi

# No file a test writes may grow beyond FSIZE blocks (256 MiB with the
# 512-byte blocks of dash, four times the physical memory): a program that
# prints without end is stopped by the system before it fills the disk.
FSIZE=${FSIZE:-524288}

# The emulators, with physical memory in a scratch file. CPU_OPTS holds
# options a run.sh adds to every run of mini-cpu (tests/lab1/run.sh: no
# cache from lab 2 on).
CPU_OPTS=
cpu() { ( ulimit -f $FSIZE 2>/dev/null; exec $TIMEOUT "$CPU" --ram "$TMP/dram.img" $CPU_OPTS "$@" ); }
jit() { ( ulimit -f $FSIZE 2>/dev/null; exec $TIMEOUT "$JIT" --ram "$TMP/dram.img" "$@" ); }

# rand N: RND becomes a number in 0 .. N-1, the next one drawn from SEED.
SEED=${SEED:-$(od -An -N2 -tu2 /dev/urandom | tr -d ' ')}
_rand_state=$(( SEED * 8 + LABNO ))
rand() {
    _rand_state=$(( (_rand_state * 1103515245 + 12345) % 2147483648 ))
    RND=$(( _rand_state / 65536 % $1 ))
}

# val [FILE] KEY: the value of KEY in an expected.txt ("KEY value" lines).
val() {
    if [ $# -gt 1 ]; then _file=$1; shift; else _file=$EXP; fi
    sed -n "s/^$1 //p" "$_file"
}

# item POINTS NAME COMMAND...: PASS when COMMAND succeeds. Lines COMMAND
# writes to the file $TMP/note are printed below the PASS line.
item() {
    _points=$1; _name=$2; shift 2
    max=$((max + _points))
    rm -f "$TMP/note"
    if "$@" > "$TMP/why" 2>&1; then
        got=$((got + _points))
        printf 'PASS %3d/%-3d %s\n' "$_points" "$_points" "$_name"
        [ -f "$TMP/note" ] && sed 's/^/        | /' "$TMP/note"
    else
        printf 'FAIL %3d/%-3d %s\n' 0 "$_points" "$_name"
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
    _want=$1; shift
    "$@" > "$TMP/out" 2> "$TMP/err"
    is "$_want" "$(cat "$TMP/out")" && return 0
    head -n 3 "$TMP/err"
    return 1
}

# hasline LINE COMMAND...: stdout and stderr of COMMAND contain the line LINE.
hasline() {
    _line=$1; shift
    "$@" > "$TMP/all" 2>&1
    grep -qxF -- "$_line" "$TMP/all" && return 0
    printf 'expected the line: %s\n' "$_line"
    grep -E '^(insns|tlb|cache|dram|disk|fault|faulting)|not implemented yet' "$TMP/all" | head -n 6
    return 1
}

# log KEY PROGRAM ARGS...: the commit log of mini-cpu PROGRAM has the
# checksum KEY of tests/lab1/expected.txt.
log() {
    _key=$1; shift
    rm -f "$TMP/log"
    cpu --commit "$TMP/log" "$@" > /dev/null 2> "$TMP/err"
    [ -s "$TMP/log" ] || { echo "mini-cpu $*: no commit log"; head -n 3 "$TMP/err"; return 1; }
    is "$(val tests/lab1/expected.txt "$_key")" "$(cksum < "$TMP/log" | awk '{print $1, $2}')" ||
        { echo "(cksum of the commit log of $*: checksum and size in bytes)"; return 1; }
}

# unit GROUP...: mini-unit GROUP (tests/unit.c) reports no failed check,
# for every GROUP. It calls the functions of a lab one at a time.
unit() {
    for _group in "$@"; do
        ( ulimit -f $FSIZE 2>/dev/null
          exec $TIMEOUT "$UNIT" --ram "$TMP/unit.img" --disk "$TMP/unit-disk.img" "$_group" < /dev/null )
        _status=$?
        [ $_status -eq 0 ] && continue
        [ $_status -eq 1 ] || echo "mini-unit $_group ended with status $_status"
        return 1
    done
}

finish() {
    printf 'lab %s score %d/%d\n' "$LABNO" "$got" "$max"
    [ -n "${SCORES:-}" ] && echo "$LABNO $got $max" >> "$SCORES"
    exit 0
}
