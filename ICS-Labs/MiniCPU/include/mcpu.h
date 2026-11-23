/* mini-cpu: an x86-64 machine that runs in user mode, built over five labs.
 *
 * This header is the whole interface between the code of the labs and the
 * framework. Every function a lab asks for is declared here under the
 * heading of its lab, with a comment that starts "Lab N:". The files under
 * src/labN/ define exactly these functions, the framework calls nothing
 * else in them, and they need nothing from the framework beyond what this
 * header declares. Leave this file unchanged.
 *
 *   lab  functions to write                         stand-in before the lab
 *   1    src/lab1/modrm.c    modrm_decode           -
 *        src/lab1/alu.c      x86_szp, x86_add,
 *                            x86_sub, x86_logic,
 *                            x86_inc, x86_dec,
 *                            x86_neg, x86_shift,
 *                            x86_mul, x86_div
 *        src/lab1/interp.c   cond, interp_run
 *   2    src/lab2/cache.c    cache_init .. flush    frame/cpu/nocache.c (no cache)
 *   3    src/lab3/mmu.c      mmu_translate,         frame/cpu/pagemap.c and
 *                            mmu_map, mmu_tlb_flush   frame/os/pagemap.c
 *                                                     (a table on the host)
 *   4    src/lab4/objload.c  obj_sections,          -
 *                            obj_symbols, obj_relocs
 *   5    src/lab5/syscall.c  sys_read, sys_open,    frame/os/sysbase.c answers
 *                            sys_close                -ENOSYS for the three
 *        src/lab5/blk.c      blk_kick, ramfs_*      -
 *
 * A later lab runs on the code of the earlier ones: the interpreter of
 * lab 1 runs every program, the code placed by the loader of lab 4
 * included, and every memory access goes through the cache of lab 2 and,
 * once it exists, the MMU of lab 3.
 *
 * The layers of a memory access, from the CPU down:
 *
 *   interp.c         an operand of one instruction, at a virtual address
 *        |
 *   frame/cpu/mem.c  the memory access unit of the CPU: page by page, then
 *        |           line by line
 *   mmu.c            page tables + TLB   -> physical address
 *        |
 *   cache.c          set-associative cache, one line per access, whole
 *        |           64-byte lines on a miss
 *   frame/cpu/dram.c a file mapped with mmap, used as physical memory
 *
 * The framework is split as the machine is. frame/cpu/ simulates the
 * hardware: the instructions, the memory access unit, the stand-ins of
 * the cache and the MMU, DRAM, the block device and the commit log.
 * frame/os/ is the software that runs on it: the loader, the system
 * calls, the allocator of page frames, the copies into and out of guest
 * memory, and the runtime of lab 4. frame/ itself holds the programs of
 * the host: mini-cpu (main.c), mini-jit (jit.c), the probe of --memtrace
 * and --cache 0 (memtrace.c) and the disassembler (translate.c, disasm.c).
 *
 * The framework is compiled with -DLAB=n, n being the highest lab present.
 */
#ifndef MCPU_H
#define MCPU_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LAB
#define LAB 5
#endif

/* What a function that is not written yet calls: prints
   "NAME is not implemented yet" and ends the program with status 3. */
__attribute__((noreturn))
static inline void todo(const char *who)
{
    fprintf(stderr, "%s is not implemented yet\n", who);
    exit(3);
}

/* ---------------------------------------------------------------- machine */

#define GUEST_RAM   (64u << 20)     /* guest physical memory, 64 MiB */
#define ROM_BASE    0x40000000ULL   /* physical address of a file mapped by mmap */
#define PAGE_SIZE   4096u
#define LINE_SIZE   64u             /* one cache line */
#define MAX_INSN    15              /* longest x86 instruction */

enum {                              /* register numbers as encoded in machine code */
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15, NREG
};
#define RFLAGS    16                /* R[16] is rflags */
#define RIP       17                /* R[17] is rip */
#define FLAGS_REG RFLAGS
#define NVEC      16                /* vector registers ymm0 .. ymm15 */
#define YMM0      18                /* register number of ymm0 in arch_get and the logs */
#define NARCH     (YMM0 + NVEC)     /* register numbers 0 .. 33 */

enum { CF = 1u << 0, PF = 1u << 2, ZF = 1u << 6,
       SF = 1u << 7, OF = 1u << 11 };

enum { MEM_FETCH = 0, MEM_READ = 1, MEM_WRITE = 2 };

enum { FAULT_NONE = 0, FAULT_PAGE, FAULT_DIV, FAULT_ILL };

/* A 256-bit value: a ymm register, and in arch_get any register value. The
   low 128 bits of ymm n are xmm n. q, d and b are its 64-bit, 32-bit and
   8-bit lanes, lane 0 lowest; f are the 32-bit lanes as floats. */
typedef union {
    uint64_t q[4];
    uint32_t d[8];
    uint8_t  b[32];
    float    f[8];
} u256;

/* The registers are one array R[0..17]; r, rflags and rip name its parts.
   Y[0..15] are the vector registers ymm0 .. ymm15. */
typedef struct mcpu {
    union {
        uint64_t R[NREG + 2];
        struct {
            uint64_t r[NREG];
            uint64_t rflags;
            uint64_t rip;
        };
    };
    u256     Y[NVEC];
    int      halted;
    int      fault;                 /* FAULT_*, set when an instruction fails */
    uint64_t fault_va;
    uint64_t icount;                /* instructions committed */
    uint64_t brk;                   /* top of the guest heap */
} mcpu;

/* Register reg of the architectural state: 0..15 the integer registers,
   16 rflags, YMM0 + i the vector register ymm i. An integer register is
   the lane q[0] of the value; its other lanes are zero. */
static inline u256 arch_get(const mcpu *cpu, int reg)
{
    u256 value = { { 0 } };
    if (reg >= YMM0)
        value = cpu->Y[reg - YMM0];
    else
        value.q[0] = cpu->R[reg];
    return value;
}

