/* nq-selftest -- checks parts A and B; needs no model file.
 *
 *   ./nq-selftest              run every group, compare with tests/expected-blocks.txt
 *   ./nq-selftest a q4_0       run only these groups: a scale_min q4_0 q4_1 q4_k q6_k
 *   ./nq-selftest --emit       print this program's results in the format of
 *                              expected-blocks.txt, compare with nothing
 *   ./nq-selftest --full       check f32_to_fp16 on all 2^32 floats
 *
 * One line per item: its name, its digest or "ok", and whether that matches. */
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "nq.h"

namespace {

/* Fixed-seed pseudo-random numbers: the same sequence on every machine. */
struct lcg {
    uint64_t s;
    explicit lcg(uint64_t seed) : s(seed) {}
    uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return (uint32_t)(s >> 32); }
    float uniform() { return (float)(next() >> 8) / (float)(1 << 24); }   /* [0, 1) */
};

uint32_t bits_of(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

/* Test weights: 8 constructed super-blocks, then 504 random ones with
   scales from 1e-4 to 1e2, of both signs. */
std::vector<float> make_input() {
    const int NSB = 512;
    std::vector<float> x((size_t)NSB * 256);

    for (int i = 0; i < 256; i++) x[i] = 0.0f;                              /* all zero */
    for (int i = 0; i < 256; i++) x[256 + i] = 0.375f;                      /* all equal */
    for (int i = 0; i < 256; i++) x[512 + i] = (i == 7) ? 100.0f : 1e-4f;   /* one outlier */
    for (int i = 0; i < 256; i++) x[768 + i] = -0.05f - 0.001f * (float)i;  /* all negative */
    for (int i = 0; i < 256; i++) x[1024 + i] = 0.05f + 0.001f * (float)i;  /* all positive */
    for (int i = 0; i < 256; i++) x[1280 + i] = (i & 1) ? 1e-7f : -1e-7f;   /* tiny */
    for (int i = 0; i < 256; i++) x[1536 + i] = (float)(i - 128) * 0.5f;    /* symmetric about 0 */
    for (int i = 0; i < 256; i++) x[1792 + i] = (i < 255) ? 0.0f : 3.0f;    /* one nonzero */

    lcg r(20260907);
    for (int b = 8; b < NSB; b++) {
        float sigma = powf(10.0f, -4.0f + 6.0f * r.uniform());              /* 1e-4 to 1e2 */
        for (int i = 0; i < 256; i++) {
            float u = r.uniform() * 2.0f - 1.0f;
            float v = r.uniform() * 2.0f - 1.0f;
            x[(size_t)b * 256 + i] = sigma * u * fabsf(v);
        }
    }
    /* truncate to BF16 and back: the values a real model holds */
    for (auto &v : x) v = nq_bf16_to_f32((uint16_t)(bits_of(v) >> 16));
    return x;
}

struct stat_t { double sse = 0, sxx = 0; float amax = 0; };

stat_t compare(const std::vector<float> &a, const std::vector<float> &b) {
    stat_t s;
    for (size_t i = 0; i < a.size(); i++) {
        double d = (double)a[i] - (double)b[i];
        s.sse += d * d;
        s.sxx += (double)a[i] * (double)a[i];
        if (fabsf((float)d) > s.amax) s.amax = fabsf((float)d);
    }
    return s;
}

std::map<std::string, std::string> load_expect(const char *path) {
    std::map<std::string, std::string> m;
    FILE *fp = fopen(path, "r");
    if (!fp) return m;
    char k[64], v[64];
    while (fscanf(fp, "%63s %63s", k, v) == 2) m[k] = v;
    fclose(fp);
    return m;
}

} /* namespace */

/* With no group named, every group runs; with names, only those, so each part
   can be checked as soon as it is written. */
static const char *GROUPS[] = { "a", "scale_min", "q4_0", "q4_1", "q4_k", "q6_k" };

