/* cuda.cu -- the operators of mini-ollama as CUDA kernels, on the GPU. All
 * the code you write in step 2 goes in this file.
 *
 * Every operator of ops.h is a host function below that launches
 * one kernel; argmax also copies its result back. One kernel body is yours:
 * matvec_q4_0_kernel, which the host function matvec_q4_0 launches. It is
 * a todo() call now; the comment above it gives its launch configuration
 * and what each thread does. Leave the host functions and the launch
 * configurations as they are. The other kernels show the same tools at
 * work and can be read as examples: rmsnorm_kernel folds a sum in shared
 * memory once, as the kernel you write does, and softmax_kernel twice.
 *
 *   make cuda            build mini-ollama-cuda and ops-selftest-cuda
 *   ./ops-selftest-cuda  check every operator against ops/scalar.c, then
 *                        time matvec on the GPU and on the CPU
 *   make run-cuda        answer prompt.txt with these operators
 *   make run-scalar      the same with the scalar operators on the CPU
 *   make remote          build and run both programs on the course server
 *
 * A kernel finds its work from the index of its thread: blockIdx.x (and
 * blockIdx.y) give the block in the grid, threadIdx.x the thread in the
 * block, blockDim.x the threads per block. A grid usually has a few more
 * threads than there are elements; a thread past the end does nothing.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ops.h"

/* Threads per block for the kernels that fold in shared memory: the shared
 * array of a block has THREADS elements. A power of 2. */
#define THREADS 256

/* Every call of the CUDA runtime goes through CHECK: an error is reported
 * with the line of the call, and the program exits. An error inside a
 * kernel (a read past the end of an array) is reported by the next call
 * that waits for the GPU. */
#define CHECK(call) check((call), __LINE__)

static void check(cudaError_t e, int line)
{
    if (e != cudaSuccess) {
        fprintf(stderr, "cuda.cu:%d: %s\n", line, cudaGetErrorString(e));
        exit(1);
    }
}

/* A kernel that is not written yet calls todo("name"). After the first
 * launch of such a kernel the host function reads the name back, reports
 * it and exits with status 3. (Device code cannot call exit.) todo() is
 * not static, so that nvcc does not warn about it once no kernel calls
 * it. */
static __device__ char todo_name[64];

__device__ void todo(const char *name)
{
    if (blockIdx.x == 0 && threadIdx.x == 0) {
        int i;
        for (i = 0; i < 63 && name[i]; i++)
            todo_name[i] = name[i];
        todo_name[i] = 0;
    }
}

static void check_todo(int *checked)
{
    char name[64];
    if (*checked)
        return;
    CHECK(cudaMemcpyFromSymbol(name, todo_name, sizeof name));     /* waits for the kernel */
    if (name[0]) {
        fprintf(stderr, "%s is not implemented yet\n", name);
        exit(3);
    }
    *checked = 1;
}

/* After every launch: a launch with a wrong configuration fails at once. */
static void launched(void)
{
    CHECK(cudaGetLastError());
}

/* Blocks of `threads` threads for n elements, one element per thread. */
static unsigned blocks(size_t n, int threads)
{
    return (unsigned)((n + threads - 1) / threads);
}

/* ---------------- matvec ---------------- */

/* y[row] for row = blockIdx.x: one block per row, launched
 * <<<rows, THREADS>>>.
 *
 * TODO: replace the todo() call with the kernel. Thread t adds up
 * W[row][c] * x[c] for the columns c = t, t + THREADS, t + 2 THREADS, ...
 * in a register. Column c is weight c % 32 of block c / 32 of the row (the
 * row starts at w + row * (cols / 32 * 18)); its code is in byte
 * 2 + c % 16 of that block, in the low 4 bits when c % 32 < 16 and in the
 * high 4 bits otherwise, and the weight is q4_0_scale(block) * (code - 8).
 * Then the block adds up its THREADS partial sums in a shared array,
 * halving their number at each step (the fold of dot_kernel in the
 * lecture's dot_cuda.cu, also in rmsnorm_kernel below), and thread 0
 * writes y[row]. The 32 threads of a
 * warp then read adjacent bytes of the row. */
