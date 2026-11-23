/* Neither hardware nor software of the machine: a tool of the host,
 * machine code back into assembly, the disassembler of mini-cpu --disasm.
 *
 * translate() reads one instruction from a buffer of bytes and writes it in
 * the AT&T syntax that objdump -d prints, in the order the bytes come:
 *
 *   1. prefixes     66 (operand size 16), f2 f3, segment prefixes, then REX;
 *                   or a VEX prefix (c4, c5), which carries REX bits itself.
 *                   Before an SSE opcode, 66 and f3 select the instruction
 *                   (the table sse_ops[] of frame/cpu/simd.c)
 *   2. opcode       one byte, or 0f and a second byte
 *   3. the switch   the opcode decides the operands: whether a ModRM byte,
 *                   an immediate or a relative offset follows
 *   4. ModRM        the register operand and the r/m operand; for a memory
 *                   operand also a SIB byte and a displacement, if present.
 *                   This step is modrm_decode of src/lab1/modrm.c.
 *   5. immediate    or the relative offset of a jump or call
 *
 * The bytes are only read: no register or memory of the machine is involved.
 * mini-cpu --disasm FILE (frame/disasm.c) prints the code of an ELF file
 * with it, in the format of objdump -d.
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "mcpu.h"

/* The instruction being translated. */
typedef struct {
    uint64_t       pc;
    const uint8_t *buf;
    unsigned       nbytes;          /* bytes in buf */
    unsigned       pos;             /* bytes read so far */
    int            bad;             /* ran past nbytes, or an unknown encoding */
    int            size;            /* operand size in bytes: 1, 2, 4 or 8 */
    uint8_t        rex;             /* REX prefix, 0 if none; the bits of VEX */
    int            n66, rep, repne; /* prefixes seen */
    const char    *seg;             /* fs or gs before a memory operand, NULL */
    int            vex, vlen, vvvv; /* VEX: its prefix, 16 or 32, VEX.vvvv */
    int            mod;             /* ModRM mod field */
    int            reg;             /* ModRM reg field with REX.R */
    int            sub;             /* ModRM reg field alone, 0..7 */
    int            rm;              /* ModRM rm register with REX.B, -1 for memory */
    modrm_t        modrm;           /* the ModRM byte; the memory operand when rm is -1 */
    char           ops[3][48];      /* operands in AT&T order, source first */
    int            nops;
    int            memop_at;        /* index in ops of the memory operand, -1 */
} dis;

/* ------------------------------------------------------------- bytes */

/* count bytes, little endian. Reading past the buffer marks the instruction bad. */
static uint64_t take(dis *inst, unsigned count)
{
    uint64_t value = 0;
    if (inst->pos + count > inst->nbytes || inst->pos + count > MAX_INSN)
        inst->bad = 1;
    else
        memcpy(&value, inst->buf + inst->pos, count);
    inst->pos += count;
    return value;
}

static int64_t take_s8(dis *inst)  { return (int8_t)take(inst, 1); }
static int64_t take_s32(dis *inst) { return (int32_t)take(inst, 4); }

/* An immediate of the operand size, at most 4 bytes, sign-extended. */
static int64_t take_imm(dis *inst)
{
    switch (inst->size) {
    case 1:  return (int8_t)take(inst, 1);
    case 2:  return (int16_t)take(inst, 2);
    default: return (int32_t)take(inst, 4);
    }
}

/* ---------------------------------------------------------- operands */

static void op(dis *inst, const char *fmt, ...)
{
    va_list ap;
    if (inst->nops == 3) { inst->bad = 1; return; }
    va_start(ap, fmt);
    vsnprintf(inst->ops[inst->nops++], sizeof inst->ops[0], fmt, ap);
    va_end(ap);
}

