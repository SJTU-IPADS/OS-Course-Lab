/* The dot product on a GPU: each thread multiplies one pair, each block
   sums its 256 products in shared memory, and the CPU adds the block sums.
   The same dot product is computed by a loop on one CPU core. The last three
   lines are the time of dot_kernel alone (CUDA events), the time of the CPU
   loop, and their ratio; each time comes with the bandwidth of its reads of
   w and x.

     curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @dot.cu $GPU_SERVER/program */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

__global__ void dot_kernel(const int *w, const int *x, int *block_sum, int n) {
    __shared__ int cache[256];                        // one array per thread block
    int tid = threadIdx.x;                            // this thread in its block: 0..255
    int idx = blockIdx.x * blockDim.x + threadIdx.x;  // this thread in the grid
    cache[tid] = (idx < n) ? w[idx] * x[idx] : 0;     // one product per thread
    __syncthreads();                                  // until all 256 products are stored
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {    // 256 products -> 1 sum in 8 steps
        if (tid < s) cache[tid] += cache[tid + s];
        __syncthreads();
    }
    if (tid == 0) block_sum[blockIdx.x] = cache[0];   // one sum per block
}

int main(void) {
    /* |w[i]*x[i]| <= 64, so the sum of 2^24 products fits in an int. */
    const int n = 1 << 24, threads = 256, reps = 20;
    const int blocks = (n + threads - 1) / threads;
    int *w = (int *)malloc(n * sizeof(int));
    int *x = (int *)malloc(n * sizeof(int));
    int *part = (int *)malloc(blocks * sizeof(int));
    for (int i = 0; i < n; i++) {
        w[i] = i % 16 - 8;
        x[i] = (i * 7) % 16 - 8;
    }

    int *dw, *dx, *dpart, sum = 0, check = 0;
    CHECK(cudaMalloc(&dw, n * sizeof(int)));
    CHECK(cudaMalloc(&dx, n * sizeof(int)));
    CHECK(cudaMalloc(&dpart, blocks * sizeof(int)));
    CHECK(cudaMemcpy(dw, w, n * sizeof(int), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dx, x, n * sizeof(int), cudaMemcpyHostToDevice));
    dot_kernel<<<blocks, threads>>>(dw, dx, dpart, n);
    CHECK(cudaGetLastError());
    // the sums of the blocks come back to the CPU, which adds them up
    CHECK(cudaMemcpy(part, dpart, blocks * sizeof(int), cudaMemcpyDeviceToHost));
    for (int b = 0; b < blocks; b++)
        sum += part[b];

    // the kernel alone, the mean of reps launches between two events
    cudaEvent_t t0, t1;
    float ms;
    CHECK(cudaEventCreate(&t0));
    CHECK(cudaEventCreate(&t1));
    CHECK(cudaEventRecord(t0));
    for (int r = 0; r < reps; r++)
        dot_kernel<<<blocks, threads>>>(dw, dx, dpart, n);
    CHECK(cudaEventRecord(t1));
    CHECK(cudaEventSynchronize(t1));
    CHECK(cudaEventElapsedTime(&ms, t0, t1));
    ms /= reps;

    // the same dot product in a loop on one CPU core, one pass
    struct timespec c0, c1;
    clock_gettime(CLOCK_MONOTONIC, &c0);
    for (int i = 0; i < n; i++)
        check += w[i] * x[i];
    clock_gettime(CLOCK_MONOTONIC, &c1);
    double cpu_ms = (c1.tv_sec - c0.tv_sec) * 1e3 + (c1.tv_nsec - c0.tv_nsec) / 1e6;

    double bytes = 2.0 * n * sizeof(int);  // the reads of w and x
    printf("blocks  %d x %d threads\nsum     %d\ncheck   %d\n", blocks, threads, sum, check);
    printf("GPU     %.3f ms  %.1f GB/s\n", ms, bytes / ms / 1e6);
    printf("CPU     %.3f ms  %.1f GB/s\n", cpu_ms, bytes / cpu_ms / 1e6);
    printf("speedup %.1fx\n", cpu_ms / ms);

    cudaFree(dw); cudaFree(dx); cudaFree(dpart);
    free(w); free(x); free(part);
    return 0;
}
