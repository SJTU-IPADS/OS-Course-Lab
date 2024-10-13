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

#include "defs.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <chcore-internal/fs_defs.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/sd_defs.h>
#include <chcore/services.h>

#include "device.h"

#define UNUSED(x) (void)(x)

/*
 * partition informations
 */
static device_partition_t dparts[5];

/* lock to protect 'dparts' */
static pthread_mutex_t dparts_lock = PTHREAD_MUTEX_INITIALIZER;

/* whether fill the 'dparts' */
static bool dparts_inited = false;

static void initialize_dparts(void)
{
        memcpy(dparts[0].device_name, "sda1", 5);
        memcpy(dparts[1].device_name, "sda2", 5);
        memcpy(dparts[2].device_name, "sda3", 5);
        memcpy(dparts[3].device_name, "sda4", 5);
        memcpy(dparts[4].device_name, "sda5", 5);
}

/* Get sd_card's first block (mbr) */
static int sd_read_mbr(char *mbr)
{
        struct sd_msg *sd_req;
        ipc_msg_t *sd_ipc_msg;
        ipc_struct_t *sd_ipc_struct;

        sd_ipc_struct = chcore_conn_srv(SERVER_SD_CARD);
        if (sd_ipc_struct == NULL) {
                errno = ENODEV;
                return -1;
        }
        sd_ipc_msg = ipc_create_msg(sd_ipc_struct, sizeof(struct sd_msg));
        sd_req = (struct sd_msg *)ipc_get_msg_data(sd_ipc_msg);
        sd_req->req = SD_REQ_READ;
        sd_req->op_size = MBR_SIZE;
        sd_req->block_number = 0;
        ipc_call(sd_ipc_struct, sd_ipc_msg);

        /* fill mbr */
        memcpy(mbr, sd_req->pbuffer, MBR_SIZE);
        ipc_destroy_msg(sd_ipc_msg);

        return 0;
}

static int chcore_parse_devices(void)
{
        int i;
        char mbr[MBR_SIZE] = {0};

        /* check if 'dparts' ready */
        if (dparts_inited) {
                return 0;
        }

        /* init 'dparts' */
        initialize_dparts();

        /* fill 'mbr' from sd_card. mbr would remain all zeros if failed */
        if (sd_read_mbr(mbr) < 0) {
                printf("[WARNING] cannot connect to sd server\n");
        }

        /*
         * parse mbr entrys to get file system type,
         * note that fields in struct pinfo may be all zeros
         */
        partition_struct_t *pinfo;
        /**
         * Currently we let first MBR_MAX_PARTS_CNT dparts points
         * to MBR main partitions. And the last dparts is used for
         * littlefs.
         */
        for (i = 0; i < MBR_MAX_PARTS_CNT; i++) {
                pinfo = (partition_struct_t *)(mbr + SD_PARTITION_INFO_OFFSET
                                               + i * SD_PARTITION_INFO_SIZE);
                dparts[i].valid = pinfo->fs_id ? true : false;
                dparts[i].partition_index = i + 1;
                dparts[i].mounted = false;
                dparts[i].partition_lba = pinfo->lba;
                if (pinfo->fs_id == FAT32_PARTITION) {
                        /* FAT32 */
                        dparts[i].partition_type = FAT32_PARTITION;
                        strncpy(dparts[i].service_name, SERVER_FAT32_FS,
                                SERVICE_NAME_LEN);
                } else if (pinfo->fs_id == EXT4_PARTITION) {
                        /* EXT4 */
                        dparts[i].partition_type = EXT4_PARTITION;
                        strncpy(dparts[i].service_name, SERVER_EXT4_FS,
                                SERVICE_NAME_LEN);
                } else if (pinfo->fs_id == 0) {
                        /* do nothing */
                } else {
                        printf("[WARNING] not supported fs type %x\n",
                               pinfo->fs_id);
                }
        }

        if (chcore_conn_srv(SERVER_LITTLEFS) != NULL) {
                dparts[MAX_PARTS_CNT - 1].valid = true;
                dparts[MAX_PARTS_CNT - 1].partition_index = 5;
                dparts[MAX_PARTS_CNT - 1].mounted = false;
                dparts[MAX_PARTS_CNT - 1].partition_lba = 0;
                dparts[MAX_PARTS_CNT - 1].partition_type = LITTLEFS_PARTITION;
                strncpy(dparts[MAX_PARTS_CNT - 1].service_name, SERVER_LITTLEFS,
                        SERVICE_NAME_LEN);
        } else {
                printf("[WARNING] cannot connect to flash server\n");
                dparts[MAX_PARTS_CNT - 1].valid = false;
        }

        dparts_inited = true;
        return 0;
}

/*
 * @arg: 'device_name' is like 'sda1'...etc
 * @return: device_partition_t* dparts, 0 for any error
 */
device_partition_t *chcore_get_server(const char *device_name)
{
        int i;

        for (i = 0; i < MAX_PARTS_CNT; i++)
                if (strcmp(dparts[i].device_name, device_name) == 0)
                        return &(dparts[i]);

        printf("[debug %d] can't find server id\n", __LINE__);
        return 0;
}