static inline void arch_set(mcpu *cpu, int reg, u256 value)
{
    if (reg >= YMM0)
        cpu->Y[reg - YMM0] = value;
    else
        cpu->R[reg] = value.q[0];
}

/* ------------------------------------------------------- lab 1: modrm.c */

/* The operands a ModRM byte encodes, with its SIB byte and displacement.
 * One operand is the register reg. The other is register rm when mod is 3
 * and a memory operand otherwise, whose address is
 *
 *     disp + base + index * scale          base and index: register values
 *     disp + address of the next instruction               when rip is set
 */
typedef struct {
    uint8_t mod;                    /* the mod field, 0..3 */
    uint8_t sub;                    /* the reg field alone, 0..7: extends some opcodes */
    int8_t  reg;                    /* the reg field with REX.R as bit 3: 0..15 */
    int8_t  rm;                     /* mod 3: the rm field with REX.B as bit 3;
                                       a memory operand: -1 */
    int8_t  base;                   /* base register 0..15, -1 for none */
    int8_t  index;                  /* index register 0..15, -1 for none */
    uint8_t scale;                  /* 1, 2, 4 or 8; 1 without an index */
    uint8_t rip;                    /* 1: relative to the next instruction */
    uint8_t dsize;                  /* bytes of the displacement: 0, 1 or 4 */
    int64_t disp;                   /* the displacement, sign-extended */
} modrm_t;

/* Lab 1: decode the ModRM byte p[0], and the SIB byte and displacement
 * that follow it when it has them. n bytes are readable at p; rex is the
 * REX prefix of the instruction, 0 if it has none (bit 2 is REX.R, bit 1
 * REX.X, bit 0 REX.B). Fills every field of *m and returns the number of
 * bytes used, 1 to 6, or 0 when the n bytes end before the operand does.
 *
 * With mod 3, base and index are -1, scale is 1, and rip, dsize and disp
 * are 0. The interpreter (frame/cpu/x86.c: modrm) and the disassembler
 * (frame/translate.c) both read their operands through this function. */
unsigned modrm_decode(const uint8_t *p, unsigned n, uint8_t rex, modrm_t *m);

/* ---------------------------- lab 1: alu.c, and frame/cpu/arith.c */

/* Values and flags of the integer instructions. size is the operand size
 * in bytes: 1, 2, 4 or 8. A flag function changes only CF, PF, ZF, SF
 * and OF. */

/* The low size bytes of v; v sign-extended from size bytes (frame/cpu/arith.c). */
uint64_t trunc_val(uint64_t v, int size);
int64_t  sext(uint64_t v, int size);

/* f with the bits m set when on is nonzero, cleared otherwise. */
static inline uint64_t flag(uint64_t f, uint64_t m, int on)
{
    return on ? f | m : f & ~m;
}

/* The top bit of a size-byte value. */
static inline uint64_t msb(int size)
{
    return 1ULL << (size * 8 - 1);
}

/* 1 if the low 8 bits of v have an even number of ones. */
static inline int parity(uint64_t v)
{
    v &= 0xff;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return !(v & 1);
}

/* Lab 1: f with ZF, SF and PF set from the size-byte result r. ZF and SF
 * look at the low size bytes of r, PF at its low 8 bits; the upper bits of
 * r may hold anything. */
uint64_t x86_szp(uint64_t f, uint64_t r, int size);

/* Lab 1: a + b + carry on size-byte operands, carry being 0 or 1. Returns
 * the result truncated to size bytes and sets all five flags in *f:
 * ZF, SF, PF from the result, CF when the unsigned sum does not fit in size
 * bytes, OF when the signed sum does not. The upper bits of a and b may
 * hold anything. add and adc use it, and x86_inc. */
uint64_t x86_add(uint64_t *f, uint64_t a, uint64_t b, int size, int carry);

/* Lab 1: a - b - borrow, as x86_add. CF is set when the subtraction
 * borrows (a < b + borrow as unsigned numbers), OF when the signed
 * difference does not fit. sub, sbb and cmp use it, and x86_dec and
 * x86_neg. */
uint64_t x86_sub(uint64_t *f, uint64_t a, uint64_t b, int size, int borrow);

/* Lab 1: the flags of and, or, xor and test. r is the result of the
 * operation; returns it truncated to size bytes and sets ZF, SF, PF from
 * it; CF and OF are cleared. */
uint64_t x86_logic(uint64_t *f, uint64_t r, int size);

/* Lab 1: v + 1 (inc) and v - 1 (dec) on size bytes. Each returns the
 * result truncated to size bytes. ZF, SF, PF and OF are those of
 * x86_add(f, v, 1, size, 0) and x86_sub(f, v, 1, size, 0); CF keeps the
 * value it had before the call. */
uint64_t x86_inc(uint64_t *f, uint64_t v, int size);
uint64_t x86_dec(uint64_t *f, uint64_t v, int size);

/* Lab 1: 0 - v on size bytes (neg): the result and the five flags of
 * x86_sub(f, 0, v, size, 0). CF is set unless v is zero. */
uint64_t x86_neg(uint64_t *f, uint64_t v, int size);

/* Lab 1: v shifted by n bits on size bytes. sub is the ModRM reg field of
 * c0/c1/d0..d3: 4 shl, 5 shr, 6 sal (the same as shl), 7 sar. Of n, only
 * the low 6 bits count when size is 8 and the low 5 bits otherwise, so the
 * count of a 1-byte shift goes up to 31. Returns the result truncated to
 * size bytes; the upper bits of v may hold anything.
 *
 * A count of zero changes no flag. Otherwise ZF, SF and PF are set from
 * the result, CF is the last bit shifted out, and
 *
 *        the n single-bit shifts                          OF
 *   shl  to the left, zeros enter at the bottom           top bit of the result != CF
 *   shr  to the right, zeros enter at the top             the top bit of v
 *   sar  to the right, the top bit of v enters at the top 0
 *
 * x86 defines OF only for a count of 1, and CF of shl and shr only for a
 * count below the width of the operand. This machine applies the rules
 * above to every count: its commit log records the flags. */
