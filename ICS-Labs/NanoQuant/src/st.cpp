/* The safetensors reader.
 *
 * The file is: an 8-byte little-endian header length N, N bytes of UTF-8 JSON,
 * then the tensor data. The JSON header is an object; each key is a tensor
 * name and its value gives dtype, shape and data_offsets. The key
 * __metadata__ is optional extra information.
 *
 * The header length is read with rd_u64le from nano_quant.h; the framework
 * does the rest. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "json.h"
#include "nq.h"

namespace {

size_t dtype_size(const std::string &d) {
    if (d == "BF16" || d == "F16") return 2;
    if (d == "F32") return 4;
    return 0;
}

/* gcc and clang check the arguments against the format; other compilers
   do without the check */
#if defined(__GNUC__) || defined(__clang__)
std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
#endif
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
    f->fd = nq_open_read(path);
    if (f->fd < 0) { f->err = fmt("%s: %s", path, strerror(errno)); return -1; }

    int64_t sz = nq_file_size(f->fd);
    if (sz < 0) { f->err = fmt("%s: %s", path, strerror(errno)); return -1; }
    f->file_size = (uint64_t)sz;

    if (f->file_size < 8) { f->err = "file is shorter than 8 bytes, no header length to read"; return -1; }

    uint8_t hdr[8];
    if (nq_read_at(f->fd, hdr, 8, 0) != 0) { f->err = "cannot read the header length"; return -1; }

    /* your function, from impl/nano_quant.cpp */
    f->header_len = rd_u64le(hdr);

    if (f->header_len == 0 || f->header_len > f->file_size - 8) {
        f->err = fmt("header length %llu is impossible: the file has only %llu bytes. "
                     "If the bytes of this number are in reverse order, check the "
                     "shift direction in rd_u64le",
                     (unsigned long long)f->header_len, (unsigned long long)f->file_size);
        return -1;
    }
    f->data_base = 8 + f->header_len;

    std::string js((size_t)f->header_len, '\0');
    if (nq_read_at(f->fd, &js[0], (size_t)f->header_len, 8) != 0) {
        f->err = "cannot read the JSON header";
        return -1;
    }

    jval root;
    std::string jerr;
    if (!json_parse(js.data(), js.size(), &root, &jerr) || root.kind != jval::OBJ) {
        f->err = "cannot parse the JSON header: " + jerr;
        return -1;
    }

    uint64_t data_span = f->file_size - f->data_base;

    for (const auto &kv : root.obj) {
        if (kv.first == "__metadata__") continue;
        const jval &v = kv.second;
        if (v.kind != jval::OBJ) { f->err = kv.first + ": entry is not an object"; return -1; }

        const jval *jd = v.get("dtype");
        const jval *js2 = v.get("shape");
        const jval *jo = v.get("data_offsets");
        if (!jd || jd->kind != jval::STR || !js2 || js2->kind != jval::ARR ||
            !jo || jo->kind != jval::ARR || jo->arr.size() != 2) {
            f->err = kv.first + ": dtype, shape or data_offsets missing";
            return -1;
        }

        st_tensor t;
        t.name  = kv.first;
        t.dtype = jd->s;
        t.elem_size = dtype_size(t.dtype);
        if (t.elem_size == 0) { f->err = kv.first + ": unsupported dtype " + t.dtype; return -1; }

        t.n_elem = 1;
        for (const auto &d : js2->arr) {
            if (!d.is_int() || d.i < 0) { f->err = kv.first + ": bad value in shape"; return -1; }
            t.shape.push_back((uint64_t)d.i);
            t.n_elem *= (uint64_t)d.i;
        }
        t.beg = (uint64_t)jo->arr[0].as_i64();
        t.end = (uint64_t)jo->arr[1].as_i64();

        if (t.end < t.beg || t.end > data_span) {
            f->err = fmt("%s: range [%llu, %llu) runs past the data area (%llu bytes)",
                         t.name.c_str(), (unsigned long long)t.beg,
                         (unsigned long long)t.end, (unsigned long long)data_span);
            return -1;
        }
        if (t.end - t.beg != t.n_elem * t.elem_size) {
            f->err = fmt("%s: range length %llu, but shape gives %llu",
                         t.name.c_str(), (unsigned long long)(t.end - t.beg),
                         (unsigned long long)(t.n_elem * t.elem_size));
            return -1;
        }
        f->tensors.push_back(std::move(t));
    }

    return 0;
}

void st_close(st_file *f) {
    if (f->fd >= 0) { nq_close(f->fd); f->fd = -1; }
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
    if (nq_read_at(f->fd, raw.data(), need, base) != 0) return 0;

    if (t->dtype == "F32") {
        memcpy(dst, raw.data(), need);
    } else {
        const uint8_t *p = raw.data();
        const bool bf = (t->dtype == "BF16");
        for (size_t i = 0; i < n_elem; i++) {
            uint16_t h = (uint16_t)((uint16_t)p[2*i] | ((uint16_t)p[2*i + 1] << 8));
            dst[i] = bf ? bf16_to_f32(h) : fp16_to_f32(h);   /* your functions */
        }
    }
    return n_elem;
}
