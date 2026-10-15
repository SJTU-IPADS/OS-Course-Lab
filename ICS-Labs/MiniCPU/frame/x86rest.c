/* The instructions the interpreter of lab 1 leaves to the framework.
 *
 * interp.c implements the instructions a lecture on x86 covers: mov, lea,
 * the eight ALU operations, test, push and pop, call and ret, jmp and jcc.
 * gcc also emits instructions outside that set: 8-bit and 16-bit forms,
 * movzx and movsx, cmov and setcc, multiply and divide, shifts, endbr64,
 * syscall; tests/lab1/flagops.py adds pushfq and popfq. x86_other runs
 * those with the same helpers as interp.c, so interp.c can hand it any
 * opcode it does not implement itself. What x86_other does here is fetch
 * the operands and store the result: the results and the flags of inc,
 * dec, neg, the shifts, multiplication and division come from alu.c, as
 * those of the eight ALU operations do.
 *
 * The floating-point instructions and three more vector instructions run
 * here as well; frame/simd.c lists them and computes their values.
 *
 * The opcodes of interp.c are illegal here, its seven vector instructions
 * included.
 */
#include "mcpu.h"

#define R x->c->R

static void illegal(x86 *x)
{
    if (!x->c->fault) {
        x->c->fault = FAULT_ILL;
        x->c->fault_va = x->pc;
    }
}

/* The rm operand at size sz, when it differs from the operand size. */
static uint64_t rm_get_sized(x86 *x, int sz)
{
    int size = x->size;
    x->size = sz;
    uint64_t v = rm_get(x);
    x->size = size;
    return v;
}

/* ah: the second byte of rax. */
static uint64_t ah(x86 *x)
{
    return (R[RAX] >> 8) & 0xff;
}

/* rax at size 2, for the results of 8-bit multiply and divide. */
static void set_ax(x86 *x, uint64_t lo, uint64_t hi)
{
    R[RAX] = (R[RAX] & ~0xffffULL) | (lo & 0xff) | ((hi & 0xff) << 8);
}

static void group3(x86 *x)                              /* f6, f7 */
{
    uint64_t v, lo, hi, q, r;
    int sz = x->size;

    if (x->sub <= 1) {                                  /* test rm, imm */
        v = fetch_imm(x);                               /* before ea: see mcpu.h */
        x86_logic(&R[RFLAGS], rm_get(x) & v, sz);
        return;
    }
    v = rm_get(x);
    switch (x->sub) {
    case 2:                                             /* not */
        rm_set(x, ~v);
        break;
    case 3:                                             /* neg */
        rm_set(x, x86_neg(&R[RFLAGS], v, sz));
        break;
    case 4: case 5:                                     /* mul, imul: rdx:rax = rax * rm */
        x86_mul(&R[RFLAGS], x->sub, get(x, RAX), v, sz, &lo, &hi);
        if (sz == 1) {
            set_ax(x, lo, hi);
        } else {
            set(x, RAX, lo);
            set(x, RDX, hi);
        }
        break;
    default:                                            /* div, idiv */
        if (x->c->fault)
            break;
        if (x86_div(x->sub, sz == 1 ? ah(x) : get(x, RDX), get(x, RAX),
                    v, sz, &q, &r) < 0) {
            x->c->fault = FAULT_DIV;
            x->c->fault_va = x->pc;
            break;
        }
        if (sz == 1) {
            set_ax(x, q, r);
        } else {
            set(x, RAX, q);
            set(x, RDX, r);
        }
        break;
    }
}

static void group5(x86 *x, unsigned op)                 /* fe, ff */
{
    uint64_t v;

    switch (x->sub) {
    case 0: case 1:                                     /* inc, dec */
        v = rm_get(x);
        rm_set(x, x->sub == 0 ? x86_inc(&R[RFLAGS], v, x->size)
                              : x86_dec(&R[RFLAGS], v, x->size));
        return;
    }
    if (op == 0xfe) {
        illegal(x);
        return;
    }
    x->size = 8;
    switch (x->sub) {
    case 2:                                             /* call *rm */
        v = rm_get(x);
        store(x, R[RSP] - 8, 8, R[RIP]);
        R[RSP] -= 8;
        R[RIP] = v;
        break;
    case 4:                                             /* jmp *rm */
        R[RIP] = rm_get(x);
        break;
    case 6:                                             /* push rm */
        v = rm_get(x);
        store(x, R[RSP] - 8, 8, v);
        R[RSP] -= 8;
        break;
    default:
        illegal(x);
        break;
    }
}

/* A floating-point instruction, row s of sse_ops[] (frame/simd.c). Its
   vector operands are xmm registers; bits 128..255 of a register it writes
   stay as they are. */
