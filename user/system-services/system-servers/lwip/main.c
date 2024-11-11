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

#include <chcore/ipc.h>
#include <chcore/pmu.h>
#include <stdio.h>
#include <pthread.h>
#include <chcore/syscall.h>
#include <errno.h>
#include <chcore/string.h>
#include <chcore/bug.h>
#include <chcore/container/list.h>

#include <lwip/debug.h>
#include <lwip/init.h>

#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/sys.h>
#include <lwip/timeouts.h>

#include <lwip/ip.h>
#include <lwip/ip4_frag.h>
#include <lwip/sockets.h>
#include <lwip/stats.h>
#include <lwip/tcp.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>
#include <netif/etharp.h>

#include <lwip/apps/snmp.h>
#include <lwip/apps/snmp_mib2.h>

#define LWIP_SRC /* needed by lwip_def */
#include <chcore-internal/lwip_defs.h>

// #define DEBUG

#define PREFIX "[lwip]"

#define info(fmt, ...)  printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define debug(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;
/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

#define MAX_SOCK_PER_CLIENT 4096
struct sockinfo_node {
        badge_t badge;
        int socket_table[MAX_SOCK_PER_CLIENT];
        struct list_head node;
};

struct list_head sockinfo_head;

static pthread_mutex_t sockinfo_lock = PTHREAD_MUTEX_INITIALIZER;

/* This function should be in the critical section */
int *register_sockinfo_node(badge_t badge)
{
        struct sockinfo_node *cur;
        int *table;

        /* check if cap exist (cap can be reused) */
        for_each_in_list (cur, struct sockinfo_node, node, &sockinfo_head) {
                if (badge == cur->badge) {
                        /* already has such node */
                        table = cur->socket_table;
                        goto out;
                }
        }

        /*
         * This operation should be done when register connection, thus no other
         * thread which holds the same cap will register sockinfo_node
         * concurrently.
         */
        cur = (struct sockinfo_node *)malloc(sizeof(struct sockinfo_node));
        if (cur == NULL) {
                table = NULL;
                goto out;
        }

        list_append(&cur->node, &sockinfo_head);
        memset(cur->socket_table, -1, sizeof(cur->socket_table));
        table = cur->socket_table;
        cur->badge = badge;
out:
        return table;
}

/*
 * Find the socket fd through client badge and the conn_id
 */
int get_socket(badge_t badge, int conn_id)
{
        struct sockinfo_node *cur;
        int ret;

        if (conn_id < 0 || conn_id >= MAX_SOCK_PER_CLIENT)
                return -EINVAL;

        pthread_mutex_lock(&sockinfo_lock);
        for_each_in_list (cur, struct sockinfo_node, node, &sockinfo_head) {
                if (badge == cur->badge) {
                        /* find socket_fd in socket_table */
                        ret = cur->socket_table[conn_id];
                        goto out;
                }
        }

        ret = -EINVAL;
out:
        pthread_mutex_unlock(&sockinfo_lock);
        return ret;
}

/*
 * Set socket fd and allocate a new conn_id
 * Return the new conn_id
 */
int set_socket(badge_t badge, int socket_fd)
{
        struct sockinfo_node *cur;
        int *table = NULL;
        int i = 0, ret = 0;

        pthread_mutex_lock(&sockinfo_lock);
        for_each_in_list (cur, struct sockinfo_node, node, &sockinfo_head) {
                if (badge == cur->badge) {
                        /* already has such node */
                        table = cur->socket_table;
                        break;
                }
        }

        if (!table) {
                /* create badge -> socket table
                 * register_sockinfo_node should be in critical section */
                table = register_sockinfo_node(badge);
                if (table == NULL) {
                        ret = -ENOMEM;
                        goto out;
                }
        }

        /* Find a suitable conn_id */
        for (i = 0; i < MAX_SOCK_PER_CLIENT; i++) {
                if (table[i] == -1) {
                        table[i] = socket_fd;
                        ret = i;
                        goto out;
                }
        }
        ret = -ENOSPC;
out:
        pthread_mutex_unlock(&sockinfo_lock);
        return ret;
}

/*
 * Free the conn_id
 */
int free_socket(badge_t badge, int conn_id)
{
        struct sockinfo_node *cur;
        int *table = NULL;
        int ret = 0;

        pthread_mutex_lock(&sockinfo_lock);
        for_each_in_list (cur, struct sockinfo_node, node, &sockinfo_head) {
                if (badge == cur->badge) {
                        /* already has such node */
                        table = cur->socket_table;
                        break;
                }
        }

        if (table) {
                table[conn_id] = -1;
                ret = 0;
        } else {
                /* cap - socket node should be created when register lwip server
                 */
                printf("FATAL BUG: No badge - socket node found!\n");
                ret = -EINVAL;
        }
        pthread_mutex_unlock(&sockinfo_lock);
        return ret;
}

