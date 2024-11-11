#include "fs_test.h"
#include "fs_test_lib.h"

ssize_t random_write(int fd, ssize_t bytes) {
        char *buf;
        
        buf = malloc(bytes);
        for (int i = 0; i < bytes; i++) {
                buf[i] = rand() % ('z' - 'a') + 'a';
        }
        buf[bytes] = '\0';

        return write(fd, buf, bytes);
}

ssize_t random_pwrite(int fd, ssize_t bytes) {
        char *buf;
        
        buf = malloc(bytes);
        for (int i = 0; i < bytes; i++) {
                buf[i] = rand() % ('z' - 'a') + 'a';
        }
        buf[bytes] = '\0';

        return pwrite(fd, buf, bytes, 0);
}

void test_mode() {
        int fd;
        ssize_t ret;
        char *buf;

        buf = malloc(100*4096l);
        /* ===== DIR TESTS ===== */

        /* --x perm */
        fs_assert_zero(mkdir("test_dir", S_IXUSR));

        /* --x creating FAIL EXPECTED */
        file_op_assert(mkdir("test_dir/test_dir1", S_IRUSR | S_IWUSR | S_IXUSR), -1, EACCES);
        printf("TEST: no new dir under dir with mode 0%o OK\n", S_IXUSR);
        file_op_assert(open("test_dir/test_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR), -1, EACCES);
        printf("TEST: no new file under dir with mode 0%o OK\n", S_IXUSR);

        /* --x opening as readonly FAIL EXPECTED */
        file_op_assert(open("test_dir", O_DIRECTORY | O_RDONLY), -1, EACCES);
        printf("TEST: cannot open dir with mode 0%o as readonly OK\n", S_IXUSR);

        /* -w- perm */
      
  printf("before chmod\n");
        chmod("test_dir", S_IWUSR);
  printf("after chmod\n");

        /* -w- creating FAIL EXPECTED */
        file_op_assert(mkdir("test_dir/test_dir1", S_IRUSR | S_IWUSR | S_IXUSR), -1, EACCES);
        printf("TEST: no search under dir with mode 0%o OK\n", S_IWUSR);
        file_op_assert(open("test_dir/test_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR), -1, EACCES);
        printf("TEST: no search under dir with mode 0%o OK\n", S_IWUSR);

        /* -wx perm */
        fs_assert_zero(chmod("test_dir", S_IXUSR | S_IWUSR));

        /* -wx creating SUCCESS EXPECTED */
        fs_assert_zero(mkdir("test_dir/test_dir1", S_IRUSR | S_IWUSR | S_IXUSR));
        printf("TEST: new dir under dir with mode 0%o OK\n", S_IWUSR | S_IXUSR);
        fd = open("test_dir/test_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd == 3);
        fs_assert_zero(close(fd));
        printf("TEST: new file under dir with mode 0%o OK\n", S_IWUSR | S_IXUSR);

        /* -wx try to read FAIL EXPECTED */
        file_op_assert(open("test_dir", O_DIRECTORY | O_RDONLY), -1, EACCES);
        printf("TEST: cannot open dir with mode 0%o as readonly OK\n", S_IXUSR | S_IWUSR);

        /* -wx deleting SUCCESS EXPECTED */
        fs_assert_zero(unlink("test_dir/test_file"));
        printf("TEST: unlink file under dir with mode 0%o OK\n", S_IWUSR | S_IXUSR);
        fs_assert_zero(rmdir("test_dir/test_dir1"));
        printf("TEST: remove dir under dir with mode 0%o OK\n", S_IWUSR | S_IXUSR);

        fs_assert_zero(rmdir("test_dir"));
        printf("TEST: DIR test OK\n");

        /* ===== FILE TESTS ===== */

        /* -w- perm */
        fd = open("test_file", O_CREAT, S_IWUSR);
        fs_assert(fd == 3);
        close(fd);

        /* -w- opening as readonly FAIL EXPECTED */
        file_op_assert(open("test_file", O_RDONLY), -1, EACCES);
        printf("TEST: cannot open file with mode 0%o as readonly OK\n", S_IWUSR);

        /* -w- opening as read/write FAIL EXPECTED */
        file_op_assert(open("test_file", O_RDWR), -1, EACCES);
        printf("TEST: cannot open file with mode 0%o as read/write OK\n", S_IWUSR);

        /* -w- opening as writeonly SUCCESS EXPECTED */
        fd = open("test_file", O_WRONLY);
        fs_assert(fd == 3);
        printf("TEST: open file with mode 0%o as writeonly OK\n", S_IWUSR);

        ret = random_write(fd, 4096l*5);
        fs_assert(ret==4096l*5);
        ret = random_pwrite(fd, 4096l*5);
        fs_assert(ret==4096l*5);
        printf("TEST: write file with mode 0%o opened as writeonly OK\n", S_IWUSR);

        file_op_assert(read(fd, buf, 4096l*5), -1, EBADF);
        file_op_assert(pread(fd, buf, 4096l*5, 0), -1, EBADF);
        printf("TEST: cannot read file with mode 0%o opened as writeonly OK\n", S_IWUSR);

        /* TODO: test x permission */
        fs_assert_zero(close(fd));

        /* -r- perm */
        fs_assert_zero(chmod("test_file", S_IRUSR));

        /* -r- perm opening as writeonly FAIL EXPECTED */
        file_op_assert(open("test_file", O_WRONLY), -1, EACCES);
        printf("TEST: cannot open file with mode 0%o as writeonly OK\n", S_IRUSR);

        /* -r- perm opening as read/write FAIL EXPECTED */
        file_op_assert(open("test_file", O_RDWR), -1, EACCES);
        printf("TEST: cannot open file with mode 0%o as read/write OK\n", S_IRUSR);

        /* -r- perm opening as readonly SUCCESS EXPECTED */
        fd = open("test_file", O_RDONLY);
        fs_assert(fd == 3);
        printf("TEST: open file with mode 0%o as readonly OK\n", S_IRUSR);

        file_op_assert(random_write(fd, 4096l*5), -1, EBADF);
        file_op_assert(random_write(fd, 4096l*5), -1, EBADF);
        printf("TEST: cannot write file with mode 0%o opened as readonly OK\n", S_IRUSR);

        ret = read(fd, buf, 4096l*5);
        fs_assert(ret == 4096l*5);
        ret = pread(fd, buf, 4096l*5, 0);
        fs_assert(ret == 4096l*5);
        printf("TEST: read file with mode 0%o opened as readonly OK\n", S_IRUSR);
        printf("the last 10 character reads: %s\n", &buf[4096l*5-10]);

        fs_assert_zero(unlink("test_file"));
        printf("TEST: FILE test OK\n");
}
