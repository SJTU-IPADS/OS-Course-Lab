/* softmax on the CPU: the softmax of mini-ollama/mini_ollama.c for
   n = 2^24 scores, computed 50 times. mini-ollama computes a softmax for
   every head of every token it generates; here the same scores are used
   each time, and the results of the last time are printed. The function
   writes its results to y instead of over the scores, and its sum is a
   double: a float sum of 2^24 terms stops growing long before the last
   term.

     gcc -O2 softmax.c -o softmax -lm
     time ./softmax */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Turn n scores into n weights that are positive and add up to 1:
   y[i] = exp(s[i] - max) / sum, the sum being that of every exp(s[j] - max). */
static void softmax(float *y, const float *s, int n)
{
    float max = s[0];
    double sum = 0;
    for (int i = 1; i < n; i++)            // 1. the largest score
        if (s[i] > max)
            max = s[i];
    for (int i = 0; i < n; i++)            // 2. the exponentials and their sum
        sum += y[i] = expf(s[i] - max);
    for (int i = 0; i < n; i++)            // 3. each divided by the sum
        y[i] /= sum;
}

int main(void)
{
    const int n = 1 << 24, times = 50;
    float *s = malloc(n * sizeof(float));
    float *y = malloc(n * sizeof(float));
    for (int i = 0; i < n; i++)
        s[i] = (i % 1000) * 0.01f - 5;      // scores from -5.00 to 4.99

    for (int t = 0; t < times; t++)
        softmax(y, s, n);

    double total = 0;
    for (int i = 0; i < n; i++)
        total += y[i];
    printf("y[0]    %.6e\ny[999]  %.6e\nsum     %.6f\n", y[0], y[999], total);
    free(s);
    free(y);
    return 0;
}
