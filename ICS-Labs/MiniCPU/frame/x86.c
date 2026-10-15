/* Helpers of the interpreter of lab 1: fetching, operands, memory, and
 * the start and end of every instruction.
 *
 * interp.c works on the registers directly (R[RAX], R[RIP], ...). What it
 * leaves to this file is the part of x86 that is bookkeeping rather than
 * computation: prefixes, 8-bit register names, page faults in the middle
 * of an instruction, and the commit log. The ModRM byte is decoded by
 * modrm_decode of src/lab1/modrm.c; modrm() here turns its result into
 * the operands of the instruction.
 */
#include <stdio.h>
#include <string.h>

#include "mcpu.h"

/* ------------------------------------------------------------- fetching */

/* w bytes at R[RIP], from the bytes x86_begin fetched. Running past them
   is a page fault at the first byte that could not be fetched, or an
   illegal instruction if all MAX_INSN bytes were there. */
static uint64_t take(x86 *x, unsigned w)
{
    mcpu *c = x->c;
    uint64_t off = c->rip - x->pc, v = 0;
    if (off + w > x->n) {
        if (!c->fault) {
            c->fault = x->n < MAX_INSN ? FAULT_PAGE : FAULT_ILL;
            c->fault_va = x->pc + x->n;
        }
    } else {
        memcpy(&v, x->buf + off, w);
    }
    c->rip += w;
    return v;
}

uint64_t fetch8(x86 *x)  { return take(x, 1); }
uint64_t fetch16(x86 *x) { return take(x, 2); }
uint64_t fetch32(x86 *x) { return take(x, 4); }
uint64_t fetch64(x86 *x) { return take(x, 8); }

uint64_t fetch_imm(x86 *x)
{
    switch (x->size) {
    case 1:  return (uint64_t)(int64_t)(int8_t)fetch8(x);
    case 2:  return (uint64_t)(int64_t)(int16_t)fetch16(x);
    default: return (uint64_t)(int64_t)(int32_t)fetch32(x);
    }
}

/* Opcode returned when the instruction could not even be fetched. */
#define OP_NONE 0x200

int vex_known(unsigned op, int pp, int l, int w, int v)
{
    (void)w;
    switch (op) {
    case 0x11ef:                                        /* vpxor */
    case 0x11fe:                                        /* vpaddd */
    case 0x1240:                                        /* vpmulld */
        return pp == 1;
    case 0x116f:                                        /* vmovdqu ymm, rm */
        return pp == 2 && v == 0;
    case 0x1202:                                        /* vphaddd, xmm only */
        return pp == 1 && l == 0;
    case 0x1339:                                        /* vextracti128 */
        return pp == 1 && l == 1 && w == 0 && v == 0;
    case 0x117e:                                        /* vmovd xmm, r/m32 */
        return pp == 1 && l == 0 && w == 0 && v == 0;
    case 0x11db:                                        /* vpand */
    case 0x1172:                                        /* vpsrld imm8: 72 /2 ib */
        return pp == 1;
    case 0x1231:                                        /* vpmovzxbd */
        return pp == 1 && v == 0;
    default:
        return 0;
    }
}

/* The VEX prefix c4 or c5, already fetched as b. Its inverted R, X and B
   bits and W become x->rex, so that modrm and the register helpers treat
   them as a REX prefix. Returns the opcode, or OP_NONE for an instruction
   vex_known rejects. */
static unsigned vex(x86 *x, unsigned b)
{
    unsigned b1 = (unsigned)fetch8(x), map = 1, w = 0, b2 = b1;
    if (b == 0xc4) {
        map = b1 & 0x1f;
        b2 = (unsigned)fetch8(x);
        w = b2 >> 7;
    }
    unsigned r = !(b1 & 0x80), xb = b == 0xc4 && !(b1 & 0x40),
             bb = b == 0xc4 && !(b1 & 0x20);
    x->vex = 1;
    x->rex = (uint8_t)(0x40 | w << 3 | r << 2 | xb << 1 | bb);
    x->size = w ? 8 : 4;
    x->v = (int)(~b2 >> 3 & 15);
    x->vlen = b2 & 4 ? 32 : 16;
    unsigned op = 0x1000 | map << 8 | (unsigned)fetch8(x);
    if (x->c->fault)
        return OP_NONE;
    if (map < 1 || map > 3 ||
        !vex_known(op, (int)(b2 & 3), x->vlen == 32, (int)w, x->v)) {
        x->c->fault = FAULT_ILL;
        x->c->fault_va = x->pc;
        return OP_NONE;
    }
    return op;
}

