/* Q4_K packs sixteen 6-bit numbers into twelve bytes. Check that it is lossless.

   16 x 6 = 96 bits = 12 bytes exactly, so the packing has to be a bijection or
   it is wrong. This walks a million random sets of sixteen values through the
   pack/unpack pair from llama.cpp and counts the ones that do not come back. */
#include <stdio.h>
#include <stdint.h>

static void put(int j, uint8_t *q, uint8_t ls, uint8_t lm) {
    if (j < 4) { q[j] = ls; q[j + 4] = lm; }
    else {
        q[j + 4]  = (uint8_t) ((ls & 0xf) | (lm & 0xf) << 4);
        q[j - 4] |= (uint8_t) ((ls >> 4) << 6);      /* the two spare top bits */
        q[j]     |= (uint8_t) ((lm >> 4) << 6);
    }
}

static void get(int j, const uint8_t *q, uint8_t *ls, uint8_t *lm) {
    if (j < 4) { *ls = q[j] & 63; *lm = q[j + 4] & 63; }
    else {
        *ls = (uint8_t) ((q[j + 4] & 0xf) | (q[j - 4] >> 6) << 4);
        *lm = (uint8_t) ((q[j + 4] >> 4)  | (q[j]     >> 6) << 4);
    }
}

static uint32_t seed = 20260906;
static uint32_t rnd(void) { seed = seed * 1664525u + 1013904223u; return seed >> 16; }

int main(void) {
    long bad = 0, trials = 1000000;
    for (long t = 0; t < trials; t++) {
        uint8_t sc[8], mn[8], q[12] = {0}, a, b;
        for (int j = 0; j < 8; j++) { sc[j] = rnd() & 63; mn[j] = rnd() & 63; }
        for (int j = 0; j < 8; j++) put(j, q, sc[j], mn[j]);
        for (int j = 0; j < 8; j++) {
            get(j, q, &a, &b);
            if (a != sc[j] || b != mn[j]) bad++;
        }
    }
    printf("%ld random sets of 16 six-bit values, %ld bytes each\n", trials, 12L);
    printf("values that did not survive the round trip: %ld\n", bad);

    uint8_t sc[8] = {63, 1, 40, 7, 62, 33, 8, 21};
    uint8_t mn[8] = {12, 63, 0, 55, 9, 41, 60, 3};
    uint8_t q[12] = {0};
    for (int j = 0; j < 8; j++) put(j, q, sc[j], mn[j]);
    printf("one example:");
    for (int i = 0; i < 12; i++) printf(" %02x", q[i]);
    printf("\n");
    return 0;
}
