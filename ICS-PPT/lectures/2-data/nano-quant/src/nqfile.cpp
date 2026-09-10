/* .nq 容器的读写。格式说明见 nq.h。
 *
 * 这一层刻意做得薄：文件头只是一张目录，数据区是量化结果原样拼接，
 * 于是「数据区的 md5」就等于「量化做得对不对」的全部内容。 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

    /* 先按 offset = 0 排一遍算出头长，再定 data_offset，头长不随之改变。 */
    uint64_t off = 0;
    for (auto &e : w->plan) {
        e.offset = off;
        off = nq_align_up(off + e.nbytes);
    }
    const uint64_t data_bytes = off;

    std::vector<uint8_t> h = build_header(w->plan, 0, data_bytes);
    w->data_offset = nq_align_up((uint64_t)h.size());
    h = build_header(w->plan, w->data_offset, data_bytes);

    w->fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (w->fd < 0) {
        fprintf(stderr, "nano-quant: %s: %s\n", path, strerror(errno));
        delete w;
        return nullptr;
    }
    std::vector<uint8_t> pad(w->data_offset - h.size(), 0);
    if (write(w->fd, h.data(), h.size()) != (ssize_t)h.size() ||
        (!pad.empty() && write(w->fd, pad.data(), pad.size()) != (ssize_t)pad.size())) {
        fprintf(stderr, "nano-quant: 写文件头失败\n");
        close(w->fd);
        delete w;
        return nullptr;
    }
    /* 数据区一次性开好，块间的对齐空隙留零。 */
    if (ftruncate(w->fd, (off_t)(w->data_offset + data_bytes)) != 0) {
        fprintf(stderr, "nano-quant: ftruncate: %s\n", strerror(errno));
        close(w->fd);
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
        fprintf(stderr, "nano-quant: %s 越界写 [%llu, %llu)，只有 %llu 字节\n",
                e.name.c_str(), (unsigned long long)byte_off,
                (unsigned long long)(byte_off + n), (unsigned long long)e.nbytes);
        return -1;
    }
    uint64_t done = 0;
    while (done < n) {
        ssize_t k = pwrite(w->fd, data + done, (size_t)(n - done),
                           (off_t)(w->data_offset + e.offset + byte_off + done));
        if (k <= 0) { fprintf(stderr, "nano-quant: 写 %s 失败\n", e.name.c_str()); return -1; }
        done += (uint64_t)k;
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
        fprintf(stderr, "nano-quant: %s 预定 %llu 字节，实到 %llu 字节\n",
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
            fprintf(stderr, "nano-quant: %s 没有写入\n", w->plan[i].name.c_str());
            rc = -1;
        }
    }
    if (close(w->fd) != 0) rc = -1;
    delete w;
    return rc;
}

int nq_read_index(const char *path, nq_index *idx) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); return -1; }

    uint8_t head[24];
    if (pread(fd, head, 24, 0) != 24 || memcmp(head, "NQ01", 4) != 0) {
        fprintf(stderr, "%s: 不是 .nq 文件\n", path);
        close(fd);
        return -1;
    }
    const uint32_t n = get_u32(head + 4);
    idx->data_offset = get_u64(head + 8);
    idx->data_bytes  = get_u64(head + 16);

    std::vector<uint8_t> buf(idx->data_offset);
    if (pread(fd, buf.data(), buf.size(), 0) != (ssize_t)buf.size()) {
        fprintf(stderr, "%s: 头读不全\n", path);
        close(fd);
        return -1;
    }
    close(fd);

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
