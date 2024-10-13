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

#include <assert.h>
#include <bits/alltypes.h>
#include <bits/syscall.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/auxv.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include <raw_syscall.h>
#include <syscall_arch.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/utsname.h>

/* ChCore Port header */
#include "fd.h"
#include "poll.h"
#include "chcore/aslr.h"
#include <chcore/bug.h>
#include <chcore/defs.h>
#include <chcore-internal/fs_defs.h>
#include <chcore/ipc.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include "futex.h"
#include "eventfd.h"
#include "pipe.h"
#include "timerfd.h"
#include "socket.h"
#include "file.h"
#include "fs_client_defs.h"
#include "chcore_mman.h"
#include <chcore-internal/fs_debug.h>
#include <chcore/memory.h>
#include "chcore_shm.h"
#include "chcore_kill.h"

#define debug(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define warn(fmt, ...)  printf("[WARN] " fmt, ##__VA_ARGS__)
#define warn_once(fmt, ...)               \
        do {                              \
                static int __warned = 0;  \
                if (__warned)             \
                        break;            \
                __warned = 1;             \
                warn(fmt, ##__VA_ARGS__); \
        } while (0)

// #define TRACE_SYSCALL
#ifdef TRACE_SYSCALL
#define syscall_trace(n) \
        printf("[dispatcher] %s: syscall_num %ld\n", __func__, n)
#else
#define syscall_trace(n) \
        do {             \
        } while (0)
#endif

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define _SYSCALL_LL_REV(a, b) \
        ((union {             \
                long long ll; \
                long l[2];    \
        }){.l = {(a), (b)}})  \
                .ll

/*
 * Used to issue ipc to user-space services
 * Register to server before executing the program
 */
cap_t fsm_server_cap = 0;
cap_t lwip_server_cap = 0;

cap_t procmgr_server_cap = 0;

int chcore_pid = 0;

static u64 heap_start = -1;
static int aslr_inited = 0;
static int heap_start_inited = 0;

/*
 * Helper function.
 */
extern int __xstatxx(int req, int fd, const char *path, int flags,
                     void *statbuf, size_t bufsize);
void dead(long n);

typedef struct {
        unsigned long a_type;
        union {
                unsigned long a_val;
                char *a_ptr;
        };
} elf_auxv_t;

void __libc_connect_services(elf_auxv_t *auxv)
{
        size_t i;
        unsigned long nr_caps;

        fsm_ipc_struct->conn_cap = 0;
        fsm_ipc_struct->server_id = FS_MANAGER;
        lwip_ipc_struct->conn_cap = 0;
        lwip_ipc_struct->server_id = NET_MANAGER;
        procmgr_ipc_struct->conn_cap = 0;
        procmgr_ipc_struct->server_id = PROC_MANAGER;

        for (i = 0; auxv[i].a_type != AT_CHCORE_CAP_CNT; i++)
                ;

        nr_caps = auxv[i].a_val;
        if (nr_caps == ENVP_NONE_CAPS) {
                // printf("%s: connect to zero system services.\n", __func__);
                return;
        }

        procmgr_server_cap = (cap_t)(auxv[i + 1].a_val);

        /* Connect to FS only */
        if ((--nr_caps) == 0)
                return;

        fsm_server_cap = (cap_t)(auxv[i + 2].a_val);

        /* Connect to FS and NET */
        if ((--nr_caps) == 0)
                return;

        lwip_server_cap = (cap_t)(auxv[i + 3].a_val);

        /* Connect to FS, NET, and PROCMGR */
        // printf("%s: connect FS, NET and PROCMGR\n", __func__);
}
weak_alias(__libc_connect_services, libc_connect_services);

/* Set the random number generator seed for ASLR use */
static inline void init_chcore_aslr()
{
        if (unlikely(!aslr_inited)) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                srand(ts.tv_nsec);
                aslr_inited = 1;
        }
}

static inline void init_heap_start()
{
        if (unlikely(!heap_start_inited)) {
                heap_start = HEAP_START + ASLR_RAND_OFFSET;
                heap_start_inited = 1;
        }
}

