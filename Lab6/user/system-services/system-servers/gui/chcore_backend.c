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

#include "chcore_backend.h"

#include <malloc.h>
#include <pthread.h>
#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>

#include <hashmap.h>
#include <libpipe.h>


/* ChCore window client struct */
struct ch_client
{
	badge_t client_badge;
	struct pipe *c2s_pipe;
	struct pipe *s2c_pipe;
	cap_t client_cap_group_cap;
	cap_t vsync_notific_cap;
	struct list_head node;
	struct hashmap *hdl_map;
};

struct chcore_backend
{
	struct hashmap *badge_wc_map; // client_badge -> ch_client
	struct list_head cwc_list;
	pthread_mutex_t cwc_lock;
};

static struct chcore_backend chb = { 0 };


static struct ch_client *create_ch_client(badge_t client_badge,
	struct pipe *c2s_pipe, struct pipe *s2c_pipe,
	int client_cap_group_cap, int vsync_notific_cap)
{
	struct ch_client *cc;
	cc = (struct ch_client *)malloc(sizeof(*cc));
	cc->client_badge = client_badge;
	cc->c2s_pipe = c2s_pipe;
	cc->s2c_pipe = s2c_pipe;
	cc->client_cap_group_cap = client_cap_group_cap;
	cc->vsync_notific_cap = vsync_notific_cap;
	init_list_head(&cc->node);
	cc->hdl_map = create_hashmap(32);
	return cc;
}

// static void del_ch_client(struct ch_client *cc)
// {
// 	del_pipe(cc->c2s_pipe);
// 	del_pipe(cc->s2c_pipe);
// 	del_hashmap(cc->hdl_map);
// 	free(cc);
// }

static inline void add_ch_client(struct ch_client *cc)
{
	list_add(&cc->node, &chb.cwc_list);
}

// static inline void rm_ch_client(struct ch_client *cc)
// {
// 	list_del(&cc->node);
// }

static inline struct ch_client *get_ch_client(badge_t client_badge)
{
	return (struct ch_client *)hashmap_get(chb.badge_wc_map,
		client_badge);
}

static void handle_ch_client_msg(struct ch_client *cc)
{
	// TODO...
}


inline void init_chcore_backend(void)
{
	chb.badge_wc_map = create_hashmap(64);
	init_list_head(&chb.cwc_list);
	pthread_mutex_init(&chb.cwc_lock, NULL);
}

void chcore_conn(ipc_msg_t *ipc_msg, badge_t client_badge)
{
	struct ch_client *cc;
	cap_t c2s_pipe_pmo, s2c_pipe_pmo;
	cap_t client_cap_group_cap, vsync_notific_cap;
	struct pipe *c2s_pipe, *s2c_pipe;

	/* Get ch_client */
	cc = get_ch_client(client_badge);
	if (cc != NULL)
		return;
	
	client_cap_group_cap = ipc_get_msg_cap(ipc_msg, 0);
	c2s_pipe_pmo = ipc_get_msg_cap(ipc_msg, 1);
	s2c_pipe_pmo = ipc_get_msg_cap(ipc_msg, 2);
	vsync_notific_cap = ipc_get_msg_cap(ipc_msg, 3);
	if (c2s_pipe_pmo < 0 || s2c_pipe_pmo < 0
		|| client_cap_group_cap < 0 || vsync_notific_cap < 0)
		return;
	
	c2s_pipe = create_pipe_from_pmo(PAGE_SIZE, c2s_pipe_pmo);
	s2c_pipe = create_pipe_from_pmo(PAGE_SIZE, s2c_pipe_pmo);
	pipe_init(c2s_pipe);
	pipe_init(s2c_pipe);

	cc = create_ch_client(client_badge, c2s_pipe, s2c_pipe,
		client_cap_group_cap, vsync_notific_cap);
	
	pthread_mutex_lock(&chb.cwc_lock);
	add_ch_client(cc);
	pthread_mutex_unlock(&chb.cwc_lock);

	ipc_set_msg_return_cap_num(ipc_msg, 1);
	ipc_set_msg_cap(ipc_msg, 0, SELF_CAP);
}

inline void chcore_handle_backend(void)
{
	struct ch_client *cc;
	pthread_mutex_lock(&chb.cwc_lock);
	for_each_in_list (cc, struct ch_client, node, &chb.cwc_list) {
		handle_ch_client_msg(cc);
	}
	pthread_mutex_unlock(&chb.cwc_lock);
}

inline void chcore_awake_clients(void)
{
	struct ch_client *cc;
	pthread_mutex_lock(&chb.cwc_lock);
	for_each_in_list (cc, struct ch_client, node, &chb.cwc_list) {
		usys_notify(cc->vsync_notific_cap);
	}
	pthread_mutex_unlock(&chb.cwc_lock);
}
