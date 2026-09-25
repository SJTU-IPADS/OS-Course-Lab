/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#ifndef IPC_CONNECTION_H
#define IPC_CONNECTION_H

/* The max number of cap allowed to be transfered during an IPC. */
#define MAX_CAP_TRANSFER 16

#define CONN_IPC_SERVER -1

#include <object/thread.h>

/*
 * There are three states of an IPC connection.
 * An IPC call request on a connection will be declined other
 * than the connection state is VALID, and a connection is only safe
 * to be recycled when its state is RECYCLE_READY.
 * See `__stop_connection` in recycle.c.
 */
enum conn_state {
        CONN_VALID = 0,
        CONN_INCOME_STOPPED,
        CONN_RECYCLE_READY,
        CONN_DEINIT_READY
};

/*
 * An ipc_server_handler_thread has such config which stores its information.
 * Note that the ipc_server_handler_thread is used for handling IPC requests.
 */
struct ipc_server_handler_config {
        /* Avoid invoking the same handler_thread concurrently */
        struct lock ipc_lock;

        /* PC */
        vaddr_t ipc_routine_entry;
        /* SP */
        vaddr_t ipc_routine_stack;
        /* Entry point of shadow thread exit routine */
        vaddr_t ipc_exit_routine_entry;
        vaddr_t destructor;

        /*
         * Record which connection uses this handler thread now.
         * Multiple connection can use the same handler_thread.
         */
        struct ipc_connection *active_conn;
};

/*
 * An ipc_server_register_cb_thread has such config which stores its
 * information. This thread is used for handling IPC registration.
 */
struct ipc_server_register_cb_config {
        struct lock register_lock;
        /* PC */
        vaddr_t register_cb_entry;
        /* SP */
        vaddr_t register_cb_stack;
        vaddr_t destructor;

        /* The caps for the connection currently building */
        cap_t conn_cap_in_client;
        /* Not used now (can be exposed to server in future) */
        cap_t conn_cap_in_server;
        cap_t shm_cap_in_server;
};

/*
 * An ipc_server_thread which invokes "reigster_server" has such config.
 * This thread, which declares an IPC service in the server process,
 * will be exposed to clients. Then, clients invokes "register_client"
 * with such ipc_server_thread.
 */
struct ipc_server_config {
        /* Callback_thread for handling client registration */
        struct thread *register_cb_thread;

        /* Record the argument from the server thread */
        unsigned long declared_ipc_routine_entry;
};

/*
 * Each connection owns one shm for exchanging data between client and server.
 * Client process registers one PMO_SHM and copies the shm_cap to the server.
 * But, client and server can map the PMO_SHM at different addresses.
 */
struct shm_for_ipc_connection {
        /*
         * The starting address of the shm in the client process's vmspace.
         * uaddr: user-level virtual address.
         */
        vaddr_t client_shm_uaddr;

        /* The starting address of the shm in the server process's vmspace. */
        vaddr_t server_shm_uaddr;
        size_t shm_size;

        /* For resource recycle */
        cap_t shm_cap_in_client;
        cap_t shm_cap_in_server;
};

struct ipc_send_cap_struct {
        bool valid;
        cap_t cap;
        cap_right_t mask;
        cap_right_t rest;
};

struct ipc_connection {
        /*
         * current client who uses this connection.
         * Note that all threads in the client process can use this connection.
         */
        struct thread *current_client_thread;

        /*
         * server_handler_thread is always fixed after establishing the
         * connection.
         * i.e., ipc_server_handler_thread
         */
        struct thread *server_handler_thread;

        /*
         * Identification of the client (cap_group).
         * This badge is always fixed with the ipc_connection and
         * will be transferred to the server during each IPC.
         * Thus, the server can identify different client processes.
         *
         * NOTE: an connection cannot be shared between multiple clients.
         */
        badge_t client_badge;
        int client_pid;

        struct shm_for_ipc_connection shm;

        /* For resource recycle */
        struct lock ownership;
        cap_t conn_cap_in_client;
        cap_t conn_cap_in_server;
        int state;
        struct ipc_send_cap_struct server_cap_buf[MAX_CAP_TRANSFER];
        struct ipc_send_cap_struct client_cap_buf[MAX_CAP_TRANSFER];
};

struct ipc_msg {
        unsigned int data_len;
        unsigned int cap_slot_number;
        unsigned int data_offset;
};

void connection_deinit(void *conn);

/* IPC related system calls */
int sys_register_server(unsigned long ipc_rountine, cap_t register_cb_cap,
                        unsigned long destructor);
cap_t sys_register_client(cap_t server_cap, unsigned long vm_config_ptr);
int sys_ipc_register_cb_return(cap_t server_thread_cap,
                               unsigned long server_thread_exit_routine,
                               unsigned long server_shm_addr);

unsigned long sys_ipc_call(cap_t conn_cap, unsigned int cap_num);
int sys_ipc_return(unsigned long ret, unsigned int cap_num);
void sys_ipc_exit_routine_return(void);

cap_t sys_ipc_get_cap(cap_t conn_cap, int index);
int sys_ipc_set_cap(cap_t conn_cap, int index, cap_t cap,
                    cap_right_t mask, cap_right_t rest);

#endif /* IPC_CONNECTION_H */