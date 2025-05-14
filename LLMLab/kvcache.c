#include "gpt2.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// add declarations for other functions if needed
extern float *malloc_and_point_activations(ActivationTensors *acts, size_t *act_sizes);
extern void attention_forward(float *out, float *preatt, float *att, float *inp, int B, int T, int C, int NH);
extern void layernorm_forward(float *out, float *mean, float *rstd, float *inp, float *w, float *b, int B, int T,
                              int C);
extern void matmul_forward(float *out, float *inp, float *w, float *b, int B, int T, int C, int C2);
extern void residual_forward(float *out, float *inp1, float *inp2, int N);
extern void encoder_forward(float *out, int *inp, float *wte, float *wpe, int B, int T, int C);
extern void softmax_forward(float *out, float *inp, int B, int T, int V);
extern void gelu_forward(float *out, float *inp, int N);

void init_kv_cache(GPT2 *model)
{
    // TODO: implement the initialization of the KV cache
}

void clear_kv_cache(GPT2 *model)
{
    // TODO: implement the clearing of the KV cache
}

void gpt2_forward_with_kvcache(GPT2 *model, int *inputs, int B, int T)
{
    // TODO: implement the forward pass with KV cache
}