/* Values of x86 arithmetic: truncation, sign extension, the eight ALU
 * operations by number and the two rotations, shared by the interpreter
 * of lab 1 and the CPU of lab 2.
 *
 * Every other result and every flag of integer arithmetic comes from
 * src/lab1/alu.c: add, sub and the logic operations through x86_alu here,
 * inc, dec, neg, the shifts, multiplication and division through direct
 * calls in frame/x86rest.c and frame/compute.c.
 */
#include "mcpu.h"

uint64_t trunc_val(uint64_t v, int size)
{
    switch (size) {
    case 1:  return v & 0xff;
    case 2:  return v & 0xffff;
    case 4:  return v & 0xffffffffu;
    default: return v;
    }
}

int64_t sext(uint64_t v, int size)
{
    switch (size) {
    case 1:  return (int8_t)v;
    case 2:  return (int16_t)v;
    case 4:  return (int32_t)v;
    default: return (int64_t)v;
    }
}

uint64_t x86_alu(uint64_t *f, int aop, uint64_t a, uint64_t b, int size)
{
    int cf = !!(*f & CF);
    switch (aop & 7) {
    case 0:  return x86_add(f, a, b, size, 0);
    case 1:  return x86_logic(f, a | b, size);
    case 2:  return x86_add(f, a, b, size, cf);
    case 3:  return x86_sub(f, a, b, size, cf);
    case 4:  return x86_logic(f, a & b, size);
    case 5:  return x86_sub(f, a, b, size, 0);
    case 6:  return x86_logic(f, a ^ b, size);
    default: return x86_sub(f, a, b, size, 0);          /* cmp */
    }
}

uint64_t x86_rotate(uint64_t *f, int sub, uint64_t v, unsigned n, int size)
{
    unsigned bits = (unsigned)size * 8;
    uint64_t r;
    n &= size == 8 ? 63 : 31;
    v = trunc_val(v, size);
    if (!n)
        return v;                                       /* flags unchanged */
    n %= bits;
    if (sub == 0) {                                     /* rol */
        r = trunc_val((v << n) | (n ? v >> (bits - n) : 0), size);
        *f = flag(*f, CF, r & 1);
        *f = flag(*f, OF, (!!(r & msb(size))) ^ (int)(r & 1));
    } else {                                            /* ror */
        r = trunc_val((v >> n) | (n ? v << (bits - n) : 0), size);
        *f = flag(*f, CF, !!(r & msb(size)));
        *f = flag(*f, OF, !!((r ^ (r << 1)) & msb(size)));
    }
    return r;
}