/* The vector registers before the instruction, for x86_end. */
static void ysave(x86 *x)
{
    memcpy(x->ysaved, x->c->Y, sizeof x->ysaved);
    x->ysave = 1;
}

unsigned x86_begin(mcpu *c, x86 *x)
{
    memset(x, 0, offsetof(x86, buf));                   /* the arrays behind are written */
    x->c = c;                                           /* before they are read */
    x->pc = c->rip;
    x->size = 4;
    x->reg = x->rm = -1;
    memcpy(x->saved, c->R, sizeof x->saved);
    x->n = fetch_bytes(c, c->rip, x->buf);

    unsigned op;
    for (;;) {                                          /* prefixes */
        op = (unsigned)fetch8(x);
        if (c->fault) return x->op = OP_NONE;
        if (op == 0x66) {                               /* f2 and f3 come before 66 */
            x->size = 2;
            if (!x->pp) x->pp = 1;
            continue;
        }
        if (op == 0xf2 || op == 0xf3) { x->pp = op == 0xf3 ? 2 : 3; continue; }
        if (op == 0x2e || op == 0x3e || op == 0x26 ||
            op == 0x36 || op == 0x64 || op == 0x65) continue;
        if ((op & 0xf0) == 0x40) {                      /* REX */
            x->rex = (uint8_t)op;
            if (op & 8) x->size = 8;
            continue;
        }
        if (op == 0xc4 || op == 0xc5) {                 /* VEX */
            ysave(x);
            return x->op = vex(x, op);
        }
        break;
    }
    if (op == 0x0f) {
        op = 0x100 | (unsigned)fetch8(x);
        if (sse_find(op, x->pp))
            ysave(x);
    }
    return x->op = c->fault ? OP_NONE : op;
}

int x86_end(x86 *x)
{
    mcpu *c = x->c;
    if (c->fault) {
        /* Precise: the instruction leaves no trace in the registers. */
        memcpy(c->R, x->saved, sizeof x->saved);
        if (x->ysave)
            memcpy(c->Y, x->ysaved, sizeof x->ysaved);
        c->rip = x->pc;
        return -1;
    }
    ++c->icount;
    commit_line(c, x->pc, x->store, x->sa, &x->sval, x->msize);
    if (opt_steps)
        fprintf(stderr, "%08lx  op %-3x rax=%-16lx rsp=%-12lx rdi=%lx\n",
                (unsigned long)x->pc, x->op, (unsigned long)c->r[RAX],
                (unsigned long)c->r[RSP], (unsigned long)c->r[RDI]);
    return c->halted ? -1 : 0;
}

static void illegal(x86 *x)
{
    if (!x->c->fault) {
        x->c->fault = FAULT_ILL;
        x->c->fault_va = x->pc;
    }
}

/* ------------------------------------------------------------- operands */

void modrm(x86 *x)
{
    mcpu *c = x->c;
    uint64_t off = c->rip - x->pc;
    unsigned avail = off < x->n ? x->n - (unsigned)off : 0;
    modrm_t m;
    unsigned k = avail ? modrm_decode(x->buf + off, avail, x->rex, &m) : 0;

    if (k == 0 || k > avail) {                          /* the bytes ran out */
        if (!c->fault) {
            c->fault = x->n < MAX_INSN ? FAULT_PAGE : FAULT_ILL;
            c->fault_va = x->pc + x->n;
        }
        c->rip += 1;
        x->rm = -1;
        return;
    }
    c->rip += k;
    x->sub = m.sub;
    x->reg = m.reg;
    x->rm = m.rm;
    if (m.rm >= 0)
        return;
    x->riprel = m.rip;
    x->addr = (uint64_t)m.disp;
    if (m.base >= 0)
        x->addr += c->r[m.base];
    if (m.index >= 0)
        x->addr += c->r[m.index] * m.scale;
}

