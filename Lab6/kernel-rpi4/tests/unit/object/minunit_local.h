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

#ifndef MINUNIT_LOCAL_H
#define MINUNIT_LOCAL_H

#include "minunit.h"

#define mu_assert_msg(test, format, arg...)              \
        MU__SAFE_BLOCK(minunit_assert++; if (!(test)) {  \
                snprintf(minunit_last_message,           \
                         MINUNIT_MESSAGE_LEN,            \
                         "%s failed:\n\t%s:%d: " format, \
                         __func__,                       \
                         __FILE__,                       \
                         __LINE__,                       \
                         ##arg);                         \
                minunit_status = 1;                      \
                return;                                  \
        })

#endif /* MINUNIT_LOCAL_H */