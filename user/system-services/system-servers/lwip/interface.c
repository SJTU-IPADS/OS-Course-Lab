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
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>

#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore-internal/net_interface.h>
#include <chcore/bug.h>
#include <chcore/string.h>

#include <lwip/debug.h>
#include <lwip/init.h>

#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>

#include <lwip/ip.h>
#include <lwip/stats.h>
#include <lwip/tcp.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <netif/etharp.h>

#define LWIP_SRC /* needed by lwip_def */
#include <chcore-internal/lwip_defs.h>

#define PREFIX "[lwip]"

#define info(fmt, ...)  printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define debug(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define STATIC_IPADDR   "192.168.22.2"
#define STATIC_NETMASK  "255.255.255.0"
#define STATIC_GATEWAY  "192.168.22.1"

struct netif_state {
        u8 mac_address[6];
        cap_t driver_thread_cap;
        ipc_struct_t *driver_in_ipc_struct;
        ipc_struct_t *driver_out_ipc_struct;
        pthread_t poll_thread_tid;
};

static err_t eth_init(struct netif *netif);
static err_t wlan_init(struct netif *netif);
static err_t net_link_output(struct netif *netif, struct pbuf *p);
static void *net_poll_thread_func(void *arg);

#ifdef CHCORE_STATIC_IP
static ip4_addr_t ipaddr, netmask, gateway;
#endif

int add_interface(ipc_msg_t *ipc_msg, struct lwip_request *lr)
{
        cap_t driver_thread_cap = ipc_get_msg_cap(ipc_msg, 0);
        struct net_interface *intf = (struct net_interface *)lr->data;

        if (driver_thread_cap <= 0) {
                error("driver_thread_cap %d is invalid", driver_thread_cap);
                return -1;
        }

        switch (intf->type) {
        case NET_INTERFACE_ETHERNET:
        case NET_INTERFACE_WLAN: {
                struct netif_state *state = (struct netif_state *)malloc(
                        sizeof(struct netif_state));
                if (state == NULL)
                        return -ENOMEM;
                memcpy(state->mac_address,
                       intf->mac_address,
                       sizeof(intf->mac_address));
                state->driver_thread_cap = driver_thread_cap;
                state->driver_in_ipc_struct = NULL;
                state->driver_out_ipc_struct = NULL;
                state->poll_thread_tid = 0;

                struct netif *netif =
                        (struct netif *)malloc(sizeof(struct netif));

#ifdef CHCORE_STATIC_IP
                ip4addr_aton(STATIC_IPADDR, &ipaddr);
                ip4addr_aton(STATIC_NETMASK, &netmask);
                ip4addr_aton(STATIC_GATEWAY, &gateway);
                if (!netif_add(netif,
                               &ipaddr,
                               &netmask,
                               &gateway,
                               state,
                               intf->type == NET_INTERFACE_ETHERNET ? eth_init :
                                                                      wlan_init,
                               tcpip_input)) {
#else
                if (!netif_add(netif,
                               NULL,
                               NULL,
                               NULL,
                               state,
                               intf->type == NET_INTERFACE_ETHERNET ? eth_init :
                                                                      wlan_init,
                               tcpip_input)) {
#endif
                        error("Failed to add netif\n");
                        if (netif)
                                free(netif);
                        break;
                }

                pthread_create(&state->poll_thread_tid,
                               NULL,
                               net_poll_thread_func,
                               netif);

                info("Added interface \"%c%c%d\"\n",
                     netif->name[0],
                     netif->name[1],
                     netif->num);
                break;
        }
        default:
                error("Unknown interface type: %d\n", intf->type);
                return -1;
        }

        return 0;
}

static err_t eth_init(struct netif *netif)
{
        struct netif_state *state = (struct netif_state *)netif->state;
        netif->hwaddr_len = sizeof(state->mac_address);
        memcpy(netif->hwaddr, state->mac_address, netif->hwaddr_len);
        netif->name[0] = 'e';
        netif->name[1] = 'n';
        netif_set_flags(netif, NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);
        netif->mtu = 1500;
        netif->output = etharp_output;
        netif->linkoutput = net_link_output;
        return ERR_OK;
}

static err_t wlan_init(struct netif *netif)
{
        struct netif_state *state = (struct netif_state *)netif->state;
        netif->hwaddr_len = sizeof(state->mac_address);
        memcpy(netif->hwaddr, state->mac_address, netif->hwaddr_len);
        netif->name[0] = 'w';
        netif->name[1] = 'l';
        netif_set_flags(netif, NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);
        netif->mtu = 1500;
        netif->output = etharp_output;
        netif->linkoutput = net_link_output;
        return ERR_OK;
}

static int net_driver_ipc(ipc_struct_t *ipc_struct, ipc_msg_t **ipc_msg_p,
                          enum NET_DRIVER_REQ req, void *data, size_t data_size,
                          int nr_args, ...)
{
        struct net_driver_request *ndr;
        ipc_msg_t *ipc_msg;
        va_list args;
        int ret = 0, i = 0;

        ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct net_driver_request));
        ndr = (struct net_driver_request *)ipc_get_msg_data(ipc_msg);
        ndr->req = req;

        va_start(args, nr_args);
        for (i = 0; i < nr_args; ++i)
                ndr->args[i] = va_arg(args, unsigned long);
        va_end(args);
        if (data) {
                BUG_ON(data_size > NET_DRIVER_DATA_LEN);
                memcpy(ndr->data, data, data_size);
        }
        ret = ipc_call(ipc_struct, ipc_msg);
        if (ipc_msg_p)
                *ipc_msg_p = ipc_msg;
        else
                ipc_destroy_msg(ipc_msg);
        return ret;
}

