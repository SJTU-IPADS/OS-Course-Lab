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

#pragma once

#include <chcore/type.h>
#include <chcore/defs.h>
#include <stddef.h>
#include <uapi/ipc.h>
#include <errno.h>
#ifdef CHCORE_OPENTRUSTEE
#include <uapi/opentrustee/ipc.h>
#endif /* CHCORE_OPENTRUSTEE */

#ifdef __cplusplus
extern "C" {
#endif

/* Starts from 1 because the uninitialized value is 0 */
enum system_server_identifier {
        FS_MANAGER = 1,
        NET_MANAGER,
        PROC_MANAGER,
};

/*
 * ipc_struct is created in **ipc_register_client** and
 * thus only used at client side.
 */
typedef struct ipc_struct {
        /* Connection_cap: used to call a server */
        cap_t conn_cap;
        /* Shared memory: used to create ipc_msg (client -> server) */
        unsigned long shared_buf;
        unsigned long shared_buf_len;

        /* A spin lock: used to coordinate the access to shared memory */
        volatile int lock;
        enum system_server_identifier server_id;
} ipc_struct_t;

extern cap_t fsm_server_cap;
extern cap_t lwip_server_cap;
extern cap_t procmgr_server_cap;
extern int init_process_in_lwip;
extern int init_lwip_lock;

/* ipc_struct for invoking system servers.
 * fsm_ipc_struct and lwip_ipc_struct are two addresses.
 * They can be used like **const** pointers.
 *
 * If a system server is related to the scalability (multi-threads) of
 * applications, we should use the following way to make the connection with it
 * as per-thread.
 *
 * For other system servers (e.g., process manager), it is OK to let multiple
 * threads share a same connection.
 */
ipc_struct_t *__fsm_ipc_struct_location(void);
ipc_struct_t *__net_ipc_struct_location(void);
ipc_struct_t *__procmgr_ipc_struct_location(void);
#define fsm_ipc_struct     (__fsm_ipc_struct_location())
#define lwip_ipc_struct    (__net_ipc_struct_location())
#define procmgr_ipc_struct (__procmgr_ipc_struct_location())

/** 
 * We use a compile time check in chcore-port/ipc.c to ensure
 * that the size of struct ipc_msg won't exceed the size of a
 * char[SERVER_IPC_MSG_BUF_SIZE].
 * 
 * With such a guarantee, we can use an on-stack buffer to 
 * initialize server-side ipc_msg instead of calling malloc() or
 * free() on IPC critical path, while keeping ipc_msg_t an opaque
 * type outside of IPC library.
 */
#define SERVER_IPC_MSG_BUF_SIZE (64)
typedef struct ipc_msg ipc_msg_t;
void __ipc_server_init_raw_msg(ipc_msg_t *ipc_msg, void *shm_ptr, unsigned int max_data_len, unsigned int cap_num);

#define IPC_SHM_AVAILABLE (IPC_PER_SHM_SIZE - sizeof(struct ipc_response_hdr))

#ifndef CHCORE_OPENTRUSTEE
#ifdef CHCORE_ARCH_X86_64
#define DECLARE_SERVER_HANDLER(name) __attribute__((naked)) void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, badge_t badge)

#define DEFINE_SERVER_HANDLER(name) \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge); \
__attribute__((visibility("hidden"))) void _##name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, badge_t badge) { \
        char buf[SERVER_IPC_MSG_BUF_SIZE]; \
        ipc_msg_t *ipc_msg = (ipc_msg_t *)buf; \
        __ipc_server_init_raw_msg(ipc_msg, shm_ptr, max_data_len, cap_num); \
        __##name(ipc_msg, badge); \
} \
__attribute__((naked)) void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, badge_t badge) { \
        __asm__ volatile("mov %%r10, %%rcx \n" \
                         "jmp _" #name:::"rcx"); \
} \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge)
#else
#define DECLARE_SERVER_HANDLER(name) void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, badge_t badge)

#define DEFINE_SERVER_HANDLER(name) \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge); \
void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, badge_t badge) { \
        char buf[SERVER_IPC_MSG_BUF_SIZE]; \
        ipc_msg_t *ipc_msg = (ipc_msg_t *)buf; \
        __ipc_server_init_raw_msg(ipc_msg, shm_ptr, max_data_len, cap_num); \
        __##name(ipc_msg, badge); \
} \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge)
#endif
#else /* CHCORE_OPENTRUSTEE */
#define DECLARE_SERVER_HANDLER(name) void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, unsigned int task_id)

