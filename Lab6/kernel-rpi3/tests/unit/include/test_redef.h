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

#ifndef TEST_REDEF_H
#define TEST_REDEF_H

#include <stdio.h>

#ifdef kdebug
#undef kdebug
#define kdebug(x, ...) printf(x, ##__VA_ARGS__)
#endif

#ifdef kinfo
#undef kinfo
#define kinfo(x, ...) printf(x, ##__VA_ARGS__)
#endif

#ifdef kwarn
#undef kwarn
#define kwarn(x, ...) printf(x, ##__VA_ARGS__)
#endif

#endif /* TEST_REDEF_H */