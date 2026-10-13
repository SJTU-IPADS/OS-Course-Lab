#!/bin/sh
# Lab 1: modrm.c, alu.c and interp.c. From lab 2 on, mini-cpu runs the
# interpreter with --interp.
#
#   sh tests/lab1/run.sh [DIR]
#
# The lab has four parts, and the items of a part pass once that part and
# the parts before it are written:
#
#   part 1   30 points   modrm.c
#   part 2   30 points   alu.c
#   part 3   30 points   interp.c: cond, the loop, the integer instructions
#   part 4   10 points   interp.c: the seven vector instructions
#
# unit: mini-unit (tests/unit.c) calls modrm_decode, the ten functions of
# alu.c (groups alu, shift and muldiv) and cond directly, without a guest
# program, so each can be checked before interp_run exists.
#
# disasm: frame/translate.c reads every operand through modrm_decode. The
# listings of mini-cpu --disasm for the guests have the checksums of
# expected.txt; a file of random memory operands (tests/lab1/memops.py) is
# assembled here and its listing compared with that of objdump -d.
#
# The guests: outputs are compared with tests/reference.py where it
# computes them, and commit logs with the checksums in
# tests/lab1/expected.txt. guest/tinytf runs with random seeds and lengths
# (SEED, see tests/lib.sh).
#
# mini-ollama: guest/mini-ollama and guest/mini-ollama-simd (guest/ollama)
# generate text from the model on their standard input and report on
# standard error how many tokens they handled and a checksum of the scores
# they computed. Here the model is a small one that tools/mkmodel.py writes
# from a random seed, with a random prompt and number of tokens; text and
# report under mini-cpu must be those of the program on this machine. For
# guest/mini-ollama-simd this machine runs guest/mini-ollama-ref, the same
# program with dot_q4 in C.
#
# Generated programs: flagops.py (random instructions that set flags),
# condops.py (every condition under every combination of flags) and
# randops.py (random instructions of interp.c on random operands) are
# assembled here and run on this machine and under mini-cpu; the outputs
# must be the same. The expected output of vecops.py (random vector
# instructions) is computed in Python.
LABNO=1 DIR=${1:-.}
. tests/lib.sh
if has --interp; then MODE=--interp; fi

hello() {
    out "hello from the guest
argc = 1" cpu $MODE guest/hello &&
    out "sum = $(python3 tests/reference.py sum 100)" cpu $MODE guest/sum &&
    out "sum = $(python3 tests/reference.py sum 1000)" cpu $MODE guest/sum x &&
    hasline "insns 100  syscalls 0  exit 1" cpu $MODE --max 100 --stats guest/sum   # stops after exactly 100
}
programs() {
    out "sum = $(python3 tests/reference.py matmul 24)" cpu $MODE guest/matmul &&
    out "sum = 1566720" cpu $MODE guest/heap &&
    out "s = 39500" cpu $MODE guest/pattern &&
    out "sum = 501590016" cpu $MODE guest/block 8
}
# two page faults: past the end of the stack, and a store to the code
faults() {
    hasline "fault (page fault) after 2853 instructions:" cpu $MODE guest/deep &&
    hasline "faulting address 0x7ffffffbec90" cpu $MODE guest/deep &&
    hasline "fault (page fault) after 2 instructions:" cpu $MODE guest/readonly &&
    hasline "faulting address 0x401000" cpu $MODE guest/readonly
}
logs1() { log hello guest/hello && log sum guest/sum; }
logs2() {
    log matmul guest/matmul && log heap guest/heap &&
    log pattern guest/pattern && log block8 guest/block 8
}
simd() {
    out "scalar = $(python3 tests/reference.py simd 19)
simd   = $(python3 tests/reference.py simd 19)" cpu $MODE guest/simd &&
    out "scalar = $(python3 tests/reference.py simd 1000)
simd   = $(python3 tests/reference.py simd 1000)" cpu $MODE guest/simd x
}
# tinytf [SEED N]: the output equals that of tests/reference.py tinytf
tinytf() {
    out "$(python3 tests/reference.py tinytf "$@")" cpu $MODE guest/tinytf "$@" && return 0
    echo "(guest/tinytf ${*:-1 8})"
    return 1
}
tinytfs() {
    tinytf || return 1
    for k in 1 2; do
        rand 1000000; s=$RND; rand 12
        tinytf $s $((RND + 1)) || return 1
    done
}

