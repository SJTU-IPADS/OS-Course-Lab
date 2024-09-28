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

#include "liblaunch.h"
#include <chcore/type.h>
#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <chcore/pmo.h>
#include <chcore/aslr.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

/* An example to launch an (libc) application process */

#define AT_NULL     0 /* end of vector */
#define AT_IGNORE   1 /* entry should be ignored */
#define AT_EXECFD   2 /* file descriptor of program */
#define AT_PHDR     3 /* program headers for program */
#define AT_PHENT    4 /* size of program header entry */
#define AT_PHNUM    5 /* number of program headers */
#define AT_PAGESZ   6 /* system page size */
#define AT_BASE     7 /* base address of interpreter */
#define AT_FLAGS    8 /* flags */
#define AT_ENTRY    9 /* entry point of program */
#define AT_NOTELF   10 /* program is not ELF */
#define AT_UID      11 /* real uid */
#define AT_EUID     12 /* effective uid */
#define AT_GID      13 /* real gid */
#define AT_EGID     14 /* effective gid */
#define AT_PLATFORM 15 /* string identifying CPU for optimizations */
#define AT_HWCAP    16 /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK   17 /* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE               23 /* secure mode boolean */
#define AT_BASE_PLATFORM        24
/* string identifying real platform, may differ from AT_PLATFORM. */
#define AT_RANDOM 25 /* address of 16 random bytes */
#define AT_HWCAP2 26 /* extension of AT_HWCAP */

#define AT_EXECFN 31 /* filename of program */

#define INIT_ENV_SIZE   PAGE_SIZE
#define RANDOM_OFF 64 /* offset of a random number */

const char PLAT[] = "aarch64";

/* enviroments */
#define SET_LD_LIB_PATH
/* A user can mount external storage on /user/lib for dynamic libraries. */
const char LD_LIB_PATH[] = "LD_LIBRARY_PATH=/:/lib:/user/lib";

struct env_buf {
        unsigned long *entries_tail;
        unsigned long *entries_end;
        char *strings_tail;
        char *strings_end;
        unsigned long strings_offset;
};

static void env_buf_append_int(struct env_buf *env_buf, unsigned long val)
{
        assert(env_buf->entries_tail < env_buf->entries_end);
        *(env_buf->entries_tail++) = val;
}

static void env_buf_append_str(struct env_buf *env_buf, const char *val)
{
        size_t val_size = strlen(val) + 1;

        assert(env_buf->strings_tail + val_size <= env_buf->strings_end);
        strlcpy(env_buf->strings_tail, val, val_size);
        env_buf_append_int(env_buf,
                           (unsigned long)env_buf->strings_tail
                                   + env_buf->strings_offset);
        env_buf->strings_tail += val_size;
}

static void env_buf_append_int_auxv(struct env_buf *env_buf, unsigned long type,
                                    unsigned long val)
{
        env_buf_append_int(env_buf, type);
        env_buf_append_int(env_buf, val);
}

static void env_buf_append_str_auxv(struct env_buf *env_buf, unsigned long type,
                                    const char *val)
{
        env_buf_append_int(env_buf, type);
        env_buf_append_str(env_buf, val);
}

#define FAKE_UGID       1000
#define FAKE_CLKTLK     100

/**
 * NOTE: The stack format:
 * http://articles.manugarg.com/aboutelfauxiliaryvectors.html
 * The url shows a 32-bit format stack, but we implemented a 64-bit stack below.
 * People as smart as you could understand the difference.
 */
