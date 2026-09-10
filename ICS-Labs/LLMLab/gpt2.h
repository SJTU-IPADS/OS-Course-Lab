#ifndef GPT2_H
#define GPT2_H

#include <stddef.h>

#define NUM_PARAMETER_TENSORS 16
#define NUM_ACTIVATION_TENSORS 23

// ----------------------------------------------------------------------------
// GPT-2 model definition

// Define the KVCache structure
typedef struct {
    float *k_cache;     // store the key cache for all layers
    float *v_cache;     // store the value cache for all layers
    int cache_capacity; // cache capacity (maximum sequence length)
    int cache_len;      // current cache length
    int is_initialized; // indicate if the cache is initialized
} KVCache;

typedef struct {
    int max_seq_len; // max sequence length, e.g. 1024
    int vocab_size;  // vocab size, e.g. 50257
    int num_layers;  // number of layers, e.g. 12
    int num_heads;   // number of heads in attention, e.g. 12
    int channels;    // number of channels, e.g. 768
} GPT2Config;

typedef struct {
    float *wte;      // (V, C)
    float *wpe;      // (maxT, C)
    float *ln1w;     // (L, C)
    float *ln1b;     // (L, C)
    float *qkvw;     // (L, 3*C, C)
    float *qkvb;     // (L, 3*C)
    float *attprojw; // (L, C, C)
    float *attprojb; // (L, C)
    float *ln2w;     // (L, C)
    float *ln2b;     // (L, C)
    float *fcw;      // (L, 4*C, C)
    float *fcb;      // (L, 4*C)
    float *fcprojw;  // (L, C, 4*C)
    float *fcprojb;  // (L, C)
    float *lnfw;     // (C)
    float *lnfb;     // (C)
} ParameterTensors;

typedef struct {
    float *encoded;   // (B, T, C)
    float *ln1;       // (L, B, T, C)
    float *ln1_mean;  // (L, B, T)
    float *ln1_rstd;  // (L, B, T)
    float *qkv;       // (L, B, T, 3*C)
    float *atty;      // (L, B, T, C)
    float *preatt;    // (L, B, NH, T, T)
    float *att;       // (L, B, NH, T, T)
    float *attproj;   // (L, B, T, C)
    float *residual2; // (L, B, T, C)
    float *ln2;       // (L, B, T, C)
    float *ln2_mean;  // (L, B, T)
    float *ln2_rstd;  // (L, B, T)
    float *fch;       // (L, B, T, 4*C)
    float *fch_gelu;  // (L, B, T, 4*C)
    float *fcproj;    // (L, B, T, C)
    float *residual3; // (L, B, T, C)
    float *lnf;       // (B, T, C)
    float *lnf_mean;  // (B, T)
    float *lnf_rstd;  // (B, T)
    float *logits;    // (B, T, V)
    float *probs;     // (B, T, V)
    float *losses;    // (B, T)
} ActivationTensors;

// add a new structure to track mmap memory
typedef struct {
    void *data;
    size_t size;
} MmapData;

typedef struct {
    GPT2Config config;
    // the weights (parameters) of the model, and their sizes
    ParameterTensors params;
    size_t param_sizes[NUM_PARAMETER_TENSORS];
    float *params_memory;
    int num_parameters;
    // gradients of the weights
    ParameterTensors grads;
    float *grads_memory;
    // buffers for the AdamW optimizer
    float *m_memory;
    float *v_memory;
    // the activations of the model, and their sizes
    ActivationTensors acts;
    size_t act_sizes[NUM_ACTIVATION_TENSORS];
    float *acts_memory;
    int num_activations;
    // gradients of the activations
    ActivationTensors grads_acts;
    float *grads_acts_memory;
    // other run state configuration
    int batch_size;     // the batch size (B) of current forward pass
    int seq_len;        // the sequence length (T) of current forward pass
    int *inputs;        // the input tokens for the current forward pass
    int *targets;       // the target tokens for the current forward pass
    float mean_loss;    // after a forward pass with targets, will be populated with
                        // the mean loss
    MmapData mmap_data; // add mmap data field
    KVCache kv_cache;
} GPT2;

void init_kv_cache(GPT2 *model);
void clear_kv_cache(GPT2 *model);
void gpt2_forward_with_kvcache(GPT2 *model, int *inputs, int B, int T);

#endif
