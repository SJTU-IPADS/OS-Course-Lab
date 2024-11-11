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

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2000-2002 Silicon Graphics, Inc.
 * All Rights Reserved.
 */

#include <setjmp.h>
#include <sys/uio.h>
#include <stddef.h>
#include <stdbool.h>
#include "global.h"

#ifdef HAVE_BTRFSUTIL_H
#include <btrfsutil.h>
#endif
#ifdef HAVE_ATTR_ATTRIBUTES_H
#include <attr/attributes.h>
#endif
#ifdef HAVE_LINUX_FIEMAP_H
#include <linux/fiemap.h>
#endif
#ifndef HAVE_ATTR_LIST
#define attr_list(path, buf, size, flags, cursor) (errno = -ENOSYS, -1)
#endif
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif
#ifdef AIO
#include <libaio.h>
io_context_t io_ctx;
#endif
#include <sys/syscall.h>
#include <sys/xattr.h>

#ifndef FS_IOC_GETFLAGS
#define FS_IOC_GETFLAGS _IOR('f', 1, long)
#endif
#ifndef FS_IOC_SETFLAGS
#define FS_IOC_SETFLAGS _IOW('f', 2, long)
#endif

#include <math.h>
#define XFS_ERRTAG_MAX       17
#define XFS_IDMODULO_MAX     31 /* user/group IDs (1 << x)  */
#define XFS_PROJIDMODULO_MAX 16 /* project IDs (1 << x)     */
#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

#ifndef HAVE_RENAMEAT2
#if !defined(SYS_renameat2) && defined(__x86_64__)
#define SYS_renameat2 316
#endif

#if !defined(SYS_renameat2) && defined(__i386__)
#define SYS_renameat2 353
#endif

static int renameat2(int dfd1, const char *path1, int dfd2, const char *path2,
                     unsigned int flags)
{
#ifdef SYS_renameat2
        return syscall(SYS_renameat2, dfd1, path1, dfd2, path2, flags);
#else
        errno = ENOSYS;
        return -1;
#endif
}
#endif

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1 << 0) /* Don't overwrite target */
#endif
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1 << 1) /* Exchange source and dest */
#endif
#ifndef RENAME_WHITEOUT
#define RENAME_WHITEOUT (1 << 2) /* Whiteout source */
#endif

#define FILELEN_MAX (32 * 4096)

typedef enum {
        OP_AFSYNC,
        OP_ALLOCSP,
        OP_AREAD,
        OP_AWRITE,
        OP_BULKSTAT,
        OP_BULKSTAT1,
        OP_CHOWN,
        OP_CLONERANGE,
        OP_COPYRANGE,
        OP_CREAT,
        OP_DEDUPERANGE,
        OP_DREAD,
        OP_DWRITE,
        OP_FALLOCATE,
        OP_FDATASYNC,
        OP_FIEMAP,
        OP_FREESP,
        OP_FSYNC,
        OP_GETATTR,
        OP_GETDENTS,
        OP_GETFATTR,
        OP_LINK,
        OP_LISTFATTR,
        OP_MKDIR,
        OP_MKNOD,
        OP_MREAD,
        OP_MWRITE,
        OP_PUNCH,
        OP_ZERO,
        OP_COLLAPSE,
        OP_INSERT,
        OP_READ,
        OP_READLINK,
        OP_READV,
        OP_REMOVEFATTR,
        OP_RENAME,
        OP_RNOREPLACE,
        OP_REXCHANGE,
        OP_RWHITEOUT,
        OP_RESVSP,
        OP_RMDIR,
        OP_SETATTR,
        OP_SETFATTR,
        OP_SETXATTR,
        OP_SNAPSHOT,
        OP_SPLICE,
        OP_STAT,
        OP_SUBVOL_CREATE,
        OP_SUBVOL_DELETE,
        OP_SYMLINK,
        OP_SYNC,
        OP_TRUNCATE,
        OP_UNLINK,
        OP_UNRESVSP,
        OP_WRITE,
        OP_WRITEV,
        OP_LAST
} opty_t;

typedef void (*opfnc_t)(int, long);

typedef struct opdesc {
        opty_t op;
        char *name;
        opfnc_t func;
        int freq;
        int iswrite;
} opdesc_t;

typedef struct fent {
        int id;
        int ft;
        int parent;
        int xattr_counter;
} fent_t;

typedef struct flist {
        int nfiles;
        int nslots;
        int tag;
        fent_t *fents;
} flist_t;

typedef struct pathname {
        int len;
        char *path;
} pathname_t;

struct print_flags {
        unsigned long mask;
        const char *name;
};

struct print_string {
        char *buffer;
        int len;
        int max;
};

#define FT_DIR     0
#define FT_DIRm    (1 << FT_DIR)
#define FT_REG     1
#define FT_REGm    (1 << FT_REG)
#define FT_SYM     2
#define FT_SYMm    (1 << FT_SYM)
#define FT_DEV     3
#define FT_DEVm    (1 << FT_DEV)
#define FT_RTF     4
#define FT_RTFm    (1 << FT_RTF)
#define FT_SUBVOL  5
#define FT_SUBVOLm (1 << FT_SUBVOL)
#define FT_nft     6
#define FT_ANYm    ((1 << FT_nft) - 1)
#define FT_REGFILE (FT_REGm | FT_RTFm)
#define FT_NOTDIR  (FT_ANYm & (~FT_DIRm & ~FT_SUBVOLm))
#define FT_ANYDIR  (FT_DIRm | FT_SUBVOLm)

#define FLIST_SLOT_INCR 16
#define NDCACHE         64

#define MAXFSIZE   ((1ULL << 63) - 1ULL)
#define MAXFSIZE32 ((1ULL << 40) - 1ULL)

#define XATTR_NAME_BUF_SIZE 18

void chown_f(int, long);
void clonerange_f(int, long);
void copyrange_f(int, long);
void creat_f(int, long);
void deduperange_f(int, long);
void fallocate_f(int, long);
void fdatasync_f(int, long);
void fiemap_f(int, long);
void fsync_f(int, long);
char *gen_random_string(int);
void getattr_f(int, long);
void getdents_f(int, long);
void getfattr_f(int, long);
void link_f(int, long);
void listfattr_f(int, long);
void mkdir_f(int, long);
void mknod_f(int, long);
void mread_f(int, long);
void mwrite_f(int, long);
void punch_f(int, long);
void zero_f(int, long);
void collapse_f(int, long);
void insert_f(int, long);
void read_f(int, long);
void readlink_f(int, long);
void readv_f(int, long);
void removefattr_f(int, long);
void rename_f(int, long);
void rnoreplace_f(int, long);
void rexchange_f(int, long);
void rwhiteout_f(int, long);
void rmdir_f(int, long);
void setattr_f(int, long);
void setfattr_f(int, long);
void snapshot_f(int, long);
void splice_f(int, long);
void stat_f(int, long);
void subvol_create_f(int, long);
void subvol_delete_f(int, long);
void symlink_f(int, long);
void sync_f(int, long);
void truncate_f(int, long);
void unlink_f(int, long);
void write_f(int, long);
void writev_f(int, long);
char *xattr_flag_to_string(int);

opdesc_t ops[] =
        {
                /* { OP_ENUM, "name", function, freq, iswrite }, */
                {OP_CHOWN, "chown", chown_f, 3, 1},
                {OP_CLONERANGE, "clonerange", clonerange_f, 4, 1},
                {OP_COPYRANGE, "copyrange", copyrange_f, 4, 1},
                {OP_CREAT, "creat", creat_f, 4, 1},
                {OP_DEDUPERANGE, "deduperange", deduperange_f, 4, 1},
                {OP_FALLOCATE, "fallocate", fallocate_f, 1, 1},
                {OP_FDATASYNC, "fdatasync", fdatasync_f, 1, 1},
                {OP_FIEMAP, "fiemap", fiemap_f, 1, 1},
                {OP_FSYNC, "fsync", fsync_f, 1, 1},
                {OP_GETATTR, "getattr", getattr_f, 1, 0},
                {OP_GETDENTS, "getdents", getdents_f, 1, 0},
                /* get extended attribute */
                {OP_GETFATTR, "getfattr", getfattr_f, 1, 0},
                {OP_LINK, "link", link_f, 1, 1},
                /* list extent attributes */
                {OP_LISTFATTR, "listfattr", listfattr_f, 1, 0},
                {OP_MKDIR, "mkdir", mkdir_f, 2, 1},
                {OP_MKNOD, "mknod", mknod_f, 2, 1},
                {OP_MREAD, "mread", mread_f, 2, 0},
                {OP_MWRITE, "mwrite", mwrite_f, 2, 1},
                {OP_PUNCH, "punch", punch_f, 1, 1},
                {OP_ZERO, "zero", zero_f, 1, 1},
                {OP_COLLAPSE, "collapse", collapse_f, 1, 1},
                {OP_INSERT, "insert", insert_f, 1, 1},
                {OP_READ, "read", read_f, 1, 0},
                {OP_READLINK, "readlink", readlink_f, 1, 0},
                {OP_READV, "readv", readv_f, 1, 0},
                /* remove (delete) extended attribute */
                {OP_REMOVEFATTR, "removefattr", removefattr_f, 1, 1},
                {OP_RENAME, "rename", rename_f, 2, 1},
                {OP_RNOREPLACE, "rnoreplace", rnoreplace_f, 2, 1},
                {OP_REXCHANGE, "rexchange", rexchange_f, 2, 1},
                {OP_RWHITEOUT, "rwhiteout", rwhiteout_f, 2, 1},
                {OP_RMDIR, "rmdir", rmdir_f, 1, 1},
                /* set attribute flag (FS_IOC_SETFLAGS ioctl) */
                {OP_SETATTR, "setattr", setattr_f, 0, 1},
                /* set extended attribute */
                {OP_SETFATTR, "setfattr", setfattr_f, 2, 1},
                /* set project id (XFS_IOC_FSSETXATTR ioctl) */
                {OP_SNAPSHOT, "snapshot", snapshot_f, 1, 1},
                {OP_SPLICE, "splice", splice_f, 1, 1},
                {OP_STAT, "stat", stat_f, 1, 0},
                {OP_SUBVOL_CREATE, "subvol_create", subvol_create_f, 1, 1},
                {OP_SUBVOL_DELETE, "subvol_delete", subvol_delete_f, 1, 1},
                {OP_SYMLINK, "symlink", symlink_f, 2, 1},
                {OP_SYNC, "sync", sync_f, 1, 1},
                {OP_TRUNCATE, "truncate", truncate_f, 2, 1},
                {OP_UNLINK, "unlink", unlink_f, 1, 1},
                {OP_WRITE, "write", write_f, 4, 1},
                {OP_WRITEV, "writev", writev_f, 4, 1},
},
         *ops_end;

flist_t flist[FT_nft] = {
        {0, 0, 'd', NULL},
        {0, 0, 'f', NULL},
        {0, 0, 'l', NULL},
        {0, 0, 'c', NULL},
        {0, 0, 'r', NULL},
        {0, 0, 's', NULL},
};

int dcache[NDCACHE];
int errrange;
int errtag;
opty_t *freq_table;
int freq_table_size;
char *homedir;
int *ilist;
int ilistlen;
off64_t maxfsize;
char *myprog;
int namerand;
int nameseq;
int nops;
int nproc = 1;
int operations = 1;
unsigned int idmodulo = XFS_IDMODULO_MAX;
unsigned int attr_mask = ~0;
int procid;
int rtpct;
unsigned long seed = 0;
ino_t top_ino;
int cleanup = 0;
int verbose = 0;
int verifiable_log = 0;
sig_atomic_t should_stop = 0;
sigjmp_buf *sigbus_jmp = NULL;
char *execute_cmd = NULL;
int execute_freq = 1;
struct print_string flag_str = {0};

void add_to_flist(int, int, int, int);
void append_pathname(pathname_t *, char *);
void check_cwd(void);
void cleanup_flist(void);
int creat_path(pathname_t *, mode_t);
void dcache_enter(int, int);
void dcache_init(void);
fent_t *dcache_lookup(int);
void dcache_purge(int, int);
void del_from_flist(int, int);
int dirid_to_name(char *, int);
void doproc(void);
int fent_to_name(pathname_t *, fent_t *);
bool fents_ancestor_check(fent_t *, fent_t *);
void fix_parent(int, int, bool);
void free_pathname(pathname_t *);
int generate_fname(fent_t *, int, pathname_t *, int *, int *);
int generate_xattr_name(int, char *, int);
int get_fname(int, long, pathname_t *, flist_t **, fent_t **, int *);
void init_pathname(pathname_t *);
int lchown_path(pathname_t *, uid_t, gid_t);
int link_path(pathname_t *, pathname_t *);
int lstat64_path(pathname_t *, struct stat64 *);
void make_freq_table(void);
int mkdir_path(pathname_t *, mode_t);
int mknod_path(pathname_t *, mode_t, dev_t);
void namerandpad(int, char *, int);
int open_file_or_dir(pathname_t *, int);
int open_path(pathname_t *, int);
DIR *opendir_path(pathname_t *);
void process_freq(char *);
int readlink_path(pathname_t *, char *, size_t);
int rename_path(pathname_t *, pathname_t *, int);
int rmdir_path(pathname_t *);
void separate_pathname(pathname_t *, char *, pathname_t *);
void show_ops(int, char *);
int stat64_path(pathname_t *, struct stat64 *);
int symlink_path(const char *, pathname_t *);
int truncate64_path(pathname_t *, off64_t);
int unlink_path(pathname_t *);
void usage(void);
void write_freq(void);
void zero_freq(void);
void non_btrfs_freq(const char *);

