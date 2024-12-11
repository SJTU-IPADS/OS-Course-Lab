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

/* File operation towards STDIN, STDOUT and STDERR */

#include "fd.h"
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <syscall_arch.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <chcore/memory.h>
#include <termios.h>
#include <chcore-internal/shell_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <unistd.h>
#include <fcntl.h>
#include <raw_syscall.h>
#include "stdio.h"

/*
 * We create a pmo to store the user's input. The size of pmo is 4096 byte but
 * the size of input buffer is 4088. Since we use the first two four-byte for
 * store the position of write/read pointer.
 */
#define BUFLEN (PAGE_SIZE - sizeof(int) * 2)

static cap_t input_notific_cap = -1;
static cap_t stdin_buff_cap = -1;
static void *stdin_buffer = NULL;
static void *stdin_pmo_addr = NULL;
static int *rpos, *wpos;
extern cap_t procmgr_server_cap;
static cap_t shell_server_cap = -1;

static ipc_struct_t *shell_msg_struct = NULL;
static int pid = -1;

static void get_buf_rpos_wpos(void)
{
        ipc_msg_t *shell_msg;
        ipc_msg_t *procmgr_msg;
        struct proc_request *proc_req;
        struct shell_req *req;

        if (input_notific_cap == -1) {
                input_notific_cap = usys_create_notifc();
                BUG_ON(input_notific_cap < 0);
        }

        if (stdin_buff_cap == -1) {
                stdin_buff_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
                BUG_ON(stdin_buff_cap < 0);
                stdin_pmo_addr = (void *)chcore_auto_map_pmo(
                        stdin_buff_cap, PAGE_SIZE, PROT_READ | PROT_WRITE);
                BUG_ON(!stdin_pmo_addr);
                
                stdin_buffer = (char *)stdin_pmo_addr + sizeof(int) * 2;
                /*
                 * We use the first 8 bytes of stdin_buffer to standard for the
                 * write/read pointer.
                 */
                rpos = (int *)stdin_pmo_addr;
                wpos = rpos + 1;
                *rpos = 0;
                *wpos = 0;
        }

        if (pid == -1) {
                pid = getpid();
        }

        if (shell_msg_struct == NULL) {
                procmgr_msg = ipc_create_msg(
                        procmgr_ipc_struct, sizeof(struct proc_request));
                proc_req = (struct proc_request *)ipc_get_msg_data(procmgr_msg);

                proc_req->req = PROC_REQ_GET_SHELL_CAP;
                ipc_call(procmgr_ipc_struct, procmgr_msg);
                BUG_ON(ipc_get_msg_return_cap_num(procmgr_msg) == 0);

                shell_server_cap = ipc_get_msg_cap(procmgr_msg, 0);
                BUG_ON(shell_server_cap < 0);
                ipc_destroy_msg(procmgr_msg);

                shell_msg_struct = ipc_register_client(shell_server_cap);
                BUG_ON(shell_msg_struct == NULL);

                shell_msg = ipc_create_msg_with_cap(
                        shell_msg_struct, sizeof(struct shell_req), 2);
                req = (struct shell_req *)ipc_get_msg_data(shell_msg);
                req->pid = pid;
                req->req = SHELL_SET_PROCESS_INFO;
                ipc_set_msg_cap(shell_msg, 0, input_notific_cap);
                ipc_set_msg_cap(shell_msg, 1, stdin_buff_cap);
                // printf("send msg\n");
                ipc_call(shell_msg_struct, shell_msg);
                ipc_destroy_msg(shell_msg);
        }
}

static int get_one_char(void)
{
        int ch;

        get_buf_rpos_wpos();

        while (*rpos == *wpos) {
                usys_wait(input_notific_cap, 1, NULL);
        }

        ch = ((char *)stdin_buffer)[*rpos];
        *rpos = (*rpos) + 1;
        if (*rpos == BUFLEN) {
                *rpos = 0;
        }

        return ch;
}

static void put(char buffer[], unsigned size)
{
        /* LAB 3 TODO BEGIN */

        /* LAB 3 TODO END */
}

#define MAX_LINE_SIZE 4095

int chcore_stdio_fcntl(int fd, int cmd, int arg)
{
        int new_fd;

        switch (cmd) {
        case F_DUPFD_CLOEXEC:
        case F_DUPFD: {
                new_fd = dup_fd_content(fd, arg);
                return new_fd;
        }
        default:
                return -EINVAL;
        }
        return -1;
}

