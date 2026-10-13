/* mini-unit: direct checks of single functions of the labs.
 *
 *   mini-unit [--ram FILE] [--disk FILE] GROUP
 *
 * A group calls the functions of one file of src/labN/ with fixed
 * arguments, without a guest program, and compares what they return and
 * what they leave in memory with the contract in include/mcpu.h. The
 * tests of each lab (tests/labN/run.sh) run its groups before the guest
 * programs, so a function can be checked before the ones built on it
 * exist.
 *
 *   lab 1   modrm  alu  shift  muldiv  cond
 *   lab 2   seq  pipe
 *   lab 4   mmu
 *   lab 5   cache
 *   lab 6   blk  ramfs  sys
 *
 * One line is printed per failed check, at most 12 per group, then
 * "GROUP: N checks, M failed". The exit status is 0 when no check failed.
 * --ram names the file used as physical memory (default dram.img), --disk
 * a scratch file the groups of lab 6 overwrite with their own image.
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mcpu.h"

static const char *group = "?";
static const char *ram_path = "dram.img";
static const char *disk_path = "unit-disk.img";
static unsigned checks, failed;

/* What the group is doing, for the message when a signal ends it: set
   before a call that tends to end that way (a division by zero, a copy
   past the end of an array). */
static const char *doing;

static void on_signal(int sig)
{
    printf("%s: ended by signal %d (%s)%s%s\n", group, sig, strsignal(sig),
           doing ? " in " : "", doing ? doing : "");
    printf("%s: %u checks so far, %u failed\n", group, checks, failed);
    fflush(stdout);
    _exit(1);
}

/* One check: ok, or a line saying what was expected. */
__attribute__((format(printf, 2, 3)))
static int check(int ok, const char *fmt, ...)
{
    ++checks;
    if (ok)
        return 1;
    if (++failed <= 12) {
        va_list ap;
        printf("%s: ", group);
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
        putchar('\n');
    }
    return 0;
}

/* The arguments are evaluated once: got is often a call with an effect. */
__attribute__((unused))
static int eq(const char *what, uint64_t want, uint64_t got)
{
    return check(want == got, "%s: expected %#llx, got %#llx", what,
                 (unsigned long long)want, (unsigned long long)got);
}

#define EQ(what, want, got) eq(what, (uint64_t)(want), (uint64_t)(got))

/* A deterministic sequence of 64-bit numbers. */
static uint64_t rnd_state = 0x9e3779b97f4a7c15ULL;
static uint64_t rnd(void)
{
    rnd_state ^= rnd_state << 13;
    rnd_state ^= rnd_state >> 7;
    rnd_state ^= rnd_state << 17;
    return rnd_state;
}

#define FLAGS5 ((uint64_t)(CF | PF | ZF | SF | OF))

static const char *flagtext(uint64_t f)
{
    static char s[4][24];
    static int k;
    char *p = s[k = (k + 1) & 3];
    snprintf(p, sizeof s[0], "%s%s%s%s%s", f & CF ? "C" : "-", f & PF ? "P" : "-",
             f & ZF ? "Z" : "-", f & SF ? "S" : "-", f & OF ? "O" : "-");
    return p;
}

/* ------------------------------------------------------------ lab 1: modrm */

static const struct {
    const char *text;
    uint8_t     b[6];
    uint8_t     rex;
    unsigned    used;
    int         mod, sub, reg, rm, base, index, scale, rip, dsize;
    int64_t     disp;
} MODRM[] = {
    { "%ebx",                 { 0xc3 }, 0, 1,                         3, 0, 0, 3, -1, -1, 1, 0, 0, 0 },
    { "%r11, reg %r9",        { 0xcb }, 0x45, 1,                      3, 1, 9, 11, -1, -1, 1, 0, 0, 0 },
    { "(%rax)",               { 0x00 }, 0, 1,                         0, 0, 0, -1, 0, -1, 1, 0, 0, 0 },
    { "(%r9), /7",            { 0x39 }, 0x41, 1,                      0, 7, 7, -1, 9, -1, 1, 0, 0, 0 },
    { "(%rax), reg %r15",     { 0x38 }, 0x44, 1,                      0, 7, 15, -1, 0, -1, 1, 0, 0, 0 },
    { "-0x8(%rax)",           { 0x40, 0xf8 }, 0, 2,                   1, 0, 0, -1, 0, -1, 1, 0, 1, -8 },
    { "0x0(%rbp)",            { 0x45, 0x00 }, 0, 2,                   1, 0, 0, -1, 5, -1, 1, 0, 1, 0 },
    { "0x0(%r13)",            { 0x45, 0x00 }, 0x41, 2,                1, 0, 0, -1, 13, -1, 1, 0, 1, 0 },
    { "0x100(%rax)",          { 0x80, 0x00, 0x01, 0x00, 0x00 }, 0, 5, 2, 0, 0, -1, 0, -1, 1, 0, 4, 0x100 },
    { "-0x80000000(%rdi)",    { 0x87, 0x00, 0x00, 0x00, 0x80 }, 0, 5, 2, 0, 0, -1, 7, -1, 1, 0, 4, -0x80000000LL },
    { "0x10(%rip)",           { 0x05, 0x10, 0x00, 0x00, 0x00 }, 0, 5, 0, 0, 0, -1, -1, -1, 1, 1, 4, 0x10 },
    { "-0x4(%rip), REX.B",    { 0x05, 0xfc, 0xff, 0xff, 0xff }, 0x41, 5, 0, 0, 0, -1, -1, -1, 1, 1, 4, -4 },
    { "(%rbx,%rcx,4)",        { 0x04, 0x8b }, 0, 2,                   0, 0, 0, -1, 3, 1, 4, 0, 0, 0 },
    { "(%rsp)",               { 0x04, 0x24 }, 0, 2,                   0, 0, 0, -1, 4, -1, 1, 0, 0, 0 },
    { "(%rsp), scale bits 01", { 0x04, 0x64 }, 0, 2,                  0, 0, 0, -1, 4, -1, 1, 0, 0, 0 },
    { "(%r12)",               { 0x04, 0x24 }, 0x41, 2,                0, 0, 0, -1, 12, -1, 1, 0, 0, 0 },
    { "(%rax,%r12,1)",        { 0x04, 0x20 }, 0x42, 2,                0, 0, 0, -1, 0, 12, 1, 0, 0, 0 },
    { "0x10(,%rax,8)",        { 0x04, 0xc5, 0x10, 0x00, 0x00, 0x00 }, 0, 6, 0, 0, 0, -1, -1, 0, 8, 0, 4, 0x10 },
    { "0x20(,%r8,4), REX.B",  { 0x04, 0x85, 0x20, 0x00, 0x00, 0x00 }, 0x43, 6, 0, 0, 0, -1, -1, 8, 4, 0, 4, 0x20 },
    { "0x10, no base, no index", { 0x04, 0x25, 0x10, 0x00, 0x00, 0x00 }, 0, 6, 0, 0, 0, -1, -1, -1, 1, 0, 4, 0x10 },
    { "0x8(%rsp)",            { 0x44, 0x24, 0x08 }, 0, 3,             1, 0, 0, -1, 4, -1, 1, 0, 1, 8 },
    { "0x7f(%rbp,%rsi,2)",    { 0x44, 0x75, 0x7f }, 0, 3,             1, 0, 0, -1, 5, 6, 2, 0, 1, 0x7f },
    { "0x0(%r13,%r9,8)",      { 0x44, 0xcd, 0x00 }, 0x43, 3,          1, 0, 0, -1, 13, 9, 8, 0, 1, 0 },
    { "-0x100(%rbx,%rdx,2)",  { 0x84, 0x53, 0x00, 0xff, 0xff, 0xff }, 0, 6, 2, 0, 0, -1, 3, 2, 2, 0, 4, -0x100 },
};

static void field(unsigned i, const char *name, int64_t want, int64_t got)
{
    check(want == got, "%02x %02x .. rex %02x, operand %s: %s expected %lld, got %lld",
          MODRM[i].b[0], MODRM[i].b[1], MODRM[i].rex, MODRM[i].text, name,
          (long long)want, (long long)got);
}

static void t_modrm(void)
{
    for (unsigned i = 0; i < sizeof MODRM / sizeof MODRM[0]; ++i) {
        uint8_t buf[8];
        modrm_t m;
        memset(buf, 0xaa, sizeof buf);
        memcpy(buf, MODRM[i].b, MODRM[i].used);

        memset(&m, 0x5a, sizeof m);
        unsigned k = modrm_decode(buf, sizeof buf, MODRM[i].rex, &m);
        field(i, "bytes used", MODRM[i].used, k);
        field(i, "mod", MODRM[i].mod, m.mod);
        field(i, "sub", MODRM[i].sub, m.sub);
        field(i, "reg", MODRM[i].reg, m.reg);
        field(i, "rm", MODRM[i].rm, m.rm);
        field(i, "base", MODRM[i].base, m.base);
        field(i, "index", MODRM[i].index, m.index);
        field(i, "scale", MODRM[i].scale, m.scale);
        field(i, "rip", MODRM[i].rip, m.rip);
        field(i, "dsize", MODRM[i].dsize, m.dsize);
        field(i, "disp", MODRM[i].disp, m.disp);

        /* exactly enough bytes, then every shorter buffer */
        k = modrm_decode(buf, MODRM[i].used, MODRM[i].rex, &m);
        field(i, "bytes used when n is just enough", MODRM[i].used, k);
        for (unsigned n = 1; n < MODRM[i].used; ++n) {
            k = modrm_decode(buf, n, MODRM[i].rex, &m);
            check(k == 0, "%02x %02x .. rex %02x, operand %s: with n = %u the bytes end "
                  "inside the operand, expected 0, got %u",
                  MODRM[i].b[0], MODRM[i].b[1], MODRM[i].rex, MODRM[i].text, n, k);
        }
    }
}

/* -------------------------------------------------------------- lab 1: alu */

typedef __int128 i128;
typedef unsigned __int128 u128;

/* The result and the five flags of a + b + c (sub 0) or a - b - c (sub 1)
   on size bytes, from arithmetic wide enough to hold the true result. */
static uint64_t wide(int sub, uint64_t a, uint64_t b, int c, int size, uint64_t *flags)
{
    int bits = size * 8;
    u128 mask = ((u128)1 << bits) - 1;
    u128 ua = a & mask, ub = b & mask;
    i128 sa = (i128)sext(a, size), sb = (i128)sext(b, size);
    u128 full = sub ? ua - ub - (u128)c : ua + ub + (u128)c;
    i128 s = sub ? sa - sb - c : sa + sb + c;
    uint64_t r = (uint64_t)(full & mask), f = 0;

    if (sub ? ua < ub + (u128)c : (full >> bits) != 0) f |= CF;
    if (s != (i128)sext(r, size)) f |= OF;
    if (r == 0) f |= ZF;
    if (r >> (bits - 1) & 1) f |= SF;
    if (!(__builtin_popcountll(r & 0xff) & 1)) f |= PF;
    *flags = f;
    return r;
}

static void alu_case(int sub, uint64_t a, uint64_t b, int c, int size)
{
    uint64_t wf, wr = wide(sub, a, b, c, size, &wf);
    uint64_t in = rnd(), f = in;
    uint64_t r = sub ? x86_sub(&f, a, b, size, c) : x86_add(&f, a, b, size, c);
    const char *op = sub ? "x86_sub" : "x86_add";

    check(r == wr, "%s(a = %#llx, b = %#llx, size %d, %s %d): result expected %#llx, got %#llx",
          op, (unsigned long long)a, (unsigned long long)b, size, sub ? "borrow" : "carry", c,
          (unsigned long long)wr, (unsigned long long)r);
    check((f & FLAGS5) == wf,
          "%s(a = %#llx, b = %#llx, size %d, %s %d): flags expected %s, got %s",
          op, (unsigned long long)a, (unsigned long long)b, size, sub ? "borrow" : "carry", c,
          flagtext(wf), flagtext(f));
    check((f & ~FLAGS5) == (in & ~FLAGS5),
          "%s changed a bit of *f outside CF, PF, ZF, SF, OF: %#llx became %#llx",
          op, (unsigned long long)in, (unsigned long long)f);
}

/* x86_inc (dec 0) and x86_dec (dec 1): the flags of v + 1 and v - 1,
   except CF, which stays as it was. */
