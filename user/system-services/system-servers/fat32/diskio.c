/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/sd_defs.h>
#include <chcore/ipc.h>
#include <chcore/services.h>
#include <stdio.h>
#include <string.h>
#include "ff.h" /* Obtains integer types */
#include "diskio.h" /* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_RAM 0 /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC 1 /* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB 2 /* Example: Map USB MSD to physical drive 2 */

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

unsigned char buffer[1024];

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

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, /* Physical drive nmuber to identify the drive */
		  BYTE *buff, /* Data buffer to store read data */
		  LBA_t sector, /* Start sector in LBA */
		  UINT count /* Number of sectors to read */
)
{
	unsigned int left = count;
	unsigned int nsector;
	unsigned int read_sector;

	read_sector = 0;
	while (1) {
		nsector = left > 6 ? 6 : left;
		sd_req->req = SD_REQ_READ;
		sd_req->block_number = (int)sector;
		sd_req->op_size = (int)nsector * 512;
		ipc_call(sd_ipc_struct, sd_ipc_msg);
		memcpy(buff + read_sector * 512, sd_req->pbuffer, sd_req->op_size);
		read_sector += nsector;
		left -= nsector;
		if (left == 0) {
			goto out;
		}
		sector += nsector;
	}

out:
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
		   const BYTE *buff, /* Data to be written */
		   LBA_t sector, /* Start sector in LBA */
		   UINT count /* Number of sectors to write */
)
{
	unsigned int left = count;
	unsigned int nsector;
	unsigned int write_sector;

	write_sector = 0;
	while (1) {
		nsector = left > 6 ? 6 : left;
		sd_req->req = SD_REQ_WRITE;
		sd_req->block_number = (int)sector;
		sd_req->op_size = (int)nsector * 512;
		memcpy(sd_req->pbuffer, buff + write_sector * 512, sd_req->op_size);
		ipc_call(sd_ipc_struct, sd_ipc_msg);
		left -= nsector;
		write_sector += nsector;
		if (left == 0) {
			goto out;
		}
		sector += nsector;
	}

out:
	return RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
		   BYTE cmd, /* Control code */
		   void *buff /* Buffer to send/receive control data */
)
{
	return RES_OK;
}
