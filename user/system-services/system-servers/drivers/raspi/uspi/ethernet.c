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

#include "ethernet.h"

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <assert.h>

#include <chcore/type.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>
#include <chcore-internal/net_interface.h>
#include <chcore/pthread.h>

#include "uspi.h"
#include "uspios.h"
#include "uspienv/macros.h"
#include "uspi/lan7800.h"

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

static TLAN7800Device *lan7800;

static u8 mac_address[MAC_ADDRESS_SIZE];

static pthread_t eth_thread_tid;
static cap_t eth_thread_cap;

static void *eth_thread_func(void *arg);
DECLARE_SERVER_HANDLER(eth_ipc_handler);

void init_eth(void)
{
	extern TLAN7800Device *USPiGetLAN7800Device(void);
	lan7800 = USPiGetLAN7800Device();
	printf("Ethernet is available? %s\n", lan7800 ? "yes" : "no");

	if (lan7800) {
		eth_thread_cap = chcore_pthread_create(
			&eth_thread_tid, NULL, eth_thread_func, NULL);
		printf("Ethernet thread created, cap: %d\n", eth_thread_cap);
	}
}

static void *eth_thread_func(void *arg)
{
	int ret;
	struct lwip_request *lr;
	ipc_msg_t *ipc_msg;

	memcpy(mac_address,
	       LAN7800DeviceGetMACAddress(lan7800)->m_Address,
	       MAC_ADDRESS_SIZE);

	// register ipc server for access from lwip
	ret = ipc_register_server(eth_ipc_handler,
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

	TEthernetHeader *pEthHeader = (TEthernetHeader *)data;
	if (pEthHeader->nProtocolType != BE(ETH_PROT_ARP)
	    && memcmp(pEthHeader->MACReceiver, mac_address, MAC_ADDRESS_SIZE)
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
		while (!LAN7800DeviceIsLinkUp(lan7800)) {
			MsDelay(100);
			if (--timeout > 0)
				continue;
			ret = NET_DRIVER_RET_LINK_DOWN;
			break;
		}
		break;
	}
	case NET_DRIVER_RECEIVE_FRAME: {
		// printf("NET_DRIVER_RECEIVE_FRAME\n");
		assert(NET_DRIVER_DATA_LEN >= USPI_FRAME_BUFFER_SIZE);
		unsigned len;
		while (!LAN7800DeviceWaitReceiveFrame(lan7800, ndr->data, &len)
		       || !filter_frame(ndr->data, len))
			;
		ndr->args[0] = len;
		break;
	}
	case NET_DRIVER_SEND_FRAME: {
		// printf("NET_DRIVER_SEND_FRAME\n");
		unsigned len = ndr->args[0];
		if (!LAN7800DeviceSendFrame(lan7800, ndr->data, len))
			ret = NET_DRIVER_RET_SEND_FAILED;
		break;
	}
	default:
		break;
	}
	ipc_return(ipc_msg, ret);
}
