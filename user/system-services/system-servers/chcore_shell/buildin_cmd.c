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
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <errno.h>

#include <chcore/syscall.h>
#include <chcore/memory.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/fs_defs.h>
#include <chcore/string.h>

#include "buildin_cmd.h"
#include "job_control.h"

cap_t fsm_scan_pmo_cap = -1;
void *fsm_scan_buf = NULL;

bool waitchild = false;
struct list_head history_cmd_head;

struct history_cmd_node *history_cmd_pointer = NULL;

static int history_cmd_count = 0;

void init_buildin_cmd(void)
{
        init_list_head(&history_cmd_head);
}

/* Retrieve the entry name from one dirent */
static void get_dent_name(struct dirent *p, char name[])
{
        unsigned long len;
        len = p->d_reclen - sizeof(p->d_ino) - sizeof(p->d_off)
              - sizeof(p->d_reclen) - sizeof(p->d_type);
        memcpy(name, p->d_name, len);
        name[len - 1] = '\0';
}

int do_complement(char *buf, char *complement, int complement_time)
{
        int ret = 0, j = 0;
        struct dirent *p;
        char name[BUFLEN];
        char scan_buf[BUFLEN];
        int r = -1;
        int offset;

        /* XXX: only support '/' here */
        int root_fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        do {
                ret = getdents(root_fd, (struct dirent *)scan_buf, BUFLEN);
                for (offset = 0; offset < ret; offset += p->d_reclen) {
                        p = (struct dirent *)(scan_buf + offset);
                        get_dent_name(p, name);
                        /* Compare the file name with the buf here */
                        if (strstr(name, buf) != NULL) {
                                /* multiple tab want to find different
                                 * alternatives */
                                if (j < complement_time) {
                                        j++;
                                } else {
                                        strlcpy(complement, name, BUFLEN);
                                        r = 0;
                                        break;
                                }
                        }
                }
        } while (ret);
        close(root_fd);

        return r;
}

int do_cd(char *cmdline)
{
        cmdline += 2;
        while (*cmdline == ' ')
                cmdline++;
        if (*cmdline == '\0')
                return 0;
        if (*cmdline != '/') {
        }
        printf("Build-in command cd %s: Not implemented!\n", cmdline);
        return 0;
}

int do_top(void)
{
        usys_top();
        return 0;
}

int do_ls(char *cmdline)
{
        struct dirent *p;
        char pathbuf[BUFLEN];
        char scan_buf[BUFLEN];
        char name[BUFLEN];
        int offset;
        int readbytes;

        pathbuf[0] = '\0';
        cmdline += 2;
        while (*cmdline == ' ')
                cmdline++;

        if (*cmdline == '\0') {
                /* Command is only `ls` */
                getcwd(pathbuf, BUFLEN);
        } else {
                /* Command is `ls DIR` */
                strlcat(pathbuf, cmdline, BUFLEN);
        }

        /* Try open `pathbuf` as dir */
        int dirfd = open(pathbuf, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dirfd < 0) {
                /* Error Handling */
                if (errno == ENOENT) {
                        printf("`%s` is not a directory\n", pathbuf);
                }
                else if (errno == EACCES) {
                        printf("No Permission!\n");
                }
                else {
                        printf("%d\n", errno);
                        printf("do_ls: strange error\n");
                }
                return dirfd;
        }
        do {
                readbytes = getdents(dirfd, (struct dirent *)scan_buf, BUFLEN);
                for (offset = 0; offset < readbytes; offset += p->d_reclen) {
                        p = (struct dirent *)(scan_buf + offset);
                        get_dent_name(p, name);
                        printf("%s\n", name);
                }
        } while (readbytes);
        close(dirfd);

        return 0;
}

void do_clear(void)
{
        char clear_buf[5];
        clear_buf[0] = 12;
        clear_buf[1] = 27;
        clear_buf[2] = '[';
        clear_buf[3] = '3';
        clear_buf[4] = 'J';
        usys_putstr((vaddr_t)clear_buf, 5);
}

/* Show the all jobs in the bg_jobs array. */
void do_jobs(void)
{
        long count = 0;
        int i;
        int total_job;

        pthread_mutex_lock(&job_mutex);
        total_job = job_count;
        /*
         * Iterate through the bg_jobs array to update the state of child
         * processes. If a child process has exited already, we need del it from
         * the bg_jobs array.
         */
        for (i = 0; i < total_job && count < JOBS_MAX;) {
                if (bg_jobs[count] != NULL) {
                        if (!check_job_state(bg_jobs[count]->pid)) {
                                printf("[%ld] + %d %s\n",
                                       count + 1,
                                       bg_jobs[count]->pid,
                                       bg_jobs[count]->job_name);
                        } else {
                                del_job(count);
                        }
                        i++;
                }
                count++;
        }
        pthread_mutex_unlock(&job_mutex);
}

