#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>

struct reg_struct
{
    int idx;
    int size;
};

#define ARCH_REG_NUM (sizeof(regs_map) / sizeof(struct reg_struct))

#ifdef __i386__

#error "register struct not supported"

#include <sys/reg.h>

#define SZ 4
#define FEATURE_STR "l<target version=\"1.0\"><architecture>i386</architecture></target>"
static uint8_t break_instr[] = {0xcc};

#define PC EIP
#define EXTRA_NUM 41
#define EXTRA_REG ORIG_EAX
#define EXTRA_SIZE 4

typedef struct user_regs_struct regs_struct;

// gdb/features/i386/32bit-core.c
struct reg_struct regs_map[] = {
    {EAX, 4},
    {ECX, 4},
    {EDX, 4},
    {EBX, 4},
    {UESP, 4},
    {EBP, 4},
    {ESI, 4},
    {EDI, 4},
    {EIP, 4},
    {EFL, 4},
    {CS, 4},
    {SS, 4},
    {DS, 4},
    {ES, 4},
    {FS, 4},
    {GS, 4},
};

#endif /* __i386__ */

#ifdef __x86_64__

#define SZ 8
#define FEATURE_STR "l<target version=\"1.0\"><architecture>i386:x86-64</architecture></target>"
static uint8_t break_instr[] = {0xcc};

typedef struct {
	unsigned long dummy_reserved, ds, r15, r14, r13, r12, rbp, rbx;
	unsigned long r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi, trapno;
	unsigned long dummy_ec, rip, cs, eflags, rsp, ss;
	unsigned long fs_base, gs_base, es, fs, gs;
} regs_struct;

#define RESERVE  0
#define DS  1
#define R15  2
#define R14  3
#define R13  4
#define R12  5
#define RBP  6
#define RBX  7
#define R11  8
#define R10  9
#define R9  10
#define R8  11
#define RAX  12
#define RCX  13
#define RDX  14
#define RSI  15
#define RDI  16
#define TRAPNO  17
#define EC  18
#define RIP  19
#define CS  20
#define EFLAGS  21
#define RSP  22
#define SS  23

#define PC RIP
#define EXTRA_NUM 57
#define EXTRA_REG RESERVE
#define EXTRA_SIZE 8

// gdb/features/i386/64bit-core.c
struct reg_struct regs_map[] = {
    {RAX, 8},
    {RBX, 8},
    {RCX, 8},
    {RDX, 8},
    {RSI, 8},
    {RDI, 8},
    {RBP, 8},
    {RSP, 8},
    {R8, 8},
    {R9, 8},
    {R10, 8},
    {R11, 8},
    {R12, 8},
    {R13, 8},
    {R14, 8},
    {R15, 8},
    {RIP, 8},
    {EFLAGS, 4},
    {CS, 4},
    {SS, 4},
    {DS, 4},
    {RESERVE, 4},
    {RESERVE, 4},
    {RESERVE, 4},
};

#endif /* __x86_64__ */

#ifdef __arm__

#error "register struct not supported"

#define SZ 4
#define FEATURE_STR "l<target version=\"1.0\"><architecture>arm</architecture></target>"

static uint8_t break_instr[] = {0xf0, 0x01, 0xf0, 0xe7};

#define PC 15
#define EXTRA_NUM 25
#define EXTRA_REG 16
#define EXTRA_SIZE 4

typedef struct user_regs regs_struct;

struct reg_struct regs_map[] = {
    {0, 4},
    {1, 4},
    {2, 4},
    {3, 4},
    {4, 4},
    {5, 4},
    {6, 4},
    {7, 4},
    {8, 4},
    {9, 4},
    {10, 4},
    {11, 4},
    {12, 4},
    {13, 4},
    {14, 4},
    {15, 4},
};

#endif /* __arm__ */

#ifdef __powerpc__

#error "register struct not supported"

#define SZ 4
#define FEATURE_STR "l<target version=\"1.0\">\
  <architecture>powerpc:common</architecture>\
  <feature name=\"org.gnu.gdb.power.core\">\
    <reg name=\"r0\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r1\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r2\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r3\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r4\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r5\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r6\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r7\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r8\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r9\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r10\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r11\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r12\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r13\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r14\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r15\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r16\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r17\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r18\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r19\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r20\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r21\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r22\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r23\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r24\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r25\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r26\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r27\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r28\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r29\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r30\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"r31\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\
    <reg name=\"msr\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"orig_r3\" bitsize=\"32\" type=\"int\"/>\
    <reg name=\"ctr\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"lr\" bitsize=\"32\" type=\"code_ptr\"/>\
    <reg name=\"xer\" bitsize=\"32\" type=\"uint32\"/>\
    <reg name=\"cr\" bitsize=\"32\" type=\"uint32\"/>\
</feature>\
</target>"

static uint8_t break_instr[] = {};

#define PC 32
#define EXTRA_NUM -1
#define EXTRA_REG -1
#define EXTRA_SIZE -1

typedef struct pt_regs regs_struct;

struct reg_struct regs_map[] = {
    {0, 4},
    {1, 4},
    {2, 4},
    {3, 4},
    {4, 4},
    {5, 4},
    {6, 4},
    {7, 4},
    {8, 4},
    {9, 4},
    {10, 4},
    {11, 4},
    {12, 4},
    {13, 4},
    {14, 4},
    {15, 4},
    {16, 4},
    {17, 4},
    {18, 4},
    {19, 4},
    {20, 4},
    {21, 4},
    {22, 4},
    {23, 4},
    {24, 4},
    {25, 4},
    {26, 4},
    {27, 4},
    {28, 4},
    {29, 4},
    {30, 4},
    {31, 4},
    {32, 4},
    {33, 4},
    {34, 4},
    {35, 4},
    {36, 4},
    {37, 4},
    {38, 4},
};

#endif /* __powerpc__ */

#ifdef __sparc__

#define SZ 4
#define FEATURE_STR "l<target version=\"1.0\"><architecture>sparc</architecture></target>"
static uint8_t break_instr[] = {0x91, 0xd0, 0x20, 0x12};

typedef struct {
    unsigned long g0, g1, g2, g3, g4, g5, g6, g7;
    unsigned long o0, o1, o2, o3, o4, o5, o6, o7;
    unsigned long l0, l1, l2, l3, l4, l5, l6, l7;
    unsigned long i0, i1, i2, i3, i4, i5, i6, i7;
    unsigned long y, psr, wim, tbr, pc, npc, fsr, csr;
} regs_struct;

#define Y 32
#define PSR 33
#define WIM 34
#define TBR 35
#define PC 36
#define NPC 37
#define FSR 38
#define CSR 39
#define FPU 40
#define EXTRA_NUM -1
#define EXTRA_REG -1
#define EXTRA_SIZE 4

struct reg_struct regs_map[] = {
    {0, 4},{1, 4},{2, 4},{3, 4},{4, 4},{5, 4},{6, 4},{7, 4},
    {8, 4},{9, 4},{10, 4},{11, 4},{12, 4},{13, 4},{14, 4},{15, 4},
    {16, 4},{17, 4},{18, 4},{19, 4},{20, 4},{21, 4},{22, 4},{23, 4},
    {24, 4},{25, 4},{26, 4},{27, 4},{28, 4},{29, 4},{30, 4},{31, 4},
    {FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},
    {FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},
    {FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},
    {FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},{FPU, 4},
    {Y, 4},{PSR, 4},{WIM, 4},{TBR, 4},{PC, 4},{NPC, 4},{FSR, 4},{CSR, 4}
};

#endif /* __sparc__ */

#endif /* ARCH_H */
