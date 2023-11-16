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

#include "proc_node.h"
#include "procmgr_dbg.h"
#include "srvmgr.h"
#include "shell_msg_handler.h"

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

static void handle_newproc(ipc_msg_t *ipc_msg, badge_t client_badge,
                           struct proc_request *pr)
{
        int input_argc = pr->newproc.argc;
        char *input_argv[PROC_REQ_ARGC_MAX];
        struct proc_node *proc_node;

        if (input_argc > PROC_REQ_ARGC_MAX) {
                ipc_return(ipc_msg, -EINVAL);
        }

        if (pr->newproc.proc_type != COMMON_APP &&
            pr->newproc.proc_type != TRACED_APP) {
                ipc_return(ipc_msg, -EINVAL);
        }

        /* Translate to argv pointers from argv offsets. */
        for (int i = 0; i < input_argc; ++i) {
                if (pr->newproc.argv_off[i] >= PROC_REQ_TEXT_SIZE) {
                        ipc_return(ipc_msg, -EINVAL);
                }
                input_argv[i] = &pr->newproc.argv_text[pr->newproc.argv_off[i]];
        }
        pr->newproc.argv_text[PROC_REQ_TEXT_SIZE - 1] = '\0';

        proc_node = procmgr_launch_process(
                input_argc,
                input_argv,
                str_join(" ", &input_argv[0], input_argc),
                true,
                client_badge,
                NULL,
                pr->newproc.proc_type);
        if (proc_node == NULL) {
                ipc_return(ipc_msg, -1);
        } else {
                ipc_return(ipc_msg, proc_node->pid);
        }
}

static void handle_kill(ipc_msg_t *ipc_msg, struct proc_request *pr)
{
        pid_t pid = pr->kill.pid;
        struct proc_node *proc_to_kill;
        int proc_cap, ret;

        debug("Kill process with pid: %d\n", pid);

        /* We only support to kill a process with the specified pid. */
        if (pid <= 0) {
                error("kill: We only support positive pid. pid: %d\n", pid);
                ipc_return(ipc_msg, -EINVAL);
        }

        proc_to_kill = get_proc_node_by_pid(pid);
        if (!proc_to_kill) {
                error("kill: No process with pid: %d\n", pid);
                ipc_return(ipc_msg, -ESRCH);
        }

        proc_cap = proc_to_kill->proc_cap;
        ret = usys_kill_group(proc_cap);
        debug("[procmgr] usys_kill_group return value: %d\n", ret);
        if (ret) {
                error("kill: usys_kill_group returns an error value: %d\n", ret);
                ipc_return(ipc_msg, -EINVAL);
        }

        ipc_return(ipc_msg, 0);
}

static void handle_wait(ipc_msg_t *ipc_msg, badge_t client_badge,
                        struct proc_request *pr)
{
        struct proc_node *client_proc;
        struct proc_node *child;
        struct proc_node *proc;
        pid_t ret_pid;

        /* Get client_proc */
        client_proc = get_proc_node(client_badge);
        assert(client_proc);

        /*
         * May be we use the child thread state to identify whether we need to
         * wait child process in the future.
         */
        if (client_proc->pid == shell_pid) {
                shell_is_waiting = true;
        }

        while (1) {
                /* Use a lock to synsynchronize this function and del_proc_node
                 */
                pthread_mutex_lock(&client_proc->lock);
                child = NULL;
                for_each_in_list (
                        proc, struct proc_node, node, &client_proc->children) {
                        if (proc->pid == pr->wait.pid || pr->wait.pid == -1) {
                                child = proc;
                                break;
                        }
                }

                if (!child
                    || (child->pid != pr->wait.pid && pr->wait.pid != -1)) {
                        /* wrong pid */
                        pthread_mutex_unlock(&client_proc->lock);
                        ipc_return(ipc_msg, -ESRCH);
                }

                if (client_proc->pid == shell_pid
                    && (!READ_ONCE(shell_is_waiting))) {
                        pthread_mutex_unlock(&client_proc->lock);
                        ipc_return(ipc_msg, -EINTR);
                }

                /* Found. */
                debug("Found process with pid=%d proc=%p\n", pr->pid, child);

                if (READ_ONCE(child->state) == PROC_STATE_EXIT) {
                        /*
                         * The exit status has been set but the node
                         * has not been removed from its parent process`s
                         * child list.
                         */
                        debug("Process (pid=%d, proc=%p) exits with %d\n",
                              pr->wait.pid,
                              child,
                              child->exitstatus);
                        pr->wait.exitstatus = child->exitstatus;
                        ret_pid = child->pid;
                        /*
                         * Delete the child node from the children list of
                         * parent. and recycle the proc node of child.
                         */
                        pthread_mutex_lock(&recycle_lock);
                        list_del(&child->node);
                        free_proc_node_resource(child);
                        free(child);
                        pthread_mutex_unlock(&recycle_lock);
                        pthread_mutex_unlock(&client_proc->lock);
                        ipc_return(ipc_msg, ret_pid);
                } else {
                        /* Child process has not exited yet, try again later */
                        pthread_mutex_unlock(&client_proc->lock);
                        usys_yield();
                }
        }
}

