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

#ifdef __cplusplus
extern "C" {
#endif

enum CPU_DRIVER_REQ {
        CPU_SET_SPEED = 1,
        CPU_GET_CLOCKRATE,
        CPU_GET_TEMPERATURE,
        DUMP_CPU_CURRENT_INFO,
};

enum CPU_SPEED {
        CPU_MAXSPEED = 1,
        CPU_LOWSPEED,
};

enum CPU_CLOCKRATE_TYPE {
        CLOCKRATE_CURRENT = 1,
        CLOCKRATE_MAX,
        CLOCKRATE_MIN,
};

enum CPU_TEMPERATURE_TYPE {
        TEMPERATURE_CURRENT = 1,
        TEMPERATURE_MAX,
};

struct cpu_driver_msg {
        enum CPU_DRIVER_REQ req;
        union {
                struct {
                        enum CPU_SPEED speed;
                } set_speed_req;
                struct {
                        enum CPU_CLOCKRATE_TYPE type;
                        unsigned int clock_rate;
                } get_clockrate_req;
                struct {
                        enum CPU_TEMPERATURE_TYPE type;
                        unsigned int temperature;
                } get_temperature_req;
                struct {
                        char cpu_info[1024];
                } dump_cpu_req;
        };
};

#ifdef __cplusplus
}
#endif