void sg_handler(int signum)
{
        switch (signum) {
        case SIGTERM:
                should_stop = 1;
                break;
        case SIGBUS:
                /*
                 * Only handle SIGBUS when mmap write to a hole and no
                 * block can be allocated due to ENOSPC, abort otherwise.
                 */
                if (sigbus_jmp) {
                        siglongjmp(*sigbus_jmp, -1);
                } else {
                        printf("Unknown SIGBUS is caught, Abort!\n");
                        abort();
                }
                /* should not reach here */
                break;
        default:
                break;
        }
}

int main(int argc, char **argv)
{
        char buf[10];
        int c;
        char *dirname = NULL;
        char *logname = NULL;
        char rpath[PATH_MAX];
        int fd;
        int i;
        char *p;
        int stat;
        struct timeval t;
        int nousage = 0;
        struct sigaction action;
        int loops = 1;
        const char *allopts = "cd:e:f:i:l:m:M:n:o:p:rs:S:vVwx:X:zH";

        errrange = errtag = 0;
        umask(0);
        nops = sizeof(ops) / sizeof(ops[0]);
        ops_end = &ops[nops];
        myprog = argv[0];
        while ((c = getopt(argc, argv, allopts)) != -1) {
                switch (c) {
                case 'c':
                        cleanup = 1;
                        break;
                case 'd':
                        dirname = optarg;
                        break;
                case 'e':
                        sscanf(optarg, "%d", &errtag);
                        if (errtag < 0) {
                                errtag = -errtag;
                                errrange = 1;
                        } else if (errtag == 0)
                                errtag = -1;
                        if (errtag >= XFS_ERRTAG_MAX) {
                                fprintf(stderr,
                                        "error tag %d too large (max %d)\n",
                                        errtag,
                                        XFS_ERRTAG_MAX - 1);
                                exit(1);
                        }
                        break;
                case 'f':
                        process_freq(optarg);
                        break;
                case 'i':
                        ilist = realloc(ilist, ++ilistlen * sizeof(*ilist));
                        ilist[ilistlen - 1] = strtol(optarg, &p, 16);
                        break;
                case 'm':
                        idmodulo = strtoul(optarg, NULL, 0);
                        if (idmodulo > XFS_IDMODULO_MAX) {
                                fprintf(stderr,
                                        "chown modulo %d too big (max %d)\n",
                                        idmodulo,
                                        XFS_IDMODULO_MAX);
                                exit(1);
                        }
                        break;
                case 'l':
                        loops = atoi(optarg);
                        break;
                case 'n':
                        operations = atoi(optarg);
                        break;
                case 'o':
                        logname = optarg;
                        break;

                case 'p':
                        nproc = atoi(optarg);
                        break;
                case 'r':
                        namerand = 1;
                        break;
                case 's':
                        seed = strtoul(optarg, NULL, 0);
                        break;
                case 'v':
                        verbose = 1;
                        break;
                case 'w':
                        write_freq();
                        break;
                case 'x':
                        execute_cmd = optarg;
                        break;
                case 'z':
                        zero_freq();
                        break;
                case 'M':
                        attr_mask = strtoul(optarg, NULL, 0);
                        break;
                case 'S':
                        i = 0;
                        if (optarg[0] == 'c')
                                i = 1;
                        show_ops(i, NULL);
                        printf("\n");
                        nousage = 1;
                        break;
                case 'V':
                        verifiable_log = 1;
                        break;

                case 'X':
                        execute_freq = strtoul(optarg, NULL, 0);
                        break;
                case '?':
                        fprintf(stderr, "%s - invalid parameters\n", myprog);
                        /* fall through */
                case 'H':
                        usage();
                        exit(1);
                }
        }

        if (!dirname) {
                /* no directory specified */
                if (!nousage)
                        usage();
                exit(1);
        }

        non_btrfs_freq(dirname);
        (void)mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR);
        if (logname && logname[0] != '/') {
                if (!getcwd(rpath, sizeof(rpath))) {
                        perror("getcwd failed");
                        exit(1);
                }
        } else {
                rpath[0] = '\0';
        }
        if (chdir(dirname) < 0) {
                perror(dirname);
                exit(1);
        }
        if (logname) {
                char path[PATH_MAX + NAME_MAX + 1];
                snprintf(path, sizeof(path), "%s/%s", rpath, logname);
                if (freopen(path, "a", stdout) == NULL) {
                        perror("freopen logfile failed");
                        exit(1);
                }
        }
        sprintf(buf, "fss%x", (unsigned int)getpid());
        fd = creat(buf, 0666);
        if (lseek64(fd, (off64_t)(MAXFSIZE32 + 1ULL), SEEK_SET) < 0)
                maxfsize = (off64_t)MAXFSIZE32;
        else
                maxfsize = (off64_t)MAXFSIZE;
        make_freq_table();
        dcache_init();
        setlinebuf(stdout);
        if (!seed) {
                gettimeofday(&t, (void *)NULL);
                seed = (int)t.tv_sec ^ (int)t.tv_usec;
                printf("seed = %ld\n", seed);
        }
        rtpct = 0;
        close(fd);

        setpgid(0, 0);
        action.sa_handler = sg_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction(SIGTERM, &action, 0)) {
                perror("sigaction failed");
                exit(1);
        }

        for (i = 0; i < nproc; i++) {
                // if (fork() == 0) {
                sigemptyset(&action.sa_mask);
                action.sa_handler = SIG_DFL;
                if (sigaction(SIGTERM, &action, 0))
                        return 1;
                action.sa_handler = sg_handler;
                if (sigaction(SIGBUS, &action, 0))
                        return 1;
#ifdef HAVE_SYS_PRCTL_H
                prctl(PR_SET_PDEATHSIG, SIGKILL);
                if (getppid() == 1) /* parent died already? */
                        return 0;
#endif
                if (logname) {
                        char path[PATH_MAX + NAME_MAX + 2 + 11];
                        snprintf(path,
                                 sizeof(path),
                                 "%s/%s.%d",
                                 rpath,
                                 logname,
                                 i);
                        if (freopen(path, "a", stdout) == NULL) {
                                perror("freopen logfile failed");
                                exit(1);
                        }
                }
                procid = i;
#ifdef AIO
                if (io_setup(128, &io_ctx) != 0) {
                        fprintf(stderr, "io_setup failed");
                        exit(1);
                }
#endif
                for (i = 0; !loops || (i < loops); i++)
                        doproc();
#ifdef AIO
                if (io_destroy(io_ctx) != 0) {
                        fprintf(stderr, "io_destroy failed");
                        return 1;
                }
#endif

                cleanup_flist();
                free(freq_table);
                return 0;
                // }
        }
        while (wait(&stat) > 0 && !should_stop) {
                continue;
        }
        action.sa_flags = SA_RESTART;
        sigaction(SIGTERM, &action, 0);
        kill(-getpid(), SIGTERM);
        while (wait(&stat) > 0)
                continue;

        if (errtag != 0) {
                close(fd);
        }

        free(freq_table);
        unlink(buf);
        return 0;
}

int add_string(struct print_string *str, const char *add)
{
        int len = strlen(add);

        if (len <= 0)
                return 0;

        if (len > (str->max - 1) - str->len) {
                str->len = str->max - 1;
                return 0;
        }

        memcpy(str->buffer + str->len, add, len);
        str->len += len;
        str->buffer[str->len] = '\0';

        return len;
}

char *translate_flags(int flags, const char *delim,
                      const struct print_flags *flag_array)
{
        int i, mask, first = 1;
        const char *add;

        if (!flag_str.buffer) {
                flag_str.buffer = malloc(4096);
                flag_str.max = 4096;
                flag_str.len = 0;
        }
        if (!flag_str.buffer)
                return NULL;
        flag_str.len = 0;
        flag_str.buffer[0] = '\0';

        for (i = 0; flag_array[i].name && flags; i++) {
                mask = flag_array[i].mask;
                if ((flags & mask) != mask)
                        continue;

                add = flag_array[i].name;
                flags &= ~mask;
                if (!first && delim)
                        add_string(&flag_str, delim);
                else
                        first = 0;
                add_string(&flag_str, add);
        }

        /* Check whether there are any leftover flags. */
        if (flags) {
                int ret;
                char number[11];

                if (!first && delim)
                        add_string(&flag_str, delim);

                ret = snprintf(number, 11, "0x%x", flags) > 0;
                if (ret > 0 && ret <= 11)
                        add_string(&flag_str, number);
        }

        return flag_str.buffer;
}

void add_to_flist(int ft, int id, int parent, int xattr_counter)
{
        fent_t *fep;
        flist_t *ftp;

        ftp = &flist[ft];
        if (ftp->nfiles == ftp->nslots) {
                ftp->nslots += FLIST_SLOT_INCR;
                ftp->fents = realloc(ftp->fents, ftp->nslots * sizeof(fent_t));
        }
        fep = &ftp->fents[ftp->nfiles++];
        fep->id = id;
        fep->ft = ft;
        fep->parent = parent;
        fep->xattr_counter = xattr_counter;
}

void append_pathname(pathname_t *name, char *str)
{
        int len;

        len = strlen(str);
#ifdef DEBUG
        /* attempting to append to a dir a zero length path */
        if (len && *str == '/' && name->len == 0) {
                fprintf(stderr, "fsstress: append_pathname failure\n");
                assert(chdir(homedir) == 0);
                abort();
                /* NOTREACHED */
        }
#endif
        name->path = realloc(name->path, name->len + 1 + len);
        strcpy(&name->path[name->len], str);
        name->len += len;
}

void check_cwd(void)
{
#ifdef DEBUG
        struct stat64 statbuf;

        if (stat64(".", &statbuf) == 0 && statbuf.st_ino == top_ino)
                return;
        assert(chdir(homedir) == 0);
        fprintf(stderr, "fsstress: check_cwd failure\n");
        abort();
        /* NOTREACHED */
#endif
}

/*
 * go thru flist and release all entries
 *
 * NOTE: this function shouldn't be called until the end of a process
 */
void cleanup_flist(void)
{
        flist_t *flp;
        int i;

        for (i = 0, flp = flist; i < FT_nft; i++, flp++) {
                flp->nslots = 0;
                flp->nfiles = 0;
                free(flp->fents);
                flp->fents = NULL;
        }
}

int creat_path(pathname_t *name, mode_t mode)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = creat(name->path, mode);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = creat_path(&newname, mode);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void dcache_enter(int dirid, int slot)
{
        dcache[dirid % NDCACHE] = slot;
}

void dcache_init(void)
{
        int i;

        for (i = 0; i < NDCACHE; i++)
                dcache[i] = -1;
}

fent_t *dcache_lookup(int dirid)
{
        fent_t *fep;
        int i;

        i = dcache[dirid % NDCACHE];
        if (i >= 0 && i < flist[FT_DIR].nfiles
            && (fep = &flist[FT_DIR].fents[i])->id == dirid)
                return fep;
        if (i >= 0 && i < flist[FT_SUBVOL].nfiles
            && (fep = &flist[FT_SUBVOL].fents[i])->id == dirid)
                return fep;
        return NULL;
}

void dcache_purge(int dirid, int ft)
{
        int *dcp;
        dcp = &dcache[dirid % NDCACHE];
        if (*dcp >= 0 && *dcp < flist[ft].nfiles
            && flist[ft].fents[*dcp].id == dirid)
                *dcp = -1;
}

/*
 * Delete the item from the list by
 * moving last entry over the deleted one;
 * unless deleted entry is the last one.
 * Input: which file list array and which slot in array
 */
void del_from_flist(int ft, int slot)
{
        flist_t *ftp;

        ftp = &flist[ft];
        if (ft == FT_DIR || ft == FT_SUBVOL)
                dcache_purge(ftp->fents[slot].id, ft);
        if (slot != ftp->nfiles - 1) {
                if (ft == FT_DIR || ft == FT_SUBVOL)
                        dcache_purge(ftp->fents[ftp->nfiles - 1].id, ft);
                ftp->fents[slot] = ftp->fents[--ftp->nfiles];
        } else
                ftp->nfiles--;
}

void delete_subvol_children(int parid)
{
        flist_t *flp;
        int i;
again:
        for (i = 0, flp = flist; i < FT_nft; i++, flp++) {
                int c;
                for (c = 0; c < flp->nfiles; c++) {
                        if (flp->fents[c].parent == parid) {
                                int id = flp->fents[c].id;
                                del_from_flist(i, c);
                                if (i == FT_DIR || i == FT_SUBVOL)
                                        delete_subvol_children(id);
                                goto again;
                        }
                }
        }
}

fent_t *dirid_to_fent(int dirid)
{
        fent_t *efep;
        fent_t *fep;
        flist_t *flp;
        int ft = FT_DIR;

        if ((fep = dcache_lookup(dirid)))
                return fep;
again:
        flp = &flist[ft];
        for (fep = flp->fents, efep = &fep[flp->nfiles]; fep < efep; fep++) {
                if (fep->id == dirid) {
                        dcache_enter(dirid, fep - flp->fents);
                        return fep;
                }
        }
        if (ft == FT_DIR) {
                ft = FT_SUBVOL;
                goto again;
        }
        return NULL;
}