static void incdec_case(int dec, uint64_t v, int size)
{
    uint64_t wf, wr = wide(dec, v, 1, 0, size, &wf);
    uint64_t in = rnd(), f = in;
    uint64_t r = dec ? x86_dec(&f, v, size) : x86_inc(&f, v, size);
    const char *op = dec ? "x86_dec" : "x86_inc";

    wf = (wf & ~(uint64_t)CF) | (in & CF);
    check(r == wr, "%s(v = %#llx, size %d): result expected %#llx, got %#llx",
          op, (unsigned long long)v, size, (unsigned long long)wr, (unsigned long long)r);
    check((f & FLAGS5) == wf,
          "%s(v = %#llx, size %d), flags %s before: expected %s, got %s (CF stays as it was)",
          op, (unsigned long long)v, size, flagtext(in), flagtext(wf), flagtext(f));
    check((f & ~FLAGS5) == (in & ~FLAGS5),
          "%s changed a bit of *f outside CF, PF, ZF, SF, OF: %#llx became %#llx",
          op, (unsigned long long)in, (unsigned long long)f);
}

/* x86_neg: the result and the flags of 0 - v. */
static void neg_case(uint64_t v, int size)
{
    uint64_t wf, wr = wide(1, 0, v, 0, size, &wf);
    uint64_t in = rnd(), f = in;
    uint64_t r = x86_neg(&f, v, size);

    check(r == wr, "x86_neg(v = %#llx, size %d): result expected %#llx, got %#llx",
          (unsigned long long)v, size, (unsigned long long)wr, (unsigned long long)r);
    check((f & FLAGS5) == wf, "x86_neg(v = %#llx, size %d): flags expected %s, got %s",
          (unsigned long long)v, size, flagtext(wf), flagtext(f));
    check((f & ~FLAGS5) == (in & ~FLAGS5),
          "x86_neg changed a bit of *f outside CF, PF, ZF, SF, OF: %#llx became %#llx",
          (unsigned long long)in, (unsigned long long)f);
}

/* v[0..15]: values of size bytes around 0, the top bit and all ones, and
   five random ones. */
static void edge_values(int size, uint64_t v[16])
{
    uint64_t top = 1ULL << (size * 8 - 1), all = top | (top - 1);
    v[0] = 0; v[1] = 1; v[2] = 2; v[3] = top - 1; v[4] = top; v[5] = top + 1;
    v[6] = all - 1; v[7] = all; v[8] = 0x7f; v[9] = 0x80; v[10] = 0xff;
    for (int k = 11; k < 16; ++k)
        v[k] = rnd() & all;
}

/* Bits above a size-byte operand, zero one time in four: the functions
   must not look at them. */
static uint64_t junk(int size)
{
    uint64_t r = rnd();
    return size < 8 && r >> 62 ? r << (size * 8) : 0;
}

static void t_alu(void)
{
    /* x86_szp */
    for (int size = 1; size <= 8; size *= 2)
        for (int k = 0; k < 400; ++k) {
            uint64_t r = k < 8 ? (uint64_t)k << (size * 8 - 3) : rnd() >> (rnd() % 64);
            if (k % 5 == 0 && size < 8)
                r &= ~0ULL << (size * 8);               /* zero in the operand, junk above */
            uint64_t in = rnd(), want = in & ~(uint64_t)(ZF | SF | PF);
            if (trunc_val(r, size) == 0) want |= ZF;
            if (r >> (size * 8 - 1) & 1) want |= SF;
            if (!(__builtin_popcountll(r & 0xff) & 1)) want |= PF;
            uint64_t got = x86_szp(in, r, size);
            check(got == want, "x86_szp(f = %#llx, r = %#llx, size %d): expected %#llx, got %#llx"
                  " (ZF, SF, PF from the low %d bytes; every other bit kept)",
                  (unsigned long long)in, (unsigned long long)r, size,
                  (unsigned long long)want, (unsigned long long)got, size);
        }

    /* x86_logic */
    for (int size = 1; size <= 8; size *= 2)
        for (int k = 0; k < 200; ++k) {
            uint64_t r = k % 7 == 0 ? rnd() << (size * 4) : rnd();
            uint64_t in = rnd(), f = in;
            uint64_t want = (x86_szp(in, r, size) & ~(uint64_t)(CF | OF));
            uint64_t got = x86_logic(&f, r, size);
            check(got == trunc_val(r, size), "x86_logic(r = %#llx, size %d): returned %#llx, "
                  "expected the low %d bytes, %#llx", (unsigned long long)r, size,
                  (unsigned long long)got, size, (unsigned long long)trunc_val(r, size));
            check(f == want, "x86_logic(f = %#llx, r = %#llx, size %d): flags expected %#llx, "
                  "got %#llx (CF and OF cleared, ZF SF PF from r)", (unsigned long long)in,
                  (unsigned long long)r, size, (unsigned long long)want, (unsigned long long)f);
        }

    /* x86_add and x86_sub: every pair of bytes, with junk above the operand */
    for (int sub = 0; sub < 2; ++sub)
        for (unsigned a = 0; a < 256; ++a)
            for (unsigned b = 0; b < 256; ++b) {
                uint64_t hi = (a * 257 + b) % 3 ? rnd() << 8 : 0;
                alu_case(sub, hi | a, (hi >> 3 << 8) | b, 0, 1);
                alu_case(sub, hi | a, (hi >> 3 << 8) | b, 1, 1);
            }

    /* the wider sizes: values around 0, the top bit and all ones */
    for (int size = 2; size <= 8; size *= 2) {
        uint64_t top = 1ULL << (size * 8 - 1), all = top | (top - 1), v[16];
        v[0] = 0; v[1] = 1; v[2] = 2; v[3] = top - 1; v[4] = top; v[5] = top + 1;
        v[6] = all - 1; v[7] = all; v[8] = 0x7f; v[9] = 0x80; v[10] = 0xff;
        for (int k = 11; k < 16; ++k)
            v[k] = rnd() & all;
        for (int sub = 0; sub < 2; ++sub)
            for (int i = 0; i < 16; ++i)
                for (int j = 0; j < 16; ++j) {
                    uint64_t hi = size < 8 && (i + j) % 2 ? rnd() << (size * 8) : 0;
                    alu_case(sub, hi | v[i], hi | v[j], 0, size);
                    alu_case(sub, hi | v[i], hi | v[j], 1, size);
                }
    }

    /* x86_inc, x86_dec and x86_neg: every byte, then the wider sizes */
    for (unsigned a = 0; a < 256; ++a)
        for (int k = 0; k < 4; ++k) {
            incdec_case(0, junk(1) | a, 1);
            incdec_case(1, junk(1) | a, 1);
            neg_case(junk(1) | a, 1);
        }
    for (int size = 2; size <= 8; size *= 2) {
        uint64_t v[16];
        edge_values(size, v);
        for (int i = 0; i < 16; ++i)
            for (int k = 0; k < 4; ++k) {
                incdec_case(0, junk(size) | v[i], size);
                incdec_case(1, junk(size) | v[i], size);
                neg_case(junk(size) | v[i], size);
            }
    }
}

/* ------------------------------------------------------------ lab 1: shift */

static const char *const SHIFT[8] = { "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" };

/* The result and the five flags of the shift sub of v by n > 0 bits, done
   as n shifts by one bit. */
static uint64_t one_bit_shifts(int sub, uint64_t v, unsigned n, int size, uint64_t *flags)
{
    uint64_t top = 1ULL << (size * 8 - 1), all = top | (top - 1);
    uint64_t r = v & all, sign = r & top, f = 0;
    int cf = 0, of;

    for (unsigned i = 0; i < n; ++i)
        if (sub == 5 || sub == 7) {
            cf = (int)(r & 1);
            r = r >> 1 | (sub == 7 ? sign : 0);
        } else {
            cf = (r & top) != 0;
            r = r << 1 & all;
        }
    of = sub == 5 ? sign != 0 : sub == 7 ? 0 : ((r & top) != 0) != cf;
    if (cf) f |= CF;
    if (of) f |= OF;
    if (r == 0) f |= ZF;
    if (r & top) f |= SF;
    if (!(__builtin_popcountll(r & 0xff) & 1)) f |= PF;
    *flags = f;
    return r;
}

static void shift_case(int sub, uint64_t v, unsigned n, int size)
{
    unsigned count = n & (size == 8 ? 63 : 31);
    uint64_t in = rnd(), f = in, wf = in & FLAGS5, wr = trunc_val(v, size);
    if (count)
        wr = one_bit_shifts(sub, v, count, size, &wf);
    uint64_t r = x86_shift(&f, sub, v, n, size);

    check(r == wr, "x86_shift(%s, v = %#llx, n = %u, size %d): result expected %#llx, got %#llx",
          SHIFT[sub], (unsigned long long)v, n, size, (unsigned long long)wr, (unsigned long long)r);
    if (count)
        check((f & FLAGS5) == wf,
              "x86_shift(%s, v = %#llx, n = %u, size %d): flags expected %s, got %s",
              SHIFT[sub], (unsigned long long)v, n, size, flagtext(wf), flagtext(f));
    else
        check(f == in, "x86_shift(%s, v = %#llx, n = %u, size %d): the count is 0 after "
              "masking, flags expected unchanged %s, got %s",
              SHIFT[sub], (unsigned long long)v, n, size, flagtext(in), flagtext(f));
    check((f & ~FLAGS5) == (in & ~FLAGS5),
          "x86_shift changed a bit of *f outside CF, PF, ZF, SF, OF: %#llx became %#llx",
          (unsigned long long)in, (unsigned long long)f);
}

static void t_shift(void)
{
    /* every count a shift instruction can pass: imm8 or cl */
    for (int size = 1; size <= 8; size *= 2) {
        uint64_t v[16];
        edge_values(size, v);
        for (int sub = 4; sub < 8; ++sub)
            for (int i = 0; i < 16; ++i)
                for (unsigned n = 0; n < 256; ++n)
                    shift_case(sub, junk(size) | v[i], n, size);
    }
}

/* ----------------------------------------------------------- lab 1: muldiv */

static void mul_case(int sub, uint64_t a, uint64_t b, int size)
{
    int bits = size * 8, over;
    u128 mask = ((u128)1 << bits) - 1, p;
    const char *op = sub == 4 ? "mul" : "imul";

    if (sub == 4) {
        p = (a & mask) * (b & mask);
        over = p >> bits != 0;
    } else {
        i128 sp = (i128)sext(a, size) * (i128)sext(b, size);
        p = (u128)sp;
        over = sp != (i128)sext((uint64_t)sp, size);
    }
    uint64_t wlo = (uint64_t)(p & mask), whi = (uint64_t)(p >> bits & mask);
    uint64_t wf = over ? CF | OF : 0;
    if (wlo == 0) wf |= ZF;
    if (wlo >> (bits - 1) & 1) wf |= SF;
    if (!(__builtin_popcountll(wlo & 0xff) & 1)) wf |= PF;

    uint64_t in = rnd(), f = in, lo = rnd(), hi = rnd();
    x86_mul(&f, sub, a, b, size, &lo, &hi);
    check(lo == wlo && hi == whi,
          "x86_mul(%s, a = %#llx, b = %#llx, size %d): expected lo %#llx, hi %#llx, "
          "got lo %#llx, hi %#llx", op, (unsigned long long)a, (unsigned long long)b, size,
          (unsigned long long)wlo, (unsigned long long)whi,
          (unsigned long long)lo, (unsigned long long)hi);
    check((f & FLAGS5) == wf, "x86_mul(%s, a = %#llx, b = %#llx, size %d): flags expected %s, got %s",
          op, (unsigned long long)a, (unsigned long long)b, size, flagtext(wf), flagtext(f));
    check((f & ~FLAGS5) == (in & ~FLAGS5),
          "x86_mul changed a bit of *f outside CF, PF, ZF, SF, OF: %#llx became %#llx",
          (unsigned long long)in, (unsigned long long)f);
}

