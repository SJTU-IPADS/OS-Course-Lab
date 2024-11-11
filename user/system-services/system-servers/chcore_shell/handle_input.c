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
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/stat.h>
#include <pthread.h>

#include <chcore/syscall.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <chcore/memory.h>
#include <chcore-internal/fs_defs.h>
#include <chcore-internal/shell_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/string.h>

#include "handle_input.h"
#include "job_control.h"
#include "buildin_cmd.h"

/* The buffer used to store the character from uart port and terminal. */
static char input_buf[INPUT_BUFLEN];
/* The write/read position of input_buf. */
static u32 rpos = 0, wpos = 0;
/* The mutex of input_buf */
static pthread_mutex_t input_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Clear uart buffer. */
void flush_buffer(void)
{
        pthread_mutex_lock(&input_buf_mutex);
        rpos = wpos;
        pthread_mutex_unlock(&input_buf_mutex);
}

/* Get char from input_buffer. It is used by shell. */
char shell_getchar(void)
{
        char ch;

        pthread_mutex_lock(&input_buf_mutex);
        do {
                if (wpos == rpos) {
                        pthread_mutex_unlock(&input_buf_mutex);
                        sched_yield();
                        pthread_mutex_lock(&input_buf_mutex);
                } else {
                        break;
                }
        } while (1);
        ch = input_buf[rpos++];
        if (rpos == INPUT_BUFLEN) {
                rpos = 0;
        }
        pthread_mutex_unlock(&input_buf_mutex);
        return ch;
}

void redisplay_on_screen(int ch)
{
        if (ch == '\r')
                ch = '\n';
        else if (ch == '\x7f' /* DELETE */) {
                /* FIXME: currently we see DELETE as BACKSPACE */
                ch = '\b';
        }
        if (ch != '\b') {
                putchar(ch);
                fflush(stdout);
        } else {
                putchar('\b');
                putchar(' ');
                putchar('\b');
                fflush(stdout);
        }
}

/* Fill character to the buffer of foreground process. */
void fill_foreground_process_stdin_buffer(void)
{
        int *fore_rpos, *fore_wpos;

        BUG_ON(foreground_buffer_addr == NULL);
        fore_rpos = (int *)foreground_pmo_addr;
        fore_wpos = fore_rpos + 1;
        pthread_mutex_lock(&input_buf_mutex);
        while (rpos != wpos
               && (((*fore_wpos) + 1) % INPUT_BUFLEN)
                          != ((*fore_rpos) % INPUT_BUFLEN)) {
                char ch = input_buf[rpos];
                redisplay_on_screen(ch);

                ((char *)foreground_buffer_addr)[*fore_wpos] =
                        input_buf[rpos++];

                *fore_wpos = (*fore_wpos) + 1;

                if (rpos == INPUT_BUFLEN) {
                        rpos = 0;
                }
                if (*fore_wpos == INPUT_BUFLEN) {
                        *fore_wpos = 0;
                }
        }
        pthread_mutex_unlock(&input_buf_mutex);
}

int handle_ctrlz(char ch)
{
        struct job *new_job;
        int pos;

        /* There is no foreground thread. So it return directly. */
        if (fg_job.pid == -1) {
                return 1;
        }

        /* Add the foreground thread to the background job list. */
        pthread_mutex_lock(&job_mutex);
        new_job = (struct job *)malloc(sizeof(struct job));
        if (new_job == NULL) {
                return 1;
        }
        new_job->notific_cap = fg_job.notific_cap;
        new_job->pid = fg_job.pid;
        new_job->pmo_cap = fg_job.pmo_cap;
        strlcpy(new_job->job_name, fg_job.job_name, sizeof(new_job->job_name));

        clean_jobs();
        pos = add_job(new_job->job_name,
                      new_job->pid,
                      new_job->pmo_cap,
                      new_job->notific_cap);
        pthread_mutex_unlock(&job_mutex);

        if (pos < 0) {
                goto out;
        }

        fg_job.pid = -1;

        /* Send this signal to procmgr to awake the main thread of shell. */
        ipc_msg_t *msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        struct proc_request *proc_req =
                (struct proc_request *)ipc_get_msg_data(msg);
        proc_req->req = PROC_REQ_RECV_SIG;
        proc_req->recv_sig.sig = ch;
        ipc_call(procmgr_ipc_struct, msg);
        ipc_destroy_msg(msg);
        printf("[%d] + %d background %s\n",
               pos + 1,
               new_job->pid,
               new_job->job_name);

        /* Discard the input before ctrl + z. */
out:
        flush_buffer();
        free(new_job);
        return 1;
}

/*
 * Check the special character. It just check CTRL + Z now.
 * When shell receive a special character, it need send this msg to here to
 * handle some extra operation. For example, if shell receive a ctrl+z, procmgr
 * need awake the shell main thread.(Shell's main thread will loop on the
 * waitpid when execute a foreground process)
 */
int check_ch(char ch)
{
        int ret = 0;

        switch (ch) {
        case CTRL('Z'):
                ret = handle_ctrlz(ch);
                break;
        default:
                break;
        }
        return ret;
}

