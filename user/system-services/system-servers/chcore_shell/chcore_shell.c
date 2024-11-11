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

/*
 * Chcore Shell v1.0
 *
 * The shell is divided into three part: chcore_shell.c, buildin_cmd.c and
 * job_control.c.
 *
 * This file implements the basic function of shell including receiving
 * user's input, handling special character such as CTRLZ and handling the
 * reading request from child process. When ccsh start, it will create two
 * threads to handle uart interrupt and child process IPC request. The main
 * thread will loop to parse and run the user's command. If there exists a
 * foreground thread, it will call waitpid to wait the thread to exit or
 * suspend.
 *
 * The buildin_cmd.c implement the buildin command of this shell.
 *
 * The job_control.c implement the foreground and background job control of the
 * shell.
 *
 * Maybe someone will extend the function of this shell in the future. Please
 * add buildin command in buildin_cmd.c and add other extra function in a
 * dependent file.
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
#include <chcore/pthread.h>

#include "job_control.h"
#include "buildin_cmd.h"
#include "handle_input.h"

#define ESC 0x1B

static char buf[BUFLEN];

/*
 * The procmgr_ipc_struct is used by main thread to call waitpid, while the
 * procmgr_ipc_struct_internal is used by other threads to communicate with
 * procmgr to create new process.
 */

static cap_t shell_ipc_server_cap = -1;

