#include <circle/machineinfo.h>
#include <circle/bcm2835.h>
#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <circle/net/netsubsystem.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>

#include <stdio.h>
#include <sys/mount.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/memory.h>
#include <chcore/ipc.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore-internal/net_interface.h>
#include <chcore/pthread.h>

#include "../circle.hpp" 

#define OPEN_NET "ChCore-WiFi"

#define FIRMWARE_PATH "/wlan/firmware/" // firmware files must be provided here
#define CONFIG_FILE   "/wlan/wpa_supplicant.conf"

struct WlanDriver {
	/* do not change this order */

	CBcm4343Device wlan;
	CWPASupplicant wpa_supplicant;

	WlanDriver():
		wlan(FIRMWARE_PATH)
		, wpa_supplicant(CONFIG_FILE)
	{
	}

	bool initialize()
	{
		bool ok = true;
		if (ok){
			ok = wlan.Initialize();
		}
		// if (ok)
		// 	ok = wlan.JoinOpenNet(OPEN_NET);
		if (ok){
			ok = wpa_supplicant.Initialize();
		}
		return ok;
	}

	static WlanDriver *get()
	{
		static WlanDriver *driver = nullptr;
		if (!driver){
			driver = new WlanDriver;
		}
		return driver;
	}
};

static CNetDevice *wlan;

static u8 mac_address[MAC_ADDRESS_SIZE];

DEFINE_SERVER_HANDLER(ipc_dispatch_wlan)
{
	int ret = 0;
	struct net_driver_request *ndr =
		(struct net_driver_request *)ipc_get_msg_data(ipc_msg);
	switch (ndr->req) {
	case NET_DRIVER_WAIT_LINK_UP: {
		// printf("NET_DRIVER_WAIT_LINK_UP\n");
		unsigned timeout = 40;
		while (!wlan->IsLinkUp()) {
			CTimer::SimpleMsDelay(100);
			if (--timeout > 0)
				continue;
			ret = NET_DRIVER_RET_LINK_DOWN;
			break;
		}
		break;
	}
	case NET_DRIVER_RECEIVE_FRAME: {
		// printf("NET_DRIVER_RECEIVE_FRAME\n");
		assert(NET_DRIVER_DATA_LEN >= FRAME_BUFFER_SIZE);
		unsigned len;
		while (!wlan->ReceiveFrame(ndr->data, &len))
			;
		ndr->args[0] = len;
		break;
	}
	case NET_DRIVER_SEND_FRAME: {
		// printf("NET_DRIVER_SEND_FRAME\n");
		unsigned len = ndr->args[0];
		if (!wlan->SendFrame(ndr->data, len))
			ret = NET_DRIVER_RET_SEND_FAILED;
		break;
	}
	default:
		break;
	}
	ipc_return(ipc_msg, ret);
}

static pthread_t ipc_server_thread_tid;
static cap_t ipc_server_thread_cap;

static void *ipc_server_thread_func(void *arg)
{
	int ret;
	struct lwip_request *lr;
	ipc_msg_t *ipc_msg;

	memcpy(mac_address, wlan->GetMACAddress()->Get(), MAC_ADDRESS_SIZE);

	// register ipc server for access from lwip
	ret = ipc_register_server(
		reinterpret_cast<server_handler>(ipc_dispatch_wlan),
		DEFAULT_CLIENT_REGISTER_HANDLER);
	if (ret < 0) {
		WARN("WLAN driver register IPC server failed");
		return 0;
	}

	// call lwip to add an ethernet interface
	ipc_msg =
		ipc_create_msg_with_cap(lwip_ipc_struct, sizeof(struct lwip_request), 1);
	lr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
	lr->req = LWIP_INTERFACE_ADD;
	// configure interface type and MAC address
	struct net_interface *intf = (struct net_interface *)lr->data;
	intf->type = NET_INTERFACE_WLAN;
	memcpy(intf->mac_address, mac_address, MAC_ADDRESS_SIZE);
	// give lwip the thread cap, so it can ipc call (poll) us
	ipc_set_msg_cap(ipc_msg, 0, ipc_server_thread_cap);
	// do the call
	ret = ipc_call(lwip_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);
	if (ret < 0) {
		WARN("Call LWIP.LWIP_INTERFACE_ADD failed");
		return 0;
	}

	usys_wait(usys_create_notifc(), true, NULL);
	return NULL;
}

int wlan_start()
{

	if (!WlanDriver::get()->initialize()) {
		error("WLAN driver failed to initialize\n");
	}

	wlan = CNetDevice::GetNetDevice(NetDeviceTypeWLAN);
	info("WLAN is available? %s\n", wlan ? "yes" : "no");

	if (wlan) {
		ipc_server_thread_cap =
			chcore_pthread_create(&ipc_server_thread_tid,
					      NULL,
					      ipc_server_thread_func,
					      NULL);
		info("WLAN ipc server thread created, cap: %d\n",
		       ipc_server_thread_cap);
	}

	return 0;
}
