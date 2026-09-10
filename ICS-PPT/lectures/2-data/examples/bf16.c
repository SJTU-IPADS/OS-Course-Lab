/* BF16 keeps FP32's exponent, so converting one to the other is a cut. */
#include <stdio.h>
#include <stdint.h>

static uint32_t bits(float f) { uint32_t b; __builtin_memcpy(&b, &f, 4); return b; }
static float    from(uint32_t b) { float f; __builtin_memcpy(&f, &b, 4); return f; }

static void print_bits(const char *label, uint32_t b, int width) {
    printf("%-9s", label);
    for (int i = width - 1; i >= 0; i--) {
        putchar((b >> i & 1) ? '1' : '0');
        if (i == width - 1 || i == width - 9) putchar(' ');   /* sign | exp | mantissa */
    }
    printf("   %.7f\n", width == 32 ? from(b) : from(b << 16));
}

int main(void) {
    float x = 3.1415927f;
    uint32_t f32 = bits(x);
    uint16_t bf16 = (uint16_t) (f32 >> 16);     /* the whole conversion */

    print_bits("fp32", f32, 32);
    print_bits("bf16", bf16, 16);
    printf("%-9s%.7f\n", "dropped", x - from((uint32_t) bf16 << 16));
    return 0;
}