static void div_case(int sub, uint64_t hi, uint64_t lo, uint64_t v, int size)
{
    static char what[160];
    int bits = size * 8, err;
    u128 mask = ((u128)1 << bits) - 1;
    u128 un = (hi & mask) << bits | (lo & mask);
    uint64_t uv = (uint64_t)(v & mask), wq = 0, wr = 0;
    const char *op = sub == 6 ? "div" : "idiv";

    if (uv == 0) {
        err = 1;
    } else if (sub == 6) {
        err = un / uv > mask;
        wq = (uint64_t)(un / uv);
        wr = (uint64_t)(un % uv);
    } else {
        i128 sn = size == 8 ? (i128)un : (i128)sext((uint64_t)un, size * 2);
        i128 sv = sext(v, size);
        if (sv == -1 && sn == (i128)((u128)1 << 127)) {
            err = 1;                                    /* the quotient is 2^127 */
        } else {
            err = sn / sv != (i128)sext((uint64_t)(sn / sv), size);
            wq = (uint64_t)((u128)(sn / sv) & mask);
            wr = (uint64_t)((u128)(sn % sv) & mask);
        }
    }

    snprintf(what, sizeof what, "x86_div(%s, hi = %#llx, lo = %#llx, v = %#llx, size %d)", op,
             (unsigned long long)hi, (unsigned long long)lo, (unsigned long long)v, size);
    uint64_t q = rnd(), r = rnd();
    doing = what;
    int got = x86_div(sub, hi, lo, v, size, &q, &r);
    doing = NULL;
    if (err) {
        check(got == -1, "%s: expected -1 (%s), got %d", what,
              uv == 0 ? "v is zero" : "the quotient does not fit in size bytes", got);
        return;
    }
    check(got == 0, "%s: expected 0, got %d", what, got);
    if (got == 0)
        check(q == wq && r == wr, "%s: expected q %#llx, r %#llx, got q %#llx, r %#llx", what,
              (unsigned long long)wq, (unsigned long long)wr,
              (unsigned long long)q, (unsigned long long)r);
}

static void t_muldiv(void)
{
    /* x86_mul: every pair of bytes, then the wider sizes */
    for (int sub = 4; sub <= 5; ++sub)
        for (unsigned a = 0; a < 256; ++a)
            for (unsigned b = 0; b < 256; ++b)
                mul_case(sub, junk(1) | a, junk(1) | b, 1);
    for (int size = 2; size <= 8; size *= 2) {
        uint64_t v[16];
        edge_values(size, v);
        for (int sub = 4; sub <= 5; ++sub)
            for (int i = 0; i < 16; ++i)
                for (int j = 0; j < 16; ++j)
                    mul_case(sub, junk(size) | v[i], junk(size) | v[j], size);
    }

    /* x86_div: every 16-bit dividend by 16 bytes, then the wider sizes */
    for (int size = 1; size <= 8; size *= 2) {
        uint64_t v[16];
        edge_values(size, v);
        for (int sub = 6; sub <= 7; ++sub) {
            if (size == 1) {
                for (unsigned a = 0; a < 65536; ++a)
                    for (int j = 0; j < 16; ++j)
                        div_case(sub, junk(1) | a >> 8, junk(1) | (a & 0xff), junk(1) | v[j], 1);
                continue;
            }
            for (int h = 0; h < 16; ++h)
                for (int i = 0; i < 16; ++i)
                    for (int j = 0; j < 16; ++j)
                        div_case(sub, junk(size) | v[h], junk(size) | v[i], junk(size) | v[j], size);
        }
    }
}

/* ------------------------------------------------------------- lab 1: cond */

static void t_cond(void)
{
    static const char *const name[16] = { "o", "no", "b", "ae", "e", "ne", "be", "a",
                                          "s", "ns", "p", "np", "l", "ge", "le", "g" };
    static const uint64_t bit[5] = { CF, PF, ZF, SF, OF };

    for (int cc = 0; cc < 16; ++cc)
        for (int m = 0; m < 32; ++m) {
            uint64_t f = rnd() & ~FLAGS5;
            for (int i = 0; i < 5; ++i)
                if (m >> i & 1)
                    f |= bit[i];
            int cf = !!(f & CF), pf = !!(f & PF), zf = !!(f & ZF),
                sf = !!(f & SF), of = !!(f & OF), want;
            switch (cc >> 1) {
            case 0:  want = of; break;
            case 1:  want = cf; break;
            case 2:  want = zf; break;
            case 3:  want = cf | zf; break;
            case 4:  want = sf; break;
            case 5:  want = pf; break;
            case 6:  want = sf ^ of; break;
            default: want = zf | (sf ^ of); break;
            }
            want ^= cc & 1;                             /* an odd code is the negation */
            int got = cond(f, cc);
            check(got == want, "cond(flags %s, cc %#x \"%s\"): expected %d, got %d",
                  flagtext(f), cc, name[cc], want, got);
            /* the opcodes pass their whole number: 7x, 0f 8x, 0f 4x, 0f 9x */
            int g2 = cond(f, 0x70 | cc), g3 = cond(f, 0x180 | cc);
            check(g2 == want && g3 == want,
                  "cond(flags %s, cc %#x) and cond(.., %#x): only the low 4 bits of cc count; "
                  "expected %d, got %d and %d", flagtext(f), 0x70 | cc, 0x180 | cc, want, g2, g3);
        }
}

/* --------------------------------------------------------------- a machine */

#if LAB >= 2

#define CODE 0x00400000ULL          /* one page of code */
#define DATA 0x00500000ULL          /* two pages of data on frames that are not adjacent */
#define HOLE 0x00600000ULL          /* never mapped */

static mcpu cpu;

static uint64_t map_page(uint64_t va, uint64_t flags)
{
    uint64_t f = mmu_alloc_frame();
    if (!f || mmu_map(va, f, flags) < 0) {
        printf("%s: cannot map %#llx\n", group, (unsigned long long)va);
        exit(2);
    }
    mmu_tlb_flush();
    return f;
}

static void machine(void)
{
    if (dram_open(ram_path, GUEST_RAM) < 0)
        exit(2);
#if LAB >= 5
    cache_init(64, 4);
#else
    cache_init(0, 0);
#endif
    memset(dram_ptr(0x1000, PAGE_SIZE), 0, PAGE_SIZE);
    mmu_set_cr3(0x1000);
    memset(&cpu, 0, sizeof cpu);
    cpu.rflags = 0x2;
    map_page(CODE, PTE_P | PTE_W | PTE_U);
    map_page(DATA, PTE_P | PTE_W | PTE_U);
    mmu_alloc_frame();                                  /* a gap between the two frames */
    map_page(DATA + PAGE_SIZE, PTE_P | PTE_W | PTE_U);
}

static void poke(uint64_t va, const void *p, unsigned n)
{
    if (mem_write(&cpu, va, p, n) < 0) {
        printf("%s: cannot write guest memory at %#llx\n", group, (unsigned long long)va);
        exit(2);
    }
}

static uint64_t peek(uint64_t va, unsigned n)
{
    uint64_t v = 0;
    if (mem_read(&cpu, va, &v, n, MEM_READ) < 0) {
        printf("%s: cannot read guest memory at %#llx\n", group, (unsigned long long)va);
        exit(2);
    }
    return v;
}

/* Decode the n bytes b as the instruction at pc. */
static insn dec(uint64_t pc, const char *bytes, unsigned n)
{
    insn d;
    decode(&d, pc, (const uint8_t *)bytes, n);
    return d;
}

/* Give register r of d the value v in in[]. */
static void give(insn *d, int r, uint64_t v)
{
    int k = insn_src(d, r);
    if (k >= 0) {
        memset(&d->in[k], 0, sizeof d->in[k]);
        d->in[k].q[0] = v;
    }
}

static uint64_t outval(const insn *d, int r)
{
    int k = insn_dst(d, r);
    return k < 0 ? 0xbad0bad0bad0bad0ULL : d->out[k].q[0];
}

/* -------------------------------------------------------------- lab 2: seq */

static void t_seq(void)
{
    mcpu *c = &cpu;
    insn d;
    uint64_t v;

    machine();

    /* a load: the address in execute, the value in memory */
    d = dec(CODE, "\x8b\x44\x8b\x10", 4);               /* mov 0x10(%rbx,%rcx,4),%eax */
    give(&d, RBX, DATA);
    give(&d, RCX, 3);
    v = 0xdeadbeef11223344ULL;
    poke(DATA + 0x1c, &v, 8);
    d.out[0].q[0] = 0x5555;
    execute(&d);
    EQ("execute, mov 0x10(%rbx,%rcx,4),%eax with rbx = 0x500000, rcx = 3: ea", DATA + 0x1c, d.ea);
    EQ("execute of a load: out[] before memory() (compute must wait for the loaded value)",
       0x5555, d.out[0].q[0]);
    memory(c, &d);
    EQ("memory, the same instruction: fault", 0, d.fault);
    EQ("memory, the same instruction: lval, 4 bytes loaded into a cleared u256",
       0x11223344, d.lval.q[0]);
    EQ("memory, the same instruction: out[] of rax", 0x11223344, outval(&d, RAX));
    c->r[RAX] = ~0ULL;
    writeback(c, &d);
    EQ("writeback, the same instruction: rax", 0x11223344, c->r[RAX]);
    EQ("writeback, the same instruction: rip", CODE + 4, c->rip);

    /* a store */
    d = dec(CODE, "\x89\x03", 2);                       /* mov %eax,(%rbx) */
    give(&d, RAX, 0xaabbccdd99887766ULL);
    give(&d, RBX, DATA + 0x100);
    v = 0xeeeeeeeeeeeeeeeeULL;
    poke(DATA + 0x100, &v, 8);
    execute(&d);
    EQ("execute, mov %eax,(%rbx): ea", DATA + 0x100, d.ea);
    EQ("execute, mov %eax,(%rbx): sa", DATA + 0x100, d.sa);
    EQ("memory, mov %eax,(%rbx): memory before memory() runs", 0xeeeeeeeeeeeeeeeeULL,
       peek(DATA + 0x100, 8));
    memory(c, &d);
    EQ("memory, mov %eax,(%rbx): fault", 0, d.fault);
    EQ("memory, mov %eax,(%rbx): the 4 bytes stored, the next 4 untouched",
       0xeeeeeeee99887766ULL, peek(DATA + 0x100, 8));

    /* lea: an address without an access; the page is not mapped */
    d = dec(CODE, "\x8d\x44\x4b\x08", 4);               /* lea 0x8(%rbx,%rcx,2),%eax */
    give(&d, RBX, HOLE);
    give(&d, RCX, 0x10);
    execute(&d);
    EQ("execute, lea 0x8(%rbx,%rcx,2),%eax: out[] of rax", (HOLE + 0x28) & 0xffffffffu,
       outval(&d, RAX));
    memory(c, &d);
    EQ("memory, lea: fault (lea does not access its address)", 0, d.fault);

    /* rip-relative: decode has put pc + len into disp */
    d = dec(CODE + 0x20, "\x8b\x05\x10\x00\x00\x00", 6);  /* mov 0x10(%rip),%eax */
    execute(&d);
    EQ("execute, mov 0x10(%rip),%eax at 0x400020: ea", CODE + 0x20 + 6 + 0x10, d.ea);

    /* a register operation is complete after execute */
    d = dec(CODE, "\x01\xd8", 2);                       /* add %ebx,%eax */
    give(&d, RAX, 0xffffffff);
    give(&d, RBX, 1);
    execute(&d);
    EQ("execute, add %ebx,%eax with eax = 0xffffffff, ebx = 1: out[] of rax", 0, outval(&d, RAX));
    EQ("execute, the same add: out[] of flags, CF PF ZF", (uint64_t)(CF | PF | ZF),
       outval(&d, FLAGS_REG) & FLAGS5);

    /* page faults */
    d = dec(CODE, "\x8b\x03", 2);                       /* mov (%rbx),%eax */
    give(&d, RBX, HOLE + 0x18);
    execute(&d);
    memory(c, &d);
    EQ("memory, load from an unmapped page: fault", FAULT_PAGE, d.fault);
    EQ("memory, load from an unmapped page: fault_va", HOLE + 0x18, d.fault_va);

    d = dec(CODE, "\x89\x03", 2);                       /* mov %eax,(%rbx) */
    give(&d, RAX, 1);
    give(&d, RBX, HOLE + 0x20);
    execute(&d);
    memory(c, &d);
    EQ("memory, store to an unmapped page: fault", FAULT_PAGE, d.fault);
    EQ("memory, store to an unmapped page: fault_va", HOLE + 0x20, d.fault_va);

    /* an access that starts on a mapped page and ends on an unmapped one:
       fault_va is the first byte that could not be translated */
    d = dec(CODE, "\x48\x8b\x03", 3);                   /* mov (%rbx),%rax */
    give(&d, RBX, DATA + 2 * PAGE_SIZE - 4);
    execute(&d);
    memory(c, &d);
    EQ("memory, 8-byte load that crosses into an unmapped page: fault", FAULT_PAGE, d.fault);
    EQ("memory, 8-byte load that crosses into an unmapped page: fault_va is the start of "
       "that page, not ea", DATA + 2 * PAGE_SIZE, d.fault_va);
    d = dec(CODE, "\x48\x89\x03", 3);                   /* mov %rax,(%rbx) */
    give(&d, RAX, 1);
    give(&d, RBX, DATA + 2 * PAGE_SIZE - 4);
    execute(&d);
    memory(c, &d);
    EQ("memory, 8-byte store that crosses into an unmapped page: fault", FAULT_PAGE, d.fault);
    EQ("memory, 8-byte store that crosses into an unmapped page: fault_va is the start of "
       "that page, not sa", DATA + 2 * PAGE_SIZE, d.fault_va);

    /* an instruction that already has a fault passes through untouched */
    d = dec(CODE, "\x89\x03", 2);
    give(&d, RAX, 0x12345678);
    give(&d, RBX, DATA + 0x200);
    v = 0;
    poke(DATA + 0x200, &v, 8);
    d.fault = FAULT_ILL;
    d.ea = 0x1234;
    execute(&d);
    EQ("execute with d->fault set: ea is left alone", 0x1234, d.ea);
    d.sa = DATA + 0x200;
    d.sval.q[0] = 0x12345678;
    memory(c, &d);
    EQ("memory with d->fault set: nothing is stored", 0, peek(DATA + 0x200, 8));
    EQ("memory with d->fault set: the fault is kept", FAULT_ILL, d.fault);

    /* cpu_step: three instructions, the third one illegal */
    poke(CODE, "\xb8\x05\x00\x00\x00" "\x83\xc0\x03" "\x06", 9);
    memset(c, 0, sizeof *c);
    c->rflags = 0x2;
    c->rip = CODE;
    EQ("cpu_step, mov $5,%eax: return value", 0, cpu_step(c));
    EQ("cpu_step, mov $5,%eax: rax", 5, c->r[RAX]);
    EQ("cpu_step, mov $5,%eax: rip", CODE + 5, c->rip);
    EQ("cpu_step, mov $5,%eax: icount (cpu_commit counts the instruction)", 1, c->icount);
    EQ("cpu_step, add $3,%eax: return value", 0, cpu_step(c));
    EQ("cpu_step, add $3,%eax: rax", 8, c->r[RAX]);
    EQ("cpu_step, add $3,%eax: rip", CODE + 8, c->rip);
    EQ("cpu_step, illegal opcode 06: return value", (uint64_t)-1, (uint64_t)(int64_t)cpu_step(c));
    EQ("cpu_step, illegal opcode 06: c->fault (cpu_trap)", FAULT_ILL, c->fault);
    EQ("cpu_step, illegal opcode 06: rip stays at the instruction", CODE + 8, c->rip);
    EQ("cpu_step, illegal opcode 06: rax", 8, c->r[RAX]);
    EQ("cpu_step, illegal opcode 06: icount", 2, c->icount);

    /* cpu_step: a load that faults leaves the registers alone */
    poke(CODE, "\x8b\x03", 2);                          /* mov (%rbx),%eax */
    memset(c, 0, sizeof *c);
    c->rflags = 0x2;
    c->rip = CODE;
    c->r[RAX] = 0x55;
    c->r[RBX] = HOLE;
    EQ("cpu_step, mov (%rbx),%eax from an unmapped page: return value", (uint64_t)-1,
       (uint64_t)(int64_t)cpu_step(c));
    EQ("cpu_step, the faulting load: c->fault", FAULT_PAGE, c->fault);
    EQ("cpu_step, the faulting load: rax is not written (no writeback after a fault)", 0x55,
       c->r[RAX]);
    EQ("cpu_step, the faulting load: rip stays at the instruction", CODE, c->rip);

    /* system calls: getpid, then exit */
    poke(CODE, "\x0f\x05" "\x0f\x05", 4);
    memset(c, 0, sizeof *c);
    c->rflags = 0x2 | ZF;
    c->rip = CODE;
    c->r[RAX] = 39;
    EQ("cpu_step, syscall getpid: return value", 0, cpu_step(c));
    EQ("cpu_step, syscall getpid: rax", 1, c->r[RAX]);
    EQ("cpu_step, syscall getpid: rcx, the address of the next instruction", CODE + 2, c->r[RCX]);
    EQ("cpu_step, syscall getpid: r11, the flags", 0x2 | ZF, c->r[R11]);
    EQ("cpu_step, syscall getpid: rip", CODE + 2, c->rip);
    c->r[RAX] = 60;
    c->r[RDI] = 7;
    EQ("cpu_step, syscall exit: return value", (uint64_t)-1, (uint64_t)(int64_t)cpu_step(c));
    EQ("cpu_step, syscall exit: halted", 1, c->halted);
    EQ("cpu_step, syscall exit: fault", 0, c->fault);
    EQ("cpu_step, syscall exit: icount (the instruction commits)", 2, c->icount);
}