void lwip_destructor(badge_t client_badge)
{
        struct sockinfo_node *cur;
        int *table = NULL;
        int i, found = 0;

        pthread_mutex_lock(&sockinfo_lock);
        for_each_in_list (cur, struct sockinfo_node, node, &sockinfo_head) {
                if (client_badge == cur->badge) {
                        /* already has such node */
                        found = 1;
                        break;
                }
        }

        if (found) {
                table = cur->socket_table;
                for (i = 0; i < MAX_SOCK_PER_CLIENT; i++) {
                        if (table[i] >= 0) {
                                lwip_close(table[i]);
                                table[i] = -1;
                        }
                }
                list_del(&cur->node);
                free(cur);
        }
        pthread_mutex_unlock(&sockinfo_lock);
}

DEFINE_SERVER_HANDLER(lwip_dispatch)
{
        int ret = 0, i = 0;
        int socket = 0, accpet_fd = 0, flags = 0, backlog = 0;
        int cmd = 0, val = 0;
        int domain = 0, protocol = 0, type = 0, level = 0, optname = 0;
        socklen_t len = 0, alen = 0, optlen = 0, namelen = 0;
        struct msghdr *msg;
        int shared_pmo = 0;
        struct sockaddr target;

        /* lr->req takes 4 bytes. */
        struct lwip_request *lr =
                (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        switch (lr->req) {
        case LWIP_CREATE_SOCKET:
                debug("LWIP_CREATE_SOCKET\n");
                domain = lr->args[0];
                type = lr->args[1];
                protocol = lr->args[2];
                socket = lwip_socket(domain, type, protocol);
                /* only set socket when succ */
                ret = (socket >= 0) ? set_socket(client_badge, socket) :
                                        socket;
                debug("LWIP_CREATE_SOCKET return %d\n", ret);
                break;
        case LWIP_SOCKET_BIND:
                debug("LWIP_SOCKET_BIND\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
#ifdef DEBUG /* Print the basic info of bind */
                struct sockaddr_in *sockaddr =
                        (struct sockaddr_in *)lr->data;
                char IP[16];
                lwip_inet_ntop(AF_INET,
                                (char *)&sockaddr->sin_addr,
                                IP,
                                sizeof(IP));
                int port = ntohs(sockaddr->sin_port);
                debug("BIND to %s:%d\n", IP, port);
#endif
                ret = lwip_bind(
                        socket, (struct sockaddr *)lr->data, len);
                break;
        case LWIP_SOCKET_RECV:
                /* here we will call recvfrom rather than recv */
                debug("LWIP_SOCKET_RECV\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
                flags = lr->args[2];
                alen = lr->args[3];
                /* If data is too large and cap_slot is set, use PMO to
                        * transfer data*/
                if ((len > LWIP_DATA_LEN)
                        && (ipc_get_msg_send_cap_num(ipc_msg) == 1)) {
                        shared_pmo = ipc_get_msg_cap(ipc_msg, 0);
                        void *buffer = malloc(len);
                        ret = lwip_recvfrom(socket,
                                                buffer,
                                                len,
                                                flags,
                                                &target,
                                                &alen);
                        usys_write_pmo(shared_pmo, 0, buffer, len);
                        if (alen > 0)
                                usys_write_pmo(
                                        shared_pmo, len, &target, alen);
                        free(buffer);
                } else {
                        ret = lwip_recvfrom(socket,
                                                lr->data,
                                                len,
                                                flags,
                                                &target,
                                                &alen);
                        if (alen > 0)
                                memcpy(((char *)lr->data + len),
                                        &target,
                                        alen);
                        debug("LWIP_SOCKET_RECV return %d\n", ret);
                        lr->args[3] = alen;
                }
                break;
        case LWIP_SOCKET_READ:
                debug("LWIP_SOCKET_READ\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
                ret = lwip_read(socket, lr->data, len);
                break;
        case LWIP_SOCKET_RMSG:
                debug("LWIP_SOCKET_RMSG\n");
                socket = get_socket(client_badge, lr->args[0]);
                flags = lr->args[1];
                len = lr->args[2];
                if (len > LWIP_DATA_LEN) {
                        if (ipc_get_msg_send_cap_num(ipc_msg) != 1) {
                                ret = -EINVAL;
                                goto out;
                        }
                        shared_pmo = ipc_get_msg_cap(ipc_msg, 0);
                        msg = malloc(len);
                        if (msg == NULL) {
                                ret = -ENOMEM;
                                goto out;
                        }
                        /* XXX: can be optimized */
                        usys_read_pmo(shared_pmo, 0, msg, len);
                        update_msg_ptr(msg);
                        ret = lwip_recvmsg(socket, msg, flags);
                        usys_write_pmo(shared_pmo, 0, msg, len);
                        free(msg);
                } else {
                        msg = (struct msghdr *)lr->data;
                        update_msg_ptr(msg);
                        ret = lwip_recvmsg(socket, msg, flags);
                }

                break;
        case LWIP_SOCKET_SEND:
                /* here we will call sendto rather than send */
                debug("LWIP_SOCKET_SEND\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
                flags = lr->args[2];
                alen = (socklen_t)lr->args[3];

                if ((len > LWIP_DATA_LEN)
                        && (ipc_get_msg_send_cap_num(ipc_msg) == 1)) {
                        shared_pmo = ipc_get_msg_cap(ipc_msg, 0);
                        void *buffer = malloc(len);
                        usys_read_pmo(shared_pmo, 0, buffer, len);
                        if (alen > 0)
                                usys_read_pmo(
                                        shared_pmo, len, &target, alen);

                        ret = lwip_sendto(socket,
                                                buffer,
                                                len,
                                                flags,
                                                &target,
                                                alen);
                        free(buffer);
                } else {
                        if (alen > 0)
                                memcpy(&target,
                                        ((char *)lr->data + len),
                                        alen);
                        ret = lwip_sendto(socket,
                                                lr->data,
                                                len,
                                                flags,
                                                &target,
                                                alen);
                }
                debug("LWIP_SOCKET_SEND return %d\n", ret);
                break;
        case LWIP_SOCKET_WRITE:
                socket = get_socket(client_badge, lr->args[0]);
                debug("LWIP_SOCKET_WRITE socket %d badge %u args %lu\n",
                        socket,
                        client_badge,
                        lr->args[0]);
                len = lr->args[1];
                ret = lwip_write(socket, lr->data, len);
                debug("LWIP_SOCKET_WRITE return %d %d\n", ret, errno);
                break;
        case LWIP_SOCKET_SMSG:
                debug("LWIP_SOCKET_SMSG\n");
                socket = get_socket(client_badge, lr->args[0]);
                flags = lr->args[1];
                len = lr->args[2];
                if (len > LWIP_DATA_LEN) {
                        if (ipc_get_msg_send_cap_num(ipc_msg) != 1) {
                                ret = -EINVAL;
                                goto out;
                        }
                        shared_pmo = ipc_get_msg_cap(ipc_msg, 0);
                        msg = malloc(len);
                        if (msg == NULL) {
                                ret = -ENOMEM;
                                goto out;
                        }
                        usys_read_pmo(shared_pmo, 0, msg, len);
                        update_msg_ptr(msg);
                        ret = lwip_sendmsg(socket, msg, flags);
                        free(msg);
                } else {
                        msg = (struct msghdr *)lr->data;
                        update_msg_ptr(msg);
                        ret = lwip_sendmsg(socket, msg, flags);
                }
                break;
        case LWIP_SOCKET_LIST:
                debug("LWIP_SOCKET_LIST\n");
                socket = get_socket(client_badge, lr->args[0]);
                backlog = lr->args[1];
                ret = lwip_listen(socket, backlog);
                break;
        case LWIP_SOCKET_CONN:
                debug("LWIP_SOCKET_CONN\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
                ret = lwip_connect(
                        socket, (struct sockaddr *)lr->data, len);

                break;
        case LWIP_SOCKET_ACPT:
                debug("LWIP_SOCKET_ACPT\n");
                socket = get_socket(client_badge, lr->args[0]);
                len = lr->args[1];
                accpet_fd = lwip_accept(
                        socket, (struct sockaddr *)lr->data, &len);
                /* only set socket when succ */
                ret = (accpet_fd >= 0) ?
                                set_socket(client_badge, accpet_fd) :
                                accpet_fd;
                /* set len to pass it back */
                lr->args[1] = len;
                break;
        case LWIP_SOCKET_CLSE:
                debug("LWIP_SOCKET_CLSE\n");
                socket = get_socket(client_badge, lr->args[0]);
                ret = lwip_close(socket);
                if (ret == 0)
                        free_socket(client_badge, lr->args[0]);
                break;
        case LWIP_SOCKET_SOPT:
                debug("LWIP_SOCKET_SOPT\n");
                socket = get_socket(client_badge, lr->args[0]);
                level = lr->args[1];
                optname = lr->args[2];
                optlen = lr->args[3];
                ret = lwip_setsockopt(
                        socket, level, optname, lr->data, optlen);
                break;
        case LWIP_REQ_FCNTL:
                debug("LWIP_REQ_FCNTL\n");
                socket = get_socket(client_badge, lr->args[0]);
                cmd = lr->args[1];
                val = lr->args[2];
                ret = lwip_fcntl(socket, cmd, val);
                break;
        case LWIP_SOCKET_GOPT:
                debug("LWIP_SOCKET_GOPT\n");
                socket = get_socket(client_badge, lr->args[0]);
                level = lr->args[1];
                optname = lr->args[2];
                optlen = lr->args[3];
                ret = lwip_getsockopt(
                        socket, level, optname, lr->data, &optlen);
                lr->args[3] = optlen;
                break;
        case LWIP_SOCKET_NAME:
                debug("LWIP_SOCKET_NAME\n");
                socket = get_socket(client_badge, lr->args[0]);
                namelen = lr->args[1];
                if (namelen == -1)
                        ret = -EINVAL;
                else
                        ret = lwip_getsockname(
                                socket,
                                (struct sockaddr *)lr->data,
                                &namelen);
                lr->args[1] = namelen;
                break;
        case LWIP_SOCKET_PEER:
                debug("LWIP_SOCKET_PEER\n");
                socket = get_socket(client_badge, lr->args[0]);
                namelen = lr->args[1];
                if (namelen == -1)
                        ret = -EINVAL;
                else
                        ret = lwip_getpeername(
                                socket,
                                (struct sockaddr *)lr->data,
                                &namelen);
                lr->args[1] = namelen;
                break;
        case LWIP_SOCKET_STDW:
                debug("LWIP_SOCKET_STDW\n");
                socket = get_socket(client_badge, lr->args[0]);
                ret = lwip_shutdown(socket, lr->args[1]);
                break;
        case LWIP_REQ_SOCKET_POLL: {
                debug("LWIP_SOCKET_POLL\n");
                unsigned long nfds = lr->args[1];
                unsigned long timeout = lr->args[2];
                struct pollfd *fd_iter;

                for (i = 0; i < nfds; i++) {
                        fd_iter = ((struct pollfd *)lr->data) + i;
                        fd_iter->fd =
                                get_socket(client_badge, fd_iter->fd);
                }
                ret = lwip_poll(
                        (struct pollfd *)lr->data, nfds, timeout);
                break;
        }
        case LWIP_SOCKET_IOCTL:
                debug("LWIP_SOCKET_IOCTL\n");
                socket = get_socket(client_badge, lr->args[0]);
                cmd = lr->args[1];
                ret = lwip_ioctl(socket, cmd, lr->data);
                break;
        case LWIP_INTERFACE_ADD: {
                debug("LWIP_INTERFACE_ADD\n");
                extern int add_interface(ipc_msg_t * ipc_msg,
                                                struct lwip_request * lr);
                ret = add_interface(ipc_msg, lr);
                break;
        }
        default:
                printf("lwip not impl req:%d\n", lr->req);
                ret = -EINVAL;
        }

        /* LWIP should return errno to the caller. */
        if (ret < 0)
                ret = -errno;

out:
        ipc_return(ipc_msg, ret);
}

err_t netif_loopif_init(struct netif *netif);

int main(int argc, char *argv[], char *envp[])
{
        struct netif netif;
        char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};

        init_list_head(&sockinfo_head);

        /* startup defaults (may be overridden by one or more opts) */
        IP4_ADDR(&gw, 192, 168, 0, 1);
        IP4_ADDR(&ipaddr, 192, 168, 0, 3);
        IP4_ADDR(&netmask, 255, 255, 255, 0);

        /* use debug flags defined by debug.h */
        debug_flags = LWIP_DBG_OFF;

        debug_flags |= (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE
                        | LWIP_DBG_FRESH | LWIP_DBG_HALT);

        strlcpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
        strlcpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
        strlcpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
        info("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);

        /* Initialize */
        tcpip_init(NULL, NULL);
        info("TCP/IP initialized.\n");

        netif_add(&netif,
                  &ipaddr,
                  &netmask,
                  &gw,
                  NULL,
                  netif_loopif_init,
                  tcpip_input);
        netif_set_link_up(&netif);
        netif_set_default(&netif);
        netif_set_up(&netif);
        info("Add netif %p\n", &netif);

        info("register server value = %u\n",
             ipc_register_server_with_destructor(lwip_dispatch,
                                                 DEFAULT_CLIENT_REGISTER_HANDLER,
                                                 lwip_destructor));

        usys_exit(0);
        return 0;
}