/* The name of register reg at size bytes; 16 and 32 name xmm reg and ymm reg. */
static const char *regname(const dis *inst, int reg, int size)
{
    static const char *const name64[16] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
    static const char *const name8_rex[8] = {
        "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil" };
    static const char *const name8_norex[8] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
    static char text[16];
    switch (size) {
    case 8:  return name64[reg];
    case 16: snprintf(text, sizeof text, "xmm%d", reg); return text;
    case 32: snprintf(text, sizeof text, "ymm%d", reg); return text;
    }
    if (reg >= 8) {
        snprintf(text, sizeof text, "%s%s", name64[reg],
                 size == 4 ? "d" : size == 2 ? "w" : "b");
        return text;
    }
    if (size == 4) { snprintf(text, sizeof text, "e%s", name64[reg] + 1); return text; }
    if (size == 2) return name64[reg] + 1;
    return inst->rex ? name8_rex[reg] : name8_norex[reg];
}

static void op_reg(dis *inst, int reg, int size)
{
    op(inst, "%%%s", regname(inst, reg, size));
}

/* The r/m operand: register rm at size bytes, or the memory operand, which
   is written out when the length of the instruction is known. */
static void op_rm(dis *inst, int size)
{
    if (inst->rm >= 0) {
        op_reg(inst, inst->rm, size);
    } else {
        inst->memop_at = inst->nops;
        op(inst, "");
    }
}

/* An immediate, shown at the operand size as objdump shows it. */
static void op_imm(dis *inst, int64_t value, int size)
{
    uint64_t truncated = trunc_val((uint64_t)value, size);
    op(inst, "$0x%llx", (unsigned long long)truncated);
}

/* ------------------------------------------------------ ModRM and SIB */

/* The ModRM byte, with its SIB byte and displacement: modrm_decode of
   lab 1 on the bytes that are left. */
static void read_modrm(dis *inst)
{
    unsigned end = inst->nbytes < MAX_INSN ? inst->nbytes : MAX_INSN;
    unsigned used = inst->pos < end ? modrm_decode(inst->buf + inst->pos, end - inst->pos,
                                                   inst->rex, &inst->modrm) : 0;
    if (used == 0 || used > end - inst->pos) {
        inst->bad = 1;
        inst->rm = -1;
        memset(&inst->modrm, 0, sizeof inst->modrm);
        inst->modrm.base = inst->modrm.index = -1;
        inst->pos += 1;
        return;
    }
    inst->pos += used;
    inst->mod = inst->modrm.mod;
    inst->sub = inst->modrm.sub;
    inst->reg = inst->modrm.reg;
    inst->rm = inst->modrm.rm;
}

/* A signed number in hex: -0x10, 0x0. */
static int signed_hex(char *out, size_t size, int64_t value)
{
    if (value < 0)
        return snprintf(out, size, "-0x%llx", (unsigned long long)-(uint64_t)value);
    return snprintf(out, size, "0x%llx", (unsigned long long)value);
}

/* The memory operand in AT&T syntax. */
static void memtext(const dis *inst, char *out, size_t size)
{
    const modrm_t *mem = &inst->modrm;
    int len = 0;
    if (inst->seg) {
        len = snprintf(out, size, "%%%s:", inst->seg);
        out += len; size -= (size_t)len; len = 0;
    }
    if (mem->rip) {
        len = signed_hex(out, size, mem->disp);
        snprintf(out + len, size - (size_t)len, "(%%rip)");
        return;
    }
    if (mem->base < 0 && mem->index < 0) {          /* an absolute address */
        snprintf(out, size, "0x%llx", (unsigned long long)mem->disp);
        return;
    }
    if (mem->dsize)
        len = signed_hex(out, size, mem->disp);
    len += snprintf(out + len, size - (size_t)len, "(");
    if (mem->base >= 0)
        len += snprintf(out + len, size - (size_t)len, "%%%s", regname(inst, mem->base, 8));
    if (mem->index >= 0)
        len += snprintf(out + len, size - (size_t)len, ",%%%s,%d",
                        regname(inst, mem->index, 8), mem->scale);
    snprintf(out + len, size - (size_t)len, ")");
}

/* ------------------------------------------------------------ opcodes */

