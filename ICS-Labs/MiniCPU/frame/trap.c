/* The syscall instruction.
 *
 * Numbers below HOSTCALL_NR are Linux system calls and go to sys_dispatch.
 * Numbers from HOSTCALL_NR up call a host function exported to guest code
 * loaded in lab 3 (frame/hostcall.c); a program run by mini-cpu never
 * uses them.
 */
#include "mcpu.h"

int syscall_enter(mcpu *c)
{
#if LAB >= 3
    if (c->r[RAX] >= HOSTCALL_NR)
        return hostcall(c, (unsigned)(c->r[RAX] - HOSTCALL_NR));
#endif
    return sys_dispatch(c);
}
