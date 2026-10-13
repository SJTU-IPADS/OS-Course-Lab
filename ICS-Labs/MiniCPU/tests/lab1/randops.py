#!/usr/bin/env python3
"""randops.py SEED [N]: a guest program of N blocks of random instructions.

    randops.py SEED [N]                      print the program
    randops.py --explain SEED N WANT GOT     describe the first difference

Prints x86-64 assembly for a program without libc. The instructions are
those of the switch of interp.c: mov, lea, the eight ALU operations, test,
push, pop, call, ret, jmp and jcc (both with rel8 and rel32, forward and
backward), in 16, 32 and 64 bits, on registers, immediates and memory
operands of every form:

    D(%rbp)   (%r12)   D(%r13)   D(%rbp,%rsi,4)   data+D(,%rcx,8)
    data+D(%rip)   data+D   (%rax)

lea also computes D(%rax,%rbx,4) on any two registers, whatever they hold.

%rbp, %r12 and %r13 point into a data area of 2048 random bytes and are
never written; %rdi points to the output buffer; %rsp is the stack. The
other eleven registers hold random values. Block 0 is fixed: push %rsp and
pop %rsp.

After every block the program appends a record of 96 bytes to the buffer:
the eleven registers (rax rbx rcx rdx rsi r8 r9 r10 r11 r14 r15) and
rflags & 0x8c5 (OF, SF, ZF, PF, CF). At the end it writes the buffer and
the data area to stdout; the exit status is 0 when everything is written.

No instruction has a result or flag that x86 leaves undefined, and no
value depends on the address of the stack, so the output on this machine
and under mini-cpu must be the same. tests/lab1/run.sh assembles the
program, runs it both ways, and compares the outputs; tests/lab2/run.sh
also runs it on the CPU of lab 2.
"""
import random
import struct
import sys

POOL = ["rax", "rbx", "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11", "r14"]
LOOP = "r15"                        # the counter of the loops, written nowhere else
DUMP = POOL + [LOOP]
REC = 8 * (len(DUMP) + 1)
DATA = 2048
BASE = {"rbp": 1024, "r12": 512, "r13": 1536}       # register: its offset in the data area
SUFFIX = {1: "b", 2: "w", 4: "l", 8: "q"}
ALU = ["add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"]
CC = ["o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g"]
FLAGS = [(0x1, "CF"), (0x4, "PF"), (0x40, "ZF"), (0x80, "SF"), (0x800, "OF")]
NFUNC = 6
# Block 0. push %rsp stores the value %rsp had before the push; pop %rsp
# ends with the loaded value in %rsp. %r14 holds the stack pointer meanwhile
# and is cleared, since no record may depend on the address of the stack.
STACK = ["push %rsp", "pop %rax", "sub %rsp,%rax",
         "mov %rsp,%r14", "lea data+1024(%rip),%rbx", "push %rbx", "pop %rsp",
         "mov %rsp,%rcx", "mov %r14,%rsp", "movq $0,%r14"]


def reg(name, size):
    """The size-byte register inside the 64-bit register name."""
    if name[1].isdigit():
        return "%" + name + {1: "b", 2: "w", 4: "d", 8: ""}[size]
    if size == 8:
        return "%" + name
    if size == 4:
        return "%e" + name[1:]
    if size == 2:
        return "%" + name[1:]
    return "%" + (name[1] + "l" if name[2] == "x" else name[1:] + "l")


