#ifdef CHCORE_OPENTRUSTEE

#include "syscall.h"
#include <sys/mman.h>
#include <unistd.h>
#include <chcore/oh_vfs.h>
#include "file.h"

int vfs_rename(int fd, const char *path)
{
        return chcore_vfs_rename(fd, path);
}

void *vfs_mmap(int32_t vfs_fd, uint64_t size, uint64_t off)
{
        return mmap(
                NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, vfs_fd, off);
}

#endif /* CHCORE_OPENTRUSTEE */
