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

#ifndef UAPI_OBJECT_H
#define UAPI_OBJECT_H

#include <uapi/types.h>

#define CAP_RIGHT_REVOKE_ALL (1 << (sizeof(cap_right_t) * 8 - 1))
#define CAP_RIGHT_COPY       (1 << (sizeof(cap_right_t) * 8 - 2))
#define CAP_RIGHT_NO_RIGHTS  (0)

#endif /* UAPI_OBJECT_H */
