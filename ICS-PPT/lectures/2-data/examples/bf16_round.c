/* FP32 to BF16: keep the top 16 bits, then round on the 16 dropped bits. */
#include <stdio.h>
#include <stdint.h>

static uint32_t bits(float f) { uint32_t b; __builtin_memcpy(&b, &f, 4); return b; }

static uint16_t to_bf16(uint32_t f32) {
    if ((f32 & 0x7fffffffU) > 0x7f800000U)
        return (uint16_t) ((f32 >> 16) | 0x0040U); /* preserve NaN as quiet NaN */
    uint16_t bf16 = (uint16_t) (f32 >> 16);   /* sign, exponent, top 7 fraction bits */
    uint16_t rest = (uint16_t) f32;           /* the 16 dropped bits */
    if (rest > 0x8000 || (rest == 0x8000 && (bf16 & 1)))
        bf16 += 1;                            /* round to nearest even; may carry into exp */
    return bf16;
}

static void print16(uint16_t h) {
    printf("%d ", h >> 15);
    for (int i = 14; i >= 7; i--) putchar((h >> i & 1) ? '1' : '0');
    putchar(' ');
    for (int i = 6; i >= 0; i--) putchar((h >> i & 1) ? '1' : '0');
}

static void show(float x) {
    uint32_t f32 = bits(x);
    uint16_t cut = (uint16_t) (f32 >> 16), r = to_bf16(f32), c;
    __bf16 h = x;                             /* the compiler's conversion, for comparison */
    __builtin_memcpy(&c, &h, 2);
    printf("%-12.9g ", x);
    print16(cut);
    printf("   ");
    print16(r);
    printf("   %s\n", r == c ? "same" : "differs");
}

int main(void) {
    printf("%-12s %-18s   %-18s   %s\n", "x", "top 16 bits", "rounded", "vs __bf16");
    show(3.1415927f);
    show(1.0f / 3);
    show(1.99999988f);
    return 0;
}
