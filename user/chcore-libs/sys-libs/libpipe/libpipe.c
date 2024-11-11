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

#include "libpipe.h"

#include <malloc.h>
#include <string.h>
#include <chcore/defs.h>
#include <chcore/memory.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/*
 * [u32 beg][u32 end][...]
 * data: [beg, end)
 */
struct pipe {
        u32 size;
        cap_t pmo_cap;
        void *data;
};

struct pipe *create_pipe_from_pmo(u32 size, cap_t pmo_cap)
{
        struct pipe *pp = (struct pipe *)malloc(sizeof(*pp));
        pp->size = size - sizeof(u32) * 2;
        pp->pmo_cap = pmo_cap;
        pp->data = chcore_auto_map_pmo(pmo_cap, size, VM_READ | VM_WRITE);
        if (pp->data == NULL)
                perror("error: ");
        return pp;
}

struct pipe *create_pipe_from_vaddr(u32 size, void *data)
{
        struct pipe *pp = (struct pipe *)malloc(sizeof(*pp));
        pp->size = size - sizeof(u32) * 2;
        pp->pmo_cap = -1;
        pp->data = data;
        return pp;
}

void del_pipe(struct pipe *pp)
{
        if (pp == NULL)
                return;
        if (pp->pmo_cap != -1) {
                chcore_auto_unmap_pmo(pp->pmo_cap,
                                      (unsigned long)pp->data,
                                      pp->size + sizeof(u32) * 2);
        } else if (pp->data != NULL) {
                free(pp->data);
        }
        free(pp);
}

inline void pipe_init(const struct pipe *pp)
{
        ((u32 *)pp->data)[0] = 0;
        ((u32 *)pp->data)[1] = 0;
}

u32 pipe_read(const struct pipe *pp, void *buf, u32 n)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        char *start = (char *)pp->data + sizeof(u32) * 2;
        u32 len, len_tmp;

        /* Something wrong happened */
        if (beg >= pp->size || end >= pp->size) {
                pipe_init(pp);
                return 0;
        }

        if (beg <= end) {
                len = MIN(end - beg, n);
                memcpy(buf, start + beg, len);
                beg += len;
        } else {
                len = MIN(pp->size - beg + end, n);
                len_tmp = MIN(pp->size - beg, len);
                memcpy(buf, start + beg, len_tmp);
                memcpy((char *)buf + len_tmp, start, len - len_tmp);
                beg = (beg + len) % pp->size;
        }

        ((u32 *)pp->data)[0] = beg;
        return len;
}

u32 pipe_read_exact(const struct pipe *pp, void *buf, u32 n)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        char *start = (char *)pp->data + sizeof(u32) * 2;
        u32 len, len_tmp;

        /* Something wrong happened */
        if (beg >= pp->size || end >= pp->size) {
                pipe_init(pp);
                return 0;
        }

        if (beg <= end) {
                if (end - beg < n)
                        return 0;
                len = n;
                memcpy(buf, start + beg, len);
                beg += len;
        } else {
                if (pp->size - beg + end < n)
                        return 0;
                len = n;
                len_tmp = MIN(pp->size - beg, len);
                memcpy(buf, start + beg, len_tmp);
                memcpy((char *)buf + len_tmp, start, len - len_tmp);
                beg = (beg + len) % pp->size;
        }

        ((u32 *)pp->data)[0] = beg;
        return len;
}

u32 pipe_write(const struct pipe *pp, const void *buf, u32 n)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        char *start = (char *)pp->data + sizeof(u32) * 2;
        u32 len, len_tmp;

        /* Something wrong happened */
        if (beg >= pp->size || end >= pp->size) {
                pipe_init(pp);
                return 0;
        }

        if (beg <= end) {
                len = MIN(pp->size - end + beg - 1, n);
                len_tmp = MIN(pp->size - end, len);
                memcpy(start + end, buf, len_tmp);
                memcpy(start, (char *)buf + len_tmp, len - len_tmp);
                end = (end + len) % pp->size;
        } else {
                len = MIN(beg - end - 1, n);
                memcpy(start + end, buf, len);
                end += len;
        }

        ((u32 *)pp->data)[1] = end;
        return len;
}

u32 pipe_write_exact(const struct pipe *pp, const void *buf, u32 n)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        char *start = (char *)pp->data + sizeof(u32) * 2;
        u32 len, len_tmp;

        /* Something wrong happened */
        if (beg >= pp->size || end >= pp->size) {
                pipe_init(pp);
                return 0;
        }

        if (beg <= end) {
                if (pp->size - end + beg - 1 < n)
                        return 0;
                len = n;
                len_tmp = MIN(pp->size - end, len);
                memcpy(start + end, buf, len_tmp);
                memcpy(start, (char *)buf + len_tmp, len - len_tmp);
                end = (end + len) % pp->size;
        } else {
                if (beg - end - 1 < n)
                        return 0;
                len = n;
                memcpy(start + end, buf, len);
                end += len;
        }

        ((u32 *)pp->data)[1] = end;
        return len;
}

bool pipe_is_empty(const struct pipe *pp)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        return beg == end;
}

bool pipe_is_full(const struct pipe *pp)
{
        u32 beg = ((u32 *)pp->data)[0];
        u32 end = ((u32 *)pp->data)[1];
        return end == beg - 1 || (beg == 0 && end == pp->size - 1);
}
