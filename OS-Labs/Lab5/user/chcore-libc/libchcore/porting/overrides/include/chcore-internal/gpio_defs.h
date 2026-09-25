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
#include <chcore/type.h>

enum GPIO_REQ {
        SET_MODE,
        SET_PULL_MODE,
        SET_PIN_LEVEL,
};

enum GPIO_PIN_MODE {
        INPUT,
        OUTPUT,
        INPUT_PULL_UP,
        INPUT_PULL_DOWN,
        ALTERNATE_FUN0,
        ALTERNATE_FUN1,
        ALTERNATE_FUN2,
        ALTERNATE_FUN3,
        ALTERNATE_FUN4,
        ALTERNATE_FUN5,
};

enum GPIO_PIN_PULL_MODE {
        PULL_MODE_OFF,
        PULL_MODE_DOWN,
        PULL_MODE_UP,
};

enum GPIO_PIN_LEVEL {
        LOW_LEVEL,
        HIGH_LEVEL,
};

struct gpio_request {
        u32 req;
        u32 npin;
        u32 opt;
};
