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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include "unistd.h"
#include <stddef.h>
#endif

#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <chcore/ipc.h>
#include <chcore/defs.h>
#include <chcore/memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <chcore/bug.h>
#include <debug_lock.h>
#include <errno.h>
#include <assert.h>
#include "pthread_impl.h"
#include "fs_client_defs.h"
#include <chcore-internal/lwip_defs.h>
#include <chcore/pthread.h>

#define CONN_CAP_SERVER -1

struct ipc_msg {
        void *data_ptr;
        unsigned int max_data_len;
        /*
         * cap_slot_number represents the number of caps of the ipc_msg.
         * This is useful for both sending IPC and returning from IPC.
         * When calling ipc_return, cap_slot_number will be set 0 automatically,
         * indicating that no cap will be sent.
         * If you want to send caps when returning from IPC,
         * use ipc_return_with_cap.
         */
        unsigned int send_cap_num;

        /* icb: ipc control block (not needed by the kernel) */
        ipc_struct_t *icb;
        struct ipc_response_hdr *response_hdr;
        enum {
                THREAD_SERVER,
                THREAD_CLIENT,
        } thread_type;
} __attribute__((aligned(sizeof(void *))));

/*
 * **fsm_ipc_struct** is an address that points to the per-thread
 * system_ipc_fsm in the pthread_t struct.
 */
ipc_struct_t *__fsm_ipc_struct_location(void)
{
        return &__pthread_self()->system_ipc_fsm;
}

/*
 * **lwip_ipc_struct** is an address that points to the per-thread
 * system_ipc_net in the pthread_t struct.
 */
ipc_struct_t *__net_ipc_struct_location(void)
{
        return &__pthread_self()->system_ipc_net;
}

ipc_struct_t *__procmgr_ipc_struct_location(void)
{
        return &__pthread_self()->system_ipc_procmgr;
}

static int connect_system_server(ipc_struct_t *ipc_struct);
static int disconnect_system_servers();

/* Interfaces for operate the ipc message (begin here) */

ipc_msg_t *ipc_create_msg(ipc_struct_t *icb, unsigned int data_len)
{
        return ipc_create_msg_with_cap(icb, data_len, 0);
}
/*
 * ipc_msg_t is constructed on the shm pointed by
 * ipc_struct_t->shared_buf.
 * A new ips_msg will override the old one.
 */
ipc_msg_t *ipc_create_msg_with_cap(ipc_struct_t *icb, unsigned int data_len,
                          unsigned int send_cap_num)
{
        BUILD_BUG_ON(sizeof(ipc_msg_t) > SERVER_IPC_MSG_BUF_SIZE);
        ipc_msg_t *ipc_msg;
        unsigned long buf_len;

        if (unlikely(icb->conn_cap == 0)) {
                /* Create the IPC connection on demand */
                if (connect_system_server(icb) != 0) {
                        printf("connect ipc server failed!\n");
                        exit(-1);
                }
        }

        /* Grab the ipc lock before setting ipc msg */
        chcore_spin_lock(&(icb->lock));

        buf_len = icb->shared_buf_len;

        /*
         * Check the total length of data and caps.
         *
         * The checks at client side is not for security but for preventing
         * unintended errors made by benign clients.
         * The server has to validate the ipc msg by itself.
         */
        if (data_len > buf_len) {
                printf("%s failed: too long msg (the usable shm size is 0x%lx)\n",
                       __func__,
                       buf_len);
                goto out_unlock;
        }

        ipc_msg = (ipc_msg_t *)malloc(sizeof(ipc_msg_t));

        if (!ipc_msg) {
                goto out_unlock;
        }

        ipc_msg->data_ptr = SHM_PTR_TO_CUSTOM_DATA_PTR(icb->shared_buf);
        ipc_msg->max_data_len = buf_len;
        ipc_msg->send_cap_num = send_cap_num;
        ipc_msg->response_hdr = (struct ipc_response_hdr *)icb->shared_buf;
        ipc_msg->icb = icb;
        ipc_msg->thread_type = THREAD_CLIENT;
                  
        return ipc_msg;
out_unlock:
        chcore_spin_unlock(&(icb->lock));
        printf("ipc create msg failed!\n");
        exit(-1);

}