/*
 * This function is local to libc and it will
 * only be executed once during the libc init time.
 *
 * It will be executed in the dynamic loader (for dynamic-apps) or
 * just before calling user main (for static-apps).
 * Nevertheless, when loading a dynamic application, it will be invoked twice.
 * This is why the variable `initialized` is required.
 */
__attribute__((constructor(101))) void __libc_chcore_init(void)
{
        static int initialized = 0;
        int fd0, fd1, fd2;
        struct termios *ts;
        char *pidstr;
        size_t i;
        elf_auxv_t *auxv;

        if (initialized == 1)
                return;
        initialized = 1;

        /* Initialize libfs_client */
        init_fs_client_side();

        /* Open stdin/stdout/stderr */
        /* STDIN */
        fd0 = alloc_fd();
        assert(fd0 == STDIN_FILENO);
        fd_dic[fd0]->type = FD_TYPE_STDIN;
        // BUG: this fd0 is a file descriptor, but fd_dic[fd0]->fd should be an
        // "fid" in fsm.srv
        fd_dic[fd0]->fd = fd0;
        fd_dic[fd0]->fd_op = &stdin_ops;
        /* Set default stdin termios flags */
        ts = &fd_dic[fd0]->termios;
        ts->c_iflag = INLCR | ICRNL;
        ts->c_oflag = OPOST | ONLCR;
        ts->c_cflag = CS8;
        ts->c_lflag = ISIG | ICANON | ECHO | ECHOE | NOFLSH;
        ts->c_cc[VMIN] = 1;
        ts->c_cc[VTIME] = 0;

        /* STDOUT */
        fd1 = alloc_fd();
        assert(fd1 == STDOUT_FILENO);
        fd_dic[fd1]->type = FD_TYPE_STDOUT;
        fd_dic[fd1]->fd = fd1;
        fd_dic[fd1]->fd_op = &stdout_ops;

        /* STDERR */
        fd2 = alloc_fd();
        assert(fd2 == STDERR_FILENO);
        fd_dic[fd2]->type = FD_TYPE_STDERR;
        fd_dic[fd2]->fd = fd2;
        fd_dic[fd2]->fd_op = &stderr_ops;

        /* ProcMgr passes PID through env variable */
        pidstr = getenv("PID");
        if (pidstr)
                chcore_pid = atoi(pidstr);

        for (i = 0; __environ[i]; i++)
                ;

        auxv = (elf_auxv_t *)(__environ + i + 1);

        /* Set seed for ASLR */
        init_chcore_aslr();
        init_heap_start();

        libc_connect_services(auxv);
}

int chcore_dup2(int a, int b)
{
        int ret;
        ret = 0;
        if (a == b)
                return a;
        if (b >= 0 && b < MAX_FD) {
                if (fd_dic[b]) {
                        ret = close(b);
                        if (ret != 0) {
                                return -1;
                        }
                }
        } else
                return -1;
        return chcore_fcntl(a, F_DUPFD, b);
}

/*
 * LibC may invoke syscall directly through assembly code like:
 * src/thread/x86_64/__set_thread_area.s
 *
 * So, we also need to rewrite those function.
 */

/*
 * TODO (tmac): some SYS_num are specific to architectures.
 * We can ag "x86_64"/"aarch64" in src directory to see that.
 *
 * Maybe we can use another file for hooking those specific SYS_num.
 */

long __syscall0(long n)
{
        if (n != SYS_sched_yield)
                syscall_trace(n);

        switch (n) {
        case SYS_getppid:
                warn_once("faked ppid\n");
                return chcore_pid;
        case SYS_getpid:
                return chcore_pid;
        case SYS_geteuid:
                warn_once("faked euid\n");
                return 0; /* root. */
        case SYS_getuid:
                warn_once("faked uid\n");
                return 0; /* root */
        case SYS_getgid:
                warn_once("faked gid\n");
                return 0; /* root */
        case SYS_sched_yield:
                chcore_syscall0(CHCORE_SYS_yield);
                return 0;
        case SYS_getegid:
                warn_once("fake egid\n");
                return 0;
        case SYS_sync:
                return chcore_sync();
        default:
                dead(n);
                return chcore_syscall0(n);
        }
}

