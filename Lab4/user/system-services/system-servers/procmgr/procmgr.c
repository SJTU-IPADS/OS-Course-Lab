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

#define _GNU_SOURCE
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore-internal/procmgr_defs.h>
#include <uapi/get_system_info.h>
#include <pthread.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sched.h>
#include <errno.h>
#include <chcore/pthread.h>
#include <unistd.h>

#include "proc_node.h"
#include "procmgr_dbg.h"
#include "srvmgr.h"
#include "shell_msg_handler.h"
#ifdef CHCORE_OPENTRUSTEE
#include "oh_mem_ops.h"
#endif /* CHCORE_OPENTRUSTEE */

#define READ_ONCE(t) (*(volatile typeof((t)) *)(&(t)))
#define IRQ_NUM 32

static char *str_join(char *delimiter, char *strings[], int n)
{
        char buf[256];
        size_t size = 256 - 1; /* 1 for the trailing \0. */
        size_t dlen = strlen(delimiter);
        char *dst = buf;
        int i;
        for (i = 0; i < n; ++i) {
                size_t l = strlen(strings[i]);
                if (i > 0)
                        l += dlen;
                if (l > size) {
                        printf("str_join string buffer overflow\n");
                        break;
                }
                if (i > 0) {
                        strlcpy(dst, delimiter, size);
                        strlcpy(dst + dlen, strings[i], size - dlen);
                } else {
                        strlcpy(dst, strings[i], size);
                }
                dst += l;
                size -= l;
        }
        *dst = 0;
        return strdup(buf);
}

#ifdef CHCORE_OPENTRUSTEE
#define MAX_ARGC_SPAWN_PROC_NAME 3

static void handle_spawn(ipc_msg_t *ipc_msg, badge_t client_badge,
                         struct proc_request *pr)
{
        int input_argc = pr->spawn.argc;
        int envp_argc = pr->spawn.envc;
        int offset;
        char argv_text[PROC_REQ_TEXT_SIZE];
        char envp_text[PROC_REQ_ENV_TEXT_SIZE];
        char *input_argv[PROC_REQ_ARGC_MAX];
        char *envp_argv[PROC_REQ_ENVC_MAX];
        struct proc_node *proc_node;
        struct new_process_args np_args;
        int ret;

        if (input_argc > PROC_REQ_ARGC_MAX
            || envp_argc > PROC_REQ_ENVC_MAX) {
                ret = -EINVAL;
                goto out;
        }
        /* Copy to prevent TOCTTOU */
        memcpy(argv_text, pr->spawn.argv_text, PROC_REQ_TEXT_SIZE);
        memcpy(envp_text, pr->spawn.envp_text, PROC_REQ_ENV_TEXT_SIZE);

        for (int i = 0; i < input_argc; ++i) {
                /* Copy to prevent TOCTTOU */
                offset = pr->spawn.argv_off[i];
                if (offset >= PROC_REQ_TEXT_SIZE) {
                        ret = -EINVAL;
                        goto out;
                }
                input_argv[i] = &argv_text[offset];
        }
        argv_text[PROC_REQ_TEXT_SIZE - 1] = '\0';

        for (int i = 0; i < envp_argc; ++i) {
                /* Copy to prevent TOCTTOU */
                offset = pr->spawn.envp_off[i];
                if (offset >= PROC_REQ_ENV_TEXT_SIZE) {
                        ret = -EINVAL;
                        goto out;
                }
                envp_argv[i] = &envp_text[offset];
        }
        envp_text[PROC_REQ_ENV_TEXT_SIZE - 1] = '\0';

        np_args.envp_argc = envp_argc;
        np_args.envp_argv = envp_argv;
        if (pr->spawn.attr_valid) {
                np_args.stack_size = pr->spawn.attr.stack_size;
                np_args.heap_size = pr->spawn.attr.heap_size;
                memcpy(&np_args.puuid, &pr->spawn.attr.uuid, sizeof(spawn_uuid_t));
        } else {
                np_args.stack_size = MAIN_THREAD_STACK_SIZE;
                np_args.heap_size = (unsigned long)-1;
                memset(&np_args.puuid, 0, sizeof(spawn_uuid_t));
        }

        ret = procmgr_launch_process(
                input_argc,
                input_argv,
                str_join(" ", &input_argv[0], input_argc < MAX_ARGC_SPAWN_PROC_NAME ? input_argc : MAX_ARGC_SPAWN_PROC_NAME),
                true,
                client_badge,
                &np_args,
                COMMON_APP,
                &proc_node);

out:
        if (ret < 0) {
                ipc_return(ipc_msg, ret);
        } else {
                pr->spawn.main_thread_id =
                        usys_get_thread_id(proc_node->proc_mt_cap);
                ipc_return(ipc_msg, proc_node->pid);
                put_proc_node(proc_node);
        }
}
#endif /* CHCORE_OPENTRUSTEE */