void __ipc_server_init_raw_msg(ipc_msg_t *ipc_msg, void *shm_ptr, unsigned int max_data_len, unsigned int cap_num)
{
        ipc_msg->data_ptr = SHM_PTR_TO_CUSTOM_DATA_PTR(shm_ptr);
        ipc_msg->max_data_len = max_data_len;
        ipc_msg->send_cap_num = cap_num;
        ipc_msg->response_hdr = (struct ipc_response_hdr *)shm_ptr;
        ipc_msg->thread_type = THREAD_SERVER;
}

char *ipc_get_msg_data(ipc_msg_t *ipc_msg)
{
        return (char *)ipc_msg->data_ptr;
}

char *ipc_get_msg_data_bounded(ipc_msg_t *ipc_msg, unsigned int size)
{       
        if (size > ipc_msg->max_data_len) {
                return NULL;
        }

        return (char *)ipc_msg->data_ptr;
}

int ipc_set_msg_data(ipc_msg_t *ipc_msg, void *data, unsigned int offset,
                     unsigned int len)
{
        if ((offset + len < offset) || (offset + len > ipc_msg->max_data_len)) {
                printf("%s failed due to overflow.\n", __func__);
                return -1;
        }

        memcpy(ipc_get_msg_data(ipc_msg) + offset, data, len);
        return 0;
}

cap_t ipc_get_msg_cap(ipc_msg_t *ipc_msg, unsigned int cap_slot_index)
{
        cap_t cap, conn_cap;
        if (ipc_msg->thread_type == THREAD_SERVER)
                conn_cap = CONN_CAP_SERVER;
        else
                conn_cap = ipc_msg->icb->conn_cap;

        while ((cap = usys_ipc_get_cap(conn_cap, cap_slot_index)) < 0) {
                if (cap != -EIPCRETRY) {
                        printf("%s failed, ret = %d\n", __func__, cap);
                        return -1;
                }
        }
        return cap;
}

int ipc_set_msg_cap(ipc_msg_t *ipc_msg, unsigned int cap_slot_index, cap_t cap)
{
        return ipc_set_msg_cap_restrict(ipc_msg,
                                        cap_slot_index,
                                        cap,
                                        CAP_RIGHT_NO_RIGHTS,
                                        CAP_RIGHT_NO_RIGHTS);
}

int ipc_set_msg_cap_restrict(ipc_msg_t *ipc_msg, unsigned int cap_slot_index,
                             cap_t cap, cap_right_t mask, cap_right_t rest)
{
        int ret;
        cap_t conn_cap;

        if (ipc_msg->thread_type == THREAD_SERVER)
                conn_cap = CONN_CAP_SERVER;
        else
                conn_cap = ipc_msg->icb->conn_cap;
        
        while ((ret = usys_ipc_set_cap_restrict(conn_cap, cap_slot_index, cap, mask, rest)) < 0) {
               if (ret != -EIPCRETRY) {
                        printf("%s failed, ret = %d\n", __func__, cap);
                        return -1;
                }
        }
        return 0;
}

unsigned int ipc_get_msg_send_cap_num(ipc_msg_t *ipc_msg)
{
        return ipc_msg->send_cap_num;
}

void ipc_set_msg_send_cap_num(ipc_msg_t *ipc_msg, unsigned int cap_num)
{
        ipc_msg->send_cap_num = cap_num;
}

unsigned int ipc_get_msg_return_cap_num(ipc_msg_t *ipc_msg)
{
        return ipc_msg->response_hdr->return_cap_num;
}

void ipc_set_msg_return_cap_num(ipc_msg_t *ipc_msg, unsigned int cap_num)
{
        ipc_msg->response_hdr->return_cap_num = cap_num;
}

int ipc_destroy_msg(ipc_msg_t *ipc_msg)
{
        /* Release the ipc lock */
        chcore_spin_unlock(&(ipc_msg->icb->lock));

        free(ipc_msg);

        return 0;
}

/* Interfaces for operate the ipc message (end here) */