void doproc(void)
{
        struct stat64 statbuf;
        char buf[10];
        char cmd[64];
        int opno;
        int rval;
        opdesc_t *p;
        int dividend;

        dividend = (operations + execute_freq) / (execute_freq + 1);
        sprintf(buf, "p%x", procid);
        (void)mkdir(buf, S_IRUSR | S_IWUSR | S_IXUSR);
        if (chdir(buf) < 0 || stat64(".", &statbuf) < 0) {
                perror(buf);
                _exit(1);
        }
        top_ino = statbuf.st_ino;
        homedir = getcwd(NULL, 0);
        if (!homedir) {
                perror("getcwd failed");
                _exit(1);
        }
        seed += procid;
        srandom(seed);
        if (namerand)
                namerand = random();
        for (opno = 0; opno < operations; opno++) {
                if (execute_cmd && opno && opno % dividend == 0) {
                        if (verbose)
                                printf("%d: execute command %s\n",
                                       opno,
                                       execute_cmd);
                        rval = system(execute_cmd);
                        if (rval)
                                fprintf(stderr,
                                        "execute command failed with "
                                        "%d\n",
                                        rval);
                }
                p = &ops[freq_table[random() % freq_table_size]];
                p->func(opno, random());
                /*
                 * test for forced shutdown by stat'ing the test
                 * directory.  If this stat returns EIO, assume
                 * the forced shutdown happened.
                 */
                if (errtag != 0 && opno % 100 == 0) {
                        rval = stat64(".", &statbuf);
                        if (rval == EIO) {
                                fprintf(stderr, "Detected EIO\n");
                                goto errout;
                        }
                }
        }
errout:
        assert(chdir("..") == 0);
        free(homedir);
        if (cleanup) {
                int ret;

                sprintf(cmd, "rm -rf %s", buf);
                ret = system(cmd);
                if (ret != 0)
                        perror("cleaning up");
                cleanup_flist();
        }
}

/*
 * build up a pathname going thru the file entry and all
 * its parent entries
 * Return 0 on error, 1 on success;
 */
int fent_to_name(pathname_t *name, fent_t *fep)
{
        flist_t *flp = &flist[fep->ft];
        char buf[NAME_MAX + 1];
        int i;
        fent_t *pfep;
        int e;

        if (fep == NULL)
                return 0;

        /* build up parent directory name */
        if (fep->parent != -1) {
                pfep = dirid_to_fent(fep->parent);
#ifdef DEBUG
                if (pfep == NULL) {
                        fprintf(stderr,
                                "%d: fent-id = %d: can't find parent id: %d\n",
                                procid,
                                fep->id,
                                fep->parent);
                }
#endif
                if (pfep == NULL)
                        return 0;
                e = fent_to_name(name, pfep);
                if (!e)
                        return 0;
                append_pathname(name, "/");
        }

        i = sprintf(buf, "%c%x", flp->tag, fep->id);
        namerandpad(fep->id, buf, i);
        append_pathname(name, buf);
        return 1;
}

bool fents_ancestor_check(fent_t *fep, fent_t *dfep)
{
        fent_t *tmpfep;

        for (tmpfep = fep; tmpfep->parent != -1;
             tmpfep = dirid_to_fent(tmpfep->parent)) {
                if (tmpfep->parent == dfep->id)
                        return true;
        }

        return false;
}

void fix_parent(int oldid, int newid, bool swap)
{
        fent_t *fep;
        flist_t *flp;
        int i;
        int j;

        for (i = 0, flp = flist; i < FT_nft; i++, flp++) {
                for (j = 0, fep = flp->fents; j < flp->nfiles; j++, fep++) {
                        if (fep->parent == oldid)
                                fep->parent = newid;
                        else if (swap && fep->parent == newid)
                                fep->parent = oldid;
                }
        }
}

void free_pathname(pathname_t *name)
{
        if (name->path) {
                free(name->path);
                name->path = NULL;
                name->len = 0;
        }
}

/*
 * Generate a filename of type ft.
 * If we have a fep which should be a directory then use it
 * as the parent path for this new filename i.e. prepend with it.
 */
int generate_fname(fent_t *fep, int ft, pathname_t *name, int *idp, int *v)
{
        char buf[NAME_MAX + 1];
        flist_t *flp;
        int id;
        int j;
        int len;
        int e;

        /* create name */
        flp = &flist[ft];
        len = sprintf(buf, "%c%x", flp->tag, id = nameseq++);
        namerandpad(id, buf, len);

        /* prepend fep parent dir-name to it */
        if (fep) {
                e = fent_to_name(name, fep);
                if (!e)
                        return 0;
                append_pathname(name, "/");
        }
        append_pathname(name, buf);

        *idp = id;
        *v = verbose;
        for (j = 0; !*v && j < ilistlen; j++) {
                if (ilist[j] == id) {
                        *v = 1;
                        break;
                }
        }
        return 1;
}

int generate_xattr_name(int xattr_num, char *buffer, int buflen)
{
        int ret;

        ret = snprintf(buffer, buflen, "user.x%d", xattr_num);
        if (ret < 0)
                return ret;
        if (ret < buflen)
                return 0;
        return -EOVERFLOW;
}

/*
 * Get file
 * Input: "which" to choose the file-types eg. non-directory
 * Input: "r" to choose which file
 * Output: file-list, file-entry, name for the chosen file.
 * Output: verbose if chosen file is on the ilist.
 */
int get_fname(int which, long r, pathname_t *name, flist_t **flpp,
              fent_t **fepp, int *v)
{
        int totalsum = 0; /* total number of matching files */
        int partialsum = 0; /* partial sum of matching files */
        fent_t *fep;
        flist_t *flp;
        int i;
        int j;
        int x;
        int e = 1; /* success */

        /*
         * go thru flist and add up number of files for each
         * category that matches with <which>.
         */
        for (i = 0, flp = flist; i < FT_nft; i++, flp++) {
                if (which & (1 << i))
                        totalsum += flp->nfiles;
        }
        if (totalsum == 0) {
                if (flpp)
                        *flpp = NULL;
                if (fepp)
                        *fepp = NULL;
                *v = verbose;
                return 0;
        }

        /*
         * Now we have possible matches between 0..totalsum-1.
         * And we use r to help us choose which one we want,
         * which when bounded by totalsum becomes x.
         */
        x = (int)(r % totalsum);
        for (i = 0, flp = flist; i < FT_nft; i++, flp++) {
                if (which & (1 << i)) {
                        if (x < partialsum + flp->nfiles) {
                                /* found the matching file entry */
                                fep = &flp->fents[x - partialsum];

                                /* fill-in what we were asked for */
                                if (name) {
                                        e = fent_to_name(name, fep);
#ifdef DEBUG
                                        if (!e) {
                                                fprintf(stderr,
                                                        "%d: failed to get path for entry:"
                                                        " id=%d,parent=%d\n",
                                                        procid,
                                                        fep->id,
                                                        fep->parent);
                                        }
#endif
                                }
                                if (flpp)
                                        *flpp = flp;
                                if (fepp)
                                        *fepp = fep;

                                /* turn on verbose if its an ilisted file */
                                *v = verbose;
                                for (j = 0; !*v && j < ilistlen; j++) {
                                        if (ilist[j] == fep->id) {
                                                *v = 1;
                                                break;
                                        }
                                }
                                return e;
                        }
                        partialsum += flp->nfiles;
                }
        }
#ifdef DEBUG
        fprintf(stderr, "fsstress: get_fname failure\n");
        abort();
#endif
        return 0;
}

void init_pathname(pathname_t *name)
{
        name->len = 0;
        name->path = NULL;
}

int lchown_path(pathname_t *name, uid_t owner, gid_t group)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = lchown(name->path, owner, group);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = lchown_path(&newname, owner, group);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int link_path(pathname_t *name1, pathname_t *name2)
{
        char buf1[NAME_MAX + 1];
        char buf2[NAME_MAX + 1];
        int down1;
        pathname_t newname1;
        pathname_t newname2;
        int rval;

        rval = link(name1->path, name2->path);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name1, buf1, &newname1);
        separate_pathname(name2, buf2, &newname2);
        if (strcmp(buf1, buf2) == 0) {
                if (chdir(buf1) == 0) {
                        rval = link_path(&newname1, &newname2);
                        assert(chdir("..") == 0);
                }
        } else {
                if (strcmp(buf1, "..") == 0)
                        down1 = 0;
                else if (strcmp(buf2, "..") == 0)
                        down1 = 1;
                else if (strlen(buf1) == 0)
                        down1 = 0;
                else if (strlen(buf2) == 0)
                        down1 = 1;
                else
                        down1 = MAX(newname1.len, 3 + name2->len)
                                <= MAX(3 + name1->len, newname2.len);
                if (down1) {
                        free_pathname(&newname2);
                        append_pathname(&newname2, "../");
                        append_pathname(&newname2, name2->path);
                        if (chdir(buf1) == 0) {
                                rval = link_path(&newname1, &newname2);
                                assert(chdir("..") == 0);
                        }
                } else {
                        free_pathname(&newname1);
                        append_pathname(&newname1, "../");
                        append_pathname(&newname1, name1->path);
                        if (chdir(buf2) == 0) {
                                rval = link_path(&newname1, &newname2);
                                assert(chdir("..") == 0);
                        }
                }
        }
        free_pathname(&newname1);
        free_pathname(&newname2);
        return rval;
}

int lstat64_path(pathname_t *name, struct stat64 *sbuf)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = lstat64(name->path, sbuf);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = lstat64_path(&newname, sbuf);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void make_freq_table(void)
{
        int f;
        int i;
        opdesc_t *p;

        for (p = ops, f = 0; p < ops_end; p++)
                f += p->freq;
        freq_table = malloc(f * sizeof(*freq_table));
        freq_table_size = f;
        for (p = ops, i = 0; p < ops_end; p++) {
                for (f = 0; f < p->freq; f++, i++)
                        freq_table[i] = p->op;
        }
}

int mkdir_path(pathname_t *name, mode_t mode)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = mkdir(name->path, mode);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = mkdir_path(&newname, mode);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int mknod_path(pathname_t *name, mode_t mode, dev_t dev)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = mknod(name->path, mode, dev);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = mknod_path(&newname, mode, dev);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void namerandpad(int id, char *buf, int i)
{
        int bucket;
        static int buckets[] = {2, 4, 8, 16, 32, 64, 128, NAME_MAX};
        int padlen;
        int padmod;

        if (namerand == 0)
                return;
        bucket = (id ^ namerand) % (sizeof(buckets) / sizeof(buckets[0]));
        padmod = buckets[bucket] + 1 - i;
        if (padmod <= 0)
                return;
        padlen = (id ^ namerand) % padmod;
        if (padlen) {
                memset(&buf[i], 'X', padlen);
                buf[i + padlen] = '\0';
        }
}

int open_file_or_dir(pathname_t *name, int flags)
{
        int fd;

        fd = open_path(name, flags);
        if (fd != -1)
                return fd;
        if (fd == -1 && errno != EISDIR)
                return fd;
        /* Directories can not be opened in write mode nor direct mode. */
        flags &= ~(O_WRONLY | O_DIRECT);
        flags |= O_RDONLY | O_DIRECTORY;
        return open_path(name, flags);
}

int open_path(pathname_t *name, int oflag)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = open(name->path, oflag);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = open_path(&newname, oflag);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

DIR *opendir_path(pathname_t *name)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        DIR *rval;

        rval = opendir(name->path);
        if (rval || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = opendir_path(&newname);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void process_freq(char *arg)
{
        opdesc_t *p;
        char *s;

        s = strchr(arg, '=');
        if (s == NULL) {
                fprintf(stderr, "bad argument '%s'\n", arg);
                exit(1);
        }
        *s++ = '\0';
        for (p = ops; p < ops_end; p++) {
                if (strcmp(arg, p->name) == 0) {
                        p->freq = atoi(s);
                        return;
                }
        }
        fprintf(stderr, "can't find op type %s for -f\n", arg);
        exit(1);
}

int readlink_path(pathname_t *name, char *lbuf, size_t lbufsiz)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = readlink(name->path, lbuf, lbufsiz);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = readlink_path(&newname, lbuf, lbufsiz);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int rename_path(pathname_t *name1, pathname_t *name2, int mode)
{
        char buf1[NAME_MAX + 1];
        char buf2[NAME_MAX + 1];
        int down1;
        pathname_t newname1;
        pathname_t newname2;
        int rval;

        if (mode == 0)
                rval = rename(name1->path, name2->path);
        else
                rval = renameat2(
                        AT_FDCWD, name1->path, AT_FDCWD, name2->path, mode);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name1, buf1, &newname1);
        separate_pathname(name2, buf2, &newname2);
        if (strcmp(buf1, buf2) == 0) {
                if (chdir(buf1) == 0) {
                        rval = rename_path(&newname1, &newname2, mode);
                        assert(chdir("..") == 0);
                }
        } else {
                if (strcmp(buf1, "..") == 0)
                        down1 = 0;
                else if (strcmp(buf2, "..") == 0)
                        down1 = 1;
                else if (strlen(buf1) == 0)
                        down1 = 0;
                else if (strlen(buf2) == 0)
                        down1 = 1;
                else
                        down1 = MAX(newname1.len, 3 + name2->len)
                                <= MAX(3 + name1->len, newname2.len);
                if (down1) {
                        free_pathname(&newname2);
                        append_pathname(&newname2, "../");
                        append_pathname(&newname2, name2->path);
                        if (chdir(buf1) == 0) {
                                rval = rename_path(&newname1, &newname2, mode);
                                assert(chdir("..") == 0);
                        }
                } else {
                        free_pathname(&newname1);
                        append_pathname(&newname1, "../");
                        append_pathname(&newname1, name1->path);
                        if (chdir(buf2) == 0) {
                                rval = rename_path(&newname1, &newname2, mode);
                                assert(chdir("..") == 0);
                        }
                }
        }
        free_pathname(&newname1);
        free_pathname(&newname2);
        return rval;
}

