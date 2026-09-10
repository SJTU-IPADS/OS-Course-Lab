/* What changes when one operand of a comparison is unsigned. */
#include <stdio.h>
#include <string.h>

/* Sum an array. length is unsigned, as a size usually is. */
static float sum_elements(const float *a, unsigned length) {
    float result = 0.0f;
    for (unsigned i = 0; i <= length - 1; i++)   /* length - 1 wraps when length is 0 */
        result += a[i];
    return result;
}

/* Is s longer than t? strlen returns size_t, which is unsigned. */
static int strlonger(const char *s, const char *t) {
    return strlen(s) - strlen(t) > 0;
}

int main(void) {
    printf("-1 < 0                 %d\n", -1 < 0);
    printf("-1 < 0u                %d\n", -1 < 0u);
    printf("2147483647 > -2147483647-1     %d\n", 2147483647 > -2147483647 - 1);
    printf("2147483647u > -2147483647-1    %d\n", 2147483647u > -2147483647 - 1);

    unsigned length = 0;
    printf("length - 1, as unsigned        %u\n", length - 1);
    printf("strlonger(\"ab\", \"abcd\")        %d\n", strlonger("ab", "abcd"));
    (void) sum_elements;
    return 0;
}