#ifdef CHCORE_ARCH_X86_64
/**
 * In ChCore IPC, a server shadow thread **always** uses usys_ipc_return syscall
 * to enter kernel, sleep and return to client thread. When recycling a
 * connection, the kernel would wait for all its server shadow thread until they
 * return to kernel through usys_ipc_return, then the kernel calls
 * ipc_shadow_thread_exit_routine on those threads to recycle server state for
 * this connection.
 *
 * On x86_64, the problem is that due to all shadow threads return to kernel
 * using syscall, and the calling convention of syscall would clobber registers,
 * so functions called by the kernel should also follow syscall calling
 * convention. However, ipc_shadow_thread_exit_routine whose assembly code is
 * generated by compiler is unaware of that and still follows SystemV calling
 * convention. So we implement the necessary translation from syscall to SystemV
 * calling convention using this naked function
 */
__attribute__((naked)) void ipc_shadow_thread_exit_routine_naked(void)
{
        __asm__ __volatile__("mov %%r10, %%rcx \n"
                             "jmp ipc_shadow_thread_exit_routine \n":::"rcx");
}

__attribute__((naked)) void ipc_shadow_thread_exit_routine_single_naked(void)
{
        __asm__ __volatile__("mov %%r10, %%rcx \n"
                             "jmp ipc_shadow_thread_exit_routine_single \n":::"rcx");
}
#endif

/* Shadow thread exit routine */
int ipc_shadow_thread_exit_routine(server_destructor destructor_func,
                                   badge_t client_badge,
                                   unsigned long server_shm_addr,
                                   unsigned long shm_size)
{
        if (destructor_func) {
                destructor_func(client_badge);
        }
        chcore_free_vaddr(server_shm_addr, shm_size);
        pthread_detach(pthread_self());
        disconnect_system_servers();
        pthread_exit(NULL);
}

void ipc_shadow_thread_exit_routine_single(server_destructor destructor_func,
                                          badge_t client_badge,
                                          unsigned long server_shm_addr,
                                          unsigned long shm_size)
{
        if (destructor_func) {
                destructor_func(client_badge);
        }
        chcore_free_vaddr(server_shm_addr, shm_size);
        usys_ipc_exit_routine_return();
}

/* A register_callback thread uses this to finish a registration */
void ipc_register_cb_return(cap_t server_thread_cap,
                            unsigned long server_thread_exit_routine,
                            unsigned long server_shm_addr)
{
        usys_ipc_register_cb_return(
                server_thread_cap, server_thread_exit_routine, server_shm_addr);
}

/* A register_callback thread is passive (never proactively run) */
void *register_cb(void *ipc_handler)
{
        cap_t server_thread_cap = 0;
        unsigned long shm_addr;

        shm_addr = chcore_alloc_vaddr(IPC_PER_SHM_SIZE);

        // printf("[server]: A new client comes in! ipc_handler: 0x%lx\n",
        // ipc_handler);

        /*
         * Create a passive thread for serving IPC requests.
         * Besides, reusing an existing thread is also supported.
         */
        pthread_t handler_tid;
        server_thread_cap = chcore_pthread_create_shadow(
                &handler_tid, NULL, ipc_handler, (void *)NO_ARG);
        BUG_ON(server_thread_cap < 0);
#ifndef CHCORE_ARCH_X86_64
        ipc_register_cb_return(server_thread_cap,
                               (unsigned long)ipc_shadow_thread_exit_routine,
                               shm_addr);
#else
        ipc_register_cb_return(
                server_thread_cap,
                (unsigned long)ipc_shadow_thread_exit_routine_naked,
                shm_addr);
#endif

        return NULL;
}

/* Register callback for single-handler-thread server */
void *register_cb_single(void *ipc_handler)
{
        static cap_t single_handler_thread_cap = -1;
        unsigned long shm_addr;

        /* alloc shm_addr */
        shm_addr = chcore_alloc_vaddr(IPC_PER_SHM_SIZE);

        /* if single handler thread isn't created */
        if (single_handler_thread_cap == -1) {
                pthread_t single_handler_tid;
                single_handler_thread_cap = chcore_pthread_create_shadow(
                        &single_handler_tid, NULL, ipc_handler, (void *)NO_ARG);
        }

        assert(single_handler_thread_cap > 0);

#ifndef CHCORE_ARCH_X86_64
        ipc_register_cb_return(
                single_handler_thread_cap,
                (unsigned long)ipc_shadow_thread_exit_routine_single,
                shm_addr);
#else
        ipc_register_cb_return(
                single_handler_thread_cap,
                (unsigned long)ipc_shadow_thread_exit_routine_single_naked,
                shm_addr);
#endif

        return NULL;
}

