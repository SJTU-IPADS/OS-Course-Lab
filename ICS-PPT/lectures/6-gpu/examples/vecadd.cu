/* Vector addition on a GPU: c[i] = a[i] + b[i], one thread per element.
   The loop of add_cpu becomes the grid of threads. The loop body becomes
   the kernel.

     curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @vecadd.cu $GPU_SERVER/program */
#include <stdio.h>
#include <stdlib.h>

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

// CPU: one thread runs the loop, i goes from 0 to n - 1
void add_cpu(const float *a, const float *b, float *c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}

// GPU: every thread runs this function once and computes one element
__global__ void add_kernel(const float *a, const float *b, float *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;    // this thread in the grid
    if (i < n)                                        // the last block has threads past n
        c[i] = a[i] + b[i];
}

int main(void) {
    /* n is not a multiple of 256: the last block has 64 elements. */
    const int n = 1000000, threads = 256;
    const int blocks = (n + threads - 1) / threads;
    size_t bytes = n * sizeof(float);
    float *a = (float *)malloc(bytes), *b = (float *)malloc(bytes);
    float *c = (float *)malloc(bytes);                // the result of the GPU
    float *check = (float *)malloc(bytes);            // the result of the CPU
    for (int i = 0; i < n; i++) {
        a[i] = i;
        b[i] = 2 * i;
    }

    float *da, *db, *dc;
    // 1. allocate device memory
    CHECK(cudaMalloc(&da, bytes));
    CHECK(cudaMalloc(&db, bytes));
    CHECK(cudaMalloc(&dc, bytes));
    // 2. copy a and b from host memory into device memory
    CHECK(cudaMemcpy(da, a, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(db, b, bytes, cudaMemcpyHostToDevice));
    // 3. start the grid: blocks x threads threads, each runs add_kernel once
    add_kernel<<<blocks, threads>>>(da, db, dc, n);
    CHECK(cudaGetLastError());
    // 4. copy c back from device memory into host memory
    CHECK(cudaMemcpy(c, dc, bytes, cudaMemcpyDeviceToHost));
    // 5. free device memory
    CHECK(cudaFree(da));
    CHECK(cudaFree(db));
    CHECK(cudaFree(dc));

    add_cpu(a, b, check, n);
    int differ = 0;
    for (int i = 0; i < n; i++)
        differ += c[i] != check[i];
    printf("blocks  %d x %d threads\nc[n-1]  %.0f\ncheck   %.0f\ndiffer  %d\n",
           blocks, threads, c[n - 1], check[n - 1], differ);
    free(a); free(b); free(c); free(check);
    return 0;
}