int rmdir_path(pathname_t *name)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = rmdir(name->path);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = rmdir_path(&newname);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void separate_pathname(pathname_t *name, char *buf, pathname_t *newname)
{
        char *slash;

        init_pathname(newname);
        slash = strchr(name->path, '/');
        if (slash == NULL) {
                buf[0] = '\0';
                return;
        }
        *slash = '\0';
        strcpy(buf, name->path);
        *slash = '/';
        append_pathname(newname, slash + 1);
}

#define WIDTH 80

void show_ops(int flag, char *lead_str)
{
        opdesc_t *p;

        if (flag < 0) {
                /* print in list form */
                int x = WIDTH;

                for (p = ops; p < ops_end; p++) {
                        if (lead_str != NULL
                            && x + strlen(p->name) >= WIDTH - 5)
                                x = printf("%s%s",
                                           (p == ops) ? "" : "\n",
                                           lead_str);
                        x += printf("%s ", p->name);
                }
                printf("\n");
        } else if (flag == 0) {
                /* Table view style */
                int f;
                for (f = 0, p = ops; p < ops_end; p++)
                        f += p->freq;

                if (f == 0)
                        flag = 1;

                for (p = ops; p < ops_end; p++) {
                        if (flag != 0 || p->freq > 0) {
                                if (lead_str != NULL)
                                        printf("%s", lead_str);
                                printf("%20s %d/%d %s\n",
                                       p->name,
                                       p->freq,
                                       f,
                                       (p->iswrite == 0) ? " " : "write op");
                        }
                }
        } else {
                /* Command line style */
                if (lead_str != NULL)
                        printf("%s", lead_str);
                printf("-z -s %ld -m %d -n %d -p %d \\\n",
                       seed,
                       idmodulo,
                       operations,
                       nproc);
                for (p = ops; p < ops_end; p++)
                        if (p->freq > 0)
                                printf("-f %s=%d \\\n", p->name, p->freq);
        }
}

int stat64_path(pathname_t *name, struct stat64 *sbuf)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = stat64(name->path, sbuf);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = stat64_path(&newname, sbuf);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int symlink_path(const char *name1, pathname_t *name)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        if (!strcmp(name1, name->path)) {
                printf("yikes! %s %s\n", name1, name->path);
                return 0;
        }

        rval = symlink(name1, name->path);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = symlink_path(name1, &newname);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int truncate64_path(pathname_t *name, off64_t length)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = truncate64(name->path, length);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = truncate64_path(&newname, length);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

int unlink_path(pathname_t *name)
{
        char buf[NAME_MAX + 1];
        pathname_t newname;
        int rval;

        rval = unlink(name->path);
        if (rval >= 0 || errno != ENAMETOOLONG)
                return rval;
        separate_pathname(name, buf, &newname);
        if (chdir(buf) == 0) {
                rval = unlink_path(&newname);
                assert(chdir("..") == 0);
        }
        free_pathname(&newname);
        return rval;
}

void usage(void)
{
        printf("Usage: %s -H   or\n", myprog);
        printf("       %s [-c][-d dir][-e errtg][-f op_name=freq][-l loops][-n nops]\n",
               myprog);
        printf("          [-p nproc][-r len][-s seed][-v][-w][-x cmd][-z][-S][-X ncmd]\n");
        printf("where\n");
        printf("   -c               clean up the test directory after each run\n");
        printf("   -d dir           specifies the base directory for operations\n");
        printf("   -e errtg         specifies error injection stuff\n");
        printf("   -f op_name=freq  changes the frequency of option name to freq\n");
        printf("                    the valid operation names are:\n");
        show_ops(-1, "                        ");
        printf("   -i filenum       get verbose output for this nth file object\n");
        printf("   -l loops         specifies the no. of times the testrun should loop.\n");
        printf("                     *use 0 for infinite (default 1)\n");
        printf("   -m modulo        uid/gid modulo for chown/chgrp (default 32)\n");
        printf("   -n nops          specifies the no. of operations per process (default 1)\n");
        printf("   -o logfile       specifies logfile name\n");
        printf("   -p nproc         specifies the no. of processes (default 1)\n");
        printf("   -r               specifies random name padding\n");
        printf("   -s seed          specifies the seed for the random generator (default random)\n");
        printf("   -v               specifies verbose mode\n");
        printf("   -w               zeros frequencies of non-write operations\n");
        printf("   -x cmd           execute command in the middle of operations\n");
        printf("   -z               zeros frequencies of all operations\n");
        printf("   -S [c,t]         prints the list of operations (omitting zero frequency) in command line or table style\n");
        printf("   -V               specifies verifiable logging mode (omitting inode numbers)\n");
        printf("   -X ncmd          number of calls to the -x command (default 1)\n");
        printf("   -H               prints usage and exits\n");
}

void write_freq(void)
{
        opdesc_t *p;

        for (p = ops; p < ops_end; p++) {
                if (!p->iswrite)
                        p->freq = 0;
        }
}

void zero_freq(void)
{
        opdesc_t *p;

        for (p = ops; p < ops_end; p++)
                p->freq = 0;
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

opty_t btrfs_ops[] = {
        OP_SNAPSHOT,
        OP_SUBVOL_CREATE,
        OP_SUBVOL_DELETE,
};

void non_btrfs_freq(const char *path)
{
        opdesc_t *p;
#ifdef HAVE_BTRFSUTIL_H
        enum btrfs_util_error e;

        e = btrfs_util_is_subvolume(path);
        if (e != BTRFS_UTIL_ERROR_NOT_BTRFS)
                return;
#endif
        for (p = ops; p < ops_end; p++) {
                int i;
                for (i = 0; i < ARRAY_SIZE(btrfs_ops); i++) {
                        if (p->op == btrfs_ops[i]) {
                                p->freq = 0;
                                break;
                        }
                }
        }
}

void inode_info(char *str, size_t sz, struct stat64 *s, int verbose)
{
        if (verbose)
                snprintf(str,
                         sz,
                         "[%ld %ld %d %d %lld %lld]",
                         verifiable_log ? -1 : (long)s->st_ino,
                         (long)s->st_nlink,
                         s->st_uid,
                         s->st_gid,
                         (long long)s->st_blocks,
                         (long long)s->st_size);
}

void chown_f(int opno, long r)
{
        int e;
        pathname_t f;
        int nbits;
        uid_t u;
        gid_t g;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_ANYm, r, &f, NULL, NULL, &v))
                append_pathname(&f, ".");
        u = (uid_t)random();
        g = (gid_t)random();
        nbits = (int)(random() % idmodulo);
        u &= (1 << nbits) - 1;
        g &= (1 << nbits) - 1;
        e = lchown_path(&f, u, g) < 0 ? errno : 0;
        check_cwd();
        if (v)
                printf("%d/%d: chown %s %d/%d %d\n",
                       procid,
                       opno,
                       f.path,
                       (int)u,
                       (int)g,
                       e);
        free_pathname(&f);
}

