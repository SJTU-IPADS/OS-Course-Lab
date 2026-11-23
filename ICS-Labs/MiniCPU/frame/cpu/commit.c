/* Hardware: what leaves the CPU, the commit log of the instructions that
 * retire and the dump of the registers after a fault.
 *
 * The commit log has one line per instruction: its address, the store if
 * it made one, then every register whose value differs from the previous
 * line, in the order rax .. r15, flags, ymm0 .. ymm15, all in hexadecimal:
 *
 *     401136 rax=2a flags=46
 *     40113a m[7fffffffefd8]=401140 rsp=7fffffffefd8
 *     4011a0 ymm2=600000005000000040000000300000002
 *
 * A ymm register and a vector store are printed as one 256-bit number,
 * without leading zeros: lane 0 is the rightmost 8 digits.
 *
 * rip is left out: the next line starts with it. The tests compare the
 * checksum of a log with that of a correct interpreter
 * (tests/lab1/expected.txt).
 */
#include <string.h>

#include "mcpu.h"

FILE *commit_out;
int opt_verbose, opt_steps;
uint64_t opt_max;

static const char *REG[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "flags", "rip",
    "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7",
    "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
};

const char *reg_name(int reg)
{
    return reg >= 0 && reg < NARCH ? REG[reg] : "?";
}

static uint64_t last[NREG + 1];         /* rax .. r15, flags of the previous line */
static u256     ylast[NVEC];

void commit_start(const mcpu *cpu)
{
    memcpy(last, cpu->R, sizeof last);
    memcpy(ylast, cpu->Y, sizeof ylast);
}

/* The low nbytes of value as one hexadecimal number. */
static void hex(FILE *out, const u256 *value, int nbytes)
{
    u256 low = { { 0 } };
    memcpy(low.b, value->b, (size_t)nbytes);
    int lane = 3;
    while (lane > 0 && !low.q[lane])
        --lane;
    fprintf(out, "%lx", (unsigned long)low.q[lane]);
    while (lane-- > 0)
        fprintf(out, "%016lx", (unsigned long)low.q[lane]);
}

void commit_line(const mcpu *cpu, uint64_t pc, int store, uint64_t sa,
                 const u256 *sval, int msize)
{
    if (!commit_out)
        return;
    fprintf(commit_out, "%lx", (unsigned long)pc);
    if (store) {
        fprintf(commit_out, " m[%lx]=", (unsigned long)sa);
        hex(commit_out, sval, msize);
    }
    for (int reg = 0; reg <= FLAGS_REG; ++reg)
        if (cpu->R[reg] != last[reg]) {
            fprintf(commit_out, " %s=%lx", reg_name(reg), (unsigned long)cpu->R[reg]);
            last[reg] = cpu->R[reg];
        }
    for (int ymm = 0; ymm < NVEC; ++ymm)
        if (memcmp(&cpu->Y[ymm], &ylast[ymm], sizeof ylast[ymm])) {
            fprintf(commit_out, " %s=", reg_name(YMM0 + ymm));
            hex(commit_out, &cpu->Y[ymm], 32);
            ylast[ymm] = cpu->Y[ymm];
        }
    fputc('\n', commit_out);
}

void cpu_dump(const mcpu *cpu, FILE *out)
{
    for (int reg = 0; reg < NREG; ++reg)
        fprintf(out, "%-4s %016lx%s", REG[reg], (unsigned long)cpu->r[reg],
                reg % 4 == 3 ? "\n" : "  ");
    fprintf(out, "rip  %016lx  flags %c%c%c%c%c\n", (unsigned long)cpu->rip,
            cpu->rflags & CF ? 'C' : '-', cpu->rflags & PF ? 'P' : '-',
            cpu->rflags & ZF ? 'Z' : '-', cpu->rflags & SF ? 'S' : '-',
            cpu->rflags & OF ? 'O' : '-');
    for (int ymm = 0; ymm < NVEC; ++ymm) {              /* the nonzero vector registers */
        const u256 *value = &cpu->Y[ymm];
        if (value->q[0] | value->q[1] | value->q[2] | value->q[3])
            fprintf(out, "%-5s %016lx %016lx %016lx %016lx\n", REG[YMM0 + ymm],
                    (unsigned long)value->q[3], (unsigned long)value->q[2],
                    (unsigned long)value->q[1], (unsigned long)value->q[0]);
    }
}
