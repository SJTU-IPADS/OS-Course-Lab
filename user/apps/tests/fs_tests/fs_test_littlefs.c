#include "fstest.h"
#include "fs_test.h"
#include "fs_test_lib.h"

char *base_dir;

static inline void print_usage(void)
{
        printf("usage: fs_test_littlefs.bin ./test_dir");
}

int main(int argc, char *argv[], char *envp[])
{
        int ret;
        struct stat stat_buf;
        char cwd[4096];

        if (argc >= 3) {
                print_usage();
                return 0;
        }

        if (argc == 2) {
                base_dir = argv[1];
        } else {
                base_dir = "/lfs/base_dir";
        }

        /* Check Dependencys */
#if (defined CHECK_TIME) && (defined HAVE_FTRUNCATE)      \
        && (defined HAVE_TRUNCATE) && (defined HAVE_PERM) \
        && (defined HAVE_SYMLINK) && (defined HAVE_CHFLAGS)
        printf("FileSystem full test is ready!\n");
#else
        printf("Dependencies are not set correctly!\n");
#endif

        /* Configure Print */
#ifdef FS_TEST_LINUX
        printf("Running FS Test on Linux\n");
#else
        printf("Running FS Test on ChCore\n");
#endif

#if CHECK_TIME
        printf("CHECK_TIME is opened\n");
#else
        printf("CHECK_TIME is closed\n");
#endif

        /* Prepare */
        fs_assert_noent(base_dir);
        ret = mkdir(base_dir, 0);
        fs_assert_zero(ret);

        ret = lstat(base_dir, &stat_buf);
        fs_assert_zero(ret);
        fs_assert(stat_buf.st_mode & S_IFDIR);

        /* Store current work direcory */
        getcwd(cwd, 4096);
        fs_assert_zero(chdir(base_dir));

        /* Test: chcore-fstest */
        test_open_littlefs();
        test_lseek();
        test_content();
        test_dup();
        /* Test: pjdfstest-port */
        srand(time(NULL));
        run_pjdtest();

        /* Finish */
        printf("\n\nClean up!\n");
        fs_assert_zero(chdir(cwd));
        ret = rmdir(base_dir);
        fs_assert_zero(ret);
        fs_assert_noent(base_dir);
        printf("\nAll fs tests passed\n");

        return 0;
}