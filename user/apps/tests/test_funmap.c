#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

char name[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt";
#define PAGE_SIZE (1 << 12)

void step(void)
{
        static int cnt = 0;
        name[0] = 'a' + cnt % 26;
        name[1] = 'a' + cnt / 26 % 26;
        name[2] = 'a' + cnt / 26 / 26 % 26;
        printf("cnt %d name %s\n", cnt, name);
        cnt++;
}

int main(void)
{
        int fd, ret, i;
        char *addr;

        for (i = 0; i < 10; i++) {
                step();

                printf("%s %d\n", __func__, __LINE__);
                fd = open(name, O_CREAT | O_RDWR);
                assert(fd > 0);

                printf("%s %d\n", __func__, __LINE__);
                ret = ftruncate(fd, PAGE_SIZE * 16);
                assert(ret == 0);

                printf("%s %d\n", __func__, __LINE__);
                addr = mmap(NULL, PAGE_SIZE * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                assert(addr != MAP_FAILED);

                printf("after mmap\n");
                // use mm to check memory usage
                sleep(2);

                printf("%s %d\n", __func__, __LINE__);
                addr[0] = 1;
                addr[PAGE_SIZE] = 1;
                addr[PAGE_SIZE * 2] = 1;
                
                printf("%s %d\n", __func__, __LINE__);
                ret = munmap(addr, PAGE_SIZE);
                assert(ret == 0);

                printf("%s %d\n", __func__, __LINE__);
                close(fd);

                printf("%s %d\n", __func__, __LINE__);
                ret = unlink(name);
                assert(ret == 0);

                printf("after unlink\n");
                // use mm to check memory usage
                sleep(2);
        }
}