/* reflink some arbitrary range of f1 to f2. */
void clonerange_f(int opno, long r)
{
#ifdef FICLONERANGE
        struct file_clone_range fcr;
        struct pathname fpath1;
        struct pathname fpath2;
        struct stat64 stat1;
        struct stat64 stat2;
        char inoinfo1[1024];
        char inoinfo2[1024];
        off64_t lr;
        off64_t off1;
        off64_t off2;
        off64_t max_off2;
        size_t len;
        int v1;
        int v2;
        int fd1;
        int fd2;
        int ret;
        int e;

        /* Load paths */
        init_pathname(&fpath1);
        if (!get_fname(FT_REGm, r, &fpath1, NULL, NULL, &v1)) {
                if (v1)
                        printf("%d/%d: clonerange read - no filename\n",
                               procid,
                               opno);
                goto out_fpath1;
        }

        init_pathname(&fpath2);
        if (!get_fname(FT_REGm, random(), &fpath2, NULL, NULL, &v2)) {
                if (v2)
                        printf("%d/%d: clonerange write - no filename\n",
                               procid,
                               opno);
                goto out_fpath2;
        }

        /* Open files */
        fd1 = open_path(&fpath1, O_RDONLY);
        e = fd1 < 0 ? errno : 0;
        check_cwd();
        if (fd1 < 0) {
                if (v1)
                        printf("%d/%d: clonerange read - open %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               e);
                goto out_fpath2;
        }

        fd2 = open_path(&fpath2, O_WRONLY);
        e = fd2 < 0 ? errno : 0;
        check_cwd();
        if (fd2 < 0) {
                if (v2)
                        printf("%d/%d: clonerange write - open %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               e);
                goto out_fd1;
        }

        /* Get file stats */
        if (fstat64(fd1, &stat1) < 0) {
                if (v1)
                        printf("%d/%d: clonerange read - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo1, sizeof(inoinfo1), &stat1, v1);

        if (fstat64(fd2, &stat2) < 0) {
                if (v2)
                        printf("%d/%d: clonerange write - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo2, sizeof(inoinfo2), &stat2, v2);

        /* Calculate offsets */
        len = (random() % FILELEN_MAX) + 1;
        len &= ~(stat1.st_blksize - 1);
        if (len == 0)
                len = stat1.st_blksize;
        if (len > stat1.st_size)
                len = stat1.st_size;

        lr = ((int64_t)random() << 32) + random();
        if (stat1.st_size == len)
                off1 = 0;
        else
                off1 = (off64_t)(lr % MIN(stat1.st_size - len, MAXFSIZE));
        off1 %= maxfsize;
        off1 &= ~(stat1.st_blksize - 1);

        /*
         * If srcfile == destfile, randomly generate destination ranges
         * until we find one that doesn't overlap the source range.
         */
        max_off2 = MIN(stat2.st_size + (1024ULL * stat2.st_blksize), MAXFSIZE);
        do {
                lr = ((int64_t)random() << 32) + random();
                off2 = (off64_t)(lr % max_off2);
                off2 %= maxfsize;
                off2 &= ~(stat2.st_blksize - 1);
        } while (stat1.st_ino == stat2.st_ino && llabs(off2 - off1) < len);

        /* Clone data blocks */
        fcr.src_fd = fd1;
        fcr.src_offset = off1;
        fcr.src_length = len;
        fcr.dest_offset = off2;

        ret = ioctl(fd2, FICLONERANGE, &fcr);
        e = ret < 0 ? errno : 0;
        if (v1 || v2) {
                printf("%d/%d: clonerange %s%s [%lld,%lld] -> %s%s [%lld,%lld]",
                       procid,
                       opno,
                       fpath1.path,
                       inoinfo1,
                       (long long)off1,
                       (long long)len,
                       fpath2.path,
                       inoinfo2,
                       (long long)off2,
                       (long long)len);

                if (ret < 0)
                        printf(" error %d", e);
                printf("\n");
        }

out_fd2:
        close(fd2);
out_fd1:
        close(fd1);
out_fpath2:
        free_pathname(&fpath2);
out_fpath1:
        free_pathname(&fpath1);
#endif
}

/* copy some arbitrary range of f1 to f2. */
void copyrange_f(int opno, long r)
{
#ifdef HAVE_COPY_FILE_RANGE
        struct pathname fpath1;
        struct pathname fpath2;
        struct stat64 stat1;
        struct stat64 stat2;
        char inoinfo1[1024];
        char inoinfo2[1024];
        loff_t lr;
        loff_t off1;
        loff_t off2;
        loff_t offset1;
        loff_t offset2;
        loff_t max_off2;
        size_t len;
        size_t length;
        int tries = 0;
        int v1;
        int v2;
        int fd1;
        int fd2;
        size_t ret = 0;
        int e;

        /* Load paths */
        init_pathname(&fpath1);
        if (!get_fname(FT_REGm, r, &fpath1, NULL, NULL, &v1)) {
                if (v1)
                        printf("%d/%d: copyrange read - no filename\n",
                               procid,
                               opno);
                goto out_fpath1;
        }

        init_pathname(&fpath2);
        if (!get_fname(FT_REGm, random(), &fpath2, NULL, NULL, &v2)) {
                if (v2)
                        printf("%d/%d: copyrange write - no filename\n",
                               procid,
                               opno);
                goto out_fpath2;
        }

        /* Open files */
        fd1 = open_path(&fpath1, O_RDONLY);
        e = fd1 < 0 ? errno : 0;
        check_cwd();
        if (fd1 < 0) {
                if (v1)
                        printf("%d/%d: copyrange read - open %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               e);
                goto out_fpath2;
        }

        fd2 = open_path(&fpath2, O_WRONLY);
        e = fd2 < 0 ? errno : 0;
        check_cwd();
        if (fd2 < 0) {
                if (v2)
                        printf("%d/%d: copyrange write - open %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               e);
                goto out_fd1;
        }

        /* Get file stats */
        if (fstat64(fd1, &stat1) < 0) {
                if (v1)
                        printf("%d/%d: copyrange read - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo1, sizeof(inoinfo1), &stat1, v1);

        if (fstat64(fd2, &stat2) < 0) {
                if (v2)
                        printf("%d/%d: copyrange write - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo2, sizeof(inoinfo2), &stat2, v2);

        /* Calculate offsets */
        len = (random() % FILELEN_MAX) + 1;
        if (len == 0)
                len = stat1.st_blksize;
        if (len > stat1.st_size)
                len = stat1.st_size;

        lr = ((int64_t)random() << 32) + random();
        if (stat1.st_size == len)
                off1 = 0;
        else
                off1 = (off64_t)(lr % MIN(stat1.st_size - len, MAXFSIZE));
        off1 %= maxfsize;

        /*
         * If srcfile == destfile, randomly generate destination ranges
         * until we find one that doesn't overlap the source range.
         */
        max_off2 = MIN(stat2.st_size + (1024ULL * stat2.st_blksize), MAXFSIZE);
        do {
                lr = ((int64_t)random() << 32) + random();
                off2 = (off64_t)(lr % max_off2);
                off2 %= maxfsize;
        } while (stat1.st_ino == stat2.st_ino && llabs(off2 - off1) < len);

        /*
         * Since len, off1 and off2 will be changed later, preserve their
         * original values.
         */
        length = len;
        offset1 = off1;
        offset2 = off2;

        while (len > 0) {
                ret = syscall(
                        __NR_copy_file_range, fd1, &off1, fd2, &off2, len, 0);
                if (ret < 0) {
                        if (errno != EAGAIN || tries++ >= 300)
                                break;
                } else if (ret > len || ret == 0)
                        break;
                else if (ret > 0)
                        len -= ret;
        }
        e = ret < 0 ? errno : 0;
        if (v1 || v2) {
                printf("%d/%d: copyrange %s%s [%lld,%lld] -> %s%s [%lld,%lld]",
                       procid,
                       opno,
                       fpath1.path,
                       inoinfo1,
                       (long long)offset1,
                       (long long)length,
                       fpath2.path,
                       inoinfo2,
                       (long long)offset2,
                       (long long)length);

                if (ret < 0)
                        printf(" error %d", e);
                else if (len && ret > len)
                        printf(" asked for %lld, copied %lld??\n",
                               (long long)len,
                               (long long)ret);
                printf("\n");
        }

out_fd2:
        close(fd2);
out_fd1:
        close(fd1);
out_fpath2:
        free_pathname(&fpath2);
out_fpath1:
        free_pathname(&fpath1);
#endif
}

/* dedupe some arbitrary range of f1 to f2...fn. */
void deduperange_f(int opno, long r)
{
#ifdef FIDEDUPERANGE
#define INFO_SZ 1024
        struct file_dedupe_range *fdr;
        struct pathname *fpath;
        struct stat64 *stat;
        char *info;
        off64_t *off;
        int *v;
        int *fd;
        int nr;
        off64_t lr;
        size_t len;
        int ret;
        int i;
        int e;

        if (flist[FT_REG].nfiles < 2)
                return;

        /* Pick somewhere between 2 and 128 files. */
        do {
                nr = random() % (flist[FT_REG].nfiles + 1);
        } while (nr < 2 || nr > 128);

        /* Alloc memory */
        fdr = malloc(nr * sizeof(struct file_dedupe_range_info)
                     + sizeof(struct file_dedupe_range));
        if (!fdr) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                return;
        }
        memset(fdr,
               0,
               (nr * sizeof(struct file_dedupe_range_info)
                + sizeof(struct file_dedupe_range)));

        fpath = calloc(nr, sizeof(struct pathname));
        if (!fpath) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_fdr;
        }

        stat = calloc(nr, sizeof(struct stat64));
        if (!stat) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_paths;
        }

        info = calloc(nr, INFO_SZ);
        if (!info) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_stats;
        }

        off = calloc(nr, sizeof(off64_t));
        if (!off) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_info;
        }

        v = calloc(nr, sizeof(int));
        if (!v) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_offsets;
        }
        fd = calloc(nr, sizeof(int));
        if (!fd) {
                printf("%d/%d: line %d error %d\n",
                       procid,
                       opno,
                       __LINE__,
                       errno);
                goto out_v;
        }
        memset(fd, 0xFF, nr * sizeof(int));

        /* Get paths for all files */
        for (i = 0; i < nr; i++)
                init_pathname(&fpath[i]);

        if (!get_fname(FT_REGm, r, &fpath[0], NULL, NULL, &v[0])) {
                if (v[0])
                        printf("%d/%d: deduperange read - no filename\n",
                               procid,
                               opno);
                goto out_pathnames;
        }

        for (i = 1; i < nr; i++) {
                if (!get_fname(
                            FT_REGm, random(), &fpath[i], NULL, NULL, &v[i])) {
                        if (v[i])
                                printf("%d/%d: deduperange write - no filename\n",
                                       procid,
                                       opno);
                        goto out_pathnames;
                }
        }

        /* Open files */
        fd[0] = open_path(&fpath[0], O_RDONLY);
        e = fd[0] < 0 ? errno : 0;
        check_cwd();
        if (fd[0] < 0) {
                if (v[0])
                        printf("%d/%d: deduperange read - open %s failed %d\n",
                               procid,
                               opno,
                               fpath[0].path,
                               e);
                goto out_pathnames;
        }

        for (i = 1; i < nr; i++) {
                fd[i] = open_path(&fpath[i], O_WRONLY);
                e = fd[i] < 0 ? errno : 0;
                check_cwd();
                if (fd[i] < 0) {
                        if (v[i])
                                printf("%d/%d: deduperange write - open %s failed %d\n",
                                       procid,
                                       opno,
                                       fpath[i].path,
                                       e);
                        goto out_fds;
                }
        }

        /* Get file stats */
        if (fstat64(fd[0], &stat[0]) < 0) {
                if (v[0])
                        printf("%d/%d: deduperange read - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath[0].path,
                               errno);
                goto out_fds;
        }

        inode_info(&info[0], INFO_SZ, &stat[0], v[0]);

        for (i = 1; i < nr; i++) {
                if (fstat64(fd[i], &stat[i]) < 0) {
                        if (v[i])
                                printf("%d/%d: deduperange write - fstat64 %s failed %d\n",
                                       procid,
                                       opno,
                                       fpath[i].path,
                                       errno);
                        goto out_fds;
                }
                inode_info(&info[i * INFO_SZ], INFO_SZ, &stat[i], v[i]);
        }

        /* Never try to dedupe more than half of the src file. */
        len = (random() % FILELEN_MAX) + 1;
        len &= ~(stat[0].st_blksize - 1);
        if (len == 0)
                len = stat[0].st_blksize / 2;
        if (len > stat[0].st_size / 2)
                len = stat[0].st_size / 2;

        /* Calculate offsets */
        lr = ((int64_t)random() << 32) + random();
        if (stat[0].st_size == len)
                off[0] = 0;
        else
                off[0] = (off64_t)(lr % MIN(stat[0].st_size - len, MAXFSIZE));
        off[0] %= maxfsize;
        off[0] &= ~(stat[0].st_blksize - 1);

        /*
         * If srcfile == destfile[i], randomly generate destination ranges
         * until we find one that doesn't overlap the source range.
         */
        for (i = 1; i < nr; i++) {
                int tries = 0;

                do {
                        lr = ((int64_t)random() << 32) + random();
                        if (stat[i].st_size <= len)
                                off[i] = 0;
                        else
                                off[i] = (off64_t)(
                                        lr
                                        % MIN(stat[i].st_size - len, MAXFSIZE));
                        off[i] %= maxfsize;
                        off[i] &= ~(stat[i].st_blksize - 1);
                } while (stat[0].st_ino == stat[i].st_ino
                         && llabs(off[i] - off[0]) < len && tries++ < 10);
        }

        /* Clone data blocks */
        fdr->src_offset = off[0];
        fdr->src_length = len;
        fdr->dest_count = nr - 1;
        for (i = 1; i < nr; i++) {
                fdr->info[i - 1].dest_fd = fd[i];
                fdr->info[i - 1].dest_offset = off[i];
        }

        ret = ioctl(fd[0], FIDEDUPERANGE, fdr);
        e = ret < 0 ? errno : 0;
        if (v[0]) {
                printf("%d/%d: deduperange from %s%s [%lld,%lld]",
                       procid,
                       opno,
                       fpath[0].path,
                       &info[0],
                       (long long)off[0],
                       (long long)len);
                if (ret < 0)
                        printf(" error %d", e);
                printf("\n");
        }
        if (ret < 0)
                goto out_fds;

        for (i = 1; i < nr; i++) {
                e = fdr->info[i - 1].status < 0 ? fdr->info[i - 1].status : 0;
                if (v[i]) {
                        printf("%d/%d: ...to %s%s [%lld,%lld]",
                               procid,
                               opno,
                               fpath[i].path,
                               &info[i * INFO_SZ],
                               (long long)off[i],
                               (long long)len);
                        if (fdr->info[i - 1].status < 0)
                                printf(" error %d", e);
                        if (fdr->info[i - 1].status == FILE_DEDUPE_RANGE_SAME)
                                printf(" %llu bytes deduplicated",
                                       fdr->info[i - 1].bytes_deduped);
                        if (fdr->info[i - 1].status
                            == FILE_DEDUPE_RANGE_DIFFERS)
                                printf(" differed");
                        printf("\n");
                }
        }

out_fds:
        for (i = 0; i < nr; i++)
                if (fd[i] >= 0)
                        close(fd[i]);
out_pathnames:
        for (i = 0; i < nr; i++)
                free_pathname(&fpath[i]);

        free(fd);
out_v:
        free(v);
out_offsets:
        free(off);
out_info:
        free(info);
out_stats:
        free(stat);
out_paths:
        free(fpath);
out_fdr:
        free(fdr);
#endif
}

void splice_f(int opno, long r)
{
        struct pathname fpath1;
        struct pathname fpath2;
        struct stat64 stat1;
        struct stat64 stat2;
        char inoinfo1[1024];
        char inoinfo2[1024];
        loff_t lr;
        loff_t off1, off2;
        size_t len;
        loff_t offset1, offset2;
        size_t length;
        size_t total;
        int v1;
        int v2;
        int fd1;
        int fd2;
        ssize_t ret1 = 0, ret2 = 0;
        size_t bytes;
        int e;
        int filedes[2];

        /* Load paths */
        init_pathname(&fpath1);
        if (!get_fname(FT_REGm, r, &fpath1, NULL, NULL, &v1)) {
                if (v1)
                        printf("%d/%d: splice read - no filename\n",
                               procid,
                               opno);
                goto out_fpath1;
        }

        init_pathname(&fpath2);
        if (!get_fname(FT_REGm, random(), &fpath2, NULL, NULL, &v2)) {
                if (v2)
                        printf("%d/%d: splice write - no filename\n",
                               procid,
                               opno);
                goto out_fpath2;
        }

        /* Open files */
        fd1 = open_path(&fpath1, O_RDONLY);
        e = fd1 < 0 ? errno : 0;
        check_cwd();
        if (fd1 < 0) {
                if (v1)
                        printf("%d/%d: splice read - open %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               e);
                goto out_fpath2;
        }

        fd2 = open_path(&fpath2, O_WRONLY);
        e = fd2 < 0 ? errno : 0;
        check_cwd();
        if (fd2 < 0) {
                if (v2)
                        printf("%d/%d: splice write - open %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               e);
                goto out_fd1;
        }

        /* Get file stats */
        if (fstat64(fd1, &stat1) < 0) {
                if (v1)
                        printf("%d/%d: splice read - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath1.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo1, sizeof(inoinfo1), &stat1, v1);

        if (fstat64(fd2, &stat2) < 0) {
                if (v2)
                        printf("%d/%d: splice write - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               fpath2.path,
                               errno);
                goto out_fd2;
        }
        inode_info(inoinfo2, sizeof(inoinfo2), &stat2, v2);

        /* Calculate offsets */
        len = (random() % FILELEN_MAX) + 1;
        if (len == 0)
                len = stat1.st_blksize;
        if (len > stat1.st_size)
                len = stat1.st_size;

        lr = ((int64_t)random() << 32) + random();
        if (stat1.st_size == len)
                off1 = 0;
        else
                off1 = (off64_t)(lr % MIN(stat1.st_size - len, MAXFSIZE));
        off1 %= maxfsize;

        /*
         * splice can overlap write, so the offset of the target file can be
         * any number. But to avoid too large offset, add a clamp of 1024 blocks
         * past the current dest file EOF
         */
        lr = ((int64_t)random() << 32) + random();
        off2 = (off64_t)(
                lr
                % MIN(stat2.st_size + (1024ULL * stat2.st_blksize), MAXFSIZE));

        /*
         * Since len, off1 and off2 will be changed later, preserve their
         * original values.
         */
        length = len;
        offset1 = off1;
        offset2 = off2;

        /* Pipe initialize */
        if (pipe(filedes) < 0) {
                if (v1 || v2) {
                        printf("%d/%d: splice - pipe failed %d\n",
                               procid,
                               opno,
                               errno);
                        goto out_fd2;
                }
        }

        bytes = 0;
        total = 0;
        while (len > 0) {
                /* move to pipe buffer */
                ret1 = splice(fd1, &off1, filedes[1], NULL, len, 0);
                if (ret1 <= 0) {
                        break;
                }
                bytes = ret1;

                /* move from pipe buffer to dst file */
                while (bytes > 0) {
                        ret2 = splice(filedes[0], NULL, fd2, &off2, bytes, 0);
                        if (ret2 < 0) {
                                break;
                        }
                        bytes -= ret2;
                }
                if (ret2 < 0)
                        break;

                len -= ret1;
                total += ret1;
        }

        if (ret1 < 0 || ret2 < 0)
                e = errno;
        else
                e = 0;
        if (v1 || v2) {
                printf("%d/%d: splice %s%s [%lld,%lld] -> %s%s [%lld,%lld] %d",
                       procid,
                       opno,
                       fpath1.path,
                       inoinfo1,
                       (long long)offset1,
                       (long long)length,
                       fpath2.path,
                       inoinfo2,
                       (long long)offset2,
                       (long long)length,
                       e);

                if (length && length > total)
                        printf(" asked for %lld, spliced %lld??\n",
                               (long long)length,
                               (long long)total);
                printf("\n");
        }

        close(filedes[0]);
        close(filedes[1]);
out_fd2:
        close(fd2);
out_fd1:
        close(fd1);
out_fpath2:
        free_pathname(&fpath2);
out_fpath1:
        free_pathname(&fpath1);
}

void creat_f(int opno, long r)
{
        int e;
        int e1;
        pathname_t f;
        int fd;
        fent_t *fep;
        int id;
        int parid;
        int type;
        int v;
        int v1;

        if (!get_fname(FT_ANYDIR, r, NULL, NULL, &fep, &v1))
                parid = -1;
        else
                parid = fep->id;
        init_pathname(&f);
        e1 = (random() % 100);
        type = rtpct ? ((e1 > rtpct) ? FT_REG : FT_RTF) : FT_REG;
        e = generate_fname(fep, type, &f, &id, &v);
        v |= v1;
        if (!e) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: creat - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        fd = creat_path(&f, 0666);
        e = fd < 0 ? errno : 0;
        e1 = 0;
        check_cwd();
        if (fd >= 0) {
                add_to_flist(type, id, parid, 0);
                close(fd);
        }
        if (v) {
                printf("%d/%d: creat %s x:%d %d %d\n",
                       procid,
                       opno,
                       f.path,
                       0,
                       e,
                       e1);
                printf("%d/%d: creat add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&f);
}

#ifdef HAVE_LINUX_FALLOC_H
struct print_flags falloc_flags[] = {{FALLOC_FL_KEEP_SIZE, "KEEP_SIZE"},
                                     {FALLOC_FL_PUNCH_HOLE, "PUNCH_HOLE"},
                                     {FALLOC_FL_NO_HIDE_STALE, "NO_HIDE_STALE"},
                                     {FALLOC_FL_COLLAPSE_RANGE,
                                      "COLLAPSE_RANGE"},
                                     {FALLOC_FL_ZERO_RANGE, "ZERO_RANGE"},
                                     {FALLOC_FL_INSERT_RANGE, "INSERT_RANGE"},
                                     {-1, NULL}};

#define translate_falloc_flags(mode) \
        ({ translate_flags(mode, "|", falloc_flags); })
#endif

void do_fallocate(int opno, long r, int mode)
{
#ifdef HAVE_LINUX_FALLOC_H
        int e;
        pathname_t f;
        int fd;
        int64_t lr;
        off64_t off;
        off64_t len;
        struct stat64 stb;
        int v;
        char st[1024];

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: do_fallocate - no filename\n",
                               procid,
                               opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_RDWR);
        if (fd < 0) {
                if (v)
                        printf("%d/%d: do_fallocate - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                return;
        }
        check_cwd();
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: do_fallocate - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % MIN(stb.st_size + (1024 * 1024), MAXFSIZE));
        off %= maxfsize;
        len = (off64_t)(random() % (1024 * 1024));
        /*
         * Collapse/insert range requires off and len to be block aligned,
         * make it more likely to be the case.
         */
        if ((mode & (FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_INSERT_RANGE))
            && (opno % 2)) {
                off = ((off + stb.st_blksize - 1) & ~(stb.st_blksize - 1));
                len = ((len + stb.st_blksize - 1) & ~(stb.st_blksize - 1));
        }
        mode |= FALLOC_FL_KEEP_SIZE & random();
        e = fallocate(fd, mode, (loff_t)off, (loff_t)len) < 0 ? errno : 0;
        if (v)
                printf("%d/%d: fallocate(%s) %s %st %lld %lld %d\n",
                       procid,
                       opno,
                       translate_falloc_flags(mode),
                       f.path,
                       st,
                       (long long)off,
                       (long long)len,
                       e);
        free_pathname(&f);
        close(fd);
#endif
}

void fallocate_f(int opno, long r)
{
#ifdef HAVE_LINUX_FALLOC_H
        do_fallocate(opno, r, 0);
#endif
}

void fdatasync_f(int opno, long r)
{
        int e;
        pathname_t f;
        int fd;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: fdatasync - no filename\n",
                               procid,
                               opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_WRONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: fdatasync - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        e = fdatasync(fd) < 0 ? errno : 0;
        if (v)
                printf("%d/%d: fdatasync %s %d\n", procid, opno, f.path, e);
        free_pathname(&f);
        close(fd);
}

#ifdef HAVE_LINUX_FIEMAP_H
struct print_flags fiemap_flags[] = {{FIEMAP_FLAG_SYNC, "SYNC"},
                                     {FIEMAP_FLAG_XATTR, "XATTR"},
                                     {-1, NULL}};

#define translate_fiemap_flags(mode) \
        ({ translate_flags(mode, "|", fiemap_flags); })
#endif

void fiemap_f(int opno, long r)
{
#ifdef HAVE_LINUX_FIEMAP_H
        int e;
        pathname_t f;
        int fd;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];
        int blocks_to_map;
        struct fiemap *fiemap;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: fiemap - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_RDWR);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: fiemap - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: fiemap - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        blocks_to_map = random() & 0xffff;
        fiemap = (struct fiemap *)malloc(
                sizeof(struct fiemap)
                + (blocks_to_map * sizeof(struct fiemap_extent)));
        if (!fiemap) {
                if (v)
                        printf("%d/%d: malloc failed \n", procid, opno);
                free_pathname(&f);
                close(fd);
                return;
        }
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % MIN(stb.st_size + (1024 * 1024), MAXFSIZE));
        off %= maxfsize;
        fiemap->fm_flags = random() & (FIEMAP_FLAGS_COMPAT | 0x10000);
        fiemap->fm_extent_count = blocks_to_map;
        fiemap->fm_mapped_extents = random() & 0xffff;
        fiemap->fm_start = off;
        fiemap->fm_length = ((int64_t)random() << 32) + random();

        e = ioctl(fd, FS_IOC_FIEMAP, (unsigned long)fiemap);
        if (v)
                printf("%d/%d: ioctl(FIEMAP) %s%s %lld %lld (%s) %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)fiemap->fm_start,
                       (long long)fiemap->fm_length,
                       translate_fiemap_flags(fiemap->fm_flags),
                       e);
        free(fiemap);
        free_pathname(&f);
        close(fd);
#endif
}

void fsync_f(int opno, long r)
{
        int e;
        pathname_t f;
        int fd;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE | FT_DIRm, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: fsync - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_file_or_dir(&f, O_WRONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: fsync - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        e = fsync(fd) < 0 ? errno : 0;
        if (v)
                printf("%d/%d: fsync %s %d\n", procid, opno, f.path, e);
        free_pathname(&f);
        close(fd);
}

char *gen_random_string(int len)
{
        static const char charset[] = "0123456789"
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        int i;
        char *s;

        if (len == 0)
                return NULL;

        s = malloc(len);
        if (!s)
                return NULL;

        for (i = 0; i < len; i++)
                s[i] = charset[random() % sizeof(charset)];

        return s;
}

void getattr_f(int opno, long r)
{
        int fd;
        int e;
        pathname_t f;
        uint fl;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_ANYm, r, &f, NULL, NULL, &v))
                append_pathname(&f, ".");
        fd = open_path(&f, O_RDWR);
        e = fd < 0 ? errno : 0;
        check_cwd();

        e = ioctl(fd, FS_IOC_GETFLAGS, &fl);
        if (v)
                printf("%d/%d: getattr %s %u %d\n", procid, opno, f.path, fl, e);
        free_pathname(&f);
        close(fd);
}

void getdents_f(int opno, long r)
{
        DIR *dir;
        pathname_t f;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_ANYDIR, r, &f, NULL, NULL, &v))
                append_pathname(&f, ".");
        dir = opendir_path(&f);
        check_cwd();
        if (dir == NULL) {
                if (v)
                        printf("%d/%d: getdents - can't open %s\n",
                               procid,
                               opno,
                               f.path);
                free_pathname(&f);
                return;
        }
        while (readdir64(dir) != NULL)
                continue;
        if (v)
                printf("%d/%d: getdents %s 0\n", procid, opno, f.path);
        free_pathname(&f);
        closedir(dir);
}

void getfattr_f(int opno, long r)
{
        fent_t *fep;
        int e;
        pathname_t f;
        int v;
        char name[XATTR_NAME_BUF_SIZE];
        char *value = NULL;
        int value_len;
        int xattr_num;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE | FT_ANYDIR, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: getfattr - no filename\n", procid, opno);
                goto out;
        }
        check_cwd();

        /*
         * If the file/dir has xattrs, pick one randomly, otherwise attempt
         * to read a xattr that doesn't exist (fgetxattr should fail with
         * errno set to ENOATTR (61) in this case).
         */
        if (fep->xattr_counter > 0)
                xattr_num = (random() % fep->xattr_counter) + 1;
        else
                xattr_num = 0;

        e = generate_xattr_name(xattr_num, name, sizeof(name));
        if (e < 0) {
                printf("%d/%d: getfattr - file %s failed to generate xattr name: %d\n",
                       procid,
                       opno,
                       f.path,
                       e);
                goto out;
        }

        value_len = getxattr(f.path, name, NULL, 0);
        if (value_len < 0) {
                if (v)
                        printf("%d/%d: getfattr file %s name %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               name,
                               errno);
                goto out;
        }

        /* A xattr without value.*/
        if (value_len == 0) {
                e = 0;
                goto out_log;
        }

        value = malloc(value_len);
        if (!value) {
                if (v)
                        printf("%d/%d: getfattr file %s failed to allocate buffer with %d bytes\n",
                               procid,
                               opno,
                               f.path,
                               value_len);
                goto out;
        }

        e = getxattr(f.path, name, value, value_len) < 0 ? errno : 0;
out_log:
        if (v)
                printf("%d/%d: getfattr file %s name %s value length %d %d\n",
                       procid,
                       opno,
                       f.path,
                       name,
                       value_len,
                       e);
out:
        free(value);
        free_pathname(&f);
}

void link_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        fent_t *fep_src;
        flist_t *flp;
        int id;
        pathname_t l;
        int parid;
        int v;
        int v1;

        init_pathname(&f);
        if (!get_fname(FT_NOTDIR, r, &f, &flp, &fep_src, &v1)) {
                if (v1)
                        printf("%d/%d: link - no file\n", procid, opno);
                free_pathname(&f);
                return;
        }
        if (!get_fname(FT_DIRm, random(), NULL, NULL, &fep, &v))
                parid = -1;
        else
                parid = fep->id;
        v |= v1;
        init_pathname(&l);
        e = generate_fname(fep, flp - flist, &l, &id, &v1);
        v |= v1;
        if (!e) {
                if (v) {
                        (void)fent_to_name(&l, fep);
                        printf("%d/%d: link - no filename from %s\n",
                               procid,
                               opno,
                               l.path);
                }
                free_pathname(&l);
                free_pathname(&f);
                return;
        }
        e = link_path(&f, &l) < 0 ? errno : 0;
        check_cwd();
        if (e == 0)
                add_to_flist(flp - flist, id, parid, fep_src->xattr_counter);
        if (v) {
                printf("%d/%d: link %s %s %d\n",
                       procid,
                       opno,
                       f.path,
                       l.path,
                       e);
                printf("%d/%d: link add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&l);
        free_pathname(&f);
}

void listfattr_f(int opno, long r)
{
        fent_t *fep;
        int e;
        pathname_t f;
        int v;
        char *buffer = NULL;
        int buffer_len;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE | FT_ANYDIR, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: listfattr - no filename\n",
                               procid,
                               opno);
                goto out;
        }
        check_cwd();

        e = listxattr(f.path, NULL, 0);
        if (e < 0) {
                if (v)
                        printf("%d/%d: listfattr %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                goto out;
        }
        buffer_len = e;
        if (buffer_len == 0) {
                if (v)
                        printf("%d/%d: listfattr %s - has no extended attributes\n",
                               procid,
                               opno,
                               f.path);
                goto out;
        }

        buffer = malloc(buffer_len);
        if (!buffer) {
                if (v)
                        printf("%d/%d: listfattr %s failed to allocate buffer with %d bytes\n",
                               procid,
                               opno,
                               f.path,
                               buffer_len);
                goto out;
        }

        e = listxattr(f.path, buffer, buffer_len) < 0 ? errno : 0;
        if (v)
                printf("%d/%d: listfattr %s buffer length %d %d\n",
                       procid,
                       opno,
                       f.path,
                       buffer_len,
                       e);
out:
        free(buffer);
        free_pathname(&f);
}

void mkdir_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        int id;
        int parid;
        int v;
        int v1;

        if (!get_fname(FT_ANYDIR, r, NULL, NULL, &fep, &v))
                parid = -1;
        else
                parid = fep->id;
        init_pathname(&f);
        e = generate_fname(fep, FT_DIR, &f, &id, &v1);
        v |= v1;
        if (!e) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: mkdir - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        e = mkdir_path(&f, S_IRUSR | S_IWUSR | S_IXUSR) < 0 ? errno : 0;
        check_cwd();
        if (e == 0)
                add_to_flist(FT_DIR, id, parid, 0);
        if (v) {
                printf("%d/%d: mkdir %s %d\n", procid, opno, f.path, e);
                printf("%d/%d: mkdir add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&f);
}

void mknod_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        int id;
        int parid;
        int v;
        int v1;

        if (!get_fname(FT_ANYDIR, r, NULL, NULL, &fep, &v))
                parid = -1;
        else
                parid = fep->id;
        init_pathname(&f);
        e = generate_fname(fep, FT_DEV, &f, &id, &v1);
        v |= v1;
        if (!e) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: mknod - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        e = mknod_path(&f, S_IFCHR | 0444, 0) < 0 ? errno : 0;
        check_cwd();
        if (e == 0)
                add_to_flist(FT_DEV, id, parid, 0);
        if (v) {
                printf("%d/%d: mknod %s %d\n", procid, opno, f.path, e);
                printf("%d/%d: mknod add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&f);
}

#ifdef HAVE_SYS_MMAN_H
struct print_flags mmap_flags[] = {{MAP_SHARED, "SHARED"},
                                   {MAP_PRIVATE, "PRIVATE"},
                                   {-1, NULL}};

#define translate_mmap_flags(flags) \
        ({ translate_flags(flags, "|", mmap_flags); })
#endif

void do_mmap(int opno, long r, int prot)
{
#ifdef HAVE_SYS_MMAN_H
        char *addr;
        int e;
        pathname_t f;
        int fd;
        size_t len;
        int64_t lr;
        off64_t off;
        int flags;
        struct stat64 stb;
        int v;
        char st[1024];
        sigjmp_buf sigbus_jmpbuf;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: do_mmap - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_RDWR);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: do_mmap - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: do_mmap - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        if (stb.st_size == 0) {
                if (v)
                        printf("%d/%d: do_mmap - %s%s zero size\n",
                               procid,
                               opno,
                               f.path,
                               st);
                free_pathname(&f);
                close(fd);
                return;
        }

        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % stb.st_size);
        off &= (off64_t)(~(sysconf(_SC_PAGE_SIZE) - 1));
        len = (size_t)(random() % MIN(stb.st_size - off, FILELEN_MAX)) + 1;

        flags = (random() % 2) ? MAP_SHARED : MAP_PRIVATE;
        addr = mmap(NULL, len, prot, flags, fd, off);
        e = (addr == MAP_FAILED) ? errno : 0;
        if (e) {
                if (v)
                        printf("%d/%d: do_mmap - mmap failed %s%s [%lld,%d,%s] %d\n",
                               procid,
                               opno,
                               f.path,
                               st,
                               (long long)off,
                               (int)len,
                               translate_mmap_flags(flags),
                               e);
                free_pathname(&f);
                close(fd);
                return;
        }

        if (prot & PROT_WRITE) {
                if ((e = sigsetjmp(sigbus_jmpbuf, 1)) == 0) {
                        sigbus_jmp = &sigbus_jmpbuf;
                        memset(addr, nameseq & 0xff, len);
                }
        } else {
                char *buf;
                if ((buf = malloc(len)) != NULL) {
                        memcpy(buf, addr, len);
                        free(buf);
                }
        }
        munmap(addr, len);
        /* set NULL to stop other functions from doing siglongjmp */
        sigbus_jmp = NULL;

        if (v)
                printf("%d/%d: %s %s%s [%lld,%d,%s] %s\n",
                       procid,
                       opno,
                       (prot & PROT_WRITE) ? "mwrite" : "mread",
                       f.path,
                       st,
                       (long long)off,
                       (int)len,
                       translate_mmap_flags(flags),
                       (e == 0) ? "0" : "Bus error");

        free_pathname(&f);
        close(fd);
#endif
}

void mread_f(int opno, long r)
{
#ifdef HAVE_SYS_MMAN_H
        do_mmap(opno, r, PROT_READ);
#endif
}

void mwrite_f(int opno, long r)
{
#ifdef HAVE_SYS_MMAN_H
        do_mmap(opno, r, PROT_WRITE);
#endif
}

void punch_f(int opno, long r)
{
#ifdef HAVE_LINUX_FALLOC_H
        do_fallocate(opno, r, FALLOC_FL_PUNCH_HOLE);
#endif
}

void zero_f(int opno, long r)
{
#ifdef HAVE_LINUX_FALLOC_H
        do_fallocate(opno, r, FALLOC_FL_ZERO_RANGE);
#endif
}

void collapse_f(int opno, long r)
{
#ifdef HAVE_LINUX_FALLOC_H
        do_fallocate(opno, r, FALLOC_FL_COLLAPSE_RANGE);
#endif
}

void insert_f(int opno, long r)
{
#ifdef HAVE_LINUX_FALLOC_H
        do_fallocate(opno, r, FALLOC_FL_INSERT_RANGE);
#endif
}

void read_f(int opno, long r)
{
        char *buf;
        int e;
        pathname_t f;
        int fd;
        size_t len;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: read - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_RDONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: read - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: read - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        if (stb.st_size == 0) {
                if (v)
                        printf("%d/%d: read - %s%s zero size\n",
                               procid,
                               opno,
                               f.path,
                               st);
                free_pathname(&f);
                close(fd);
                return;
        }
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % stb.st_size);
        lseek64(fd, off, SEEK_SET);
        len = (random() % FILELEN_MAX) + 1;
        buf = malloc(len);
        e = read(fd, buf, len) < 0 ? errno : 0;
        free(buf);
        if (v)
                printf("%d/%d: read %s%s [%lld,%d] %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)off,
                       (int)len,
                       e);
        free_pathname(&f);
        close(fd);
}

void readlink_f(int opno, long r)
{
        char buf[PATH_MAX];
        int e;
        pathname_t f;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_SYMm, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: readlink - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        e = readlink_path(&f, buf, PATH_MAX) < 0 ? errno : 0;
        check_cwd();
        if (v)
                printf("%d/%d: readlink %s %d\n", procid, opno, f.path, e);
        free_pathname(&f);
}

void readv_f(int opno, long r)
{
        char *buf;
        int e;
        pathname_t f;
        int fd;
        size_t len;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];
        struct iovec *iov = NULL;
        int iovcnt;
        size_t iovb;
        size_t iovl;
        int i;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: readv - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_RDONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: readv - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: readv - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        if (stb.st_size == 0) {
                if (v)
                        printf("%d/%d: readv - %s%s zero size\n",
                               procid,
                               opno,
                               f.path,
                               st);
                free_pathname(&f);
                close(fd);
                return;
        }
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % stb.st_size);
        lseek64(fd, off, SEEK_SET);
        len = (random() % FILELEN_MAX) + 1;
        buf = malloc(len);

        iovcnt = (random() % MIN(len, IOV_MAX)) + 1;
        iov = calloc(iovcnt, sizeof(struct iovec));
        iovl = len / iovcnt;
        iovb = 0;
        for (i = 0; i < iovcnt; i++) {
                (iov + i)->iov_base = (buf + iovb);
                (iov + i)->iov_len = iovl;
                iovb += iovl;
        }

        e = readv(fd, iov, iovcnt) < 0 ? errno : 0;
        free(buf);
        if (v)
                printf("%d/%d: readv %s%s [%lld,%d,%d] %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)off,
                       (int)iovl,
                       iovcnt,
                       e);
        free_pathname(&f);
        close(fd);
}

void removefattr_f(int opno, long r)
{
        fent_t *fep;
        int e;
        pathname_t f;
        int v;
        char name[XATTR_NAME_BUF_SIZE];
        int xattr_num;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE | FT_ANYDIR, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: removefattr - no filename\n",
                               procid,
                               opno);
                goto out;
        }
        check_cwd();

        /*
         * If the file/dir has xattrs, pick one randomly, otherwise attempt to
         * remove a xattr that doesn't exist (fremovexattr should fail with
         * errno set to ENOATTR (61) in this case).
         */
        if (fep->xattr_counter > 0)
                xattr_num = (random() % fep->xattr_counter) + 1;
        else
                xattr_num = 0;

        e = generate_xattr_name(xattr_num, name, sizeof(name));
        if (e < 0) {
                printf("%d/%d: removefattr - file %s failed to generate xattr name: %d\n",
                       procid,
                       opno,
                       f.path,
                       e);
                goto out;
        }

        e = removexattr(f.path, name) < 0 ? errno : 0;
        if (v)
                printf("%d/%d: removefattr file %s name %s %d\n",
                       procid,
                       opno,
                       f.path,
                       name,
                       e);