/* ------------------------------------------------------------- lab 2: pipe */

static void put_stage(int s, insn d, int valid)
{
    memset(&pipe_st[s], 0, sizeof pipe_st[s]);
    pipe_st[s].valid = valid;
    pipe_st[s].d = d;
}

static void t_pipe(void)
{
    mcpu *c = &cpu;
    const uint64_t pc = CODE + 0x100;
    insn add = dec(pc, "\x01\xd8", 2);                  /* add %ebx,%eax: writes rax, flags */
    insn load = dec(pc, "\x8b\x03", 2);                 /* mov (%rbx),%eax: writes rax, late */
    insn use = dec(pc, "\x89\xc1", 2);                  /* mov %eax,%ecx: reads rax */
    insn other = dec(pc, "\x89\xda", 2);                /* mov %ebx,%edx: writes rdx */
    insn d;

    machine();

    /* pipe_predict */
    d = dec(pc, "\x75\xfe", 2);                         /* jne to itself */
    EQ("pipe_predict, jne backward (target == pc)", pc, pipe_predict(&d));
    d = dec(pc, "\x75\xf0", 2);
    EQ("pipe_predict, jne backward", pc + 2 - 16, pipe_predict(&d));
    d = dec(pc, "\x75\x10", 2);
    EQ("pipe_predict, jne forward: not taken", pc + 2, pipe_predict(&d));
    d = dec(pc, "\x0f\x84\x00\x01\x00\x00", 6);
    EQ("pipe_predict, je rel32 forward: not taken", pc + 6, pipe_predict(&d));
    d = dec(pc, "\xeb\x10", 2);
    EQ("pipe_predict, jmp forward", pc + 2 + 16, pipe_predict(&d));
    d = dec(pc, "\xe8\x00\x02\x00\x00", 5);
    EQ("pipe_predict, call", pc + 5 + 0x200, pipe_predict(&d));
    d = dec(pc, "\xc3", 1);
    EQ("pipe_predict, ret: falls through", pc + 1, pipe_predict(&d));
    EQ("pipe_predict, add: falls through", pc + 2, pipe_predict(&add));
    d = dec(pc, "\x06", 1);                             /* illegal: fault set, len 0 */
    EQ("pipe_predict, an instruction decode rejected: pc + 1", pc + 1, pipe_predict(&d));

    /* pipe_ready */
    static const struct {
        const char *text;
        int mem, wb;                /* 0 empty, 1 add, 2 load, 3 other, 4 add but not valid */
        int forward, want;
    } R[] = {
        { "no writer in MEM or WB", 0, 0, 1, 1 },
        { "no writer in MEM or WB", 0, 0, 0, 1 },
        { "MEM holds add (writes rax)", 1, 0, 1, 1 },
        { "MEM holds add (writes rax)", 1, 0, 0, 0 },
        { "MEM holds a load into rax", 2, 0, 1, 0 },
        { "MEM holds a load into rax", 2, 0, 0, 0 },
        { "WB holds add (writes rax)", 0, 1, 1, 1 },
        { "WB holds add (writes rax)", 0, 1, 0, 0 },
        { "WB holds a load into rax", 0, 2, 1, 1 },
        { "WB holds a load into rax", 0, 2, 0, 0 },
        { "MEM holds add, WB a load into rax: the nearest writer decides", 1, 2, 1, 1 },
        { "MEM holds a load into rax, WB add: the nearest writer decides", 2, 1, 1, 0 },
        { "MEM holds an instruction that writes rdx only", 3, 0, 0, 1 },
        { "MEM holds an instruction that writes rdx only, WB add", 3, 1, 1, 1 },
        { "MEM holds an instruction that writes rdx only, WB add", 3, 1, 0, 0 },
        { "MEM is empty (valid 0) but its insn field still says add", 4, 0, 0, 1 },
        { "WB is empty (valid 0) but its insn field still says add", 0, 4, 0, 1 },
    };
    const insn *pick[5] = { &other, &add, &load, &other, &add };
    for (unsigned i = 0; i < sizeof R / sizeof R[0]; ++i) {
        memset(pipe_st, 0, sizeof pipe_st);
        put_stage(S_MEM, *pick[R[i].mem], R[i].mem >= 1 && R[i].mem <= 3);
        put_stage(S_WB, *pick[R[i].wb], R[i].wb >= 1 && R[i].wb <= 3);
        opt_forward = R[i].forward;
        int got = pipe_ready(&use);
        check(got == R[i].want, "pipe_ready, mov %%eax,%%ecx in ID, %s, %s forwarding: "
              "expected %d, got %d", R[i].text, R[i].forward ? "with" : "without",
              R[i].want, got);
    }
    opt_forward = 1;

    /* pipe_forward */
    memset(c, 0, sizeof *c);
    c->r[RAX] = 111;
    c->r[RBX] = 222;
    d = dec(pc, "\xb8\x07\x00\x00\x00", 5);             /* mov $7,%eax, executed */
    execute(&d);
    memset(pipe_st, 0, sizeof pipe_st);
    put_stage(S_MEM, d, 1);
    d = add;
    pipe_forward(c, &d);
    EQ("pipe_forward, add %ebx,%eax behind mov $7,%eax in MEM: in[] of rax", 7,
       d.in[insn_src(&d, RAX)].q[0]);
    EQ("pipe_forward, the same: in[] of rbx, from the register file", 222,
       d.in[insn_src(&d, RBX)].q[0]);
    pipe_st[S_MEM].valid = 0;
    d = add;
    pipe_forward(c, &d);
    EQ("pipe_forward, MEM empty: in[] of rax, from the register file", 111,
       d.in[insn_src(&d, RAX)].q[0]);

    /* pipe_resolve */
    for (int s = S_EX; s <= S_MEM; ++s)
        for (int kind = 0; kind < 3; ++kind) {          /* mispredicted, predicted, fault */
            memset(pipe_st, 0, sizeof pipe_st);
            for (int k = 0; k < NSTAGE; ++k)
                put_stage(k, other, 1);
            d = dec(pc, "\x75\x10", 2);
            d.next = pc + 0x12;
            pipe_st[s].d = d;
            pipe_st[s].pred = kind == 1 ? pc + 0x12 : pc + 2;
            if (kind == 2)
                pipe_st[s].d.fault = FAULT_PAGE;
            pipe_pc = 0x1111;
            pipe_resolve(s);
            const char *what = kind == 0 ? "a mispredicted branch" :
                               kind == 1 ? "a correctly predicted branch" :
                                           "an instruction with a fault";
            const char *where = s == S_EX ? "EX" : "MEM";
            for (int k = 0; k < NSTAGE; ++k) {
                int want = !(kind == 0 && k < s);
                check(pipe_st[k].valid == want, "pipe_resolve(%s), %s: valid of stage %d "
                      "expected %d, got %d", where, what, k, want, pipe_st[k].valid);
            }
            check(pipe_pc == (kind == 0 ? pc + 0x12 : 0x1111),
                  "pipe_resolve(%s), %s: pipe_pc expected %#llx, got %#llx", where, what,
                  (unsigned long long)(kind == 0 ? pc + 0x12 : 0x1111),
                  (unsigned long long)pipe_pc);
        }

    /* pipe_wb: an instruction with a fault is trapped and not written back */
    memset(pipe_st, 0, sizeof pipe_st);
    d = load;
    give(&d, RBX, HOLE);
    execute(&d);
    memory(c, &d);                                      /* the page fault */
    if (insn_dst(&d, RAX) >= 0)
        d.out[insn_dst(&d, RAX)].q[0] = 0x77;
    put_stage(S_WB, d, 1);
    c->r[RAX] = 0x55;
    c->rip = 0x1234;
    c->fault = 0;
    EQ("pipe_wb with a faulted load in WB: return value", 1, pipe_wb(c, 20));
    EQ("pipe_wb with a faulted load in WB: c->fault (cpu_trap)", FAULT_PAGE, c->fault);
    EQ("pipe_wb with a faulted load in WB: rax is not written", 0x55, c->r[RAX]);
    EQ("pipe_wb with a faulted load in WB: rip is the pc of the instruction", pc, c->rip);
    c->fault = 0;
    d = add;
    give(&d, RAX, 1);
    give(&d, RBX, 2);
    execute(&d);
    put_stage(S_WB, d, 1);
    uint64_t ic = c->icount;
    EQ("pipe_wb with add %ebx,%eax in WB: return value (the run goes on)", 0, pipe_wb(c, 21));
    EQ("pipe_wb with add %ebx,%eax in WB: rax", 3, c->r[RAX]);
    EQ("pipe_wb with add %ebx,%eax in WB: the stage is emptied", 0, pipe_st[S_WB].valid);
    EQ("pipe_wb with add %ebx,%eax in WB: icount grows by (cpu_commit)", 1, c->icount - ic);

    /* pipe_move: MEM needs three cycles, so everything behind it waits */
    memset(pipe_st, 0, sizeof pipe_st);
    d = other; d.t[S_MEM] = 5; d.mcyc = 3;
    put_stage(S_MEM, d, 1);
    d = other; d.t[S_EX] = 6;
    put_stage(S_EX, d, 1);
    d = other; d.t[S_ID] = 6;
    put_stage(S_ID, d, 1);
    d = other; d.t[S_IF] = 6;
    put_stage(S_IF, d, 1);
    pipe_st[S_IF].pred = 0x77;
    pipe_move(7);
    check(pipe_st[S_MEM].valid && !pipe_st[S_WB].valid && pipe_st[S_EX].valid &&
          pipe_st[S_ID].valid && pipe_st[S_IF].valid && pipe_st[S_IF].d.t[S_IF] == 6,
          "pipe_move(7) with an instruction in MEM since cycle 5 for 3 cycles: nothing may "
          "move; valid of IF ID EX MEM WB is %d %d %d %d %d",
          pipe_st[S_IF].valid, pipe_st[S_ID].valid, pipe_st[S_EX].valid,
          pipe_st[S_MEM].valid, pipe_st[S_WB].valid);
    pipe_move(8);
    check(pipe_st[S_WB].valid && pipe_st[S_WB].d.t[S_WB] == 8,
          "pipe_move(8): the instruction of MEM enters WB with t[S_WB] = 8; valid %d, t %llu",
          pipe_st[S_WB].valid, (unsigned long long)pipe_st[S_WB].d.t[S_WB]);
    check(pipe_st[S_MEM].valid && pipe_st[S_MEM].d.t[S_MEM] == 8,
          "pipe_move(8): the instruction of EX enters MEM with t[S_MEM] = 8; valid %d, t %llu",
          pipe_st[S_MEM].valid, (unsigned long long)pipe_st[S_MEM].d.t[S_MEM]);
    check(pipe_st[S_EX].valid && pipe_st[S_EX].d.t[S_EX] == 8,
          "pipe_move(8): the instruction of ID enters EX with t[S_EX] = 8; valid %d, t %llu",
          pipe_st[S_EX].valid, (unsigned long long)pipe_st[S_EX].d.t[S_EX]);
    check(pipe_st[S_ID].valid && pipe_st[S_ID].d.t[S_ID] == 8 && pipe_st[S_ID].pred == 0x77,
          "pipe_move(8): the whole stage IF (insn and pred) enters ID with t[S_ID] = 8; "
          "valid %d, t %llu, pred %#llx", pipe_st[S_ID].valid,
          (unsigned long long)pipe_st[S_ID].d.t[S_ID], (unsigned long long)pipe_st[S_ID].pred);
    check(pipe_st[S_IF].valid && pipe_st[S_IF].d.t[S_IF] == 8,
          "pipe_move(8): the empty IF becomes valid with t[S_IF] = 8; valid %d, t %llu",
          pipe_st[S_IF].valid, (unsigned long long)pipe_st[S_IF].d.t[S_IF]);

    /* an instruction in ID waits for its operand; IF waits behind it */
    memset(pipe_st, 0, sizeof pipe_st);
    d = load; d.t[S_MEM] = 9; d.mcyc = 1;
    put_stage(S_MEM, d, 1);                             /* moves to WB */
    d = load; d.t[S_EX] = 9;
    put_stage(S_EX, d, 1);                              /* moves to MEM: a late writer of rax */
    d = use; d.t[S_ID] = 9;
    put_stage(S_ID, d, 1);
    d = other; d.t[S_IF] = 9;
    put_stage(S_IF, d, 1);
    pipe_move(10);
    check(pipe_st[S_ID].valid && !pipe_st[S_EX].valid && pipe_st[S_ID].d.t[S_ID] == 9,
          "pipe_move(10), mov %%eax,%%ecx in ID behind a load into rax that enters MEM: "
          "it stays in ID and EX is empty; valid of ID %d, EX %d",
          pipe_st[S_ID].valid, pipe_st[S_EX].valid);
    check(pipe_st[S_IF].valid && pipe_st[S_IF].d.t[S_IF] == 9,
          "pipe_move(10): the instruction in IF stays behind the one waiting in ID; "
          "t[S_IF] is %llu", (unsigned long long)pipe_st[S_IF].d.t[S_IF]);

    /* pipe_mem: memory() once, in the first cycle of the instruction */
    uint64_t v = 0x1122334455667788ULL;
    poke(DATA + 0x700, &v, 8);
    memset(pipe_st, 0, sizeof pipe_st);
    d = load;
    give(&d, RBX, DATA + 0x700);
    execute(&d);
    d.t[S_MEM] = 5;
    put_stage(S_MEM, d, 1);
    pipe_mem(c, 5);
    EQ("pipe_mem in the first cycle of a load (now == t[S_MEM]): lval", 0x55667788,
       pipe_st[S_MEM].d.lval.q[0]);
    check(pipe_st[S_MEM].d.mcyc >= 1, "pipe_mem in the first cycle: mcyc is set, got %u",
          pipe_st[S_MEM].d.mcyc);
    v = 0;
    poke(DATA + 0x700, &v, 8);
    pipe_st[S_MEM].d.mcyc = 3;
    pipe_mem(c, 6);
    EQ("pipe_mem in a later cycle of the same instruction: memory() is not called again, lval",
       0x55667788, pipe_st[S_MEM].d.lval.q[0]);
#if LAB >= 5
    /* a cache miss during memory() costs opt_miss cycles in MEM */
    opt_miss = 7;
    cache_flush();
    cache_init(64, 4);                                  /* every line invalid */
    put_stage(S_MEM, d, 1);
    pipe_mem(c, 5);
    EQ("pipe_mem, a load whose line is not in the cache, opt_miss 7: mcyc", 7,
       pipe_st[S_MEM].d.mcyc);
    put_stage(S_MEM, d, 1);
    pipe_mem(c, 5);
    EQ("pipe_mem, the same load again (TLB and cache hit): mcyc", 1, pipe_st[S_MEM].d.mcyc);

    /* a late branch (ret) whose load misses, opt_miss 3: resolved in the last cycle */
    v = CODE + 0x300;
    poke(DATA + 0x708, &v, 8);
    memset(pipe_st, 0, sizeof pipe_st);
    for (int k = S_IF; k < S_MEM; ++k)
        put_stage(k, other, 1);
    d = dec(pc, "\xc3", 1);                             /* ret */
    give(&d, RSP, DATA + 0x708);
    execute(&d);
    d.t[S_MEM] = 5;
    put_stage(S_MEM, d, 1);
    pipe_st[S_MEM].pred = pc + 1;
    pipe_pc = 0x2222;
    opt_miss = 3;
    cache_flush();
    cache_init(64, 4);                                  /* the load misses: mcyc 3 */
    pipe_mem(c, 5);
    EQ("pipe_mem, ret in MEM with mcyc 3, first cycle: the target is known but not "
       "resolved yet, pipe_pc", 0x2222, pipe_pc);
    pipe_mem(c, 6);
    check(pipe_pc == 0x2222 && pipe_st[S_EX].valid && pipe_st[S_ID].valid && pipe_st[S_IF].valid,
          "pipe_mem, ret in MEM with mcyc 3, second cycle: nothing resolved; pipe_pc %#llx, "
          "valid of IF ID EX %d %d %d", (unsigned long long)pipe_pc, pipe_st[S_IF].valid,
          pipe_st[S_ID].valid, pipe_st[S_EX].valid);
    pipe_mem(c, 7);
    check(pipe_pc == CODE + 0x300 && !pipe_st[S_EX].valid && !pipe_st[S_ID].valid &&
          !pipe_st[S_IF].valid, "pipe_mem, ret in MEM with mcyc 3, last cycle (now + 1 == "
          "t[S_MEM] + mcyc): resolved, pipe_pc %#llx (expected %#llx), valid of IF ID EX "
          "%d %d %d", (unsigned long long)pipe_pc, (unsigned long long)(CODE + 0x300),
          pipe_st[S_IF].valid, pipe_st[S_ID].valid, pipe_st[S_EX].valid);

    opt_miss = 10;
#endif
}