uint64_t ea(x86 *x)
{
    if (x->rm >= 0) {                                   /* lea with a register */
        illegal(x);
        return 0;
    }
    return x->riprel ? x->addr + x->c->rip : x->addr;
}

/* Without REX, 8-bit register numbers 4..7 name ah, ch, dh and bh. */
static int is_hi(const x86 *x, int n, int size)
{
    return size == 1 && !x->rex && n >= 4 && n < 8;
}

static uint64_t get_sized(x86 *x, int n, int size)
{
    if (is_hi(x, n, size))
        return (x->c->r[n - 4] >> 8) & 0xff;
    return trunc_val(x->c->R[n], size);
}

static void set_sized(x86 *x, int n, int size, uint64_t v)
{
    uint64_t *r = &x->c->R[n];
    switch (size) {
    case 1:
        if (is_hi(x, n, size)) {
            r = &x->c->r[n - 4];
            *r = (*r & ~0xff00ULL) | ((v & 0xff) << 8);
        } else {
            *r = (*r & ~0xffULL) | (v & 0xff);
        }
        break;
    case 2:  *r = (*r & ~0xffffULL) | (v & 0xffff); break;
    case 4:  *r = v & 0xffffffffu; break;               /* clears the upper half */
    default: *r = v; break;
    }
}

uint64_t get(x86 *x, int n)
{
    return get_sized(x, n, x->size);
}

void set(x86 *x, int n, uint64_t v)
{
    set_sized(x, n, x->size, v);
}

uint64_t load(x86 *x, uint64_t addr, int size)
{
    uint64_t v = 0;
    if (x->c->fault)
        return 0;
    if (mem_read(x->c, addr, &v, (unsigned)size, MEM_READ) < 0)
        x->c->fault = FAULT_PAGE;                       /* mem_read set fault_va */
    return v;
}

void store(x86 *x, uint64_t addr, int size, uint64_t v)
{
    if (x->c->fault)
        return;
    if (mem_write(x->c, addr, &v, (unsigned)size) < 0) {
        x->c->fault = FAULT_PAGE;
        return;
    }
    x->store = 1;
    x->msize = (uint8_t)size;
    x->sa = addr;
    memset(&x->sval, 0, sizeof x->sval);
    x->sval.q[0] = trunc_val(v, size);
}

uint64_t rm_get(x86 *x)
{
    return x->rm >= 0 ? get(x, x->rm) : load(x, ea(x), x->size);
}

void rm_set(x86 *x, uint64_t v)
{
    if (x->rm >= 0)
        set(x, x->rm, v);
    else
        store(x, ea(x), x->size, v);
}

/* ------------------------------------------------------ vector operands */

void rm_vget(x86 *x, u256 *v, int n)
{
    if (x->rm >= 0) {
        *v = x->c->Y[x->rm];
        memset(v->b + n, 0, 32 - (size_t)n);
        return;
    }
    memset(v, 0, sizeof *v);
    if (!x->c->fault && mem_read(x->c, ea(x), v->b, (unsigned)n, MEM_READ) < 0)
        x->c->fault = FAULT_PAGE;
}

void vset(x86 *x, int reg, const u256 *v, int n)
{
    u256 *y = &x->c->Y[reg];
    memmove(y->b, v->b, (size_t)n);
    memset(y->b + n, 0, 32 - (size_t)n);
}

void rm_vset(x86 *x, const u256 *v, int n)
{
    if (x->rm >= 0) {
        vset(x, x->rm, v, n);
        return;
    }
    uint64_t a = ea(x);
    if (x->c->fault)
        return;
    if (mem_write(x->c, a, v->b, (unsigned)n) < 0) {
        x->c->fault = FAULT_PAGE;
        return;
    }
    x->store = 1;
    x->msize = (uint8_t)n;
    x->sa = a;
    memset(&x->sval, 0, sizeof x->sval);
    memcpy(x->sval.b, v->b, (size_t)n);
}