uint64_t x86_shift(uint64_t *f, int sub, uint64_t v, unsigned n, int size);

/* Lab 1: the product of the low size bytes of a and b. sub is the ModRM
 * reg field of f6/f7: 4 mul (a and b are unsigned), 5 imul (signed); the
 * two- and three-operand forms of imul pass 5. The product has 2 * size
 * bytes: *lo gets its lower size bytes and *hi its upper size bytes, each
 * truncated to size bytes. CF and OF are both set when the product does
 * not fit in size bytes (mul: *hi is not zero; imul: the product differs
 * from *lo sign-extended) and both cleared when it does. ZF, SF and PF
 * are set from *lo; x86 leaves them undefined. */
void     x86_mul(uint64_t *f, int sub, uint64_t a, uint64_t b, int size,
                 uint64_t *lo, uint64_t *hi);

/* Lab 1: hi:lo divided by the low size bytes of v. sub is the ModRM reg
 * field of f6/f7: 6 div (unsigned), 7 idiv (signed: the quotient is
 * rounded toward zero and the remainder has the sign of the dividend).
 * The dividend has 2 * size bytes, its upper size bytes in hi and its
 * lower size bytes in lo. Returns 0 and stores the quotient in *q and the
 * remainder in *r, each truncated to size bytes. Returns -1, a divide
 * error, when v is zero or the quotient does not fit in size bytes. No
 * flag changes. The upper bits of hi, lo and v may hold anything.
 *
 * gcc has the integer types __int128 and unsigned __int128, wide enough
 * for the product and the dividend of 8-byte operands. */
int      x86_div(int sub, uint64_t hi, uint64_t lo, uint64_t v, int size,
                 uint64_t *q, uint64_t *r);

/* The rest are in frame/cpu/arith.c. */
/* One of the eight ALU operations by its number aop, the ModRM reg field
 * of group 1: 0 add, 1 or, 2 adc, 3 sbb, 4 and, 5 sub, 6 xor, 7 cmp. Calls
 * x86_add, x86_sub and x86_logic. Returns the result; cmp returns a - b,
 * which the caller does not store. */
uint64_t x86_alu(uint64_t *f, int aop, uint64_t a, uint64_t b, int size);
/* rol (sub 0) or ror (sub 1) of v by n: the rotations of c0/c1/d0..d3,
 * which lab 1 does not implement. */
uint64_t x86_rotate(uint64_t *f, int sub, uint64_t v, unsigned n, int size);

/* ---------------------------------------------------- lab 1: interp.c */

/* Lab 1: whether the condition code cc of jcc, setcc and cmovcc holds
 * under the flags f: 1 or 0. Only the low 4 bits of cc count, so the
 * opcode itself can be passed:
 *
 *   0 o    OF              1 no   !OF
 *   2 b    CF              3 ae   !CF
 *   4 e    ZF              5 ne   !ZF
 *   6 be   CF or ZF        7 a    neither CF nor ZF
 *   8 s    SF              9 ns   !SF
 *   a p    PF              b np   !PF
 *   c l    SF != OF        d ge   SF == OF
 *   e le   ZF or SF != OF  f g    !ZF and SF == OF
 *
 * interp.c uses it for jcc, x86_other for cmovcc and setcc. */
int cond(uint64_t f, int cc);

/* The instruction being interpreted, filled by x86_begin and modrm
 * (frame/cpu/x86.c). interp.c reads op, size, rex, reg, sub, v and vlen; the
 * other fields belong to the helpers. */
typedef struct {
    mcpu    *c;
    uint64_t pc;                    /* address of the instruction */
    unsigned op;                    /* opcode after the prefixes; 0f xx is 0x100 | xx,
                                       a VEX opcode is 0x1000 | map << 8 | xx */
    int      size;                  /* operand size: 4, 8 with REX.W, 2 with 0x66 */
    uint8_t  rex;                   /* REX prefix, 0 if none; the R, X, B, W bits of VEX */
    int      vex;                   /* the instruction has a VEX prefix */
    int      vlen;                  /* VEX.L: 16 bytes (xmm), 32 bytes (ymm) */
    int      v;                     /* VEX.vvvv: the second source register, 0..15 */
    int      pp;                    /* prefix 66 (1), f3 (2) or f2 (3), 0 if none: it
                                       selects among the SSE instructions of one opcode */
    int      reg;                   /* ModRM reg field with REX.R: a register number */
    int      sub;                   /* ModRM reg field alone, 0..7: extends the opcode */
    int      rm;                    /* ModRM rm register, -1 when the operand is memory */
    uint8_t  riprel;                /* the address is relative to the next instruction */
    uint64_t addr;                  /* address of the memory operand, without rip */
    unsigned n;                     /* how many bytes of buf could be fetched */
    uint8_t  store, msize;          /* the instruction stored msize bytes */
    uint8_t  ysave;                 /* ysaved holds the vector registers */
    uint64_t sa;                    /* at address sa */
    uint8_t  buf[MAX_INSN];         /* bytes fetched at pc */
    u256     sval;                  /* of value sval */
    uint64_t saved[NREG + 2];       /* the registers before the instruction */
    u256     ysaved[NVEC];          /* saved only for a vector or floating-point
                                       instruction: no other writes them */
} x86;

/* Lab 1: the interpreter. Runs instructions until the guest exits or
 * faults, or until opt_max instructions have run (opt_max 0: no limit).
 * Every pass of its loop runs one instruction:
 *
 *   op = x86_begin(c, &x)     fetch the instruction at R[RIP]
 *   switch (op)               execute it; an opcode without a case goes
 *                             to x86_other(&x)
 *   x86_end(&x)               -1 ends the loop
 *
 * src/lab1/interp.c lists the opcodes that need a case. */