/*
 * FIXME: We just implement the uart interrupt on raspi board now and we make
 * shell to handle a part of uart interrupt. So we need to distinguish between
 * raspi platform and other platform. It is not elegant and low scalability.
 * Maybe we can add a new level to handle interrupt and receive character or try
 * other method to change this implementation.
 */
#ifdef UART_IRQ
void handle_uart_irq_internal(void)
{
        char ch;

        do {
                ch = usys_getc();
                if (ch == (char)-1)
                        break;

                if (check_ch(ch)) {
                        break;
                }
                pthread_mutex_lock(&input_buf_mutex);
                input_buf[wpos] = ch;
                wpos++;
                if (wpos == INPUT_BUFLEN) {
                        wpos = 0;
                }
                if (wpos == rpos) {
                        rpos++;
                        if (rpos == INPUT_BUFLEN) {
                                rpos = 0;
                        }
                }
                pthread_mutex_unlock(&input_buf_mutex);
        } while (1);

        // pthread_mutex_lock(&job_mutex);
        if (pthread_mutex_trylock(&job_mutex) == 0) {
                if (fg_job.pid > 0 && fg_job.notific_cap > 0) {
                        fill_foreground_process_stdin_buffer();
                        usys_notify(fg_job.notific_cap);
                }
                pthread_mutex_unlock(&job_mutex);
        }
        // pthread_mutex_unlock(&job_mutex);
}

void *handle_uart_irq(void *arg)
{
        cap_t irq_cap;

        irq_cap = usys_irq_register(UART_IRQ);
        BUG_ON(irq_cap < 0);

        while (1) {
                usys_irq_wait(irq_cap, true);
                // printf("shell irq receive:%d\n");
                handle_uart_irq_internal();
        }

        return NULL;
}

#else

/* Polling for input. */
void *other_plat_get_char(void *arg)
{
        char ch;

        while (1) {
                do {
                        ch = usys_getc();
                        if (ch == (char)-1)
                                break;

                        if (check_ch(ch)) {
                                break;
                        }
                        pthread_mutex_lock(&input_buf_mutex);
                        input_buf[wpos] = ch;
                        wpos++;
                        if (wpos == INPUT_BUFLEN) {
                                wpos = 0;
                        }
                        if (wpos == rpos) {
                                rpos++;
                                if (rpos == INPUT_BUFLEN) {
                                        rpos = 0;
                                }
                        }
                        pthread_mutex_unlock(&input_buf_mutex);
                } while (1);

                // pthread_mutex_lock(&job_mutex);
                if (pthread_mutex_trylock(&job_mutex) == 0) {
                        if (fg_job.pid > 0 && fg_job.notific_cap > 0) {
                                fill_foreground_process_stdin_buffer();
                                usys_notify(fg_job.notific_cap);
                        }
                        pthread_mutex_unlock(&job_mutex);
                }
                sched_yield();
        }
        return NULL;
}
#endif

/* Record the infomation of process which request the user's input. */
void set_process_info(cap_t notific_cap, cap_t buffer_cap, pid_t pid)
{
        int i = 0, j = 0;
        int total_job;

        // printf("pid:%d %d\n", pid, fg_job.pid);
        pthread_mutex_lock(&job_mutex);
        if (pid == fg_job.pid) {
                fg_job.notific_cap = notific_cap;
                fg_job.pmo_cap = buffer_cap;
                foreground_pmo_addr = (void *)chcore_auto_map_pmo(
                        buffer_cap, PAGE_SIZE, PROT_READ | PROT_WRITE);
                BUG_ON(foreground_pmo_addr == NULL);
                foreground_buffer_addr = foreground_pmo_addr + sizeof(u32) * 2;
        } else {
                total_job = job_count;
                for (i = 0; i < total_job && j < JOBS_MAX;) {
                        /*
                         * Make an assumption that the thread which gets input
                         * have not exited.
                         */
                        if (bg_jobs[j] != NULL) {
                                if (bg_jobs[j]->pid == pid) {
                                        bg_jobs[j]->pmo_cap = buffer_cap;
                                        bg_jobs[j]->notific_cap = notific_cap;
                                        break;
                                }
                                i++;
                        }
                        j++;
                }
                /*
                 * There is no job that can match the client. So it is not the
                 * child process of shell.
                 */
                goto out;
        }

        if (rpos != wpos) {
                fill_foreground_process_stdin_buffer();
        }
out:
        pthread_mutex_unlock(&job_mutex);
}

/* append to the input buffer (called by the terminal) */
void append_input_buffer(const char buf[], size_t size)
{
        for (size_t i = 0; i < size; i++) {
                if (check_ch(buf[i])) {
                        continue;
                }

                pthread_mutex_lock(&input_buf_mutex);
                input_buf[wpos] = buf[i];
                wpos++;
                if (wpos == INPUT_BUFLEN) {
                        wpos = 0;
                }
                if (wpos == rpos) {
                        rpos++;
                        if (rpos == INPUT_BUFLEN) {
                                rpos = 0;
                        }
                }
                pthread_mutex_unlock(&input_buf_mutex);
        }

        if (pthread_mutex_trylock(&job_mutex) == 0) {
                if (fg_job.pid > 0 && fg_job.notific_cap > 0) {
                        fill_foreground_process_stdin_buffer();
                        usys_notify(fg_job.notific_cap);
                }
                pthread_mutex_unlock(&job_mutex);
        }
}

