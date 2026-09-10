/* nano-quant —— 读 safetensors，按配方量化，写 .nq。
 *
 *   nano-quant plan  model.safetensors [--recipe q4_k_m]
 *   nano-quant quant model.safetensors [--recipe q4_k_m] -o out.nq
 *
 * plan 只把清单打到标准输出，一行一个张量：
 *     名字 类型 各维长度… 字节数
 * 于是账目可以直接接给别的程序：
 *     nano-quant plan m.safetensors | awk '{s += $NF} END {print s}'
 * 汇总走标准错误，不混进管道。 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nq.h"

namespace {

struct job {
    int         st_index;
    std::string hf_name;
    std::string gg_name;
    nq_type     type;
    uint64_t    n_elem;
    std::vector<uint64_t> ne;      /* GGUF 顺序 */
};

void usage() {
    fprintf(stderr,
        "用法：\n"
        "  nano-quant plan  <model.safetensors> [--recipe q4_0|q4_1|q4_k|q4_k_m]\n"
        "  nano-quant quant <model.safetensors> [--recipe ...] -o <out.nq> [--limit N]\n");
}

/* 建清单：过滤、改名、定类型、算字节数。顺序就是 safetensors 头里的顺序。 */
int build_jobs(const st_file &f, nq_recipe r, std::vector<job> *out) {
    int n_layers = 0, n_selected = 0;
    for (int i = 0; i < st_count(&f); i++) {
        const st_tensor *t = st_get(&f, i);
        if (!tensor_selected(t->name.c_str())) continue;
        n_selected++;
        int l = layer_of(t->name.c_str());
        if (l + 1 > n_layers) n_layers = l + 1;
    }
    if (n_selected == 0) {
        fprintf(stderr, "nano-quant: 一个语言模型张量都没选中，检查 tensor_selected\n");
        return -1;
    }

    for (int i = 0; i < st_count(&f); i++) {
        const st_tensor *t = st_get(&f, i);
        if (!tensor_selected(t->name.c_str())) continue;

        char buf[256];
        if (gguf_name(t->name.c_str(), buf, sizeof buf) != 0) {
            fprintf(stderr, "nano-quant: 不认识的张量名 %s\n", t->name.c_str());
            return -1;
        }

        job j;
        j.st_index = i;
        j.hf_name  = t->name;
        j.gg_name  = buf;
        j.n_elem   = t->n_elem;
        j.type     = recipe_pick(r, t->name.c_str(), (int)t->shape.size(),
                                 layer_of(t->name.c_str()), n_layers);

        /* safetensors 的 shape 行优先，GGUF 的 ne 第 0 维变化最快，两者相反。 */
        for (size_t k = t->shape.size(); k-- > 0; ) j.ne.push_back(t->shape[k]);

        const nq_type_info *ti = nq_type_of(j.type);
        if (j.ne.empty() || j.ne[0] % (uint64_t)ti->block_elems != 0) {
            fprintf(stderr, "nano-quant: %s 的行长 %llu 不是 %s 块长 %d 的整数倍\n",
                    j.gg_name.c_str(), (unsigned long long)(j.ne.empty() ? 0 : j.ne[0]),
                    ti->name, ti->block_elems);
            return -1;
        }
        out->push_back(std::move(j));
    }
    return n_layers;
}

void report(const st_file &f, const std::vector<job> &jobs, int n_layers, nq_recipe r,
            uint64_t src_bytes) {
    uint64_t total = 0, n_w = 0;
    for (const auto &j : jobs) { total += nq_type_bytes(j.type, j.n_elem); n_w += j.n_elem; }

    int      n_skip = 0;
    uint64_t b_skip = 0;
    for (int i = 0; i < st_count(&f); i++) {
        const st_tensor *t = st_get(&f, i);
        if (tensor_selected(t->name.c_str())) continue;
        n_skip++;
        b_skip += t->end - t->beg;
    }

    fprintf(stderr, "配方 %s，%d 层，%zu 个张量，%" PRIu64 " 个权重\n",
            nq_recipe_name(r), n_layers, jobs.size(), n_w);
    fprintf(stderr, "源 %" PRIu64 " 字节，产物张量数据 %" PRIu64 " 字节，"
            "平均 %.4f 位/权重，压缩 %.3f 倍\n",
            src_bytes, total, 8.0 * (double)total / (double)n_w,
            (double)src_bytes / (double)total);
    fprintf(stderr, "跳过 %d 个张量，%" PRIu64 " 字节\n", n_skip, b_skip);
}

} /* namespace */