#endif /* LAB >= 2 */

/* -------------------------------------------------------------- lab 4: mmu */

#if LAB >= 4

/* The entry of the last level for va, 0 if a table on the way is absent. */
static uint64_t leaf(uint64_t va, uint64_t *table)
{
    uint64_t t = mmu_cr3();
    for (int lvl = 3; lvl >= 1; --lvl) {
        uint64_t e = pte_read(t, PT_INDEX(va, lvl));
        if (!(e & PTE_P))
            return 0;
        check((e & (PTE_P | PTE_W | PTE_U)) == (PTE_P | PTE_W | PTE_U),
              "mmu_map: the entry of level %d for %#llx is %#llx; a table entry has "
              "PTE_P | PTE_W | PTE_U", lvl, (unsigned long long)va, (unsigned long long)e);
        t = PTE_ADDR(e);
    }
    if (table)
        *table = t;
    return pte_read(t, PT_INDEX(va, 0));
}

static void t_mmu(void)
{
    const uint64_t va = 0x00007f1234567000ULL, ro = 0x00007f1234569000ULL;
    uint64_t pa = 0, f1, f2, f3, f4, before, after, e, t = 0, h, m, pf;

    machine();
    mmu_tlb_flush();
    tlb_hits = tlb_misses = page_faults = 0;

    EQ("mmu_translate of an unmapped page: return value", (uint64_t)-1,
       (uint64_t)(int64_t)mmu_translate(va + 0x10, MEM_READ, &pa));
    EQ("mmu_translate of an unmapped page: page_faults", 1, page_faults);

    /* a page far from every other mapping needs three new tables */
    f1 = mmu_alloc_frame();
    before = mmu_alloc_frame();
    EQ("mmu_map(va, f1, P | W | U): return value", 0,
       mmu_map(va, f1, PTE_P | PTE_W | PTE_U));
    after = mmu_alloc_frame();
    EQ("mmu_map of a page in an empty part of the address space: frames taken for tables "
       "(PDPT, PD, PT)", 3, (after - before) / PAGE_SIZE - 1);
    e = leaf(va, &t);
    EQ("mmu_map(va, f1, P | W | U): the entry of level 0", f1 | PTE_P | PTE_W | PTE_U, e);

    f2 = mmu_alloc_frame();
    before = mmu_alloc_frame();
    mmu_map(va + PAGE_SIZE, f2, PTE_P | PTE_W | PTE_U);
    after = mmu_alloc_frame();
    EQ("mmu_map of the next page: frames taken for tables (the tables exist)", 0,
       (after - before) / PAGE_SIZE - 1);
    mmu_tlb_flush();

    /* translation, TLB counters, accessed and dirty bits */
    h = tlb_hits; m = tlb_misses;
    EQ("mmu_translate(va + 0x123, MEM_READ): return value", 0,
       mmu_translate(va + 0x123, MEM_READ, &pa));
    EQ("mmu_translate(va + 0x123, MEM_READ): pa", f1 + 0x123, pa);
    EQ("first translation after a flush: tlb_misses grows by", 1, tlb_misses - m);
    EQ("first translation after a flush: tlb_hits grows by", 0, tlb_hits - h);
    e = leaf(va, NULL);
    EQ("after a read: PTE_A set and PTE_D clear in the entry of level 0",
       f1 | PTE_P | PTE_W | PTE_U | PTE_A, e);
    pa = 0;
    EQ("second translation of the page: return value", 0,
       mmu_translate(va + 0xfff, MEM_FETCH, &pa));
    EQ("second translation of the page: pa", f1 + 0xfff, pa);
    EQ("second translation of the page: tlb_hits grows by", 1, tlb_hits - h);
    EQ("second translation of the page: tlb_misses grows by", 1, tlb_misses - m);

    mmu_tlb_flush();
    EQ("mmu_translate(va, MEM_WRITE) after a flush: return value", 0,
       mmu_translate(va, MEM_WRITE, &pa));
    e = leaf(va, NULL);
    EQ("after a write that walked the tables: PTE_A and PTE_D set",
       f1 | PTE_P | PTE_W | PTE_U | PTE_A | PTE_D, e);
    EQ("the other page of the table is untouched", f2 | PTE_P | PTE_W | PTE_U,
       leaf(va + PAGE_SIZE, NULL));

    /* a fetch that walks the tables sets PTE_A, not PTE_D */
    mmu_tlb_flush();
    EQ("mmu_translate(va + PAGE_SIZE + 0x40, MEM_FETCH) after a flush: return value", 0,
       mmu_translate(va + PAGE_SIZE + 0x40, MEM_FETCH, &pa));
    EQ("after a fetch that walked the tables: PTE_A set, PTE_D clear",
       f2 | PTE_P | PTE_W | PTE_U | PTE_A, leaf(va + PAGE_SIZE, NULL));

    /* page 0 is not mapped: an empty TLB slot must not answer for it */
    mmu_tlb_flush();
    pf = page_faults;
    EQ("mmu_translate(0x10, MEM_READ) right after a flush (page 0 is not mapped): "
       "return value", (uint64_t)-1, (uint64_t)(int64_t)mmu_translate(0x10, MEM_READ, &pa));
    EQ("mmu_translate(0x10, MEM_READ): page_faults grows by", 1, page_faults - pf);

    /* a read-only page */
    f3 = mmu_alloc_frame();
    mmu_map(ro, f3, PTE_P | PTE_U);
    mmu_tlb_flush();
    pf = page_faults;
    EQ("read-only page, MEM_WRITE right after a flush: return value", (uint64_t)-1,
       (uint64_t)(int64_t)mmu_translate(ro, MEM_WRITE, &pa));
    EQ("read-only page, MEM_WRITE: page_faults grows by", 1, page_faults - pf);
    EQ("read-only page, MEM_READ: return value", 0, mmu_translate(ro + 8, MEM_READ, &pa));
    EQ("read-only page, MEM_READ: pa", f3 + 8, pa);
    EQ("read-only page, MEM_WRITE when the page is in the TLB: return value", (uint64_t)-1,
       (uint64_t)(int64_t)mmu_translate(ro, MEM_WRITE, &pa));
    EQ("read-only page, MEM_WRITE when the page is in the TLB: page_faults grows by", 2,
       page_faults - pf);

    /* mmu_map replaces an entry */
    f4 = mmu_alloc_frame();
    EQ("mmu_map of a mapped page: return value", 0, mmu_map(ro, f4, PTE_P | PTE_W | PTE_U));
    mmu_tlb_flush();
    EQ("after mapping the page again and a flush: MEM_WRITE", 0,
       mmu_translate(ro + 4, MEM_WRITE, &pa));
    EQ("after mapping the page again and a flush: pa", f4 + 4, pa);

    /* back to f3 without PTE_W: the old entry is replaced, not merged with */
    EQ("mmu_map(ro, f3, P | U) over the entry for f4: return value", 0,
       mmu_map(ro, f3, PTE_P | PTE_U));
    mmu_tlb_flush();
    EQ("after mapping ro back to f3: MEM_READ return value", 0, mmu_translate(ro + 8, MEM_READ, &pa));
    EQ("after mapping ro back to f3: pa is in f3 (the entry is replaced, not ORed)", f3 + 8, pa);
    EQ("after mapping ro back to f3 without PTE_W: MEM_WRITE is a page fault again",
       (uint64_t)-1, (uint64_t)(int64_t)mmu_translate(ro, MEM_WRITE, &pa));
    uint64_t f5 = mmu_alloc_frame();
    mmu_map(va + 2 * PAGE_SIZE, f5 + 0x123, PTE_P | PTE_U);
    EQ("mmu_map with offset bits in pa: the entry of level 0 is (pa & ~0xfff) | flags | PTE_P",
       f5 | PTE_P | PTE_U, leaf(va + 2 * PAGE_SIZE, NULL));

    /* the TLB holds a translation until it is flushed: change the entry
       behind its back */
    mmu_tlb_flush();
    mmu_translate(va, MEM_READ, &pa);
    e = leaf(va, &t);
    pte_write(t, PT_INDEX(va, 0), f2 | (e & 0xfff));
    h = tlb_hits; m = tlb_misses;
    pa = 0;
    mmu_translate(va + 1, MEM_READ, &pa);
    EQ("a page table entry changed without a flush: the TLB still answers with the old frame",
       f1 + 1, pa);
    EQ("the same translation: a TLB hit, tlb_hits grows by", 1, tlb_hits - h);
    mmu_tlb_flush();
    mmu_translate(va + 1, MEM_READ, &pa);
    EQ("after mmu_tlb_flush the walk finds the new frame", f2 + 1, pa);
    EQ("after mmu_tlb_flush: tlb_misses grows by", 1, tlb_misses - m);
}