static void sse(x86 *x, const sse_op *s)
{
    mcpu *c = x->c;
    u256 a, b, r;

    modrm(x);
    int mem = x->rm < 0;
    x->size = x->rex & 8 ? 8 : 4;                       /* of an integer operand: 66 is */
    switch (s->form) {                                  /* no size prefix here */
    case SSE_RM_TO_REG:
    case SSE_CMP:
        rm_vget(x, &b, mem ? s->width : 16);
        a = c->Y[x->reg];
        sse_compute(s, mem, x->size, &a, &b, &r, &R[RFLAGS]);
        if (s->form == SSE_RM_TO_REG)
            vset(x, x->reg, &r, 32);
        break;
    case SSE_REG_TO_RM:
        b = c->Y[x->reg];
        if (mem) {
            rm_vset(x, &b, s->width);
        } else {
            a = c->Y[x->rm];
            sse_compute(s, 0, x->size, &a, &b, &r, &R[RFLAGS]);
            vset(x, x->rm, &r, 32);
        }
        break;
    case SSE_INT_TO_REG:
        b.q[0] = rm_get(x);
        a = c->Y[x->reg];
        sse_compute(s, mem, x->size, &a, &b, &r, &R[RFLAGS]);
        vset(x, x->reg, &r, 32);
        break;
    case SSE_RM_TO_INT:
        rm_vget(x, &b, mem ? s->width : 16);
        sse_compute(s, mem, x->size, &b, &b, &r, &R[RFLAGS]);
        set(x, x->reg, r.q[0]);
        break;
    default:                                            /* SSE_REG_TO_INT */
        b = c->Y[x->reg];
        sse_compute(s, mem, x->size, &b, &b, &r, &R[RFLAGS]);
        rm_set(x, r.q[0]);
        break;
    }
}

/* The vector instructions outside interp.c (frame/simd.c). */
static void vex_other(x86 *x)
{
    u256 a, b, r;
    unsigned imm;

    modrm(x);
    switch (x->op) {
    case 0x11db:                                        /* vpand rm, v, reg */
        rm_vget(x, &b, x->vlen);
        a = x->c->Y[x->v];
        vex_compute(x->op, x->vlen, 0, &a, &b, &r);
        vset(x, x->reg, &r, x->vlen);
        break;
    case 0x1172:                                        /* vpsrld imm8, rm, v: 72 /2 ib */
        imm = (unsigned)fetch8(x);
        if (x->sub != 2 || x->rm < 0) {
            illegal(x);
            break;
        }
        rm_vget(x, &b, x->vlen);
        vex_compute(x->op, x->vlen, imm, &b, &b, &r);
        vset(x, x->v, &r, x->vlen);
        break;
    case 0x1231:                                        /* vpmovzxbd xmm/m64, ymm */
        rm_vget(x, &b, x->vlen / 4);
        vex_compute(x->op, x->vlen, 0, &b, &b, &r);
        vset(x, x->reg, &r, x->vlen);
        break;
    default:                                            /* interp.c */
        illegal(x);
        break;
    }
}

