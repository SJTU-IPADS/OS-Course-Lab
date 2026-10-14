/* Lab 1: an interpreter for x86-64.
 *
 * The registers are the array R[]: R[RAX] .. R[R15], R[RFLAGS], R[RIP],
 * and the vector registers Y[0..15]. interp_run() is one loop; every pass
 * runs one instruction:
 *
 *   op = x86_begin(c, &x)     fetch the instruction at R[RIP], skip the
 *                             prefixes, read the opcode; R[RIP] now points
 *                             past the opcode
 *   switch (op)               read the rest of the instruction with fetch8,
 *                             fetch32, fetch_imm and modrm, which all advance
 *                             R[RIP]; compute; write R[] and memory
 *   x86_end(&x)               count the instruction and write the commit log
 *
 * The switch is written in two parts of the lab. Part 3: mov, lea, the
 * eight ALU operations, test, push, pop, call, ret, jmp and jcc. Part 4:
 * seven AVX2 instructions. Every other opcode goes to x86_other() in
 * frame/x86rest.c. Flags are computed by alu.c; cond() decides a
 * condition code from them.
 *
 * The loop is written out, with six cases as examples: the eight
 * operations with an immediate (0x81, 0x83), push, the two jmp, and of
 * part 4 vphaddd and vextracti128. You write cond() and the other cases.
 *
 * When an instruction faults (a page fault in load or store, an illegal
 * opcode), x86_end restores R[] as it was before the instruction, so a
 * case may write registers before a later access fails.
 *
 * mcpu.h has the contract of both functions and of every helper.
 */
#include <string.h>

#include "mcpu.h"

#define R c->R
#define Y c->Y

int cond(uint64_t f, int cc)
{
    /* TODO: return 1 if condition cc & 0xf holds under the flags f, else 0.
     * The flags are the bits CF, PF, ZF, SF and OF of f.
     *
     *   0 o    OF              1 no   !OF
     *   2 b    CF              3 ae   !CF
     *   4 e    ZF              5 ne   !ZF
     *   6 be   CF or ZF        7 a    neither CF nor ZF
     *   8 s    SF              9 ns   !SF
     *   a p    PF              b np   !PF
     *   c l    SF != OF        d ge   SF == OF
     *   e le   ZF or SF != OF  f g    !ZF and SF == OF
     */
    (void)f; (void)cc;
    todo("cond");
}