#endif /* LAB >= 4 */

/* ------------------------------------------------------------ lab 5: cache */

#if LAB >= 5

#define MEMBASE 0x00800000ULL       /* physical memory no frame of the tests uses */

static uint8_t pattern(uint64_t pa)
{
    return (uint8_t)(pa * 7 + (pa >> 8) * 3 + 3);
}

static void fill_mem(void)
{
    uint8_t *p = dram_ptr(MEMBASE, 0x2000);
    for (unsigned i = 0; i < 0x2000; ++i)
        p[i] = pattern(MEMBASE + i);
}

static void counts(const char *what, uint64_t hits, uint64_t misses, uint64_t wb)
{
    check(cache_hits == hits && cache_misses == misses && cache_writebacks == wb,
          "%s: expected hits %llu misses %llu writebacks %llu, got %llu %llu %llu", what,
          (unsigned long long)hits, (unsigned long long)misses, (unsigned long long)wb,
          (unsigned long long)cache_hits, (unsigned long long)cache_misses,
          (unsigned long long)cache_writebacks);
}

static void t_cache(void)
{
    const uint64_t P = MEMBASE, A = MEMBASE, B = MEMBASE + 64, C = MEMBASE + 128;
    uint8_t buf[256], *mem;
    int ok;

    if (dram_open(ram_path, GUEST_RAM) < 0)
        exit(2);
    mem = dram_ptr(MEMBASE, 0x2000);

    /* an access across two lines; write-back */
    fill_mem();
    cache_init(4, 2);
    dram_reads = dram_writes = 0;
    counts("after cache_init", 0, 0, 0);
    cache_read(P + 60, buf, 8);
    ok = 1;
    for (unsigned i = 0; i < 8; ++i)
        ok &= buf[i] == pattern(P + 60 + i);
    check(ok, "cache_read of 8 bytes at line offset 60: the bytes differ from memory");
    counts("cache_read of 8 bytes at line offset 60 (two lines)", 0, 2, 0);
    EQ("the same: dram_line_read calls", 2, dram_reads);
    cache_read(P + 60, buf, 8);
    counts("the same read again", 2, 2, 0);
    cache_write(P + 62, "\x11\x22\x33\x44", 4);
    counts("cache_write of 4 bytes at line offset 62 (both lines cached)", 4, 2, 0);
    check(mem[62] == pattern(P + 62) && mem[65] == pattern(P + 65),
          "cache_write: memory changed before any write-back (the cache is write-back)");
    cache_read(P + 56, buf, 16);
    check(!memcmp(buf + 6, "\x11\x22\x33\x44", 4) && buf[5] == pattern(P + 61) &&
          buf[10] == pattern(P + 66),
          "cache_read after cache_write: expected the 4 bytes written between the old ones");
    cache_flush();
    counts("cache_flush with two dirty lines", 6, 2, 2);
    EQ("cache_flush: dram_line_write calls", 2, dram_writes);
    check(!memcmp(mem + 62, "\x11\x22\x33\x44", 4) && mem[61] == pattern(P + 61) &&
          mem[66] == pattern(P + 66), "cache_flush: memory does not hold the bytes written");
    cache_flush();
    counts("a second cache_flush (the lines are clean)", 6, 2, 2);
    cache_read(P + 62, buf, 4);
    counts("cache_read after cache_flush (the lines stay valid)", 8, 2, 2);

    /* cache_init again: counters and lines start over */
    fill_mem();
    cache_init(1, 2);
    counts("a second cache_init", 0, 0, 0);

    /* least recently used, one set of two ways */
    cache_read(A, buf, 1);
    cache_read(B, buf, 1);
    counts("1 set, 2 ways: read A, B", 0, 2, 0);
    cache_read(A, buf, 1);
    cache_read(C, buf, 1);
    counts("then A, C (C replaces B, the least recently used)", 1, 3, 0);
    cache_read(A, buf, 1);
    counts("then A (still cached)", 2, 3, 0);
    cache_read(B, buf, 1);
    counts("then B (was evicted)", 2, 4, 0);
    cache_read(C, buf, 1);
    counts("then C (evicted by B: A was used later)", 2, 5, 0);

    /* a dirty line is written back to its own address when it is evicted */
    fill_mem();
    cache_init(4, 1);
    dram_reads = dram_writes = 0;
    cache_write(P + 5 * 64 + 3, "\xa5", 1);
    counts("4 sets, 1 way: cache_write of one byte to an uncached line (write allocation)",
           0, 1, 0);
    EQ("write allocation: the line is read from memory first, dram_line_read calls", 1,
       dram_reads);
    cache_read(P + (5 + 4) * 64, buf, 1);               /* same set, another tag */
    counts("then a read of the line 4 lines further (same set): the dirty line goes back",
           0, 2, 1);
    ok = mem[5 * 64 + 3] == 0xa5;
    for (unsigned i = 0; i < 64; ++i)
        if (i != 3)
            ok &= mem[5 * 64 + i] == pattern(P + 5 * 64 + i);
    check(ok, "eviction of a dirty line: memory at the address of the line must hold the "
              "byte written and the 63 old bytes around it");
    for (unsigned i = 0; i < 0x2000; ++i)
        if (i / 64 != 5 && mem[i] != pattern(P + i)) {
            check(0, "eviction of a dirty line wrote to physical address %#llx, outside "
                     "the line", (unsigned long long)(P + i));
            break;
        }
    cache_read(P + 5 * 64, buf, 8);
    check(buf[3] == 0xa5 && buf[2] == pattern(P + 5 * 64 + 2),
          "the evicted line read again: expected the byte written before the eviction");
    counts("the evicted line read again", 0, 3, 1);
    cache_flush();
    counts("cache_flush with only clean lines", 0, 3, 1);

    /* a long access; sets = ways = 0 means one line */
    fill_mem();
    doing = "cache_init(0, 0) and a cache_read (0 sets or ways count as 1)";
    cache_init(0, 0);
    cache_read(P + 10, buf, 200);
    doing = NULL;
    ok = 1;
    for (unsigned i = 0; i < 200; ++i)
        ok &= buf[i] == pattern(P + 10 + i);
    check(ok, "cache_read of 200 bytes (four lines) with a cache of one line: wrong bytes");
    counts("cache_init(0, 0), cache_read of 200 bytes at offset 10 (four lines)", 0, 4, 0);
    dram_reads = 0;
    cache_write(P + 0x100, buf, 200);
    EQ("cache_write of 200 bytes at offset 0x100 (three whole lines and part of a fourth): "
       "write allocation reads every line first, dram_line_read calls", 4, dram_reads);
    cache_flush();
    ok = 1;
    for (unsigned i = 0; i < 200; ++i)
        ok &= mem[0x100 + i] == pattern(P + 10 + i);
    check(ok && mem[0x100 + 200] == pattern(P + 0x100 + 200),
          "cache_write of 200 bytes with a cache of one line, then cache_flush: memory "
          "does not hold the bytes");
    counts("the same write and flush", 0, 8, 4);
}

#endif /* LAB >= 5 */

/* ------------------------------------------------- lab 6: blk, ramfs, sys */

#if LAB >= 6

#define RING_PA  0x800
#define BUF_PA   0x00900000ULL

static uint8_t disk_byte(uint64_t off)
{
    return (uint8_t)(off * 13 + (off >> 9) * 31 + 5);
}

static uint8_t file_byte(uint64_t i)
{
    return (uint8_t)(i * 31 + (i >> 8) * 17 + 7);
}

static int write_file(const uint8_t *p, size_t n)
{
    int fd = open(disk_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || write(fd, p, n) != (ssize_t)n) {
        perror(disk_path);
        exit(2);
    }
    return fd;
}

static blk_req req(uint32_t type, uint64_t sector, uint64_t addr, uint32_t len)
{
    blk_req q = { type, 0, sector, addr, len, 0 };
    return q;
}