static err_t net_link_output(struct netif *netif, struct pbuf *p)
{
        struct netif_state *state = (struct netif_state *)netif->state;

        if (!state->driver_out_ipc_struct) {
                state->driver_out_ipc_struct =
                        ipc_register_client(state->driver_thread_cap);
        }

        if (p->tot_len > NET_DRIVER_DATA_LEN) {
                error("pbuf is too large to output, total length: %hu\n",
                      p->tot_len);
                return ERR_BUF;
        }

        ipc_msg_t *ipc_msg = ipc_create_msg(state->driver_out_ipc_struct,
                                            sizeof(struct net_driver_request));
        struct net_driver_request *ndr =
                (struct net_driver_request *)ipc_get_msg_data(ipc_msg);
        ndr->req = NET_DRIVER_SEND_FRAME;
        ndr->args[0] = p->tot_len;

        // TODO: can we do zero-copy send here?
        size_t offset = 0;
        struct pbuf *q = p;
        while (q) {
                memcpy(&ndr->data[offset], q->payload, q->len);
                offset += q->len;
                q = q->next;
        }

        int ret = ipc_call(state->driver_out_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);
        if (ret < 0) {
                error("Failed to output to driver for interface \"%c%c%d\"\n",
                      netif->name[0],
                      netif->name[1],
                      netif->num);
                netif_set_link_down(netif);
                return ERR_ABRT;
        }

        return ERR_OK;
}

static void *net_poll_thread_func(void *arg)
{
        int ret;
        struct netif *netif = (struct netif *)arg;
        struct netif_state *state = (struct netif_state *)netif->state;

        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(1, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        usys_yield();

        info("%s, driver_thread_cap: %d\n", __func__, state->driver_thread_cap);

        state->driver_in_ipc_struct =
                ipc_register_client(state->driver_thread_cap);
        if (!state->driver_in_ipc_struct) {
                error("Failed to connect to driver service of interface \"%c%c%d\"\n",
                      netif->name[0],
                      netif->name[1],
                      netif->num);
                return 0;
        }

        info("Connected to driver service of interface \"%c%c%d\"\n",
             netif->name[0],
             netif->name[1],
             netif->num);

        netif_set_up(netif);

        ret = net_driver_ipc(state->driver_in_ipc_struct,
                             NULL,
                             NET_DRIVER_WAIT_LINK_UP,
                             NULL,
                             0,
                             0);
        if (ret < 0) {
                error("Link of interface \"%c%c%d\" is not up\n",
                      netif->name[0],
                      netif->name[1],
                      netif->num);
                netif_set_down(netif);
                return 0;
        }
        netif_set_link_up(netif);
        info("Link of interface \"%c%c%d\" is up\n",
             netif->name[0],
             netif->name[1],
             netif->num);
        netif_set_default(netif);

#ifdef CHCORE_STATIC_IP
        info("DHCP disabled for interface \"%c%c%d\", use a static IP instead\n",
             netif->name[0],
             netif->name[1],
             netif->num);
        info("IP: %s, netmask: %s, gateway: %s\n",
             STATIC_IPADDR, STATIC_NETMASK, STATIC_GATEWAY);
#else
        if (dhcp_start(netif) != ERR_OK) {
                error("Failed to start DHCP for interface \"%c%c%d\"\n",
                      netif->name[0],
                      netif->name[1],
                      netif->num);
                netif_set_down(netif);
                return 0;
        }
        info("Started DHCP for interface \"%c%c%d\"\n",
             netif->name[0],
             netif->name[1],
             netif->num);
#endif

        ipc_msg_t *ipc_msg = ipc_create_msg(state->driver_in_ipc_struct,
                                            sizeof(struct net_driver_request));
        struct net_driver_request *ndr =
                (struct net_driver_request *)ipc_get_msg_data(ipc_msg);
        ndr->req = NET_DRIVER_RECEIVE_FRAME;
        while (1) {
                ret = ipc_call(state->driver_in_ipc_struct, ipc_msg);
                if (ret == NET_DRIVER_RET_NO_FRAME) {
                        continue;
                }
                if (ret < 0) {
                        error("Failed to receive frame from interface \"%c%c%d\", link may be down\n",
                              netif->name[0],
                              netif->name[1],
                              netif->num);
                        netif_set_link_down(netif);
                        netif_set_down(netif);
                        break;
                }

                u32 len = ndr->args[0];

                // TODO: can we do zero-copy receive here?
                struct pbuf *p = NULL;
                while (!p) {
                        p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
                        /* if pbuf memory is exhausted, wait for free */
                        if (!p)
                                usys_yield();
                }
                memcpy(p->payload, ndr->data, len);

                netif->input(p, netif);
#ifndef CHCORE_STATIC_IP
                static bool got_dhcp_addr = false;
                if (!got_dhcp_addr && dhcp_supplied_address(netif)) {
                        char ip_str[16] = {0}, nm_str[16] = {0},
                             gw_str[16] = {0};
                        strlcpy(ip_str,
                                ip4addr_ntoa(&netif->ip_addr),
                                sizeof(ip_str));
                        strlcpy(nm_str,
                                ip4addr_ntoa(&netif->netmask),
                                sizeof(nm_str));
                        strlcpy(gw_str,
                                ip4addr_ntoa(&netif->gw),
                                sizeof(gw_str));
                        info("DHCP got\n"
                             "  IP: %s\n"
                             "  Netmask: %s\n"
                             "  Gateway: %s\n",
                             ip_str,
                             nm_str,
                             gw_str);
                        got_dhcp_addr = true;
                }
#endif
        }
        ipc_destroy_msg(ipc_msg);

        return 0;
}
