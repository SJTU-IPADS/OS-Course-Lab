/* nq2gguf —— 把 .nq 加上一份元数据，拼成 llama.cpp 能读的 GGUF。
 *
 *   nq2gguf out.nq --meta meta.kv -o model.gguf
 *
 * meta.kv 由 tools/mkmeta.py 从 Hugging Face 上的 config.json 与 tokenizer.json
 * 生成，里面已经是 GGUF 的键值编码，这里原样搬进去，另外补两个与量化有关的键。
 *
 * GGUF 的排布：
 *   'GGUF' | u32 版本 | u64 张量数 | u64 键值对数 | 键值对… | 张量目录… | 对齐 | 张量数据
 * 张量目录里的 offset 相对数据区起点，数据区起点对齐到 general.alignment。 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
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
void put_str(std::vector<uint8_t> &v, const std::string &s) {
    put_u64(v, s.size());
    v.insert(v.end(), s.begin(), s.end());
}
void put_kv_u32(std::vector<uint8_t> &v, const char *k, uint32_t x) {
    put_str(v, k);
    put_u32(v, 4);            /* GGUF_TYPE_UINT32 */
    put_u32(v, x);
}

/* GGUF 的 file_type 编号，与 llama.cpp 的 llama_ftype 一致。 */
uint32_t ftype_of(const nq_index &idx) {
    bool q4_0 = false, q4_1 = false, q4_k = false, q6_k = false;
    for (const auto &e : idx.entries) {
        switch (e.type) {
            case NQ_TYPE_Q4_0: q4_0 = true; break;
            case NQ_TYPE_Q4_1: q4_1 = true; break;
            case NQ_TYPE_Q4_K: q4_k = true; break;
            case NQ_TYPE_Q6_K: q6_k = true; break;
            default: break;
        }
    }
    if (q4_0) return 2;                       /* MOSTLY_Q4_0 */
    if (q4_1) return 3;                       /* MOSTLY_Q4_1 */
    if (q4_k) return q6_k ? 15 : 14;          /* MOSTLY_Q4_K_M / _S */
    return 1;                                 /* MOSTLY_F16 */
}

int read_all(const char *path, std::vector<uint8_t> *out) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); return -1; }
    off_t sz = lseek(fd, 0, SEEK_END);
    out->resize((size_t)sz);
    size_t done = 0;
    while (done < out->size()) {
        ssize_t n = pread(fd, out->data() + done, out->size() - done, (off_t)done);
        if (n <= 0) { close(fd); return -1; }
        done += (size_t)n;
    }
    close(fd);
    return 0;
}

int copy_range(int in_fd, uint64_t in_off, int out_fd, uint64_t out_off, uint64_t len) {
    std::vector<uint8_t> buf(1 << 20);
    uint64_t done = 0;
    while (done < len) {
        size_t want = buf.size();
        if (want > len - done) want = (size_t)(len - done);
        ssize_t got = pread(in_fd, buf.data(), want, (off_t)(in_off + done));
        if (got <= 0) return -1;
        ssize_t put = 0;
        while (put < got) {
            ssize_t k = pwrite(out_fd, buf.data() + put, (size_t)(got - put),
                               (off_t)(out_off + done + (uint64_t)put));
            if (k <= 0) return -1;
            put += k;
        }
        done += (uint64_t)got;
    }
    return 0;
}

} /* namespace */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "用法：nq2gguf <in.nq> --meta <meta.kv> -o <out.gguf>\n");
        return 2;
    }
    const char *in = argv[1], *meta_path = nullptr, *out = nullptr;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--meta") && i + 1 < argc) meta_path = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) out = argv[++i];
        else { fprintf(stderr, "不认识的参数：%s\n", argv[i]); return 2; }
    }
    if (!meta_path || !out) {
        fprintf(stderr, "nq2gguf: 需要 --meta 与 -o\n");
        return 2;
    }

    nq_index idx;
    if (nq_read_index(in, &idx) != 0) return 1;

    std::vector<uint8_t> meta;
    if (read_all(meta_path, &meta) != 0) return 1;
    if (meta.size() < 16 || memcmp(meta.data(), "NQKV", 4) != 0) {
        fprintf(stderr, "%s: 不是 mkmeta.py 生成的元数据\n", meta_path);
        return 1;
    }
    uint32_t meta_kv = 0;
    for (int i = 0; i < 4; i++) meta_kv |= (uint32_t)meta[4 + i] << (8 * i);
    uint64_t meta_bytes = 0;
    for (int i = 7; i >= 0; i--) meta_bytes = (meta_bytes << 8) | meta[8 + (size_t)i];
    if (16 + meta_bytes != meta.size()) {
        fprintf(stderr, "%s: 长度字段 %" PRIu64 " 与文件 %zu 字节不符\n",
                meta_path, meta_bytes, meta.size());
        return 1;
    }

    /* 自己补的两个键 */
    std::vector<uint8_t> extra;
    put_kv_u32(extra, "general.file_type", ftype_of(idx));
    put_kv_u32(extra, "general.quantization_version", 2);
    const uint32_t extra_kv = 2;

    std::vector<uint8_t> head;
    head.insert(head.end(), {'G', 'G', 'U', 'F'});
    put_u32(head, 3);
    put_u64(head, idx.entries.size());
    put_u64(head, (uint64_t)meta_kv + extra_kv);
    head.insert(head.end(), meta.begin() + 16, meta.end());
    head.insert(head.end(), extra.begin(), extra.end());

    for (const auto &e : idx.entries) {
        put_str(head, e.name);
        put_u32(head, (uint32_t)e.ne.size());
        for (uint64_t d : e.ne) put_u64(head, d);
        put_u32(head, (uint32_t)e.type);
        put_u64(head, e.offset);          /* .nq 与 GGUF 都按 32 对齐，偏移可直接沿用 */
    }

    const uint64_t data_start = nq_align_up((uint64_t)head.size());
    head.resize((size_t)data_start, 0);

    int ofd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ofd < 0) { fprintf(stderr, "%s: %s\n", out, strerror(errno)); return 1; }
    if (write(ofd, head.data(), head.size()) != (ssize_t)head.size()) {
        fprintf(stderr, "nq2gguf: 写头失败\n");
        return 1;
    }
    if (ftruncate(ofd, (off_t)(data_start + idx.data_bytes)) != 0) {
        fprintf(stderr, "nq2gguf: ftruncate: %s\n", strerror(errno));
        return 1;
    }

    int ifd = open(in, O_RDONLY);
    if (ifd < 0) { fprintf(stderr, "%s: %s\n", in, strerror(errno)); return 1; }
    if (copy_range(ifd, idx.data_offset, ofd, data_start, idx.data_bytes) != 0) {
        fprintf(stderr, "nq2gguf: 搬数据区失败\n");
        return 1;
    }
    close(ifd);
    if (close(ofd) != 0) { fprintf(stderr, "nq2gguf: 收尾失败\n"); return 1; }

    fprintf(stderr, "%s：%zu 个张量，%" PRIu32 " 个键值对，头 %" PRIu64 " 字节，"
            "数据 %" PRIu64 " 字节\n",
            out, idx.entries.size(), meta_kv + extra_kv, data_start, idx.data_bytes);
    printf("%" PRIu64 "\n", data_start + idx.data_bytes);
    return 0;
}