static void t_blk(void)
{
    static uint8_t img[3 * BLK_SECTOR];
    uint8_t got[BLK_SECTOR], *buf;
    int fd, ok;

    if (dram_open(ram_path, GUEST_RAM) < 0)
        exit(2);
    for (unsigned i = 0; i < sizeof img; ++i)
        img[i] = disk_byte(i);
    fd = write_file(img, sizeof img);
    blk_ring *r = (blk_ring *)dram_ptr(RING_PA, sizeof *r);
    buf = dram_ptr(BUF_PA, 0x4000);
    memset(r, 0, sizeof *r);
    r->ndesc = BLK_RING;
    blk_reads = blk_writes = 0;

    /* an empty ring */
    r->doorbell = 1;
    blk_kick(r, fd);
    check(r->doorbell == 0 && r->head == 0 && blk_reads == 0,
          "blk_kick with head == tail: only the doorbell is cleared; doorbell %u head %u reads %llu",
          r->doorbell, r->head, (unsigned long long)blk_reads);

    /* one read */
    memset(buf, 0xff, 0x4000);
    r->desc[0] = req(BLK_IN, 1, BUF_PA, BLK_SECTOR);
    r->tail = 1;
    r->doorbell = 1;
    blk_kick(r, fd);
    EQ("BLK_IN of sector 1: status", 1, r->desc[0].status);
    EQ("BLK_IN of sector 1: head", 1, r->head);
    EQ("BLK_IN of sector 1: doorbell", 0, r->doorbell);
    EQ("BLK_IN of sector 1: blk_reads", 1, blk_reads);
    check(!memcmp(buf, img + BLK_SECTOR, BLK_SECTOR) && buf[BLK_SECTOR] == 0xff,
          "BLK_IN of sector 1: the buffer must hold the 512 bytes at offset 512 of the "
          "image and nothing beyond");

    /* three requests for one doorbell: a write, then reads */
    for (unsigned i = 0; i < BLK_SECTOR; ++i)
        buf[0x1000 + i] = (uint8_t)(0xa5 ^ i);
    r->desc[1] = req(BLK_OUT, 2, BUF_PA + 0x1000, BLK_SECTOR);
    r->desc[2] = req(BLK_IN, 2, BUF_PA + 0x2000, BLK_SECTOR);
    r->desc[3] = req(BLK_IN, 0, BUF_PA + 0x3000, BLK_SECTOR);
    r->tail = 4;
    r->doorbell = 1;
    blk_kick(r, fd);
    check(r->head == 4 && r->doorbell == 0 && r->desc[1].status == 1 &&
          r->desc[2].status == 1 && r->desc[3].status == 1,
          "three requests pending: every one served; head %u (expected 4), status %u %u %u",
          r->head, r->desc[1].status, r->desc[2].status, r->desc[3].status);
    EQ("three requests pending: blk_writes", 1, blk_writes);
    EQ("three requests pending: blk_reads", 3, blk_reads);
    ok = pread(fd, got, BLK_SECTOR, 2 * BLK_SECTOR) == BLK_SECTOR &&
         !memcmp(got, buf + 0x1000, BLK_SECTOR);
    check(ok, "BLK_OUT of sector 2: the image must hold the buffer at offset 1024");
    ok = pread(fd, got, BLK_SECTOR, BLK_SECTOR) == BLK_SECTOR &&
         !memcmp(got, img + BLK_SECTOR, BLK_SECTOR);
    check(ok, "BLK_OUT of sector 2: sector 1 of the image must be unchanged");
    check(!memcmp(buf + 0x2000, buf + 0x1000, BLK_SECTOR),
          "BLK_IN of sector 2 after the BLK_OUT: requests are served in ring order, so "
          "the read sees the new contents");
    check(!memcmp(buf + 0x3000, img, BLK_SECTOR), "BLK_IN of sector 0: wrong bytes");

    /* buffers outside guest memory */
    r->desc[4] = req(BLK_IN, 0, GUEST_RAM - 256, BLK_SECTOR);
    r->desc[5] = req(BLK_OUT, 0, GUEST_RAM, BLK_SECTOR);
    r->desc[6] = req(BLK_IN, 0, 0xffffffffffffff00ULL, BLK_SECTOR);
    r->desc[7] = req(BLK_IN, 1, BUF_PA, BLK_SECTOR);
    r->tail = 8;
    r->doorbell = 1;
    memset(buf, 0, BLK_SECTOR);
    blk_kick(r, fd);
    check(r->desc[4].status == 2 && r->desc[5].status == 2 && r->desc[6].status == 2,
          "a buffer that is not inside guest memory: status 2; got %u %u %u "
          "(addr GUEST_RAM - 256, GUEST_RAM, 0xffffffffffffff00)",
          r->desc[4].status, r->desc[5].status, r->desc[6].status);
    check(r->head == 8 && r->desc[7].status == 1 && !memcmp(buf, img + BLK_SECTOR, BLK_SECTOR),
          "the request after three failed ones is still served; head %u, status %u",
          r->head, r->desc[7].status);
    EQ("failed requests are not counted: blk_reads", 4, blk_reads);
    EQ("failed requests are not counted: blk_writes", 1, blk_writes);
    ok = pread(fd, got, BLK_SECTOR, 0) == BLK_SECTOR && !memcmp(got, img, BLK_SECTOR);
    check(ok, "BLK_OUT with a buffer outside guest memory: the image must be unchanged");

    /* a write the host refuses: the image opened read-only */
    {
        int ro = open(disk_path, O_RDONLY);
        uint64_t w0 = blk_writes;
        r->desc[8] = req(BLK_OUT, 1, BUF_PA + 0x1000, BLK_SECTOR);
        r->tail = 9;
        r->doorbell = 1;
        blk_kick(r, ro);
        close(ro);
        check(r->desc[8].status == 2 && r->head == 9,
              "BLK_OUT when pwrite fails (image opened read-only): status 2 and head advanced; "
              "status %u, head %u", r->desc[8].status, r->head);
        EQ("BLK_OUT when pwrite fails: blk_writes unchanged", w0, blk_writes);
    }

    /* reading past the end of the image; the ring wraps */
    r->head = r->tail = 14;
    memset(buf, 0xff, 0x4000);
    r->desc[14] = req(BLK_IN, 2, BUF_PA, 2 * BLK_SECTOR);
    r->desc[15] = req(BLK_IN, 100, BUF_PA + 0x1000, BLK_SECTOR);
    r->desc[0] = req(BLK_IN, 0, BUF_PA + 0x2000, BLK_SECTOR);
    r->desc[1] = req(BLK_IN, 1, BUF_PA + 0x3000, BLK_SECTOR);
    r->tail = 18;
    r->doorbell = 1;
    blk_kick(r, fd);
    check(r->head == 18 && r->desc[14].status && r->desc[15].status &&
          r->desc[0].status == 1 && r->desc[1].status == 1,
          "head 14, tail 18: desc[14], desc[15], desc[0], desc[1] are served "
          "(index head %% BLK_RING); head %u, status %u %u %u %u", r->head,
          r->desc[14].status, r->desc[15].status, r->desc[0].status, r->desc[1].status);
    check(r->desc[14].status == 1 && r->desc[15].status == 1,
          "BLK_IN that ends or starts beyond the end of the image is not an error (the "
          "missing bytes are zeros): status 1 expected, got %u and %u",
          r->desc[14].status, r->desc[15].status);
    ok = buf[2 * BLK_SECTOR] == 0xff;
    for (unsigned i = 0; i < BLK_SECTOR; ++i)
        ok &= buf[i] == (uint8_t)(0xa5 ^ i) && buf[BLK_SECTOR + i] == 0;
    check(ok, "BLK_IN of 1024 bytes at the last sector of the image: 512 bytes of the "
              "sector, then zeros");
    ok = 1;
    for (unsigned i = 0; i < BLK_SECTOR; ++i)
        ok &= buf[0x1000 + i] == 0;
    check(ok, "BLK_IN of a sector past the end of the image: 512 zero bytes, status 1");
    check(!memcmp(buf + 0x2000, img, BLK_SECTOR) &&
          !memcmp(buf + 0x3000, img + BLK_SECTOR, BLK_SECTOR),
          "desc[0] and desc[1] after the wrap: wrong bytes");
    close(fd);
}

/* An image as tools/mkfs.py writes it: a.txt (10 bytes), b.bin (1300
   bytes), empty (0 bytes). bad != 0 spoils the magic. */
static void make_image(int bad)
{
    static uint8_t img[5 * BLK_SECTOR];
    static const struct { const char *name; uint32_t start, size; } F[3] = {
        { "a.txt", 1, 10 }, { "b.bin", 2, 1300 }, { "empty", 5, 0 } };
    uint32_t n = 3;

    memset(img, 0, sizeof img);
    memcpy(img, bad ? "MCPUFS00" : RAMFS_MAGIC, 8);
    memcpy(img + 8, &n, 4);
    for (unsigned i = 0; i < 3; ++i) {
        uint8_t *e = img + 12 + 32 * i;
        strcpy((char *)e, F[i].name);
        memcpy(e + 24, &F[i].start, 4);
        memcpy(e + 28, &F[i].size, 4);
    }
    memcpy(img + BLK_SECTOR, "0123456789", 10);
    for (unsigned i = 0; i < 1300; ++i)
        img[2 * BLK_SECTOR + i] = file_byte(i);
    close(write_file(img, sizeof img));
}

/* The machine with the image attached and mounted. */
static void machine_fs(void)
{
    machine();
    make_image(0);
    if (blk_attach(disk_path, RING_PA) < 0)
        exit(2);
    EQ("ramfs_mount: return value", 0, ramfs_mount());
}

/* The n bytes of guest memory at va are bytes first .. of b.bin. */
static int same_bytes(uint64_t va, unsigned first, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
        if ((uint8_t)peek(va + i, 1) != file_byte(first + i))
            return 0;
    return 1;
}

