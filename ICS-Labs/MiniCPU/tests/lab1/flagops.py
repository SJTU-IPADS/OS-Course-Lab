#!/usr/bin/env python3
"""flagops.py SEED [N]: a guest program of N random instructions that set flags.

Prints x86-64 assembly for a program without libc. Every instruction k runs
on random operands and random incoming flags:

    # k: ...                 the instruction, its operands and flags in
    push $FLAGS; popfq       incoming CF, PF, ZF, SF, OF
    movabs $A,%rbx           the destination (%rbx, or %rax)
    movabs $B,%rcx           the source (%rcx), when there is one
    INSTRUCTION
    pushfq; pop %rdx         the flags after it

and appends a record of 10 bytes to a buffer: the destination register
(8 bytes) and rflags & MASK (2 bytes), little endian. MASK is 0x8c5 (OF, SF,
ZF, PF, CF), without OF for a shift by more than one bit, where x86 leaves
OF undefined. At the end the program writes the buffer to stdout; the exit status is 0
when the whole buffer is written.

The instructions: add, or, adc, sbb, and, sub, xor, cmp and test with a
register or an immediate, inc, dec, neg, and shl, shr, sar by a constant;
all four operand sizes. Operands have random upper bits, so a flag computed
from all 64 bits instead of the operand size is wrong.

tests/lab1/run.sh assembles the program, runs it on this machine and under
mini-cpu, and compares the two outputs.
"""
import random
import sys

SIZES = {1: ("b", "%bl", "%cl", "%al"), 2: ("w", "%bx", "%cx", "%ax"),
         4: ("l", "%ebx", "%ecx", "%eax"), 8: ("q", "%rbx", "%rcx", "%rax")}
# adc and sbb twice: their carry in makes the edge cases of CF
ALU = ["add", "or", "adc", "sbb", "and", "sub", "xor", "cmp", "adc", "sbb"]
FLAGS = [(0x1, "CF"), (0x4, "PF"), (0x40, "ZF"), (0x80, "SF"), (0x800, "OF")]


def names(f):
    return " ".join(n for m, n in FLAGS if f & m) or "-"


def operand(rng, size):
    """A 64-bit value whose low size bytes are often an edge case."""
    bits = size * 8
    top = 1 << (bits - 1)
    low = rng.choice([0, 0, 1, 2, top - 1, top, top + 1, 2 * top - 1, 2 * top - 1,
                      2 * top - 2, 0x7f, 0x80, 0xff, rng.randrange(2 * top),
                      rng.randrange(2 * top), rng.randrange(2 * top)])
    high = 0 if size == 8 else rng.choice([0, rng.randrange(1 << 64)]) >> bits << bits
    return (high | low) & ((1 << 64) - 1)


def immediate(rng, size):
    """An immediate gas encodes for this size: imm8, or imm16/imm32."""
    if rng.random() < 0.5 or size == 1:
        return rng.choice([0, 1, 0x7f, -1, -0x80, rng.randrange(-0x80, 0x80)])
    lim = 1 << (16 if size == 2 else 32) - 1
    return rng.choice([lim - 1, -lim, rng.randrange(-lim, lim)])


def instruction(rng):
    """(text, size, mask, dest) of one random instruction."""
    size = rng.choice([1, 2, 4, 8])
    suf, dst, src, acc = SIZES[size]
    mask = 0x8c5
    kind = rng.choice(["alu", "alu", "alu", "imm", "imm", "test", "unary", "shift"])
    if kind == "imm" and rng.random() < 0.3:
        dst = acc
    if kind == "alu":
        text = "%s%s %s,%s" % (rng.choice(ALU), suf, src, dst)
    elif kind == "imm":
        text = "%s%s $%d,%s" % (rng.choice(ALU), suf, immediate(rng, size), dst)
    elif kind == "test":
        text = rng.choice(["test%s %s,%s" % (suf, src, dst),
                           "test%s $%d,%s" % (suf, immediate(rng, size), dst)])
    elif kind == "unary":
        text = "%s%s %s" % (rng.choice(["inc", "dec", "neg"]), suf, dst)
    else:
        n = rng.choice([1, 1, rng.randrange(0, size * 8)])
        if n != 1:
            mask &= ~0x800
        text = "%s%s $%d,%s" % (rng.choice(["shl", "shr", "sar"]), suf, n, dst)
    return text, size, mask, "%rax" if dst == acc else "%rbx"


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit("usage: flagops.py SEED [N]")
    rng = random.Random(int(sys.argv[1]))
    n = int(sys.argv[2]) if len(sys.argv) == 3 else 400
    out = ["\t.globl _start", "\t.text", "_start:", "\tlea buf(%rip),%rdi"]
    for k in range(n):
        text, size, mask, dest = instruction(rng)
        fin = sum(m for m, _ in FLAGS if rng.random() < 0.5)
        if text.startswith(("adc", "sbb")) and rng.random() < 0.5:
            fin |= 0x1                                  # carry in, more often
        a, b = operand(rng, size), operand(rng, size)
        out.append("# %d: %s    %s = %#x, %%rcx = %#x, flags in: %s"
                   % (k, text, dest, a, b, names(fin)))
        out += ["\tpush $%#x" % fin, "\tpopfq",
                "\tmovabs $%#x,%s" % (a, dest), "\tmovabs $%#x,%%rcx" % b,
                "\t" + text,
                "\tpushfq", "\tpop %rdx", "\tand $%#x,%%edx" % mask,
                "\tmov %s,(%%rdi)" % dest, "\tmov %dx,8(%rdi)", "\tlea 10(%rdi),%rdi"]
    out += ["\tlea buf(%rip),%rsi", "\tmov $%d,%%ebx" % (10 * n),
            "more:", "\tmov $1,%eax", "\tmov $1,%edi", "\tmov %rbx,%rdx", "\tsyscall",
            "\tmov $1,%edi", "\ttest %rax,%rax", "\tjle done",
            "\tadd %rax,%rsi", "\tsub %rax,%rbx", "\tjne more",
            "\txor %edi,%edi",
            "done:", "\tmov $60,%eax", "\tsyscall",
            "\t.bss", "buf:", "\t.zero %d" % (10 * n)]
    print("\n".join(out))


if __name__ == "__main__":
    main()