void interp_run(mcpu *c)
{
    /* The loop and its switch are below; the switch has one case per line
     * of the two lists that follow. Two more cases, as examples of the
     * helpers; the second belongs to part 4:
     *
     *     case 0x89:                              mov: rm = reg
     *         modrm(&x);
     *         rm_set(&x, get(&x, x.reg));
     *         break;
     *
     *     case 0x11ef:                            vpxor: ymm reg = ymm v ^ rm
     *         modrm(&x);
     *         rm_vget(&x, &b, x.vlen);
     *         for (int i = 0; i < 4; ++i)
     *             r.q[i] = Y[x.v].q[i] ^ b.q[i];
     *         vset(&x, x.reg, &r, x.vlen);
     *         break;
     *
     * Part 3: the integer instructions. x86_begin has read the prefixes
     * and the opcode; a case reads the bytes after the opcode, and the
     * opcode alone decides which bytes those are. The list is grouped by
     * them. The name in parentheses is the first program under guest/
     * that runs the instruction. The guests simd, tinytf and
     * mini-ollama-simd also need part 4; no other guest does.
     *
     * Nothing after the opcode. The register of push and pop is
     * (op & 7) | ((x.rex & 1) << 3).
     *   0x50 .. 0x57    push                                    (block)
     *   0x58 .. 0x5f    pop                                     (block)
     *   0xc3            ret                                     (hello)
     *
     * The operand bytes (ModRM, SIB, displacement): call modrm(&x). reg
     * is register x.reg, read and written with get and set; rm is a
     * register or memory, read and written with rm_get and rm_set.
     *   0x89            mov: rm = reg                           (hello)
     *   0x8b            mov: reg = rm                           (hello)
     *   0x8d            lea: reg = ea(&x), the address of rm    (hello)
     *   0x01 0x09 0x11 0x19 0x21 0x29 0x31 0x39
     *                   add or adc sbb and sub xor cmp, in this
     *                   order: rm = rm OP reg; the number of the
     *                   operation is op >> 3                    (hello)
     *   0x03 0x0b 0x13 0x1b 0x23 0x2b 0x33 0x3b
     *                   the same eight: reg = reg OP rm         (matmul)
     *   0x85            test: the flags of rm & reg; no operand
     *                   is written                              (hello)
     *
     * The operand bytes, then an immediate: call modrm(&x), read the
     * immediate, and only then call rm_get and rm_set. rm is the only
     * operand; x.sub holds the reg field of the operand bytes.
     *   0xc7            mov: rm = imm, imm = fetch_imm(&x)      (block)
     *   0x81            the eight operations: rm = rm OP imm,
     *                   imm = fetch_imm(&x); the number of the
     *                   operation is x.sub                      (deep)
     *   0x83            as 0x81 with imm = (int8_t)fetch8(&x)   (hello)
     *
     * An immediate. The register is numbered as in push.
     *   0xb8 .. 0xbf    mov: register = imm; imm = fetch64(&x)
     *                   when x.size is 8, else fetch_imm(&x)    (hello)
     *
     * A relative offset rel, signed. Once it is read, R[RIP] is the
     * address of the next instruction and the jump is R[RIP] += rel.
     *   0xe8            call: rel = (int32_t)fetch32(&x); push
     *                   R[RIP], then jump                       (hello)
     *   0xeb            jmp: rel = (int8_t)fetch8(&x)           (hello)
     *   0xe9            jmp: rel = (int32_t)fetch32(&x)         (block)
     *   0x70 .. 0x7f    jcc: rel = (int8_t)fetch8(&x); jump
     *                   when cond(R[RFLAGS], op)                (hello)
     *   0x180 .. 0x18f  jcc, the bytes 0f 80 .. 0f 8f:
     *                   rel = (int32_t)fetch32(&x)              (hello)
     *
     * Part 4: the vector subset, the seven instructions of linear_simd in
     * guest/linear.s, which guest/simd and guest/tinytf run; dot_q4 of
     * guest/mini-ollama-simd uses five of them. Until a case exists, the
     * instruction goes to x86_other() and is reported as illegal. A VEX
     * opcode is 0x1000 | map << 8 | byte, map 1 for 0f, 2 for 0f38, 3 for
     * 0f3a; x.v is the register in VEX.vvvv, x.vlen the width in bytes, 16
     * for xmm and 32 for ymm. Integer lanes are 32 bits: Y[n].d[i].
     *
     *   0x11ef  vpxor   reg = v ^ rm
     *   0x116f  vmovdqu reg <- rm (register or memory)
     *   0x11fe  vpaddd  reg = v + rm, lane by lane, vlen / 4 lanes
     *   0x1240  vpmulld reg = v * rm, low 32 bits of each product
     *   0x1202  vphaddd xmm only: reg = { v0+v1, v2+v3, rm0+rm1, rm2+rm3 }
     *   0x1339  vextracti128 xmm rm (or 16 bytes of memory) <- the half
     *           of ymm reg selected by bit 0 of an 8-bit immediate: 0 low,
     *           1 high
     *   0x117e  vmovd   32-bit rm (register or memory) <- lane 0 of
     *           xmm reg
     *
     * Three rules that hold for every case:
     *   - A relative offset counts from the next instruction: read it
     *     first, then add it to R[RIP]. For the same reason read an
     *     immediate before rm_get, rm_set or ea, which add R[RIP] to a
     *     rip-relative address.
     *   - cmp (operation 7) computes the flags but writes no operand.
     *   - pop reads the stack, then moves R[RSP], then writes the
     *     register, so that pop %rsp ends with the loaded value.
     *
     * An opcode not handled here goes to x86_other(), which treats the
     * opcodes of this list as illegal: the run stops with a register dump
     * whose rip is the address of the instruction, which
     * mini-cpu --disasm and objdump -d show.
     */
    x86 x;
    uint64_t v;
    int64_t rel;
    u256 a, b, r;

    while (!c->halted && !(opt_max && c->icount >= opt_max)) {
        unsigned op = x86_begin(c, &x);

        switch (op) {
        /* the eight operations: rm = rm OP imm; the number of the
           operation is x.sub, and cmp (7) writes no operand */
        case 0x81: case 0x83:
            modrm(&x);
            v = op == 0x83 ? (uint64_t)(int64_t)(int8_t)fetch8(&x) : fetch_imm(&x);
            v = x86_alu(&R[RFLAGS], x.sub, rm_get(&x), v, x.size);
            if (x.sub != 7)
                rm_set(&x, v);
            break;

        /* push: the register is (op & 7) | ((x.rex & 1) << 3) */
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
            v = R[(op & 7) | ((x.rex & 1) << 3)];
            store(&x, R[RSP] - 8, 8, v);
            R[RSP] -= 8;
            break;

        /* jmp: a 1-byte offset (0xeb) or a 4-byte offset (0xe9) */
        case 0xeb:
            rel = (int8_t)fetch8(&x);
            R[RIP] += (uint64_t)rel;
            break;
        case 0xe9:
            rel = (int32_t)fetch32(&x);
            R[RIP] += (uint64_t)rel;
            break;

        /* TODO: part 3, the other cases of the integer list above */

        /* vphaddd, xmm only: reg = { v0+v1, v2+v3, rm0+rm1, rm2+rm3 } */
        case 0x1202:
            modrm(&x);
            rm_vget(&x, &b, 16);
            a = Y[x.v];
            memset(&r, 0, sizeof r);
            r.d[0] = a.d[0] + a.d[1];
            r.d[1] = a.d[2] + a.d[3];
            r.d[2] = b.d[0] + b.d[1];
            r.d[3] = b.d[2] + b.d[3];
            vset(&x, x.reg, &r, 16);
            break;

        /* vextracti128: xmm rm (or 16 bytes of memory) <- the low (0) or
           high (1) half of ymm reg, by bit 0 of the immediate */
        case 0x1339:
            modrm(&x);
            v = fetch8(&x);
            memset(&r, 0, sizeof r);
            r.q[0] = Y[x.reg].q[(v & 1) * 2];
            r.q[1] = Y[x.reg].q[(v & 1) * 2 + 1];
            rm_vset(&x, &r, 16);
            break;

        /* TODO: part 4, the other five vector instructions */

        default:
            x86_other(&x);
            break;
        }

        if (x86_end(&x) < 0)
            break;
    }
}
