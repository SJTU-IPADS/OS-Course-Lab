/* Two 4-bit lanes in one byte: what "overflow" has to mean for each lane.

   add_packed works because bits 0-2 of each lane can be added normally --
   7 + 7 = 14 still fits in the lane, so no carry leaves it. Bit 3 is then
   added with xor, which drops the carry instead of passing it to the lane
   above. */
#include <stdio.h>
#include <stdint.h>

static uint8_t lo4(uint8_t b) { return b & 0x0f; }
static uint8_t hi4(uint8_t b) { return b >> 4; }
static uint8_t pack(uint8_t lo, uint8_t hi) { return (uint8_t) (hi << 4 | (lo & 0x0f)); }

/* What each lane should hold: its own sum, wrapped at 16, no influence
   from the other lane. */
static uint8_t reference(uint8_t a, uint8_t b) {
    return pack((lo4(a) + lo4(b)) & 0x0f, (hi4(a) + hi4(b)) & 0x0f);
}

static uint8_t add_naive(uint8_t a, uint8_t b) {
    return (uint8_t) (a + b);                          /* a carry crosses bit 3 */
}

static uint8_t add_packed(uint8_t a, uint8_t b) {
    uint8_t s = (uint8_t) ((a & 0x77) + (b & 0x77));   /* no carry leaves a lane */
    return (uint8_t) (s ^ ((a ^ b) & 0x88));           /* bit 3 added, carry dropped */
}

int main(void) {
    unsigned bad_naive = 0, bad_packed = 0;
    for (unsigned a = 0; a < 256; a++)
        for (unsigned b = 0; b < 256; b++) {
            uint8_t want = reference((uint8_t) a, (uint8_t) b);
            bad_naive  += add_naive ((uint8_t) a, (uint8_t) b) != want;
            bad_packed += add_packed((uint8_t) a, (uint8_t) b) != want;
        }

    uint8_t x = pack(9, 2), y = pack(8, 3);    /* lanes: 9+8 wraps, 2+3 does not */
    printf("x = 0x%02x (lanes %u, %u)   y = 0x%02x (lanes %u, %u)\n",
           x, lo4(x), hi4(x), y, lo4(y), hi4(y));
    printf("a + b        -> 0x%02x  (lanes %u, %u)\n",
           add_naive(x, y), lo4(add_naive(x, y)), hi4(add_naive(x, y)));
    printf("add_packed   -> 0x%02x  (lanes %u, %u)\n",
           add_packed(x, y), lo4(add_packed(x, y)), hi4(add_packed(x, y)));
    printf("wrong out of 65536 pairs:  a + b %u,  add_packed %u\n",
           bad_naive, bad_packed);
    return 0;
}
