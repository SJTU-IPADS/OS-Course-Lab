/* Every identity on this slide, checked over all 65536 pairs of bytes. */
#include <stdint.h>
#include <stdio.h>

static uint8_t lowest_one(uint8_t x) {          /* the reference answer */
    for (int i = 0; i < 8; i++)
        if (x >> i & 1) return (uint8_t) (1u << i);
    return 0;
}

#define CHECK(name, expr)                                                   \
    do {                                                                    \
        long bad = 0;                                                       \
        for (int a = 0; a < 256; a++)                                       \
            for (int b = 0; b < 256; b++) {                                 \
                uint8_t x = (uint8_t) a, y = (uint8_t) b;                   \
                (void) y;                                                   \
                if (!(expr)) bad++;                                         \
            }                                                               \
        printf("%-32s %s\n", name, bad ? "FAILS" : "holds, 65536 pairs");   \
    } while (0)

int main(void) {
    CHECK("~(x & y) == ~x | ~y", (uint8_t) ~(x & y) == (uint8_t) (~x | ~y));
    CHECK("~(x | y) == ~x & ~y", (uint8_t) ~(x | y) == (uint8_t) (~x & ~y));
    CHECK("x ^ y ^ y == x", (uint8_t) (x ^ y ^ y) == x);
    CHECK("~(x ^ y) == x & y | ~x & ~y", (uint8_t) ~(x ^ y)
          == (uint8_t) ((x & y) | (uint8_t) (~x & ~y)));
    CHECK("-x == ~x + 1", (uint8_t) -x == (uint8_t) (~x + 1));
    CHECK("x & -x is the lowest 1", (uint8_t) (x & -x) == lowest_one(x));
    return 0;
}