out:
        free_pathname(&f);
}

struct print_flags renameat2_flags[] = {{RENAME_NOREPLACE, "NOREPLACE"},
                                        {RENAME_EXCHANGE, "EXCHANGE"},
                                        {RENAME_WHITEOUT, "WHITEOUT"},
                                        {-1, NULL}};

#define translate_renameat2_flags(mode) \
        ({ translate_flags(mode, "|", renameat2_flags); })

void do_renameat2(int opno, long r, int mode)
{
        fent_t *dfep;
        int e;
        pathname_t f;
        fent_t *fep;
        flist_t *flp;
        int id;
        pathname_t newf;
        int oldid = 0;
        int parid;
        int oldparid = 0;
        int which;
        int v;
        int v1;

        /* get an existing path for the source of the rename */
        init_pathname(&f);
        which = (mode == RENAME_WHITEOUT) ? FT_DEVm : FT_ANYm;
        if (!get_fname(which, r, &f, &flp, &fep, &v1)) {
                if (v1)
                        printf("%d/%d: rename - no source filename\n",
                               procid,
                               opno);
                free_pathname(&f);
                return;
        }

        /*
         * Both pathnames must exist for the RENAME_EXCHANGE, and in
         * order to maintain filelist/filename integrity, we should
         * restrict exchange operation to files of the same type.
         */
        if (mode == RENAME_EXCHANGE) {
                which = 1 << (flp - flist);
                init_pathname(&newf);
                if (!get_fname(which, random(), &newf, NULL, &dfep, &v)) {
                        if (v)
                                printf("%d/%d: rename - no target filename\n",
                                       procid,
                                       opno);
                        free_pathname(&newf);
                        free_pathname(&f);
                        return;
                }
                if (which == FT_DIRm
                    && (fents_ancestor_check(fep, dfep)
                        || fents_ancestor_check(dfep, fep))) {
                        if (v)
                                printf("%d/%d: rename(REXCHANGE) %s and %s "
                                       "have ancestor-descendant relationship\n",
                                       procid,
                                       opno,
                                       f.path,
                                       newf.path);
                        free_pathname(&newf);
                        free_pathname(&f);
                        return;
                }
                v |= v1;
                id = dfep->id;
                parid = dfep->parent;
        } else {
                /*
                 * Get an existing directory for the destination parent
                 * directory name.
                 */
                if (!get_fname(FT_DIRm, random(), NULL, NULL, &dfep, &v))
                        parid = -1;
                else
                        parid = dfep->id;
                v |= v1;

                /*
                 * Generate a new path using an existing parent directory
                 * in name.
                 */
                init_pathname(&newf);
                e = generate_fname(dfep, flp - flist, &newf, &id, &v1);
                v |= v1;
                if (!e) {
                        if (v) {
                                (void)fent_to_name(&f, dfep);
                                printf("%d/%d: rename - no filename from %s\n",
                                       procid,
                                       opno,
                                       f.path);
                        }
                        free_pathname(&newf);
                        free_pathname(&f);
                        return;
                }
        }
        e = rename_path(&f, &newf, mode) < 0 ? errno : 0;
        check_cwd();
        if (e == 0) {
                int xattr_counter = fep->xattr_counter;
                bool swap = (mode == RENAME_EXCHANGE) ? true : false;
                int ft = flp - flist;

                oldid = fep->id;
                oldparid = fep->parent;

                /*
                 * Swap the parent ids for RENAME_EXCHANGE, and replace the
                 * old parent id for the others.
                 */
                if (ft == FT_DIR || ft == FT_SUBVOL)
                        fix_parent(oldid, id, swap);

                if (mode == RENAME_WHITEOUT) {
                        fep->xattr_counter = 0;
                        add_to_flist(flp - flist, id, parid, xattr_counter);
                } else if (mode == RENAME_EXCHANGE) {
                        fep->xattr_counter = dfep->xattr_counter;
                        dfep->xattr_counter = xattr_counter;
                } else {
                        del_from_flist(flp - flist, fep - flp->fents);
                        add_to_flist(flp - flist, id, parid, xattr_counter);
                }
        }
        if (v) {
                printf("%d/%d: rename(%s) %s to %s %d\n",
                       procid,
                       opno,
                       translate_renameat2_flags(mode),
                       f.path,
                       newf.path,
                       e);
                if (e == 0) {
                        printf("%d/%d: rename source entry: id=%d,parent=%d\n",
                               procid,
                               opno,
                               oldid,
                               oldparid);
                        printf("%d/%d: rename target entry: id=%d,parent=%d\n",
                               procid,
                               opno,
                               id,
                               parid);
                }
        }
        free_pathname(&newf);
        free_pathname(&f);
}

