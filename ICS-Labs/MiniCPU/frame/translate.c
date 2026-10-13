/* Machine code back into assembly: the disassembler of mini-cpu --disasm.
 *
 * translate() reads one instruction from a buffer of bytes and writes it in
 * the AT&T syntax that objdump -d prints, in the order the bytes come:
 *
 *   1. prefixes     66 (operand size 16), f2 f3, segment prefixes, then REX;
 *                   or a VEX prefix (c4, c5), which carries REX bits itself.
 *                   Before an SSE opcode, 66 and f3 select the instruction
 *                   (the table sse_ops[] of frame/simd.c)
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
    unsigned       n;               /* bytes in buf */
    unsigned       i;               /* bytes read so far */
    int            bad;             /* ran past n, or an unknown encoding */
    int            size;            /* operand size in bytes: 1, 2, 4 or 8 */
    uint8_t        rex;             /* REX prefix, 0 if none; the bits of VEX */
    int            n66, rep, repne; /* prefixes seen */
    const char    *seg;             /* fs or gs before a memory operand, NULL */
    int            vex, vlen, v;    /* VEX: its prefix, 16 or 32, VEX.vvvv */
    int            mod;             /* ModRM mod field */
    int            reg;             /* ModRM reg field with REX.R */
    int            sub;             /* ModRM reg field alone, 0..7 */
    int            rm;              /* ModRM rm register with REX.B, -1 for memory */
    modrm_t        m;               /* the ModRM byte; the memory operand when rm is -1 */
    char           ops[3][48];      /* operands in AT&T order, source first */
    int            nops;
    int            memop_at;        /* index in ops of the memory operand, -1 */
} dis;

/* ------------------------------------------------------------- bytes */

/* w bytes, little endian. Reading past the buffer marks the instruction bad. */
static uint64_t take(dis *d, unsigned w)
{
    uint64_t v = 0;
    if (d->i + w > d->n || d->i + w > MAX_INSN)
        d->bad = 1;
    else
        memcpy(&v, d->buf + d->i, w);
    d->i += w;
    return v;
}

static int64_t take_s8(dis *d)  { return (int8_t)take(d, 1); }
static int64_t take_s32(dis *d) { return (int32_t)take(d, 4); }

/* An immediate of the operand size, at most 4 bytes, sign-extended. */
static int64_t take_imm(dis *d)
{
    switch (d->size) {
    case 1:  return (int8_t)take(d, 1);
    case 2:  return (int16_t)take(d, 2);
    default: return (int32_t)take(d, 4);
    }
}

/* ---------------------------------------------------------- operands */

static void op(dis *d, const char *fmt, ...)
{
    va_list ap;
    if (d->nops == 3) { d->bad = 1; return; }
    va_start(ap, fmt);
    vsnprintf(d->ops[d->nops++], sizeof d->ops[0], fmt, ap);
    va_end(ap);
}

/* The name of register n at size bytes; 16 and 32 name xmm n and ymm n. */
static const char *regname(const dis *d, int n, int size)
{
    static const char *const q[16] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
    static const char *const b[8] = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil" };
    static const char *const h[8] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
    static char s[16];
    switch (size) {
    case 8:  return q[n];
    case 16: snprintf(s, sizeof s, "xmm%d", n); return s;
    case 32: snprintf(s, sizeof s, "ymm%d", n); return s;
    }
    if (n >= 8) {
        snprintf(s, sizeof s, "%s%s", q[n], size == 4 ? "d" : size == 2 ? "w" : "b");
        return s;
    }
    if (size == 4) { snprintf(s, sizeof s, "e%s", q[n] + 1); return s; }
    if (size == 2) return q[n] + 1;
    return d->rex ? b[n] : h[n];
}

static void op_reg(dis *d, int n, int size)
{
    op(d, "%%%s", regname(d, n, size));
}

/* The r/m operand: register rm at size bytes, or the memory operand, which
   is written out when the length of the instruction is known. */
static void op_rm(dis *d, int size)
{
    if (d->rm >= 0) {
        op_reg(d, d->rm, size);
    } else {
        d->memop_at = d->nops;
        op(d, "");
    }
}

