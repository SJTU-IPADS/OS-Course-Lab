/* nq2gguf -- add metadata to a .nq and assemble a GGUF that llama.cpp reads.
 *
 *   nq2gguf out.nq --meta meta.kv -o model.gguf
 *
 * meta.kv is made by tools/mkmeta.py from the config.json and tokenizer.json
 * of the Hugging Face repository. It is already GGUF key-value encoding and is
 * copied as is; two keys about the quantization are added.
 *
 * GGUF layout:
 *   'GGUF' | u32 version | u64 tensors | u64 key-value pairs | pairs... |
 *   tensor directory... | padding | tensor data
 * Offsets in the directory are relative to the data area, which starts on a
 * multiple of general.alignment. */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* GGUF file_type numbers, as in llama.cpp's llama_ftype. */
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
    int fd = nq_open_read(path);
    if (fd < 0) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); return -1; }
    int64_t sz = nq_file_size(fd);
    if (sz < 0) { nq_close(fd); return -1; }
    out->resize((size_t)sz);
    int rc = nq_read_at(fd, out->data(), out->size(), 0);
    nq_close(fd);
    return rc;
}

int copy_range(int in_fd, uint64_t in_off, int out_fd, uint64_t out_off, uint64_t len) {
    std::vector<uint8_t> buf(1 << 20);
    uint64_t done = 0;
    while (done < len) {
        size_t want = buf.size();
        if (want > len - done) want = (size_t)(len - done);
        if (nq_read_at(in_fd, buf.data(), want, in_off + done) != 0) return -1;
        if (nq_write_at(out_fd, buf.data(), want, out_off + done) != 0) return -1;
        done += want;
    }
    return 0;
}

} /* namespace */

int main(int argc, char **argv) {
    nq_stdout_binary();
    if (argc < 2) {
        fprintf(stderr, "usage: nq2gguf <in.nq> --meta <meta.kv> -o <out.gguf>\n");
        return 2;
    }
    const char *in = argv[1], *meta_path = nullptr, *out = nullptr;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--meta") && i + 1 < argc) meta_path = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) out = argv[++i];
        else { fprintf(stderr, "nq2gguf: unknown argument %s\n", argv[i]); return 2; }
    }
    if (!meta_path || !out) {
        fprintf(stderr, "nq2gguf: needs --meta and -o\n");
        return 2;
    }

    nq_index idx;
    if (nq_read_index(in, &idx) != 0) return 1;

    std::vector<uint8_t> meta;
    if (read_all(meta_path, &meta) != 0) return 1;
    if (meta.size() < 16 || memcmp(meta.data(), "NQKV", 4) != 0) {
        fprintf(stderr, "%s: not metadata made by mkmeta.py\n", meta_path);
        return 1;
    }
    uint32_t meta_kv = 0;
    for (int i = 0; i < 4; i++) meta_kv |= (uint32_t)meta[4 + i] << (8 * i);
    uint64_t meta_bytes = 0;
    for (int i = 7; i >= 0; i--) meta_bytes = (meta_bytes << 8) | meta[8 + (size_t)i];
    if (16 + meta_bytes != meta.size()) {
        fprintf(stderr, "%s: length field says %" PRIu64 ", file has %zu bytes\n",
                meta_path, meta_bytes, meta.size());
        return 1;
    }

    /* the two added keys */
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
        put_u64(head, e.offset);          /* .nq and GGUF both align to 32: same offsets */
    }

    const uint64_t data_start = nq_align_up((uint64_t)head.size());
    head.resize((size_t)data_start, 0);

    int ofd = nq_open_write(out);
    if (ofd < 0) { fprintf(stderr, "%s: %s\n", out, strerror(errno)); return 1; }
    if (nq_write_at(ofd, head.data(), head.size(), 0) != 0) {
        fprintf(stderr, "nq2gguf: cannot write the header\n");
        return 1;
    }

    int ifd = nq_open_read(in);
    if (ifd < 0) { fprintf(stderr, "%s: %s\n", in, strerror(errno)); return 1; }
    if (copy_range(ifd, idx.data_offset, ofd, data_start, idx.data_bytes) != 0) {
        fprintf(stderr, "nq2gguf: cannot copy the data area\n");
        return 1;
    }
    nq_close(ifd);
    if (nq_close(ofd) != 0) { fprintf(stderr, "nq2gguf: cannot close the output file\n"); return 1; }

    fprintf(stderr, "%s: %zu tensors, %" PRIu32 " key-value pairs, header %" PRIu64 " bytes, "
            "data %" PRIu64 " bytes\n",
            out, idx.entries.size(), meta_kv + extra_kv, data_start, idx.data_bytes);
    printf("%" PRIu64 "\n", data_start + idx.data_bytes);
    return 0;
}