void x86_other(x86 *x)
{
    unsigned op = x->op;
    uint64_t v, a;
    int sz;

    if (x->vex) {
        vex_other(x);
        return;
    }
    const sse_op *s = sse_find(op, x->pp);
    if (s) {
        sse(x, s);
        return;
    }
    if (op < 0x40 && (op & 7) < 6) {                    /* ALU: 8-bit forms, al/eax imm */
        int aop = op >> 3, form = op & 7;
        if (form == 1 || form == 3) {
            illegal(x);                                 /* interp.c */
            return;
        }
        if (form == 0 || form == 2 || form == 4)
            x->size = 1;
        if (form >= 4) {
            v = x86_alu(&R[RFLAGS], aop, get(x, RAX), fetch_imm(x), x->size);
            if (aop != 7) set(x, RAX, v);
            return;
        }
        modrm(x);
        if (form == 0) {
            v = x86_alu(&R[RFLAGS], aop, rm_get(x), get(x, x->reg), 1);
            if (aop != 7) rm_set(x, v);
        } else {
            v = x86_alu(&R[RFLAGS], aop, get(x, x->reg), rm_get(x), 1);
            if (aop != 7) set(x, x->reg, v);
        }
        return;
    }

    switch (op) {
    case 0x63:                                          /* movsxd r, rm32 */
        modrm(x);
        set(x, x->reg, (uint64_t)sext(rm_get_sized(x, 4), 4));
        break;
    case 0x68: case 0x6a:                               /* push imm */
        v = op == 0x68 ? (uint64_t)(int64_t)(int32_t)fetch32(x)
                       : (uint64_t)(int64_t)(int8_t)fetch8(x);
        store(x, R[RSP] - 8, 8, v);
        R[RSP] -= 8;
        break;
    case 0x69: case 0x6b: case 0x1af: {                 /* imul r, rm, imm; imul r, rm */
        uint64_t lo, hi;
        modrm(x);
        if (op == 0x1af)
            v = get(x, x->reg);
        else if (op == 0x6b)
            v = (uint64_t)(int64_t)(int8_t)fetch8(x);
        else
            v = fetch_imm(x);
        a = rm_get(x);
        x86_mul(&R[RFLAGS], 5, a, v, x->size, &lo, &hi);
        set(x, x->reg, lo);
        break;
    }
    case 0x80:                                          /* group 1, 8-bit */
        x->size = 1;
        modrm(x);
        v = fetch_imm(x);
        a = x86_alu(&R[RFLAGS], x->sub, rm_get(x), v, 1);
        if (x->sub != 7) rm_set(x, a);
        break;
    case 0x84:                                          /* test rm8, r8 */
        x->size = 1;
        modrm(x);
        x86_logic(&R[RFLAGS], rm_get(x) & get(x, x->reg), 1);
        break;
    case 0x87:                                          /* xchg rm, r */
        modrm(x);
        a = rm_get(x);
        v = get(x, x->reg);
        rm_set(x, v);
        set(x, x->reg, a);
        break;
    case 0x88:                                          /* mov rm8, r8 */
        x->size = 1;
        modrm(x);
        rm_set(x, get(x, x->reg));
        break;
    case 0x8a:                                          /* mov r8, rm8 */
        x->size = 1;
        modrm(x);
        set(x, x->reg, rm_get(x));
        break;
    case 0x90:                                          /* nop */
        break;
    case 0x98:                                          /* cwtl, cltq */
        set(x, RAX, (uint64_t)sext(R[RAX], x->size / 2));
        break;
    case 0x99:                                          /* cltd, cqto */
        set(x, RDX, sext(get(x, RAX), x->size) < 0 ? ~0ULL : 0);
        break;
    case 0x9c:                                          /* pushfq */
        store(x, R[RSP] - 8, 8, R[RFLAGS]);
        R[RSP] -= 8;
        break;
    case 0x9d:                                          /* popfq: the five flags only */
        v = load(x, R[RSP], 8);
        R[RSP] += 8;
        R[RFLAGS] = (v & (CF | PF | ZF | SF | OF)) | 2;
        break;
    case 0xa8: case 0xa9:                               /* test al/eax, imm */
        if (op == 0xa8) x->size = 1;
        x86_logic(&R[RFLAGS], get(x, RAX) & fetch_imm(x), x->size);
        break;
    case 0xb0: case 0xb1: case 0xb2: case 0xb3:         /* mov r8, imm8 */
    case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        x->size = 1;
        set(x, (op & 7) | ((x->rex & 1) << 3), fetch8(x));
        break;
    case 0xc0: case 0xc1: case 0xd0: case 0xd1: case 0xd2: case 0xd3: {
        unsigned n;                                     /* shifts and rotations */
        if (op == 0xc0 || op == 0xd0 || op == 0xd2) x->size = 1;
        modrm(x);
        if (x->sub == 2 || x->sub == 3) {               /* rcl, rcr */
            illegal(x);
            break;
        }
        if (op <= 0xc1) n = (unsigned)fetch8(x);
        else if (op <= 0xd1) n = 1;
        else n = (unsigned)(R[RCX] & 0xff);
        v = rm_get(x);
        rm_set(x, x->sub <= 1 ? x86_rotate(&R[RFLAGS], x->sub, v, n, x->size)
                              : x86_shift(&R[RFLAGS], x->sub, v, n, x->size));
        break;
    }
    case 0xc6:                                          /* mov rm8, imm8 */
        x->size = 1;
        modrm(x);
        rm_set(x, fetch_imm(x));
        break;
    case 0xc9:                                          /* leave */
        v = load(x, R[RBP], 8);
        R[RSP] = R[RBP] + 8;
        R[RBP] = v;
        break;
    case 0xf6: case 0xf7:
        if (op == 0xf6) x->size = 1;
        modrm(x);
        group3(x);
        break;
    case 0xfe: case 0xff:
        if (op == 0xfe) x->size = 1;
        modrm(x);
        group5(x, op);
        break;
    case 0x105:                                         /* syscall */
        syscall_enter(x->c);
        R[RCX] = R[RIP];
        R[R11] = R[RFLAGS];
        break;
    case 0x10d: case 0x11e: case 0x11f:                 /* endbr64, multi-byte nop */
        modrm(x);
        break;
    case 0x1b6: case 0x1b7: case 0x1be: case 0x1bf:     /* movzx, movsx */
        sz = (op & 1) ? 2 : 1;
        modrm(x);
        v = rm_get_sized(x, sz);
        set(x, x->reg, op >= 0x1be ? (uint64_t)sext(v, sz) : v);
        break;
    default:
        if ((op & 0x1f0) == 0x140) {                    /* cmovcc r, rm */
            modrm(x);
            v = rm_get(x);
            set(x, x->reg, cond(R[RFLAGS], (int)op) ? v : get(x, x->reg));
        } else if ((op & 0x1f0) == 0x190) {             /* setcc rm8 */
            x->size = 1;
            modrm(x);
            rm_set(x, (uint64_t)cond(R[RFLAGS], (int)op));
        } else {
            illegal(x);
        }
        break;
    }
}
