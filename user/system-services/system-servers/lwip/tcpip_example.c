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
#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <chcore/pmu.h>
#include <chcore/string.h>

#include <lwip/debug.h>
#include <lwip/init.h>

#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>

#include <lwip/ip.h>
#include <lwip/ip4_frag.h>
#include <lwip/stats.h>
#include <lwip/tcp.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <netif/etharp.h>
#include <lwip/sockets.h>

#include <lwip/apps/snmp.h>
#include <lwip/apps/snmp_mib2.h>
#include <pthread.h>


/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;
#ifdef LWIP_SNMP
#undef LWIP_SNMP
#endif
#define LWIP_SNMP 0
#if LWIP_SNMP
/* SNMP trap destination cmd option */
static ip_addr_t trap_addr;

static const struct snmp_mib *mibs[] = {&mib2, &mib_private};
#endif

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

#if LWIP_SNMP
/* enable == 1, disable == 2 */
u8_t snmpauthentraps_set = 2;
#endif

#define CLIENT_PORT_NUM 6002
#define CLIENT_IP_ADDR "192.168.0.3"

#define SERVER_PORT_NUM 6001
#define SERVER_IP_ADDR "192.168.0.3"

void *client_routine(void *arg)
{
	int socket_fd;
	struct sockaddr_in sa, ra;

	int recv_data;
	char data_buffer[80];	/* Creates an TCP lwip_socket (SOCK_STREAM) with Internet Protocol Family (PF_INET).
				 * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
				 */

	for (int i = 0; i < 10; i ++) {
		socket_fd = lwip_socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd < 0) {
			printf("lwip_socket call failed");
			usys_exit(0);
		}

		printf("[CLIENT] socket fd %d\n", socket_fd);

		memset(&sa, 0, sizeof(struct sockaddr));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr(CLIENT_IP_ADDR);
		sa.sin_port = htons(CLIENT_PORT_NUM + i);

		/* lwip_bind the TCP lwip_socket to the port SENDER_PORT_NUM and to the current
		* machines IP address (Its defined by SENDER_IP_ADDR).
		* Once lwip_bind is successful for UDP sockets application can operate
		* on the lwip_socket descriptor for sending or receiving data.
		*/
		if (lwip_bind(socket_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in))
		== -1) {
			printf("lwip_bind to Port Number %d ,IP address %s failed\n",
			CLIENT_PORT_NUM, CLIENT_IP_ADDR);
			lwip_close(socket_fd);
			usys_exit(1);
		}
		/* Receiver connects to server ip-address. */

		printf("[CLEINT] create socket sucess %d\n", socket_fd);

		memset(&ra, 0, sizeof(struct sockaddr_in));
		ra.sin_family = AF_INET;
		ra.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
		ra.sin_port = htons(SERVER_PORT_NUM);

		usys_yield();

		printf("[CLEINT] will call connect\n");

		if (lwip_connect(socket_fd, (struct sockaddr *)&ra, sizeof(struct sockaddr_in)) < 0) {
			printf("lwip_connect failed \n");
			lwip_close(socket_fd);
			usys_exit(2);
		}

		printf("[CLEINT] lwip connect return\n");

		recv_data = lwip_recv(socket_fd, data_buffer, sizeof(data_buffer), 0);
		if (recv_data < 0) {

			printf("lwip_recv failed \n");
			lwip_close(socket_fd);
			usys_exit(2);
		}
		data_buffer[recv_data] = '\0';
		printf("[CLEINT] received data: %s\n", data_buffer);

		lwip_close(socket_fd);
	}

	usys_exit(0);
	return NULL;
}


void *server_routine(void *arg)
{
	int socket_fd, accept_fd;
	int sent_data;
	socklen_t addr_size;
	char data_buffer[80];
	struct sockaddr_in sa, isa;

	/* Creates an TCP lwip_socket (SOCK_STREAM) with Internet Protocol Family (PF_INET).
	 * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
	 */

	socket_fd = lwip_socket(PF_INET, SOCK_STREAM, 0);

	if (socket_fd < 0) {

		printf("lwip_socket call failed");
		usys_exit(0);
	}

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
	sa.sin_port = htons(SERVER_PORT_NUM);

	/* lwip_bind the TCP lwip_socket to the port SENDER_PORT_NUM and to the current
	 * machines IP address (Its defined by SENDER_IP_ADDR).
	 * Once lwip_bind is successful for UDP sockets application can operate
	 * on the lwip_socket descriptor for sending or receiving data.
	 */
	if (lwip_bind(socket_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		printf("lwip_bind to Port Number %d ,IP address %s failed\n",
		       SERVER_PORT_NUM, SERVER_IP_ADDR);
		lwip_close(socket_fd);
		usys_exit(1);
	}

	printf("[SERVER] create socket sucess %d\n", socket_fd);
	lwip_listen(socket_fd, 5);
	printf("[SERVER] listen return \n");

	while (1) {
		addr_size = sizeof(isa);
		accept_fd = lwip_accept(socket_fd, (struct sockaddr *)&isa, &addr_size);
		printf("[SERVER] accept %d\n", accept_fd);
		if (accept_fd < 0) {

			printf("lwip_accept failed\n");
			lwip_close(socket_fd);
			usys_exit(2);
		}
		strcpy(data_buffer, "Hello World\n");
		sent_data = lwip_send(accept_fd, data_buffer, sizeof("Hello World"), 0);

		if (sent_data < 0) {
			printf("lwip_send failed\n");
			lwip_close(socket_fd);
			usys_exit(3);
		}
		lwip_close(accept_fd);
	}

	lwip_close(socket_fd);

	usys_exit(0);
	return NULL;
}

err_t netif_loopif_init(struct netif *netif);

int main(int argc, char **argv)
{
	struct netif netif;
	char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};
	pthread_t client, server;

	/* startup defaults (may be overridden by one or more opts) */
	IP4_ADDR(&gw, 192, 168, 0, 1);
	IP4_ADDR(&ipaddr, 192, 168, 0, 3);
	IP4_ADDR(&netmask, 255, 255, 255, 0);

	/* use debug flags defined by debug.h */
	debug_flags = LWIP_DBG_OFF;

	debug_flags |= (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE |
			LWIP_DBG_FRESH | LWIP_DBG_HALT);

	strlcpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
	strlcpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
	strlcpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
	printf("[LWIP] Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);

	/* Initialize */
	tcpip_init(NULL, NULL);
	printf("[LWIP] TCP/IP initialized.\n");


	netif_add(&netif, &ipaddr, &netmask, &gw, NULL, netif_loopif_init, tcpip_input);
	netif_set_link_up(&netif);
	netif_set_default(&netif);
	netif_set_up(&netif);
	printf("[LWIP] Add netif %p\n", &netif);

	pthread_create(&server, NULL, server_routine, NULL);
	pthread_create(&client, NULL, client_routine, NULL);
	
	pthread_join(server, NULL);
	pthread_join(client, NULL);

	usys_exit(0);
	return 0;
}
