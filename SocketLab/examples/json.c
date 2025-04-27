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

#include "json/simple_json.h"
#include <stdio.h>

int main() {
    // Create a JSON object
    create_json_object("my_json");

    // Parse a JSON string
    const char *json_string =
        "{\"class\": \"ICS\", \"semester\": \"2024-2025-2\"}";
    parse_json("my_json", json_string);

    // Retrieve a value from the JSON object
    const char *class = get_json_value_str("my_json", "class");
    printf("class: %s\n", class);
    const char *semester = get_json_value_str("my_json", "semester");
    printf("semester: %s\n", semester);

    // Destroy the JSON object
    destroy_json_object("my_json");

    return 0;
}
