/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <common/types.h>
#include <mm/vmspace.h>
#include <object/thread.h>
#include <ipc/notification.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <string.h>

void sys_exit_group(int code)
{
}

int trans_uva_to_kva(u64 uva, u64 *kva)
{
        return 0;
}

int signal_notific(struct notification *n)
{
        return 0;
}

void futex_init(void *ptr)
{       
}

void futex_deinit(void *ptr)
{       
}

int sys_futex_wake(int *uaddr, int futex_op, int val)
{
        return 0;
}

void printk(const char *fmt, ...)
{
        va_list va;

        va_start(va, fmt);
        vprintf(fmt, va);
        va_end(va);
}

void *kmalloc(size_t size)
{
        return malloc(size);
}

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif
void *get_pages(int order)
{
        size_t size;

        size = (1 << order) * PAGE_SIZE;
        return aligned_alloc(PAGE_SIZE, size);
}

void *kzalloc(size_t size)
{
        void *ptr;

        ptr = kmalloc(size);
        memset(ptr, 0, size);
        return ptr;
}

void kfree(void *ptr)
{
        free(ptr);
}

int thread_init(struct thread *thread, struct cap_group *cap_group, u64 stack,
                u64 pc, u32 prio, u32 type, s32 aff);
struct thread *create_user_thread_stub(struct cap_group *cap_group)
{
        cap_t r;
        struct thread *thread;

        thread = obj_alloc(TYPE_THREAD, sizeof(*thread));
        assert(thread != NULL);
        thread_init(thread, cap_group, 0, 0, 0, TYPE_USER, 0);
        r = cap_alloc(cap_group, thread);
        assert(r > 0);

        return thread;
}

#ifndef DEFAULT_KERNEL_STACK_SZ
#define DEFAULT_KERNEL_STACK_SZ 4096
#endif
struct thread_ctx *create_thread_ctx(u32 type)
{
        return kmalloc(DEFAULT_KERNEL_STACK_SZ);
}

void destroy_thread_ctx(struct thread *thread)
{
        kfree(thread->thread_ctx);
}

int vmspace_init(struct vmspace *vmspace, unsigned long pcid)
{
        return 0;
}

int pmo_init(struct pmobject *pmo, pmo_type_t type, size_t len, paddr_t paddr)
{
        return 0;
}

cap_t create_pmo(size_t size, pmo_type_t type, struct cap_group *cap_group,
                 paddr_t paddr, struct pmobject **new_pmo, cap_right_t rights)
{
        return 0;
}

void init_thread_ctx(struct thread *thread, u64 stack, u64 func, u32 prio,
                     u32 type, s32 aff)
{
}

struct elf_file *elf_parse_file(const char *code)
{
        return NULL;
}

int vmspace_map_range(struct vmspace *vmspace, vaddr_t va, size_t len,
                      vmr_prop_t flags, struct pmobject *pmo)
{
        return 0;
}

void flush_idcache(void)
{
}

u32 smp_get_cpu_id(void)
{
        return 0;
}

int fake_sched_init(void)
{
        return 0;
}
int fake_sched(void)
{
        return 0;
}
int fake_sched_enqueue(struct thread *thread)
{
        return 0;
}
int fake_sched_dequeue(struct thread *thread)
{
        return 0;
}
struct sched_ops fake_sched_ops = {.sched_init = fake_sched_init,
                                   .sched = fake_sched,
                                   .sched_enqueue = fake_sched_enqueue,
                                   .sched_dequeue = fake_sched_dequeue};
struct sched_ops *cur_sched_ops = &fake_sched_ops;

void switch_vmspace_to(struct vmspace *vmspace)
{
}

int read_bin_from_fs(char *path, char **buf)
{
        return 0;
}
int fs_read(const char *path, off_t offset, void *buf, size_t count)
{
        return 0;
}

struct thread *current_threads[PLAT_CPU_NUM];

const char binary_server_bin_start;
const char binary_user_bin_start;
const char binary_sh_bin_start;

ssize_t fs_get_size(const char *path)
{
        return 0;
}
int copy_to_user(void *dst, void *src, size_t size)
{
        memcpy(dst, src, size);
        return 0;
}
int copy_from_user(void *dst, void *src, size_t size)
{
        memcpy(dst, src, size);
        return 0;
}

int sys_map_pmo(cap_t a, cap_t b, unsigned long c,
                unsigned long d, unsigned long e)
{
        return 0;
}
const char binary_procmgr_bin_start = 0;
const unsigned long binary_procmgr_bin_size = 0;
void *cpio_extract_single(const void *addr, const char *target,
                          void *(*f)(const void *start, size_t size,
                                     void *data),
                          void *data)
{
        return NULL;
}
void arch_set_thread_arg0(struct thread *thread, unsigned long arg)
{
        return;
}
void arch_set_thread_tls(struct thread *thread, unsigned long tls)
{
        return;
}
void set_thread_arch_spec_state(struct thread *thread)
{
        return;
}

s32 atomic_cmpxchg_32(s32 *ptr, s32 oldval, s32 newval)
{
        return 0;
}

u64 switch_context(void)
{
        return 0;
}
void eret_to_thread(u64 sp)
{
}

void irq_deinit(void *ptr)
{
}
void pmo_deinit(void *ptr)
{
}
void connection_deinit(void *ptr)
{
}
void notification_deinit(void *ptr)
{
}
void vmspace_deinit(void *ptr)
{
}
void ptrace_deinit(void *ptr)
{
}