void rename_f(int opno, long r)
{
        do_renameat2(opno, r, 0);
}

void rnoreplace_f(int opno, long r)
{
        do_renameat2(opno, r, RENAME_NOREPLACE);
}

void rexchange_f(int opno, long r)
{
        do_renameat2(opno, r, RENAME_EXCHANGE);
}

void rwhiteout_f(int opno, long r)
{
        do_renameat2(opno, r, RENAME_WHITEOUT);
}

void rmdir_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        int oldid = 0;
        int oldparid = 0;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_DIRm, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: rmdir - no directory\n", procid, opno);
                free_pathname(&f);
                return;
        }
        e = rmdir_path(&f) < 0 ? errno : 0;
        check_cwd();
        if (e == 0) {
                oldid = fep->id;
                oldparid = fep->parent;
                del_from_flist(FT_DIR, fep - flist[FT_DIR].fents);
        }
        if (v) {
                printf("%d/%d: rmdir %s %d\n", procid, opno, f.path, e);
                if (e == 0)
                        printf("%d/%d: rmdir del entry: id=%d,parent=%d\n",
                               procid,
                               opno,
                               oldid,
                               oldparid);
        }
        free_pathname(&f);
}

void setattr_f(int opno, long r)
{
        int fd;
        int e;
        pathname_t f;
        uint fl;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_ANYm, r, &f, NULL, NULL, &v))
                append_pathname(&f, ".");
        fd = open_path(&f, O_RDWR);
        e = fd < 0 ? errno : 0;
        check_cwd();

        fl = attr_mask & (uint)random();
        e = ioctl(fd, FS_IOC_SETFLAGS, &fl);
        if (v)
                printf("%d/%d: setattr %s %x %d\n", procid, opno, f.path, fl, e);
        free_pathname(&f);
        close(fd);
}

