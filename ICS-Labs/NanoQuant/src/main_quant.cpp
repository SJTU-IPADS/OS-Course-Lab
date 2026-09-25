/* nano-quant -- read a safetensors file, quantize it by a recipe, write a .nq.
 *
 *   nano-quant plan  model.safetensors [--recipe q4_k_m]
 *   nano-quant quant model.safetensors [--recipe q4_k_m] -o out.nq
 *
 * plan prints the plan on standard output, one tensor per line:
 *     name type dim... bytes
 * so the plan can be the input of another program:
 *     nano-quant plan m.safetensors | awk '{s += $NF} END {print s}'
 * The summary goes to standard error. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nq.h"

namespace {

struct job {
    int         st_index;
    std::string hf_name;
    std::string gg_name;
    nq_type     type;
    uint64_t    n_elem;
    std::vector<uint64_t> ne;      /* GGUF order */
};

void usage() {
    fprintf(stderr,
        "usage:\n"
        "  nano-quant plan  <model.safetensors> [--recipe q4_0|q4_1|q4_k|q4_k_m]\n"
        "  nano-quant quant <model.safetensors> [--recipe ...] -o <out.nq> [--limit N]\n");
}

/* Build the plan: select, rename, pick a type, count bytes. The order is the
   order of the safetensors header. */
int build_jobs(const st_file &f, nq_recipe r, std::vector<job> *out) {
    int n_layers = 0, n_selected = 0;
    for (int i = 0; i < st_count(&f); i++) {
        const st_tensor *t = st_get(&f, i);
        if (!nq_tensor_selected(t->name.c_str())) continue;
        n_selected++;
        int l = nq_layer_of(t->name.c_str());
        if (l + 1 > n_layers) n_layers = l + 1;
    }
    if (n_selected == 0) {
        fprintf(stderr, "nano-quant: the file has no language model tensor\n");
        return -1;
    }

    for (int i = 0; i < st_count(&f); i++) {
        const st_tensor *t = st_get(&f, i);
        if (!nq_tensor_selected(t->name.c_str())) continue;

        char buf[256];
        if (nq_gguf_name(t->name.c_str(), buf, sizeof buf) != 0) {
            fprintf(stderr, "nano-quant: unknown tensor name %s\n", t->name.c_str());
            return -1;
        }

        job j;
        j.st_index = i;
        j.hf_name  = t->name;
        j.gg_name  = buf;
        j.n_elem   = t->n_elem;
        j.type     = nq_recipe_pick(r, t->name.c_str(), (int)t->shape.size(),
                                    nq_layer_of(t->name.c_str()), n_layers);

        /* safetensors shape is row major, GGUF ne has dimension 0 fastest: reversed. */
        for (size_t k = t->shape.size(); k-- > 0; ) j.ne.push_back(t->shape[k]);

        const nq_type_info *ti = nq_type_of(j.type);
        if (j.ne.empty() || j.ne[0] % (uint64_t)ti->block_elems != 0) {
            fprintf(stderr, "nano-quant: %s: row length %llu is not a multiple of the %s block length %d\n",
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
        if (nq_tensor_selected(t->name.c_str())) continue;
        n_skip++;
        b_skip += t->end - t->beg;
    }

    fprintf(stderr, "recipe %s, %d layers, %zu tensors, %" PRIu64 " weights\n",
            nq_recipe_name(r), n_layers, jobs.size(), n_w);
    fprintf(stderr, "source %" PRIu64 " bytes, output tensor data %" PRIu64 " bytes, "
            "%.4f bits/weight, %.3fx smaller\n",
            src_bytes, total, 8.0 * (double)total / (double)n_w,
            (double)src_bytes / (double)total);
    fprintf(stderr, "skipped %d tensors, %" PRIu64 " bytes\n", n_skip, b_skip);
}

} /* namespace */

int main(int argc, char **argv) {
    nq_stdout_binary();
    if (argc < 3) { usage(); return 2; }

    const char *cmd  = argv[1];
    const char *path = argv[2];
    const char *out  = nullptr;
    nq_recipe   r    = NQ_RECIPE_Q4_K_M;
    long        limit = -1;

    for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--recipe") && i + 1 < argc) {
            if (nq_recipe_parse(argv[++i], &r) != 0) {
                fprintf(stderr, "nano-quant: no recipe named %s\n", argv[i]);
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
    if (!out) { fprintf(stderr, "nano-quant: quant needs -o\n"); return 2; }

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

    /* Work in chunks, so peak memory depends on the chunk length alone.
       4 Mi elements is a multiple of every block length. */
    const uint64_t CHUNK = 4u << 20;
    std::vector<float>   src(CHUNK);
    std::vector<uint8_t> dst;

    for (size_t i = 0; i < jobs.size(); i++) {
        const job &j = jobs[i];
        const uint64_t row = j.ne[0];
        uint64_t step = CHUNK - CHUNK % row;          /* whole rows per chunk */
        if (step == 0) step = row;
        dst.resize((size_t)nq_type_bytes(j.type, step));

        uint64_t done = 0, obytes = 0;
        while (done < j.n_elem) {
            uint64_t n = j.n_elem - done;
            if (n > step) n = step;
            if (st_read_f32(&f, st_get(&f, j.st_index), done, src.data(), (size_t)n) != n) {
                fprintf(stderr, "nano-quant: cannot read %s\n", j.hf_name.c_str());
                return 1;
            }
            nq_quantize(j.type, src.data(), n, dst.data());
            const uint64_t nb = nq_type_bytes(j.type, n);
            if (nq_write_chunk(w, (int)i, obytes, dst.data(), nb) != 0) return 1;
            done   += n;
            obytes += nb;
        }
        nq_write_done(w, (int)i);
        if (nq_is_terminal(2)) {
            fprintf(stderr, "\r%zu/%zu %-40s", i + 1, jobs.size(), j.gg_name.c_str());
            fflush(stderr);
        }
    }
    if (nq_is_terminal(2)) fprintf(stderr, "\r%-60s\r", "");

    if (nq_write_end(w) != 0) return 1;
    st_close(&f);

    nq_index idx;
    if (nq_read_index(out, &idx) != 0) return 1;
    std::string md5 = nq_md5_file_range(out, idx.data_offset, idx.data_bytes);

    report(f, jobs, n_layers, r, src_bytes);
    printf("%s  %s  %" PRIu64 "\n", md5.c_str(), out, idx.data_bytes);
    return 0;
}