/* An immediate, shown at the operand size as objdump shows it. */
static void op_imm(dis *d, int64_t v, int size)
{
    uint64_t u = trunc_val((uint64_t)v, size);
    op(d, "$0x%llx", (unsigned long long)u);
}

/* ------------------------------------------------------ ModRM and SIB */

/* The ModRM byte, with its SIB byte and displacement: modrm_decode of
   lab 1 on the bytes that are left. */
static void read_modrm(dis *d)
{
    unsigned end = d->n < MAX_INSN ? d->n : MAX_INSN;
    unsigned k = d->i < end ? modrm_decode(d->buf + d->i, end - d->i, d->rex, &d->m) : 0;
    if (k == 0 || k > end - d->i) {
        d->bad = 1;
        d->rm = -1;
        memset(&d->m, 0, sizeof d->m);
        d->m.base = d->m.index = -1;
        d->i += 1;
        return;
    }
    d->i += k;
    d->mod = d->m.mod;
    d->sub = d->m.sub;
    d->reg = d->m.reg;
    d->rm = d->m.rm;
}

/* A signed number in hex: -0x10, 0x0. */
static int signed_hex(char *s, size_t n, int64_t v)
{
    if (v < 0)
        return snprintf(s, n, "-0x%llx", (unsigned long long)-(uint64_t)v);
    return snprintf(s, n, "0x%llx", (unsigned long long)v);
}

