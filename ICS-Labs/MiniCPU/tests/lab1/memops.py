#!/usr/bin/env python3
"""Random instructions with memory operands, for the test of translate().

    python3 tests/lab1/memops.py SEED [N]

Prints an assembly file of N instructions (default 400). Every memory
operand has a random base, index, scale and displacement, including the
encodings that need care: %rsp and %r12 as a base (always a SIB byte),
%rbp and %r13 as a base (always a displacement), %r12 as an index, no base
(a SIB byte with a 4-byte displacement), no base and no index, and %rip.
The test assembles the file with gcc -c and compares the output of
mini-cpu --disasm with that of objdump -d.
"""
import random
import sys

Q = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]


def reg(n, size):
    if size == 8:
        return "%" + Q[n]
    if n >= 8:
        return "%" + Q[n] + {4: "d", 2: "w", 1: "b"}[size]
    if size == 4:
        return "%e" + Q[n][1:]
    return "%" + Q[n][1:]


def disp(r):
    k = r.randrange(4)
    if k == 0:
        return ""
    if k == 1:
        return str(r.randrange(-128, 128))
    return str(r.randrange(-2**31, 2**31))


def mem(r):
    k = r.randrange(10)
    if k == 0:
        return (disp(r) or "8") + "(%rip)"
    if k == 1:
        return str(r.randrange(0, 2**31))
    base = r.choice([None] + list(range(16)) + [4, 12, 5, 13])
    index = r.choice([None] + [i for i in range(16) if i != 4] + [12])
    if base is None and index is None:
        base = r.choice([4, 12])
    s = disp(r) + "("
    if base is not None:
        s += "%" + Q[base]
    if index is not None:
        s += ",%" + Q[index] + "," + str(r.choice([1, 2, 4, 8]))
    return s + ")"


def insn(r):
    m = mem(r)
    n = r.randrange(16)
    size = r.choice([4, 8])
    k = r.randrange(9)
    if k == 0:
        return f"mov {m},{reg(n, size)}"
    if k == 1:
        return f"mov {reg(n, size)},{m}"
    if k == 2:
        return f"lea {m},{reg(n, 8)}"
    if k == 3:
        op = r.choice(["add", "sub", "and", "or", "xor", "cmp", "adc", "sbb"])
        return f"{op}{'lq'[size == 8]} ${r.randrange(-128, 128)},{m}"
    if k == 4:
        return f"movb ${r.randrange(256)},{m}"
    if k == 5:
        return f"imul {m},{reg(n, size)}"
    if k == 6:
        return f"movslq {m},{reg(n, 8)}"
    if k == 7:
        return f"vmovdqu {m},%ymm{n}"
    return f"vpaddd {m},%ymm{r.randrange(16)},%ymm{n}"


def main():
    seed = int(sys.argv[1])
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 400
    r = random.Random(seed)
    print(".text")
    for _ in range(count):
        print("\t" + insn(r))


main()