#define DEFINE_SERVER_HANDLER(name) \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge); \
void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, unsigned int task_id) { \
        char buf[SERVER_IPC_MSG_BUF_SIZE]; \
        ipc_msg_t *ipc_msg = (ipc_msg_t *)buf; \
        __ipc_server_init_raw_msg(ipc_msg, shm_ptr, max_data_len, cap_num); \
        __##name(ipc_msg, taskid_to_pid(task_id)); \
} \
static void __##name(ipc_msg_t *ipc_msg, badge_t client_badge)

#define DEFINE_SERVER_HANDLER_TASKID(name) \
static void __##name(ipc_msg_t *ipc_msg, pid_t pid, cap_t tid); \
void name(void *shm_ptr, unsigned int max_data_len, unsigned int cap_num, unsigned int task_id) { \
        char buf[SERVER_IPC_MSG_BUF_SIZE]; \
        ipc_msg_t *ipc_msg = (ipc_msg_t *)buf; \
        __ipc_server_init_raw_msg(ipc_msg, shm_ptr, max_data_len, cap_num); \
        __##name(ipc_msg, taskid_to_pid(task_id), taskid_to_tid(task_id)); \
} \
static void __##name(ipc_msg_t *ipc_msg, pid_t pid, cap_t tid)
#endif /* CHCORE_OPENTRUSTEE */

typedef void (*server_destructor)(badge_t);

/* Registeration interfaces */
ipc_struct_t *ipc_register_client(cap_t server_thread_cap);

void *register_cb(void *ipc_handler);
void *register_cb_single(void *ipc_handler);

#define DEFAULT_CLIENT_REGISTER_HANDLER register_cb
#define DEFAULT_DESTRUCTOR              NULL

int ipc_register_server(server_handler server_handler,
                        void *(*client_register_handler)(void *));
int ipc_register_server_with_destructor(server_handler server_handler,
                                        void *(*client_register_handler)(void *),
                                        server_destructor server_destructor);

/* IPC message operating interfaces */
ipc_msg_t *ipc_create_msg(ipc_struct_t *icb, unsigned int data_len);
ipc_msg_t *ipc_create_msg_with_cap(ipc_struct_t *icb, unsigned int data_len, unsigned int send_cap_num);
char *ipc_get_msg_data(ipc_msg_t *ipc_msg);
cap_t ipc_get_msg_cap(ipc_msg_t *ipc_msg, unsigned int cap_id);
int ipc_set_msg_data(ipc_msg_t *ipc_msg, void *data, unsigned int offset, unsigned int len);
int ipc_set_msg_cap_restrict(ipc_msg_t *ipc_msg, unsigned int cap_slot_index,
                             cap_t cap, cap_right_t mask, cap_right_t rest);
int ipc_set_msg_cap(ipc_msg_t *ipc_msg, unsigned int cap_slot_index, cap_t cap);
unsigned int ipc_get_msg_send_cap_num(ipc_msg_t *ipc_msg);
void ipc_set_msg_send_cap_num(ipc_msg_t *ipc_msg, unsigned int cap_num);
unsigned int ipc_get_msg_return_cap_num(ipc_msg_t *ipc_msg);
void ipc_set_msg_return_cap_num(ipc_msg_t *ipc_msg, unsigned int cap_num);
int ipc_destroy_msg(ipc_msg_t *ipc_msg);

/* IPC issue/finish interfaces */
long ipc_call(ipc_struct_t *icb, ipc_msg_t *ipc_msg);
_Noreturn void ipc_return(ipc_msg_t *ipc_msg, long ret);
_Noreturn void ipc_return_with_cap(ipc_msg_t *ipc_msg, long ret);
int ipc_client_close_connection(ipc_struct_t *ipc_struct);

int simple_ipc_forward(ipc_struct_t *ipc_struct, void *data, int len);

/*
 * Magic number for coordination between client and server:
 * the client should wait until the server has reigsterd the service.
 */
#define NONE_INFO ((void *)(-1UL))

#ifdef __cplusplus
}
#endif
