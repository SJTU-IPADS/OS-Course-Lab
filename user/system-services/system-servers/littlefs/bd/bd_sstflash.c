#include "lfs.h"
#include "lfs_util.h"
#include "stddef.h"
#include <chcore-internal/flash_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <errno.h>
#include "chcore_littlefs_defs.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define PREFIX "[bd_flash]"
// #define DEBUG

#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define debug(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

ipc_struct_t *driver_icb = NULL;

static pthread_mutex_t lfs_global_lock;

char *__flash_req_to_buf(struct flash_request *req) {
        return (char *)req + sizeof(struct flash_request);
}

static int lfs_lock(const struct lfs_config *cfg)
{
        return pthread_mutex_lock(&lfs_global_lock);
}

static int lfs_unlock(const struct lfs_config *cfg)
{
        return pthread_mutex_unlock(&lfs_global_lock);
}

int lfs_sstflash_read(const struct lfs_config *cfg, lfs_block_t block,
                      lfs_off_t off, void *buffer, lfs_size_t size)
{
        int ret;
        void *buf_pos;
        char *req_data_buf;
        lfs_size_t remains = size; 
        lfs_size_t driver_mtu = IPC_SHM_AVAILABLE - sizeof(struct flash_request);
        lfs_size_t req_data_size;
        struct flash_request *driver_req;
        ipc_msg_t *driver_msg;

        debug("lfs_sstflash_read(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")\n",
            (void*)cfg, block, off, buffer, size);

        driver_msg = ipc_create_msg(driver_icb, sizeof(struct flash_request));
        driver_req = (struct flash_request *)ipc_get_msg_data(driver_msg);
        
        driver_req->req = FLASH_REQ_READ;
        driver_req->read.block = block;
        driver_req->read.offset = off;

        buf_pos = buffer;
        while (remains) {
                req_data_size = lfs_min(remains, driver_mtu);
                
                driver_req->read.length = req_data_size;

                ret = ipc_call(driver_icb, driver_msg);
                if (ret < 0) {
                        ret = LFS_ERR_CORRUPT;
                        goto out;
                }

                req_data_buf = __flash_req_to_buf(driver_req);
                memcpy(buf_pos, req_data_buf, req_data_size);

                remains -= req_data_size;
                buf_pos += req_data_size;
                driver_req->read.offset += req_data_size;
        }
out:
        ipc_destroy_msg(driver_msg);
        debug("lfs_sstflash_read -> %d\n", 0);
        return LFS_ERR_OK;
}

int lfs_sstflash_prog(const struct lfs_config *cfg, lfs_block_t block,
                      lfs_off_t off, const void *buffer, lfs_size_t size)
{
        int ret;
        const void *buf_pos;
        char *req_data_buf;
        lfs_size_t remains = size; 
        lfs_size_t driver_mtu = IPC_SHM_AVAILABLE - sizeof(struct flash_request);
        lfs_size_t req_data_size;
        struct flash_request *driver_req;
        ipc_msg_t *driver_msg;

        debug("lfs_sstflash_prog(%p, "
                "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")\n",
            (void*)cfg, block, off, buffer, size);

        driver_msg = ipc_create_msg(driver_icb, sizeof(struct flash_request));
        driver_req = (struct flash_request *)ipc_get_msg_data(driver_msg);
        
        driver_req->req = FLASH_REQ_WRITE;
        driver_req->write.block = block;
        driver_req->write.offset = off;

        buf_pos = buffer;
        while (remains) {
                req_data_size = lfs_min(remains, driver_mtu);
                
                driver_req->write.length = req_data_size;
                req_data_buf = __flash_req_to_buf(driver_req);
                memcpy(req_data_buf, buf_pos, req_data_size);

                ret = ipc_call(driver_icb, driver_msg);
                if (ret < 0) {
                        ret = LFS_ERR_CORRUPT;
                        goto out;
                }

                remains -= req_data_size;
                buf_pos += req_data_size;
                driver_req->write.offset += req_data_size;
        }
out:
        ipc_destroy_msg(driver_msg);
        debug("lfs_sstflash_prog -> %d\n", 0);
        return LFS_ERR_OK;
}

int lfs_sstflash_erase(const struct lfs_config *cfg, lfs_block_t block)
{
        int ret;
        ipc_msg_t *driver_msg;
        struct flash_request *driver_req;

        debug("lfs_sstflash_erase(%p, 0x%"PRIx32")\n", (void*)cfg, block);
        
        driver_msg = ipc_create_msg(driver_icb, sizeof(struct flash_request));
        driver_req = (struct flash_request*)ipc_get_msg_data(driver_msg);
        driver_req->req = FLASH_REQ_ERASE;
        driver_req->erase.block = block;
        driver_req->erase.count = 1;

        ret = (int)ipc_call(driver_icb, driver_msg);
        if (ret < 0) {
            ret = LFS_ERR_CORRUPT;
        }
        ipc_destroy_msg(driver_msg);
        debug("lfs_sstflash_erase -> %d\n", 0);
        return LFS_ERR_OK;
}

int lfs_sstflash_sync(const struct lfs_config *cfg)
{
        return LFS_ERR_OK;
}

// clang-format off
struct lfs_config bd_cfg = {
        .read = lfs_sstflash_read,
        .prog = lfs_sstflash_prog,
        .erase = lfs_sstflash_erase,
        .sync = lfs_sstflash_sync,
        .lock = lfs_lock,
        .unlock = lfs_unlock,

        .read_size = 64,
        .prog_size = 64,
        .block_size = 8192,
        .block_count = 768,
        .block_cycles = 500,
        .cache_size = 4096,
        .lookahead_size = 16
};
// clang-format on

int init_bd()
{
        int ret;
        unsigned int driver_cap;

        ipc_msg_t *proc_ipc_msg;
        struct proc_request *proc_req;

        proc_ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(proc_req->get_service_cap.service_name, SERVER_FLASH, SERVICE_NAME_LEN);

        ret = (int)ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        if (ret != 0) {
                goto out;
        }

        driver_cap = ipc_get_msg_cap(proc_ipc_msg, 0);

        driver_icb = ipc_register_client((int)driver_cap);

        if (!driver_icb) {
                ret = -EINVAL;
                goto out;
        }

out:
        ipc_destroy_msg(proc_ipc_msg);
        return ret;
}