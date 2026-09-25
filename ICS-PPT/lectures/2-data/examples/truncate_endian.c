/* Truncation is defined on the value; reading a prefix is defined on the bytes.
   The two agree on a little-endian machine and disagree on a big-endian one. */
#include <stdint.h>
#include <stdio.h>

static void put_le32(unsigned char *b, uint32_t v) {
    for (int i = 0; i < 4; i++) b[i] = (unsigned char) (v >> (8 * i));
}
static void put_be32(unsigned char *b, uint32_t v) {
    for (int i = 0; i < 4; i++) b[i] = (unsigned char) (v >> (8 * (3 - i)));
}
static uint16_t get_le16(const unsigned char *b) { return (uint16_t) (b[0] | b[1] << 8); }
static uint16_t get_be16(const unsigned char *b) { return (uint16_t) (b[1] | b[0] << 8); }

static void show(const char *label, const unsigned char *b) {
    printf("%-24s %02x %02x %02x %02x\n", label, b[0], b[1], b[2], b[3]);
}

int main(void) {
    uint32_t x = 0x12345678;
    unsigned char le[4], be[4];
    put_le32(le, x);
    put_be32(be, x);

    printf("%-24s 0x%08x\n", "x", x);
    printf("%-24s 0x%04x\n", "(uint16_t) x", (uint16_t) x);
    show("bytes, little endian", le);
    show("bytes, big endian", be);
    printf("%-24s 0x%04x\n", "first two bytes, LE", get_le16(le));
    printf("%-24s 0x%04x\n", "first two bytes, BE", get_be16(be));
    return 0;
}
