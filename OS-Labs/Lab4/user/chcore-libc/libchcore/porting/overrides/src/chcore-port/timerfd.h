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

#ifndef CHCORE_PORT_TIMERFD_H
#define CHCORE_PORT_TIMERFD_H

#include <sys/timerfd.h>
#include <poll.h>

int chcore_timerfd_create(int clockid, int flags);
int chcore_timerfd_settime(int fd, int flags, struct itimerspec *new_value,
                           struct itimerspec *old_value);
int chcore_timerfd_gettime(int fd, struct itimerspec *curr_value);

#endif /* CHCORE_PORT_TIMERFD_H */
