#!/usr/bin/env python3
"""condops.py: a guest program that tries every condition under every flags.

    condops.py                      print the program, x86-64 assembly without libc
    condops.py --explain WANT GOT   describe the first record that differs

There are 16 condition codes and 32 combinations of CF, PF, ZF, SF and OF.
For each of the 512 pairs, numbered k = cc * 32 + combination, the program
loads the flags and runs four instructions with that condition:

    push $FLAGS; popfq
    setCC  %bl                   1 when the condition holds
    cmovCC %ecx,%eax             %eax becomes 2 when it holds, else stays 1
    jCC    rel8                  %edx: 1 when the jump was taken
    jCC    rel32                 %esi: 1 when the jump was taken

and appends a record of four 32-bit numbers (%ebx, %eax, %edx, %esi) to a
buffer, which it writes to stdout at the end. setcc and cmovcc are run by
the framework with cond(); the two jumps are the cases of interp.c.

tests/lab1/run.sh assembles the program, runs it on this machine and under
mini-cpu, and compares the two outputs.
"""
import struct
import sys

CC = ["o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g"]
FLAGS = [(0x1, "CF"), (0x4, "PF"), (0x40, "ZF"), (0x80, "SF"), (0x800, "OF")]
FIELDS = ["set%s %%bl", "cmov%s %%ecx,%%eax", "j%s rel8, 1 = taken", "j%s rel32, 1 = taken"]
REC = 16


def flags_of(k):
    return sum(m for i, (m, _) in enumerate(FLAGS) if k % 32 >> i & 1)


def names(f):
    return " ".join(n for m, n in FLAGS if f & m) or "none"


def program():
    out = ["\t.globl _start", "\t.text", "_start:", "\tlea buf(%rip),%rdi"]
    for k in range(512):
        cc, f = k // 32, flags_of(k)
        out += ["# %d: condition %s, flags: %s" % (k, CC[cc], names(f)),
                "\tpush $%#x" % f, "\tpopfq",
                "\tmov $0,%ebx", "\tmov $1,%eax", "\tmov $2,%ecx",
                "\tset%s %%bl" % CC[cc],
                "\tcmov%s %%ecx,%%eax" % CC[cc],
                "\tmov $1,%edx",
                "\tj%s 1f" % CC[cc],
                "\tmov $0,%edx",
                "1:\tmov $1,%esi",
                "\t.byte 0x0f, %#x" % (0x80 + cc),     # jcc rel32
                "\t.long 2f - . - 4",
                "\tmov $0,%esi",
                "2:\tmov %ebx,(%rdi)", "\tmov %eax,4(%rdi)",
                "\tmov %edx,8(%rdi)", "\tmov %esi,12(%rdi)", "\tlea 16(%rdi),%rdi"]
    out += ["\tlea buf(%rip),%rsi", "\tmov $%d,%%ebx" % (REC * 512),
            "more:", "\tmov $1,%eax", "\tmov $1,%edi", "\tmov %rbx,%rdx", "\tsyscall",
            "\tmov $1,%edi", "\ttest %rax,%rax", "\tjle done",
            "\tadd %rax,%rsi", "\tsub %rax,%rbx", "\tjne more",
            "\txor %edi,%edi",
            "done:", "\tmov $60,%eax", "\tsyscall",
            "\t.bss", "buf:", "\t.zero %d" % (REC * 512)]
    return "\n".join(out)


def explain(want_path, got_path):
    want = open(want_path, "rb").read()
    got = open(got_path, "rb").read()
    for k in range(512):
        w, g = want[k * REC:(k + 1) * REC], got[k * REC:(k + 1) * REC]
        if w == g:
            continue
        if len(g) < REC:
            print("the output of mini-cpu ends before record %d" % k)
            return
        cc, f = k // 32, flags_of(k)
        print("condition %s (cc %#x) with flags: %s" % (CC[cc], cc, names(f)))
        for name, a, b in zip(FIELDS, struct.unpack("<4I", w), struct.unpack("<4I", g)):
            if a != b:
                print("  %-28s expected %d, got %d" % (name % CC[cc], a, b))
        return
    print("the outputs are equal")


def main():
    if len(sys.argv) == 1:
        print(program())
    elif len(sys.argv) == 4 and sys.argv[1] == "--explain":
        explain(sys.argv[2], sys.argv[3])
    else:
        sys.exit("usage: condops.py [--explain WANT GOT]")


if __name__ == "__main__":
    main()