void setfattr_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        int v;
        int value_len;
        char name[XATTR_NAME_BUF_SIZE];
        char *value = NULL;
        int flag = 0;
        int xattr_num;

        init_pathname(&f);
        if (!get_fname(FT_REGFILE | FT_ANYDIR, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: setfattr - no filename\n", procid, opno);
                goto out;
        }
        check_cwd();

        if ((fep->xattr_counter > 0) && (random() % 2)) {
                /*
                 * Use an existing xattr name for replacing its value or
                 * create again a xattr that was previously deleted.
                 */
                xattr_num = (random() % fep->xattr_counter) + 1;
                if (random() % 2)
                        flag = XATTR_REPLACE;
        } else {
                /* Use a new xattr name. */
                xattr_num = fep->xattr_counter + 1;
                /*
                 * Don't always use the create flag because even if our xattr
                 * counter is 0, we may still have xattrs associated to this
                 * file (this happens when xattrs are added to a file through
                 * one of its other hard links), so we can end up updating an
                 * existing xattr too.
                 */
                if (random() % 2)
                        flag = XATTR_CREATE;
        }

        /*
         * The maximum supported value size depends on the filesystem
         * implementation, but 100 bytes is a safe value for most filesystems
         * at least.
         */
        value_len = random() % 101;
        value = gen_random_string(value_len);
        if (!value && value_len > 0) {
                if (v)
                        printf("%d/%d: setfattr - file %s failed to allocate value with %d bytes\n",
                               procid,
                               opno,
                               f.path,
                               value_len);
                goto out;
        }
        e = generate_xattr_name(xattr_num, name, sizeof(name));
        if (e < 0) {
                printf("%d/%d: setfattr - file %s failed to generate xattr name: %d\n",
                       procid,
                       opno,
                       f.path,
                       e);
                goto out;
        }

        e = setxattr(f.path, name, value, value_len, flag) < 0 ? errno : 0;
        if (e == 0)
                fep->xattr_counter++;
        if (v)
                printf("%d/%d: setfattr file %s name %s flag %s value length %d: %d\n",
                       procid,
                       opno,
                       f.path,
                       name,
                       xattr_flag_to_string(flag),
                       value_len,
                       e);
out:
        free(value);
        free_pathname(&f);
}

void snapshot_f(int opno, long r)
{
#ifdef HAVE_BTRFSUTIL_H
        enum btrfs_util_error e;
        pathname_t f;
        pathname_t newf;
        fent_t *fep;
        int id;
        int parid;
        int v;
        int v1;
        int err;

        init_pathname(&f);
        if (!get_fname(FT_SUBVOLm, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d/%d: snapshot - no subvolume\n",
                               procid,
                               opno);
                free_pathname(&f);
                return;
        }
        init_pathname(&newf);
        parid = fep->id;
        err = generate_fname(fep, FT_SUBVOL, &newf, &id, &v1);
        v |= v1;
        if (!err) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: snapshot - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        e = btrfs_util_create_snapshot(f.path, newf.path, 0, NULL, NULL);
        if (e == BTRFS_UTIL_OK)
                add_to_flist(FT_SUBVOL, id, parid, 0);
        if (v) {
                printf("%d/%d: snapshot %s->%s %d(%s)\n",
                       procid,
                       opno,
                       f.path,
                       newf.path,
                       e,
                       btrfs_util_strerror(e));
                printf("%d/%d: snapshot add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&newf);
        free_pathname(&f);
#endif
}

void stat_f(int opno, long r)
{
        int e;
        pathname_t f;
        struct stat64 stb;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_ANYm, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: stat - no entries\n", procid, opno);
                free_pathname(&f);
                return;
        }
        e = lstat64_path(&f, &stb) < 0 ? errno : 0;
        check_cwd();
        if (v)
                printf("%d/%d: stat %s %d\n", procid, opno, f.path, e);
        free_pathname(&f);
}

void subvol_create_f(int opno, long r)
{
#ifdef HAVE_BTRFSUTIL_H
        enum btrfs_util_error e;
        pathname_t f;
        fent_t *fep;
        int id;
        int parid;
        int v;
        int v1;
        int err;

        init_pathname(&f);
        if (!get_fname(FT_ANYDIR, r, NULL, NULL, &fep, &v))
                parid = -1;
        else
                parid = fep->id;
        err = generate_fname(fep, FT_SUBVOL, &f, &id, &v1);
        v |= v1;
        if (!err) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: subvol_create - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        e = btrfs_util_create_subvolume(f.path, 0, NULL, NULL);
        if (e == BTRFS_UTIL_OK)
                add_to_flist(FT_SUBVOL, id, parid, 0);
        if (v) {
                printf("%d/%d: subvol_create %s %d(%s)\n",
                       procid,
                       opno,
                       f.path,
                       e,
                       btrfs_util_strerror(e));
                printf("%d/%d: subvol_create add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&f);
#endif
}

void subvol_delete_f(int opno, long r)
{
#ifdef HAVE_BTRFSUTIL_H
        enum btrfs_util_error e;
        pathname_t f;
        fent_t *fep;
        int v;
        int oldid;
        int oldparid;

        init_pathname(&f);
        if (!get_fname(FT_SUBVOLm, r, &f, NULL, &fep, &v)) {
                if (v)
                        printf("%d:%d: subvol_delete - no subvolume\n",
                               procid,
                               opno);
                free_pathname(&f);
                return;
        }
        e = btrfs_util_delete_subvolume(f.path, 0);
        check_cwd();
        if (e == BTRFS_UTIL_OK) {
                oldid = fep->id;
                oldparid = fep->parent;
                delete_subvol_children(oldid);
                del_from_flist(FT_SUBVOL, fep - flist[FT_SUBVOL].fents);
        }
        if (v) {
                printf("%d/%d: subvol_delete %s %d(%s)\n",
                       procid,
                       opno,
                       f.path,
                       e,
                       btrfs_util_strerror(e));
                if (e == BTRFS_UTIL_OK)
                        printf("%d/%d: subvol_delete del entry: id=%d,parent=%d\n",
                               procid,
                               opno,
                               oldid,
                               oldparid);
        }
        free_pathname(&f);
#endif
}

void symlink_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        int i;
        int id;
        int len;
        int parid;
        int v;
        int v1;
        char *val;

        if (!get_fname(FT_ANYDIR, r, NULL, NULL, &fep, &v))
                parid = -1;
        else
                parid = fep->id;
        init_pathname(&f);
        e = generate_fname(fep, FT_SYM, &f, &id, &v1);
        v |= v1;
        if (!e) {
                if (v) {
                        (void)fent_to_name(&f, fep);
                        printf("%d/%d: symlink - no filename from %s\n",
                               procid,
                               opno,
                               f.path);
                }
                free_pathname(&f);
                return;
        }
        len = (int)(random() % PATH_MAX);
        val = malloc(len + 1);
        if (len)
                memset(val, 'x', len);
        val[len] = '\0';
        for (i = 10; i < len - 1; i += 10)
                val[i] = '/';
        e = symlink_path(val, &f) < 0 ? errno : 0;
        check_cwd();
        if (e == 0)
                add_to_flist(FT_SYM, id, parid, 0);
        free(val);
        if (v) {
                printf("%d/%d: symlink %s %d\n", procid, opno, f.path, e);
                printf("%d/%d: symlink add id=%d,parent=%d\n",
                       procid,
                       opno,
                       id,
                       parid);
        }
        free_pathname(&f);
}

/* ARGSUSED */
void sync_f(int opno, long r)
{
        sync();
        if (verbose)
                printf("%d/%d: sync\n", procid, opno);
}

void truncate_f(int opno, long r)
{
        int e;
        pathname_t f;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];

        init_pathname(&f);
        if (!get_fname(FT_REGFILE, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: truncate - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        e = stat64_path(&f, &stb) < 0 ? errno : 0;
        check_cwd();
        if (e > 0) {
                if (v)
                        printf("%d/%d: truncate - stat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % MIN(stb.st_size + (1024 * 1024), MAXFSIZE));
        off %= maxfsize;
        e = truncate64_path(&f, off) < 0 ? errno : 0;
        check_cwd();
        if (v)
                printf("%d/%d: truncate %s%s %lld %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)off,
                       e);
        free_pathname(&f);
}

void unlink_f(int opno, long r)
{
        int e;
        pathname_t f;
        fent_t *fep;
        flist_t *flp;
        int oldid = 0;
        int oldparid = 0;
        int v;

        init_pathname(&f);
        if (!get_fname(FT_NOTDIR, r, &f, &flp, &fep, &v)) {
                if (v)
                        printf("%d/%d: unlink - no file\n", procid, opno);
                free_pathname(&f);
                return;
        }
        e = unlink_path(&f) < 0 ? errno : 0;
        check_cwd();
        if (e == 0) {
                oldid = fep->id;
                oldparid = fep->parent;
                del_from_flist(flp - flist, fep - flp->fents);
        }
        if (v) {
                printf("%d/%d: unlink %s %d\n", procid, opno, f.path, e);
                if (e == 0)
                        printf("%d/%d: unlink del entry: id=%d,parent=%d\n",
                               procid,
                               opno,
                               oldid,
                               oldparid);
        }
        free_pathname(&f);
}

void write_f(int opno, long r)
{
        char *buf;
        int e;
        pathname_t f;
        int fd;
        size_t len;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];

        init_pathname(&f);
        if (!get_fname(FT_REGm, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: write - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_WRONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: write - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: write - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % MIN(stb.st_size + (1024 * 1024), MAXFSIZE));
        off %= maxfsize;
        lseek64(fd, off, SEEK_SET);
        len = (random() % FILELEN_MAX) + 1;
        buf = malloc(len);
        memset(buf, nameseq & 0xff, len);
        e = write(fd, buf, len) < 0 ? errno : 0;
        free(buf);
        if (v)
                printf("%d/%d: write %s%s [%lld,%d] %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)off,
                       (int)len,
                       e);
        free_pathname(&f);
        close(fd);
}

void writev_f(int opno, long r)
{
        char *buf;
        int e;
        pathname_t f;
        int fd;
        size_t len;
        int64_t lr;
        off64_t off;
        struct stat64 stb;
        int v;
        char st[1024];
        struct iovec *iov = NULL;
        int iovcnt;
        size_t iovb;
        size_t iovl;
        int i;

        init_pathname(&f);
        if (!get_fname(FT_REGm, r, &f, NULL, NULL, &v)) {
                if (v)
                        printf("%d/%d: writev - no filename\n", procid, opno);
                free_pathname(&f);
                return;
        }
        fd = open_path(&f, O_WRONLY);
        e = fd < 0 ? errno : 0;
        check_cwd();
        if (fd < 0) {
                if (v)
                        printf("%d/%d: writev - open %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               e);
                free_pathname(&f);
                return;
        }
        if (fstat64(fd, &stb) < 0) {
                if (v)
                        printf("%d/%d: writev - fstat64 %s failed %d\n",
                               procid,
                               opno,
                               f.path,
                               errno);
                free_pathname(&f);
                close(fd);
                return;
        }
        inode_info(st, sizeof(st), &stb, v);
        lr = ((int64_t)random() << 32) + random();
        off = (off64_t)(lr % MIN(stb.st_size + (1024 * 1024), MAXFSIZE));
        off %= maxfsize;
        lseek64(fd, off, SEEK_SET);
        len = (random() % FILELEN_MAX) + 1;
        buf = malloc(len);
        memset(buf, nameseq & 0xff, len);

        iovcnt = (random() % MIN(len, IOV_MAX)) + 1;
        iov = calloc(iovcnt, sizeof(struct iovec));
        iovl = len / iovcnt;
        iovb = 0;
        for (i = 0; i < iovcnt; i++) {
                (iov + i)->iov_base = (buf + iovb);
                (iov + i)->iov_len = iovl;
                iovb += iovl;
        }

        e = writev(fd, iov, iovcnt) < 0 ? errno : 0;
        free(buf);
        free(iov);
        if (v)
                printf("%d/%d: writev %s%s [%lld,%d,%d] %d\n",
                       procid,
                       opno,
                       f.path,
                       st,
                       (long long)off,
                       (int)iovl,
                       iovcnt,
                       e);
        free_pathname(&f);
        close(fd);
}

char *xattr_flag_to_string(int flag)
{
        if (flag == XATTR_CREATE)
                return "create";
        if (flag == XATTR_REPLACE)
                return "replace";
        return "none";
}