static const char *const alu_name[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
static const char *const cc_name[16] = { "o", "no", "b", "ae", "e", "ne", "be", "a",
                                         "s", "ns", "p", "np", "l", "ge", "le", "g" };
static const char *const shift_name[8] = { "rol", "ror", "rcl", "rcr", "shl", "shr", NULL, "sar" };
static const char *const grp3_name[8] = { "test", NULL, "not", "neg", "mul", "imul", "div", "idiv" };

/* The size suffix AT&T syntax adds when no register operand gives the size. */
static char suffix(int size)
{
    return size == 1 ? 'b' : size == 2 ? 'w' : size == 4 ? 'l' : 'q';
}

/* Mnemonic flags. */
enum {
    SUF  = 1,                       /* add the size suffix to a memory-only operation */
    STAR = 2,                       /* an indirect jump or call: * before the operand */
};

/* A jump or call to the address after the instruction plus rel. */
static void op_target(dis *inst, int64_t rel)
{
    op(inst, "%llx", (unsigned long long)(inst->pc + inst->pos + (uint64_t)rel));
}

/* The instructions with a VEX prefix. Returns the mnemonic, NULL if unknown. */
static const char *vex_insn(dis *inst, unsigned map, unsigned pp, unsigned opcode)
{
    int vlen = inst->vlen;
    if (map == 1 && pp == 2 && opcode == 0x6f) {        /* vmovdqu rm, reg */
        read_modrm(inst); op_rm(inst, vlen); op_reg(inst, inst->reg, vlen);
        return "vmovdqu";
    }
    if (map == 1 && pp == 2 && opcode == 0x7f) {        /* vmovdqu reg, rm */
        read_modrm(inst); op_reg(inst, inst->reg, vlen); op_rm(inst, vlen);
        return "vmovdqu";
    }
    if ((map == 1 && pp == 1 && (opcode == 0xef || opcode == 0xfe || opcode == 0xfa ||
                                 opcode == 0xd4 || opcode == 0xdb)) ||
        (map == 2 && pp == 1 && (opcode == 0x40 || opcode == 0x02))) {  /* rm, vvvv, reg */
        read_modrm(inst);
        op_rm(inst, vlen); op_reg(inst, inst->vvvv, vlen); op_reg(inst, inst->reg, vlen);
        switch (map << 8 | opcode) {
        case 0x1ef: return "vpxor";
        case 0x1db: return "vpand";
        case 0x1fe: return "vpaddd";
        case 0x1fa: return "vpsubd";
        case 0x1d4: return "vpaddq";
        case 0x240: return "vpmulld";
        default:    return "vphaddd";
        }
    }
    if (map == 1 && pp == 1 && opcode == 0x72) {        /* vpsrld imm, rm, vvvv: 72 /2 ib */
        read_modrm(inst);
        int64_t imm = (uint8_t)take(inst, 1);
        if (inst->sub != 2 || inst->rm < 0)
            return NULL;
        op_imm(inst, imm, 1); op_reg(inst, inst->rm, vlen); op_reg(inst, inst->vvvv, vlen);
        return "vpsrld";
    }
    if (map == 2 && pp == 1 && opcode == 0x31) {        /* vpmovzxbd xmm/m64, reg */
        read_modrm(inst); op_rm(inst, 16); op_reg(inst, inst->reg, vlen);
        return "vpmovzxbd";
    }
    if (map == 3 && pp == 1 && opcode == 0x39 && vlen == 32) {  /* vextracti128 imm, reg, rm */
        read_modrm(inst);
        int64_t imm = (uint8_t)take(inst, 1);
        op_imm(inst, imm, 1); op_reg(inst, inst->reg, 32); op_rm(inst, 16);
        return "vextracti128";
    }
    if (map == 1 && pp == 1 && opcode == 0x7e && vlen == 16) {  /* vmovd xmm, r/m32 */
        read_modrm(inst); op_reg(inst, inst->reg, 16); op_rm(inst, inst->rex & 8 ? 8 : 4);
        return inst->rex & 8 ? "vmovq" : "vmovd";
    }
    if (map == 1 && pp == 1 && opcode == 0x6e && vlen == 16) {  /* vmovd r/m32, xmm */
        read_modrm(inst); op_rm(inst, inst->rex & 8 ? 8 : 4); op_reg(inst, inst->reg, 16);
        return inst->rex & 8 ? "vmovq" : "vmovd";
    }
    return NULL;
}

/* The instruction after its prefixes. Returns the mnemonic, NULL if
   unknown, and sets *flags. */
static const char *legacy_insn(dis *inst, unsigned opcode, int *flags)
{
    static char name[16];
    int64_t imm;
    unsigned opcode2;

    if (opcode < 0x40 && (opcode & 7) < 6) {            /* ALU: 00 .. 3d */
        switch (opcode & 7) {
        case 0: inst->size = 1; /* fall through */
        case 1: read_modrm(inst);
                op_reg(inst, inst->reg, inst->size); op_rm(inst, inst->size); break;
        case 2: inst->size = 1; /* fall through */
        case 3: read_modrm(inst);
                op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size); break;
        case 4: inst->size = 1; /* fall through */
        case 5: imm = take_imm(inst);
                op_imm(inst, imm, inst->size); op_reg(inst, RAX, inst->size); break;
        }
        return alu_name[opcode >> 3];
    }
    if (opcode >= 0x50 && opcode <= 0x5f) {             /* push r, pop r */
        op_reg(inst, (opcode & 7) | (inst->rex & 1) << 3, 8);
        return opcode < 0x58 ? "push" : "pop";
    }
    if (opcode >= 0x70 && opcode <= 0x7f) {             /* jcc rel8 */
        op_target(inst, take_s8(inst));
        snprintf(name, sizeof name, "j%s", cc_name[opcode & 15]);
        return name;
    }
    if (opcode >= 0xb0 && opcode <= 0xbf) {             /* mov r, imm */
        int reg = (opcode & 7) | (inst->rex & 1) << 3;
        if (opcode < 0xb8) {
            inst->size = 1;
            op_imm(inst, (uint8_t)take(inst, 1), 1);
            op_reg(inst, reg, 1);
            return "mov";
        }
        if (inst->size == 8) {
            op_imm(inst, (int64_t)take(inst, 8), 8);
            op_reg(inst, reg, 8);
            return "movabs";
        }
        op_imm(inst, take_imm(inst), inst->size);
        op_reg(inst, reg, inst->size);
        return "mov";
    }

    switch (opcode) {
    case 0x63:                                          /* movslq r/m32, r64 */
        if (inst->size != 8)
            return NULL;
        read_modrm(inst); op_rm(inst, 4); op_reg(inst, inst->reg, 8);
        return "movslq";
    case 0x68: case 0x6a:                               /* push imm */
        imm = opcode == 0x68 ? take_s32(inst) : take_s8(inst);
        op_imm(inst, imm, 8);
        return "push";
    case 0x69: case 0x6b:                               /* imul imm, r/m, r */
        read_modrm(inst);
        imm = opcode == 0x69 ? take_imm(inst) : take_s8(inst);
        op_imm(inst, imm, inst->size);
        op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size);
        return "imul";
    case 0x80: case 0x81: case 0x83:                    /* ALU imm, r/m */
        if (opcode == 0x80) inst->size = 1;
        read_modrm(inst);
        imm = opcode == 0x81 ? take_imm(inst) : take_s8(inst);
        op_imm(inst, imm, inst->size); op_rm(inst, inst->size);
        *flags = SUF;
        return alu_name[inst->sub];
    case 0x84: case 0x85: case 0x86: case 0x87:         /* test, xchg r, r/m */
    case 0x88: case 0x89:                               /* mov r, r/m */
        if (!(opcode & 1)) inst->size = 1;
        read_modrm(inst); op_reg(inst, inst->reg, inst->size); op_rm(inst, inst->size);
        return opcode < 0x86 ? "test" : opcode < 0x88 ? "xchg" : "mov";
    case 0x8a: case 0x8b:                               /* mov r/m, r */
        if (opcode == 0x8a) inst->size = 1;
        read_modrm(inst); op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size);
        return "mov";
    case 0x8d:                                          /* lea m, r */
        read_modrm(inst);
        if (inst->rm >= 0)
            return NULL;
        op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size);
        return "lea";
    case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x90:
        if (opcode != 0x90 || inst->rex & 1) {          /* xchg rax, r */
            op_reg(inst, RAX, inst->size);
            op_reg(inst, (opcode & 7) | (inst->rex & 1) << 3, inst->size);
            return "xchg";
        }
        if (inst->rep) { inst->rep = 0; return "pause"; }
        if (inst->size == 2) {
            op_reg(inst, RAX, 2); op_reg(inst, RAX, 2);
            return "xchg";
        }
        return "nop";
    case 0x98: return inst->size == 8 ? "cltq" : inst->size == 4 ? "cwtl" : "cbtw";
    case 0x99: return inst->size == 8 ? "cqto" : inst->size == 4 ? "cltd" : "cwtd";
    case 0x9c: return "pushf";
    case 0x9d: return "popf";
    case 0xa0: case 0xa1: case 0xa2: case 0xa3: {       /* mov al/eax <-> moffs */
        if (opcode % 2 == 0) inst->size = 1;
        char addr[24];
        snprintf(addr, sizeof addr, "0x%llx", (unsigned long long)take(inst, 8));
        if (opcode < 0xa2) { op(inst, "%s", addr); op_reg(inst, RAX, inst->size); }
        else               { op_reg(inst, RAX, inst->size); op(inst, "%s", addr); }
        return "movabs";
    }
    case 0xa8: case 0xa9:                               /* test imm, al / eax */
        if (opcode == 0xa8) inst->size = 1;
        imm = take_imm(inst);
        op_imm(inst, imm, inst->size); op_reg(inst, RAX, inst->size);
        return "test";
    case 0xc0: case 0xc1: case 0xd0: case 0xd1: case 0xd2: case 0xd3:  /* shifts */
        if (!(opcode & 1)) inst->size = 1;
        read_modrm(inst);
        if (!shift_name[inst->sub])
            return NULL;
        if (opcode <= 0xc1)      op_imm(inst, (uint8_t)take(inst, 1), 1);
        else if (opcode <= 0xd1) op(inst, "$1");
        else                     op_reg(inst, RCX, 1);
        op_rm(inst, inst->size);
        *flags = SUF;
        return shift_name[inst->sub];
    case 0xc3:
        if (inst->rep) { inst->rep = 0; return "repz ret"; }
        return "ret";
    case 0xc6: case 0xc7:                               /* mov imm, r/m */
        if (opcode == 0xc6) inst->size = 1;
        read_modrm(inst);
        if (inst->sub != 0)
            return NULL;
        op_imm(inst, take_imm(inst), inst->size); op_rm(inst, inst->size);
        *flags = SUF;
        return "mov";
    case 0xc9: return "leave";
    case 0xcc: return "int3";
    case 0xe8: case 0xe9:                               /* call, jmp rel32 */
        op_target(inst, take_s32(inst));
        return opcode == 0xe8 ? "call" : "jmp";
    case 0xeb:                                          /* jmp rel8 */
        op_target(inst, take_s8(inst));
        return "jmp";
    case 0xf4: return "hlt";
    case 0xf6: case 0xf7:                               /* group 3 */
        if (opcode == 0xf6) inst->size = 1;
        read_modrm(inst);
        if (!grp3_name[inst->sub])
            return NULL;
        if (inst->sub == 0)
            op_imm(inst, take_imm(inst), inst->size);
        op_rm(inst, inst->size);
        *flags = SUF;
        return grp3_name[inst->sub];
    case 0xfe: case 0xff:                               /* group 4 and 5 */
        if (opcode == 0xfe) inst->size = 1;
        read_modrm(inst);
        if (inst->sub <= 1) {
            op_rm(inst, inst->size);
            *flags = SUF;
            return inst->sub ? "dec" : "inc";
        }
        if (opcode == 0xfe)
            return NULL;
        if (inst->sub == 2 || inst->sub == 4) {         /* call *, jmp * */
            op_rm(inst, 8);
            *flags = STAR;
            return inst->sub == 2 ? "call" : "jmp";
        }
        if (inst->sub == 6) {
            op_rm(inst, 8);
            return "push";
        }
        return NULL;
    case 0x0f:
        break;
    default:
        return NULL;
    }

    opcode2 = (unsigned)take(inst, 1);                  /* 0f xx */
    const sse_op *row = sse_find(0x100 | opcode2,
                                 inst->rep ? 2 : inst->repne ? 3 : inst->n66 == 1);
    if (row) {                                          /* SSE: the prefix is part of the opcode */
        int isz = inst->rex & 8 ? 8 : 4;                /* of an integer operand */
        inst->rep = inst->repne = 0;
        inst->size = isz;
        read_modrm(inst);
        switch (row->form) {
        case SSE_REG_TO_RM:  op_reg(inst, inst->reg, 16); op_rm(inst, 16); break;
        case SSE_INT_TO_REG: op_rm(inst, isz); op_reg(inst, inst->reg, 16); break;
        case SSE_RM_TO_INT:  op_rm(inst, 16); op_reg(inst, inst->reg, isz); break;
        case SSE_REG_TO_INT: op_reg(inst, inst->reg, 16); op_rm(inst, isz); break;
        default:             op_rm(inst, 16); op_reg(inst, inst->reg, 16); break;
        }
        if (row->width)
            return row->name;
        if (opcode2 == 0x2a) {                          /* cvtsi2ssl, cvtsi2ssq from memory */
            *flags = SUF;
            return row->name;
        }
        return isz == 8 ? "movq" : "movd";
    }
    if (opcode2 >= 0x40 && opcode2 <= 0x4f) {           /* cmovcc r/m, r */
        read_modrm(inst); op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size);
        snprintf(name, sizeof name, "cmov%s", cc_name[opcode2 & 15]);
        return name;
    }
    if (opcode2 >= 0x80 && opcode2 <= 0x8f) {           /* jcc rel32 */
        op_target(inst, take_s32(inst));
        snprintf(name, sizeof name, "j%s", cc_name[opcode2 & 15]);
        return name;
    }
    if (opcode2 >= 0x90 && opcode2 <= 0x9f) {           /* setcc r/m8 */
        inst->size = 1;
        read_modrm(inst); op_rm(inst, 1);
        snprintf(name, sizeof name, "set%s", cc_name[opcode2 & 15]);
        return name;
    }
    switch (opcode2) {
    case 0x05: return "syscall";
    case 0x0b: return "ud2";
    case 0x1e:                                          /* f3 0f 1e fa */
        if (inst->rep && take(inst, 1) == 0xfa) { inst->rep = 0; return "endbr64"; }
        return NULL;
    case 0x1f:                                          /* nop r/m */
        read_modrm(inst); op_rm(inst, inst->size);
        *flags = SUF;
        return "nop";
    case 0xaf:                                          /* imul r/m, r */
        read_modrm(inst); op_rm(inst, inst->size); op_reg(inst, inst->reg, inst->size);
        return "imul";
    case 0xb6: case 0xb7: case 0xbe: case 0xbf: {       /* movzx, movsx */
        int from = opcode2 & 1 ? 2 : 1;
        read_modrm(inst); op_rm(inst, from); op_reg(inst, inst->reg, inst->size);
        snprintf(name, sizeof name, "mov%c%c%c", opcode2 < 0xb8 ? 'z' : 's',
                 suffix(from), suffix(inst->size));
        return name;
    }
    }
    return NULL;
}