/* May be used in the future to control the behaviour of the shell. */
static void enable_raw_mode(void)
{
        struct termios raw;
        tcgetattr(STDIN_FILENO, &raw);

        /* input modes: no break, no CR to NL, no parity check, no strip char,
         * no start/stop output control. */
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        /* output modes - disable post processing */
        raw.c_oflag &= ~(OPOST);
        /* control modes - set 8 bit chars */
        raw.c_cflag |= (CS8);
        /* local modes - echoing off, canonical off, no extended functions,
         * no signal chars (^Z,^C) */
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        /* control chars - set return condition: min number of bytes and timer.
         * We want read to return every single byte, without timeout. */
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

        /* put terminal in raw mode after flushing */
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void back_space(int n)
{
        int i;

        for (i = 0; i < n; i++) {
                putchar('\b');
                putchar(' ');
                putchar('\b');
        }

        fflush(stdout);
}

void clear_line(void)
{
        putchar(ESC);
        putchar('[');
        putchar('2');
        putchar('K');
        putchar('\r');

        fflush(stdout);
}

void show_prompt(const char *prompt)
{
        if (prompt != NULL) {
                printf("%s", prompt);
                fflush(stdout);
        }
}

char *readline(const char *prompt)
{
        int i = 0, j = 0;
        size_t length = 0;
        int index = 0;
        signed char c = 0;
        char ch = 0;
        int ret = 0;
        char complement[BUFLEN];
        char temp[BUFLEN];
        int complement_time = 0;

        while (1) {
        c = shell_getchar();
        handle_new_char:
                if (c < 0) {
                        /* Chinese input may lead to error */
                        continue;
                } else if (c == '\b' || c == '\x7f') {
                        if (index == 0)
                                continue;
                        /*
                         * NOTE: Handling end-of-line deletions separately for
                         * reducing reoutput.
                         */
                        /*
                         * Copy the characters after the deleted character to
                         * the position of the deleted character.
                         */
                        strlcpy(buf + index - 1,
                                buf + index,
                                BUFLEN - index + 1);
                        buf[i] = 0;

                        back_space(1);
                        index--;
                        i--;
                        if (index == i) {
                                continue;
                        }
                        /* Redisplay the characters after the deleted character.
                         */
                        for (j = index; j <= i; j++) {
                                putchar(buf[j]);
                        }
                        putchar(' ');
                        /* Move the cursor to the appropriate position */
                        for (j = i; j >= index; j--) {
                                putchar('\b');
                        }
                        fflush(stdout);
                } else if (c >= ' ' && i < BUFLEN - 1) {
                        /*
                         * NOTE: Handling end-of-line additions separately for
                         * reducing reoutput.
                         */
                        if (i == index) {
                                buf[index++] = c;
                                i++;
                                buf[index] = 0;
                                putchar(buf[index - 1]);
                                fflush(stdout);
                                continue;
                        }
                        /* Leave space for newly entered characters */
                        strlcpy(temp, buf + index, BUFLEN);
                        strlcpy(buf + index + 1, temp, BUFLEN - index - 1);
                        buf[index++] = c;
                        i++;
                        buf[i] = 0;

                        /* Redisplay the characters after the newly entered
                         * character. */
                        for (j = index - 1; j < i; j++) {
                                putchar(buf[j]);
                        }

                        /* Move the cursor to the appropriate position */
                        for (j = i; j > index; j--) {
                                putchar('\b');
                        }

                        fflush(stdout);
                } else if (c == '\n' || c == '\r') {
                        putchar('\n');
                        fflush(stdout);
                        buf[i] = 0;
                        break;
                } else if (c == ESC) {
                        /* Handle direction key. */
                        ch = shell_getchar();
                        if (ch == '[') {
                                ch = shell_getchar();
                                switch (ch) {
                                case 'A':
                                        /* ↑ */
                                        if (do_up()) {
                                                clear_line();
                                                show_prompt(prompt);
                                                length = strlen(
                                                        history_cmd_pointer
                                                                ->cmd);
                                                for (j = 0; j < length; j++) {
                                                        buf[j] =
                                                                history_cmd_pointer
                                                                        ->cmd[j];
                                                        putchar(buf[j]);
                                                }
                                                buf[j] = 0;
                                                i = (int)strlen(
                                                        history_cmd_pointer
                                                                ->cmd);
                                                index = i;
                                        }
                                        break;
                                case 'B':
                                        /* ↓ */
                                        if (do_down()) {
                                                clear_line();
                                                show_prompt(prompt);
                                                length = strlen(
                                                        history_cmd_pointer
                                                                ->cmd);
                                                for (j = 0; j < length; j++) {
                                                        buf[j] =
                                                                history_cmd_pointer
                                                                        ->cmd[j];
                                                        putchar(buf[j]);
                                                }
                                                buf[j] = 0;
                                                i = (int)strlen(
                                                        history_cmd_pointer
                                                                ->cmd);
                                                index = i;
                                        }
                                        break;
                                case 'C':
                                        /* → */
                                        if (index < i) {
                                                putchar(buf[index]);
                                                index++;
                                        }
                                        break;
                                case 'D':
                                        /* ← */
                                        if (index > 0) {
                                                putchar('\b');
                                                index--;
                                        }
                                        break;
                                }
                        }
                        fflush(stdout);
                        continue;
                } else if (c == '\t') { /* auto complement */
                        /* Complement init */
                        complement_time = 0;
                        buf[i] = 0;
                        /* In case not find anything */
                        strlcpy(complement, buf, BUFLEN);
                        do {
                                /* Handle tab here */
                                ret = do_complement(
                                        buf, complement, complement_time);
                                /* No new findings */
                                if (ret == -1) {
                                        /* Not finding anything */
                                        if (complement_time == 0) {
                                                c = shell_getchar();
                                                goto handle_new_char;
                                        }
                                        complement_time = 0;
                                        continue;
                                }
                                /* Return to the head of the line */
                                for (j = 0; j < i; j++)
                                        printf("\b \b");
                                printf("%s", complement);
                                /* flush to show */
                                fflush(stdout);
                                /* Update i */
                                i = (int)strlen(complement);
                                index = i;
                                /* Find a different next time */
                                complement_time++;
                                /* Get next char */
                                c = shell_getchar();
                        } while (c == '\t');
                        if (c == '\n' || c == '\r') { /* End of input */
                                strlcpy(buf, complement, BUFLEN);
                                break;
                        } else {
                                strlcpy(buf, complement, BUFLEN);
                                /* Get new char, has to handle */
                                goto handle_new_char;
                        }
                }
        }
        return buf;
}

int builtin_cmd(char *cmdline)
{
        int ret, i;
        char cmd[BUFLEN];
        for (i = 0; cmdline[i] != ' ' && cmdline[i] != '\0'; i++)
                cmd[i] = cmdline[i];
        cmd[i] = '\0';
        if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
                usys_poweroff();
                return 1;
        }
        if (!strcmp(cmd, "cd")) {
                ret = do_cd(cmdline);
                return !ret ? 1 : -1;
        }
        if (!strcmp(cmd, "ls")) {
                ret = do_ls(cmdline);
                return !ret ? 1 : -1;
        }
        if (!strcmp(cmd, "clear")) {
                do_clear();
                return 1;
        }
        if (!strcmp(cmd, "top")) {
                ret = do_top();
                return !ret ? 1 : -1;
        }
        if (!strcmp(cmd, "jobs")) {
                do_jobs();
                return 1;
        }
        if (!strcmp(cmd, "fg")) {
                ret = do_fg(cmdline);
                return ret;
        }
        if (!strcmp(cmd, "history")) {
                do_history();
                return 1;
        }
        if (!strcmp(cmd, "mem")) {
                usys_get_mem_usage_msg();
                return 1;
        }
        if (!strcmp(cmd, "mm")) {
                struct free_mem_info free_mem_info;
                ret = usys_get_free_mem_size(&free_mem_info);

                if (ret < 0) {
                        printf("Error when getting free mem info: %d\n", ret);
                } else {
                        printf("free_mem: %lx, total_mem: %lx\n", free_mem_info.free_mem_size, 
                        free_mem_info.total_mem_size);
                }
                
                return 1;
        }
        return 0;
}