__global__ void matvec_q4_0_kernel(float *y, const uint8_t *w, const float *x, int cols)
{
    todo("matvec_q4_0_kernel");
}

static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols)
{
    static int checked;
    matvec_q4_0_kernel<<<(unsigned)rows, THREADS>>>(y, w, x, (int)cols);
    launched();
    check_todo(&checked);
}

/* ---------------- embed ---------------- */

/* One thread per weight: thread j of block b decodes weight j of Q4_0
 * block b. Launched <<<n / 32, 32>>>. */
__global__ void embed_kernel(float *x, const uint8_t *row)
{
    int b = blockIdx.x, j = threadIdx.x;
    const uint8_t *blk = row + b * Q4_0_BLOCK_BYTES;
    int q = j < 16 ? blk[2 + j] & 15 : blk[2 + j - 16] >> 4;
    x[b * 32 + j] = q4_0_scale(blk) * (q - 8);
}

static void embed(float *x, const uint8_t *row, size_t n)
{
    embed_kernel<<<(unsigned)(n / 32), 32>>>(x, row);
    launched();
}

/* ---------------- rmsnorm ---------------- */

/* Vector v = blockIdx.x of x (d floats from x + v * d), normalized into
 * the same place of y: one block per vector, launched <<<n_vec, THREADS>>>.
 * Thread t adds up x[i]^2 for i = t, t + THREADS, t + 2 THREADS, ... of
 * the vector in a register; the block adds up the THREADS sums in a shared
 * array, halving their number at each step; then every thread takes the
 * total from part[0], computes scale = 1 / sqrtf(total / d + eps), and
 * writes y[i] = x[i] * scale * gain[i] for the same i as before. y may be
 * x: a thread writes only the elements it has read itself. */
__global__ void rmsnorm_kernel(float *y, const float *x, const float *gain, int d, float eps)
{
    __shared__ float part[THREADS];
    const float *u = x + (size_t)blockIdx.x * d;
    float *out = y + (size_t)blockIdx.x * d;
    int t = threadIdx.x;
    float ss = 0;
    for (int i = t; i < d; i += THREADS)
        ss += u[i] * u[i];
    part[t] = ss;
    __syncthreads();
    for (int k = THREADS / 2; k > 0; k /= 2) {          /* the sum of 2k values */
        if (t < k)
            part[t] += part[t + k];
        __syncthreads();
    }
    float scale = 1 / sqrtf(part[0] / d + eps);
    for (int i = t; i < d; i += THREADS)
        out[i] = u[i] * scale * gain[i];
}

static void rmsnorm(float *y, const float *x, const float *gain, int n_vec, int d, float eps)
{
    rmsnorm_kernel<<<n_vec, THREADS>>>(y, x, gain, d, eps);
    launched();
}

/* ---------------- rope ---------------- */

/* One thread per pair: thread i of block h turns the pair (v[i],
 * v[i + d/2]) of vector h. Launched <<<n_vec, d / 2>>>.
 *
 * pow, cos and sin are taken in double and rounded to float: powf, cosf
 * and sinf of the GPU are a few units in the last place away from those of
 * the CPU, and an angle of up to 511 radians turns that into a visible
 * difference from ops/scalar.c. The rounded double results are those of
 * the CPU functions. */
__global__ void rope_kernel(float *v, int d, int pos, float base)
{
    int i = threadIdx.x, half = d / 2;
    float *u = v + blockIdx.x * d;
    float angle = pos * (float)pow((double)base, (double)(-2.0f * i / d));
    float c = (float)cos((double)angle), s = (float)sin((double)angle);
    float a = u[i], b = u[i + half];
    u[i]        = a * c - b * s;
    u[i + half] = b * c + a * s;
}

static void rope(float *v, int n_vec, int d, int pos, float base)
{
    rope_kernel<<<n_vec, d / 2>>>(v, d, pos, base);
    launched();
}

/* ---------------- attention ---------------- */

/* One thread per score: thread t of row blockIdx.y of the grid computes
 * s[h * n_tok + t] for the head h = blockIdx.y. The block first copies the
 * query of its head into shared memory (d floats, given at the launch),
 * since every thread of the block reads all of it. Launched
 * <<<(ceil(n_tok / THREADS), n_head), THREADS, d floats>>>. */