int main(int argc, char **argv) {
    nq_stdout_binary();
    bool emit = false, full = false;
    const char *expath = "tests/expected-blocks.txt";
    std::set<std::string> want;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--emit")) emit = true;
        else if (!strcmp(argv[i], "--full")) full = true;
        else if (!strcmp(argv[i], "--expect") && i + 1 < argc) expath = argv[++i];
        else if (argv[i][0] != '-') {
            bool known = false;
            for (const char *g : GROUPS) if (!strcmp(argv[i], g)) known = true;
            if (!known) { fprintf(stderr, "nq-selftest: no group named %s\n", argv[i]); return 2; }
            want.insert(argv[i]);
        }
        else {
            fprintf(stderr, "usage: nq-selftest [--emit] [--full] [--expect FILE] [group...]\n"
                            "groups: a scale_min q4_0 q4_1 q4_k q6_k; none named runs them all\n");
            return 2;
        }
    }
    /* the 12 scale bytes are part of Q4_K, so q4_k also runs scale_min */
    if (want.count("q4_k")) want.insert("scale_min");
    auto run = [&](const char *g) { return want.empty() || want.count(g) != 0; };

    std::map<std::string, std::string> got;
    int bad = 0;

    /* ---- A1: rd_u64le ---- */
    if (run("a")) {
        const uint8_t p[8] = { 0x40, 0x29, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
        const uint8_t q[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
        const uint8_t z[8] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };
        bool ok = rd_u64le(p) == 76096ull
               && rd_u64le(q) == 0xffffffffffffffffull
               && rd_u64le(z) == 4294967296ull;
        got["rd_u64le"] = ok ? "ok" : "wrong";
        if (!ok) bad++;
    }

    /* ---- A2: bf16_to_f32 and fp16_to_f32 on all 65536 bit patterns ---- */
    if (run("a")) {
        long nb = 0, nf = 0;
        for (uint32_t h = 0; h < 65536; h++) {
            if (bits_of(bf16_to_f32((uint16_t)h)) != bits_of(nq_bf16_to_f32((uint16_t)h))) nb++;
            uint32_t a = bits_of(fp16_to_f32((uint16_t)h));
            uint32_t b = bits_of(nq_fp16_to_f32((uint16_t)h));
            /* a NaN may carry any sign and mantissa; both must be NaN */
            bool nan_a = (a & 0x7f800000u) == 0x7f800000u && (a & 0x7fffffu);
            bool nan_b = (b & 0x7f800000u) == 0x7f800000u && (b & 0x7fffffu);
            if (nan_a && nan_b) continue;
            if (a != b) nf++;
        }
        got["bf16_to_f32"] = nb ? "wrong" : "ok";
        got["fp16_to_f32"] = nf ? "wrong" : "ok";
        if (nb) bad++;
        if (nf) bad++;
    }

    /* ---- A3: f32_to_fp16 ---- */
    if (run("a")) {
        long n = 0;
        uint64_t step = full ? 1 : 251;          /* 251 is prime, so the steps drift across the exponent field */
        for (uint64_t u = 0; u < 0x100000000ull && n == 0; u += step) {
            float f;
            uint32_t bits = (uint32_t)u;
            memcpy(&f, &bits, 4);
            if (f != f) continue;                /* NaNs are not compared */
            if (f32_to_fp16(f) != nq_f32_to_fp16(f)) {
                fprintf(stderr, "f32_to_fp16(%.9g) (bits %08x) gives %04x, should be %04x\n",
                        (double)f, bits, f32_to_fp16(f), nq_f32_to_fp16(f));
                n++;
            }
        }
        /* every value half precision holds exactly must survive the round trip */
        for (uint32_t h = 0; h < 65536 && n == 0; h++) {
            if ((h & 0x7c00) == 0x7c00) continue;
            float f = nq_fp16_to_f32((uint16_t)h);
            if (f32_to_fp16(f) != (uint16_t)h) {
                fprintf(stderr, "f32_to_fp16 does not give back %04x after the round trip\n", h);
                n++;
            }
        }
        got["f32_to_fp16"] = n ? "wrong" : "ok";
        if (n) bad++;
    }

    /* ---- B1: put_scale_min writes pair j and nothing else ----
       The 12 bytes start with arbitrary contents and the pairs are written in
       a different order each round. After every write, pair j must read back
       as written and the other seven pairs as they were. Since get_scale_min
       reads each of the 96 bits exactly once, this fixes every byte. */
    if (run("scale_min")) {
        static const int order[8] = { 5, 0, 7, 2, 4, 1, 6, 3 };
        lcg r(64);
        long n = 0;
        for (int a = 0; a < 64; a++) {
            for (int b = 0; b < 64; b++) {
                uint8_t q[12];
                for (int k = 0; k < 12; k++) q[k] = (uint8_t)(r.next() >> 24);
                for (int k = 0; k < 8; k++) {
                    const int j = order[(k + a + b) & 7];
                    const uint8_t sc = (uint8_t)((a + j) & 63);
                    const uint8_t m  = (uint8_t)((b + 3 * j) & 63);
                    uint8_t want_sc[8], want_m[8];
                    for (int i = 0; i < 8; i++) get_scale_min(i, q, &want_sc[i], &want_m[i]);
                    want_sc[j] = sc;
                    want_m[j]  = m;
                    put_scale_min(j, q, sc, m);
                    for (int i = 0; i < 8; i++) {
                        uint8_t gs, gm;
                        get_scale_min(i, q, &gs, &gm);
                        if (gs != want_sc[i] || gm != want_m[i]) n++;
                    }
                }
            }
        }
        got["scale_min"] = n ? "wrong" : "ok";
        if (n) bad++;
    }

    /* ---- B2: bytes and restored values of each format ---- */
    if (run("q4_0") || run("q4_1") || run("q4_k") || run("q6_k")) {
        std::vector<float> x = make_input();
        const uint64_t n = x.size();

        struct { const char *key; nq_type t; } fmts[] = {
            { "q4_0", NQ_TYPE_Q4_0 },
            { "q4_1", NQ_TYPE_Q4_1 },
            { "q4_k", NQ_TYPE_Q4_K },
            { "q6_k", NQ_TYPE_Q6_K },
        };

        for (auto &fm : fmts) {
            if (!run(fm.key)) continue;
            std::vector<uint8_t> enc((size_t)nq_type_bytes(fm.t, n));
            nq_quantize(fm.t, x.data(), n, enc.data());
            got[std::string(fm.key) + "_bytes"] = nq_md5_hex(enc.data(), enc.size());

            std::vector<float> back(x.size());
            nq_dequantize(fm.t, enc.data(), n, back.data());
            got[std::string(fm.key) + "_back"] =
                nq_md5_hex((const uint8_t *)back.data(), back.size() * 4);

            stat_t s = compare(x, back);
            fprintf(stderr, "%-5s relative rms error %.5f, max abs error %.6g\n",
                    fm.key, sqrt(s.sse / s.sxx), (double)s.amax);
        }
    }

    if (emit) {
        for (const auto &kv : got) printf("%s %s\n", kv.first.c_str(), kv.second.c_str());
        return 0;
    }

    std::map<std::string, std::string> expect = load_expect(expath);
    if (expect.empty()) fprintf(stderr, "nq-selftest: no expected values in %s, printing results only\n", expath);

    for (const auto &kv : got) {
        auto it = expect.find(kv.first);
        const char *verdict = "";
        if (it != expect.end()) {
            if (it->second == kv.second) verdict = "  matches";
            else { verdict = "  DIFFERS"; bad++; }
        }
        printf("%-12s %s%s\n", kv.first.c_str(), kv.second.c_str(), verdict);
    }
    return bad ? 1 : 0;
}
