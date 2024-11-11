#include <circle/machineinfo.h>
#include <circle/bcm2711.h>
#include <circle/logger.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/bcm54213.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/ipc.h>
#include <chcore/pthread.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore-internal/net_interface.h>

#include "../circle.hpp"

// TODO: too much duplicated code from uspi/ethernet.c
// maybe should move to chcore-internal/net_interface.h

#define MAC_ADDRESS_SIZE 6
#define IP_ADDRESS_SIZE  4

typedef struct TEthernetHeader {
	u8 MACReceiver[MAC_ADDRESS_SIZE];
	u8 MACSender[MAC_ADDRESS_SIZE];
	u16 nProtocolType;
#define ETH_PROT_ARP 0x806
} PACKED TEthernetHeader;

typedef struct TARPPacket {
	u16 nHWAddressSpace;
#define HW_ADDR_ETHER 1
	u16 nProtocolAddressSpace;
#define PROT_ADDR_IP 0x800
	u8 nHWAddressLength;
	u8 nProtocolAddressLength;
	u16 nOPCode;
#define ARP_REQUEST 1
#define ARP_REPLY   2
	u8 HWAddressSender[MAC_ADDRESS_SIZE];
	u8 ProtocolAddressSender[IP_ADDRESS_SIZE];
	u8 HWAddressTarget[MAC_ADDRESS_SIZE];
	u8 ProtocolAddressTarget[IP_ADDRESS_SIZE];
} PACKED TARPPacket;

typedef struct TARPFrame {
	TEthernetHeader Ethernet;
	TARPPacket ARP;
} PACKED TARPFrame;

static CBcm54213Device *bcm54213 = nullptr;

static u8 mac_address[MAC_ADDRESS_SIZE];

static pthread_t eth_thread_tid;
static cap_t eth_thread_cap;

static void *eth_thread_func(void *arg);
DECLARE_SERVER_HANDLER(eth_ipc_handler);

static void init_io_mapping_bcm54213()
{
	
	int ret;
	cap_t bcm54213_pmo_cap = usys_create_device_pmo(
		ARM_BCM54213_BASE,
		ROUND_UP(ARM_BCM54213_END - ARM_BCM54213_BASE, PAGE_SIZE));
	ret = usys_map_pmo(SELF_CAP,
			   bcm54213_pmo_cap,
			   ARM_BCM54213_BASE,
			   VM_READ | VM_WRITE);
	BUG_ON(ret < 0);
}

struct Bcm54213Driver {
	/* do not change this order */
	CBcm54213Device bcm54213;

	bool initialize()
	{
		bool ok = true;
		if (ok)
			ok = bcm54213.Initialize();
		return ok;
	}

	static Bcm54213Driver *get()
	{
		static Bcm54213Driver *driver = nullptr;
		if (!driver)
			driver = new Bcm54213Driver;
		return driver;
	}
};

static void *eth_thread_func(void *arg)
{
	int ret;
	struct lwip_request *lr;
	ipc_msg_t *ipc_msg;

	auto mac = bcm54213->GetMACAddress();
	mac->CopyTo(mac_address);

	// register ipc server for access from lwip
	ret = ipc_register_server(
		reinterpret_cast<server_handler>(eth_ipc_handler),
		DEFAULT_CLIENT_REGISTER_HANDLER);
	if (ret < 0) {
		WARN("Ethernet thread register IPC server failed");
		return 0;
	}

	// call lwip to add an ethernet interface
	ipc_msg =
		ipc_create_msg_with_cap(lwip_ipc_struct, sizeof(struct lwip_request), 1);
	lr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
	lr->req = LWIP_INTERFACE_ADD;
	// configure interface type and MAC address
	struct net_interface *intf = (struct net_interface *)lr->data;
	intf->type = NET_INTERFACE_ETHERNET;
	memcpy(intf->mac_address, mac_address, MAC_ADDRESS_SIZE);
	// give lwip the thread cap, so it can ipc call (poll) us
	ipc_set_msg_cap(ipc_msg, 0, eth_thread_cap);
	// do the call
	ret = ipc_call(lwip_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);
	if (ret < 0) {
		WARN("Call LWIP.LWIP_INTERFACE_ADD failed");
		return 0;
	}

	while (1) {
		usys_yield();
	}
}

static bool filter_frame(void *data, unsigned len)
{
	if (len < sizeof(TEthernetHeader))
		return false;

	TEthernetHeader *eth_header = (TEthernetHeader *)data;
	if (eth_header->nProtocolType != BE(ETH_PROT_ARP)
	    && memcmp(eth_header->MACReceiver, mac_address, MAC_ADDRESS_SIZE)
		       != 0) {
		// neither an ARP frame, nor a frame for me
		return false;
	}
	return true;
}

DEFINE_SERVER_HANDLER(eth_ipc_handler)
{
	int ret = 0;
	struct net_driver_request *ndr =
		(struct net_driver_request *)ipc_get_msg_data(ipc_msg);
	switch (ndr->req) {
	case NET_DRIVER_WAIT_LINK_UP: {
		// printf("NET_DRIVER_WAIT_LINK_UP\n");
		unsigned timeout = 40;
		while (!bcm54213->IsLinkUp()) {
			CTimer::Get()->SimpleMsDelay(100);
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
		while (!bcm54213->ReceiveFrame(ndr->data, &len)
		       || !filter_frame(ndr->data, len))
			;
		ndr->args[0] = len;
		break;
	}
	case NET_DRIVER_SEND_FRAME: {
		// printf("NET_DRIVER_SEND_FRAME\n");
		unsigned len = ndr->args[0];
		if (!bcm54213->SendFrame(ndr->data, len))
			ret = NET_DRIVER_RET_SEND_FAILED;
		break;
	}
	default:
		break;
	}
	ipc_return(ipc_msg, ret);
}

__attribute__((unused)) static void debug_hexdump(const void *pStart, unsigned nBytes,
			  const char *pSource)
{
	u8 *pOffset = (u8 *)pStart;

	CLogger::Get()->Write(pSource,
			      LogDebug,
			      "Dumping 0x%X bytes starting at 0x%lX",
			      nBytes,
			      (unsigned long)(uintptr)pOffset);

	while (nBytes > 0) {
		CLogger::Get()->Write(
			pSource,
			LogDebug,
			"%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
			(unsigned)(uintptr)pOffset & 0xFFFF,
			(unsigned)pOffset[0],
			(unsigned)pOffset[1],
			(unsigned)pOffset[2],
			(unsigned)pOffset[3],
			(unsigned)pOffset[4],
			(unsigned)pOffset[5],
			(unsigned)pOffset[6],
			(unsigned)pOffset[7],
			(unsigned)pOffset[8],
			(unsigned)pOffset[9],
			(unsigned)pOffset[10],
			(unsigned)pOffset[11],
			(unsigned)pOffset[12],
			(unsigned)pOffset[13],
			(unsigned)pOffset[14],
			(unsigned)pOffset[15]);

		pOffset += 16;

		if (nBytes >= 16) {
			nBytes -= 16;
		} else {
			nBytes = 0;
		}
	}
}

int bcm54213_start()
{
	init_io_mapping_bcm54213();
	Bcm54213Driver::get()->initialize();
	bcm54213 = &Bcm54213Driver::get()->bcm54213;

	eth_thread_cap = chcore_pthread_create(
		&eth_thread_tid, NULL, eth_thread_func, NULL);
	info("Ethernet thread created, cap: %d\n", eth_thread_cap);


	return 0;
}