static inline bool is_whitespace(char ch)
{
        return ch == ' ' || ch == '\t';
}

static int parse_args(char *cmdline, char *pathbuf, char *argv[])
{
        int nr_args = 0;

        pathbuf[0] = '\0';
        while (*cmdline == ' ')
                cmdline++;

        if (*cmdline == '\0')
                return -ENOENT;

        if (*cmdline != '/')
                strlcpy(pathbuf, "/", BUFLEN);

        /* Copy to pathbuf. */
        strlcat(pathbuf, cmdline, BUFLEN);

        while (*pathbuf) {
                /* Skip all spaces. */
                while (*pathbuf && is_whitespace(*pathbuf))
                        pathbuf++;

                if (!*pathbuf)
                        /* End of string. */
                        break;

                /* remove ' from argv. */
                if (*pathbuf == '\'') {
                        pathbuf++;
                        while (*pathbuf && is_whitespace(*pathbuf))
                                pathbuf++;

                        if (!*pathbuf) {
                                printf("Error: must have one right ' after left ' !\n");
                                return -EINVAL;
                        }

                        argv[nr_args++] = pathbuf;
                        while (*pathbuf && *pathbuf != '\'')
                                pathbuf++;

                        *pathbuf = '\0';
                        pathbuf++;
                        if (*pathbuf && !is_whitespace(*pathbuf)) {
                                printf("Error: must have space between the right ' and the new arg!\n");
                                return -EINVAL;
                        }
                } else {
                        argv[nr_args++] = pathbuf;

                        while (*pathbuf && !is_whitespace(*pathbuf))
                                pathbuf++;

                        if (*pathbuf) {
                                assert(is_whitespace(*pathbuf));
                                *pathbuf = '\0';
                                pathbuf++;
                        }
                }
        }

        return nr_args;
}

int run_cmd(char *cmdline)
{
        char pathbuf[BUFLEN];
        char *argv[NR_ARGS_MAX];
        int argc, pos = 0;
        pid_t pid = 0;
        bool run_background = false;

        argc = parse_args(cmdline, pathbuf, argv);
        if (argc < 0) {
                printf("[Shell] Parsing arguments error\n");
                return argc;
        }
        BUG_ON(shell_ipc_server_cap == -1);

        if (strcmp(argv[argc - 1], "&") == 0) {
                argc--;
                run_background = true;
        }

        pthread_mutex_lock(&job_mutex);
        // pid = chcore_new_process_with_cap(
        // 	argc, argv, false, 1, &shell_ipc_server_cap);
        pid = chcore_new_process(argc, argv);

        if (pid > 0) {
                clean_jobs();
                if (run_background) {
                        pos = add_job(argv[0], pid, -1, -1);
                        if (pos >= 0) {
                                waitchild = false;
                        }
                }
                if (!run_background || pos < 0) {
                        // printf("foreground thread pid:%d\n", pid);
                        fg_job.pid = pid;
                        strlcpy(fg_job.job_name,
                                argv[0],
                                sizeof(fg_job.job_name));
                        waitchild = true;
                }
        }
        pthread_mutex_unlock(&job_mutex);
        // flush_buffer();

        return pid;
}

