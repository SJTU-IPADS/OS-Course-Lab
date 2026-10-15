/* Lab 1: the results and flags of the ALU operations.
 *
 * rflags keeps five status flags of the last operation that sets them:
 *
 *   CF  bit 0    carry out of the top bit (add), borrow into it (sub)
 *   PF  bit 2    the low 8 bits of the result have an even number of ones
 *   ZF  bit 6    the result is zero
 *   SF  bit 7    the top bit of the result
 *   OF  bit 11   the result as a signed number is wrong: it overflowed
 *
 * The top bit and "zero" refer to the operand size (1, 2, 4 or 8 bytes),
 * never to all 64 bits. A function here changes only these five bits of
 * the flags. mcpu.h has the contract of each function and five helpers:
 *
 *   trunc_val(v, size)   the low size bytes of v
 *   sext(v, size)        v sign-extended from size bytes, as an int64_t
 *   msb(size)            the top bit of a size-byte value
 *   parity(v)            1 if the low 8 bits of v have an even number of ones
 *   flag(f, m, on)       f with the bits m set (on != 0) or cleared
 *
 * The functions build on one another: x86_add, x86_sub and x86_logic take
 * ZF, SF and PF from x86_szp; x86_inc, x86_dec and x86_neg are x86_add and
 * x86_sub with one operand fixed; x86_shift and x86_mul compute CF and OF
 * themselves and take the other three flags from x86_szp.
 *
 * Four of them are written out as examples: x86_add, x86_inc, x86_div,
 * and in x86_shift the branch of shl and sal (the default of the switch).
 * You write the other six and the shr and sar cases of x86_shift.
 *
 * The interpreter of this lab and the CPU of lab 2 call them for every
 * integer instruction: the first four through x86_alu (frame/arith.c),
 * which picks one by the number of the operation, the other six directly
 * (frame/x86rest.c, frame/compute.c).
 */
#include "mcpu.h"

uint64_t x86_szp(uint64_t f, uint64_t r, int size)
{
    /* TODO: return f with ZF, SF and PF set from the size-byte result r;
     * every other bit of f stays as it is.
     *
     *   ZF  the low size bytes of r are all zero
     *   SF  the top bit of those bytes
     *   PF  parity of the low 8 bits
     *
     * Examples, size 1:
     *   r = 0x100    ZF PF       (the low byte is 0)
     *   r = 0x80     SF          (one bit set: odd parity)
     *   r = 0x03     PF
     */
    (void)f; (void)r; (void)size;
    todo("x86_szp");
}

uint64_t x86_logic(uint64_t *f, uint64_t r, int size)
{
    /* TODO: r is the result of and, or, xor or test. Set ZF, SF and PF
     * from it, clear CF and OF, and return r truncated to size bytes.
     */
    (void)f; (void)r; (void)size;
    todo("x86_logic");
}

uint64_t x86_add(uint64_t *f, uint64_t a, uint64_t b, int size, int carry)
{
    /* r = a + b + carry on size bytes; set the five flags in *f and
     * return r truncated to size bytes. Truncate a and b first.
     *
     *   ZF SF PF  x86_szp
     *   CF        the true sum, as unsigned numbers, does not fit in size
     *             bytes. With size 8 the sum wraps in uint64_t: compare r
     *             with an operand. The sum can wrap to exactly a when b is
     *             all ones and carry is 1.
     *   OF        a and b have the same top bit and r has the other one
     *
     * Examples, size 1, carry 0:
     *   0xff + 0x01 = 0x00   CF ZF PF
     *   0x7f + 0x01 = 0x80   OF SF
     *   0x80 + 0x80 = 0x00   CF OF ZF PF
     * and with carry 1:
     *   0x00 + 0xff = 0x00   CF ZF PF
     */
    uint64_t ta = trunc_val(a, size), tb = trunc_val(b, size);
    uint64_t r = trunc_val(ta + tb + (uint64_t)carry, size);
    *f = x86_szp(*f, r, size);
    *f = flag(*f, CF, r < ta || (carry && r == ta));
    *f = flag(*f, OF, (~(ta ^ tb) & (ta ^ r) & msb(size)) != 0);
    return r;
}