# ollama PROGRAM REFERENCE: both get the model of tools/mkmodel.py MS on
# standard input, the prompt MP and the number of tokens MN; REFERENCE runs
# on this machine, PROGRAM under mini-cpu. They print the same text and, on
# standard error, the same report.
ollama() {
    rand 1000000; MS=$RND
    rand 4
    case $RND in
    0) MP="hello" ;;
    1) MP="the cat in the hat" ;;
    2) MP="What is 2+2?" ;;
    *) MP="an hour in the rain" ;;
    esac
    rand 8; MN=$((RND + 5))
    python3 tools/mkmodel.py $MS > "$TMP/model.gguf" &&
        "$2" "$MP" $MN < "$TMP/model.gguf" > "$TMP/want" 2> "$TMP/want2" ||
        { echo "$2: does not run on this machine"; head -n 3 "$TMP/want2"; return 1; }
    cpu $MODE "$1" "$MP" $MN < "$TMP/model.gguf" > "$TMP/got" 2> "$TMP/got2"
    cmp -s "$TMP/want" "$TMP/got" && cmp -s "$TMP/want2" "$TMP/got2" && return 0
    echo "(python3 tools/mkmodel.py $MS > m.gguf; mini-cpu $MODE $1 \"$MP\" $MN < m.gguf)"
    printf 'expected: %s\ngot:      %s\n' "$(cat "$TMP/want")" "$(cat "$TMP/got")"
    diff "$TMP/want2" "$TMP/got2" | grep '^[<>]' | head -n 4       # the report on stderr
    return 1
}
ollamas() { for k in 1 2; do ollama "$@" || return 1; done; }

# listing FILE: address and instruction of every line of objdump -d FILE,
# without symbol names, and the same for mini-cpu --disasm FILE
listing() {
    awk -F '\t' 'NF >= 3 { a = $1; gsub(/[ :]/, "", a); t = $3
                           sub(/ *<[^>]*>/, "", t); sub(/# 0x/, "# ", t); sub(/ +$/, "", t)
                           print a "\t" t }'
}
disasm_guests() {
    for g in hello sum matmul heap pattern deep block simd tinytf; do
        "$CPU" --disasm guest/$g > "$TMP/dis" 2>&1
        is "$(val "disasm-$g")" "$(cksum < "$TMP/dis" | awk '{print $1, $2}')" && continue
        echo "(mini-cpu --disasm guest/$g: compare with objdump -d guest/$g)"
        head -n 2 "$TMP/dis" | grep 'not implemented'
        return 1
    done
}
disasm_random() {
    rand 1000000
    python3 tests/lab1/memops.py "$RND" 600 > "$TMP/memops.s" &&
        ${CC:-gcc} -c -o "$TMP/memops.o" "$TMP/memops.s" || return 1
    objdump -d "$TMP/memops.o" | listing > "$TMP/want"
    "$CPU" --disasm "$TMP/memops.o" 2> "$TMP/err" | listing > "$TMP/got"
    cmp -s "$TMP/want" "$TMP/got" && return 0
    echo "(tests/lab1/memops.py $RND 600: objdump -d, then mini-cpu --disasm)"
    head -n 2 "$TMP/err"
    diff "$TMP/want" "$TMP/got" | grep '^[<>]' | head -n 4
    return 1
}

