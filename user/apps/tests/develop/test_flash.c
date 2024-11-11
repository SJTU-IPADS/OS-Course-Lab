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

#include <stdio.h>
#include <sys/mman.h>
#include <chcore/ipc.h>
#include <chcore/memory.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/flash_defs.h>

#include "test_tools.h"

int main()
{
	int flash_server_cap, ret;
	struct proc_request *proc_req;
	ipc_msg_t *proc_ipc_msg, *flash_ipc_msg;
	ipc_struct_t *flash_ipc_struct;
	struct flash_request *flash_req;
	unsigned char *buf;

	info("test flash beginning...\n");

	proc_ipc_msg = ipc_create_msg(
		procmgr_ipc_struct, sizeof(struct proc_request));
	proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
	proc_req->req = PROC_REQ_GET_SERVICE_CAP;
	strncpy(proc_req->get_service_cap.service_name, SERVER_FLASH, SERVICE_NAME_LEN);

	ret = ipc_call(procmgr_ipc_struct, proc_ipc_msg);
	if (ret < 0) {
		error("cannot connect to flash driver, ret = %d\n", ret);
		return ret;
	}
	flash_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);
	ipc_destroy_msg(proc_ipc_msg);

	flash_ipc_struct = ipc_register_client(flash_server_cap);
	flash_ipc_msg = ipc_create_msg(flash_ipc_struct,
		sizeof(struct flash_request));
	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_ERASE;
	flash_req->erase.block = 0;
	flash_req->erase.count = 1;
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash erase failed!");

	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_WRITE;
	flash_req->write.block = 0;
	flash_req->write.offset = 0;
	flash_req->write.length = 1024;
	buf = (unsigned char *)((void *)flash_req + sizeof(struct flash_request));
	for (int i = 0; i < 1024; i++) {
		*buf = i % 256;
		buf++;
	}
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash write failed!");

	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_READ;
	flash_req->write.block = 0;
	flash_req->write.offset = 0;
	flash_req->write.length = 1024;
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash read failed!");
	buf = (unsigned char *)((void *)flash_req + sizeof(struct flash_request));
	for (int i = 0; i < 1024; i++) {
		if (*buf != i % 256) {
			error("read error: %u should be 0x%x, but read 0x%x\n",
				i, i % 256, (unsigned)*buf);
			exit(1);
		}
		buf++;
	}

	info("test flash phase 1 finished\n");

	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_ERASE;
	flash_req->erase.block = 10;
	flash_req->erase.count = 2;
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash erase failed!");

	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_WRITE;
	flash_req->write.block = 11;
	flash_req->write.offset = 1024;
	flash_req->write.length = 1024;
	buf = (unsigned char *)((void *)flash_req + sizeof(struct flash_request));
	for (int i = 0; i < 1024; i++) {
		*buf = i % 128;
		buf++;
	}
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash write failed!");

	flash_req = (struct flash_request *)ipc_get_msg_data(flash_ipc_msg);
	flash_req->req = FLASH_REQ_READ;
	flash_req->write.block = 11;
	flash_req->write.offset = 1024;
	flash_req->write.length = 1024;
	ret = ipc_call(flash_ipc_struct, flash_ipc_msg);
	chcore_assert(ret == 0, "flash read failed!");
	buf = (unsigned char *)((void *)flash_req + sizeof(struct flash_request));
	for (int i = 0; i < 1024; i++) {
		if (*buf != i % 128) {
			error("read error: %u should be 0x%x, but read 0x%x\n",
				i, i % 256, (unsigned)*buf);
			break;
		}
		buf++;
	}

	info("test flash phase 2 finished\n");

	ipc_destroy_msg(flash_ipc_msg);
	info("test flash succeeded\n");
	return 0;
}