static void t_ramfs(void)
{
    mcpu *c = &cpu;
    const uint64_t V = DATA;        /* two mapped pages; HOLE is not mapped */
    int fd, fd2, fds[RAMFS_MAXFD + 1], ok;
    long n;

    machine_fs();
    EQ("ramfs_mount: ramfs_ndir", 3, ramfs_ndir);
    check(!strcmp(ramfs_dir[1].name, "b.bin") && ramfs_dir[1].start == 2 &&
          ramfs_dir[1].size == 1300,
          "ramfs_mount: ramfs_dir[1] expected b.bin, sector 2, 1300 bytes; got %.24s, %u, %u",
          ramfs_dir[1].name, ramfs_dir[1].start, ramfs_dir[1].size);

    EQ("ramfs_open(\"a\"): a prefix of a.txt is not a name", (uint64_t)-1,
       (uint64_t)(int64_t)ramfs_open("a"));
    EQ("ramfs_open(\"\")", (uint64_t)-1, (uint64_t)(int64_t)ramfs_open(""));
    EQ("ramfs_open of a name that is not in the directory", (uint64_t)-1,
       (uint64_t)(int64_t)ramfs_open("nosuch"));
    fd = ramfs_open("b.bin");
    check(fd >= 3, "ramfs_open(\"b.bin\"): expected a descriptor of 3 or above, got %d", fd);

    /* across a sector boundary and across a page boundary */
    n = ramfs_read(fd, V + PAGE_SIZE - 100, 700, c);
    EQ("ramfs_read of 700 bytes of b.bin: return value", 700, n);
    check(same_bytes(V + PAGE_SIZE - 100, 0, 700),
          "ramfs_read of 700 bytes into a buffer that spans two pages (frames not adjacent): "
          "the guest memory differs from the file");
    n = ramfs_read(fd, V, 4096, c);
    EQ("ramfs_read of 4096 bytes with 600 bytes left: return value", 600, n);
    check(same_bytes(V, 700, 600), "the second ramfs_read continues at byte 700 of the file");
    EQ("ramfs_read at the end of the file", 0, ramfs_read(fd, V, 16, c));

    /* every descriptor has its own position */
    fd2 = ramfs_open("b.bin");
    check(fd2 >= 3 && fd2 != fd, "a second ramfs_open(\"b.bin\"): expected another "
          "descriptor than %d, got %d", fd, fd2);
    EQ("ramfs_read of 1 byte on the second descriptor: return value", 1,
       ramfs_read(fd2, V + 0x800, 1, c));
    check(same_bytes(V + 0x800, 0, 1), "the second descriptor starts at byte 0 of the file");
    for (int k = 0; k < 520; k += 7) {                  /* many short reads */
        n = ramfs_read(fd2, V + 0x900, 7, c);
        if (!check(n == 7 && same_bytes(V + 0x900, 1 + (unsigned)k, 7),
                   "ramfs_read of 7 bytes at byte %d of b.bin: returned %ld, or wrong bytes",
                   1 + k, n))
            break;
    }

    /* a buffer that is not mapped */
    ramfs_close(fd2);
    fd2 = ramfs_open("b.bin");
    EQ("ramfs_read into an unmapped buffer: return value", (uint64_t)-1,
       (uint64_t)ramfs_read(fd2, HOLE, 10, c));
    EQ("the read after it, into a mapped buffer: return value", 4, ramfs_read(fd2, V, 4, c));
    check(same_bytes(V, 0, 4), "a read that copied nothing leaves the position where it was");
    ramfs_close(fd2);
    fd2 = ramfs_open("b.bin");
    n = ramfs_read(fd2, V + 2 * PAGE_SIZE - 512, 700, c);
    EQ("ramfs_read of 700 bytes when only the first 512 fit before an unmapped page: "
       "the bytes copied so far", 512, n);
    EQ("the read after it: return value", 4, ramfs_read(fd2, V, 4, c));
    check(same_bytes(V, 512, 4), "the read after a short read continues at byte 512");

    /* an empty file */
    int fe = ramfs_open("empty");
    check(fe >= 3, "ramfs_open(\"empty\"): got %d", fe);
    EQ("ramfs_read of an empty file", 0, ramfs_read(fe, V, 16, c));

    /* the table of open files */
    ramfs_close(fd);
    ramfs_close(fd2);
    ramfs_close(fe);
    ok = 1;
    for (int i = 0; i < RAMFS_MAXFD; ++i) {
        fds[i] = ramfs_open("a.txt");
        ok &= fds[i] >= 3;
        for (int j = 0; j < i; ++j)
            ok &= fds[i] != fds[j];
    }
    check(ok, "RAMFS_MAXFD files open at once: expected %d different descriptors of 3 or above",
          RAMFS_MAXFD);
    EQ("one more ramfs_open when RAMFS_MAXFD files are open", (uint64_t)-1,
       (uint64_t)(int64_t)ramfs_open("a.txt"));
    EQ("ramfs_close of an open descriptor", 0, ramfs_close(fds[2]));
    EQ("ramfs_close of the same descriptor again", (uint64_t)-1,
       (uint64_t)(int64_t)ramfs_close(fds[2]));
    EQ("ramfs_read on a closed descriptor", (uint64_t)-1, (uint64_t)ramfs_read(fds[2], V, 4, c));
    fd = ramfs_open("b.bin");
    check(fd >= 3, "ramfs_open after a close: the slot is free again; got %d", fd);
    EQ("the reopened descriptor starts at position 0", 3, ramfs_read(fd, V, 3, c));
    check(same_bytes(V, 0, 3), "the reopened descriptor reads its own file, b.bin");
    EQ("ramfs_close(0)", (uint64_t)-1, (uint64_t)(int64_t)ramfs_close(0));
    EQ("ramfs_close(2)", (uint64_t)-1, (uint64_t)(int64_t)ramfs_close(2));
    EQ("ramfs_close(-1)", (uint64_t)-1, (uint64_t)(int64_t)ramfs_close(-1));
    EQ("ramfs_close(1000)", (uint64_t)-1, (uint64_t)(int64_t)ramfs_close(1000));
    EQ("ramfs_read(1, ...)", (uint64_t)-1, (uint64_t)ramfs_read(1, V, 4, c));
    EQ("ramfs_read(1000, ...)", (uint64_t)-1, (uint64_t)ramfs_read(1000, V, 4, c));

    /* a header that claims more files than RAMFS_MAXFILE */
    {
        static uint8_t big[2 * BLK_SECTOR];
        uint32_t many = 100;
        memset(big, 0x5a, sizeof big);
        memcpy(big, RAMFS_MAGIC, 8);
        memcpy(big + 8, &many, 4);
        close(write_file(big, sizeof big));
        if (blk_attach(disk_path, RING_PA) < 0)
            exit(2);
        doing = "ramfs_mount of an image whose header says 100 files";
        EQ("ramfs_mount of an image whose header says 100 files: return value", 0, ramfs_mount());
        doing = NULL;
        EQ("ramfs_mount of an image whose header says 100 files: ramfs_ndir is capped at "
           "RAMFS_MAXFILE", RAMFS_MAXFILE, ramfs_ndir);
    }

    /* an image that is not a ramfs */
    make_image(1);
    if (blk_attach(disk_path, RING_PA) < 0)
        exit(2);
    fflush(stdout);
    int keep = dup(2), null = open("/dev/null", O_WRONLY);
    dup2(null, 2);                                      /* the message of ramfs_mount */
    n = ramfs_mount();
    dup2(keep, 2);
    close(keep);
    close(null);
    EQ("ramfs_mount of an image with another magic", (uint64_t)-1, (uint64_t)n);
}

static void t_sys(void)
{
    mcpu *c = &cpu;
    const uint64_t V = DATA, END = DATA + 2 * PAGE_SIZE;    /* END is not mapped */
    long fd, n;
    int p[2];

    machine_fs();

    poke(V, "a.txt", 6);
    fd = sys_open(c, V);
    check(fd >= 3, "sys_open(\"a.txt\"): expected a descriptor, got %ld", fd);
    poke(V + 0x40, "nosuch", 7);
    EQ("sys_open of a name that is not in the directory: -ENOENT", (uint64_t)-ENOENT,
       (uint64_t)sys_open(c, V + 0x40));
    EQ("sys_open with a pointer to an unmapped page: -EFAULT", (uint64_t)-EFAULT,
       (uint64_t)sys_open(c, HOLE));
    poke(END - 6, "a.txt", 6);                          /* the NUL is the last mapped byte */
    n = sys_open(c, END - 6);
    check(n >= 3, "sys_open of a name whose NUL is the last byte of a mapped page: "
          "expected a descriptor, got %ld (read the name one byte at a time)", n);
    sys_close(c, (uint64_t)n);
    poke(END - 4, "a.tx", 4);                           /* no NUL before the unmapped page */
    EQ("sys_open of a name that runs into an unmapped page: -EFAULT", (uint64_t)-EFAULT,
       (uint64_t)sys_open(c, END - 4));

    {
        char longname[64];
        memset(longname, 'x', 63);
        poke(END - 63, longname, 63);                    /* 63 bytes, no NUL before the unmapped page */
        EQ("sys_open of a 63-byte name without a NUL that ends at an unmapped page: the name "
           "is cut at 63 bytes, -ENOENT (not -EFAULT: the 64th byte is never read)",
           (uint64_t)-ENOENT, (uint64_t)sys_open(c, END - 63));
    }
    EQ("sys_read(fd, buf, 4) on a.txt: return value", 4, sys_read(c, (uint64_t)fd, V + 0x100, 4));
    EQ("sys_read(fd, buf, 4) on a.txt: the bytes \"0123\"", 0x33323130, peek(V + 0x100, 4));
    EQ("sys_read(fd, buf, 100) with 6 bytes left", 6, sys_read(c, (uint64_t)fd, V + 0x100, 100));
    EQ("sys_read at the end of the file", 0, sys_read(c, (uint64_t)fd, V + 0x100, 100));
    EQ("sys_read on a descriptor that is not open: -EBADF", (uint64_t)-EBADF,
       (uint64_t)sys_read(c, 99, V, 4));
    EQ("sys_close on a descriptor that is not open: -EBADF", (uint64_t)-EBADF,
       (uint64_t)sys_close(c, 99));
    EQ("sys_close(1): not a file of the ramfs, -EBADF", (uint64_t)-EBADF,
       (uint64_t)sys_close(c, 1));
    EQ("sys_close of an open descriptor", 0, sys_close(c, (uint64_t)fd));
    EQ("sys_close of the same descriptor again: -EBADF", (uint64_t)-EBADF,
       (uint64_t)sys_close(c, (uint64_t)fd));
    EQ("sys_read after sys_close: -EBADF", (uint64_t)-EBADF,
       (uint64_t)sys_read(c, (uint64_t)fd, V, 4));

    /* descriptor 0 is the standard input of the host: a pipe here */
    if (pipe(p) < 0 || write(p[1], "hello, guest", 12) != 12 || dup2(p[0], 0) < 0) {
        perror("pipe");
        exit(2);
    }
    poke(V + 0x200, "................", 16);
    EQ("sys_read(0, buf, 5) with 12 bytes on the standard input: return value", 5,
       sys_read(c, 0, V + 0x200, 5));
    EQ("sys_read(0, buf, 5): the bytes \"hello\", and the byte after them untouched",
       0x2e2e2e6f6c6c6568ULL, peek(V + 0x200, 8));
    EQ("sys_read(0, buf, 100) with 7 bytes left: return value", 7,
       sys_read(c, 0, V + 0x200, 100));
    n = write(p[1], "xy", 2);
    EQ("sys_read(0, unmapped buffer, 2): -EFAULT", (uint64_t)-EFAULT,
       (uint64_t)sys_read(c, 0, HOLE, 2));
    /* descriptor 2 is the host's as well */
    {
        int q[2], err = dup(2);
        fflush(stderr);
        if (err < 0 || pipe(q) < 0 || write(q[1], "err", 3) != 3 || dup2(q[0], 2) < 0)
            exit(2);
        n = sys_read(c, 2, V + 0x200, 8);
        dup2(err, 2);                                   /* stderr is back */
        close(err); close(q[0]); close(q[1]);
        EQ("sys_read(2, buf, 8) with 3 bytes readable on descriptor 2 of the host", 3, n);
    }
    /* at most 4096 bytes of the host per read */
    {
        static char many[6000];
        memset(many, 'm', sizeof many);
        if (write(p[1], many, sizeof many) != (ssize_t)sizeof many)
            exit(2);
        doing = "sys_read(0, buf, 6000)";
        EQ("sys_read(0, buf, 6000) with 6000 bytes on the standard input: at most 4096", 4096,
           sys_read(c, 0, V, 6000));
        doing = NULL;
        EQ("the rest of them with another sys_read(0, buf, 6000)", 6000 - 4096,
           sys_read(c, 0, V, 6000));
    }
    close(p[1]);
    EQ("sys_read(0, buf, 10) at the end of the input", 0, sys_read(c, 0, V + 0x200, 10));
    close(0);
    close(p[0]);
    EQ("sys_read(0, ...) when the host read fails: -EIO", (uint64_t)-EIO,
       (uint64_t)sys_read(c, 0, V + 0x200, 10));
    (void)n;
}

#endif /* LAB >= 6 */

/* -------------------------------------------------------------------- main */

static const struct {
    const char *name;
    void (*run)(void);
} GROUPS[] = {
    { "modrm", t_modrm }, { "alu", t_alu }, { "shift", t_shift }, { "muldiv", t_muldiv },
    { "cond", t_cond },
#if LAB >= 2
    { "seq", t_seq }, { "pipe", t_pipe },
#endif
#if LAB >= 4
    { "mmu", t_mmu },
#endif
#if LAB >= 5
    { "cache", t_cache },
#endif
#if LAB >= 6
    { "blk", t_blk }, { "ramfs", t_ramfs }, { "sys", t_sys },
#endif
};

int main(int argc, char **argv)
{
    int i = 1;
    for (; i + 1 < argc && argv[i][0] == '-'; i += 2) {
        if (!strcmp(argv[i], "--ram"))       ram_path = argv[i + 1];
        else if (!strcmp(argv[i], "--disk")) disk_path = argv[i + 1];
        else break;
    }
    if (i + 1 != argc) {
        fputs("usage: mini-unit [--ram FILE] [--disk FILE] GROUP\ngroups:", stderr);
        for (unsigned g = 0; g < sizeof GROUPS / sizeof GROUPS[0]; ++g)
            fprintf(stderr, " %s", GROUPS[g].name);
        fputc('\n', stderr);
        return 2;
    }
    for (unsigned g = 0; g < sizeof GROUPS / sizeof GROUPS[0]; ++g)
        if (!strcmp(argv[i], GROUPS[g].name)) {
            group = GROUPS[g].name;
            signal(SIGFPE, on_signal);
            signal(SIGSEGV, on_signal);
            signal(SIGBUS, on_signal);
            signal(SIGILL, on_signal);
            signal(SIGABRT, on_signal);
            GROUPS[g].run();
            if (failed > 12)
                printf("%s: ... and %u more\n", group, failed - 12);
            printf("%s: %u checks, %u failed\n", group, checks, failed);
            return failed != 0;
        }
    fprintf(stderr, "mini-unit: no group named %s in a build of lab %d\n", argv[i], LAB);
    return 2;
}