uint64_t x86_sub(uint64_t *f, uint64_t a, uint64_t b, int size, int borrow)
{
    /* TODO: r = a - b - borrow on size bytes; set the five flags in *f and
     * return r truncated to size bytes. Truncate a and b first.
     *
     *   ZF SF PF  x86_szp
     *   CF        the subtraction borrows: a < b + borrow as unsigned
     *             numbers (b + borrow can itself wrap when b is all ones)
     *   OF        a and b have different top bits and r has the top bit
     *             of b
     *
     * Examples, size 1, borrow 0:
     *   0x00 - 0x01 = 0xff   CF SF PF
     *   0x80 - 0x01 = 0x7f   OF
     *   0x05 - 0x05 = 0x00   ZF PF
     * and with borrow 1:
     *   0x05 - 0xff = 0x05   CF PF
     */
    (void)f; (void)a; (void)b; (void)size; (void)borrow;
    todo("x86_sub");
}

uint64_t x86_inc(uint64_t *f, uint64_t v, int size)
{
    /* v + 1 on size bytes. inc sets ZF, SF, PF and OF as add does
     * and leaves CF as it was: call x86_add, then put the CF of before the
     * call back into *f.
     *
     * Examples, size 1:
     *   0xff + 1 = 0x00   ZF PF, CF as before (add would set it)
     *   0x7f + 1 = 0x80   OF SF, CF as before
     */
    uint64_t cf = *f & CF;
    uint64_t r = x86_add(f, v, 1, size, 0);
    *f = (*f & ~(uint64_t)CF) | cf;
    return r;
}

uint64_t x86_dec(uint64_t *f, uint64_t v, int size)
{
    /* TODO: v - 1 on size bytes, with x86_sub; CF as it was, as in x86_inc.
     *
     * Examples, size 1:
     *   0x00 - 1 = 0xff   SF PF, CF as before (sub would set it)
     *   0x80 - 1 = 0x7f   OF, CF as before
     */
    (void)f; (void)v; (void)size;
    todo("x86_dec");
}

uint64_t x86_neg(uint64_t *f, uint64_t v, int size)
{
    /* TODO: 0 - v on size bytes: the result and all five flags of x86_sub.
     *
     * Examples, size 1:
     *   0 - 0x01 = 0xff   CF SF PF
     *   0 - 0x00 = 0x00   ZF PF
     *   0 - 0x80 = 0x80   CF OF SF
     */
    (void)f; (void)v; (void)size;
    todo("x86_neg");
}

uint64_t x86_shift(uint64_t *f, int sub, uint64_t v, unsigned n, int size)
{
    /* v shifted by n bits on size bytes; sub is 4 shl, 5 shr,
     * 6 sal (the same as shl), 7 sar.
     *
     *   1. Keep the low 6 bits of n when size is 8, the low 5 bits
     *      otherwise, and truncate v to size bytes.
     *   2. If n is now 0, return v: no flag changes.
     *   3. Compute the result r, the last bit shifted out (CF) and OF:
     *
     *        shl  r = v << n          CF = bit (size * 8 - n) of v    OF = top bit of r != CF
     *        shr  r = v >> n          CF = bit (n - 1) of v           OF = top bit of v
     *        sar  r = v >> n, signed  CF = bit (n - 1) of v, signed   OF = 0
     *
     *      "signed" is v sign-extended from size bytes: sext(v, size).
     *      On 1 and 2 bytes n can exceed the width of the operand: shl
     *      then shifts out a 0 last, so CF is 0, and bit (n - 1) of the
     *      truncated v, the CF of shr, is 0 as well.
     *   4. ZF, SF, PF from x86_szp; return r truncated to size bytes.
     *
     * Examples, size 1:
     *   shl 0x81 by 1 = 0x02   CF OF
     *   shl 0xc0 by 1 = 0x80   CF SF
     *   shr 0x81 by 1 = 0x40   CF OF
     *   sar 0x81 by 1 = 0xc0   CF SF PF
     *   sar 0x81 by 9 = 0xff   CF SF PF
     *   shl 0x01 by 9 = 0x00   ZF PF
     */
    unsigned bits = (unsigned)size * 8;
    n &= size == 8 ? 63 : 31;
    v = trunc_val(v, size);
    if (!n)
        return v;                                       /* flags unchanged */
    uint64_t r;
    int cf, of;
    switch (sub) {
    case 5:                                             /* shr */
        /* TODO: cf, r and of of shr, by the table of step 3 */
        todo("shr in x86_shift");
    case 7:                                             /* sar */
        /* TODO: cf, r and of of sar, by the table of step 3 */
        todo("sar in x86_shift");
    default:                                            /* shl, sal */
        cf = n <= bits ? (int)(v >> (bits - n)) & 1 : 0;
        r = trunc_val(v << n, size);
        of = !!(r & msb(size)) ^ cf;
        break;
    }
    *f = x86_szp(*f, r, size);
    *f = flag(*f, CF, cf);
    *f = flag(*f, OF, of);
    return r;
}

