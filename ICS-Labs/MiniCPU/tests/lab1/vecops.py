#!/usr/bin/env python3
"""vecops.py SEED [N]: a guest program of N blocks of random vector instructions.

    vecops.py SEED [N]                    print the program
    vecops.py --want SEED N               write the expected output to stdout
    vecops.py --explain SEED N GOT        describe the first difference

Prints x86-64 assembly for a program without libc that uses the seven
vector instructions of interp.c:

    vmovdqu  rm -> reg         vpxor  vpaddd  vpmulld      xmm and ymm
    vphaddd                    xmm only
    vextracti128 $i, ymm -> xmm or 16 bytes of memory
    vmovd    xmm -> r32 or 4 bytes of memory

on all sixteen registers, with register and memory operands. A write to an
xmm register clears the upper half of the ymm register; the program checks
that as well.

After every block the program appends a record of 72 bytes to a buffer:
two registers, 32 bytes each, stored with vextracti128, and %rax after a
vmovd into %eax. At the end it writes the buffer and a scratch area of 256
bytes, the target of the stores, to stdout.

The expected output is computed here, in Python, so the test does not
depend on the vector instructions of this machine. tests/lab1/run.sh
assembles the program, runs it under mini-cpu, and compares.
"""
import random
import struct
import sys

M32 = 0xffffffff
REC = 72
VDATA = 512                         # read-only operands, %rbp points to its middle
SCRATCH = 256                       # stores go here, %r12 points to its start
OPS = {"vpxor": lambda a, b: a ^ b,
       "vpaddd": lambda a, b: (a + b) & M32,
       "vpmulld": lambda a, b: (a * b) & M32}