int do_fg(char *cmdline)
{
        long int index = 0;
        char input_string[NR_ARGS_MAX] = {0};
        char *stop_string;
        int i = 0;
        int pid;
        int ret = -1;

        cmdline += strlen("fg ");

        while (cmdline[i] != '\0' && cmdline[i] != ' ') {
                input_string[i] = cmdline[i];
                i++;
                /* The string length exceeds the buffer length. */
                if (i == NR_ARGS_MAX - 1) {
                        goto out;
                }
        }

        /*
         * NOTE: If the string contains non-numeric elements, the stop_string
         * will not equal '\0'.
         */
        index = strtol(input_string, &stop_string, 10) - 1;
        if (*stop_string != '\0') {
                goto out;
        }
        if (index < 0 || index > JOBS_MAX - 1) {
                goto out;
        }

        pthread_mutex_lock(&job_mutex);
        if (bg_jobs[index] == NULL) {
                goto unlock;
        }

        if (!check_job_state(bg_jobs[index]->pid)) {
                waitchild = true;
                printf("[%ld] - %d continued %s\n",
                       index + 1,
                       bg_jobs[index]->pid,
                       bg_jobs[index]->job_name);
                pid = bg_jobs[index]->pid;
                if (bg_jobs[index]->pmo_cap > 0) {
                        foreground_pmo_addr =
                                chcore_auto_map_pmo(bg_jobs[index]->pmo_cap,
                                                    PAGE_SIZE,
                                                    PROT_READ | PROT_WRITE);
                }

                fg_job.pmo_cap = bg_jobs[index]->pmo_cap;
                foreground_buffer_addr = foreground_pmo_addr + sizeof(u32) * 2;
                strlcpy(fg_job.job_name,
                        bg_jobs[index]->job_name,
                        sizeof(fg_job.job_name));
                fg_job.pid = pid;
                fg_job.notific_cap = bg_jobs[index]->notific_cap;
                del_job(index);
                ret = pid;
        }

unlock:
        pthread_mutex_unlock(&job_mutex);

out:
        if (ret == -1) {
                printf("fg: no such job in background\n");
        }
        return ret;
}

/* Free the oldest record. */
static void free_history_record(void)
{
        struct history_cmd_node *temp;

        if (!list_empty(&history_cmd_head)) {
                temp = container_of(history_cmd_head.prev,
                                    struct history_cmd_node,
                                    cmd_node);
                list_del(&temp->cmd_node);
                free(temp->cmd);
                free(temp);
        }
}

void add_cmd_to_history(char *cmd)
{
        struct history_cmd_node *cmd_node;
        int cmd_buffer_size;

        cmd_buffer_size = MIN(BUFLEN, strlen(cmd) + 1);

        if (strlen(cmd) != 0) {
                cmd_node = (struct history_cmd_node *)malloc(
                        sizeof(struct history_cmd_node));
                if (cmd_node == NULL) {
                        return;
                }
                cmd_node->cmd = (char *)malloc(cmd_buffer_size);
                cmd_node->index = history_cmd_count + 1;
                strlcpy(cmd_node->cmd, cmd, cmd_buffer_size);
                list_add(&cmd_node->cmd_node, &history_cmd_head);
                history_cmd_count++;
                if (history_cmd_count > MAX_HISTORY_CMD_RECORD) {
                        free_history_record();
                }
        }
}

/*
 * TODO: We save record in memory now. We can write this record to disk in the
 * future.
 */
void do_history(void)
{
        struct history_cmd_node *cmd;

        for_each_in_list_reverse (
                cmd, struct history_cmd_node, cmd_node, &history_cmd_head) {
                printf("%4d  %s\n", cmd->index, cmd->cmd);
        }
}

bool do_up(void)
{
        if (history_cmd_pointer == NULL) {
                if (list_empty(&history_cmd_head)) {
                        return false;
                }
                history_cmd_pointer = container_of(history_cmd_head.next,
                                                   struct history_cmd_node,
                                                   cmd_node);
        } else if (history_cmd_pointer->cmd_node.next != &history_cmd_head) {
                history_cmd_pointer =
                        container_of(history_cmd_pointer->cmd_node.next,
                                     struct history_cmd_node,
                                     cmd_node);
        } else {
                return false;
        }
        return true;
}

bool do_down(void)
{
        if (history_cmd_pointer == NULL) {
                return false;
        }
        if (history_cmd_pointer->cmd_node.prev != &history_cmd_head) {
                history_cmd_pointer =
                        container_of(history_cmd_pointer->cmd_node.prev,
                                     struct history_cmd_node,
                                     cmd_node);
                return true;
        }
        return false;
}

void clear_history_point(void)
{
        history_cmd_pointer = NULL;
}
