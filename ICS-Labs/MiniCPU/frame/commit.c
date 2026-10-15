/* What leaves the CPU: the commit log, the timeline, faults.
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
 * rip is left out: the next line starts with it. The log is built from the
 * registers themselves, so the interpreter of lab 1, seq.c and the pipeline
 * of lab 2 must produce the same log, byte for byte. The timeline (lab 2)
 * adds the cycle in which the instruction entered each stage;
 * tools/pipemodel.py reads it.
 */
#include <string.h>

#include "mcpu.h"

FILE *commit_out, *timeline_out;
int opt_verbose, opt_steps;
int opt_forward = 1;
uint64_t opt_max;
unsigned opt_miss = 10;

static const char *REG[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "flags", "rip",
    "ymm0", "ymm1", "ymm2", "ymm3", "ymm4", "ymm5", "ymm6", "ymm7",
    "ymm8", "ymm9", "ymm10", "ymm11", "ymm12", "ymm13", "ymm14", "ymm15"
};

static const char *CLS[] = {
    "ALU", "MOV", "LOAD", "STORE", "MUL", "DIV",
    "BR", "JMP", "CALL", "RET", "SYS", "NOP"
};

const char *reg_name(int n)
{
    return n >= 0 && n < NARCH ? REG[n] : "?";
}

static void reglist(FILE *f, const uint8_t *v, unsigned n)
{
    if (!n)
        fputc('-', f);
    for (unsigned i = 0; i < n; ++i)
        fprintf(f, "%s%s", i ? "," : "", reg_name(v[i]));
}

static uint64_t last[NREG + 1];         /* rax .. r15, flags of the previous line */
static u256     ylast[NVEC];

void commit_start(const mcpu *c)
{
    memcpy(last, c->R, sizeof last);
    memcpy(ylast, c->Y, sizeof ylast);
}

/* The low n bytes of v as one hexadecimal number. */
static void hex(FILE *f, const u256 *v, int n)
{
    u256 t = { { 0 } };
    memcpy(t.b, v->b, (size_t)n);
    int k = 3;
    while (k > 0 && !t.q[k])
        --k;
    fprintf(f, "%lx", (unsigned long)t.q[k]);
    while (k-- > 0)
        fprintf(f, "%016lx", (unsigned long)t.q[k]);
}

void commit_line(const mcpu *c, uint64_t pc, int store, uint64_t sa,
                 const u256 *sval, int msize)
{
    if (!commit_out)
        return;
    fprintf(commit_out, "%lx", (unsigned long)pc);
    if (store) {
        fprintf(commit_out, " m[%lx]=", (unsigned long)sa);
        hex(commit_out, sval, msize);
    }
    for (int n = 0; n <= FLAGS_REG; ++n)
        if (c->R[n] != last[n]) {
            fprintf(commit_out, " %s=%lx", reg_name(n), (unsigned long)c->R[n]);
            last[n] = c->R[n];
        }
    for (int n = 0; n < NVEC; ++n)
        if (memcmp(&c->Y[n], &ylast[n], sizeof ylast[n])) {
            fprintf(commit_out, " %s=", reg_name(YMM0 + n));
            hex(commit_out, &c->Y[n], 32);
            ylast[n] = c->Y[n];
        }
    fputc('\n', commit_out);
}

void cpu_commit(mcpu *c, const insn *d)
{
    ++c->icount;
    commit_line(c, d->pc, d->store, d->sa, &d->sval, d->msize);
    if (timeline_out) {
        fprintf(timeline_out, "%lx %u %s %u %lx %lx %u %u %u %u ",
                (unsigned long)d->pc, d->len, CLS[d->cls], d->br,
                (unsigned long)d->target, (unsigned long)d->next, d->taken,
                insn_late(d), d->exec, d->mcyc);
        reglist(timeline_out, d->src, d->nsrc);
        fputc(' ', timeline_out);
        reglist(timeline_out, d->dst, d->ndst);
        for (int k = 0; k < 5; ++k)
            fprintf(timeline_out, " %lu", (unsigned long)d->t[k]);
        fputc('\n', timeline_out);
    }
    if (opt_steps)
        fprintf(stderr, "%08lx  %-5s rax=%-16lx rsp=%-12lx rdi=%lx\n",
                (unsigned long)d->pc, CLS[d->cls], (unsigned long)c->r[RAX],
                (unsigned long)c->r[RSP], (unsigned long)c->r[RDI]);
}

void cpu_trap(mcpu *c, const insn *d)
{
    c->fault = d->fault;
    c->fault_va = d->fault_va;
    c->rip = d->pc;
}

void cpu_dump(const mcpu *c, FILE *f)
{
    for (int i = 0; i < NREG; ++i)
        fprintf(f, "%-4s %016lx%s", REG[i], (unsigned long)c->r[i],
                i % 4 == 3 ? "\n" : "  ");
    fprintf(f, "rip  %016lx  flags %c%c%c%c%c\n", (unsigned long)c->rip,
            c->rflags & CF ? 'C' : '-', c->rflags & PF ? 'P' : '-',
            c->rflags & ZF ? 'Z' : '-', c->rflags & SF ? 'S' : '-',
            c->rflags & OF ? 'O' : '-');
    for (int i = 0; i < NVEC; ++i) {                    /* the nonzero vector registers */
        const u256 *y = &c->Y[i];
        if (y->q[0] | y->q[1] | y->q[2] | y->q[3])
            fprintf(f, "%-5s %016lx %016lx %016lx %016lx\n", REG[YMM0 + i],
                    (unsigned long)y->q[3], (unsigned long)y->q[2],
                    (unsigned long)y->q[1], (unsigned long)y->q[0]);
    }
}
