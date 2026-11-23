/* Hardware: values of x86 arithmetic for the interpreter of lab 1,
 * truncation, sign extension, the eight ALU operations by number and the
 * two rotations.
 *
 * Every other result and every flag of integer arithmetic comes from
 * src/lab1/alu.c: add, sub and the logic operations through x86_alu here,
 * inc, dec, neg, the shifts, multiplication and division through direct
 * calls in frame/cpu/x86rest.c.
 */
#include "mcpu.h"

uint64_t trunc_val(uint64_t value, int size)
{
    switch (size) {
    case 1:  return value & 0xff;
    case 2:  return value & 0xffff;
    case 4:  return value & 0xffffffffu;
    default: return value;
    }
}

int64_t sext(uint64_t value, int size)
{
    switch (size) {
    case 1:  return (int8_t)value;
    case 2:  return (int16_t)value;
    case 4:  return (int32_t)value;
    default: return (int64_t)value;
    }
}

uint64_t x86_alu(uint64_t *flags, int aop, uint64_t dest, uint64_t src,
                 int size)
{
    int cf = !!(*flags & CF);
    switch (aop & 7) {
    case 0:  return x86_add(flags, dest, src, size, 0);
    case 1:  return x86_logic(flags, dest | src, size);
    case 2:  return x86_add(flags, dest, src, size, cf);
    case 3:  return x86_sub(flags, dest, src, size, cf);
    case 4:  return x86_logic(flags, dest & src, size);
    case 5:  return x86_sub(flags, dest, src, size, 0);
    case 6:  return x86_logic(flags, dest ^ src, size);
    default: return x86_sub(flags, dest, src, size, 0); /* cmp */
    }
}

uint64_t x86_rotate(uint64_t *flags, int sub, uint64_t value, unsigned count,
                    int size)
{
    unsigned bits = (unsigned)size * 8;
    uint64_t result;
    count &= size == 8 ? 63 : 31;
    value = trunc_val(value, size);
    if (!count)
        return value;                                   /* flags unchanged */
    count %= bits;
    if (sub == 0) {                                     /* rol */
        result = trunc_val((value << count) |
                           (count ? value >> (bits - count) : 0), size);
        *flags = flag(*flags, CF, result & 1);
        *flags = flag(*flags, OF, (!!(result & msb(size))) ^ (int)(result & 1));
    } else {                                            /* ror */
        result = trunc_val((value >> count) |
                           (count ? value << (bits - count) : 0), size);
        *flags = flag(*flags, CF, !!(result & msb(size)));
        *flags = flag(*flags, OF, !!((result ^ (result << 1)) & msb(size)));
    }
    return result;
}
