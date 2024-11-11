#include "chcore_littlefs_defs.h"
#include <lfs.h>
#include <bd/lfs_rambd.h>
#include <pthread.h>

lfs_rambd_t bd;

static pthread_mutex_t lfs_global_lock;

static int lfs_lock(const struct lfs_config *cfg)
{
        return pthread_mutex_lock(&lfs_global_lock);
}

static int lfs_unlock(const struct lfs_config *cfg)
{
        return pthread_mutex_unlock(&lfs_global_lock);
}

struct lfs_config bd_cfg = {
        .context = &bd,

        .read = lfs_rambd_read,
        .prog = lfs_rambd_prog,
        .erase = lfs_rambd_erase,
        .sync = lfs_rambd_sync,
        .lock = lfs_lock,
        .unlock = lfs_unlock,

        .read_size = 16,
        .prog_size = 16,
        .block_size = 512,
        .block_count = 102400,
        .block_cycles = -1,
        .cache_size = 64,
        .lookahead_size = 16
};

int init_bd() {
        return 0;
}