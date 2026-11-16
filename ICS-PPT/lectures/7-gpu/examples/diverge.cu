/* Branch divergence: two kernels with the same two branches. Half of the
   threads take the first branch and half the second in both kernels, so
   the useful work is the same. branch_by_lane chooses by threadIdx.x % 2:
   both branches occur in every warp, and the warp runs one branch after the
   other. branch_by_warp chooses by threadIdx.x / 32 % 2: all 32 threads of
   a warp take the same branch, and the warp runs that branch only.

     curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @diverge.cu $GPU_SERVER/program */
#include <stdio.h>
#include <stdlib.h>

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

#define STEPS 1024                                    // multiply-adds per branch

// the first branch: STEPS dependent multiply-adds, v -> v * 0.999 + 0.001
__device__ float branch_one(float v) {
    for (int k = 0; k < STEPS; k++)
        v = v * 0.999f + 0.001f;
    return v;
}

// the second branch: STEPS dependent multiply-adds, v -> v * v - 0.25
__device__ float branch_two(float v) {
    for (int k = 0; k < STEPS; k++)
        v = v * v - 0.25f;
    return v;
}

__global__ void branch_by_lane(float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    float v = i * 1e-7f;
    if (threadIdx.x % 2 == 0)                         // even and odd lanes of every warp
        v = branch_one(v);
    else
        v = branch_two(v);
    if (i < n) y[i] = v;
}

__global__ void branch_by_warp(float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    float v = i * 1e-7f;
    if (threadIdx.x / 32 % 2 == 0)                    // even and odd warps of every block
        v = branch_one(v);
    else
        v = branch_two(v);
    if (i < n) y[i] = v;
}

typedef void (*kernel)(float *, int);

int main(void) {
    const int n = 1 << 22, threads = 256, blocks = n / threads, reps = 10;
    float *y;
    CHECK(cudaMalloc(&y, n * sizeof(float)));
    cudaEvent_t t0, t1;
    CHECK(cudaEventCreate(&t0));
    CHECK(cudaEventCreate(&t1));
    const char *name[] = {"branch_by_lane", "branch_by_warp"};
    const char *test[] = {"threadIdx.x % 2", "threadIdx.x / 32 % 2"};
    kernel run[] = {branch_by_lane, branch_by_warp};
    float ms[2];

    printf("n = %d threads, %d multiply-adds per thread\n", n, STEPS);
    printf("%-15s %-21s %9s\n", "kernel", "branch on", "time(ms)");
    for (int k = 0; k < 2; k++) {
        run[k]<<<blocks, threads>>>(y, n);            // warm-up
        CHECK(cudaGetLastError());
        CHECK(cudaEventRecord(t0));
        for (int r = 0; r < reps; r++)
            run[k]<<<blocks, threads>>>(y, n);
        CHECK(cudaEventRecord(t1));
        CHECK(cudaEventSynchronize(t1));
        CHECK(cudaEventElapsedTime(&ms[k], t0, t1));
        ms[k] /= reps;
        printf("%-15s %-21s %9.3f\n", name[k], test[k], ms[k]);
    }
    printf("ratio %.2f\n", ms[0] / ms[1]);
    cudaFree(y);
    return 0;
}
