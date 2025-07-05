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

#pragma once

typedef int cap_t;
typedef int badge_t;
/*
 * Rights of capabilities.
 * Rights can be divided into two types, one is object-specific rights (such
 * as PMO_READ, PMO_WRITE, PMO_EXEC, PMO_COW), and the other is some general
 * rights (CAP_RIGHT_COPY and CAP_RIGHT_REVOKE_ALL) to define capability
 * actions.
 * NOTE: When defining new capability rights, it is necessary to be careful
 * NOT to cause CONFLICTS between the above two types of rights.
 */
typedef int cap_right_t;