void interp_run(mcpu *c);
extern uint64_t opt_max;            /* --max N: stop after N instructions, 0 for no limit */

/* Start an instruction at R[RIP]: save the registers, fetch its bytes,
   skip the prefixes, and read the opcode. R[RIP] then points past it. */
unsigned x86_begin(mcpu *c, x86 *x);
/* Finish it: after a fault restore the registers and stop; otherwise
   write the commit log. Returns 0, or -1 after a fault or exit. */
int      x86_end(x86 *x);
/* The opcodes interp.c leaves to the framework: prefixes of the operand
   size other than REX.W, 8-bit forms, multiply and divide, shifts,
   movzx and movsx, cmov and setcc, endbr64 and nop, syscall, the
   floating-point instructions and the vector instructions outside interp.c
   (frame/cpu/simd.c lists both), and others. An opcode it does not know either
   is an illegal instruction. */
void     x86_other(x86 *x);

/* Bytes at R[RIP], little endian; R[RIP] advances past them. */
uint64_t fetch8(x86 *x);
uint64_t fetch16(x86 *x);
uint64_t fetch32(x86 *x);
uint64_t fetch64(x86 *x);
/* An immediate of the operand size, at most 32 bits, sign-extended. */
uint64_t fetch_imm(x86 *x);

/* Read the ModRM byte, and SIB and displacement if present, with
   modrm_decode; sets reg, sub and rm. */
void     modrm(x86 *x);
/* Register n at the operand size. set clears the upper half of a 64-bit
   register when the size is 4, and keeps the upper bits when it is 1 or 2. */
uint64_t get(x86 *x, int n);
void     set(x86 *x, int n, uint64_t v);
/* The rm operand at the operand size: register rm, or memory at ea. */
uint64_t rm_get(x86 *x);
void     rm_set(x86 *x, uint64_t v);
/* Address of the memory operand. A rip-relative address counts from the
   end of the instruction, so read the immediate before calling this. */
uint64_t ea(x86 *x);
/* size bytes of guest memory. A failed access marks a page fault. */
uint64_t load(x86 *x, uint64_t addr, int size);
void     store(x86 *x, uint64_t addr, int size, uint64_t v);

/* Vector operands, n bytes wide: 16 for xmm, 32 for ymm, 4 for a dword.
   rm_vget reads the rm operand, register ymm rm or n bytes of memory, into
   *v and clears the bytes above n. rm_vset and vset write n bytes of *v to
   the rm operand or to ymm register reg; a register write clears the
   bytes above n, as every VEX instruction does. */
void     rm_vget(x86 *x, u256 *v, int n);
void     rm_vset(x86 *x, const u256 *v, int n);
void     vset(x86 *x, int reg, const u256 *v, int n);

/* Whether a VEX instruction is one this machine implements: opcode op
   (0x1000 | map << 8 | byte), prefix pp (0 none, 1 66, 2 f3, 3 f2), VEX.L,
   VEX.W, and vvvv decoded to a register number. */
int      vex_known(unsigned op, int pp, int l, int w, int v);

/* --------------------------------------- frame/cpu/simd.c: SSE and AVX2 */

/* The floating-point instructions of the machine: scalar single precision
 * in the SSE encoding (a prefix, 0f, one opcode byte, ModRM), what gcc
 * emits for float arithmetic. They work on the low 128 bits of a vector
 * register, xmm n, and leave bits 128..255 as they are. One row of
 * sse_ops[] describes one instruction: op is 0x100 | the opcode byte, pp
 * the prefix (x86.pp), width the bytes of its memory operand (0: the
 * integer operand size, 4 or 8 with REX.W). The form names the operands:
 *
 *   SSE_RM_TO_REG    xmm reg <- xmm reg (op) xmm/m: moves, arithmetic, logic
 *   SSE_REG_TO_RM    xmm/m   <- xmm reg: the moves in the other direction
 *   SSE_CMP          flags   <- xmm reg compared with xmm/m32
 *   SSE_INT_TO_REG   xmm reg <- r/m32, r/m64: cvtsi2ss, movd, movq
 *   SSE_RM_TO_INT    r32, r64 <- xmm/m32: cvttss2si
 *   SSE_REG_TO_INT   r/m32, r/m64 <- xmm reg: movd, movq
 */
enum { SSE_RM_TO_REG = 0, SSE_REG_TO_RM, SSE_CMP,
       SSE_INT_TO_REG, SSE_RM_TO_INT, SSE_REG_TO_INT };

typedef struct {
    uint16_t    op;
    uint8_t     pp, form, width;
    const char *name;               /* as objdump -d prints it */
} sse_op;

extern const sse_op sse_ops[];
/* The row of opcode op under prefix pp, NULL if the machine has none. */
const sse_op *sse_find(unsigned op, int pp);

/* What the instruction of row computes. dest is the register it writes,
   as it was (the integer destinations and SSE_CMP take the reg operand
   here), other the other operand, an integer in q[0]; mem says that other
   came from memory. *result becomes the whole new value of the
   destination, an integer in q[0]; SSE_CMP sets CF, PF and ZF in *flags
   and clears SF and OF. isz is the integer operand size. */
void sse_compute(const sse_op *row, int mem, int isz, const u256 *dest, const u256 *other,
                 u256 *result, uint64_t *flags);

/* The vector instructions outside interp.c: vpand (0x11db), vpsrld by an
   immediate (0x1172 /2) and vpmovzxbd (0x1231). from_vvvv is the register
   vvvv (not read by vpmovzxbd), from_rm the rm operand, imm the immediate;
   *result becomes the low vlen bytes of the result, the bytes above
   cleared. */