/* STDIN */
static ssize_t chcore_stdio_read(int fd, void *buf, size_t count)
{
        struct fd_desc *fdesc = fd_dic[fd];
        assert(fdesc);

        struct termios *ts = &fdesc->termios;
        char *char_buf = (char *)buf;
        ssize_t n = 0;
        int ch;
        bool canon_mode = ts->c_lflag & ICANON;

        /*
         * Read from stdin according to termios.
         * For simplicity, we don't support every flag.
         */
        while (n < MIN(count, MAX_LINE_SIZE)) {
                ch = get_one_char();
                if (ch == '\r') {
                        if (ts->c_iflag & IGNCR) {
                                /* ignore CR */
                                continue;
                        } else if (ts->c_iflag & ICRNL) {
                                /* tranlate CR to NL */
                                ch = '\n';
                        }
                } else if (ch == '\n' && (ts->c_iflag & INLCR)) {
                        ch = '\r';
                } else if (ch == '\x7f' /* DELETE */) {
                        /* FIXME: currently we see DELETE as BACKSPACE */
                        ch = '\b';
                }

                if (ch == '\b' && canon_mode) {
                        n--; 
                        continue;
                }

                /* add ch to buffer */
                char_buf[n++] = ch;

                if (!canon_mode && n >= ts->c_cc[VMIN]) {
                        /* VMIN == 0 and VTIME are ignored */
                        break;
                } else if (canon_mode && ch == '\n') {
                        /* canonical mode + new line */
                        break;
                }
        }

        char_buf[n] = '\0';
        return n;
}

static ssize_t chcore_stdin_write(int fd, void *buf, size_t count)
{
        return -EINVAL;
}

static int chcore_stdin_close(int fd)
{
        free_fd(fd);
        return 0;
}

static int chcore_stdio_ioctl(int fd, unsigned long request, void *arg)
{
        /* A fake window size */
        if (request == TIOCGWINSZ) {
                struct winsize *ws;

                ws = (struct winsize *)arg;
                ws->ws_row = 30;
                ws->ws_col = 80;
                ws->ws_xpixel = 0;
                ws->ws_ypixel = 0;
                return 0;
        }
        struct fd_desc *fdesc = fd_dic[fd];
        assert(fdesc);
        switch (request) {
        case TCGETS: {
                struct termios *t = (struct termios *)arg;
                *t = fdesc->termios;
                return 0;
        }
        case TCSETS:
        case TCSETSW:
        case TCSETSF: {
                struct termios *t = (struct termios *)arg;
                fdesc->termios = *t;
                return 0;
        }
        default:
                warn("Unsupported ioctl fd=%d, cmd=0x%lx, arg=0x%lx\n",
                fd,
                request,
                arg);
                break;
        }

        return 0;
}

static int chcore_stdio_poll(int fd, struct pollarg *arg)
{
        if ((fd_dic[fd]->type == FD_TYPE_STDIN
             || fd_dic[fd]->type == FD_TYPE_STDOUT)) {
                get_buf_rpos_wpos();
                if (*rpos != *wpos)
                        return POLLIN;
        }
        return 0;
}

struct fd_ops stdin_ops = {
        .read = chcore_stdio_read,
        .write = chcore_stdin_write,
        .close = chcore_stdin_close,
        .poll = chcore_stdio_poll,
        .ioctl = chcore_stdio_ioctl,
        .fcntl = chcore_stdio_fcntl,
};

/* STDOUT */
#define STDOUT_BUFSIZE 1024

static ssize_t chcore_stdout_write(int fd, void *buf, size_t count)
{
        /* TODO: stdout should also follow termios flags */
        char buffer[STDOUT_BUFSIZE];
        size_t size = 0;

        for (char *p = buf; p < (char *)buf + count; p++) {
                if (size + 2 > STDOUT_BUFSIZE) {
                        put(buffer, size);
                        size = 0;
                }

                if (*p == '\n') {
                        buffer[size++] = '\r';
                }
                buffer[size++] = *p;
        }

        if (size > 0) {
                put(buffer, size);
        }

        return count;
}

static int chcore_stdout_close(int fd)
{
        free_fd(fd);
        return 0;
}

struct fd_ops stdout_ops = {
        .read = chcore_stdio_read,
        .write = chcore_stdout_write,
        .close = chcore_stdout_close,
        .poll = chcore_stdio_poll,
        .ioctl = chcore_stdio_ioctl,
        .fcntl = chcore_stdio_fcntl,
};

/* STDERR */
static ssize_t chcore_stderr_read(int fd, void *buf, size_t count)
{
        return -EINVAL;
}

static ssize_t chcore_stderr_write(int fd, void *buf, size_t count)
{
        return chcore_stdout_write(fd, buf, count);
}

static int chcore_stderr_close(int fd)
{
        free_fd(fd);
        return 0;
}

struct fd_ops stderr_ops = {
        .read = chcore_stderr_read,
        .write = chcore_stderr_write,
        .close = chcore_stderr_close,
        .poll = NULL,
        .ioctl = chcore_stdio_ioctl,
        .fcntl = chcore_stdio_fcntl,
};