/*
 * @arg: 'fs_cap' is fatfs server_cap, 'partition' is partition_index in device
 * @return: error code returned by fatfs server
 */
int chcore_initial_fat_partition(cap_t fs_cap, int partition)
{
        int ret;
        ipc_msg_t *ipc_msg;
        struct fs_request *fr_ptr;
        ipc_struct_t *ipc_struct = ipc_register_client(fs_cap);
        ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_MOUNT;
        fr_ptr->mount.paritition = partition;

        ipc_set_msg_data(ipc_msg, (char *)fr_ptr, 0, sizeof(struct fs_request));
        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

/*
 * Once start a ext4fs server, transform parititon offset to server using
 * FS_REQ_MOUNT
 * @arg: 'fs_cap' is ext4fs server_cap, 'partition_lba' is partition offset
 * @return: error code returned by ext4fs server
 */
int chcore_initial_ext4_partition(int fs_cap, uint64_t partition_lba)
{
        int ret;
        ipc_msg_t *ipc_msg;
        struct fs_request *fr_ptr;
        ipc_struct_t *ipc_struct = ipc_register_client(fs_cap);
        ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_MOUNT;
        fr_ptr->mount.offset = partition_lba; // Store partition_lba in offset

        ipc_set_msg_data(ipc_msg, (char *)fr_ptr, 0, sizeof(struct fs_request));
        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

int chcore_initial_littlefs_partition(int fs_cap, uint64_t partition_lba)
{
        return chcore_initial_ext4_partition(fs_cap, partition_lba);
}

/*
 * @arg: 'device_name' is like 'sda1'.. etc
 * @return: corresponding fs server cap
 */
int mount_storage_device(const char *device_name)
{
        struct proc_request *proc_req;
        cap_t fs_server_cap = 0;
        device_partition_t *fs_server;
        ipc_msg_t *proc_ipc_msg;

        /*
         * Refill 'dparts' to refresh partition_information
         * Write 'dparts' only first call 'chcore_parse_devices'
         */
        pthread_mutex_lock(&dparts_lock);
        if (chcore_parse_devices() < 0) {
                return -1;
        }
        print_devices();
        pthread_mutex_unlock(&dparts_lock);

        /* get corresponding fs server id */
        fs_server = chcore_get_server(device_name);
        if (fs_server == NULL || !fs_server->valid) {
                errno = EINVAL;
                return -1;
        }

        /* prepare structs to IPC call FSM */
        proc_ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(proc_req->get_service_cap.service_name, fs_server->service_name,
                SERVICE_NAME_LEN);
        proc_req->get_service_cap.service_name[SERVICE_NAME_LEN - 1] = '\0';
        ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        fs_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);
        ipc_destroy_msg(proc_ipc_msg);

        /* deal with different fs type */
        if (strncmp(fs_server->service_name, SERVER_FAT32_FS,
                    SERVICE_NAME_LEN) == 0) {
                /* FAT type */
                if (!fs_server->mounted) {
                        chcore_initial_fat_partition(
                                fs_server_cap, fs_server->partition_index);
                        fs_server->mounted = true;
                }
        } else if (strncmp(fs_server->service_name, SERVER_EXT4_FS,
                           SERVICE_NAME_LEN) == 0) {
                /* EXT4 type */
                if (!fs_server->mounted) {
                        chcore_initial_ext4_partition(fs_server_cap,
                                                      fs_server->partition_lba);
                }
        } else if (strncmp(fs_server->service_name, SERVER_LITTLEFS,
                           SERVICE_NAME_LEN) == 0) {
                /* LITTLEFS type */
                if (!fs_server->mounted) {
                        chcore_initial_littlefs_partition(
                                fs_server_cap, fs_server->partition_lba);
                }
        } else {
                /* others */
                printf("[WARNING] Not supported fs type \n");
        }

        return fs_server_cap;
}

void print_partition(partition_struct_t *p)
{
        printf("partition infomation:\n");

        /* parse boot */
        if (p->boot == 0x0)
                printf("  boot: no\n");
        else if (p->boot == 0x80)
                printf("  boot: yes\n");
        else
                printf("  boot: (unknown)\n");

        /* print fs id */
        printf("  file system id: 0x%x\n", p->fs_id);

        /* parse fs type */
        if (p->fs_id == EXT4_PARTITION)
                printf("  file system: Linux File System\n");
        else if (p->fs_id == FAT32_PARTITION)
                printf("  file system: W95 FAT32 (LBA)\n");
        else if (p->fs_id == 0)
                printf("  file system: Unused\n");
        else
                printf("  file system: (unknown)\n");

        /* parse partition lba */
        printf("  partition lba: 0x%x = %d\n", p->lba, p->lba);

        /* parse partition size */
        printf("  partition size: 0x%x = %d\n",
               p->total_sector,
               p->total_sector);
}

void print_devices(void)
{
        /* Now, only support sd device */
        int i;
        printf("[SDCARD]\n");
        printf("Device\tServer\tPartition\tType\n");
        for (i = 0; i < 5; i++)
                if (dparts[i].valid)
                        printf("%s\t%s\t%d\t%x\n",
                               dparts[i].device_name,
                               dparts[i].service_name,
                               dparts[i].partition_index,
                               dparts[i].partition_type);
}