int main(int argc, char **argv) {
    if (argc < 3) { usage(); return 2; }

    const char *cmd  = argv[1];
    const char *path = argv[2];
    const char *out  = nullptr;
    nq_recipe   r    = NQ_RECIPE_Q4_K_M;
    long        limit = -1;

    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--recipe") && i + 1 < argc) {
            if (nq_recipe_parse(argv[++i], &r) != 0) {
                fprintf(stderr, "nano-quant: 没有这个配方：%s\n", argv[i]);
                return 2;
            }
        } else if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            out = argv[++i];
        } else if (!strcmp(argv[i], "--limit") && i + 1 < argc) {
            limit = strtol(argv[++i], nullptr, 10);
        } else {
            usage();
            return 2;
        }
    }

    st_file f;
    if (st_open(&f, path) != 0) {
        fprintf(stderr, "nano-quant: %s\n", f.err.c_str());
        return 1;
    }

    std::vector<job> jobs;
    int n_layers = build_jobs(f, r, &jobs);
    if (n_layers < 0) return 1;
    if (limit >= 0 && (size_t)limit < jobs.size()) jobs.resize((size_t)limit);

    uint64_t src_bytes = 0;
    for (const auto &j : jobs) src_bytes += j.n_elem * st_get(&f, j.st_index)->elem_size;

    if (!strcmp(cmd, "plan")) {
        for (const auto &j : jobs) {
            printf("%s %s", j.gg_name.c_str(), nq_type_of(j.type)->name);
            for (uint64_t d : j.ne) printf(" %" PRIu64, d);
            printf(" %" PRIu64 "\n", nq_type_bytes(j.type, j.n_elem));
        }
        report(f, jobs, n_layers, r, src_bytes);
        st_close(&f);
        return 0;
    }

    if (strcmp(cmd, "quant") != 0) { usage(); return 2; }
    if (!out) { fprintf(stderr, "nano-quant: quant 需要 -o\n"); return 2; }

    std::vector<nq_entry> plan;
    for (const auto &j : jobs) {
        nq_entry e;
        e.name   = j.gg_name;
        e.type   = j.type;
        e.ne     = j.ne;
        e.offset = 0;
        e.nbytes = nq_type_bytes(j.type, j.n_elem);
        plan.push_back(std::move(e));
    }

    nq_writer *w = nq_write_begin(out, plan);
    if (!w) return 1;

    /* 分段处理，峰值内存只与段长有关，与最大的张量无关。
       段长取 4 Mi 个元素，正好是所有块长的整数倍。 */
    const uint64_t CHUNK = 4u << 20;
    std::vector<float>   src(CHUNK);
    std::vector<uint8_t> dst;

    for (size_t i = 0; i < jobs.size(); i++) {
        const job &j = jobs[i];
        const nq_type_info *ti = nq_type_of(j.type);
        const uint64_t row = j.ne[0];
        uint64_t step = CHUNK - CHUNK % row;          /* 段长取整到整行 */
        if (step == 0) step = row;
        dst.resize((size_t)nq_type_bytes(j.type, step));

        uint64_t done = 0, obytes = 0;
        while (done < j.n_elem) {
            uint64_t n = j.n_elem - done;
            if (n > step) n = step;
            if (st_read_f32(&f, st_get(&f, j.st_index), done, src.data(), (size_t)n) != n) {
                fprintf(stderr, "nano-quant: 读 %s 失败\n", j.hf_name.c_str());
                return 1;
            }
            nq_quantize(j.type, src.data(), n, dst.data());
            const uint64_t nb = nq_type_bytes(j.type, n);
            if (nq_write_chunk(w, (int)i, obytes, dst.data(), nb) != 0) return 1;
            done   += n;
            obytes += nb;
        }
        nq_write_done(w, (int)i);
        if (isatty(2)) {
            fprintf(stderr, "\r%zu/%zu %-40s", i + 1, jobs.size(), j.gg_name.c_str());
            fflush(stderr);
        }
        (void)ti;
    }
    if (isatty(2)) fprintf(stderr, "\r%-60s\r", "");

    if (nq_write_end(w) != 0) return 1;
    st_close(&f);

    nq_index idx;
    if (nq_read_index(out, &idx) != 0) return 1;
    std::string md5 = nq_md5_file_range(out, idx.data_offset, idx.data_bytes);

    report(f, jobs, n_layers, r, src_bytes);
    printf("%s  %s 数据区 %" PRIu64 " 字节\n", md5.c_str(), out, idx.data_bytes);
    return 0;
}
