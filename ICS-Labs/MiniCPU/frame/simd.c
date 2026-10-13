/* The instructions of the machine outside the integer subset and the seven
 * vector instructions of interp.c: what they compute.
 *
 *   SSE     scalar single-precision arithmetic, the moves and the logic
 *           operations that come with it: what gcc emits for float code
 *           (guest/ollama is compiled with -msse2). 23 instructions in
 *           30 encodings, the rows of sse_ops[].
 *   AVX2    vpand, vpsrld by an immediate and vpmovzxbd, which
 *           guest/ollama/dot_q4.s uses next to the seven of interp.c to
 *           unpack 4-bit weights.
 *
 * The functions here read no register file and no memory: x86_other of
 * the interpreter (frame/x86rest.c) and compute() of the CPU of lab 2
 * (frame/compute.c) fetch the operands, call them and store the result,
 * so both write the same commit log. frame/decode.c and frame/translate.c
 * read the operands of an SSE instruction from the same table.
 *
 * Floating-point arithmetic is done by the host in IEEE single precision,
 * rounding to nearest. There is no MXCSR register: exceptions are masked
 * and the rounding mode is fixed, which is the state a program starts in.
 * movaps and movdqa do not check the alignment of their memory operand.
 */
#include <math.h>
#include <string.h>

#include "mcpu.h"

const sse_op sse_ops[] = {
    { 0x110, 0, SSE_RM_TO_REG, 16, "movups" },
    { 0x111, 0, SSE_REG_TO_RM, 16, "movups" },
    { 0x110, 2, SSE_RM_TO_REG,  4, "movss" },       /* from memory: bits 32..127 cleared */
    { 0x111, 2, SSE_REG_TO_RM,  4, "movss" },
    { 0x128, 0, SSE_RM_TO_REG, 16, "movaps" },
    { 0x129, 0, SSE_REG_TO_RM, 16, "movaps" },
    { 0x16f, 1, SSE_RM_TO_REG, 16, "movdqa" },
    { 0x17f, 1, SSE_REG_TO_RM, 16, "movdqa" },
    { 0x16f, 2, SSE_RM_TO_REG, 16, "movdqu" },
    { 0x17f, 2, SSE_REG_TO_RM, 16, "movdqu" },
    { 0x17e, 2, SSE_RM_TO_REG,  8, "movq" },        /* bits 64..127 cleared */
    { 0x1d6, 1, SSE_REG_TO_RM,  8, "movq" },
    { 0x16e, 1, SSE_INT_TO_REG, 0, "movd" },        /* movq with REX.W */
    { 0x17e, 1, SSE_REG_TO_INT, 0, "movd" },
    { 0x12a, 2, SSE_INT_TO_REG, 0, "cvtsi2ss" },
    { 0x12c, 2, SSE_RM_TO_INT,  4, "cvttss2si" },
    { 0x12e, 0, SSE_CMP,        4, "ucomiss" },
    { 0x12f, 0, SSE_CMP,        4, "comiss" },
    { 0x151, 2, SSE_RM_TO_REG,  4, "sqrtss" },
    { 0x154, 0, SSE_RM_TO_REG, 16, "andps" },
    { 0x155, 0, SSE_RM_TO_REG, 16, "andnps" },
    { 0x156, 0, SSE_RM_TO_REG, 16, "orps" },
    { 0x157, 0, SSE_RM_TO_REG, 16, "xorps" },
    { 0x158, 2, SSE_RM_TO_REG,  4, "addss" },
    { 0x159, 2, SSE_RM_TO_REG,  4, "mulss" },
    { 0x15c, 2, SSE_RM_TO_REG,  4, "subss" },
    { 0x15d, 2, SSE_RM_TO_REG,  4, "minss" },
    { 0x15e, 2, SSE_RM_TO_REG,  4, "divss" },
    { 0x15f, 2, SSE_RM_TO_REG,  4, "maxss" },
    { 0x1ef, 1, SSE_RM_TO_REG, 16, "pxor" },
    { 0, 0, 0, 0, NULL }
};

const sse_op *sse_find(unsigned op, int pp)
{
    static const sse_op *at[256][4];                    /* by opcode byte and prefix */
    static int ready;

    if (!ready) {
        for (const sse_op *s = sse_ops; s->name; ++s)
            at[s->op & 0xff][s->pp] = s;
        ready = 1;
    }
    return op >= 0x100 && op < 0x200 ? at[op & 0xff][pp & 3] : NULL;
}

