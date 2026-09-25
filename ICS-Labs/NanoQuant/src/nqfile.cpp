/* Reading and writing the .nq container; nq.h describes the format.
 *
 * The container is kept plain on purpose: the header is only a directory and
 * the data area is the quantized tensors laid end to end, so the md5 of the
 * data area covers the quantized bytes and nothing else: the names, types and
 * shapes are in the header. */
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "nq.h"

namespace {

void put_u32(std::vector<uint8_t> &v, uint32_t x) {
    for (int i = 0; i < 4; i++) v.push_back((uint8_t)(x >> (8 * i)));
}
void put_u64(std::vector<uint8_t> &v, uint64_t x) {
    for (int i = 0; i < 8; i++) v.push_back((uint8_t)(x >> (8 * i)));
}
uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
uint64_t get_u64(const uint8_t *p) {
    uint64_t x = 0;
    for (int i = 7; i >= 0; i--) x = (x << 8) | p[i];
    return x;
}

std::vector<uint8_t> build_header(const std::vector<nq_entry> &plan, uint64_t data_offset,
                                  uint64_t data_bytes) {
    std::vector<uint8_t> h;
    h.insert(h.end(), {'N', 'Q', '0', '1'});
    put_u32(h, (uint32_t)plan.size());
    put_u64(h, data_offset);
    put_u64(h, data_bytes);
    for (const auto &e : plan) {
        put_u32(h, (uint32_t)e.name.size());
        h.insert(h.end(), e.name.begin(), e.name.end());
        put_u32(h, (uint32_t)e.type);
        put_u32(h, (uint32_t)e.ne.size());
        for (uint64_t d : e.ne) put_u64(h, d);
        put_u64(h, e.offset);
        put_u64(h, e.nbytes);
    }
    return h;
}

} /* namespace */

struct nq_writer {
    int                   fd = -1;
    std::vector<nq_entry> plan;
    uint64_t              data_offset = 0;
    std::vector<bool>     written;
    std::string           path;
};

nq_writer *nq_write_begin(const char *path, const std::vector<nq_entry> &plan_in) {
    nq_writer *w = new nq_writer();
    w->plan = plan_in;
    w->path = path;

    /* The header length does not depend on the offsets, so lay it out once with
       offset 0 to measure it, then fix data_offset. */
    uint64_t off = 0;
    for (auto &e : w->plan) {
        e.offset = off;
        off = nq_align_up(off + e.nbytes);
    }
    const uint64_t data_bytes = off;

    std::vector<uint8_t> h = build_header(w->plan, 0, data_bytes);
    w->data_offset = nq_align_up((uint64_t)h.size());
    h = build_header(w->plan, w->data_offset, data_bytes);

    w->fd = nq_open_write(path);
    if (w->fd < 0) {
        fprintf(stderr, "nano-quant: %s: %s\n", path, strerror(errno));
        delete w;
        return nullptr;
    }
    h.resize((size_t)w->data_offset, 0);
    if (nq_write_at(w->fd, h.data(), h.size(), 0) != 0) {
        fprintf(stderr, "nano-quant: cannot write the file header\n");
        nq_close(w->fd);
        delete w;
        return nullptr;
    }
    /* Give the file its full length in one go by writing the last byte of the
       data area; the bytes before it read as zero, so the alignment gaps are
       zero without being written. */
    const uint8_t zero = 0;
    if (data_bytes > 0 && nq_write_at(w->fd, &zero, 1, w->data_offset + data_bytes - 1) != 0) {
        fprintf(stderr, "nano-quant: cannot lengthen %s: %s\n", path, strerror(errno));
        nq_close(w->fd);
        delete w;
        return nullptr;
    }
    w->written.assign(w->plan.size(), false);
    return w;
}

int nq_write_chunk(nq_writer *w, int i, uint64_t byte_off, const uint8_t *data, uint64_t n) {
    if (i < 0 || i >= (int)w->plan.size()) return -1;
    const nq_entry &e = w->plan[(size_t)i];
    if (byte_off + n > e.nbytes) {
        fprintf(stderr, "nano-quant: %s: write to [%llu, %llu) past its %llu bytes\n",
                e.name.c_str(), (unsigned long long)byte_off,
                (unsigned long long)(byte_off + n), (unsigned long long)e.nbytes);
        return -1;
    }
    if (nq_write_at(w->fd, data, (size_t)n, w->data_offset + e.offset + byte_off) != 0) {
        fprintf(stderr, "nano-quant: cannot write %s\n", e.name.c_str());
        return -1;
    }
    return 0;
}

void nq_write_done(nq_writer *w, int i) {
    if (i >= 0 && i < (int)w->plan.size()) w->written[(size_t)i] = true;
}

int nq_write_tensor(nq_writer *w, int i, const uint8_t *data, uint64_t nbytes) {
    if (i < 0 || i >= (int)w->plan.size()) return -1;
    const nq_entry &e = w->plan[(size_t)i];
    if (nbytes != e.nbytes) {
        fprintf(stderr, "nano-quant: %s: expected %llu bytes, got %llu\n",
                e.name.c_str(), (unsigned long long)e.nbytes, (unsigned long long)nbytes);
        return -1;
    }
    if (nq_write_chunk(w, i, 0, data, nbytes) != 0) return -1;
    nq_write_done(w, i);
    return 0;
}

int nq_write_end(nq_writer *w) {
    int rc = 0;
    for (size_t i = 0; i < w->plan.size(); i++) {
        if (!w->written[i]) {
            fprintf(stderr, "nano-quant: %s was never written\n", w->plan[i].name.c_str());
            rc = -1;
        }
    }
    if (nq_close(w->fd) != 0) rc = -1;
    delete w;
    return rc;
}

int nq_read_index(const char *path, nq_index *idx) {
    int fd = nq_open_read(path);
    if (fd < 0) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); return -1; }

    uint8_t head[24];
    if (nq_read_at(fd, head, 24, 0) != 0 || memcmp(head, "NQ01", 4) != 0) {
        fprintf(stderr, "%s: not a .nq file\n", path);
        nq_close(fd);
        return -1;
    }
    const uint32_t n = get_u32(head + 4);
    idx->data_offset = get_u64(head + 8);
    idx->data_bytes  = get_u64(head + 16);

    std::vector<uint8_t> buf(idx->data_offset);
    if (nq_read_at(fd, buf.data(), buf.size(), 0) != 0) {
        fprintf(stderr, "%s: header is incomplete\n", path);
        nq_close(fd);
        return -1;
    }
    nq_close(fd);

    size_t p = 24;
    idx->entries.clear();
    for (uint32_t k = 0; k < n; k++) {
        if (p + 4 > buf.size()) return -1;
        uint32_t nl = get_u32(&buf[p]); p += 4;
        if (p + nl + 8 > buf.size()) return -1;
        nq_entry e;
        e.name.assign((const char *)&buf[p], nl); p += nl;
        e.type = (nq_type)get_u32(&buf[p]); p += 4;
        uint32_t nd = get_u32(&buf[p]); p += 4;
        for (uint32_t d = 0; d < nd; d++) { e.ne.push_back(get_u64(&buf[p])); p += 8; }
        e.offset = get_u64(&buf[p]); p += 8;
        e.nbytes = get_u64(&buf[p]); p += 8;
        idx->entries.push_back(std::move(e));
    }
    return 0;
}