int ipc_register_server(server_handler server_handler,
                        void *(*client_register_handler)(void *))
{
        return ipc_register_server_with_destructor(
                server_handler, client_register_handler, DEFAULT_DESTRUCTOR);
}

/*
 * Currently, a server thread can only invoke this interface once.
 * But, a server can use another thread to register a new service.
 */
int ipc_register_server_with_destructor(server_handler server_handler,
                                        void *(*client_register_handler)(void *),
                                        server_destructor server_destructor)
{
        cap_t register_cb_thread_cap;
        int ret;

/*
 * Create a passive thread for handling IPC registration.
 * - run after a client wants to register
 * - be responsible for initializing the ipc connection
 */
#define ARG_SET_BY_KERNEL 0
        pthread_t handler_tid;
        register_cb_thread_cap =
                chcore_pthread_create_register_cb(&handler_tid,
                                                  NULL,
                                                  client_register_handler,
                                                  (void *)ARG_SET_BY_KERNEL);
        BUG_ON(register_cb_thread_cap < 0);
        /*
         * Kernel will pass server_handler as the argument for the
         * register_cb_thread.
         */
        ret = usys_register_server((unsigned long)server_handler,
                                   (unsigned long)register_cb_thread_cap,
                                   (unsigned long)server_destructor);
        if (ret != 0) {
                printf("%s failed (retval is %d)\n", __func__, ret);
        }
        return ret;
}

struct client_shm_config {
        cap_t shm_cap;
        unsigned long shm_addr;
};

/*
 * A client thread can register itself for multiple times.
 *
 * The returned ipc_struct_t is from heap,
 * so the callee needs to free it.
 */
ipc_struct_t *ipc_register_client(cap_t server_thread_cap)
{
        cap_t conn_cap;
        ipc_struct_t *client_ipc_struct;

        struct client_shm_config shm_config;
        cap_t shm_cap;

        client_ipc_struct = malloc(sizeof(ipc_struct_t));
        if (client_ipc_struct == NULL) {
                return NULL;
        }

        /*
         * Before registering client on the server,
         * the client allocates the shm (and shares it with
         * the server later).
         *
         * Now we used PMO_DATA instead of PMO_SHM because:
         * - SHM (IPC_PER_SHM_SIZE) only contains one page and
         *   PMO_DATA is thus more efficient.
         *
         * If the SHM becomes larger, we can use PMO_SHM instead.
         * Both types are tested and can work well.
         */

        // shm_cap = usys_create_pmo(IPC_PER_SHM_SIZE, PMO_SHM);
        shm_cap = usys_create_pmo(IPC_PER_SHM_SIZE, PMO_DATA);
        if (shm_cap < 0) {
                printf("usys_create_pmo ret %d\n", shm_cap);
                goto out_free_client_ipc_struct;
        }

        shm_config.shm_cap = shm_cap;
        shm_config.shm_addr = chcore_alloc_vaddr(IPC_PER_SHM_SIZE);

        // printf("%s: register_client with shm_addr 0x%lx\n",
        //      __func__, shm_config.shm_addr);

        while (1) {
                conn_cap = usys_register_client(server_thread_cap,
                                                (unsigned long)&shm_config);

                if (conn_cap == -EIPCRETRY) {
                        // printf("client: Try to connect again ...\n");
                        /* The server IPC may be not ready. */
                        usys_yield();
                } else if (conn_cap < 0) {
                        printf("client: %s failed (return %d), server_thread_cap is %d\n",
                               __func__,
                               conn_cap,
                               server_thread_cap);
                        goto out_free_vaddr;
                } else {
                        /* Success */
                        break;
                }
        }

        client_ipc_struct->lock = 0;
        client_ipc_struct->shared_buf = shm_config.shm_addr;
        client_ipc_struct->shared_buf_len = IPC_PER_SHM_SIZE;
        client_ipc_struct->conn_cap = conn_cap;

        return client_ipc_struct;

out_free_vaddr:
        usys_revoke_cap(shm_cap, false);
        chcore_free_vaddr(shm_config.shm_addr, IPC_PER_SHM_SIZE);

out_free_client_ipc_struct:
        free(client_ipc_struct);

        return NULL;
}

