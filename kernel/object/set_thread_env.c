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

#include <mm/mm.h>
#include <common/util.h>
#include <lib/krand.h>

#include "thread_env.h"

/*
 * Setup the initial environment for a user process (main thread).
 * According to Libc convention, we current set the environment on the user stack.
 */

#define AT_NULL     0 /* end of vector */
#define AT_PHDR     3 /* program headers for program */
#define AT_PHENT    4 /* size of program header entry */
#define AT_PHNUM    5 /* number of program headers */
#define AT_PAGESZ   6 /* system page size */
#define AT_UID      11 /* real uid */
#define AT_EUID     12 /* effective uid */
#define AT_GID      13 /* real gid */
#define AT_EGID     14 /* effective gid */
#define AT_HWCAP    16 /* arch dependent hints at CPU capabilities */
#define AT_SECURE   23 /* secure mode boolean */
#define AT_RANDOM   25 /* address of 16 random bytes */

#define RANDOM_OFF 64 /* offset of a 64bit random number */

struct env_buf {
        unsigned long *entries_tail;
        unsigned long *entries_end;
        char *strings_tail;
        char *strings_end;
        unsigned long strings_offset;
};

static void env_buf_append_int(struct env_buf *env_buf, unsigned long val)
{
        BUG_ON(env_buf->entries_tail >= env_buf->entries_end);
        *(env_buf->entries_tail++) = val;
}

static void env_buf_append_str(struct env_buf *env_buf, const char *val)
{
        int i = 0;
        int val_size = strlen(val) + 1;

        BUG_ON(env_buf->strings_tail + val_size > env_buf->strings_end);
        while (val[i] != '\0') {
                env_buf->strings_tail[i] = val[i];
                i++;
        }
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

#define FAKE_UGID 0

/*
 * For setting up the stack (env) of some process.
 *
 * env: stack top address used by kernel
 * top_vaddr: stack top address mapped to user
 */
void prepare_env(char *env, vaddr_t top_vaddr, char *name,
                 struct process_metadata *meta)
{
        struct env_buf env_buf;
        /* clear env */
        memset(env, 0, ENV_SIZE_ON_STACK);

        env_buf.entries_tail = (unsigned long *)env;
        env_buf.entries_end = (unsigned long *)(env + ENV_SIZE_ON_STACK / 2);
        env_buf.strings_tail = (char *)env_buf.entries_end;
        env_buf.strings_end = env + ENV_SIZE_ON_STACK;
        env_buf.strings_offset = top_vaddr - ENV_SIZE_ON_STACK - (vaddr_t)env;

        /** argc */
        env_buf_append_int(&env_buf, 1);
        /** argv(only program cmd) */
        env_buf_append_str(&env_buf, name);

        /** end of argv */
        env_buf_append_int(&env_buf, 0);

        /** end of envp(empty envp) */
        env_buf_append_int(&env_buf, 0);

        env_buf_append_int_auxv(&env_buf, AT_SECURE, 0);
        env_buf_append_int_auxv(&env_buf, AT_PAGESZ, PAGE_SIZE);
        env_buf_append_int_auxv(&env_buf, AT_PHDR, meta->phdr_addr);
        env_buf_append_int_auxv(&env_buf, AT_PHENT, meta->phentsize);
        env_buf_append_int_auxv(&env_buf, AT_PHNUM, meta->phnum);
        env_buf_append_int_auxv(&env_buf, AT_UID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_EUID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_GID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_EGID, FAKE_UGID);
        env_buf_append_int_auxv(&env_buf, AT_HWCAP, 0);
        // env_buf_append_str_auxv(&env_buf, AT_PLATFORM, PLAT);

        /* set a random canary for procmgr */
        size_t *canary = (size_t *)(env + ENV_SIZE_ON_STACK - RANDOM_OFF);
        *canary = krand();
        env_buf_append_int_auxv(
                &env_buf, AT_RANDOM, top_vaddr - RANDOM_OFF);

        env_buf_append_int_auxv(&env_buf, AT_NULL, 0);

        /* add more auxv here */
}