void x86_mul(uint64_t *f, int sub, uint64_t a, uint64_t b, int size,
             uint64_t *lo, uint64_t *hi)
{
    /* TODO: the product of the low size bytes of a and b, 2 * size bytes
     * long; sub is 4 mul (unsigned), 5 imul (signed).
     *
     *   1. Extend a and b to the full product width: zero-extended for
     *      mul (trunc_val), sign-extended for imul (sext). The product of
     *      two 8-byte operands needs unsigned __int128 or __int128.
     *   2. *lo = the lower size bytes of the product, *hi = the upper size
     *      bytes, both truncated to size bytes.
     *   3. CF and OF: both 1 when the product does not fit in size bytes,
     *      both 0 when it does.
     *        mul   *hi is not zero
     *        imul  the product is not *lo sign-extended
     *   4. ZF, SF, PF from x86_szp on *lo.
     *
     * Examples, size 1:
     *   mul   0x10 * 0x10 = 0x0100   lo 0x00, hi 0x01   CF OF ZF PF
     *   mul   0xff * 0x01 = 0x00ff   lo 0xff, hi 0x00   SF PF
     *   imul  0xff * 0x01 = 0xffff   lo 0xff, hi 0xff   SF PF      (-1 * 1 = -1)
     *   imul  0x40 * 0x02 = 0x0080   lo 0x80, hi 0x00   CF OF SF   (128 is not an int8_t)
     */
    (void)f; (void)sub; (void)a; (void)b; (void)size; (void)lo; (void)hi;
    todo("x86_mul");
}

int x86_div(int sub, uint64_t hi, uint64_t lo, uint64_t v, int size,
            uint64_t *q, uint64_t *r)
{
    /* the dividend hi:lo, 2 * size bytes long, divided by the low
     * size bytes of v; sub is 6 div (unsigned), 7 idiv (signed). No flag
     * changes.
     *
     *   1. Truncate hi, lo and v to size bytes. If v is 0, return -1.
     *   2. Put the dividend together: hi shifted left by size * 8 bits,
     *      or lo. With size 8 it needs unsigned __int128. For idiv, read
     *      the dividend as a signed number of 2 * size bytes and v as a
     *      signed number of size bytes.
     *   3. Divide. The / and % of C round as idiv does: the quotient
     *      toward zero, the remainder with the sign of the dividend.
     *      Divide in a type wider than the dividend where there is one
     *      (__int128 holds every dividend): in a type of exactly its
     *      width, the most negative dividend divided by -1 overflows in C
     *      too, and the host CPU reports its own divide error. With size 8
     *      there is no wider type; return -1 for that one case before
     *      dividing.
     *   4. If the quotient does not fit in size bytes (unsigned for div,
     *      signed for idiv), return -1.
     *   5. *q = quotient, *r = remainder, both truncated to size bytes;
     *      return 0.
     *
     * Examples, size 1:
     *   div   0x0107 / 0x10      q 0x10, r 0x07
     *   div   0x0100 / 0x01      -1: the quotient 0x100 needs 9 bits
     *   idiv  0xfff9 / 0x02      q 0xfd, r 0xff   (-7 / 2 = -3, remainder -1)
     *   idiv  0xff80 / 0xff      -1: -128 / -1 = 128 is not an int8_t
     */
    int bits = size * 8;
    v = trunc_val(v, size);
    if (v == 0)
        return -1;
    if (sub == 6) {
        unsigned __int128 nn = ((unsigned __int128)trunc_val(hi, size) << bits) | trunc_val(lo, size);
        unsigned __int128 qq = nn / v;
        if (qq > trunc_val(~0ULL, size))
            return -1;
        *q = (uint64_t)qq;
        *r = (uint64_t)(nn % v);
    } else {
        __int128 nn = size == 8
            ? (__int128)(((unsigned __int128)hi << 64) | lo)
            : (__int128)sext((trunc_val(hi, size) << bits) | trunc_val(lo, size), size * 2);
        __int128 dv = sext(v, size);
        if (dv == -1 && nn == (__int128)((unsigned __int128)1 << 127))
            return -1;                                  /* the quotient is 2^127 */
        __int128 qq = nn / dv;
        if (qq != (__int128)sext((uint64_t)qq, size))
            return -1;
        *q = trunc_val((uint64_t)qq, size);
        *r = trunc_val((uint64_t)(nn % dv), size);
    }
    return 0;
}