/* Store foreground processes and their input buffers' addresses in stacks. */
static struct job fg_proc_stack[FG_PROC_STACK_SIZE];
static void *fg_pmo_addr_stack[FG_PROC_STACK_SIZE];
static void *fg_buffer_addr[FG_PROC_STACK_SIZE];
static int fg_proc_stack_top;

/* Push current foreground process into the stack. */
static int push_fg_proc_stack(void)
{
        if (fg_proc_stack_top >= FG_PROC_STACK_SIZE) {
                return -1;
        }
        fg_proc_stack[fg_proc_stack_top] = fg_job;
        fg_pmo_addr_stack[fg_proc_stack_top] = foreground_pmo_addr;
        fg_buffer_addr[fg_proc_stack_top] = foreground_buffer_addr;
        ++fg_proc_stack_top;
        return 0;
}

/* Pop current foreground process from the stack and recover pmo/buffer address
 * as well. */
static void pop_fg_proc_stack(void)
{
        BUG_ON(fg_proc_stack == 0);
        --fg_proc_stack_top;
        fg_job = fg_proc_stack[fg_proc_stack_top];
        foreground_pmo_addr = fg_pmo_addr_stack[fg_proc_stack_top];
        foreground_buffer_addr = fg_buffer_addr[fg_proc_stack_top];
        memset(&fg_proc_stack[fg_proc_stack_top], 0, sizeof(struct job));
        fg_pmo_addr_stack[fg_proc_stack_top] = NULL;
        fg_buffer_addr[fg_proc_stack_top] = NULL;
}

/*
 * When a process want to read character from uart buffer, it must send it's
 * necessary information to shell such as notification cap and buffer pmo cap.
 * So when an interrupt comes in, shell can fill character to foreground
 * process's buffer and awake it.
 *
 * The terminal sends keyboard input to the shell.
 */
DEFINE_SERVER_HANDLER(shell_dispatch)
{
        struct shell_req *req = (struct shell_req *)ipc_get_msg_data(ipc_msg);

        switch (req->req) {
        case SHELL_SET_PROCESS_INFO:
                if (ipc_get_msg_send_cap_num(ipc_msg) != 2) {
                        printf("Request para error!\n");
                        break;
                }
                set_process_info(ipc_get_msg_cap(ipc_msg, 0),
                                 ipc_get_msg_cap(ipc_msg, 1),
                                 req->pid);
                break;
        case SHELL_APPEND_INPUT_BUFFER:
                append_input_buffer(req->buf, req->size);
                break;
        case SHELL_SET_FG_PROCESS:
                if (push_fg_proc_stack() == 0) {
                        fg_job.pid = req->pid;
                        printf("Pushed %d, Current fg pid: %d\n",
                               fg_job.pid,
                               req->pid);
                } else {
                        printf("Too much fg process!\n");
                }
                break;
        case SHELL_FG_PROCESS_RETURN:
                if (fg_job.pid == req->pid) {
                        pop_fg_proc_stack();
                        printf("Popped %d, returned from pid: %d\n",
                               fg_job.pid,
                               req->pid);
                } else {
                        printf("Unmatched current fg process pid!\n");
                }
                break;
        default:
                printf("wrong request\n");
                break;
        }
        ipc_return(ipc_msg, 0);
}

void *shell_server(void *arg)
{
        int ret = ipc_register_server(shell_dispatch, register_cb_single);
        BUG_ON(ret < 0);

        /*
         * TODO: we should suspend the thread that register a ipc server. We
         * should use a more elegant way to do that.
         */
        usys_wait(usys_create_notifc(), true, NULL);
        return NULL;
}

/*
 * FIXME: The child processes of shell need know the cap of shell server. At the
 * begining, I want to send the cap by chcore_new_process, and add the cap to
 * child processes when call the launch_process_with_pmo_caps function. But it
 * will trigger a bug before the process start. So I maintain a field in
 * proc_node struct to record the cap of shell that the process belongs to.
 * Maybe we can send the cap by launch_process_with_pmo_caps directly in the
 * future.
 */
void send_cap_to_procmgr(cap_t cap)
{
        ipc_msg_t *msg;
        struct proc_request *proc_req;

        msg = ipc_create_msg_with_cap(
                procmgr_ipc_struct, sizeof(struct proc_request), 1);
        proc_req = (struct proc_request *)ipc_get_msg_data(msg);

        ipc_set_msg_cap(msg, 0, cap);
        proc_req->req = PROC_REQ_SET_SHELL_CAP;
        ipc_call(procmgr_ipc_struct, msg);

        ipc_destroy_msg(msg);
}