void vex_compute(unsigned op, int vlen, unsigned imm, const u256 *from_vvvv,
                 const u256 *from_rm, u256 *result);

/* The instruction at bytes, nbytes of them read from address pc, in the
   AT&T syntax of objdump -d: writes at most size bytes, the NUL included,
   to out and returns the length of the instruction. An unknown or
   truncated instruction is written as "(bad)" with length 1
   (frame/translate.c). */
unsigned translate(uint64_t pc, const uint8_t *bytes, unsigned nbytes, char *out, size_t size);

/* mini-cpu --disasm: the code sections of the ELF file at path, one
   instruction per line, as "address:<tab>bytes<tab>instruction"
   (frame/disasm.c). Returns 0, or -1 if the file cannot be read. */
int disasm(const char *path, FILE *out);

/* ------------------------------------------------------- lab 2: cache.c */

/* A physical address has three fields, | tag | set | offset |: the offset
 * takes 6 bits (a line is LINE_SIZE = 64 bytes), so the line number is
 * pa / 64; the set is the line number modulo the number of sets, the tag
 * the line number divided by it.
 *
 * The cache is set-associative with write-back and write allocation, and
 * evicts the least recently used line of a set. Below it there are only
 * dram_line_read and dram_line_write, which move one whole line. */

/* DRAM is the file dram.img, mapped into the host process, and every line
 * moved between the cache and DRAM (dram_line_read, dram_line_write) is
 * removed from the caches of the host, so it is read from or written to
 * the DRAM of the host. The cache keeps its lines in memory of the host,
 * which the caches of the host serve, so a hit costs a few loads and
 * stores. */

/* Lab 2: (re)build the cache with sets * ways lines, all invalid, and
 * reset the three counters. The driver calls it before loading the
 * program and again after: release what the earlier call allocated.
 * 0 for sets or ways means 1. src/lab2/cache.c gives this function
 * written out, as an example. */
void cache_init(unsigned sets, unsigned ways);

/* Lab 2: copy nbytes bytes at physical address pa to dst, through the cache.
 * The bytes lie within one line: pa % LINE_SIZE + nbytes <= LINE_SIZE, and
 * nbytes is at least 1. The memory access unit of the CPU (frame/cpu/mem.c)
 * splits every access at the line boundaries, as the hardware does, so a
 * call is exactly one cache access (frame/memtrace.c stops the program
 * when a call breaks this rule). A line found in its set is a hit
 * (cache_hits). Otherwise it is a miss (cache_misses): the victim is an
 * invalid line of the set if there is one, else the least recently used;
 * a dirty victim is first written back with dram_line_write
 * (cache_writebacks); then dram_line_read brings the line in. */
void cache_read(uint64_t pa, void *dst, unsigned nbytes);

/* Lab 2: copy nbytes bytes from src to physical address pa, through the cache.
 * The bytes lie within one line, as for cache_read. The same search as
 * cache_read, including the read of the whole line on a miss (write
 * allocation); then the bytes go into the line and the line becomes dirty.
 * Nothing is written to memory here. */
void cache_write(uint64_t pa, const void *src, unsigned nbytes);

/* Lab 2: write every dirty line back with dram_line_write, count each in
 * cache_writebacks, and mark it clean. The lines stay valid. */
void cache_flush(void);

extern uint64_t cache_hits, cache_misses, cache_writebacks;     /* frame/cpu/dram.c */

/* frame/cpu/dram.c: physical memory, whole lines only */
int      dram_open(const char *path, size_t size);
void     dram_close(void);
uint8_t *dram_ptr(uint64_t pa, size_t nbytes);  /* NULL when out of range */
/* The file open as descriptor fd of the host becomes read-only memory at
   ROM_BASE, without being copied. Returns its size, 0 if that fails. */
uint64_t dram_rom(int fd);
/* One whole line, the one holding pa, read from or written to the DRAM of
   the host, counted in dram_reads and dram_writes. */
void     dram_line_read(uint64_t pa, uint8_t *dst);
void     dram_line_write(uint64_t pa, const uint8_t *src);
extern uint64_t dram_reads, dram_writes;

/* The machine without a cache, mini-cpu --cache 0. opt_nocache is set, and
   frame/memtrace.c sends every call of cache_read and cache_write to these
   two functions instead of the cache of lab 2 (cache_flush then does
   nothing). A call, within one line, is one transfer, made and counted
   like those of dram_line_read and dram_line_write: the bytes read come
   from the DRAM of the host, and only the bytes written go to it. */
extern int opt_nocache;
void uncached_read(uint64_t pa, void *dst, unsigned nbytes);
void uncached_write(uint64_t pa, const void *src, unsigned nbytes);

/* mini-cpu --stats: the lines "cache" and "dram" (frame/cpu/dram.c).
   Without a cache every line access is a transfer. */
void memory_stats(FILE *out);

/* -------------------------------------------------------- lab 3: mmu.c */

enum { PTE_P = 1u << 0, PTE_W = 1u << 1, PTE_U = 1u << 2,
       PTE_A = 1u << 5, PTE_D = 1u << 6 };

/* The low 48 bits of a virtual address are split as on x86-64:
 *
 *     47..39  PML4 index     38..30  PDPT index    29..21  PD index
 *     20..12  PT index       11..0   page offset
 *
 * Each level is a 4 KiB table of 512 eight-byte entries: the low 12 bits
 * of an entry are flags (PTE_*), bit 12 and up the physical address of the
 * next table or of the page frame. mmu_cr3() is the address of the PML4. */

/* Index of va in the table of level lvl, from 3 (PML4) down to 0 (PT). */
#define PT_INDEX(va, lvl) ((unsigned)(((va) >> (12 + 9 * (lvl))) & 0x1ff))
/* The physical address held in an entry. */
#define PTE_ADDR(entry)   ((entry) & 0x000ffffffffff000ULL)

/* Entry idx of the table at physical address table, read and written
   through the cache, like every other access to guest memory. */