long __syscall1(long n, long a)
{
        syscall_trace(n);

        switch (n) {
#ifdef SYS_unlink
        case SYS_unlink:
                return __syscall3(SYS_unlinkat, AT_FDCWD, a, 0);
#endif
#ifdef SYS_rmdir
        case SYS_rmdir:
                return __syscall3(SYS_unlinkat, AT_FDCWD, a, AT_REMOVEDIR);
#endif
#ifdef CHCORE_SERVER_POSIX_SHM
        case SYS_shmdt:
                return chcore_shmdt((void *)a);
#endif
        case SYS_close:
                return chcore_close(a);
        case SYS_umask:
                debug("ignore SYS_umask\n");
                return 0;
        case SYS_uname: {
                struct utsname *buf = (struct utsname *)a;
                strcpy(buf->sysname, "ChCore");
                strcpy(buf->nodename, "");
                strcpy(buf->release, "0.0.0");
                strcpy(buf->version, "0.0.0");
                strcpy(buf->machine, "");
                strcpy(buf->domainname, "");
                return 0;
        }
        case SYS_exit:
                /* Thread exit: a is exitcode */
                chcore_syscall1(CHCORE_SYS_thread_exit, a);
                printf("[libc] error: thread_exit should never return.\n");
                return 0;
        case SYS_exit_group:
                /* Group exit: a is exitcode */
                chcore_syscall1(CHCORE_SYS_exit_group, a);
                printf("[libc] error: process_exit should never return.\n");
                return 0;
        case SYS_set_tid_address: {
                /* TODO: use the impl of oh-tee */
                return chcore_syscall1(CHCORE_SYS_set_tid_address, a);
        }
        case SYS_brk:
                /*
                 * Ideally we should do this initialization in only one place
                 * (__libc_chcore_init()), unluckily that won't work because
                 * there will be malloc()s before the __libc_chcore_init()
                 * function is called. So we also do the initialization here.
                 */
                init_chcore_aslr();
                init_heap_start();
                return chcore_syscall2(CHCORE_SYS_handle_brk, a, heap_start);
        case SYS_chdir: {
                return chcore_chdir((const char *)a);
        }
        case SYS_fchdir: {
                return chcore_fchdir(a);
        } break;
        case SYS_fsync: {
                return chcore_fsync(a);
        }
        case SYS_fdatasync: {
                return chcore_fdatasync(a);
        }
        case SYS_dup: {
                return __syscall3(SYS_fcntl, a, F_DUPFD, 0);
        }
        case SYS_sysinfo: {
                warn_once("SYS_sysinfo is not implemented.\n");
                return -1;
        }
        case SYS_epoll_create1:
                return chcore_epoll_create1(a);
#ifdef SYS_pipe
        case SYS_pipe:
                return __syscall2(SYS_pipe2, a, 0);
#endif
        default:
                dead(n);
                return chcore_syscall1(n, a);
        }
}