__global__ void attn_scores_kernel(float *s, const float *q, const float *k_cache,
                                   int n_head, int n_head_kv, int d, int n_tok)
{
    extern __shared__ float qh[];
    int h = blockIdx.y, t = blockIdx.x * blockDim.x + threadIdx.x;
    for (int i = threadIdx.x; i < d; i += blockDim.x)
        qh[i] = q[h * d + i];
    __syncthreads();
    if (t >= n_tok)
        return;
    int off = h / (n_head / n_head_kv) * d;             /* its key head */
    const float *k = k_cache + (size_t)t * n_head_kv * d + off;
    float sum = 0;
    for (int i = 0; i < d; i++)
        sum += qh[i] * k[i];
    s[h * n_tok + t] = sum / sqrtf((float)d);
}

static void attn_scores(float *s, const float *q, const float *k_cache,
                        int n_head, int n_head_kv, int d, int n_tok)
{
    dim3 grid(blocks(n_tok, THREADS), n_head);
    attn_scores_kernel<<<grid, THREADS, d * sizeof(float)>>>(s, q, k_cache, n_head, n_head_kv, d, n_tok);
    launched();
}

/* One block per row of scores, launched <<<n_row, THREADS>>>. Two folds
 * in the shared array: first the largest score, then the sum of the
 * exponentials. Each thread works on the elements i = t, t + THREADS, ...
 * of the row. */
__global__ void softmax_kernel(float *s, int n)
{
    __shared__ float part[THREADS];
    float *r = s + (size_t)blockIdx.x * n;
    int t = threadIdx.x;

    float max = -INFINITY;
    for (int i = t; i < n; i += blockDim.x)
        max = fmaxf(max, r[i]);
    part[t] = max;
    __syncthreads();
    for (int k = blockDim.x / 2; k > 0; k /= 2) {       /* the largest of 2k values */
        if (t < k)
            part[t] = fmaxf(part[t], part[t + k]);
        __syncthreads();
    }
    max = part[0];
    __syncthreads();                                    /* everyone has read part[0] */

    float sum = 0;
    for (int i = t; i < n; i += blockDim.x) {
        float e = expf(r[i] - max);
        r[i] = e;
        sum += e;
    }
    part[t] = sum;
    __syncthreads();
    for (int k = blockDim.x / 2; k > 0; k /= 2) {       /* the sum of 2k values */
        if (t < k)
            part[t] += part[t + k];
        __syncthreads();
    }
    float total = part[0];
    for (int i = t; i < n; i += blockDim.x)
        r[i] /= total;
}

static void softmax(float *s, int n_row, int n)
{
    softmax_kernel<<<n_row, THREADS>>>(s, n);
    launched();
}

/* One thread per output element: thread j of block h adds up the scores
 * of head h times element j of the values, token after token. The 32
 * threads of a warp read adjacent floats of a value. Launched
 * <<<n_head, d>>>. */
__global__ void attn_values_kernel(float *out, const float *s, const float *v_cache,
                                   int n_head, int n_head_kv, int d, int n_tok)
{
    int h = blockIdx.x, j = threadIdx.x;
    int off = h / (n_head / n_head_kv) * d, kv = n_head_kv * d;
    float sum = 0;
    for (int t = 0; t < n_tok; t++)
        sum += s[h * n_tok + t] * v_cache[(size_t)t * kv + off + j];
    out[h * d + j] = sum;
}

static void attn_values(float *out, const float *s, const float *v_cache,
                        int n_head, int n_head_kv, int d, int n_tok)
{
    attn_values_kernel<<<n_head, d>>>(out, s, v_cache, n_head, n_head_kv, d, n_tok);
    launched();
}

/* ---------------- one element per thread ---------------- */

/* Launched <<<ceil(n / THREADS), THREADS>>>. */
__global__ void swiglu_kernel(float *gate, const float *up, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        float g = gate[i];
        gate[i] = g / (1 + expf(-g)) * up[i];
    }
}