static void handle_newproc(ipc_msg_t *ipc_msg, badge_t client_badge,
                           struct proc_request *pr)
{
        int input_argc = pr->newproc.argc;
        int offset;
        char argv_text[PROC_REQ_TEXT_SIZE];
        char *input_argv[PROC_REQ_ARGC_MAX];
        struct proc_node *proc_node;
        int ret;

        if (input_argc > PROC_REQ_ARGC_MAX) {
                ret = -EINVAL;
                goto out;
        }

        if (pr->newproc.proc_type != COMMON_APP &&
            pr->newproc.proc_type != TRACED_APP) {
                ret = -EINVAL;
                goto out;
        }

        /* Copy to prevent TOCTTOU */
        memcpy(argv_text, pr->newproc.argv_text, PROC_REQ_TEXT_SIZE);

        /* Translate to argv pointers from argv offsets. */
        for (int i = 0; i < input_argc; ++i) {
                /* Copy to prevent TOCTTOU */
                offset = pr->newproc.argv_off[i];
                if (offset >= PROC_REQ_TEXT_SIZE) {
                        ret = -EINVAL;
                        goto out;
                }
                input_argv[i] = &argv_text[offset];
        }
        argv_text[PROC_REQ_TEXT_SIZE - 1] = '\0';

        ret = procmgr_launch_process(
                input_argc,
                input_argv,
                str_join(" ", &input_argv[0], input_argc),
                true,
                client_badge,
                NULL,
                pr->newproc.proc_type,
                &proc_node);
        if (ret == 0) {
                ret = proc_node->pid;
                put_proc_node(proc_node);
        }

out:
        ipc_return(ipc_msg, ret);
}

static void handle_kill(ipc_msg_t *ipc_msg, struct proc_request *pr)
{
        pid_t pid = pr->kill.pid;
        struct proc_node *proc_to_kill;
        int proc_cap, ret = 0;

        debug("Kill process with pid: %d\n", pid);

        /* We only support to kill a process with the specified pid. */
        if (pid <= 0) {
                error("kill: We only support positive pid. pid: %d\n", pid);
                ret = -EINVAL;
                goto out;
        }

        proc_to_kill = get_proc_node_by_pid(pid);
        if (!proc_to_kill) {
                error("kill: No process with pid: %d\n", pid);
                ret = -ESRCH;
                goto out;
        }

        if (proc_to_kill->badge < MIN_FREE_APP_BADGE) {
                error("kill: Cannot kill system server or driver!\n");
                ret = -EINVAL;
                goto out_put_proc_node;
        }

        proc_cap = proc_to_kill->proc_cap;
        ret = usys_kill_group(proc_cap);
        debug("[procmgr] usys_kill_group return value: %d\n", ret);
        if (ret) {
                error("kill: usys_kill_group returns an error value: %d\n",
                      ret);
                ret = -EINVAL;
                goto out_put_proc_node;
        }

out_put_proc_node:
        put_proc_node(proc_to_kill);
out:
        ipc_return(ipc_msg, ret);
}

static void handle_wait(ipc_msg_t *ipc_msg, badge_t client_badge,
                        struct proc_request *pr)
{
        struct proc_node *client_proc;
        pid_t ret;
        int status;

        /* Get client_proc */
        client_proc = get_proc_node(client_badge);
        if (!client_proc) {
                ret = -EINVAL;
                ipc_return(ipc_msg, ret);
        }

        /*
         * May be we use the child thread state to identify whether we need to
         * wait child process in the future.
         */
        if (client_proc->pid == shell_pid) {
                shell_is_waiting = true;
        }

        while (1) {
                if (client_proc->pid == shell_pid && !shell_is_waiting) {
                        ret = -EINTR;
                        break;
                }

                ret = wait_reap_child(client_proc, pr->wait.pid, &status);
                if (ret == -ECHILD) {
                        /* No child corresponding to the given pid */
                        break;
                } else if (ret != 0) {
                        /* Successfully release resources of the child */
                        pr->wait.exitstatus = status;
                        break;
                }
                /* Some child processes may have exited. */
        }

        put_proc_node(client_proc);
        ipc_return(ipc_msg, ret);
}

static void handle_get_thread_cap(ipc_msg_t *ipc_msg, badge_t client_badge,
                                  struct proc_request *pr)
{
        struct proc_node *client_proc;
        struct proc_node *child;
        cap_t proc_mt_cap;
        int ret = 0;

