/* safetensors 读取器。
 *
 * 文件的形状是：8 字节小端头长度 N，N 字节 UTF-8 的 JSON 头，其余是张量数据。
 * JSON 头是一个对象，每个键是张量名，值给出 dtype、shape 与 data_offsets；
 * 键 __metadata__ 是可选的附加信息，不是张量。
 *
 * 头长度这一步交给学生的 rd_u64le，别的都由框架完成。 */
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "json.h"
#include "nq.h"

namespace {

size_t dtype_size(const std::string &d) {
    if (d == "BF16" || d == "F16") return 2;
    if (d == "F32") return 4;
    return 0;
}

std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char *f, ...) {
    char b[512];
    va_list ap;
    va_start(ap, f);
    vsnprintf(b, sizeof b, f, ap);
    va_end(ap);
    return std::string(b);
}

} /* namespace */

int st_open(st_file *f, const char *path) {
    f->fd = open(path, O_RDONLY);
    if (f->fd < 0) { f->err = fmt("%s: %s", path, strerror(errno)); return -1; }

    off_t sz = lseek(f->fd, 0, SEEK_END);
    if (sz < 0) { f->err = fmt("%s: %s", path, strerror(errno)); return -1; }
    f->file_size = (uint64_t)sz;

    if (f->file_size < 8) { f->err = "文件不足 8 字节，读不出头长度"; return -1; }

    uint8_t hdr[8];
    if (pread(f->fd, hdr, 8, 0) != 8) { f->err = "读头长度失败"; return -1; }

    /* 这一句调的是学生写的函数 */
    f->header_len = rd_u64le(hdr);

    if (f->header_len == 0 || f->header_len > f->file_size - 8) {
        f->err = fmt("头长度 %llu 不可能：文件只有 %llu 字节。"
                     "若这个数看着像字节顺序反了，检查 rd_u64le 的移位方向",
                     (unsigned long long)f->header_len, (unsigned long long)f->file_size);
        return -1;
    }
    f->data_base = 8 + f->header_len;

    std::string js((size_t)f->header_len, '\0');
    if (pread(f->fd, &js[0], (size_t)f->header_len, 8) != (ssize_t)f->header_len) {
        f->err = "读 JSON 头失败";
        return -1;
    }

    jval root;
    std::string jerr;
    if (!json_parse(js.data(), js.size(), &root, &jerr) || root.kind != jval::OBJ) {
        f->err = "JSON 头解析失败：" + jerr;
        return -1;
    }

    uint64_t data_span = f->file_size - f->data_base;

    for (const auto &kv : root.obj) {
        if (kv.first == "__metadata__") continue;
        const jval &v = kv.second;
        if (v.kind != jval::OBJ) { f->err = kv.first + "：条目不是对象"; return -1; }

        const jval *jd = v.get("dtype");
        const jval *js2 = v.get("shape");
        const jval *jo = v.get("data_offsets");
        if (!jd || jd->kind != jval::STR || !js2 || js2->kind != jval::ARR ||
            !jo || jo->kind != jval::ARR || jo->arr.size() != 2) {
            f->err = kv.first + "：缺 dtype/shape/data_offsets";
            return -1;
        }

        st_tensor t;
        t.name  = kv.first;
        t.dtype = jd->s;
        t.elem_size = dtype_size(t.dtype);
        if (t.elem_size == 0) { f->err = kv.first + "：不支持的 dtype " + t.dtype; return -1; }

        t.n_elem = 1;
        for (const auto &d : js2->arr) {
            if (!d.is_int() || d.i < 0) { f->err = kv.first + "：shape 里有非法值"; return -1; }
            t.shape.push_back((uint64_t)d.i);
            t.n_elem *= (uint64_t)d.i;
        }
        t.beg = (uint64_t)jo->arr[0].as_i64();
        t.end = (uint64_t)jo->arr[1].as_i64();

        if (t.end < t.beg || t.end > data_span) {
            f->err = fmt("%s：区间 [%llu, %llu) 超出数据区（%llu 字节）",
                         t.name.c_str(), (unsigned long long)t.beg,
                         (unsigned long long)t.end, (unsigned long long)data_span);
            return -1;
        }
        if (t.end - t.beg != t.n_elem * t.elem_size) {
            f->err = fmt("%s：区间长度 %llu 与 shape 算出的 %llu 不符",
                         t.name.c_str(), (unsigned long long)(t.end - t.beg),
                         (unsigned long long)(t.n_elem * t.elem_size));
            return -1;
        }
        f->tensors.push_back(std::move(t));
    }

    return 0;
}

void st_close(st_file *f) {
    if (f->fd >= 0) { close(f->fd); f->fd = -1; }
}

int st_count(const st_file *f) { return (int)f->tensors.size(); }

const st_tensor *st_get(const st_file *f, int i) {
    if (i < 0 || i >= (int)f->tensors.size()) return nullptr;
    return &f->tensors[(size_t)i];
}

const st_tensor *st_find(const st_file *f, const char *name) {
    for (const auto &t : f->tensors) if (t.name == name) return &t;
    return nullptr;
}

size_t st_read_f32(const st_file *f, const st_tensor *t,
                   uint64_t elem_off, float *dst, size_t n_elem) {
    if (!t || elem_off > t->n_elem) return 0;
    if (n_elem > t->n_elem - elem_off) n_elem = (size_t)(t->n_elem - elem_off);
    if (n_elem == 0) return 0;

    const uint64_t base = f->data_base + t->beg + elem_off * t->elem_size;
    const size_t   need = n_elem * t->elem_size;

    std::vector<uint8_t> raw(need);
    size_t done = 0;
    while (done < need) {
        ssize_t got = pread(f->fd, raw.data() + done, need - done, (off_t)(base + done));
        if (got <= 0) return 0;
        done += (size_t)got;
    }

    if (t->dtype == "F32") {
        memcpy(dst, raw.data(), need);
    } else {
        const uint8_t *p = raw.data();
        const bool bf = (t->dtype == "BF16");
        for (size_t i = 0; i < n_elem; i++) {
            uint16_t h = (uint16_t)((uint16_t)p[2*i] | ((uint16_t)p[2*i + 1] << 8));
            dst[i] = bf ? bf16_to_f32(h) : fp16_to_f32(h);   /* 学生的函数 */
        }
    }
    return n_elem;
}