static void construct_init_env(char *env, vaddr_t top_vaddr,
                               struct process_metadata *meta, char *name,
                               struct pmo_map_request *pmo_map_reqs,
                               int nr_pmo_map_reqs, cap_t caps[], int nr_caps,
                               int argc, char **argv,
                               int envp_argc, char ** envp_argv,
                               int pid, unsigned long load_offset,
                               struct pmo_map_request *pmos_map_requests,
                               int cnt)
{
        int i;
        struct env_buf env_buf;
        char pidstr[16];
        int cow_cnt = 1;
        int libc_pmo_cap = 0;

        memset(env, 0, INIT_ENV_SIZE);

        /* The strings part starts from the middle of the page. */
        env_buf.entries_tail = (unsigned long *)env;
        env_buf.entries_end = (unsigned long *)(env + INIT_ENV_SIZE / 2);
        env_buf.strings_tail = (char *)env_buf.entries_end;
        env_buf.strings_end = env + INIT_ENV_SIZE;
        env_buf.strings_offset = top_vaddr - INIT_ENV_SIZE - (unsigned long)env;

        /* argc, argv */
        env_buf_append_int(&env_buf, argc);

        for (i = 0; i < argc; i++) {
                env_buf_append_str(&env_buf, argv[i]);
        }

        env_buf_append_int(&env_buf, 0);

        /* envp */
        env_buf_append_str(&env_buf, LD_LIB_PATH);
        snprintf(pidstr, 15, "PID=%d\n", pid);
        env_buf_append_str(&env_buf, pidstr);

        for (i = 0; i < envp_argc; i++) {
                env_buf_append_str(&env_buf, envp_argv[i]);
        }

        env_buf_append_int(&env_buf, 0);

        /* auxv */
        env_buf_append_int_auxv(&env_buf, AT_SECURE, 0);
        env_buf_append_int_auxv(&env_buf, AT_PAGESZ, PAGE_SIZE);
        env_buf_append_int_auxv(
                &env_buf, AT_PHDR, meta->phdr_addr + load_offset);
        env_buf_append_int_auxv(&env_buf, AT_PHENT, meta->phentsize);
        env_buf_append_int_auxv(&env_buf, AT_PHNUM, meta->phnum);
        env_buf_append_int_auxv(&env_buf, AT_FLAGS, meta->flags);
        env_buf_append_int_auxv(&env_buf, AT_ENTRY, meta->entry + load_offset);
        env_buf_append_int_auxv(&env_buf, AT_UID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_EUID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_GID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_EGID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_CLKTCK, FAKE_CLKTLK);
        env_buf_append_int_auxv(&env_buf, AT_HWCAP, 0);
        env_buf_append_str_auxv(&env_buf, AT_PLATFORM, PLAT);

        /* set a random canary for process */
        size_t *canary = (size_t *)(env + INIT_ENV_SIZE - RANDOM_OFF);
        *canary = rand();
        env_buf_append_int_auxv(
                &env_buf, AT_RANDOM, top_vaddr - RANDOM_OFF);

        env_buf_append_int_auxv(&env_buf, AT_BASE, 0);
        env_buf_append_int_auxv(
                &env_buf, AT_CHCORE_PMO_CNT, nr_pmo_map_reqs ?: ENVP_NONE_PMOS);
        for (i = 0; i < nr_pmo_map_reqs; i++) {
                env_buf_append_int_auxv(
                        &env_buf, AT_CHCORE_PMO_MAP_ADDR, pmo_map_reqs[i].addr);
        }

        env_buf_append_int_auxv(
                &env_buf, AT_CHCORE_CAP_CNT, nr_caps ?: ENVP_NONE_CAPS);
        for (i = 0; i < nr_caps; i++) {
                env_buf_append_int_auxv(&env_buf, AT_CHCORE_CAP_VAL, caps[i]);
        }
        for (int i = 0; i < cnt; ++i) {
                struct pmo_map_request *req = &pmos_map_requests[i];
                if (cow_cnt && (((req->perm) & VMR_COW) == VMR_COW)){
                        libc_pmo_cap = req->ret;
                        cow_cnt--;
                }
        }
        env_buf_append_int_auxv(
                &env_buf, AT_CHCORE_LIBC_PMO_CAP, libc_pmo_cap);
        env_buf_append_int_auxv(&env_buf, AT_NULL, 0);
}

/* A wrapper of usys_create_pmo. */
static void create_pmos(struct pmo_request *req, int cnt)
{
        for (int i = 0; i < cnt; ++i) {
                req[i].ret_cap = usys_create_pmo(req[i].size, req[i].type);
        }
}