long __syscall2(long n, long a, long b)
{
        syscall_trace(n);

        switch (n) {
#ifdef SYS_stat
        case SYS_stat:
                return __syscall4(SYS_newfstatat, AT_FDCWD, a, b, 0);
#endif
#ifdef SYS_lstat
        case SYS_lstat:
                return __syscall4(
                        SYS_newfstatat, AT_FDCWD, a, b, AT_SYMLINK_NOFOLLOW);
#endif
#ifdef SYS_access
        case SYS_access:
                return __syscall4(SYS_faccessat, AT_FDCWD, a, b, 0);
#endif
#ifdef SYS_rename
        case SYS_rename:
                return __syscall4(SYS_renameat, AT_FDCWD, a, AT_FDCWD, b);
#endif
#ifdef SYS_symlink
        case SYS_symlink:
                return __syscall3(SYS_symlinkat, a, AT_FDCWD, b);
#endif
#ifdef SYS_mkdir
        case SYS_mkdir:
                return __syscall3(SYS_mkdirat, AT_FDCWD, a, b);
#endif
#ifdef SYS_open
        case SYS_open:
                return __syscall6(SYS_open, a, b, 0, 0, 0, 0);
#endif
        case SYS_fstat: {
                if (fd_dic[a] == 0) {
                        return -EBADF;
                }

                return __xstatxx(FS_REQ_FSTAT,
                                 a, /* fd */
                                 NULL, /* path  */
                                 0, /* flags */
                                 (struct stat *)b, /* statbuf */
                                 sizeof(struct stat) /* bufsize */
                );
        }
#ifdef SYS_chmod
        case SYS_chmod: {
                return chcore_fchmodat(AT_FDCWD, (char *)a, b);
        }
#endif
        case SYS_statfs: {
                return __xstatxx(FS_REQ_STATFS,
                                 AT_FDCWD, /* fd */
                                 (const char *)a, /* path  */
                                 0, /* flags */
                                 (struct statfs *)b, /* statbuf */
                                 sizeof(struct statfs) /* bufsize */
                );
        }
#ifdef SYS_dup2
        case SYS_dup2: {
                return chcore_dup2(a, b);
        }
#endif
        case SYS_fstatfs: {
                if (fd_dic[a] == 0) {
                        return -EBADF;
                }

                return __xstatxx(FS_REQ_FSTATFS,
                                 a, /* fd */
                                 NULL, /* path  */
                                 0, /* flags */
                                 (struct statfs *)b, /* statbuf */
                                 sizeof(struct statfs) /* bufsize */
                );
        }
        case SYS_getcwd: {
                return chcore_getcwd((char *)a, b);
        }
        case SYS_ftruncate: {
                return chcore_ftruncate(a, b);
        }
        case SYS_clock_gettime: {
                return chcore_syscall2(CHCORE_SYS_clock_gettime, a, b);
        }
        case SYS_set_robust_list: {
                /* TODO Futex not implemented! */
                return 0;
        }
        case SYS_munmap: {
                /* munmap: @a is addr, @b is len */
                // return chcore_syscall2(CHCORE_SYS_handle_munmap, a, b);
                return chcore_munmap((void *)a, b);
        }
        case SYS_setpgid: {
                warn_once("setpgid is not implemented.\n");
                return 0;
        }
        case SYS_membarrier: {
                // warn_once("SYS_membarrier is not implmeneted.\n");
                return 0;
        }
        case SYS_getrusage:
                warn_once("getrusage is not implemented.\n");
                memset((void *)b, 0, sizeof(struct rusage));
                return 0;
        case SYS_fcntl: {
                return __syscall3(n, a, b, 0);
        }
        case SYS_eventfd2: {
                return chcore_eventfd(a, b);
        }
        case SYS_timerfd_create:
                return chcore_timerfd_create(a, b);
#ifdef SYS_timerfd_gettime64
        case SYS_timerfd_gettime64:
#endif
        case SYS_timerfd_gettime:
                return chcore_timerfd_gettime(a, (struct itimerspec *)b);
        case SYS_umount2: {
                return chcore_umount((const char *)a);
        }
        case SYS_kill:
                return chcore_kill(a, b);
        default:
                dead(n);
                return chcore_syscall2(n, a, b);
        }
}

