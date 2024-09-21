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

#ifndef LIB_PRINTK_H
#define LIB_PRINTK_H

typedef void (*graphic_putc_handler)(char c);
extern graphic_putc_handler graphic_putc;

void set_graphic_putc_handler(graphic_putc_handler f);
void printk(const char *fmt, ...);

int simple_sprintf(char *str, const char *fmt, ...);

#endif /* LIB_PRINTK_H */