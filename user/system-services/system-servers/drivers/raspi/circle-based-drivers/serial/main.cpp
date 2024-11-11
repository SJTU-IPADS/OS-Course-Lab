#include <circle/machineinfo.h>
#include <circle/bcm2835.h>
#include <circle/interrupt.h>
#include <circle/serial.h>
#include <stdio.h>
#include <circle/devicenameservice.h>
#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore-internal/serial_defs.h>
#include <sched.h>
#include <stdlib.h>
#include "../circle.hpp"

struct SerialDriver {
	CSerialDevice serial;

	SerialDriver()
		: serial(CInterruptSystem::Get())
	{
	}

	bool initialize()
	{
		bool bOK = true;
		if (bOK){
			bOK = serial.Initialize(115200);
		}
		
		return bOK;
	}

	static SerialDriver *get()
	{
		static SerialDriver *driver = nullptr;
		if (!driver)
			driver = new SerialDriver;
		return driver;
	}
};

DEFINE_SERVER_HANDLER(ipc_dispatch_serial)
{
	struct serial_request *msg;
	int ret = 0;

	msg = (struct serial_request *)ipc_get_msg_data(ipc_msg);
	switch (msg->serial_req) {
	case READ:
		ret = SerialDriver::get()->serial.Read(
			(u8 *)msg->rw_struct.data, msg->rw_struct.size);
		break;
	case WRITE:
		ret = SerialDriver::get()->serial.Write(
			(u8 *)msg->rw_struct.data, msg->rw_struct.size);
		break;
	default:
		ret = -1;
		printf("Incorrect sd req! Please retry!");
	}
	ipc_return(ipc_msg, ret);
}

void* serial(void* args)
{
	int *thread_ret = (int*)malloc(sizeof(int));
	*thread_ret = 0;
	if (!SerialDriver::get()->initialize()) {
		*thread_ret = -EIO;
		return thread_ret;
	}

	int ret = ipc_register_server(
		reinterpret_cast<server_handler>(ipc_dispatch_serial),
		register_cb_single);

	*thread_ret = ret;
	return thread_ret;
}