long __syscall3(long n, long a, long b, long c)
{
        int ret;
        /* avoid recurrent invocation */
        if (n != SYS_writev && n != SYS_ioctl && n != SYS_read)
                syscall_trace(n);

        switch (n) {
#ifdef SYS_readlink
        case SYS_readlink:
                return __syscall4(SYS_readlinkat, AT_FDCWD, a, b, c);
#endif
#ifdef SYS_open
        case SYS_open:
                return __syscall6(SYS_open, a, b, c, 0, 0, 0);
#endif
#ifdef CHCORE_SERVER_POSIX_SHM
        case SYS_shmget:
                return chcore_shmget(a, b, c);
        case SYS_shmat:
                return (unsigned long)chcore_shmat(a, (void *)b, c);
        case SYS_shmctl:
                return chcore_shmctl(a, b, (void *)c);
#endif
        case SYS_readv:
                return __syscall6(SYS_readv, a, b, c, 0, 0, 0);
        case SYS_ioctl: {
                return chcore_ioctl(a, b, (void *)c);
        }
        case SYS_writev: {
                return __syscall6(SYS_writev, a, b, c, 0, 0, 0);
        }
        case SYS_read: {
                return chcore_read((int)a, (void *)b, (size_t)c);
        }
        case SYS_sched_getaffinity: {
                /*
                 * @a: tid, 0 means the calling thread
                 * @b: the size of mask
                 * @c: CPU mask
                 */
                ret = chcore_syscall1(CHCORE_SYS_get_affinity, a);
                if (ret >= 0) {
                        CPU_SET(ret, c);
                        return b;
                } else {
                        /* Fail to get the affinity */
                        return ret;
                }
        }
        case SYS_sched_setaffinity: {
                /*
                 * @a: tid, 0 means the calling thread
                 * @b: the size of mask
                 * @c: CPU mask
                 */
                for (size_t i = 0; i < b; ++i) {
                        /* TODO: we only support set one cpu core now */
                        if (CPU_ISSET(i, c)) {
                                return chcore_syscall2(
                                        CHCORE_SYS_set_affinity, a, i);
                        }
                }
                return -1;
        }
        case SYS_lseek: {
                return chcore_lseek(a, b, c);
        }
        case SYS_mkdirat: {
                return chcore_mkdirat(a, (const char *)b, c);
        }
        case SYS_unlinkat: {
                return chcore_unlinkat(a, (const char *)b, c);
        }
        case SYS_symlinkat: {
                return chcore_symlinkat((const char *)a, b, (const char *)c);
        }
        case SYS_getdents64: {
                return chcore_getdents64(a, (char *)b, c);
        }
        case SYS_openat: {
                return __syscall6(SYS_openat, a, b, c, /* mode */ 0, 0, 0);
        }
        case SYS_futex: {
                /* Multiple sys_futex entries here because the number of
                 * parameter varies in different futex ops. */
                return chcore_syscall6(CHCORE_SYS_futex, a, b, c, 0, 0, 0);
        }
        case SYS_fcntl: {
                return chcore_fcntl(a, b, c);
        }
        case SYS_fchmodat: {
                return chcore_fchmodat(a, (char *)b, c);
        }
        case SYS_fchown: {
                warn_once("SYS_fchown is not implemented.\n");
                return 0;
        }
        case SYS_madvise: {
                warn_once("SYS_madvise is not implemented.\n");
                return 0;
        }
        case SYS_dup3: {
                return chcore_dup2(a, b);
        }
        case SYS_mprotect: {
                /* mproctect: @a addr, @b len, @c prot */
                return chcore_syscall3(CHCORE_SYS_handle_mprotect, a, b, c);
        }
        case SYS_mincore: {
                // TODO: no support now (just tell the caller: every
                // page is in memory)
                warn("SYS_mincore is not implemented.\n");

                /* mincore: @a addr, @b length, @c vec */
                size_t size;
                size_t cnt;
                size_t i;
                unsigned char *vec;

                size = ROUND_UP(b, PAGE_SIZE);
                cnt = size / PAGE_SIZE;
                vec = (unsigned char *)c;

                for (i = 0; i < cnt; ++i) {
                        vec[i] = (char)1;
                }
                return 0;
        }
        /**
         * SYS_ftruncate compat layer (for ABI not forcing 64bit
         * argument start at even parameters)
         */
        case SYS_ftruncate: {
                return chcore_ftruncate(a, _SYSCALL_LL_REV(b, c));
        }
        case SYS_faccessat: {
                return chcore_faccessat(a, (const char *)b, c, 0);
        }
        default:
                dead(n);
                return chcore_syscall3(n, a, b, c);
        }
}