unsigned translate(uint64_t pc, const uint8_t *bytes, unsigned nbytes, char *out, size_t size)
{
    dis inst;
    const char *name = NULL;
    char prefix[64] = "", mnem[80], mem[64], args[160];
    int flags = 0, len;
    unsigned byte;

    memset(&inst, 0, sizeof inst);
    inst.pc = pc;
    inst.buf = bytes;
    inst.nbytes = nbytes;
    inst.size = 4;
    inst.rm = -1;
    inst.memop_at = -1;

    /* 1. prefixes */
    for (;;) {
        byte = (unsigned)take(&inst, 1);
        if (inst.bad) break;
        if (byte == 0x66) {
            if (inst.n66++) strcat(prefix, "data16 ");
            inst.size = 2;
        } else if (byte == 0xf3) {
            inst.rep = 1;
        } else if (byte == 0xf2) {
            inst.repne = 1;
        } else if (byte == 0x2e) {
            strcat(prefix, "cs ");
        } else if (byte == 0x3e) {
            strcat(prefix, "ds ");
        } else if (byte == 0x64 || byte == 0x65) {
            inst.seg = byte == 0x64 ? "fs" : "gs";
        } else if ((byte & 0xf0) == 0x40) {             /* REX, the last prefix */
            inst.rex = (uint8_t)byte;
            if (byte & 8) inst.size = 8;
            byte = (unsigned)take(&inst, 1);
            break;
        } else {
            break;
        }
        if (strlen(prefix) > 40) inst.bad = 1;
    }

    /* 2 .. 5 */
    if (!inst.bad && (byte == 0xc4 || byte == 0xc5)) {
        unsigned vex1 = (unsigned)take(&inst, 1), vex2 = vex1, map = 1, rex_w = 0;
        if (byte == 0xc4) {
            map = vex1 & 0x1f;
            vex2 = (unsigned)take(&inst, 1);
            rex_w = vex2 >> 7;
        }
        inst.vex = 1;
        inst.rex = (uint8_t)(0x40 | rex_w << 3 | !(vex1 & 0x80) << 2 |
                             (byte == 0xc4 && !(vex1 & 0x40)) << 1 |
                             (byte == 0xc4 && !(vex1 & 0x20)));
        inst.vvvv = (int)(~vex2 >> 3 & 15);
        inst.vlen = vex2 & 4 ? 32 : 16;
        byte = (unsigned)take(&inst, 1);
        if (!inst.bad && !inst.n66 && !inst.rep && !inst.repne)
            name = vex_insn(&inst, map, vex2 & 3, byte);
    } else if (!inst.bad) {
        name = legacy_insn(&inst, byte, &flags);
    }

    if (!name || inst.bad) {
        snprintf(out, size, "(bad)");
        return 1;
    }
    if (inst.rep)   strcat(prefix, "repz ");
    if (inst.repne) strcat(prefix, "repnz ");

    /* the mnemonic, with a suffix when no register operand gives the size */
    len = snprintf(mnem, sizeof mnem, "%s%s", prefix, name);
    if ((flags & SUF) && inst.rm < 0)
        snprintf(mnem + len, sizeof mnem - (size_t)len, "%c", suffix(inst.size));

    /* the operands, source first */
    args[0] = 0;
    for (int operand = 0; operand < inst.nops; ++operand) {
        if (operand == inst.memop_at)
            memtext(&inst, mem, sizeof mem);
        len = (int)strlen(args);
        snprintf(args + len, sizeof args - (size_t)len, "%s%s%s", operand ? "," : "",
                 (flags & STAR) ? "*" : "",
                 operand == inst.memop_at ? mem : inst.ops[operand]);
    }
    if (!inst.nops)
        len = snprintf(out, size, "%s", mnem);
    else
        len = snprintf(out, size, "%-6s %s", mnem, args);
    if (inst.memop_at >= 0 && inst.modrm.rip && len >= 0 && (size_t)len < size)
        snprintf(out + len, size - (size_t)len, "        # %llx",
                 (unsigned long long)(pc + inst.pos + (uint64_t)inst.modrm.disp));
    return inst.pos;
}
