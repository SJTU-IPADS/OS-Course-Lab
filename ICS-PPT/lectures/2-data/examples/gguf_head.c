/* Read the first three fields of a GGUF header, both byte orders. */
#include <stdio.h>
#include <stdint.h>

static uint32_t le32(const unsigned char *b) {          /* little endian */
    return (uint32_t) b[0] | (uint32_t) b[1] << 8 |
           (uint32_t) b[2] << 16 | (uint32_t) b[3] << 24;
}

static uint32_t be32(const unsigned char *b) {          /* big endian */
    return (uint32_t) b[3] | (uint32_t) b[2] << 8 |
           (uint32_t) b[1] << 16 | (uint32_t) b[0] << 24;
}

int main(void) {
    unsigned char h[16];
    FILE *f = fopen("tiny.gguf", "rb");
    if (fread(h, 1, sizeof(h), f) != sizeof(h)) return 1;
    fclose(f);

    printf("magic          %c%c%c%c\n", h[0], h[1], h[2], h[3]);
    printf("version   LE   %u\n", le32(h + 4));
    printf("version   BE   %u\n", be32(h + 4));
    return 0;
}
