#include <circle/machineinfo.h>
#include <circle/bcm2835.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/devicenameservice.h>
#include <SDCard/chcore/sdhostdevice.h>
#include <SDCard/chcore/emmcdevice.h>

#include <stdio.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/sd_defs.h>
#include <chcore/bug.h>

#include "config.h"
#include "block_cache.h"
#include "../circle.hpp"

#if SD_SERVER_DEBUG >= 1
#define sd_srv_dbg_base(fmt, ...) \
	printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define sd_srv_dbg_base(fmt, ...) \
	do {                      \
	} while (0)
#endif

#if SD_SERVER_DEBUG >= 2
#define sd_srv_dbg(fmt, ...) \
	printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define sd_srv_dbg(fmt, ...) \
	do {                 \
	} while (0)
#endif

/*---------------------------------------------------------------------*/

#if SD_ACCESS_PATTERN == 1
#define sd_srv_dbg_pattern(fmt, ...) \
	printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define sd_srv_dbg_pattern(fmt, ...) \
	do {                         \
	} while (0)
#endif

/*---------------------------------------------------------------------*/

struct SdDriver {
	/* do not change this order */
#if RASPPI == 3
	CSDHostDevice sd;
	// CEMMCDevice sd; // if use EMMC, WLAN can't be used
#elif RASPPI == 4
	CEMMCDevice sd; // this will use EMMC2, no conflict with WLAN
#endif

	SdDriver()
		: sd(CInterruptSystem::Get(), CTimer::Get())
	{

	}

	bool initialize()
	{
		bool bOK = true;
		if (bOK)
			bOK = sd.Initialize();
		return bOK;
	}

	static SdDriver *get()
	{
		static SdDriver *driver = nullptr;
		if (!driver)
			driver = new SdDriver;
		return driver;
	}
};

extern "C" {
/*
 * `sd_device_read` is function pointer acquired by `block_cache` module
 * `buf` and `lba` means Buffer and Logical Block Address corresponding
 * `read_curry` is argument acquired by `block_cache` to deal with various
 * 	`device_read` argument for other driver, but is not used in sd driver
 */
int sd_device_read(char *buf, int lba, void *read_curry)
{
	return SdDriver::get()->sd.DoRead((u8 *)buf, BLOCK_SIZE, lba);
}

/*
 * Similar with `sd_device_write`. Read comment of `sd_device_read` for help
 */
int sd_device_write(char *buf, int lba, void *write_curry)
{
	return SdDriver::get()->sd.DoWrite((u8 *)buf, BLOCK_SIZE, lba);
}
}

struct storage_block_cache sd_block_cache;

int do_read(struct sd_msg *msg)
{
	int i;
	sd_srv_dbg_pattern("R %d\n", msg->block_number);

	BUG_ON(msg->op_size < 0 || (size_t)msg->op_size > sizeof(msg->pbuffer));
	BUG_ON((msg->op_size % BLOCK_SIZE) != 0);

	for (i = 0; i < msg->op_size / BLOCK_SIZE; ++i) {
		bc_read(&sd_block_cache, msg->block_number + i, msg->pbuffer + BLOCK_SIZE * i);
	}

	return BLOCK_SIZE;
}

int do_write(struct sd_msg *msg)
{
	int i;
	sd_srv_dbg_pattern("W %d\n", msg->block_number);

	BUG_ON(msg->op_size < 0 || (size_t)msg->op_size > sizeof(msg->pbuffer));
	BUG_ON((msg->op_size % BLOCK_SIZE) != 0);

	for (i = 0; i < msg->op_size / BLOCK_SIZE; ++i) {
		bc_write(&sd_block_cache, msg->pbuffer + BLOCK_SIZE * i, msg->block_number + i);
	}

	return BLOCK_SIZE;
}

DEFINE_SERVER_HANDLER(ipc_dispatch_sd)
{
	struct sd_msg *msg;
	int ret = 0;

	msg = (struct sd_msg *)ipc_get_msg_data(ipc_msg);
	switch (msg->req) {
	case SD_REQ_READ:
		ret = do_read(msg);
		break;
	case SD_REQ_WRITE:
		ret = do_write(msg);
		break;
	default:
		ret = -1;
		printf("Incorrect sd req! Please retry!");
	}
	ipc_return(ipc_msg, ret);
}


void* sdcard(void* args)
{
	int ret;
	int *thread_ret;
	thread_ret = (int*)malloc(sizeof(int));
	*thread_ret = 0;
	if (!SdDriver::get()->initialize()) {
		*thread_ret = -EIO;
		return thread_ret;
	}

	/* Initialize SD storage_block_cache */
	bc_init(&sd_block_cache, sd_device_read, sd_device_write);
	ret = ipc_register_server(
		reinterpret_cast<server_handler>(ipc_dispatch_sd),
		register_cb_single);
	if (ret < 0) {
		*thread_ret = ret;
	}

	return thread_ret;
}