pid_t chcore_waitpid(pid_t pid, int *status, int options, int d)
{
        ipc_msg_t *ipc_msg;
        int ret;
        struct proc_request pr;
        struct proc_request *reply_pr;

        /* register_ipc_client is unnecessary here */
        // procmgr_ipc_struct = ipc_register_client(procmgr_server_cap);
        // assert(procmgr_ipc_struct);

        ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        pr.req = PROC_REQ_WAIT;
        pr.wait.pid = pid;

        ipc_set_msg_data(ipc_msg, &pr, 0, sizeof(pr));
        /* This ipc_call returns the pid. */
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        assert(ret);
        if (ret > 0) {
                /* Get the actual exit status. */
                reply_pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
                if (status) {
                        *status = reply_pr->wait.exitstatus;
                }
        }
        // debug("pid=%d => exitstatus: %d\n", pid, pr.exitstatus);

        ipc_destroy_msg(ipc_msg);
        return ret;
}

long __syscall4(long n, long a, long b, long c, long d)
{
        syscall_trace(n);

        switch (n) {
        case SYS_wait4: {
                return chcore_waitpid((pid_t)a, (int *)b, (int)c, (int)d);
        }
        case SYS_newfstatat: {
                return __xstatxx(FS_REQ_FSTATAT,
                                 a, /* dirfd */
                                 (const char *)b, /* path  */
                                 d, /* flags */
                                 (struct stat *)c, /* statbuf */
                                 sizeof(struct stat) /* bufsize */
                );
        }
        case SYS_readlinkat: {
                return chcore_readlinkat(a, (const char *)b, (char *)c, d);
        }
#ifdef SYS_renameat
        case SYS_renameat: {
                return chcore_renameat(a, (const char *)b, c, (const char *)d);
        }
#endif
        case SYS_faccessat:
        case SYS_faccessat2: {
                return chcore_faccessat(a, (const char *)b, c, d);
        }
        case SYS_fallocate: {
                return chcore_fallocate(a, b, c, d);
        }
        case SYS_futex: {
                return chcore_syscall6(CHCORE_SYS_futex, a, b, c, d, 0, 0);
        }
        case SYS_rt_sigprocmask: {
                // warn_once("SYS_rt_sigprocmask is not implemented.\n");
                return 0;
        }
        case SYS_rt_sigaction: {
                warn_once("SYS_rt_sigaction is not implemented.\n");
                return 0;
        }
        case SYS_openat: {
                return __syscall6(SYS_openat, a, b, c, d, 0, 0);
        }
#ifdef SYS_timerfd_settime64
        case SYS_timerfd_settime64:
#endif
        case SYS_timerfd_settime:
                return chcore_timerfd_settime(
                        a, b, (struct itimerspec *)c, (struct itimerspec *)d);
        case SYS_prlimit64: {
                warn_once("SYS_prlimit64 is not implemented.\n");
                return -1;
        }
        case SYS_utimensat: {
                warn("SYS_utimensat is implemented.\n");
                return 0;
        }
        /**
         * SYS_ftruncate compat layer (for ABI forcing 64bit
         * argument start at even parameters, b would be zero here)
         */
        case SYS_ftruncate: {
                return chcore_ftruncate(a, _SYSCALL_LL_REV(c, d));
        }
        case SYS_epoll_ctl:
                return chcore_epoll_ctl(a, b, c, (struct epoll_event *)d);
        default:
                dead(n);
                return chcore_syscall4(n, a, b, c, d);
        }
}

long __syscall5(long n, long a, long b, long c, long d, long e)
{
        syscall_trace(n);

        switch (n) {
        case SYS_statx: {
                return chcore_statx(a, (const char *)b, c, d, (char *)e);
        }
        case SYS_renameat2: {
                /* Since @e is always zero, it could be ignored. */
                return chcore_renameat(a, (const char *)b, c, (const char *)d);
        }
        case SYS_futex:
                return chcore_syscall6(CHCORE_SYS_futex, a, b, c, d, e, 0);
        case SYS_mremap: {
                warn("SYS_mremap is not implemented.\n");
                /*
                 * At least, our libc can work without mremap
                 * since it will just try to invoke it and
                 * have backup solution.
                 */
                errno = -EINVAL;
                return -1;
        }
        case SYS_prctl: {
                warn_once("SYS_prctl is not implemented\n");
                return -1;
        }
        case SYS_mount: {
                return chcore_mount((const char *)a,
                                    (const char *)b,
                                    (const char *)c,
                                    (unsigned long)d,
                                    (const void *)e);
        }
        /**
         * lseek compat layer(for using and returning 64bit data of lseek()
         * on 32bit archs)
         */
#ifdef SYS__llseek
        case SYS__llseek: {
                off_t off = (off_t)b << 32 | c;
                off_t ret = chcore_lseek(a, off, e);
                *(off_t *)d = ret;
                /**
                 * Intentionally truncated, see unistd/lseek.c
                 */
                return ret < 0 ? ret : 0;
        }
#endif
        default:
                dead(n);
                return chcore_syscall5(n, a, b, c, d, e);
        }
}

