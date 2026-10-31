/* Exercise: y = a * x + y for n = 10^7 floats on the GPU, one thread per
   element. main is complete: it copies x and y to the device, launches
   axpy, copies y back and compares it with the result of the CPU. Write
   the body of axpy. "differ 0" means every element is right.

     curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @axpy.cu $GPU_SERVER/program */
#include <stdio.h>
#include <stdlib.h>

#define CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); \
    exit(1); } } while (0)

__global__ void axpy(float a, const float *x, float *y, int n) {
    // TODO: y[i] = a * x[i] + y[i] for the element of this thread
}

int main(void) {
    const int n = 10000000, threads = 256;
    const int blocks = (n + threads - 1) / threads;   // the last block has threads past n
    const float a = 2.5f;
    size_t bytes = n * sizeof(float);
    float *x = (float *)malloc(bytes), *y = (float *)malloc(bytes);
    float *check = (float *)malloc(bytes);            // the result of the CPU
    for (int i = 0; i < n; i++) {
        x[i] = i % 100;
        y[i] = 1.0f;
        check[i] = a * x[i] + y[i];
    }

    float *dx, *dy;
    CHECK(cudaMalloc(&dx, bytes));
    CHECK(cudaMalloc(&dy, bytes));
    CHECK(cudaMemcpy(dx, x, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dy, y, bytes, cudaMemcpyHostToDevice));
    axpy<<<blocks, threads>>>(a, dx, dy, n);
    CHECK(cudaGetLastError());
    CHECK(cudaMemcpy(y, dy, bytes, cudaMemcpyDeviceToHost));   // waits for axpy
    CHECK(cudaFree(dx));
    CHECK(cudaFree(dy));

    int differ = 0;
    for (int i = 0; i < n; i++)
        differ += y[i] != check[i];
    printf("blocks  %d x %d threads\ny[n-1]  %.1f\ndiffer  %d\n",
           blocks, threads, y[n - 1], differ);
    free(x); free(y); free(check);
    return 0;
}