static void handle_get_thread_cap(ipc_msg_t *ipc_msg, badge_t client_badge,
                                  struct proc_request *pr)
{
        struct proc_node *client_proc;
        struct proc_node *child;
        struct proc_node *proc;
        cap_t proc_mt_cap;

        /* Get client_proc */
        client_proc = get_proc_node(client_badge);
        assert(client_proc);

        pthread_mutex_lock(&client_proc->lock);

        child = NULL;
        for_each_in_list (
                proc, struct proc_node, node, &client_proc->children) {
                if (proc->pid == pr->get_mt_cap.pid) {
                        child = proc;
                }
        }
        if (!child
            || (child->pid != pr->get_mt_cap.pid && pr->get_mt_cap.pid != -1)) {
                pthread_mutex_unlock(&client_proc->lock);
                ipc_return(ipc_msg, -ENOENT);
        }

        /* Found. */
        debug("Found process with pid=%d proc=%p\n", pr->get_mt_cap.pid, child);

        proc_mt_cap = child->proc_mt_cap;

        pthread_mutex_unlock(&client_proc->lock);

        /*
         * Set the main-thread cap in the ipc_msg and
         * the following ipc_return_with_cap will transfer the cap.
         */
        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, proc_mt_cap);
        ipc_return_with_cap(ipc_msg, 0);
}

static int get_all_child_process_info(struct proc_node *root_proc)
{
        struct proc_node *proc_node;
        cap_t proc_cap;
        int ret = 0;
        struct process_info pbuffer;
        struct thread_info *tbuffer;

        for_each_in_list (proc_node, struct proc_node, node, &root_proc->children) {
                proc_cap = proc_node->proc_cap;
                ret = usys_get_system_info(GSI_PROCESS, (void *)&pbuffer,
                                                sizeof(struct process_info), (int)proc_cap);
                if (ret < 0) {
                        printf("Error occurs when get process info!");
                        return ret;
                }
                printf("process name: %s  thread cnt: %d  rss: %lu  vss: %lu\n", 
                        pbuffer.cap_group_name,
                        pbuffer.thread_num,
                        pbuffer.rss,
                        pbuffer.vss);
                
                tbuffer = malloc(sizeof(struct thread_info) *
                                 pbuffer.thread_num);
                ret = usys_get_system_info(GSI_THREAD, (void *)tbuffer,
                                           sizeof(struct thread_info) * pbuffer.thread_num,
                                           (int)proc_cap);
                if (ret < 0) {
                        free(tbuffer);
                        printf("Error occurs when get thread info!");
                        return ret;
                }
                for (int i = 0; i < pbuffer.thread_num; i++) {
                        printf("process name: %s  type: %d  state: %d  cpuid: %d  affinity: %d  prio: %d\n", 
                                pbuffer.cap_group_name, tbuffer[i].type, tbuffer[i].state, 
                                tbuffer[i].cpuid, tbuffer[i].affinity, tbuffer[i].prio);
                }
                free(tbuffer);
                
                if (!list_empty(&proc_node->children)) {
                        ret = get_all_child_process_info(proc_node);
                        if (ret < 0)
                                return ret;
                }
        }

        return ret;
}