int ipc_client_close_connection(ipc_struct_t *ipc_struct)
{
        int ret;
        while (1) {
                ret = usys_ipc_close_connection(ipc_struct->conn_cap);

                if (ret == -EAGAIN) {
                        usys_yield();
                } else if (ret < 0) {
                        goto out;
                } else {
                        break;
                }
        }

        chcore_free_vaddr(ipc_struct->shared_buf, ipc_struct->shared_buf_len);
        free(ipc_struct);
out:
        return ret;
}

/* Client uses **ipc_call** to issue an IPC request */
long ipc_call(ipc_struct_t *icb, ipc_msg_t *ipc_msg)
{
        long ret;

        if (unlikely(icb->conn_cap == 0)) {
                /* Create the IPC connection on demand */
                if ((ret = connect_system_server(icb)) != 0)
                        return ret;
        }

        do {
                ret = usys_ipc_call(icb->conn_cap, 
                                    ipc_get_msg_send_cap_num(ipc_msg));
        } while (ret == -EIPCRETRY);

        return ret;
}

/* Server uses **ipc_return** to finish an IPC request */
void ipc_return(ipc_msg_t *ipc_msg, long ret)
{
        if (ipc_msg != NULL) {
                ipc_set_msg_return_cap_num(ipc_msg, 0);
        }
        usys_ipc_return((unsigned long)ret, 0);
}

/*
 * IPC return and copy back capabilities.
 * Use different ipc return interface because cap_slot_number
 * is valid only when we have cap to return. So we need to reset it to
 * 0 in ipc_return which has no cap to return.
 */
void ipc_return_with_cap(ipc_msg_t *ipc_msg, long ret)
{
        usys_ipc_return((unsigned long)ret, ipc_get_msg_return_cap_num(ipc_msg));
}

int simple_ipc_forward(ipc_struct_t *ipc_struct, void *data, int len)
{
        ipc_msg_t *ipc_msg;
        int ret;

        ipc_msg = ipc_create_msg(ipc_struct, len);
        ret = ipc_set_msg_data(ipc_msg, data, 0, len);
        
        if (ret < 0) {
                return -EINVAL;
        }

        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static void ipc_struct_copy(ipc_struct_t *dst, ipc_struct_t *src)
{
        dst->conn_cap = src->conn_cap;
        dst->shared_buf = src->shared_buf;
        dst->shared_buf_len = src->shared_buf_len;
        dst->lock = src->lock;
}

static int connect_system_server(ipc_struct_t *ipc_struct)
{
        ipc_struct_t *tmp;

        switch (ipc_struct->server_id) {
        case FS_MANAGER: {
                tmp = ipc_register_client(fsm_server_cap);
                if (tmp == NULL) {
                        printf("%s: failed to connect FS\n", __func__);
                        return -1;
                }
                break;
        }
        case NET_MANAGER: {
                tmp = ipc_register_client(lwip_server_cap);
                if (tmp == NULL) {
                        printf("%s: failed to connect NET\n", __func__);
                        return -1;
                }
                break;
        }
        case PROC_MANAGER: {
                tmp = ipc_register_client(procmgr_server_cap);
                if (tmp == NULL) {
                        printf("%s: failed to connect PROCMGR\n", __func__);
                        return -1;
                }
                break;
        }
        default:
                printf("%s: unsupported system server id %d\n",
                       __func__,
                       ipc_struct->server_id);
                return -1;
        }

        /* Copy the newly allocated ipc_struct to the per_thread ipc_struct */
        ipc_struct_copy(ipc_struct, tmp);
        free(tmp);

        return 0;
}

#define disconnect_server(server)                                  \
        do {                                                       \
                int ret;                                           \
                if (!server) {                                     \
                        printf(#server "is NULL!\n");              \
                        break;                                     \
                }                                                  \
                if ((server)->conn_cap) {                          \
                        ret = ipc_client_close_connection(server); \
                        if (ret < 0)                               \
                                return ret;                        \
                }                                                  \
        } while (0)

int disconnect_system_servers(void)
{
        disconnect_server(procmgr_ipc_struct);
        disconnect_server(fsm_ipc_struct);
        disconnect_server(lwip_ipc_struct);
        disconnect_mounted_fs();
        return 0;
}