class Gen:
    def __init__(self, seed):
        self.rng = random.Random(seed)
        self.data = [self.rng.choice([0, 1, M32, 0x7fffffff, 0x80000000, self.rng.randrange(1 << 32),
                                      self.rng.randrange(1 << 32), self.rng.randrange(1 << 32)])
                     for _ in range(VDATA // 4)]
        self.y = [[0] * 8 for _ in range(16)]           # lanes of ymm0 .. ymm15
        self.scratch = bytearray(SCRATCH)

    def name(self, r, n):
        return "%%%smm%d" % ("y" if n == 32 else "x", r)

    def source(self, n):
        """(lines to run first, operand text, lanes) of a source of n bytes:
        a register or memory in the data area."""
        rng = self.rng
        if rng.random() < 0.5:
            r = rng.randrange(16)
            return [], self.name(r, n), self.y[r][:n // 4]
        lane = rng.randrange(0, (VDATA - n) // 4 + 1)
        off = 4 * lane
        form = rng.choice(["rbp", "rbp", "rip", "abs", "index"])
        if form == "rbp":
            pre, text = [], "%d(%%rbp)" % (off - VDATA // 2)
        elif form == "rip":
            pre, text = [], "vdata+%d(%%rip)" % off
        elif form == "abs":
            pre, text = [], "vdata+%d" % off
        else:
            pre, text = ["mov $%d,%%esi" % lane], "%d(%%rbp,%%rsi,4)" % (-VDATA // 2)
        return pre, text, self.data[lane:lane + n // 4]

    def set(self, r, lanes):
        """Write lanes to register r; the lanes above are cleared."""
        self.y[r] = list(lanes) + [0] * (8 - len(lanes))

    def store(self, off, lanes):
        self.scratch[off:off + 4 * len(lanes)] = struct.pack("<%dI" % len(lanes), *lanes)

    def instruction(self):
        rng = self.rng
        kind = rng.choice(["vmovdqu", "vmovdqu", "op", "op", "op", "op", "vphaddd",
                           "vextracti128", "vextracti128", "vmovd"])
        n = rng.choice([16, 32])
        d = rng.randrange(16)
        if kind == "vmovdqu":
            pre, src, lanes = self.source(n)
            # the assembler may encode a register source 8..15 with opcode 7f,
            # the store form, which the machine does not have
            if src.startswith("%") and int(src[4:]) >= 8 and d < 8:
                d += 8
            self.set(d, lanes)
            return pre + ["vmovdqu %s,%s" % (src, self.name(d, n))]
        if kind == "op":
            op = rng.choice(list(OPS))
            v = rng.randrange(16)
            pre, src, b = self.source(n)
            a = self.y[v][:n // 4]
            self.set(d, [OPS[op](x, y) for x, y in zip(a, b)])
            return pre + ["%s %s,%s,%s" % (op, src, self.name(v, n), self.name(d, n))]
        if kind == "vphaddd":
            v = rng.randrange(16)
            pre, src, b = self.source(16)
            a = self.y[v]
            self.set(d, [(a[0] + a[1]) & M32, (a[2] + a[3]) & M32,
                         (b[0] + b[1]) & M32, (b[2] + b[3]) & M32])
            return pre + ["vphaddd %s,%s,%s" % (src, self.name(v, 16), self.name(d, 16))]
        s = rng.randrange(16)
        if kind == "vextracti128":
            i = rng.choice([0, 0, 1, 1, 1, 2, 3])       # only bit 0 counts
            half = self.y[s][4 * (i & 1):4 * (i & 1) + 4]
            if rng.random() < 0.5:
                self.set(d, half)
                return ["vextracti128 $%d,%s,%s" % (i, self.name(s, 32), self.name(d, 16))]
            off = rng.randrange(0, SCRATCH - 16 + 1)
            self.store(off, half)
            return ["vextracti128 $%d,%s,%d(%%r12)" % (i, self.name(s, 32), off)]
        off = rng.randrange(0, SCRATCH - 4 + 1)         # vmovd to memory
        self.store(off, self.y[s][:1])
        return ["vmovd %s,%d(%%r12)" % (self.name(s, 16), off)]

    def program(self, n):
        """(assembly text, expected output, the blocks). A block is
        (lines, registers recorded, expected record)."""
        rng = self.rng
        out = ["\t.globl _start", "\t.text", "_start:", "\tlea buf(%rip),%rdi",
               "\tlea vdata+%d(%%rip),%%rbp" % (VDATA // 2), "\tlea scratch(%rip),%r12"]
        for r in range(16):                             # every register starts with data
            lane = rng.randrange(0, VDATA // 4 - 7)
            self.set(r, self.data[lane:lane + 8])
            out.append("\tvmovdqu vdata+%d(%%rip),%%ymm%d" % (4 * lane, r))
        blocks, want = [], b""
        for k in range(n):
            body = []
            for _ in range(rng.randrange(4, 9)):
                body += self.instruction()
            a, b, c = rng.randrange(16), rng.randrange(16), rng.randrange(16)
            rec = struct.pack("<8I8IQ", *(self.y[a] + self.y[b] + [self.y[c][0]]))
            blocks.append((body, (a, b, c), rec))
            want += rec
            out.append("# block %d" % k)
            out += ["\t" + x for x in body]
            out += ["\tvextracti128 $0,%%ymm%d,(%%rdi)" % a, "\tvextracti128 $1,%%ymm%d,16(%%rdi)" % a,
                    "\tvextracti128 $0,%%ymm%d,32(%%rdi)" % b, "\tvextracti128 $1,%%ymm%d,48(%%rdi)" % b,
                    "\tmov $-1,%rax", "\tvmovd %%xmm%d,%%eax" % c, "\tmov %rax,64(%rdi)",
                    "\tlea %d(%%rdi),%%rdi" % REC]
        out += ["# the end: write the records and the scratch area",
                "\tlea buf(%rip),%rsi", "\tmov $%d,%%ebx" % (REC * n), "\tcall put",
                "\tlea scratch(%rip),%rsi", "\tmov $%d,%%ebx" % SCRATCH, "\tcall put",
                "\txor %edi,%edi", "\tmov $60,%eax", "\tsyscall",
                "put:", "\tmov $1,%eax", "\tmov $1,%edi", "\tmov %rbx,%rdx", "\tsyscall",
                "\ttest %rax,%rax", "\tjle fail",
                "\tadd %rax,%rsi", "\tsub %rax,%rbx", "\tjne put", "\tret",
                "fail:", "\tmov $1,%edi", "\tmov $60,%eax", "\tsyscall",
                "\t.data", "vdata:"]
        out += ["\t.long %#x" % v for v in self.data]
        out += ["\t.bss", "scratch:", "\t.zero %d" % SCRATCH, "buf:", "\t.zero %d" % (REC * n)]
        return "\n".join(out), want + bytes(self.scratch), blocks


def hexlanes(b):
    return " ".join("%08x" % v for v in struct.unpack("<%dI" % (len(b) // 4), b))


def explain(seed, n, got_path):
    _, want, blocks = Gen(seed).program(n)
    got = open(got_path, "rb").read()
    if want == got:
        print("the outputs are equal")
        return
    for k, (body, (a, b, c), rec) in enumerate(blocks):
        g = got[k * REC:(k + 1) * REC]
        if g == rec:
            continue
        if len(g) < REC:
            print("the output of mini-cpu ends before the record of block %d" % k)
            return
        print("block %d of vecops.py %d %d (the records before it are equal);"
              " lanes 0 .. 7 in hexadecimal:" % (k, seed, n))
        for name, i in (("ymm%d" % a, 0), ("ymm%d" % b, 32)):
            if rec[i:i + 32] != g[i:i + 32]:
                print("  %-5s expected %s" % (name, hexlanes(rec[i:i + 32])))
                print("  %-5s got      %s" % ("", hexlanes(g[i:i + 32])))
        if rec[64:] != g[64:]:
            print("  mov $-1,%%rax; vmovd %%xmm%d,%%eax: rax expected %#x, got %#x"
                  % (c, struct.unpack("<Q", rec[64:])[0], struct.unpack("<Q", g[64:])[0]))
        print("  the instructions of the block (an instruction of an earlier block that wrote"
              " a register not recorded there may be the cause as well):")
        for line in body:
            print("    " + line)
        return
    for i in range(n * REC, len(want)):
        if i >= len(got):
            print("the output of mini-cpu ends inside the scratch area")
            return
        if want[i] != got[i]:
            print("vecops.py %d %d: every record is equal, but byte %d of the scratch area"
                  " is %#04x, expected %#04x" % (seed, n, i - n * REC, got[i], want[i]))
            print("(vextracti128 or vmovd to memory stored a wrong value, a wrong number"
                  " of bytes, or to a wrong address)")
            return
    print("the output of mini-cpu is longer than expected")


def main():
    a = sys.argv[1:]
    if len(a) == 3 and a[0] == "--want":
        sys.stdout.buffer.write(Gen(int(a[1])).program(int(a[2]))[1])
    elif len(a) == 4 and a[0] == "--explain":
        explain(int(a[1]), int(a[2]), a[3])
    elif len(a) in (1, 2) and a[0].isdigit():
        print(Gen(int(a[0])).program(int(a[1]) if len(a) == 2 else 40)[0])
    else:
        sys.exit("usage: vecops.py SEED [N]\n       vecops.py --want SEED N\n"
                 "       vecops.py --explain SEED N GOT")


if __name__ == "__main__":
    main()
