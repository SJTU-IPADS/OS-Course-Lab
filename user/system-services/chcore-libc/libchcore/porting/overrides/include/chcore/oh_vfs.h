#ifndef OH_VFS_H
#define OH_VFS_H
#include <stdint.h>

void *vfs_mmap(int32_t fd, uint64_t size, uint64_t off);
int vfs_rename(int, const char *);

#endif