        /* Get client_proc */
        client_proc = get_proc_node(client_badge);

        if (!client_proc) {
                ret = -EINVAL;
                goto out;
        }

        child = get_child(client_proc, pr->get_mt_cap.pid);

        if (!child) {
                ret = -ENOENT;
                goto out_put_client;
        }

        /* Found. */
        debug("Found process with pid=%d proc=%p\n", pr->get_mt_cap.pid, child);

        proc_mt_cap = child->proc_mt_cap;

        /*
         * Set the main-thread cap in the ipc_msg and
         * the following ipc_return_with_cap will transfer the cap.
         */
        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, proc_mt_cap);

        put_proc_node(child);

out_put_client:
        put_proc_node(client_proc);
out:
        ipc_return_with_cap(ipc_msg, ret);
}

void print_proc(struct proc_node *proc)
{
        cap_t proc_cap;
        int ret = 0;
        struct process_info pbuffer;
        struct thread_info *tbuffer;

        proc_cap = proc->proc_cap;
        ret = usys_get_system_info(GSI_PROCESS, (void *)&pbuffer,
                                        sizeof(struct process_info), (int)proc_cap);
        if (ret < 0) {
                printf("Error occurs when get process info!\n");
                return;
        }
        printf("process name: %s  thread cnt: %d  rss: %lu  vss: %lu\n", 
                pbuffer.cap_group_name,
                pbuffer.thread_num,
                pbuffer.rss,
                pbuffer.vss);
        
        tbuffer = malloc(sizeof(struct thread_info) *
                                pbuffer.thread_num);
        if (!tbuffer) {
                printf("Malloc failed, Error occurs when get thread info!\n");
                return;
        }
        ret = usys_get_system_info(GSI_THREAD, (void *)tbuffer,
                                        sizeof(struct thread_info) * pbuffer.thread_num,
                                        (int)proc_cap);
        if (ret < 0) {
                free(tbuffer);
                printf("Error occurs when get thread info!\n");
                return;
        }
        for (int i = 0; i < pbuffer.thread_num; i++) {
                printf("process name: %s  type: %d  state: %d  cpuid: %d  affinity: %d  prio: %d\n", 
                        pbuffer.cap_group_name, tbuffer[i].type, tbuffer[i].state, 
                        tbuffer[i].cpuid, tbuffer[i].affinity, tbuffer[i].prio);
        }
        free(tbuffer);
}

void handle_get_system_info(ipc_msg_t *ipc_msg)
{
        unsigned int *irq_buffer = calloc(IRQ_NUM, sizeof(unsigned int));
        int ret = 0;

        printf("GSI begins...\n");

        if (!irq_buffer) {
                goto out;
        }

        ret = usys_get_system_info(GSI_IRQ, (void *)irq_buffer, sizeof(unsigned int) * IRQ_NUM, 0);
        if (ret < 0) {
                goto out;
        }
        for (int i = 0; i < IRQ_NUM; i++) {
                printf("irqno: %d, hit %u times\n", i, irq_buffer[i]);
        }

        for_all_proc_do(print_proc);

        printf("GSI finished!\n");
out:
        free(irq_buffer);
        ipc_return(ipc_msg, ret);
}

#ifdef CHCORE_OPENTRUSTEE
static void handle_info_proc_by_pid(ipc_msg_t *ipc_msg, badge_t client_badge,
                                    struct proc_request *pr)
{
        struct proc_node *proc;
        int ret = 0;

        proc = get_proc_node_by_pid(pr->info_proc_by_pid.pid);

        if (proc == NULL) {
                ret = -ENOENT;
                goto out;
        }

        memcpy(&pr->info_proc_by_pid.info.uuid,
               &proc->puuid,
               sizeof(spawn_uuid_t));
        pr->info_proc_by_pid.info.stack_size = proc->stack_size;

out:
        ipc_return(ipc_msg, ret);
}
#endif /* CHCORE_OPENTRUSTEE */

static int init_procmgr(void)
{
        /* Init proc_node manager */
        init_proc_node_mgr();

        /* Init server manager */
        init_srvmgr();

#ifdef CHCORE_OPENTRUSTEE
        oh_mem_ops_init();
#endif /* CHCORE_OPENTRUSTEE */
        return 0;
}