/* A wrapper of usys_map_pmo. */
static int map_pmos(cap_t target_process_cap,
                    struct pmo_map_request *pmos_map_requests, int cnt)
{
        int ret = 0;
        for (int i = 0; i < cnt; ++i) {
                struct pmo_map_request *req = &pmos_map_requests[i];
                req->ret = usys_map_pmo(
                        target_process_cap, req->pmo_cap, req->addr, req->perm);
                if (req->ret < 0)
                        ret = req->ret;
        }
        return ret;
}

/** All processes have 2 pmo for stack and stack overflow detection */
#define DEFAULT_PMO_CNT (2)

/**
 * @brief Launch a new process in ChCore. The new process would execute
 * programs in the given ELF file.
 *
 * As a microkernel, ChCore does not have the concept of "process" actually.
 * So we have to build a traditional "process" step by step in the userspace.
 *
 * To build a process, we have to perform following steps:
 * Step-1: Create cap group of the new process. In ChCore, all system resources
 * used by a process are represented using capabilities, so a process is a
 * collection of capabilities, i.e. cap group, from where the kernel stands.
 * Step-2: Transfer all capabilities specified in args to the new cap group.
 * Step-3: Create pmo of the stack, and a forbidden pmo to detect stack
 * overflow. Step-4: Construct auxiliary vector on the stack. Step-5: Map the
 * two pmos and pmos of segments in the ELF file to construct the virtual memory
 * space of the new process. Step-6: Map custom pmos specified in args into the
 * new process. Step-7: Create the main/root thread of the new process. At this
 * point, all hardware state and system resources required by the new process is
 * ready, and the new thread would be scheduled by the kernel later.
 *
 * @param lp_args [In] see documentation of struct launch_process_args.
 * @return 0 on success, otherwise an error code. All memory resources
 * allocated by this function would be freed on error.
 */
