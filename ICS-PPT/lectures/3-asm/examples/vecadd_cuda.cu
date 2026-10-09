/* Vector addition on a GPU: c[i] = a[i] + b[i], one thread per element.
   The loop of add_cpu becomes the grid of threads; the loop body becomes
   the kernel.

     nvcc -O2 -arch=native vecadd_cuda.cu -o vecadd_cuda
     ./vecadd_cuda

   Without an NVIDIA GPU, send this file to the course server:

     curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @vecadd_cuda.cu $GPU_SERVER/program */
#include <stdio.h>
#include <stdlib.h>

// CPU: one thread runs the loop, i goes from 0 to n - 1
void add_cpu(const int *a, const int *b, int *c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}

// GPU: every thread runs this function once and computes one element
__global__ void add_kernel(const int *a, const int *b, int *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;    // this thread in the grid
    if (i < n)                                        // the last block has threads past n
        c[i] = a[i] + b[i];
}

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

int main(void) {
    /* n is not a multiple of 256: the last block has 64 elements. */
    const int n = 1000000, threads = 256;
    const int blocks = (n + threads - 1) / threads;
    int *a = (int *)malloc(n * sizeof(int));
    int *b = (int *)malloc(n * sizeof(int));
    int *c = (int *)malloc(n * sizeof(int));          // the result of the GPU
    int *check = (int *)malloc(n * sizeof(int));      // the result of the CPU
    for (int i = 0; i < n; i++) {
        a[i] = i;
        b[i] = 2 * i;
    }

    int *da, *db, *dc, differ = 0;
    // 1. allocate device memory and copy a and b from host memory into it
    CHECK(cudaMalloc(&da, n * sizeof(int)));
    CHECK(cudaMalloc(&db, n * sizeof(int)));
    CHECK(cudaMalloc(&dc, n * sizeof(int)));
    CHECK(cudaMemcpy(da, a, n * sizeof(int), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(db, b, n * sizeof(int), cudaMemcpyHostToDevice));
    // 2. start the grid: blocks x threads threads, each runs add_kernel once
    add_kernel<<<blocks, threads>>>(da, db, dc, n);
    CHECK(cudaGetLastError());
    // 3. copy c back from device memory into host memory
    CHECK(cudaMemcpy(c, dc, n * sizeof(int), cudaMemcpyDeviceToHost));

    add_cpu(a, b, check, n);
    for (int i = 0; i < n; i++)
        differ += c[i] != check[i];
    printf("blocks  %d x %d threads\nc[n-1]  %d\ncheck   %d\ndiffer  %d\n",
           blocks, threads, c[n - 1], check[n - 1], differ);

    cudaFree(da); cudaFree(db); cudaFree(dc);
    free(a); free(b); free(c); free(check);
    return 0;
}
