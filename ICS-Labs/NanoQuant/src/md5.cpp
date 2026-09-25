/* MD5. nano-quant quant reports the digest of the .nq data area. */
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "nq.h"

namespace {

struct md5_ctx {
    uint32_t st[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
    uint64_t len = 0;
    uint8_t  buf[64];
    size_t   n = 0;
};

const uint32_t K[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391};
const int S[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5, 9,14,20,5, 9,14,20,5, 9,14,20,5, 9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21};

inline uint32_t rol(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }

void md5_block(md5_ctx &c, const uint8_t *p) {
    uint32_t M[16];
    for (int i = 0; i < 16; i++)
        M[i] = (uint32_t)p[4*i] | ((uint32_t)p[4*i+1] << 8) |
               ((uint32_t)p[4*i+2] << 16) | ((uint32_t)p[4*i+3] << 24);
    uint32_t a = c.st[0], b = c.st[1], d = c.st[2], e = c.st[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f; int g;
        if (i < 16)      { f = (b & d) | (~b & e);          g = i; }
        else if (i < 32) { f = (e & b) | (~e & d);          g = (5*i + 1) & 15; }
        else if (i < 48) { f = b ^ d ^ e;                   g = (3*i + 5) & 15; }
        else             { f = d ^ (b | ~e);                g = (7*i) & 15; }
        uint32_t tmp = e;
        e = d; d = b;
        b = b + rol(a + f + K[i] + M[g], S[i]);
        a = tmp;
    }
    c.st[0] += a; c.st[1] += b; c.st[2] += d; c.st[3] += e;
}

void md5_update(md5_ctx &c, const uint8_t *p, size_t n) {
    c.len += n;
    while (n) {
        size_t k = 64 - c.n;
        if (k > n) k = n;
        memcpy(c.buf + c.n, p, k);
        c.n += k; p += k; n -= k;
        if (c.n == 64) { md5_block(c, c.buf); c.n = 0; }
    }
}

std::string md5_final(md5_ctx &c) {
    uint64_t bits = c.len * 8;
    uint8_t pad = 0x80;
    md5_update(c, &pad, 1);
    uint8_t z = 0;
    while (c.n != 56) md5_update(c, &z, 1);
    uint8_t lb[8];
    for (int i = 0; i < 8; i++) lb[i] = (uint8_t)(bits >> (8*i));
    md5_update(c, lb, 8);
    char out[33];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            snprintf(out + i*8 + j*2, 3, "%02x", (unsigned)((c.st[i] >> (8*j)) & 0xff));
    return std::string(out, 32);
}

} /* namespace */

std::string nq_md5_hex(const uint8_t *data, size_t n) {
    md5_ctx c;
    md5_update(c, data, n);
    return md5_final(c);
}

std::string nq_md5_file_range(const char *path, uint64_t off, uint64_t len) {
    int fd = nq_open_read(path);
    if (fd < 0) return std::string();
    md5_ctx c;
    std::vector<uint8_t> buf(1 << 20);
    uint64_t done = 0;
    while (done < len) {
        size_t want = buf.size();
        if (want > len - done) want = (size_t)(len - done);
        if (nq_read_at(fd, buf.data(), want, off + done) != 0) { nq_close(fd); return std::string(); }
        md5_update(c, buf.data(), want);
        done += want;
    }
    nq_close(fd);
    return md5_final(c);
}
