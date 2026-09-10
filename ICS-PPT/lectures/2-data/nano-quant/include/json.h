/* 够用的 JSON 读取器。safetensors 的头是 JSON，所以框架需要一个。
   保留对象里键的出现顺序，整数与浮点分开存，不做 Unicode 转义还原以外的加工。 */
#ifndef NQ_JSON_H
#define NQ_JSON_H

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

struct jval {
    enum kind_t { NUL, BOOL, INT, NUM, STR, ARR, OBJ } kind = NUL;
    bool        b   = false;
    int64_t     i   = 0;
    double      d   = 0;
    std::string s;
    std::vector<jval>                          arr;
    std::vector<std::pair<std::string, jval>>  obj;

    const jval *get(const char *key) const {
        for (const auto &kv : obj) if (kv.first == key) return &kv.second;
        return nullptr;
    }
    bool is_int() const { return kind == INT; }
    int64_t as_i64() const { return kind == INT ? i : (int64_t)d; }
};

bool json_parse(const char *p, size_t n, jval *out, std::string *err);

#endif