static void swiglu(float *gate, const float *up, int n)
{
    swiglu_kernel<<<blocks(n, THREADS), THREADS>>>(gate, up, n);
    launched();
}

/* Launched <<<ceil(n / THREADS), THREADS>>>. */
__global__ void add_kernel(float *x, const float *y, int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        x[i] += y[i];
}

static void add(float *x, const float *y, int n)
{
    add_kernel<<<blocks(n, THREADS), THREADS>>>(x, y, n);
    launched();
}

/* ---------------- argmax ---------------- */

#define ARGMAX_THREADS 1024             /* one block, as large as a block can be */

static __device__ int argmax_result;

/* One block. Thread t finds the largest of x[t], x[t + 1024], ... and the
 * first index that holds it; the block then folds the 1024 (value, index)
 * pairs, keeping the larger value, and of two equal values the smaller
 * index. Launched <<<1, ARGMAX_THREADS>>>. */
__global__ void argmax_kernel(const float *x, int n)
{
    __shared__ float val[ARGMAX_THREADS];
    __shared__ int idx[ARGMAX_THREADS];
    int t = threadIdx.x, at = -1;                       /* -1: no element yet */
    float best = 0;
    for (int i = t; i < n; i += blockDim.x)
        if (at < 0 || x[i] > best) {
            best = x[i];
            at = i;
        }
    val[t] = best;
    idx[t] = at;
    __syncthreads();
    for (int k = blockDim.x / 2; k > 0; k /= 2) {
        if (t < k) {
            int j = idx[t + k];
            if (j >= 0 && (idx[t] < 0 || val[t + k] > val[t] ||
                           (val[t + k] == val[t] && j < idx[t]))) {
                val[t] = val[t + k];
                idx[t] = j;
            }
        }
        __syncthreads();
    }
    if (t == 0)
        argmax_result = idx[0];
}

static int argmax(const float *x, int n)
{
    int i;
    argmax_kernel<<<1, ARGMAX_THREADS>>>(x, n);
    launched();
    CHECK(cudaMemcpyFromSymbol(&i, argmax_result, sizeof i));   /* waits for the kernel */
    return i;
}

/* ---------------- the GPU and its memory ---------------- */

static const char *init(void)
{
    static char line[1024];
    int dev, sms, khz, bits;
    cudaDeviceProp p;
    CHECK(cudaGetDevice(&dev));
    CHECK(cudaGetDeviceProperties(&p, dev));
    CHECK(cudaDeviceGetAttribute(&sms, cudaDevAttrMultiProcessorCount, dev));
    CHECK(cudaDeviceGetAttribute(&khz, cudaDevAttrMemoryClockRate, dev));
    CHECK(cudaDeviceGetAttribute(&bits, cudaDevAttrGlobalMemoryBusWidth, dev));
    /* peak bandwidth: 2 transfers per memory clock, bus width in bytes */
    snprintf(line, sizeof line, "%s, %d SMs, %.1f GiB, peak %.0f GB/s",
             p.name, sms, p.totalGlobalMem / 1073741824.0, 2.0 * khz * 1e3 * (bits / 8) / 1e9);
    return line;
}

static void *alloc(size_t bytes)
{
    void *p;
    CHECK(cudaMalloc(&p, bytes ? bytes : 1));
    CHECK(cudaMemset(p, 0, bytes));
    return p;
}

static void *upload(const void *host, size_t bytes)
{
    void *p;
    CHECK(cudaMalloc(&p, bytes ? bytes : 1));
    CHECK(cudaMemcpy(p, host, bytes, cudaMemcpyHostToDevice));
    return p;
}

static void download(void *host, const void *mem, size_t bytes)
{
    CHECK(cudaMemcpy(host, mem, bytes, cudaMemcpyDeviceToHost));
}

static void finish(void)
{
    CHECK(cudaDeviceSynchronize());
}

extern "C" const struct ops ops_cuda = {
    "cuda (ops/cuda.cu)",
    init, alloc, upload, download, finish,
    matvec_q4_0, embed, rmsnorm, rope,
    attn_scores, softmax, attn_values, swiglu, add, argmax,
};