void handle_get_system_info(ipc_msg_t *ipc_msg)
{
        unsigned int *irq_buffer = calloc(IRQ_NUM, sizeof(unsigned int));
        int ret = 0;
        struct proc_node *proc = get_proc_node(INIT_BADGE);

        printf("GSI begins...\n");

        ret = usys_get_system_info(GSI_IRQ, (void *)irq_buffer, sizeof(unsigned int) * IRQ_NUM, 0);
        if (ret < 0) {
                goto out;
        }
        for (int i = 0; i < IRQ_NUM; i++) {
                printf("irqno: %d, hit %u times\n", i, irq_buffer[i]);
        }

	ret = get_all_child_process_info(proc);
        if (ret < 0) {
                goto out;
        }

        printf("GSI finished!\n");
out:
        free(irq_buffer);
        ipc_return(ipc_msg, ret);
}

static int init_procmgr(void)
{
        /* Init proc_node manager */
        init_proc_node_mgr();

        /* Init server manager */
        init_srvmgr();
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
        case PROC_REQ_GET_SERVER_CAP:
                handle_get_server_cap(ipc_msg, pr);
                break;
        case PROC_REQ_KILL:
                handle_kill(ipc_msg, pr);
                break;
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

        /* Do not modify the order of creating system servers */
        printf("User Init: booting fs server (FSMGR and real FS) \n");

        srv_path = "/tmpfs.srv";
        proc_node = procmgr_launch_basic_server(
                1, &srv_path, "tmpfs", true, INIT_BADGE);
        if (proc_node == NULL) {
                BUG("procmgr_launch_basic_server tmpfs failed");
        }
        tmpfs_cap = proc_node->proc_mt_cap;
        /*
         * We set the cap of tmpfs before starting fsm to ensure that tmpfs is
         * available after fsm is started.
         */
        set_tmpfs_cap(tmpfs_cap);

        /* FSM gets badge 2 and tmpfs uses the fixed badge (10) for it */
        srv_path = "/fsm.srv";
        proc_node = procmgr_launch_basic_server(
                1, &srv_path, "fsm", true, INIT_BADGE);
        if (proc_node == NULL) {
                BUG("procmgr_launch_basic_server fsm failed");
        }
        fsm_server_cap = proc_node->proc_mt_cap;
        fsm_ipc_struct->server_id = FS_MANAGER;

        printf("User Init: booting network server\n");
        /* Pass the FS cap to NET since it may need to read some config files */
        /* Net gets badge 3 */
        srv_path = "/lwip.srv";
        proc_node = procmgr_launch_process(
                1, &srv_path, "lwip", true, INIT_BADGE, NULL, SYSTEM_SERVER);
        if (proc_node == NULL) {
                BUG("procmgr_launch_process lwip failed");
        }
        lwip_server_cap = proc_node->proc_mt_cap;
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
        char *shell_argv = "chcore_shell.bin";
        char *userland_argv = "userland.bin";
        char *hello_argv = "hello_chcore.bin";

#define CONFIG_MACHINE_ARM 0
#if CONFIG_MACHINE_ARM == 1
        /*
         * Because the machine arm occupy the serial port, so ChCore can not
         * receive keybord and send characters to screen from uart now. We start
         * the arm server at here for test. It can be deleted when hdmi and
         * keyboard driver are ready.
         */
        char *args[1];
        args[0] = "/machine_arm.bin";

        procmgr_launch_process(
                1, args, "machine_arm", true, INIT_BADGE, NULL, COMMON_APP);
#endif

        /* Start shell. */
        procmgr_launch_process(
                1, &shell_argv, "chcore-shell", true, INIT_BADGE, NULL, COMMON_APP);
 
        /* Lab 3 Test */
        procmgr_launch_process(
                1, &userland_argv, "userland", true, INIT_BADGE, NULL, COMMON_APP);
        procmgr_launch_process(
                1, &hello_argv, "hello_chcore", true, INIT_BADGE, NULL, COMMON_APP);
        /* Lab 3 Test End*/

#if defined(CHCORE_PLAT_RASPI3) && defined(CHCORE_SERVER_GUI)
        char *terminal_argv = "terminal.bin";
        procmgr_launch_process(
                1, &terminal_argv, "terminal", true, INIT_BADGE, NULL, COMMON_APP);
#endif
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

        boot_default_servers();

        boot_default_apps();

        /* Boot some configurable servers which should be booted lazily */
        boot_secondary_servers();

        /* Boot configurable daemon services */
        start_daemon_service();

        usys_wait(usys_create_notifc(), 1, NULL);
        return 0;
}