long __syscall6(long n, long a, long b, long c, long d, long e, long f)
{
        if (n != SYS_io_submit && n != SYS_read)
                syscall_trace(n);

        switch (n) {
        case SYS_mmap: {
                /* TODO */
                if (e != -1) {
                        return (long)chcore_fmap((void *)a,
                                                 (size_t)b,
                                                 (int)c,
                                                 (int)d,
                                                 (int)e,
                                                 (off_t)f);
                }
                return (long)chcore_mmap(
                        (void *)a, (size_t)b, (int)c, (int)d, (int)e, (off_t)f);
        }
        /**
         * For mmap to access files larger than 32bits,
         * the @f argument has been divided by PAGE_SIZE in SYS_mmap2
         */
#ifdef SYS_mmap2
        case SYS_mmap2: {
                if (e != -1) {
                        off_t off = (off_t)f * PAGE_SIZE;
                        return (long)chcore_fmap((void *)a,
                                                 (size_t)b,
                                                 (int)c,
                                                 (int)d,
                                                 (int)e,
                                                 off);
                }
                return (long)chcore_mmap(
                        (void *)a, (size_t)b, (int)c, (int)d, (int)e, (off_t)f);
        }
#endif
        case SYS_fsync: {
                return chcore_fsync(a);
        }
        case SYS_fdatasync: {
                return chcore_fdatasync(a);
        }
        case SYS_close: {
                return chcore_close(a);
        }
        case SYS_ioctl: {
                return chcore_ioctl(a, b, (void *)c);
        }
        /* Network syscalls */
        case SYS_socket: {
                return chcore_socket(a, b, c);
        }
        case SYS_setsockopt: {
                return chcore_setsocketopt(a, b, c, (const void *)d, e);
        }
        case SYS_getsockopt: {
                return chcore_getsocketopt(
                        a, b, c, (void *)d, (socklen_t *restrict)e);
        }
        case SYS_getsockname: {
                return chcore_getsockname(a,
                                          (struct sockaddr *restrict)b,
                                          (socklen_t *restrict)c);
        }
        case SYS_getpeername: {
                return chcore_getpeername(a,
                                          (struct sockaddr *restrict)b,
                                          (socklen_t *restrict)c);
        }
        case SYS_bind: {
                return chcore_bind(a, (const struct sockaddr *)b, (socklen_t)c);
        }
        case SYS_listen: {
                return chcore_listen(a, b);
        }
        case SYS_accept: {
                return chcore_accept(a,
                                     (struct sockaddr *restrict)b,
                                     (socklen_t *restrict)c);
        }
        case SYS_connect: {
                return chcore_connect(
                        a, (const struct sockaddr *)b, (socklen_t)c);
        }
        case SYS_sendto: {
                return chcore_sendto(
                        a, (const void *)b, c, d, (const struct sockaddr *)e, f);
        }
        case SYS_sendmsg: {
                return chcore_sendmsg(a, (const struct msghdr *)b, c);
        }

        case SYS_recvfrom: {
                return chcore_recvfrom(a,
                                       (void *restrict)b,
                                       c,
                                       d,
                                       (struct sockaddr *restrict)e,
                                       (socklen_t *restrict)f);
        }
        case SYS_recvmsg: {
                return chcore_recvmsg(a, (struct msghdr *)b, c);
        }
        case SYS_shutdown: {
                return chcore_shutdown(a, b);
        }
#ifdef SYS_open
        case SYS_open:
                return __syscall6(SYS_openat, AT_FDCWD, a, b, c, 0, 0);
#endif
        case SYS_openat: {
                return chcore_openat(a, (const char *)b, c, d);
        }
        case SYS_write: {
                return chcore_write((int)a, (void *)b, (size_t)c);
        }
        case SYS_read: {
                return chcore_read((int)a, (void *)b, (size_t)c);
        }
        case SYS_epoll_pwait:
                return chcore_epoll_pwait(
                        a, (struct epoll_event *)b, c, d, (sigset_t *)e);
#ifdef SYS_poll
        case SYS_poll:
                return chcore_poll((struct pollfd *)a, b, c);
#endif
        case SYS_ppoll:
                return chcore_ppoll((struct pollfd *)a,
                                    b,
                                    (struct timespec *)c,
                                    (sigset_t *)d);
        case SYS_futex:
                return chcore_syscall6(CHCORE_SYS_futex, a, b, c, d, e, f);

        case SYS_readv: {
                return chcore_readv(a, (const struct iovec *)b, c);
        }
        case SYS_writev: {
                return chcore_writev(a, (const struct iovec *)b, c);
        }
#ifdef SYS_clock_nanosleep_time64
        case SYS_clock_nanosleep_time64:
#else
        case SYS_clock_nanosleep:
#endif
                /* Currently ChCore only support CLOCK_MONOTONIC */
                BUG_ON(a != CLOCK_MONOTONIC);
                BUG_ON(b != 0);
                return chcore_syscall4(CHCORE_SYS_clock_nanosleep, a, b, c, d);
        case SYS_nanosleep:
                /*
                 * In original libc, CLOCK_REALTIME is used.
                 * Neverthelss, ChCore simply uses tick, indicating supporting
                 * CLOCK_MONOTONIC only. It just affects the precision.
                 *
                 * Actually, this flag will not be checked in the kernel.
                 */
                return chcore_syscall4(
                        CHCORE_SYS_clock_nanosleep, CLOCK_MONOTONIC, 0, a, b);
        case SYS_getrandom: {
                // TODO: now this syscall does nothing,
                // it should generate random bytes later.
                warn("this getrandom is just a fake implementation\n");
                return b;
        }
        case SYS_gettid: {
                /* TODO: use the impl of oh-tee */
                return (long)pthread_self();
        }
        case SYS_rt_sigtimedwait: {
                warn_once("SYS_rt_sigtimedwait is not implemented.\n");
                return 0;
        }
        /**
         * SYS_fallocate compat layer
         */
        case SYS_fallocate: {
                return chcore_fallocate(
                        a, b, _SYSCALL_LL_REV(c, d), _SYSCALL_LL_REV(e, f));
        }
        /**
         * musl pread/pwrite wrapper uses syscall_cp to issue syscall, which
         * would be translate to a function with no variadic parameters, and
         * then that function would invoke
         * __syscall6 unconditionally. (see src/internal/syscall.h and
         * src/thread/pthread_cancel.c) So we have to check CHCORE_ARCH_XXX to
         * determine whether we have to use _SYSCALL_LL_REV to combine two 32bit
         * arguments. Fortunately, offset is an even parameter of pread/pwrite,
         * so there is no difference between 32bit ABIs.
         */
        case SYS_pread64: {
#if !defined(CHCORE_ARCH_SPARC)
                return chcore_pread(a, (void *)b, c, d);
#else
                return chcore_pread(a, (void *)b, c, _SYSCALL_LL_REV(d, e));
#endif
        }
        case SYS_pwrite64: {
#if !defined(CHCORE_ARCH_SPARC)
                return chcore_pwrite(a, (void *)b, c, d);
#else
                return chcore_pwrite(a, (void *)b, c, _SYSCALL_LL_REV(d, e));
#endif
        }
        default:
                dead(n);
                return chcore_syscall6(n, a, b, c, d, e, f);
        }
}

void dead(long n)
{
        printf("Unsupported syscall %d, bye.\n", n);
        volatile int *addr = (int *)n;
        *addr = 0;
}