static inline uint64_t pte_read(uint64_t table, unsigned idx)
{
    uint64_t entry;
    cache_read(table + idx * 8, &entry, 8);
    return entry;
}

static inline void pte_write(uint64_t table, unsigned idx, uint64_t entry)
{
    cache_write(table + idx * 8, &entry, 8);
}

/* Lab 3: translate va for an access of kind acc (MEM_FETCH, MEM_READ or
 * MEM_WRITE). Returns 0 and sets *pa, or -1 on a page fault.
 *
 *   1. Look up the page of va in the TLB; count the lookup in tlb_hits or
 *      tlb_misses.
 *   2. On a miss walk the four tables from mmu_cr3() with pte_read. An
 *      entry without PTE_P at any level is a page fault. In the entry of
 *      level 0 set PTE_A, and PTE_D as well for MEM_WRITE, and write the
 *      entry back with pte_write when that changed it. Put the page into
 *      the TLB with the flags of that entry.
 *   3. MEM_WRITE to a page without PTE_W is a page fault.
 *   4. *pa = frame + offset of va in the page.
 *
 * Every page fault adds one to page_faults. The size of the TLB and its
 * replacement are yours to choose, with one requirement: a page that was
 * just translated is found in the TLB by the next lookup, and stays there
 * until mmu_tlb_flush or until other pages push it out. */
int  mmu_translate(uint64_t va, int acc, uint64_t *pa);

/* Lab 3: empty the TLB. The framework calls it after it changed page
 * tables: the loader after mapping the program, brk after growing the
 * heap, arena_seal after removing write permission. */
void mmu_tlb_flush(void);

/* Lab 3: map the page of va to the frame of pa with the given flags. Walk
 * from mmu_cr3(); at levels 3 to 1 an entry without PTE_P gets a new table
 * from mmu_alloc_frame() (0 means out of memory: return -1), entered as
 * frame | PTE_P | PTE_W | PTE_U. The entry of level 0 becomes
 * (pa & ~0xfff) | flags | PTE_P, replacing whatever was there. Returns 0.
 * The caller flushes the TLB. */
int  mmu_map(uint64_t va, uint64_t pa, uint64_t flags);

extern uint64_t tlb_hits, tlb_misses, page_faults;      /* frame/cpu/cr3.c */

/* frame/cpu/cr3.c: the register that holds the root of the page tables;
   writing it empties the TLB. */
void     mmu_set_cr3(uint64_t pa);
uint64_t mmu_cr3(void);
/* frame/os/frames.c */
uint64_t mmu_alloc_frame(void);     /* a zeroed frame, 0 when memory is exhausted */

/* The stand-in before lab 3: entry pte of virtual page number vpn in the
   table on the host that frame/cpu/pagemap.c translates with, written by
   mmu_map of frame/os/pagemap.c. Returns 0, or -1 when the table is full. */
int      pagemap_set(uint64_t vpn, uint64_t pte);

/* ---------------------------------------------------- lab 4: objload.c */

/* A relocatable object file (gcc -c), mapped read-only, and where it was
   loaded. obj_open fills everything but the contents of secbase[] and
   value[], which start out as zeros. */
#include <elf.h>

typedef struct {
    const char *path;
    uint8_t    *data;               /* the whole file */
    size_t      size;
    Elf64_Ehdr *eh;
    Elf64_Shdr *sh;                 /* section headers, eh->e_shnum of them */
    const char *shstr;              /* section name string table */
    Elf64_Sym  *sym;                /* .symtab: nsym entries, inside the file */
    unsigned    nsym;
    const char *str;                /* string table of the symbol names */
    uint64_t   *secbase;            /* guest address of each section, 0 if not loaded */
    uint64_t   *value;              /* guest address of each symbol */
    int         loaded;             /* obj_load succeeded */
} obj;

int  obj_open(obj *object, const char *path);        /* frame/os/objfile.c */
void obj_close(obj *object);
/* The name of section sec_idx; of symbol sym_idx, a section symbol having
   the name of its section; of a relocation type, "?" for one not listed
   below. */
const char *obj_secname(const obj *object, unsigned sec_idx);
const char *obj_symname(const obj *object, unsigned sym_idx);
const char *obj_relname(unsigned type);

/* Load object into guest memory and relocate it: obj_sections, obj_symbols,
   obj_relocs, in this order. Returns 0, or -1 as soon as one of them does.
   With opt_verbose it prints every loaded section, and every relocation
   with the bytes found at its place afterwards. */
int      obj_load(mcpu *cpu, obj *object);
/* Guest address of a symbol defined by object, 0 if absent or not loaded. */
uint64_t obj_lookup(const obj *object, const char *name);
/* Guest address of code that jumps to target: 13 bytes in the code arena,
   one per target, as an entry of a PLT. Returns 0 on an error. */
uint64_t obj_stub(mcpu *cpu, uint64_t target);

/* Lab 4: copy the sections into guest memory. Section sec is loaded when
 * it has SHF_ALLOC and its size is not 0: reserve sh_size bytes aligned to
 * sh_addralign with arena_alloc (ARENA_CODE with SHF_EXECINSTR, else
 * ARENA_DATA), fill them with guest_write, from the file at sh_offset or
 * with zeros for SHT_NOBITS, and record the address in object->secbase[sec].
 * Returns 0, or -1 when arena_alloc returns 0 or guest_write fails. */
int obj_sections(mcpu *cpu, obj *object);

/* Lab 4: give every symbol its guest address in object->value[sym], sym from 1:
 *   st_shndx SHN_UNDEF   the host function of that name, host_lookup(name);
 *                        if there is none, print
 *                        "<object->path>: undefined symbol <name>" to stderr and
 *                        return -1. A symbol with an empty name keeps 0.
 *   SHN_ABS              st_value
 *   a loaded section     object->secbase[st_shndx] + st_value
 *   anything else        0
 * Returns 0 or -1. */
