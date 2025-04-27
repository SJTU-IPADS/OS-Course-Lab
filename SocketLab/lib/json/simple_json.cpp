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

#include "json.hpp"

#include <cassert>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

using JSON_WRAPPER = std::pair<nlohmann::json, std::string *>;
static std::unordered_map<std::string, JSON_WRAPPER> json_map;
static std::mutex json_map_mutex; // Mutex to protect the map

static void initialize_wrapper(JSON_WRAPPER &wrapper) {
    // Initialize the JSON wrapper
    wrapper.first = nlohmann::json::object();
    wrapper.second = new std::string();
    assert(wrapper.second &&
           "Failed to allocate memory for JSON wrapper string");
}

static void destroy_wrapper(JSON_WRAPPER &wrapper) {
    // Clean up the JSON wrapper
    delete wrapper.second;
    wrapper.second = nullptr;
}

#ifdef __cplusplus
extern "C" {
#endif

void create_json_object(const char *object_name) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    if (json_map.find(object_name) != json_map.end()) {
        std::cerr << "Object already exists: " << object_name << std::endl;
        assert(false && "Failed to create JSON object");
    }

    // Insert the object into the map
    json_map[object_name] = JSON_WRAPPER();
    initialize_wrapper(json_map[object_name]);
}

void destroy_json_object(const char *object_name) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Clean up the JSON wrapper
    destroy_wrapper(it->second);
    json_map.erase(it);
}

int parse_json(const char *object_name, const char *json_string) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Parse the JSON string
    nlohmann::json &json_obj = it->second.first;
    try {
        json_obj = nlohmann::json::parse(json_string);
    } catch (const nlohmann::json::parse_error &e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

int set_json_value_str(const char *object_name, const char *key,
                       const char *value) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Set the value in the JSON object
    try {
        it->second.first[key] = value;
    } catch (const nlohmann::json::type_error &e) {
        std::cerr << "Type error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

int set_json_value_bool(const char *object_name, const char *key, int value) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Set the boolean value in the JSON object
    try {
        it->second.first[key] = static_cast<bool>(value);
    } catch (const nlohmann::json::type_error &e) {
        std::cerr << "Type error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

const char *get_json_value_str(const char *object_name, const char *key) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Get the value from the JSON object
    nlohmann::json &json_obj = it->second.first;
    if (!json_obj.contains(key)) {
        std::cerr << "Key not found: " << key << std::endl;
        assert(false && "Failed to find key in JSON object");
        return nullptr;
    }

    // Return the value as a string
    *(it->second.second) = json_obj[key].get<std::string>();
    return it->second.second->c_str();
}

int get_json_value_bool(const char *object_name, const char *key) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
    }

    // Get the value from the JSON object
    nlohmann::json &json_obj = it->second.first;
    if (!json_obj.contains(key)) {
        std::cerr << "Key not found: " << key << std::endl;
        assert(false && "Failed to find key in JSON object");
        return 0;
    }

    // Return the value as a boolean
    return json_obj[key].get<bool>();
}

const char *dump_json_object(const char *object_name) {
    // Lock the mutex to protect the map
    std::lock_guard<std::mutex> lock(json_map_mutex);

    // Check if the object name is valid
    auto it = json_map.find(object_name);
    if (it == json_map.end()) {
        std::cerr << "Object not found: " << object_name << std::endl;
        assert(false && "Failed to find JSON object");
        return nullptr;
    }

    // Dump the JSON object to a string
    nlohmann::json &json_obj = it->second.first;
    try {
        *it->second.second = json_obj.dump();
    } catch (const nlohmann::json::type_error &e) {
        std::cerr << "Type error: " << e.what() << std::endl;
        return nullptr;
    }
    return it->second.second->c_str();
}

#ifdef __cplusplus
}
#endif