void sse_compute(const sse_op *s, int mem, int isz, const u256 *a, const u256 *b,
                 u256 *r, uint64_t *f)
{
    float x = a->f[0], y = b->f[0];
    unsigned op = s->op & 0xff;

    switch (s->form) {
    case SSE_CMP:                                       /* unordered: one of them is a NaN */
        *f &= ~(uint64_t)(CF | PF | ZF | SF | OF);
        if (x != x || y != y) *f |= CF | PF | ZF;
        else if (x < y)       *f |= CF;
        else if (x == y)      *f |= ZF;
        return;
    case SSE_RM_TO_INT:                                 /* cvttss2si: towards zero; a value */
        memset(r, 0, sizeof *r);                        /* that does not fit gives 80..0 */
        if (isz == 8)
            r->q[0] = y >= -0x1p63f && y < 0x1p63f ? (uint64_t)(int64_t)y : 1ULL << 63;
        else
            r->q[0] = y >= -0x1p31f && y < 0x1p31f ? (uint32_t)(int32_t)y : 1u << 31;
        return;
    case SSE_REG_TO_INT:                                /* movd, movq */
        memset(r, 0, sizeof *r);
        r->q[0] = isz == 8 ? b->q[0] : b->d[0];
        return;
    case SSE_INT_TO_REG:
        *r = *a;
        if (op == 0x2a) {                               /* cvtsi2ss */
            r->f[0] = isz == 8 ? (float)(int64_t)b->q[0] : (float)(int32_t)b->q[0];
        } else {                                        /* movd, movq */
            r->q[0] = isz == 8 ? b->q[0] : (uint32_t)b->q[0];
            r->q[1] = 0;
        }
        return;
    }

    *r = *a;                                            /* a vector register is written */
    switch (op) {
    case 0x51: r->f[0] = sqrtf(y); break;
    case 0x54: r->q[0] = a->q[0] & b->q[0];  r->q[1] = a->q[1] & b->q[1];  break;
    case 0x55: r->q[0] = ~a->q[0] & b->q[0]; r->q[1] = ~a->q[1] & b->q[1]; break;
    case 0x56: r->q[0] = a->q[0] | b->q[0];  r->q[1] = a->q[1] | b->q[1];  break;
    case 0x57:
    case 0xef: r->q[0] = a->q[0] ^ b->q[0];  r->q[1] = a->q[1] ^ b->q[1];  break;
    case 0x58: r->f[0] = x + y; break;
    case 0x59: r->f[0] = x * y; break;
    case 0x5c: r->f[0] = x - y; break;
    case 0x5d: r->f[0] = x < y ? x : y; break;          /* minss, maxss: the second operand */
    case 0x5e: r->f[0] = x / y; break;                  /* when one of them is a NaN */
    case 0x5f: r->f[0] = x > y ? x : y; break;
    default:                                            /* the moves */
        if (s->width == 16) {
            r->q[0] = b->q[0];
            r->q[1] = b->q[1];
        } else if (s->width == 8) {                     /* movq */
            r->q[0] = b->q[0];
            r->q[1] = 0;
        } else {                                        /* movss */
            r->d[0] = b->d[0];
            if (mem) {
                r->d[1] = 0;
                r->q[1] = 0;
            }
        }
        break;
    }
}

void vex_compute(unsigned op, int vlen, unsigned imm, const u256 *a, const u256 *b,
                 u256 *r)
{
    u256 t;

    memset(&t, 0, sizeof t);
    switch (op) {
    case 0x11db:                                        /* vpand */
        for (int i = 0; i < vlen / 8; ++i)
            t.q[i] = a->q[i] & b->q[i];
        break;
    case 0x1172:                                        /* vpsrld: a count above 31 gives 0 */
        for (int i = 0; i < vlen / 4; ++i)
            t.d[i] = imm > 31 ? 0 : b->d[i] >> imm;
        break;
    default:                                            /* vpmovzxbd: byte i becomes lane i */
        for (int i = 0; i < vlen / 4; ++i)
            t.d[i] = b->b[i];
        break;
    }
    *r = t;
}
