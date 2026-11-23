/* Software: the entry of the kernel for the syscall instruction.
 *
 * The instruction itself is executed by x86_other (frame/cpu/x86rest.c),
 * which calls syscall_enter. Numbers below HOSTCALL_NR are Linux system
 * calls and go to sys_dispatch (frame/os/sysbase.c). Numbers from
 * HOSTCALL_NR up call a host function exported to guest code loaded in
 * lab 4 (frame/os/hostcall.c); a program run by mini-cpu never uses them.
 */
#include "mcpu.h"

int syscall_enter(mcpu *cpu)
{
#if LAB >= 4
    if (cpu->r[RAX] >= HOSTCALL_NR)
        return hostcall(cpu, (unsigned)(cpu->r[RAX] - HOSTCALL_NR));
#endif
    return sys_dispatch(cpu);
}