int obj_symbols(obj *object);

/* Lab 4: apply every relocation of every SHT_RELA section whose target
 * section (sh_info) is loaded. With S the value of the symbol, A the
 * addend and P = secbase of the target + r_offset, the address patched:
 *
 *   R_X86_64_64      8 bytes  S + A
 *   R_X86_64_32      4 bytes  S + A, must fit in 32 bits unsigned
 *   R_X86_64_32S     4 bytes  S + A, must fit in 32 bits signed
 *   R_X86_64_PC32    4 bytes  S + A - P, must fit in 32 bits signed
 *   R_X86_64_PLT32   4 bytes  S + A - P; when that does not fit, S is
 *                             replaced by obj_stub(cpu, S), code within reach
 *                             that jumps to S (0 from obj_stub is an error)
 *
 * The value is written at P with guest_write. A value that does not fit
 * prints "<object->path>: <obj_relname> against <obj_symname> overflows", a
 * type not in this table "<object->path>: unimplemented relocation type
 * <number>", both to stderr, and the function returns -1; so it does when
 * guest_write fails. Returns 0 when all relocations are applied. */
int obj_relocs(mcpu *cpu, obj *object);

/* Guest memory for sections, below 2 GB (frame/os/arena.c). */
enum { ARENA_DATA = 0, ARENA_CODE = 1 };
uint64_t arena_alloc(uint64_t size, uint64_t align, int kind);
void     arena_range(int kind, uint64_t *base, uint64_t *used);
/* Remove write permission from the code pages allocated so far. */
void     arena_seal(void);

/* Guest address of the entry of a host function such as nn_matmul,
   0 if the name is not exported (frame/os/hostcall.c). */
uint64_t host_lookup(const char *name);
#define HOSTCALL_BASE 0x00007f0000000000ULL
#define HOSTCALL_NR   0x1000        /* system call numbers of host functions */
/* Map the page of host function entries; run a host function. */
int hostcall_install(mcpu *cpu);
int hostcall(mcpu *cpu, unsigned idx);
extern uint64_t hostcall_count;

extern int opt_verbose;             /* mini-jit -v */

/* ----------------------------------------------- lab 5: syscall.c, blk.c */

/* The system calls follow the Linux x86-64 convention: the number in rax,
 * the arguments in rdi rsi rdx r10 r8 r9, the result in rax, a negative
 * result being a negated errno. sys_dispatch (frame/os/sysbase.c) implements
 * write, brk, getpid, exit, fstat of the descriptors 0 to 2, and mmap of
 * standard input (read-only: the file appears in the address space of the
 * guest without being copied); it passes read, open and close to the
 * three functions below; before lab 5 these three return -ENOSYS.
 *
 * A pointer argument is a guest virtual address: the host reaches the
 * bytes with guest_read and guest_write, through the page tables and the
 * cache of the guest. */
int  sys_dispatch(mcpu *cpu);
extern uint64_t syscall_count;      /* frame/os/sysbase.c */

/* Lab 5: read(fd, buf, count). Descriptors 0, 1 and 2 are those of the host:
 * read at most 4096 bytes with read(2) into a buffer of the host and copy
 * what arrived to the guest at buf with guest_write. Returns the count,
 * -EIO if the host read fails, -EFAULT if the copy fails. Any other
 * descriptor is a file of the ramfs: ramfs_read, and -EBADF when it
 * returns -1. */
long sys_read(mcpu *cpu, uint64_t fd, uint64_t buf, uint64_t count);

/* Lab 5: open(path). path is the guest address of a NUL-terminated name:
 * read it one byte at a time with guest_read until the NUL or 63 bytes,
 * whichever comes first; -EFAULT if a byte cannot be read. Then ramfs_open; -ENOENT when
 * it returns -1, otherwise the descriptor. */
long sys_open(mcpu *cpu, uint64_t path);

/* Lab 5: close(fd). ramfs_close; 0, or -EBADF when it returns -1. */
long sys_close(mcpu *cpu, uint64_t fd);

/* The block device. Its whole interface is a ring in guest physical
 * memory: the driver places a request in desc[tail % BLK_RING], advances
 * tail and sets doorbell; the device processes desc[head % BLK_RING] up to
 * tail, advancing head. Data buffers are physical addresses: the device
 * does not go through the page tables, as with DMA on a real machine. */
#define BLK_SECTOR 512u
#define BLK_RING   16u

enum { BLK_IN = 0, BLK_OUT = 1 };

typedef struct {
    uint32_t type;                  /* BLK_IN reads the disk, BLK_OUT writes it */
    uint32_t reserved;
    uint64_t sector;
    uint64_t addr;                  /* guest physical address of the data */
    uint32_t len;
    uint32_t status;                /* 0 pending, 1 done, 2 error */
} blk_req;

typedef struct {
    uint32_t head, tail;            /* the driver advances tail, the device head */
    uint32_t ndesc;
    uint32_t doorbell;
    blk_req  desc[BLK_RING];
} blk_ring;

/* frame/os/blkdrv.c, the driver: open the image and put the ring at
   physical address ring_pa. Returns 0, or -1 if the image cannot be
   opened. */
int  blk_attach(const char *image, uint64_t ring_pa);
/* The driver side: place one request on the ring, set the doorbell and
   ring it, which calls blk_kick. Returns 0 if the device reports success,
   else -1. */
int  blk_request(uint32_t type, uint64_t sector, uint64_t pa, uint32_t len);
/* frame/cpu/blkdev.c, the device: connect it to the image and tell it
   where the ring is (-1 if the image cannot be opened); the doorbell,
   which processes the ring with blk_kick (-1 before blkdev_open). */
int  blkdev_open(const char *image, uint64_t ring_pa);
int  blkdev_doorbell(void);
extern uint64_t blk_reads, blk_writes;  /* frame/cpu/blkdev.c */

