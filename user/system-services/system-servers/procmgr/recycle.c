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

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/ring_buffer.h>

#include "proc_node.h"
#include "procmgr_dbg.h"

pthread_mutex_t recycle_lock;

/* A recycle_msg is set by the kernel when one process needs to
 * be recycled.
 */
struct recycle_msg {
        badge_t badge;
        int exitcode;
        int padding;
};

struct ring_buffer *recycle_msg_buffer = NULL;
#define MAX_MSG_NUM 100

int usys_cap_group_recycle(int);

void *recycle_routine(void *arg)
{
        cap_t notific_cap;
        int ret;

        struct recycle_msg msg;
        struct proc_node *proc_to_recycle;

        /* Recycle_thread will wait on this notification */
        notific_cap = usys_create_notifc();

        /* The msg on recycling which process is in msg_buffer */
        recycle_msg_buffer =
                new_ringbuffer(MAX_MSG_NUM, sizeof(struct recycle_msg));
        assert(recycle_msg_buffer);

        ret = usys_register_recycle_thread(notific_cap,
                                           (vaddr_t)recycle_msg_buffer);
        assert(ret == 0);

        while (1) {
                usys_wait(notific_cap, 1 /* Block */, NULL /* No timeout */);
                while (get_one_msg(recycle_msg_buffer, &msg)) {
                        proc_to_recycle = get_proc_node(msg.badge);
                        assert(proc_to_recycle != 0);

                        pthread_mutex_lock(&recycle_lock);
                        proc_to_recycle->exitstatus = msg.exitcode;
                        /*
                         * chcore_waitpid() will block until the state of
                         * corresponding process is set as PROC_STATE_EXIT.
                         */
                        proc_to_recycle->state = PROC_STATE_EXIT;

                        do {
                                ret = usys_cap_group_recycle(
                                        proc_to_recycle->proc_cap);
                                if (ret == -EAGAIN)
                                        sched_yield();
                        } while (ret == -EAGAIN);

                        /*
                         * Since de_proc_node will free the pcid/asid,
                         * it is necessary to ensure the tlbs of the id
                         * has been flushed.
                         * Currently, this can be ensured when the recycle is
                         * done.
                         */

                        del_proc_node(proc_to_recycle);
                        pthread_mutex_unlock(&recycle_lock);
                }
        }
}