void printf_welcome(void)
{
        printf("\n");
        /* Wait (10ms) the console log to output completely. */
        usleep(10000);
        printf(" ______     __  __     ______     __  __     ______     __         __        \n");
        printf("/\\  ___\\   /\\ \\_\\ \\   /\\  ___\\   /\\ \\_\\ \\   /\\  ___\\   /\\ \\       /\\ \\       \n");
        printf("\\ \\ \\____  \\ \\  __ \\  \\ \\___  \\  \\ \\  __ \\  \\ \\  __\\   \\ \\ \\____  \\ \\ \\____  \n");
        printf(" \\ \\_____\\  \\ \\_\\ \\_\\  \\/\\_____\\  \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_____\\  \\ \\_____\\ \n");
        printf("  \\/_____/   \\/_/\\/_/   \\/_____/   \\/_/\\/_/   \\/_____/   \\/_____/   \\/_____/ \n");
        printf("\n");
        printf("\nWelcome to ChCore shell!");
        fflush(stdout);
}

int main(void)
{
        char *buf;
        int ret = 0;
        pthread_t shell_pid;
        pthread_t receive_char_thread_pid;

        fsm_scan_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        fail_cond(fsm_scan_pmo_cap < 0,
                  "usys create_ret ret %d\n",
                  fsm_scan_pmo_cap);

        fsm_scan_buf = chcore_auto_map_pmo(
                fsm_scan_pmo_cap, PAGE_SIZE, VM_READ | VM_WRITE);
        fail_cond(!fsm_scan_buf, "chcore_auto_map_pmo failed\n");

#ifdef UART_IRQ
        pthread_create(&receive_char_thread_pid, NULL, handle_uart_irq, NULL);
#else /* CHCORE_PLAT_RASPI3 */
        pthread_create(
                &receive_char_thread_pid, NULL, other_plat_get_char, NULL);
#endif /* CHCORE_PLAT_RASPI3 */
        shell_ipc_server_cap =
                chcore_pthread_create(&shell_pid, NULL, shell_server, NULL);
        pthread_mutex_init(&job_mutex, NULL);

        init_buildin_cmd();

        send_cap_to_procmgr(shell_ipc_server_cap);

        enable_raw_mode();
        /* After booting system servers, run the ChCore shell. */
        // printf("\nWelcome to ChCore shell!");
        printf_welcome();

        while (1) {
                /* The CI output formatting script wipes off "\n$ " pattern. To
                 * reduce the probability that "\n$ " is split by other output,
                 * we print it with write instead of printf and fflush. If
                 * printf and fflush is used, "\n$ " will be printed with two
                 * flushes, one for "\n" and one for "$ ". */
                write(STDOUT_FILENO, "\n$ ", 3);

                buf = readline("$ ");
                clear_history_point();
                if (buf == NULL)
                        usys_exit(0);
                if (strlen(buf) == 0)
                        continue;
                add_cmd_to_history(buf);
                ret = builtin_cmd(buf);
                if (ret != 0)
                        goto handle_wait;
                ret = run_cmd(buf);
                if (ret < 0) {
                        printf("Cannot run %s, ERROR %d\n", buf, ret);
                }
        handle_wait:
                memset(buf, 0, strlen(buf));
                if (waitchild) {
                        /* FIXME: only wait for child process when `waitchild`
                           is on, I think this should only be a temporary
                           solution, we need full-featured job control in the
                           future. */
                        pid_t pid = ret;
                        int status;
                        /* wait for the foreground process */
                        chcore_waitpid(pid, &status, 0, 0);

                        /* Flush the information of foreground process. */
                        pthread_mutex_lock(&job_mutex);
                        fg_job.pid = -1;
                        fg_job.notific_cap = -1;
                        if (fg_job.pmo_cap > 0) {
                                chcore_auto_unmap_pmo(
                                        fg_job.pmo_cap,
                                        (unsigned long)foreground_pmo_addr,
                                        PAGE_SIZE);
                        }
                        fg_job.pmo_cap = -1;
                        foreground_buffer_addr = NULL;
                        waitchild = false;
                        pthread_mutex_unlock(&job_mutex);
                }
        }
}