/* Lab 5: the device side. Process every request of ring from head to
 * tail against the image open as fd, then clear ring->doorbell. For a
 * request req, in this order:
 *   - its buffer is req->len bytes of guest physical memory at req->addr:
 *     dram_ptr(req->addr, req->len). NULL means the buffer is not inside
 *     guest memory: status 2, and no byte of the host is read or written.
 *     On a real machine the IOMMU makes this check.
 *   - BLK_IN: pread req->len bytes at offset req->sector * BLK_SECTOR into
 *     the buffer; bytes beyond the end of the image read as zeros. Count
 *     it in blk_reads.
 *   - BLK_OUT: pwrite the buffer at that offset. Count it in blk_writes.
 *   - status 1, or 2 if pread or pwrite failed (and then no count);
 *     advance ring->head. */
void blk_kick(blk_ring *ring, int fd);

/* The ramfs: read-only files on the block device. Layout of the image,
 * written by tools/mkfs.py:
 *
 *     sector 0    magic[8] "MCPUFS01", uint32 nfile, then nfile entries of
 *                 32 bytes: name[24], uint32 first sector, uint32 bytes
 *     sector 1..  file contents, each file starting on a sector boundary
 */
#define RAMFS_MAGIC   "MCPUFS01"
#define RAMFS_NAMELEN 24
#define RAMFS_MAXFILE 16
#define RAMFS_MAXFD   8             /* files open at the same time */

typedef struct {
    char     name[RAMFS_NAMELEN];
    uint32_t start;                 /* first sector */
    uint32_t size;                  /* bytes */
} ramfs_dirent;

extern ramfs_dirent ramfs_dir[RAMFS_MAXFILE];   /* frame/os/blkdrv.c */
extern unsigned     ramfs_ndir;

/* Lab 5: read the directory. Take one frame with mmu_alloc_frame() as the
 * bounce page, the buffer of every request this file makes, and keep its
 * address. Read sector 0 into it with blk_request, check the magic (if it
 * differs print "ramfs: bad magic in the image" to stderr), and copy the
 * file count and the entries into ramfs_ndir and ramfs_dir, at most
 * RAMFS_MAXFILE of them. The bytes of the page are at
 * dram_ptr(page, BLK_SECTOR). Returns 0, or -1 on any failure. */
int  ramfs_mount(void);

/* Lab 5: open the file called name. Returns a descriptor, 3 or above, at
 * position 0, or -1 if there is no such file or RAMFS_MAXFD files are
 * open already. A closed descriptor can be handed out again. */
int  ramfs_open(const char *name);

/* Lab 5: read up to count bytes of file fd from its position into guest
 * memory at virtual address va, and advance the position. Returns the
 * number of bytes read: 0 at the end of the file, fewer than count near it.
 * Returns -1 for a descriptor that is not open.
 *
 * One sector at a time: blk_request(BLK_IN, sector, bounce page,
 * BLK_SECTOR), then guest_write of the part of the sector that belongs to
 * this read. The device takes a physical address while va is virtual, and
 * a guest buffer may span frames that are not adjacent; hence the bounce
 * page. If a step fails, return the bytes copied so far, with the position
 * advanced by as many, or -1 if none. */
long ramfs_read(int fd, uint64_t va, long count, mcpu *cpu);

/* Lab 5: close descriptor fd. Returns 0, or -1 if it is not open. */
int  ramfs_close(int fd);

/* mini-cpu --ls: one line per entry of ramfs_dir (frame/os/blkdrv.c). */
void ramfs_list(FILE *out);

/* -------------------------------------------------------------- framework */

/* frame/cpu/mem.c, the memory access unit of the CPU: one access of an
   instruction at a virtual address, nbytes at most LINE_SIZE (a longer
   one ends the program with status 4), translated page by page with
   mmu_translate and made line by line with cache_read or cache_write.
   Returns -1 and sets cpu->fault_va if a page is absent; never sets
   cpu->fault. */
int mem_read(mcpu *cpu, uint64_t va, void *dst, unsigned nbytes, int acc);
int mem_write(mcpu *cpu, uint64_t va, const void *src, unsigned nbytes);

/* Copy up to MAX_INSN bytes at pc into bytes; returns how many were readable. */
unsigned fetch_bytes(mcpu *cpu, uint64_t pc, uint8_t *bytes);

/* frame/os/copy.c: software copies a range of guest virtual memory of any
   length, as copy_from_user and copy_to_user do, cut at line boundaries
   into one mem_read or mem_write per line. guest_write with src NULL
   writes nbytes zero bytes. Both return 0, or -1 with cpu->fault_va set
   if a page is absent; the pieces before it have been copied. */
int guest_read(mcpu *cpu, uint64_t va, void *dst, uint64_t nbytes);
int guest_write(mcpu *cpu, uint64_t va, const void *src, uint64_t nbytes);

/* The syscall instruction: host functions (lab 4) or sys_dispatch
   (frame/os/trap.c). */
int  syscall_enter(mcpu *cpu);

/* The commit log line of the instruction at pc (x86_end writes it): the
   store, if any, then every register that differs from the previous line
   (frame/cpu/commit.c). */
void commit_line(const mcpu *cpu, uint64_t pc, int store, uint64_t sa,
                 const u256 *sval, int msize);
/* The registers the first line of the commit log is compared with. */
void commit_start(const mcpu *cpu);
void cpu_dump(const mcpu *cpu, FILE *out);
const char *reg_name(int reg);

/* Load the ELF executable at path with its arguments (frame/os/loader.c). */
int  loader_exec(mcpu *cpu, const char *path, char **argv, int argc);

extern FILE *commit_out;
extern FILE *memtrace_out;          /* frame/memtrace.c, mini-cpu --memtrace */
extern int opt_steps;               /* mini-cpu -v: one line per instruction */

#endif
