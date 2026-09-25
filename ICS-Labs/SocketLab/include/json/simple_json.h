/*
 * Copyright (c) 2025 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void create_json_object(const char *object_name);
void destroy_json_object(const char *object_name);
int parse_json(const char *object_name, const char *json_string);
int set_json_value_str(const char *object_name, const char *key,
                       const char *value);
int set_json_value_bool(const char *object_name, const char *key, int value);
const char *get_json_value_str(const char *object_name, const char *key);
int get_json_value_bool(const char *object_name, const char *key);
const char *dump_json_object(const char *object_name);

#ifdef __cplusplus
}
#endif