class Gen:
    def __init__(self, seed):
        self.rng = random.Random(seed)
        self.label = 0

    def new_label(self):
        self.label += 1
        return ".L%d" % self.label

    def pick(self, exclude=()):
        return self.rng.choice([r for r in POOL if r not in exclude])

    def imm(self, size):
        """An immediate of an ALU operation or a mov to memory."""
        rng = self.rng
        if rng.random() < 0.5:
            return rng.choice([0, 1, -1, 0x7f, -0x80, rng.randrange(-0x80, 0x80)])
        lim = 1 << (15 if size == 2 else 31)
        return rng.choice([lim - 1, -lim, rng.randrange(-lim, lim)])

    def mem(self, size, exclude=()):
        """(lines to run first, operand text) of a memory operand of size bytes."""
        rng = self.rng
        form = rng.choice(["base", "base", "sib", "index", "rip", "abs", "ptr"])
        if form == "base":
            b = rng.choice(list(BASE))
            if rng.random() < 0.3:
                d = 0                                   # (%r12), 0(%rbp), 0(%r13)
            elif rng.random() < 0.5:
                d = rng.randrange(-128, 128)            # disp8
            else:
                d = rng.randrange(-BASE[b], DATA - BASE[b] - size + 1)
            return [], "%s(%%%s)" % (d if d else "", b)
        if form == "ptr":                               # any register as the base
            r = self.pick(exclude)
            off = rng.randrange(0, DATA - size + 1)
            return ["lea data+%d(%%rip),%%%s" % (off, r)], "(%%%s)" % r
        if form in ("sib", "index"):
            x = self.pick(exclude)
            scale = rng.choice([1, 2, 4, 8])
            i = rng.randrange(0, 16)
            pre = ["mov $%d,%s" % (i, reg(x, 4))]
            if form == "sib":
                b = rng.choice(list(BASE))
                lo, hi = -BASE[b], DATA - BASE[b] - size - i * scale
                d = rng.randrange(-128, 128) if rng.random() < 0.5 and lo <= -128 and hi >= 127 \
                    else rng.randrange(lo, hi + 1)
                return pre, "%s(%%%s,%%%s,%d)" % (d if d else "", b, x, scale)
            off = rng.randrange(0, DATA - size - i * scale + 1)
            return pre, "data+%d(,%%%s,%d)" % (off, x, scale)
        off = rng.randrange(0, DATA - size + 1)
        return [], "data+%d(%%rip)" % off if form == "rip" else "data+%d" % off

    def simple(self, exclude=()):
        """One instruction without a jump, as a list of lines; the registers
        in exclude are not written."""
        rng = self.rng
        size = rng.choice([2, 4, 4, 8, 8])
        s = SUFFIX[size]
        kind = rng.choice(["mov", "mov", "movimm", "lea", "alu", "alu", "alu", "alu", "test",
                           "byte"])
        dst = self.pick(exclude)
        src = rng.choice(POOL + list(BASE))
        if kind == "mov":
            form = rng.choice(["rr", "load", "store"])
            if form == "rr":
                return ["mov%s %s,%s" % (s, reg(src, size), reg(dst, size))]
            pre, m = self.mem(size, exclude)
            if form == "load":
                return pre + ["mov%s %s,%s" % (s, m, reg(dst, size))]
            return pre + ["mov%s %s,%s" % (s, reg(src, size), m)]
        if kind == "movimm":
            form = rng.choice(["b8", "b8", "c7", "mem"])
            if form == "b8" and size == 8:              # b8+r with REX.W: imm64
                return ["movabs $%#x,%s" % (rng.randrange(1 << 64), reg(dst, 8))]
            if form == "b8":                            # imm16 or imm32
                return ["mov%s $%#x,%s" % (s, rng.randrange(1 << size * 8), reg(dst, size))]
            if form == "c7":                            # c7 /0 with REX.W: imm32, sign-extended
                return ["movq $%d,%s" % (self.imm(8), reg(dst, 8))]
            pre, m = self.mem(size, exclude)
            return pre + ["mov%s $%d,%s" % (s, self.imm(size), m)]
        if kind == "lea":
            size = rng.choice([4, 8])
            if rng.random() < 0.5:                      # arithmetic: any registers, any values
                a, b = self.pick(), self.pick()
                d = rng.choice([0, rng.randrange(-128, 128), rng.randrange(-1 << 31, 1 << 31)])
                return ["lea %s(%%%s,%%%s,%d),%s" % (d if d else "", a, b, rng.choice([1, 2, 4, 8]),
                                                     reg(dst, size))]
            pre, m = self.mem(8, exclude)
            return pre + ["lea %s,%s" % (m, reg(dst, size))]
        if kind == "test":
            if rng.random() < 0.5:
                return ["test%s %s,%s" % (s, reg(src, size), reg(dst, size))]
            pre, m = self.mem(size, exclude)
            return pre + ["test%s %s,%s" % (s, reg(src, size), m)]
        if kind == "byte":                              # 8-bit forms: the framework
            lo = [r for r in ("rax", "rbx", "rcx", "rdx") if r not in exclude]
            return ["%sb %s,%s" % (rng.choice(ALU), reg(rng.choice(["rax", "rbx", "rcx", "rdx"]), 1),
                                   reg(rng.choice(lo), 1))]
        op = rng.choice(ALU)
        form = rng.choice(["rr", "rr", "load", "store", "imm", "imm", "immmem"])
        if form == "rr":
            return ["%s%s %s,%s" % (op, s, reg(src, size), reg(dst, size))]
        if form == "imm":
            return ["%s%s $%d,%s" % (op, s, self.imm(size), reg(dst, size))]
        pre, m = self.mem(size, exclude)
        if form == "load":
            return pre + ["%s%s %s,%s" % (op, s, m, reg(dst, size))]
        if form == "store":
            return pre + ["%s%s %s,%s" % (op, s, reg(src, size), m)]
        return pre + ["%s%s $%d,%s" % (op, s, self.imm(size), m)]

    def some(self, lo, hi, exclude=()):
        out = []
        for _ in range(self.rng.randrange(lo, hi + 1)):
            out += self.simple(exclude)
        return out

    def item(self):
        """One item of a block: an instruction, or a group with jumps."""
        rng = self.rng
        kind = rng.choice(["simple"] * 8 + ["stack", "stack", "call", "call", "jcc", "jcc",
                                              "jcc32", "jmp", "jmp32", "back", "loop"])
        if kind == "simple":
            return self.simple()
        if kind == "stack":
            a, b = self.pick(), self.pick()
            if rng.random() < 0.3:                      # two values, popped in reverse
                c, d = self.pick(), self.pick()
                return (["push %%%s" % a, "push %%%s" % c] + self.some(0, 1) +
                        ["pop %%%s" % d] + self.some(0, 1) + ["pop %%%s" % b])
            return ["push %%%s" % a] + self.some(0, 2) + ["pop %%%s" % b]
        if kind == "call":
            return ["call f%d" % rng.randrange(NFUNC)]
        if kind in ("jcc", "jcc32"):                    # forward, over one to three instructions
            lab, cc = self.new_label(), rng.randrange(16)
            out = self.some(1, 1)                       # sets the flags, most of the time
            if kind == "jcc":
                out.append("j%s %s" % (CC[cc], lab))
            else:
                out += [".byte 0x0f, %#x    # j%s rel32" % (0x80 + cc, CC[cc]),
                        ".long %s - . - 4" % lab]
            return out + self.some(1, 3) + [lab + ":"]
        if kind in ("jmp", "jmp32"):
            lab = self.new_label()
            out = ["jmp %s" % lab] if kind == "jmp" else \
                  [".byte 0xe9    # jmp rel32", ".long %s - . - 4" % lab]
            return out + self.some(1, 2) + [lab + ":"]
        if kind == "back":                              # a backward jmp, rel8 or rel32
            a, b, c = self.new_label(), self.new_label(), self.new_label()
            back = ["jmp %s" % b] if rng.random() < 0.5 else \
                   [".byte 0xe9    # jmp rel32", ".long %s - . - 4" % b]
            return (["jmp %s" % a, b + ":"] + self.some(1, 2) + ["jmp %s" % c, a + ":"] +
                    self.some(0, 1) + back + [c + ":"])
        lab = self.new_label()                          # a loop: a backward jcc
        back = ["jne %s" % lab] if rng.random() < 0.5 else \
               [".byte 0x0f, 0x85    # jne rel32", ".long %s - . - 4" % lab]
        return (["mov $%d,%%r15d" % rng.randrange(1, 5), lab + ":"] + self.some(1, 3, (LOOP,)) +
                ["sub $1,%r15d"] + back)

    def function(self, k):
        rng = self.rng
        out = ["f%d:" % k]
        saved = self.pick() if rng.random() < 0.5 else None
        if saved:
            out.append("push %%%s" % saved)
        out += self.some(1, 3)
        if k and rng.random() < 0.4:
            out += ["call f%d" % rng.randrange(k)] + self.some(0, 1)
        if saved:
            out.append("pop %%%s" % saved)
        return out + ["ret"]

    def program(self, n):
        """(assembly text, the lines of every block)."""
        rng = self.rng
        funcs = [self.function(k) for k in range(NFUNC)]
        out = ["\t.globl _start", "\t.text"]
        for f in funcs[:NFUNC // 2]:                    # called backwards
            out += [f[0]] + ["\t" + x for x in f[1:]]
        out += ["_start:", "\tlea buf(%rip),%rdi", "\tlea data+1024(%rip),%rbp",
                "\tlea data+512(%rip),%r12", "\tlea data+1536(%rip),%r13"]
        out += ["\tmovabs $%#x,%%%s" % (rng.randrange(1 << 64), r) for r in DUMP]
        blocks = []
        for k in range(n):
            body = []
            for _ in range(rng.randrange(4, 9)):
                body += self.item()
            if k == 0:                                  # the stack pointer as an operand
                body = STACK
            blocks.append(body)
            out.append("# block %d" % k)
            out += [x if x.endswith(":") else "\t" + x for x in body]
            out += ["\tmov %%%s,%d(%%rdi)" % (r, 8 * i) for i, r in enumerate(DUMP)]
            out += ["\tpushfq", "\tpop %rax", "\tand $0x8c5,%eax",
                    "\tmov %%rax,%d(%%rdi)" % (8 * len(DUMP)), "\tmov (%rdi),%rax",
                    "\tlea %d(%%rdi),%%rdi" % REC]
        out += ["# the end: write the records and the data area",
                "\tlea buf(%rip),%rsi", "\tmov $%d,%%ebx" % (REC * n), "\tcall put",
                "\tlea data(%rip),%rsi", "\tmov $%d,%%ebx" % DATA, "\tcall put",
                "\txor %edi,%edi", "\tmov $60,%eax", "\tsyscall",
                "put:", "\tmov $1,%eax", "\tmov $1,%edi", "\tmov %rbx,%rdx", "\tsyscall",
                "\ttest %rax,%rax", "\tjle fail",
                "\tadd %rax,%rsi", "\tsub %rax,%rbx", "\tjne put", "\tret",
                "fail:", "\tmov $1,%edi", "\tmov $60,%eax", "\tsyscall"]
        for f in funcs[NFUNC // 2:]:
            out += [f[0]] + ["\t" + x for x in f[1:]]
        out += ["\t.data", "data:"]
        out += ["\t.quad %#x" % rng.randrange(1 << 64) for _ in range(DATA // 8)]
        out += ["\t.bss", "buf:", "\t.zero %d" % (REC * n)]
        return "\n".join(out), blocks, funcs


def names(f):
    return " ".join(n for m, n in FLAGS if f & m) or "none"


def explain(seed, n, want_path, got_path):
    want = open(want_path, "rb").read()
    got = open(got_path, "rb").read()
    _, blocks, funcs = Gen(seed).program(n)
    if want == got:
        print("the outputs are equal")
        return
    for k in range(n):
        w, g = want[k * REC:(k + 1) * REC], got[k * REC:(k + 1) * REC]
        if w == g:
            continue
        if len(g) < REC:
            print("the output of mini-cpu ends before the record of block %d" % k)
            return
        print("block %d of randops.py %d %d (the registers were equal before it):" % (k, seed, n))
        wv, gv = struct.unpack("<12Q", w), struct.unpack("<12Q", g)
        for name, a, b in zip(DUMP, wv, gv):
            if a != b:
                print("  %-5s expected %#018x, got %#018x" % (name, a, b))
        if wv[-1] != gv[-1]:
            print("  flags expected: %s, got: %s" % (names(wv[-1]), names(gv[-1])))
        print("  the instructions of the block:")
        called = set()
        for line in blocks[k]:
            print("    " + line)
            if line.startswith("call f"):
                called.add(int(line[6:]))
        for c in sorted(called):
            print("  " + "; ".join(funcs[c]))
        return
    for i in range(n * REC, len(want)):
        if i >= len(got):
            print("the output of mini-cpu ends inside the data area")
            return
        if want[i] != got[i]:
            print("randops.py %d %d: every register record is equal, but the byte at data+%d"
                  " is %#04x, expected %#04x" % (seed, n, i - n * REC, got[i], want[i]))
            print("(a store wrote a wrong value, a wrong number of bytes, or to a wrong address)")
            return
    print("the output of mini-cpu is longer than expected")


def main():
    a = sys.argv[1:]
    if len(a) == 5 and a[0] == "--explain":
        explain(int(a[1]), int(a[2]), a[3], a[4])
    elif len(a) in (1, 2) and a[0].isdigit():
        print(Gen(int(a[0])).program(int(a[1]) if len(a) == 2 else 60)[0])
    else:
        sys.exit("usage: randops.py SEED [N]\n       randops.py --explain SEED N WANT GOT")


if __name__ == "__main__":
    main()
