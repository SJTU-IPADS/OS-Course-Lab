/* Lab 1: the ModRM byte, with its SIB byte and displacement.
 *
 * An instruction with operands names them in the byte after the opcode:
 *
 *     ModRM   bit  7 6    5 4 3    2 1 0
 *                  mod    reg      rm
 *
 *   reg     a register operand, or three more opcode bits (m->sub)
 *   mod 11  the other operand is the register rm
 *   else    the other operand is in memory; rm names the base register,
 *           with two exceptions:
 *             rm 100           a SIB byte follows and gives base and index
 *             rm 101, mod 00   no base: 4 bytes of displacement follow, and
 *                              the address is relative to the next
 *                              instruction (%rip)
 *
 *     SIB     bit  7 6    5 4 3    2 1 0
 *                  scale  index    base
 *
 *   scale   0, 1, 2, 3 for a factor of 1, 2, 4, 8
 *   index   register number 100 (after REX.X is added) means no index
 *   base    field 101 with mod 00 means no base: 4 bytes of displacement
 *
 *   displacement   mod 01: 1 byte; mod 10: 4 bytes; mod 00: none, except
 *                  the two cases without a base; always sign-extended
 *
 * A REX prefix adds a fourth bit to three of the fields: REX.R (bit 2 of
 * rex) to reg, REX.X (bit 1) to index, REX.B (bit 0) to rm and to base.
 * The exceptions above test the 3-bit field of rm and of base, before the
 * REX bit is added: with REX.B, rm 100 still has a SIB byte (%r12 as a
 * base needs one), and rm 101 with mod 00 is still %rip. The index is
 * tested after REX.X is added: %r12 is a valid index.
 *
 *     bytes              rex   operand            fields of *m
 *     c3                 0     %ebx (mod 11)      rm 3
 *     00                 0     (%rax)             base 0
 *     40 f8              0     -0x8(%rax)         base 0, dsize 1, disp -8
 *     45 00              0     0x0(%rbp)          base 5, dsize 1, disp 0
 *     80 00 01 00 00     0     0x100(%rax)        base 0, dsize 4
 *     05 10 00 00 00     0     0x10(%rip)         rip 1, dsize 4, disp 0x10
 *     04 8b              0     (%rbx,%rcx,4)      base 3, index 1, scale 4
 *     04 24              0     (%rsp)             base 4, no index
 *     04 20              0x42  (%rax,%r12,1)      base 0, index 12, scale 1
 *     04 c5 10 00 00 00  0     0x10(,%rax,8)      no base, index 0, scale 8
 *
 * mcpu.h has the contract: modrm_t and modrm_decode. Three parts of the
 * machine call this function: the interpreter (frame/x86.c), the
 * disassembler (frame/translate.c) and the decoder of the CPU of lab 2
 * (frame/decode.c).
 */
#include "mcpu.h"

unsigned modrm_decode(const uint8_t *p, unsigned n, uint8_t rex, modrm_t *m)
{
    /* TODO: decode the ModRM byte p[0], then the SIB byte and the
     * displacement if the operand has them, and fill every field of *m.
     *
     *   1. mod, sub and reg from p[0]. With mod 3: rm, and base = index =
     *      -1, scale = 1, rip = dsize = 0, disp = 0; return 1.
     *   2. A memory operand: rm = -1, then base, index, scale and rip from
     *      the rm field, or from the SIB byte when rm is 100. Without an
     *      index, scale is 1.
     *   3. dsize from mod and from whether there is a base; disp from the
     *      next dsize bytes, little endian, sign-extended.
     *   4. Return the number of bytes used.
     *
     * Before reading a byte check that it is one of the n bytes at p;
     * return 0 if it is not.
     */
    (void)p; (void)n; (void)rex; (void)m;
    todo("modrm_decode");
}