int launch_process_with_pmos_caps(struct launch_process_args *lp_args)
{
        cap_t new_process_cap;
        cap_t main_thread_cap;
        int ret;
        vaddr_t pc;
        /* for creating pmos */
        struct pmo_request pmo_requests[DEFAULT_PMO_CNT];
        cap_t main_stack_cap;
        cap_t forbid_area_cap;
        u64 offset;
        u64 stack_top, stack_base;
        int i;
        /* for mapping pmos */
        struct pmo_map_request *seg_pmos_map_requests = NULL;
        cap_t *transfer_caps = NULL;
        vaddr_t load_offset = lp_args->load_offset;
        char init_env[INIT_ENV_SIZE];

        /*
         * user_elf: elf struct
         * child_process_cap: if not NULL, set to child_process_cap that can be
         *                    used in current process.
         *
         * child_main_thread_cap: if not NULL, set to child_main_thread_cap
         *                        that can be used in current process.
         *
         * pmo_map_reqs, nr_pmo_map_reqs: input pmos to map in the new process
         *
         * caps, nr_caps: copy from farther process to child process
         *
         * cpuid: affinity
         *
         * argc/argv: the number of arguments and the arguments.
         *
         * badge: the badge is generated by the invokder (usually procmgr).
         *
         * pcid: the pcid is fixed for root process and servers,
         * 	 and the rest is generated by procmgr.
         */
        struct user_elf *user_elf = lp_args->user_elf;
        cap_t *child_process_cap = lp_args->child_process_cap;
        cap_t *child_main_thread_cap = lp_args->child_main_thread_cap;
        struct pmo_map_request *pmo_map_reqs = lp_args->pmo_map_reqs;
        int nr_pmo_map_reqs = lp_args->nr_pmo_map_reqs;
        cap_right_t *masks = NULL, *rests = NULL;
        cap_t *origin_pmos = NULL, *shrunk_pmos = NULL;
        cap_t *caps = lp_args->caps;
        int nr_caps = lp_args->nr_caps;
        s32 cpuid = lp_args->cpuid;
        int argc = lp_args->argc;
        char **argv = lp_args->argv;
        int envp_argc = lp_args->envp_argc;
        char **envp_argv = lp_args->envp_argv;
        badge_t badge = lp_args->badge;
        int pid = lp_args->pid;
        u64 pcid = lp_args->pcid;
        /* The name of process/cap_group: mainly used for debugging */
        char *process_name = lp_args->process_name;
        unsigned long stack_size = ROUND_UP(lp_args->stack_size, PAGE_SIZE);
        bool is_traced = lp_args->is_traced;

        /* for usys_creat_thread */
        struct thread_args args;
        struct cap_group_args cg_args;

        seg_pmos_map_requests = malloc(sizeof(struct pmo_map_request)
                                       * (user_elf->segs_nr + DEFAULT_PMO_CNT));
        masks = malloc(sizeof(*masks) * user_elf->segs_nr);
        rests = malloc(sizeof(*rests) * user_elf->segs_nr);
        origin_pmos = malloc(sizeof(*origin_pmos) * user_elf->segs_nr);
        shrunk_pmos = malloc(sizeof(*shrunk_pmos) * user_elf->segs_nr);
        if (!seg_pmos_map_requests || !masks || !rests || !shrunk_pmos) {
                goto nomem;
        }

        /* Step-1: Create cap group of the new process. */
        cg_args.badge = badge;
        cg_args.name = (vaddr_t)process_name;
        cg_args.name_len = strlen(process_name);
        cg_args.pcid = pcid;
        cg_args.pid = pid;
#ifdef CHCORE_OPENTRUSTEE
        cg_args.heap_size = lp_args->heap_size;
        if (lp_args->puuid != NULL) {
                cg_args.puuid = (vaddr_t)&lp_args->puuid->uuid;
        } else {
                cg_args.puuid = 0;
        }
#endif
        new_process_cap = usys_create_cap_group((unsigned long)&cg_args);
        if (new_process_cap < 0) {
                printf("%s: fail to create new_process_cap (ret: %d)\n",
                       __func__,
                       new_process_cap);
                goto fail;
        }

        /** Step-2: Transfer capabilities */
        if (nr_caps > 0) {
                transfer_caps = malloc(sizeof(int) * nr_caps);
                /* usys_transfer_caps is used during process creation */
                ret = usys_transfer_caps(
                        new_process_cap, caps, nr_caps, transfer_caps);
                if (ret != 0) {
                        printf("usys_transfer_caps ret %d\n", ret);
                        usys_exit(-1);
                }
        }

        pc = user_elf->elf_meta.entry;

        /* Step-3: Create pmo of the stack, and a forbidden pmo to detect stack
         * overflow. */
        pmo_requests[0].size = stack_size;
        pmo_requests[0].type = PMO_ANONYM;

        pmo_requests[1].size = PAGE_SIZE;
        pmo_requests[1].type = PMO_FORBID;

        create_pmos((void *)pmo_requests, DEFAULT_PMO_CNT);

        /* get result caps */
        main_stack_cap = pmo_requests[0].ret_cap;
        if (main_stack_cap < 0) {
                printf("%s: fail to create_pmos (ret cap: %d)\n",
                       __func__,
                       main_stack_cap);
                goto fail;
        }

        /* Map a forbidden pmo in case of stack overflow */
        forbid_area_cap = pmo_requests[1].ret_cap;
        if (forbid_area_cap < 0) {
                printf("%s: fail to create_pmos (ret cap: %d)\n",
                       __func__,
                       forbid_area_cap);
                goto fail;
        }

        /* Step-4: Map the two pmos and pmos of segments in the ELF file */
        stack_base = (MAIN_THREAD_STACK_BASE
                      + ASLR_RAND_OFFSET)
                     & (~stack_size);

        /* Map the main stack pmo */
        seg_pmos_map_requests[0].pmo_cap = main_stack_cap;
        seg_pmos_map_requests[0].addr = stack_base;
        seg_pmos_map_requests[0].perm = VM_READ | VM_WRITE;

        /* Map the forbidden area in case of stack overflow */
        seg_pmos_map_requests[1].pmo_cap = forbid_area_cap;
        seg_pmos_map_requests[1].addr = stack_base - PAGE_SIZE;
        seg_pmos_map_requests[1].perm = VM_FORBID;

        for (i = 0; i < user_elf->segs_nr; ++i) {
                origin_pmos[i] = user_elf->user_elf_segs[i].elf_pmo;
                /*
                 * Drop CAP_RIGHT_REVOKE_ALL to prevent applications from
                 * maliciously revoking all of this file PMO.
                 */
                if (user_elf->user_elf_segs[i].perm & VMR_COW) {
                        masks[i] = CAP_RIGHT_REVOKE_ALL;
                } else {
                        masks[i] = CAP_RIGHT_NO_RIGHTS;
                }
                rests[i] = CAP_RIGHT_NO_RIGHTS;
        }

        ret = usys_transfer_caps_restrict(SELF_CAP,
                                          origin_pmos,
                                          user_elf->segs_nr,
                                          shrunk_pmos,
                                          masks,
                                          rests);
        if (ret != 0) {
                printf("%s: fail to restrict pmo rights (ret: %d)\n", __func__, ret);
                goto fail;
        }

        /* Map each segment in the elf binary */
        for (i = 0; i < user_elf->segs_nr; ++i) {
                seg_pmos_map_requests[DEFAULT_PMO_CNT + i].pmo_cap =
                        shrunk_pmos[i];
                seg_pmos_map_requests[DEFAULT_PMO_CNT + i].addr = ROUND_DOWN(
                        user_elf->user_elf_segs[i].p_vaddr + load_offset,
                        PAGE_SIZE);
                seg_pmos_map_requests[DEFAULT_PMO_CNT + i].perm =
                        user_elf->user_elf_segs[i].perm;
        }

        ret = map_pmos(new_process_cap,
                       (void *)seg_pmos_map_requests,
                       DEFAULT_PMO_CNT + i);
        if (ret != 0) {
                printf("%s: fail to map_pmos (ret: %d)\n", __func__, ret);
                goto fail;
        }

        for (i = 0; i < user_elf->segs_nr; ++i) {
                usys_revoke_cap(shrunk_pmos[i], false);
        }

        /* Step-5: Construct auxiliary vector on the stack. */
        stack_top = stack_base + stack_size;

        construct_init_env(init_env,
                           stack_top,
                           &user_elf->elf_meta,
                           user_elf->path,
                           pmo_map_reqs,
                           nr_pmo_map_reqs,
                           transfer_caps,
                           nr_caps,
                           argc,
                           argv,
                           envp_argc, 
                           envp_argv,
                           pid,
                           load_offset,
                           (void *)seg_pmos_map_requests,
                           DEFAULT_PMO_CNT + i);
        offset = stack_size - INIT_ENV_SIZE;

        free(transfer_caps);

        ret = usys_write_pmo(main_stack_cap, offset, init_env, INIT_ENV_SIZE);
        if (ret != 0) {
                printf("%s: fail to write_pmo (ret: %d)\n", __func__, ret);
                goto fail;
        }
        
        /* Remove two caps just created for the new process in procmgr */
        usys_revoke_cap(main_stack_cap, false);
        usys_revoke_cap(forbid_area_cap, false);

        /** Step-6: Map custom pmos specified in args into the new process. */
        if (nr_pmo_map_reqs) {
                ret = map_pmos(
                        new_process_cap, (void *)pmo_map_reqs, nr_pmo_map_reqs);
                if (ret != 0) {
                        printf("%s: fail to map_pmos (ret: %d)\n",
                               __func__,
                               ret);
                        goto fail;
                }
        }

        /** Step-7: Create the main/root thread of the new process. */
        args.cap_group_cap = new_process_cap;
        args.stack = stack_base + offset;
        args.pc = pc + load_offset;
        args.arg = (unsigned long)NULL;
        args.prio = MAIN_THREAD_PRIO;
        args.tls = cpuid;
        args.clear_child_tid = NULL;
        if (is_traced) {
                args.type = TYPE_TRACEE;
        } else {
                args.type = TYPE_USER;
        }
        main_thread_cap = usys_create_thread((unsigned long)&args);

        if (child_process_cap)
                *child_process_cap = new_process_cap;
        if (child_main_thread_cap)
                *child_main_thread_cap = main_thread_cap;

        free(seg_pmos_map_requests);
        return 0;
fail:
        free(seg_pmos_map_requests);
        return -EINVAL;
nomem:
        free(shrunk_pmos);
        free(origin_pmos);
        free(masks);
        free(rests);
        free(seg_pmos_map_requests);
        return -ENOMEM;
}
