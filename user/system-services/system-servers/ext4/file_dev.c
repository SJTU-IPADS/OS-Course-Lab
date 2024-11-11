/*
 * Copyright (c) 2013 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define DEBUG printf("[INFO] %s: %d\n", __FILE__, __LINE__)

#include "ext4_config.h"
#include "ext4_blockdev.h"
#include "ext4_errno.h"
#include "file_dev.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/sd_defs.h>
#include <chcore/ipc.h>
#include <chcore/services.h>

/**@brief   Default filename.*/
static const char *fname = "ext2";

/**@brief   Image block size.*/
#define EXT4_FILEDEV_BSIZE 512

/**@brief   Image file descriptor.*/
static FILE *dev_file;

/**@brief   Partition LBA.*/
uint64_t partition_lba;

#define DROP_LINUXCACHE_BUFFERS 0

/**********************BLOCKDEV INTERFACE**************************************/
static int file_dev_open(struct ext4_blockdev *bdev);
static int file_dev_bread(struct ext4_blockdev *bdev, void *buf,
			  uint64_t blk_id, uint32_t blk_cnt);
static int file_dev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			   uint64_t blk_id, uint32_t blk_cnt);
static int file_dev_close(struct ext4_blockdev *bdev);

/******************************************************************************/
EXT4_BLOCKDEV_STATIC_INSTANCE(file_dev, EXT4_FILEDEV_BSIZE, 0, file_dev_open,
			      file_dev_bread, file_dev_bwrite, file_dev_close,
			      0, 0);

/************************* sd_server operations *******************************/
/*
 * WARNING:
 * These structs have no concurrency protection. It is OK when we use register_cb_single.
 * But it might go wrong when we use register_cb. 
 */
/*
 * FIXME:
 * Add concurrency protection when use register_cb.
 */
static struct sd_msg *sd_req;
static ipc_msg_t *sd_ipc_msg;
static ipc_struct_t *sd_ipc_struct;

int sd_ipc_init(void)
{
	sd_ipc_struct = chcore_conn_srv(SERVER_SD_CARD);
	/*
	 * This message struct will be reused throughout the life cycle of this fs.
	 * So we do not destroy it now.
	 */
	sd_ipc_msg = ipc_create_msg(sd_ipc_struct, sizeof(struct sd_msg));
	sd_req = (struct sd_msg *)ipc_get_msg_data(sd_ipc_msg);

	return 0;
}

#define SD_BLOCK_SIZE 512
static int sd_read(void *buf, uint64_t lba)
{
	sd_req->req = SD_REQ_READ;
	sd_req->block_number = lba;
	sd_req->op_size = 512;
	ipc_call(sd_ipc_struct, sd_ipc_msg);

	memcpy(buf, sd_req->pbuffer, 512);

	return EOK;
}

static int sd_write(uint64_t lba, void *buf)
{
	sd_req->req = SD_REQ_WRITE;
	sd_req->block_number = lba;
	sd_req->op_size = 512;
	memcpy(sd_req->pbuffer, buf, 512);
	ipc_call(sd_ipc_struct, sd_ipc_msg);

	return EOK;
}

/******************************************************************************/
static int file_dev_open(struct ext4_blockdev *bdev)
{
	dev_file = malloc(512); // dummy dev_file

	file_dev.part_offset = 0;
	file_dev.part_size =
		INT64_MAX; // TODO: change to real partition size in sdcard
	file_dev.bdif->ph_bcnt = INT64_MAX; // TODO: part_size / 512

	sd_ipc_init();

	return EOK;
}

/******************************************************************************/

static int file_dev_bread(struct ext4_blockdev *bdev, void *buf,
			  uint64_t blk_id, uint32_t blk_cnt)
{
	if (!blk_cnt)
		return EOK;

	void *buf_ptr;
	uint64_t blk_id_ptr;
	int i;
	int r = EOK;
	for (i = 0, buf_ptr = buf, blk_id_ptr = blk_id; i < blk_cnt;
	     i++, buf_ptr += SD_BLOCK_SIZE, blk_id_ptr++) {
		r |= sd_read(buf_ptr, partition_lba + blk_id_ptr);
	}

	return r;
}

static void drop_cache(void)
{
#if defined(__linux__) && DROP_LINUXCACHE_BUFFERS
	int fd;
	char *data = "3";

	sync();
	fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	write(fd, data, sizeof(char));
	close(fd);
#endif
}

/******************************************************************************/
static int file_dev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			   uint64_t blk_id, uint32_t blk_cnt)
{
	if (!blk_cnt)
		return EOK;

	void *buf_ptr;
	uint64_t blk_id_ptr;
	int i;
	int r = EOK;

/*
	 * The argument 'buf' is const type, but we need to assign it to a temporal
	 * variable to split 'buf' into blocks. Using pragma to ignore warning.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
	for (i = 0, buf_ptr = buf, blk_id_ptr = blk_id; i < blk_cnt;
	     i++, buf_ptr += SD_BLOCK_SIZE, blk_id_ptr++) {
		r |= sd_write(partition_lba + blk_id_ptr, buf_ptr);
	}
#pragma GCC diagnostic pop

	drop_cache();
	return EOK;
}
/******************************************************************************/
static int file_dev_close(struct ext4_blockdev *bdev)
{
	return EOK;
}

/******************************************************************************/
struct ext4_blockdev *file_dev_get(void)
{
	return &file_dev;
}
/******************************************************************************/
void file_dev_name_set(const char *n)
{
	fname = n;
}
/******************************************************************************/