# flags: the guest of flagops.py writes 10 bytes per instruction k: the
# destination register and the flags. The first differing record names k.
flags() {
    rand 1000000
    python3 tests/lab1/flagops.py "$RND" 1000 > "$TMP/flagops.s" && build flagops &&
        "$TMP/flagops" > "$TMP/want" || { echo "flagops: does not run on this machine"; return 1; }
    cpu $MODE "$TMP/flagops" > "$TMP/got" 2> "$TMP/err"
    cmp -s "$TMP/want" "$TMP/got" && return 0
    _b=$(cmp "$TMP/want" "$TMP/got" 2>&1 | sed -n 's/.* byte \([0-9]*\).*/\1/p')
    [ -n "$_b" ] || { echo "(the output of mini-cpu is shorter)"; head -n 3 "$TMP/err"; return 1; }
    _k=$(( (_b - 1) / 10 ))
    echo "(tests/lab1/flagops.py $RND 1000, instruction $_k)"
    grep "^# $_k: " "$TMP/flagops.s" | cut -c3-
    for _w in want got; do
        od -An -tx8 -N8 -j $((_k * 10)) "$TMP/$_w" | tr -d ' ' > "$TMP/v"
        od -An -tx2 -N2 -j $((_k * 10 + 8)) "$TMP/$_w" | tr -d ' ' > "$TMP/f"
        printf '%-5s value 0x%s, flags: %s\n' "$_w:" "$(cat "$TMP/v")" "$(flagnames "0x$(cat "$TMP/f")")"
    done
    return 1
}
# flagnames HEX: the names of the flags set in HEX
flagnames() {
    _s=
    [ $(( $1 & 0x1 )) -ne 0 ] && _s="$_s CF"
    [ $(( $1 & 0x4 )) -ne 0 ] && _s="$_s PF"
    [ $(( $1 & 0x40 )) -ne 0 ] && _s="$_s ZF"
    [ $(( $1 & 0x80 )) -ne 0 ] && _s="$_s SF"
    [ $(( $1 & 0x800 )) -ne 0 ] && _s="$_s OF"
    echo "${_s:- -}" | cut -c2-
}
simdlogs() { log simd guest/simd && log simdx guest/simd x && log tinytf guest/tinytf; }
conds()   { condops_make && condops_run; }
# two programs of random integer instructions, two of vector instructions
randoms() { for k in 1 2; do randops_make && randops_run || return 1; done; }
vectors() { for k in 1 2; do vecops_make && vecops_run || return 1; done; }

# part N TEXT: the heading of a part
part() { echo "-- part $1: $2"; }

part 1 "modrm.c, 30 points"
item 10 "unit: modrm_decode, every form and every truncated length"  unit modrm
item 10 "disasm: the guests"                           disasm_guests
item 10 "disasm: random memory operands (SIB, displacement)"  disasm_random

part 2 "alu.c, 30 points"
item 10 "unit: alu.c, x86_szp to x86_neg"              unit alu
item 10 "unit: alu.c, x86_shift"                       unit shift
item 10 "unit: alu.c, x86_mul and x86_div"             unit muldiv

part 3 "interp.c, the integer instructions, 30 points"
item  3 "unit: cond, 16 conditions under 32 combinations of flags"  unit cond
item  3 "hello, sum: output"                           hello
item  3 "matmul, heap, pattern, block: output"         programs
item  3 "page faults: past the stack, store to the code"  faults
item  3 "commit logs: hello, sum"                      logs1
item  3 "commit logs: matmul, heap, pattern, block"    logs2
item  3 "flags: random operands, against this machine" flags
item  3 "conditions: jcc, setcc, cmovcc under all flags"  conds
item  3 "random instructions: registers and memory, against this machine"  randoms
item  3 "mini-ollama: two small models, against this machine"  ollamas guest/mini-ollama guest/mini-ollama

part 4 "interp.c, the vector instructions, 10 points"
item  2 "simd: scalar and vector loops, n = 19 and 1000"  simd
item  2 "tinytf: seed 1 and two random seeds"          tinytfs
item  2 "simd, tinytf: commit logs (vector registers)" simdlogs
item  2 "random vector instructions: registers and memory"  vectors
item  2 "mini-ollama-simd: dot_q4 on ymm registers"    ollamas guest/mini-ollama-simd guest/mini-ollama-ref
finish
