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

#include "fs_test.h"
#include "fs_test_lib.h"
#include "fstest_utils.h"
#include "unistd.h"

static char *sub_dir = "for_test_mmap";
char *test_mmap_dir;

static void generate_random_file_properties(int *filesize, int *offset)
{
        int a, b;
        srand((unsigned int)clock());
        /* guarantee that file larger than one page */
        a = TEST_FILE_SIZE;
        b = rand() % (a / PAGE_SIZE);
        b *= PAGE_SIZE;
        *filesize = a;
        *offset = b;
}

static void test_mmap_basic(char *fname)
{
        int fd, offset, filesize, mmap_size;
        char *addr;

        generate_random_file_properties(&filesize, &offset);
        fd = init_file(fname, filesize);
        mmap_size = PAGE_SIZE * ((filesize - offset) / PAGE_SIZE);
        fs_test_log("\nfilesize: 0x%x, offset: 0x%x, mmap_size: 0x%x\n",
                    filesize,
                    offset,
                    mmap_size);
        addr = mmap(
                NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        fs_test_eq(random_read_test(addr, offset, mmap_size), 0);
        fs_test_eq(random_write_test(addr, offset, mmap_size, fd), 0);

        deinit_file(fd, fname);
}

static void *test_mmap_basic_thread(void *args)
{
        unsigned long i = (unsigned long)args;
        char *fname;
        char buf[NAME_LENGTH];

        sprintf(buf, "test_mmap%ld", i);
        fname = path_join(test_mmap_dir, buf);
        test_mmap_basic(fname);

        return 0;
}

// test open diffrent file
static void test_mmap_multi_thread(void)
{
        int i = 0;

        fs_test_log("test_mmap_multi_thread begin\n");

        pthread_t thread_id[THREA_NUM];
        for (i = 0; i < THREA_NUM; ++i)
                pthread_create(&thread_id[i],
                               NULL,
                               test_mmap_basic_thread,
                               (void *)(unsigned long)i);
        for (i = 0; i < THREA_NUM; ++i)
                pthread_join(thread_id[i], NULL);

        fs_test_log("test_mmap multi thread done\n");
        return;
}

struct mmap_test_thread_args {
        int num;
        char *addr;
        int fd, offset, mmap_size;
};

static void *test_mmap_basic_thread_addr(void *args)
{
        struct mmap_test_thread_args *mps =
                (struct mmap_test_thread_args *)args;
        char *addr;
        int offset, mmap_size;

        addr = mps->addr;
        mmap_size = mps->mmap_size;
        offset = mps->offset;
        fs_test_eq(random_read_test(addr, offset, mmap_size), 0);

        return 0;
}

// test mmap one file
void test_mmap_multi_thread_mmap_onefile(void)
{
        int i = 0;
        char *fname;
        int fd, offset, filesize, mmap_size;
        char *addr;
        pthread_t thread_id[THREA_NUM];
        struct mmap_test_thread_args *mps[THREA_NUM];

        generate_random_file_properties(&filesize, &offset);
        fname = path_join(test_mmap_dir, "test_mmap.txt");
        fd = init_file(fname, filesize);
        mmap_size = PAGE_SIZE * ((filesize - offset) / PAGE_SIZE);

        fs_test_log("filesize: 0x%x, offset=0x%x, mmap_size=0x%x\n",
                    filesize,
                    offset,
                    mmap_size);

        addr = mmap(
                NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        for (i = 0; i < THREA_NUM; i++) {
                mps[i] = (struct mmap_test_thread_args *)malloc(sizeof(mps));
                mps[i]->fd = fd;
                mps[i]->addr = addr;
                mps[i]->mmap_size = mmap_size;
                mps[i]->offset = offset;
                mps[i]->num = i;
                pthread_create(&thread_id[i],
                               NULL,
                               test_mmap_basic_thread_addr,
                               (void *)mps[i]);
        }
        for (i = 0; i < THREA_NUM; i++)
                pthread_join(thread_id[i], NULL);
        deinit_file(fd, fname);
        printf("test_mmap  multi thread done\n");
        return;
}

void test_mmap_basic_single_process(void)
{
        int fd, offset, filesize, mmap_size;
        char *addr;
        char *fname;

        generate_random_file_properties(&filesize, &offset);
        fname = "test_mmap.txt";
        fd = init_file(fname, filesize);
        mmap_size = PAGE_SIZE * ((filesize - offset) / PAGE_SIZE);
        fs_test_log("filesize: 0x%x, offset=0x%x, mmap_size=0x%x\n",
                    filesize,
                    offset,
                    mmap_size);
        addr = mmap(
                NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
        fs_test_eq(random_read_test(addr, offset, mmap_size), 0);
        close(fd);
}

void test_mmap_multi_fs(void)
{
        char *fname1;
        char *fname2;
        char *fname3;

        /* mount sd card with two partitions, in ChCore only */
        fs_test_eq(mount("sda1", "/part1", 0, 0, 0), 0);
        fs_test_eq(mount("sda2", "/part2", 0, 0, 0), 0);
        fname1 = path_join("/part1", "test_mmap1.txt");
        fs_assert_noent(fname1);
        fname2 = path_join("/part2", "test_mmap2.txt");
        fs_assert_noent(fname2);
        fname3 = path_join(test_mmap_dir, "test_mmap3.txt");
        fs_assert_noent(fname3);

        printf("test_mmap_multi_fs fname : %s %s %s\n", fname1, fname2, fname3);
        test_mmap_basic(fname1);
        test_mmap_basic(fname2);
        test_mmap_basic(fname3);
        printf("test_mmap_multi_fs finished\n");
}

static void test_mmap_multi_process(void)
{
#ifdef FS_TEST_LINUX
        int i = 0;
        pid_t pid, pid1;
        pid = fork();
        fs_assert(pid >= 0);
        if (pid == 0) {
                execl("../fs_mmap_subprocess.bin", NULL);
                return;
        } else {
                pid1 = fork();
                if (pid1 == 0) {
                        execl("../fs_mmap_subprocess.bin", NULL);
                        return;
                }
                printf("hello\n");
                waitpid(pid1, NULL, 0);
                fs_test_eq(unlink("test_mmap.txt"), 0);
                return;
        }
#else
        char cwd[4096];
        getcwd(cwd, 4096);

        char *args[2];
        args[0] = "/fs_test_mmap.bin";
        args[1] = cwd; /* sending cwd for isolation in test_fs_concurrency */
        int status, i;
        pid_t pid[PROCESS_NUM];
        fs_test_log("\ntest_mmap_multi_process begin\n");

        for (i = 0; i < PROCESS_NUM; i++)
                pid[i] = create_process(2, args, NULL);
        for (i = 0; i < PROCESS_NUM; i++) {
                if (pid[i] > 0) {
                        chcore_waitpid(pid[i], &status, 0, 0);
                } else {
                        printf("failed to create process \n");
                }
        }

        /* path relative to cwd */
        fs_test_eq(unlink("test_mmap.txt"), 0);

        fs_test_log("\ntest_mmap_multi_process finished\n");
        return;
#endif
}

static void test_mmap_full(int file_size, int mmap_num)
{
        char *fname;
        int fd, offset;
        char *addr;
        int r;

        fs_test_log("file_size=0x%x, mmap_num=0x%x\n", file_size, mmap_num);

        fname = path_join(test_mmap_dir, "test_mmap.txt");
        fd = init_file(fname, file_size);
        addr = mmap(NULL,
                    mmap_num * PAGE_SIZE,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    0);

        offset = 0;
        if (mmap_num * PAGE_SIZE <= file_size) {
                r = random_read_test(addr, offset, mmap_num * PAGE_SIZE);
                fs_test_eq(r, 0);
                r = random_write_test(addr, offset, mmap_num * PAGE_SIZE, fd);
                fs_test_eq(r, 0);
        } else {
                r = random_read_test(
                        addr, offset, (file_size / PAGE_SIZE) * PAGE_SIZE);

                fs_test_eq(r, 0);
                r = random_write_test(
                        addr, offset, (file_size / PAGE_SIZE) * PAGE_SIZE, fd);
                fs_test_eq(r, 0);
        }

        printf("\ntest_mmap_full %d* PAGE*SIZE finished \n", mmap_num);
        deinit_file(fd, fname);
}

// test one thread mmap many files
static void test_mmap_single(int file_num)
{
        test_mmap_dir = sub_dir;
        char *fname[file_num];
        int fd[file_num], offset, filesize = 0, mmap_size = 0, i;
        char buf[1000];
        char *addr[file_num];
        int r;

        fs_test_log("\nfile_num = %d\n", file_num);

        generate_random_file_properties(&filesize, &offset);
        memset(buf, 0, 1000);
        for (i = 0; i < file_num; i++) {
                sprintf(buf, "test_mmap%d", i);
                fname[i] = path_join(test_mmap_dir, buf);
                fd[i] = init_file(fname[i], filesize);
                mmap_size = PAGE_SIZE * ((filesize - offset) / PAGE_SIZE);
                addr[i] = mmap(NULL,
                               mmap_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               fd[i],
                               offset);
        }

        r = 0;
        for (i = 0; i < file_num; ++i) {
                r |= random_read_test(addr[i], offset, mmap_size);
                r |= random_write_test(addr[i], offset, mmap_size, fd[i]);
                deinit_file(fd[i], fname[i]);
        }
        fs_test_eq(r, 0);

        fs_test_log("\ntest_mmap_single finished\n");
}

void test_mmap(void)
{
        int ret;

        /* Prepare */
        test_mmap_dir = sub_dir;
        printf("\ntest_mmap begin at : %s\n", test_mmap_dir);

        fs_assert_noent(test_mmap_dir);
        ret = mkdir(test_mmap_dir, S_IRUSR | S_IWUSR | S_IXUSR);
        fs_assert_zero(ret);

        /* Test Start */
        test_mmap_basic(path_join(test_mmap_dir, "test_mmap.txt"));
        test_mmap_single(50);
#ifdef FS_ON_SD_CARD
        test_mmap_multi_fs();
#endif
        test_mmap_full(40 * PAGE_SIZE, 5);
        test_mmap_full(45 * PAGE_SIZE, 45);
        test_mmap_full(50 * PAGE_SIZE, 60);
        test_mmap_multi_thread();
        test_mmap_multi_thread_mmap_onefile();
        test_mmap_multi_process();

        /* Finish */
        ret = rmdir(test_mmap_dir);
        fs_assert_zero(ret);
        fs_assert_noent(test_mmap_dir);

        printf("\ntest_mmap finished\n");
}