/* The memory operand in AT&T syntax. */
static void memtext(const dis *d, char *s, size_t n)
{
    const modrm_t *m = &d->m;
    int k = 0;
    if (d->seg) {
        k = snprintf(s, n, "%%%s:", d->seg);
        s += k; n -= (size_t)k; k = 0;
    }
    if (m->rip) {
        k = signed_hex(s, n, m->disp);
        snprintf(s + k, n - (size_t)k, "(%%rip)");
        return;
    }
    if (m->base < 0 && m->index < 0) {              /* an absolute address */
        snprintf(s, n, "0x%llx", (unsigned long long)m->disp);
        return;
    }
    if (m->dsize)
        k = signed_hex(s, n, m->disp);
    k += snprintf(s + k, n - (size_t)k, "(");
    if (m->base >= 0)
        k += snprintf(s + k, n - (size_t)k, "%%%s", regname(d, m->base, 8));
    if (m->index >= 0)
        k += snprintf(s + k, n - (size_t)k, ",%%%s,%d", regname(d, m->index, 8), m->scale);
    snprintf(s + k, n - (size_t)k, ")");
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
static void op_target(dis *d, int64_t rel)
{
    op(d, "%llx", (unsigned long long)(d->pc + d->i + (uint64_t)rel));
}

/* The instructions with a VEX prefix. Returns the mnemonic, NULL if unknown. */
static const char *vex_insn(dis *d, unsigned map, unsigned pp, unsigned b)
{
    int w = d->vlen;
    if (map == 1 && pp == 2 && b == 0x6f) {             /* vmovdqu rm, reg */
        read_modrm(d); op_rm(d, w); op_reg(d, d->reg, w);
        return "vmovdqu";
    }
    if (map == 1 && pp == 2 && b == 0x7f) {             /* vmovdqu reg, rm */
        read_modrm(d); op_reg(d, d->reg, w); op_rm(d, w);
        return "vmovdqu";
    }
    if ((map == 1 && pp == 1 && (b == 0xef || b == 0xfe || b == 0xfa || b == 0xd4 ||
                                 b == 0xdb)) ||
        (map == 2 && pp == 1 && (b == 0x40 || b == 0x02))) {  /* rm, v, reg */
        read_modrm(d); op_rm(d, w); op_reg(d, d->v, w); op_reg(d, d->reg, w);
        switch (map << 8 | b) {
        case 0x1ef: return "vpxor";
        case 0x1db: return "vpand";
        case 0x1fe: return "vpaddd";
        case 0x1fa: return "vpsubd";
        case 0x1d4: return "vpaddq";
        case 0x240: return "vpmulld";
        default:    return "vphaddd";
        }
    }
    if (map == 1 && pp == 1 && b == 0x72) {             /* vpsrld imm, rm, v: 72 /2 ib */
        read_modrm(d);
        int64_t imm = (uint8_t)take(d, 1);
        if (d->sub != 2 || d->rm < 0)
            return NULL;
        op_imm(d, imm, 1); op_reg(d, d->rm, w); op_reg(d, d->v, w);
        return "vpsrld";
    }
    if (map == 2 && pp == 1 && b == 0x31) {             /* vpmovzxbd xmm/m64, reg */
        read_modrm(d); op_rm(d, 16); op_reg(d, d->reg, w);
        return "vpmovzxbd";
    }
    if (map == 3 && pp == 1 && b == 0x39 && w == 32) {  /* vextracti128 imm, reg, rm */
        read_modrm(d);
        int64_t imm = (uint8_t)take(d, 1);
        op_imm(d, imm, 1); op_reg(d, d->reg, 32); op_rm(d, 16);
        return "vextracti128";
    }
    if (map == 1 && pp == 1 && b == 0x7e && w == 16) {  /* vmovd xmm, r/m32 */
        read_modrm(d); op_reg(d, d->reg, 16); op_rm(d, d->rex & 8 ? 8 : 4);
        return d->rex & 8 ? "vmovq" : "vmovd";
    }
    if (map == 1 && pp == 1 && b == 0x6e && w == 16) {  /* vmovd r/m32, xmm */
        read_modrm(d); op_rm(d, d->rex & 8 ? 8 : 4); op_reg(d, d->reg, 16);
        return d->rex & 8 ? "vmovq" : "vmovd";
    }
    return NULL;
}

/* The instruction after its prefixes. Returns the mnemonic, NULL if
   unknown, and sets *flags. */
static const char *legacy_insn(dis *d, unsigned b, int *flags)
{
    static char name[16];
    int64_t imm;
    unsigned b2;

    if (b < 0x40 && (b & 7) < 6) {                      /* ALU: 00 .. 3d */
        switch (b & 7) {
        case 0: d->size = 1; /* fall through */
        case 1: read_modrm(d); op_reg(d, d->reg, d->size); op_rm(d, d->size); break;
        case 2: d->size = 1; /* fall through */
        case 3: read_modrm(d); op_rm(d, d->size); op_reg(d, d->reg, d->size); break;
        case 4: d->size = 1; /* fall through */
        case 5: imm = take_imm(d); op_imm(d, imm, d->size); op_reg(d, RAX, d->size); break;
        }
        return alu_name[b >> 3];
    }
    if (b >= 0x50 && b <= 0x5f) {                       /* push r, pop r */
        op_reg(d, (b & 7) | (d->rex & 1) << 3, 8);
        return b < 0x58 ? "push" : "pop";
    }
    if (b >= 0x70 && b <= 0x7f) {                       /* jcc rel8 */
        op_target(d, take_s8(d));
        snprintf(name, sizeof name, "j%s", cc_name[b & 15]);
        return name;
    }
    if (b >= 0xb0 && b <= 0xbf) {                       /* mov r, imm */
        int r = (b & 7) | (d->rex & 1) << 3;
        if (b < 0xb8) {
            d->size = 1;
            op_imm(d, (uint8_t)take(d, 1), 1);
            op_reg(d, r, 1);
            return "mov";
        }
        if (d->size == 8) {
            op_imm(d, (int64_t)take(d, 8), 8);
            op_reg(d, r, 8);
            return "movabs";
        }
        op_imm(d, take_imm(d), d->size);
        op_reg(d, r, d->size);
        return "mov";
    }

    switch (b) {
    case 0x63:                                          /* movslq r/m32, r64 */
        if (d->size != 8)
            return NULL;
        read_modrm(d); op_rm(d, 4); op_reg(d, d->reg, 8);
        return "movslq";
    case 0x68: case 0x6a:                               /* push imm */
        imm = b == 0x68 ? take_s32(d) : take_s8(d);
        op_imm(d, imm, 8);
        return "push";
    case 0x69: case 0x6b:                               /* imul imm, r/m, r */
        read_modrm(d);
        imm = b == 0x69 ? take_imm(d) : take_s8(d);
        op_imm(d, imm, d->size); op_rm(d, d->size); op_reg(d, d->reg, d->size);
        return "imul";
    case 0x80: case 0x81: case 0x83:                    /* ALU imm, r/m */
        if (b == 0x80) d->size = 1;
        read_modrm(d);
        imm = b == 0x81 ? take_imm(d) : take_s8(d);
        op_imm(d, imm, d->size); op_rm(d, d->size);
        *flags = SUF;
        return alu_name[d->sub];
    case 0x84: case 0x85: case 0x86: case 0x87:         /* test, xchg r, r/m */
    case 0x88: case 0x89:                               /* mov r, r/m */
        if (!(b & 1)) d->size = 1;
        read_modrm(d); op_reg(d, d->reg, d->size); op_rm(d, d->size);
        return b < 0x86 ? "test" : b < 0x88 ? "xchg" : "mov";
    case 0x8a: case 0x8b:                               /* mov r/m, r */
        if (b == 0x8a) d->size = 1;
        read_modrm(d); op_rm(d, d->size); op_reg(d, d->reg, d->size);
        return "mov";
    case 0x8d:                                          /* lea m, r */
        read_modrm(d);
        if (d->rm >= 0)
            return NULL;
        op_rm(d, d->size); op_reg(d, d->reg, d->size);
        return "lea";
    case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x90:
        if (b != 0x90 || d->rex & 1) {                  /* xchg rax, r */
            op_reg(d, RAX, d->size); op_reg(d, (b & 7) | (d->rex & 1) << 3, d->size);
            return "xchg";
        }
        if (d->rep) { d->rep = 0; return "pause"; }
        if (d->size == 2) {
            op_reg(d, RAX, 2); op_reg(d, RAX, 2);
            return "xchg";
        }
        return "nop";
    case 0x98: return d->size == 8 ? "cltq" : d->size == 4 ? "cwtl" : "cbtw";
    case 0x99: return d->size == 8 ? "cqto" : d->size == 4 ? "cltd" : "cwtd";
    case 0x9c: return "pushf";
    case 0x9d: return "popf";
    case 0xa0: case 0xa1: case 0xa2: case 0xa3: {       /* mov al/eax <-> moffs */
        if (b % 2 == 0) d->size = 1;
        char addr[24];
        snprintf(addr, sizeof addr, "0x%llx", (unsigned long long)take(d, 8));
        if (b < 0xa2) { op(d, "%s", addr); op_reg(d, RAX, d->size); }
        else          { op_reg(d, RAX, d->size); op(d, "%s", addr); }
        return "movabs";
    }
    case 0xa8: case 0xa9:                               /* test imm, al / eax */
        if (b == 0xa8) d->size = 1;
        imm = take_imm(d);
        op_imm(d, imm, d->size); op_reg(d, RAX, d->size);
        return "test";
    case 0xc0: case 0xc1: case 0xd0: case 0xd1: case 0xd2: case 0xd3:  /* shifts */
        if (!(b & 1)) d->size = 1;
        read_modrm(d);
        if (!shift_name[d->sub])
            return NULL;
        if (b <= 0xc1)      op_imm(d, (uint8_t)take(d, 1), 1);
        else if (b <= 0xd1) op(d, "$1");
        else                op_reg(d, RCX, 1);
        op_rm(d, d->size);
        *flags = SUF;
        return shift_name[d->sub];
    case 0xc3:
        if (d->rep) { d->rep = 0; return "repz ret"; }
        return "ret";
    case 0xc6: case 0xc7:                               /* mov imm, r/m */
        if (b == 0xc6) d->size = 1;
        read_modrm(d);
        if (d->sub != 0)
            return NULL;
        op_imm(d, take_imm(d), d->size); op_rm(d, d->size);
        *flags = SUF;
        return "mov";
    case 0xc9: return "leave";
    case 0xcc: return "int3";
    case 0xe8: case 0xe9:                               /* call, jmp rel32 */
        op_target(d, take_s32(d));
        return b == 0xe8 ? "call" : "jmp";
    case 0xeb:                                          /* jmp rel8 */
        op_target(d, take_s8(d));
        return "jmp";
    case 0xf4: return "hlt";
    case 0xf6: case 0xf7:                               /* group 3 */
        if (b == 0xf6) d->size = 1;
        read_modrm(d);
        if (!grp3_name[d->sub])
            return NULL;
        if (d->sub == 0)
            op_imm(d, take_imm(d), d->size);
        op_rm(d, d->size);
        *flags = SUF;
        return grp3_name[d->sub];
    case 0xfe: case 0xff:                               /* group 4 and 5 */
        if (b == 0xfe) d->size = 1;
        read_modrm(d);
        if (d->sub <= 1) {
            op_rm(d, d->size);
            *flags = SUF;
            return d->sub ? "dec" : "inc";
        }
        if (b == 0xfe)
            return NULL;
        if (d->sub == 2 || d->sub == 4) {               /* call *, jmp * */
            op_rm(d, 8);
            *flags = STAR;
            return d->sub == 2 ? "call" : "jmp";
        }
        if (d->sub == 6) {
            op_rm(d, 8);
            return "push";
        }
        return NULL;
    case 0x0f:
        break;
    default:
        return NULL;
    }

    b2 = (unsigned)take(d, 1);                          /* 0f xx */
    const sse_op *s = sse_find(0x100 | b2, d->rep ? 2 : d->repne ? 3 : d->n66 == 1);
    if (s) {                                            /* SSE: the prefix is part of the opcode */
        int isz = d->rex & 8 ? 8 : 4;                   /* of an integer operand */
        d->rep = d->repne = 0;
        d->size = isz;
        read_modrm(d);
        switch (s->form) {
        case SSE_REG_TO_RM:  op_reg(d, d->reg, 16); op_rm(d, 16); break;
        case SSE_INT_TO_REG: op_rm(d, isz); op_reg(d, d->reg, 16); break;
        case SSE_RM_TO_INT:  op_rm(d, 16); op_reg(d, d->reg, isz); break;
        case SSE_REG_TO_INT: op_reg(d, d->reg, 16); op_rm(d, isz); break;
        default:             op_rm(d, 16); op_reg(d, d->reg, 16); break;
        }
        if (s->width)
            return s->name;
        if (b2 == 0x2a) {                               /* cvtsi2ssl, cvtsi2ssq from memory */
            *flags = SUF;
            return s->name;
        }
        return isz == 8 ? "movq" : "movd";
    }
    if (b2 >= 0x40 && b2 <= 0x4f) {                     /* cmovcc r/m, r */
        read_modrm(d); op_rm(d, d->size); op_reg(d, d->reg, d->size);
        snprintf(name, sizeof name, "cmov%s", cc_name[b2 & 15]);
        return name;
    }
    if (b2 >= 0x80 && b2 <= 0x8f) {                     /* jcc rel32 */
        op_target(d, take_s32(d));
        snprintf(name, sizeof name, "j%s", cc_name[b2 & 15]);
        return name;
    }
    if (b2 >= 0x90 && b2 <= 0x9f) {                     /* setcc r/m8 */
        d->size = 1;
        read_modrm(d); op_rm(d, 1);
        snprintf(name, sizeof name, "set%s", cc_name[b2 & 15]);
        return name;
    }
    switch (b2) {
    case 0x05: return "syscall";
    case 0x0b: return "ud2";
    case 0x1e:                                          /* f3 0f 1e fa */
        if (d->rep && take(d, 1) == 0xfa) { d->rep = 0; return "endbr64"; }
        return NULL;
    case 0x1f:                                          /* nop r/m */
        read_modrm(d); op_rm(d, d->size);
        *flags = SUF;
        return "nop";
    case 0xaf:                                          /* imul r/m, r */
        read_modrm(d); op_rm(d, d->size); op_reg(d, d->reg, d->size);
        return "imul";
    case 0xb6: case 0xb7: case 0xbe: case 0xbf: {       /* movzx, movsx */
        int from = b2 & 1 ? 2 : 1;
        read_modrm(d); op_rm(d, from); op_reg(d, d->reg, d->size);
        snprintf(name, sizeof name, "mov%c%c%c", b2 < 0xb8 ? 'z' : 's',
                 suffix(from), suffix(d->size));
        return name;
    }
    }
    return NULL;
}

unsigned translate(uint64_t pc, const uint8_t *buf, unsigned n, char *out, size_t size)
{
    dis d;
    const char *name = NULL;
    char prefix[64] = "", mnem[80], mem[64], args[160];
    int flags = 0, k;
    unsigned b;

    memset(&d, 0, sizeof d);
    d.pc = pc;
    d.buf = buf;
    d.n = n;
    d.size = 4;
    d.rm = -1;
    d.memop_at = -1;

    /* 1. prefixes */
    for (;;) {
        b = (unsigned)take(&d, 1);
        if (d.bad) break;
        if (b == 0x66) {
            if (d.n66++) strcat(prefix, "data16 ");
            d.size = 2;
        } else if (b == 0xf3) {
            d.rep = 1;
        } else if (b == 0xf2) {
            d.repne = 1;
        } else if (b == 0x2e) {
            strcat(prefix, "cs ");
        } else if (b == 0x3e) {
            strcat(prefix, "ds ");
        } else if (b == 0x64 || b == 0x65) {
            d.seg = b == 0x64 ? "fs" : "gs";
        } else if ((b & 0xf0) == 0x40) {                /* REX, the last prefix */
            d.rex = (uint8_t)b;
            if (b & 8) d.size = 8;
            b = (unsigned)take(&d, 1);
            break;
        } else {
            break;
        }
        if (strlen(prefix) > 40) d.bad = 1;
    }

    /* 2 .. 5 */
    if (!d.bad && (b == 0xc4 || b == 0xc5)) {
        unsigned b1 = (unsigned)take(&d, 1), b2 = b1, map = 1, w = 0;
        if (b == 0xc4) {
            map = b1 & 0x1f;
            b2 = (unsigned)take(&d, 1);
            w = b2 >> 7;
        }
        d.vex = 1;
        d.rex = (uint8_t)(0x40 | w << 3 | !(b1 & 0x80) << 2 |
                          (b == 0xc4 && !(b1 & 0x40)) << 1 | (b == 0xc4 && !(b1 & 0x20)));
        d.v = (int)(~b2 >> 3 & 15);
        d.vlen = b2 & 4 ? 32 : 16;
        b = (unsigned)take(&d, 1);
        if (!d.bad && !d.n66 && !d.rep && !d.repne)
            name = vex_insn(&d, map, b2 & 3, b);
    } else if (!d.bad) {
        name = legacy_insn(&d, b, &flags);
    }

    if (!name || d.bad) {
        snprintf(out, size, "(bad)");
        return 1;
    }
    if (d.rep)   strcat(prefix, "repz ");
    if (d.repne) strcat(prefix, "repnz ");

    /* the mnemonic, with a suffix when no register operand gives the size */
    k = snprintf(mnem, sizeof mnem, "%s%s", prefix, name);
    if ((flags & SUF) && d.rm < 0)
        snprintf(mnem + k, sizeof mnem - (size_t)k, "%c", suffix(d.size));

    /* the operands, source first */
    args[0] = 0;
    for (int j = 0; j < d.nops; ++j) {
        if (j == d.memop_at)
            memtext(&d, mem, sizeof mem);
        k = (int)strlen(args);
        snprintf(args + k, sizeof args - (size_t)k, "%s%s%s", j ? "," : "",
                 (flags & STAR) ? "*" : "",
                 j == d.memop_at ? mem : d.ops[j]);
    }
    if (!d.nops)
        k = snprintf(out, size, "%s", mnem);
    else
        k = snprintf(out, size, "%-6s %s", mnem, args);
    if (d.memop_at >= 0 && d.m.rip && k >= 0 && (size_t)k < size)
        snprintf(out + k, size - (size_t)k, "        # %llx",
                 (unsigned long long)(pc + d.i + (uint64_t)d.m.disp));
    return d.i;
}