DEFINE_SERVER_HANDLER(procmgr_dispatch)
{
        struct proc_request *pr;

        debug("new request from client_badge: 0x%x\n", client_badge);

        // if (ipc_msg->data_len < sizeof(pr->req)) {
        //         error("procmgr: no operation num\n");
        //         ipc_return(ipc_msg, -EINVAL);
        // }

        pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        debug("req: %d\n", pr->req);

        switch (pr->req) {
        case PROC_REQ_NEWPROC:
                handle_newproc(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_WAIT:
                handle_wait(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_GET_MT_CAP:
                handle_get_thread_cap(ipc_msg, client_badge, pr);
                break;
        /*
         * Get server_cap by server_id.
         * Skip get_proc_node for the following IPC REQ.
         * This is because FSM will invoke this IPC but it is not
         * booted by procmgr and @client_proc is not required in the IPC.
         */
        case PROC_REQ_GET_SERVICE_CAP:
                handle_get_service_cap(ipc_msg, pr);
                break;
        case PROC_REQ_SET_SERVICE_CAP:
                handle_set_service_cap(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_KILL:
                handle_kill(ipc_msg, pr);
                break;
#ifdef CHCORE_OPENTRUSTEE
        case PROC_REQ_SPAWN:
                handle_spawn(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_INFO_PROC_BY_PID:
                handle_info_proc_by_pid(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_TRANSFER_PMO_CAP:
                handle_transfer_pmo_cap(ipc_msg, client_badge);
                break;
        case PROC_REQ_TEE_ALLOC_SHM:
                handle_tee_alloc_sharemem(ipc_msg, client_badge);
                break;
        case PROC_REQ_TEE_GET_SHM:
                handle_tee_get_sharemem(ipc_msg, client_badge);
                break;
        case PROC_REQ_TEE_FREE_SHM:
                handle_tee_free_sharemem(ipc_msg, client_badge);
                break;
#endif /* CHCORE_OPENTRUSTEE */
        case PROC_REQ_RECV_SIG:
                handle_recv_sig(ipc_msg, pr);
                break;
        case PROC_REQ_CHECK_STATE:
                handle_check_state(ipc_msg, client_badge, pr);
                break;
        case PROC_REQ_GET_SHELL_CAP:
                handle_get_shell_cap(ipc_msg);
                break;
        case PROC_REQ_SET_SHELL_CAP:
                handle_set_shell_cap(ipc_msg, client_badge);
                break;
        case PROC_REQ_GET_TERMINAL_CAP:
                handle_get_terminal_cap(ipc_msg);
                break;
        case PROC_REQ_SET_TERMINAL_CAP:
                handle_set_terminal_cap(ipc_msg);
                break;
	case PROC_REQ_GET_SYSTEM_INFO:
		handle_get_system_info(ipc_msg);
		break;
        default:
                error("Invalid request type!\n");
                /* Client should check if the return value is correct */
                ipc_return(ipc_msg, -EBADRQC);
                break;
        }
}

void *recycle_routine(void *arg);

/*
 * Procmgr is the first user process and there is no system services now.
 * So, override the default libc_connect_services.
 */
void libc_connect_services(char *envp[])
{
        procmgr_ipc_struct->conn_cap = 0;
        procmgr_ipc_struct->server_id = PROC_MANAGER;
        return;
}

void boot_default_servers(void)
{
        char *srv_path;
        cap_t tmpfs_cap;
        struct proc_node *proc_node;
        int ret;

        /* Do not modify the order of creating system servers */
        printf("User Init: booting fs server (FSMGR and real FS) \n");

        srv_path = "/tmpfs.srv";
        ret = procmgr_launch_basic_server(
                1, &srv_path, "tmpfs", true, INIT_BADGE, &proc_node);
        if (ret < 0) {
                BUG("procmgr_launch_basic_server tmpfs failed");
        }
        tmpfs_cap = proc_node->proc_mt_cap;
        put_proc_node(proc_node);
        /*
         * We set the cap of tmpfs before starting fsm to ensure that tmpfs is
         * available after fsm is started.
         */
        set_tmpfs_cap(tmpfs_cap);

        /* FSM gets badge 2 and tmpfs uses the fixed badge (10) for it */
        srv_path = "/fsm.srv";
        ret = procmgr_launch_basic_server(
                1, &srv_path, "fsm", true, INIT_BADGE, &proc_node);
        if (ret < 0) {
                BUG("procmgr_launch_basic_server fsm failed");
        }
        fsm_server_cap = proc_node->proc_mt_cap;
        fsm_ipc_struct->server_id = FS_MANAGER;
        put_proc_node(proc_node);

#ifdef CHCORE_OPENTRUSTEE
        return;
#endif /* CHCORE_OPENTRUSTEE */

#if defined(CHCORE_SERVER_LWIP)
        printf("User Init: booting network server\n");
        /* Pass the FS cap to NET since it may need to read some config files */
        /* Net gets badge 3 */
        srv_path = "/lwip.srv";
        ret = procmgr_launch_process(1,
                                     &srv_path,
                                     "lwip",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     SYSTEM_SERVER,
                                     &proc_node);
        if (ret < 0) {
                BUG("procmgr_launch_process lwip failed");
        }
        lwip_server_cap = proc_node->proc_mt_cap;
        put_proc_node(proc_node);
#endif
}

void *handler_thread_routine(void *arg)
{
        int ret;
        ret = ipc_register_server(procmgr_dispatch,
                                  DEFAULT_CLIENT_REGISTER_HANDLER);
        printf("[procmgr] register server value = %d\n", ret);
        usys_wait(usys_create_notifc(), 1, NULL);
        return NULL;
}

void boot_default_apps(void)
{
#ifdef CHCORE_OPENTRUSTEE
        char *chanmgr_argv = "/chanmgr.srv";
        char *gtask_argv = "/gtask.elf";
        struct proc_node *gtask_node;
        int ret;

        /* Start ipc channel manager for OpenTrustee. */
        ret = procmgr_launch_process(1,
                                     &chanmgr_argv,
                                     "chanmgr",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     COMMON_APP,
                                     NULL);
        BUG_ON(ret != 0);
        /* Start OpenTrustee gtask. */
        ret = procmgr_launch_process(1,
                                     &gtask_argv,
                                     "gtask",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     SYSTEM_SERVER,
                                     &gtask_node);
        BUG_ON(ret != 0);
        printf("%s: gtask pid %d\n", __func__, gtask_node->pid);
        put_proc_node(gtask_node);
        return;
#endif /* CHCORE_OPENTRUSTEE */

        /* Start shell. */

        char *test_ipc_argv= "test_ipc.bin";
        (void)procmgr_launch_process(1,
                                     &test_ipc_argv,
                                     "test_ipc",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     COMMON_APP,
                                     NULL);

        char *shell_argv = "chcore_shell.bin";
        (void)procmgr_launch_process(1,
                                     &shell_argv,
                                     "chcore-shell",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     COMMON_APP,
                                     NULL);

#if defined(CHCORE_PLAT_RASPI3) && defined(CHCORE_SERVER_GUI)
        char *terminal_argv = "terminal.bin";
        (void)procmgr_launch_process(1,
                                     &terminal_argv,
                                     "terminal",
                                     true,
                                     INIT_BADGE,
                                     NULL,
                                     COMMON_APP,
                                     NULL);
#endif
}

static void bind_to_cpu(int cpu)
{
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);

        /*
         * Invoke usys_yield to ensure current thread to run on the target CPU.
         * If current thread is running on another CPU, it will be migrated to
         * the specific CPU after setting affinity.
        */
        usys_yield();
}

void *thread_routine1(void *arg)
{
        bind_to_cpu(0);
        for (int i = 0; i < 3; ++i) {
                printf("Hello from thread 2\n");
                usys_yield();
        }
        printf("Cooperative Schedluing Test Done!\n");
        return NULL;
}

void *thread_routine2(void *arg)
{
        printf("Hello, I am thread 3. I'm spinning.\n");

        while (1) {
        }
        return 0;
}

void test_sched(void)
{
        pthread_t tid;
        pthread_create(&tid, NULL, thread_routine1, NULL);
        usys_yield();
        for (int i = 0; i < 3; ++i) {
                printf("Hello from thread 1\n");
                usys_yield();
        }
        pthread_join(tid, NULL);

        printf("Thread 1 creates a spinning thread!\n");
        pthread_create(&tid, NULL, thread_routine2, NULL);
        usys_yield();
        printf("Thread 1 successfully regains the control!\n");
        printf("Preemptive Schedluing Test Done!\n");
}

int main(int argc, char *argv[], char *envp[])
{
        cap_t cap;
        pthread_t recycle_thread;
        pthread_t procmgr_handler_tid;

        pthread_create(&recycle_thread, NULL, recycle_routine, NULL);

        init_procmgr();
        cap = chcore_pthread_create(
                &procmgr_handler_tid, NULL, handler_thread_routine, NULL);

        __procmgr_server_cap = cap;

        init_root_proc_node();

        test_sched();
        boot_default_servers();

        /* Configure system servers, and boot some of them. */
        boot_secondary_servers();

        boot_default_apps();

#ifndef CHCORE_OPENTRUSTEE
        /* Boot configurable daemon services */
        start_daemon_service();
#endif /* CHCORE_OPENTRUSTEE */

        usys_wait(usys_create_notifc(), 1, NULL);
        return 0;
}
