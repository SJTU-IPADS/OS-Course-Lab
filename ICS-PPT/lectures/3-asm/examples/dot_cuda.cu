/* The dot product on a GPU: each thread multiplies one pair, each block
   sums its 256 products in shared memory, and the CPU adds the block sums.

     nvcc -O2 -arch=native dot_cuda.cu -o dot_cuda
     ./dot_cuda */
#include <stdio.h>
#include <stdlib.h>

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

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

int main(void) {
    /* |w[i]*x[i]| <= 64, so the sum of 2^24 products fits in an int. */
    const int n = 1 << 24, threads = 256;
    const int blocks = (n + threads - 1) / threads;
    int *w = (int *)malloc(n * sizeof(int));
    int *x = (int *)malloc(n * sizeof(int));
    int *part = (int *)malloc(blocks * sizeof(int));
    for (int i = 0; i < n; i++) {
        w[i] = i % 16 - 8;
        x[i] = (i * 7) % 16 - 8;
    }

    int *dw, *dx, *dpart, sum = 0, check = 0;
    // 1. allocate device memory and copy w and x from host memory into it
    CHECK(cudaMalloc(&dw, n * sizeof(int)));
    CHECK(cudaMalloc(&dx, n * sizeof(int)));
    CHECK(cudaMalloc(&dpart, blocks * sizeof(int)));
    CHECK(cudaMemcpy(dw, w, n * sizeof(int), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dx, x, n * sizeof(int), cudaMemcpyHostToDevice));
    // 2. start the grid: blocks x threads threads, each runs dot_kernel once
    dot_kernel<<<blocks, threads>>>(dw, dx, dpart, n);
    CHECK(cudaGetLastError());
    // 3. copy the sums of the blocks back and add them up on the CPU
    CHECK(cudaMemcpy(part, dpart, blocks * sizeof(int), cudaMemcpyDeviceToHost));
    for (int b = 0; b < blocks; b++)
        sum += part[b];

    for (int i = 0; i < n; i++)
        check += w[i] * x[i];
    printf("blocks  %d x %d threads\nsum     %d\ncheck   %d\n", blocks, threads, sum, check);

    cudaFree(dw); cudaFree(dx); cudaFree(dpart);
    free(w); free(x); free(part);
    return 0;
}
