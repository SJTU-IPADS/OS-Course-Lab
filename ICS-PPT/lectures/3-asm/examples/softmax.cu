/* softmax on the GPU: the softmax of softmax.c for the same 2^24 scores,
   computed 50 times, printing the same three lines. main and the host
   function softmax are complete; the bodies of the three kernels are left
   for you to write.

   softmax runs the three steps of softmax.c, one kernel each, with one
   thread per score and THREADS = 256 threads per block:

     1. block_max  each block writes the largest of its scores to
                   part[blockIdx.x]; the host takes the largest of these.
     2. exp_sum    each thread writes y[i] = expf(s[i] - max); each block
                   writes the sum of its exponentials to part[blockIdx.x];
                   the host adds these up.
     3. divide     each thread divides y[i] by the sum.

   Steps 1 and 2 are reductions, written as dot_kernel of dot_cuda.cu.

     nvcc -O2 -arch=native softmax.cu -o softmax_cuda
     time ./softmax_cuda */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 256

// 1. part[blockIdx.x] = the largest of the block's scores s[idx], idx < n
__global__ void block_max(const float *s, float *part, int n) {
    // TODO
}

// 2. y[idx] = expf(s[idx] - max) for idx < n;
//    part[blockIdx.x] = the sum of the block's y[idx]
__global__ void exp_sum(float *y, const float *s, float max, float *part, int n) {
    // TODO
}

// 3. y[idx] = y[idx] / sum for idx < n
__global__ void divide(float *y, float sum, int n) {
    // TODO
}

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

/* softmax of the n scores at ds into dy, both in GPU memory. dpart (GPU)
   and part (host) hold one float per block. */
static void softmax(float *dy, const float *ds, float *dpart, float *part, int n) {
    const int blocks = (n + THREADS - 1) / THREADS;

    // 1. the largest score: one per block on the GPU, the largest of those here
    block_max<<<blocks, THREADS>>>(ds, dpart, n);
    CHECK(cudaMemcpy(part, dpart, blocks * sizeof(float), cudaMemcpyDeviceToHost));
    float max = part[0];
    for (int b = 1; b < blocks; b++)
        if (part[b] > max) max = part[b];

    // 2. the exponentials, and their sum: one per block, added up here
    exp_sum<<<blocks, THREADS>>>(dy, ds, max, dpart, n);
    CHECK(cudaMemcpy(part, dpart, blocks * sizeof(float), cudaMemcpyDeviceToHost));
    double sum = 0;
    for (int b = 0; b < blocks; b++)
        sum += part[b];

    // 3. each divided by the sum
    divide<<<blocks, THREADS>>>(dy, (float)sum, n);
    CHECK(cudaGetLastError());
}

int main(void) {
    const int n = 1 << 24, times = 50, blocks = (n + THREADS - 1) / THREADS;
    float *s = (float *)malloc(n * sizeof(float));
    float *y = (float *)malloc(n * sizeof(float));
    float *part = (float *)malloc(blocks * sizeof(float));
    for (int i = 0; i < n; i++)
        s[i] = (i % 1000) * 0.01f - 5;              // the scores of softmax.c

    float *ds, *dy, *dpart;
    CHECK(cudaMalloc(&ds, n * sizeof(float)));
    CHECK(cudaMalloc(&dy, n * sizeof(float)));
    CHECK(cudaMalloc(&dpart, blocks * sizeof(float)));
    CHECK(cudaMemcpy(ds, s, n * sizeof(float), cudaMemcpyHostToDevice));

    for (int t = 0; t < times; t++)
        softmax(dy, ds, dpart, part, n);
    CHECK(cudaMemcpy(y, dy, n * sizeof(float), cudaMemcpyDeviceToHost));

    double total = 0;
    for (int i = 0; i < n; i++)
        total += y[i];
    printf("y[0]    %.6e\ny[999]  %.6e\nsum     %.6f\n", y[0], y[999], total);

    cudaFree(ds); cudaFree(dy); cudaFree(dpart);
    free(s); free(y); free(part);
    return 0;
}
