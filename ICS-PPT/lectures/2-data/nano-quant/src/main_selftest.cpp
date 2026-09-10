/* nq-selftest —— A、B 两部分的自查，不需要模型文件。
 *
 *   ./nq-selftest              跑全部检查，与 tests/reference-blocks.txt 对照
 *   ./nq-selftest a q4_0       只跑点到名的组：a scale_min q4_0 q4_1 q4_k q6_k
 *   ./nq-selftest --emit       只把参考摘要打到标准输出（助教用它生成上面那个文件）
 *   ./nq-selftest --full       f32_to_fp16 改成遍历全部 2^32 个 float
 *
 * 每一项的结果是一行：项目名、摘要或 ok、以及是否与参考一致。 */
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

/* 固定的伪随机数，换台机器也是同一串。 */
struct lcg {
    uint64_t s;
    explicit lcg(uint64_t seed) : s(seed) {}
    uint32_t next() { s = s * 6364136223846793005ull + 1442695040888963407ull; return (uint32_t)(s >> 32); }
    float uniform() { return (float)(next() >> 8) / (float)(1 << 24); }   /* [0, 1) */
};

uint32_t bits_of(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

/* 测试用的权重：8 个刻意构造的超块，加上 504 个随机超块，
   量级横跨 1e-6 到 1e2，正负都有。 */
std::vector<float> make_input() {
    const int NSB = 512;
    std::vector<float> x((size_t)NSB * 256);

    for (int i = 0; i < 256; i++) x[i] = 0.0f;                              /* 全零 */
    for (int i = 0; i < 256; i++) x[256 + i] = 0.375f;                      /* 全相等 */
    for (int i = 0; i < 256; i++) x[512 + i] = (i == 7) ? 100.0f : 1e-4f;   /* 一个离群值 */
    for (int i = 0; i < 256; i++) x[768 + i] = -0.05f - 0.001f * (float)i;  /* 全负 */
    for (int i = 0; i < 256; i++) x[1024 + i] = 0.05f + 0.001f * (float)i;  /* 全正 */
    for (int i = 0; i < 256; i++) x[1280 + i] = (i & 1) ? 1e-7f : -1e-7f;   /* 极小 */
    for (int i = 0; i < 256; i++) x[1536 + i] = (float)(i - 128) * 0.5f;    /* 关于 0 对称 */
    for (int i = 0; i < 256; i++) x[1792 + i] = (i < 255) ? 0.0f : 3.0f;    /* 单点 */

    lcg r(20260907);
    for (int b = 8; b < NSB; b++) {
        float sigma = powf(10.0f, -4.0f + 6.0f * r.uniform());              /* 1e-4 到 1e2 */
        for (int i = 0; i < 256; i++) {
            float u = r.uniform() * 2.0f - 1.0f;
            float v = r.uniform() * 2.0f - 1.0f;
            x[(size_t)b * 256 + i] = sigma * u * fabsf(v);
        }
    }
    /* 截断到 BF16 再放回来，取值范围与真实模型里的一致 */
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

std::map<std::string, std::string> load_ref(const char *path) {
    std::map<std::string, std::string> m;
    FILE *fp = fopen(path, "r");
    if (!fp) return m;
    char k[64], v[64];
    while (fscanf(fp, "%63s %63s", k, v) == 2) m[k] = v;
    fclose(fp);
    return m;
}

} /* namespace */

/* 不带参数跑全部检查；带参数只跑点到名的那几组，
   于是写完一部分就能单独看这一部分过没过。 */
static const char *GROUPS[] = { "a", "scale_min", "q4_0", "q4_1", "q4_k", "q6_k" };

int main(int argc, char **argv) {
    bool emit = false, full = false;
    const char *refpath = "tests/reference-blocks.txt";
    std::set<std::string> want;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--emit")) emit = true;
        else if (!strcmp(argv[i], "--full")) full = true;
        else if (!strcmp(argv[i], "--ref") && i + 1 < argc) refpath = argv[++i];
        else if (argv[i][0] != '-') {
            bool known = false;
            for (const char *g : GROUPS) if (!strcmp(argv[i], g)) known = true;
            if (!known) { fprintf(stderr, "没有这一组：%s\n", argv[i]); return 2; }
            want.insert(argv[i]);
        }
        else {
            fprintf(stderr, "用法：nq-selftest [--emit] [--full] [--ref FILE] [组...]\n"
                            "组：a scale_min q4_0 q4_1 q4_k q6_k，不写则全跑\n");
            return 2;
        }
    }
    /* q4_k 的 12 字节打包属于 q4_k，点名 q4_k 时把它一起跑上 */
    if (want.count("q4_k")) want.insert("scale_min");
    auto run = [&](const char *g) { return want.empty() || want.count(g) != 0; };

    std::map<std::string, std::string> got;
    int bad = 0;

    /* ---- A1：rd_u64le ---- */
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

    /* ---- A2：bf16_to_f32、fp16_to_f32，遍历全部 65536 个位型 ---- */
    if (run("a")) {
        long nb = 0, nf = 0;
        for (uint32_t h = 0; h < 65536; h++) {
            if (bits_of(bf16_to_f32((uint16_t)h)) != bits_of(nq_bf16_to_f32((uint16_t)h))) nb++;
            uint32_t a = bits_of(fp16_to_f32((uint16_t)h));
            uint32_t b = bits_of(nq_fp16_to_f32((uint16_t)h));
            /* NaN 的尾数位不作要求，只要求同为 NaN 且符号相同 */
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

    /* ---- A3：f32_to_fp16 ---- */
    if (run("a")) {
        long n = 0;
        uint64_t step = full ? 1 : 251;          /* 251 是质数，不与指数域对齐 */
        for (uint64_t u = 0; u < 0x100000000ull && n == 0; u += step) {
            float f;
            uint32_t bits = (uint32_t)u;
            memcpy(&f, &bits, 4);
            if (f != f) continue;                /* NaN 不比 */
            if (f32_to_fp16(f) != nq_f32_to_fp16(f)) {
                fprintf(stderr, "f32_to_fp16(%.9g)（位型 %08x）得 %04x，应为 %04x\n",
                        (double)f, bits, f32_to_fp16(f), nq_f32_to_fp16(f));
                n++;
            }
        }
        /* 半精度能精确表示的每一个值都必须原样回来 */
        for (uint32_t h = 0; h < 65536 && n == 0; h++) {
            if ((h & 0x7c00) == 0x7c00) continue;
            float f = nq_fp16_to_f32((uint16_t)h);
            if (f32_to_fp16(f) != (uint16_t)h) {
                fprintf(stderr, "f32_to_fp16 往返丢了 %04x\n", h);
                n++;
            }
        }
        got["f32_to_fp16"] = n ? "wrong" : "ok";
        if (n) bad++;
    }

    /* ---- B1：put_scale_min 与 get_scale_min 互逆，穷举 ---- */
    if (run("scale_min")) {
        long n = 0;
        for (int a = 0; a < 64; a++) {
            for (int b = 0; b < 64; b++) {
                uint8_t q[12];
                memset(q, 0, 12);
                for (int j = 0; j < 8; j++)
                    put_scale_min(j, q, (uint8_t)((a + j) & 63), (uint8_t)((b + 3 * j) & 63));
                for (int j = 0; j < 8; j++) {
                    uint8_t sc, m;
                    get_scale_min(j, q, &sc, &m);
                    if (sc != ((a + j) & 63) || m != ((b + 3 * j) & 63)) n++;
                }
            }
        }
        got["scale_min"] = n ? "wrong" : "ok";
        if (n) bad++;
    }

    /* ---- B2：三种格式的字节与还原值 ---- */
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
            fprintf(stderr, "%-5s 相对均方根误差 %.5f，最大绝对误差 %.6g\n",
                    fm.key, sqrt(s.sse / s.sxx), (double)s.amax);
        }
    }

    if (emit) {
        for (const auto &kv : got) printf("%s %s\n", kv.first.c_str(), kv.second.c_str());
        return 0;
    }

    std::map<std::string, std::string> ref = load_ref(refpath);
    if (ref.empty()) fprintf(stderr, "找不到参考文件 %s，只报自己的结果\n", refpath);

    for (const auto &kv : got) {
        auto it = ref.find(kv.first);
        const char *verdict = "";
        if (it != ref.end()) {
            if (it->second == kv.second) verdict = "  与参考一致";
            else { verdict = "  与参考不符"; bad++; }
        }
        printf("%-12s %s%s\n", kv.first.c_str(), kv.second.c_str(), verdict);
    }
    return bad ? 1 : 0;
}
