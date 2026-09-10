/* What this machine's byte order is, and what Linux offers for the other one. */
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

static void show(const char *label, uint32_t v) {
    const unsigned char *b = (const unsigned char *) &v;
    printf("%-14s 0x%08x   bytes %02x %02x %02x %02x\n", label, v,
           b[0], b[1], b[2], b[3]);
}

int main(void) {
    uint32_t host = 0x01020304;
    printf("__BYTE_ORDER__ %s\n",
           __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? "little" : "big");
    show("host", host);
    show("htole32", htole32(host));          /* identity on this machine */
    show("htobe32", htobe32(host));          /* one byte-swap instruction */
    return 0;
}
