// Original Author: Andrej Karpathy
// https://github.com/karpathy/llm.c
#include "thread-sync.h"
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
// ----------------------------------------------------------------------------
// all the individual layers' forward passes
// B = batch_size, T = sequence_length, C = channels, V = vocab_size
void encoder_forward(float *out, int *inp, float *wte, float *wpe, int B, int T,
                     int C) {
  // out is (B,T,C). At each position (b,t), a C-dimensional vector summarizing
  // token & position inp is (B,T) of integers, holding the token ids at each
  // (b,t) position wte is (V,C) of token embeddings, short for "weight token
  // embeddings" wpe is (maxT,C) of position embeddings, short for "weight
  // positional embedding"
  for (int b = 0; b < B; b++) {
    for (int t = 0; t < T; t++) {
      // seek to the output position in out[b,t,:]
      float *out_bt = out + b * T * C + t * C;
      // get the index of the token at inp[b, t]
      int ix = inp[b * T + t];
      // seek to the position in wte corresponding to the token
      float *wte_ix = wte + ix * C;
      // seek to the position in wpe corresponding to the position
      float *wpe_t = wpe + t * C;
      // add the two vectors and store the result in out[b,t,:]
      for (int i = 0; i < C; i++) {
        out_bt[i] = wte_ix[i] + wpe_t[i];
      }
    }
  }
}
void layernorm_forward(float *out, float *mean, float *rstd, float *inp,
                       float *weight, float *bias, int B, int T, int C) {
  // reference:
  // https://pytorch.org/docs/stable/generated/torch.nn.LayerNorm.html both inp
  // and out are (B,T,C) of the activations mean and rstd are (B,T) buffers, to
  // be used later in backward pass at each position (b,t) of the input, the
  // C-dimensional vector of activations gets normalized, then scaled and
  // shifted
  float eps = 1e-5f;
  for (int b = 0; b < B; b++) {
    for (int t = 0; t < T; t++) {
      // seek to the input position inp[b,t,:]
      float *x = inp + b * T * C + t * C;
      // calculate the mean
      float m = 0.0f;
      for (int i = 0; i < C; i++) {
        m += x[i];
      }
      m = m / C;
      // calculate the variance (without any bias correction)
      float v = 0.0f;
      for (int i = 0; i < C; i++) {
        float xshift = x[i] - m;
        v += xshift * xshift;
      }
      v = v / C;
      // calculate the rstd (reciprocal standard deviation)
      float s = 1.0f / sqrtf(v + eps);
      // seek to the output position in out[b,t,:]
      float *out_bt = out + b * T * C + t * C;
      for (int i = 0; i < C; i++) {
        float n = (s * (x[i] - m));        // normalize
        float o = n * weight[i] + bias[i]; // scale and shift
        out_bt[i] = o;                     // write
      }
      // cache the mean and rstd for the backward pass later
      mean[b * T + t] = m;
      rstd[b * T + t] = s;
    }
  }
}
void matmul_forward(float *out, float *inp, float *weight, float *bias, int B,
                    int T, int C, int OC) {
  // most of the running time is spent here and in matmul_backward
  // OC is short for "output channels"
  // inp is (B,T,C), weight is (OC, C), bias is (OC)
  // out will be (B,T,OC)
  for (int b = 0; b < B; b++) {
    for (int t = 0; t < T; t++) {
      float *out_bt = out + b * T * OC + t * OC;
      float *inp_bt = inp + b * T * C + t * C;
      for (int o = 0; o < OC; o++) {
        float val = (bias != NULL) ? bias[o] : 0.0f;
        float *wrow = weight + o * C;
        for (int i = 0; i < C; i++) {
          val += inp_bt[i] * wrow[i];
        }
        out_bt[o] = val;
      }
    }
  }
}
void attention_forward(float *out, float *preatt, float *att, float *inp, int B,
                       int T, int C, int NH) {
  // input is (B, T, 3C) holding the query, key, value (Q, K, V) vectors
  // preatt, att are (B, NH, T, T). NH = number of heads, T = sequence length
  // that holds the pre-attention and post-attention scores (used in backward)
  // output is (B, T, C)
  // attention is the only layer that mixes information across time
  // every other operation is applied at every (b,t) position independently
  // (and of course, no layer mixes information across batch)
  int C3 = C * 3;
  int hs = C / NH; // head size
  float scale = 1.0 / sqrtf(hs);
  for (int b = 0; b < B; b++) {
    for (int t = 0; t < T; t++) {
      for (int h = 0; h < NH; h++) {
        float *query_t = inp + b * T * C3 + t * C3 + h * hs;
        float *preatt_bth = preatt + b * NH * T * T + h * T * T + t * T;
        float *att_bth = att + b * NH * T * T + h * T * T + t * T;
        // pass 1: calculate query dot key and maxval
        float maxval = -10000.0f; // TODO something better
        for (int t2 = 0; t2 <= t; t2++) {
          float *key_t2 =
              inp + b * T * C3 + t2 * C3 + h * hs + C; // +C because it's key
          // (query_t) dot (key_t2)
          float val = 0.0f;
          for (int i = 0; i < hs; i++) {
            val += query_t[i] * key_t2[i];
          }
          val *= scale;
          if (val > maxval) {
            maxval = val;
          }
          preatt_bth[t2] = val;
        }
        // pass 2: calculate the exp and keep track of sum
        // maxval is being calculated and subtracted only for numerical
        // stability
        float expsum = 0.0f;
        for (int t2 = 0; t2 <= t; t2++) {
          float expv = expf(preatt_bth[t2] - maxval);
          expsum += expv;
          att_bth[t2] = expv;
        }
        float expsum_inv = expsum == 0.0f ? 0.0f : 1.0f / expsum;
        // pass 3: normalize to get the softmax
        for (int t2 = 0; t2 < T; t2++) {
          if (t2 <= t) {
            att_bth[t2] *= expsum_inv;
          } else {
            // causal attention mask. not strictly necessary to set to zero here
            // only doing this explicitly for debugging and checking to PyTorch
            att_bth[t2] = 0.0f;
          }
        }
        // pass 4: accumulate weighted values into the output of attention
        float *out_bth = out + b * T * C + t * C + h * hs;
        for (int i = 0; i < hs; i++) {
          out_bth[i] = 0.0f;
        }
        for (int t2 = 0; t2 <= t; t2++) {
          float *value_t2 = inp + b * T * C3 + t2 * C3 + h * hs +
                            C * 2; // +C*2 because it's value
          float att_btht2 = att_bth[t2];
          for (int i = 0; i < hs; i++) {
            out_bth[i] += att_btht2 * value_t2[i];
          }
        }
      }
    }
  }
}
#define GELU_SCALING_FACTOR sqrtf(2.0f / M_PI)
void gelu_forward(float *out, float *inp, int N) {
  // (approximate) GeLU elementwise non-linearity in the MLP block of
  // Transformer
  for (int i = 0; i < N; i++) {
    float x = inp[i];
    float cube = 0.044715f * x * x * x;
    out[i] = 0.5f * x * (1.0f + tanhf(GELU_SCALING_FACTOR * (x + cube)));
  }
}
void residual_forward(float *out, float *inp1, float *inp2, int N) {
  for (int i = 0; i < N; i++) {
    out[i] = inp1[i] + inp2[i];
  }
}
void softmax_forward(float *probs, float *logits, int B, int T, int V) {
  // output: probs are (B,T,V) of the probabilities (sums to 1.0 in each b,t
  // position) input: logits is (B,T,V) of the unnormalized log probabilities
  for (int b = 0; b < B; b++) {
    for (int t = 0; t < T; t++) {
      // probs <- softmax(logits)
      float *logits_bt = logits + b * T * V + t * V;
      float *probs_bt = probs + b * T * V + t * V;
      // maxval is only calculated and subtracted for numerical stability
      float maxval = -10000.0f; // TODO something better
      for (int i = 0; i < V; i++) {
        if (logits_bt[i] > maxval) {
          maxval = logits_bt[i];
        }
      }
      float sum = 0.0f;
      for (int i = 0; i < V; i++) {
        probs_bt[i] = expf(logits_bt[i] - maxval);
        sum += probs_bt[i];
      }
      for (int i = 0; i < V; i++) {
        probs_bt[i] /= sum;
      }
    }
  }
}
// ----------------------------------------------------------------------------
// GPT-2 model definition
// the parameters of the model
#define NUM_PARAMETER_TENSORS 16
#define NUM_ACTIVATION_TENSORS 23

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
  int batch_size;  // the batch size (B) of current forward pass
  int seq_len;     // the sequence length (T) of current forward pass
  int *inputs;     // the input tokens for the current forward pass
  int *targets;    // the target tokens for the current forward pass
  float mean_loss; // after a forward pass with targets, will be populated with
                   // the mean loss
  MmapData mmap_data; // add mmap data field
} GPT2;

// allocate memory for the parameters and point the individual tensors to the
// right places
float *malloc_and_point_parameters(ParameterTensors *params,
                                   size_t *param_sizes, float *params_base) {
  size_t num_parameters = 0;
  for (size_t i = 0; i < NUM_PARAMETER_TENSORS; i++) {
    num_parameters += param_sizes[i];
  }

  // use the params_base instead of allocating new memory
  float *params_memory = params_base;

  // assign all the tensors
  float **ptrs[] = {
      &params->wte,     &params->wpe,     &params->ln1w,     &params->ln1b,
      &params->qkvw,    &params->qkvb,    &params->attprojw, &params->attprojb,
      &params->ln2w,    &params->ln2b,    &params->fcw,      &params->fcb,
      &params->fcprojw, &params->fcprojb, &params->lnfw,     &params->lnfb};
  float *params_memory_iterator = params_memory;
  for (size_t i = 0; i < NUM_PARAMETER_TENSORS; i++) {
    *(ptrs[i]) = params_memory_iterator;
    params_memory_iterator += param_sizes[i];
  }
  return params_memory;
}

void gpt2_build_from_checkpoint(GPT2 *model, char *checkpoint_path) {
  // use open to open the file
  int fd = open(checkpoint_path, O_RDONLY);
  if (fd == -1) {
    printf("Error opening model file\n");
    exit(1);
  }

  // get the file size
  struct stat st;
  if (fstat(fd, &st) == -1) {
    printf("Error getting file size\n");
    close(fd);
    exit(1);
  }

  // use mmap to map the file
  void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    printf("Error mapping file\n");
    close(fd);
    exit(1);
  }

  // suggest the operating system that this memory region will be accessed
  // sequentially
  madvise(file_data, st.st_size, MADV_SEQUENTIAL);

  // save the mmap data for later cleanup
  model->mmap_data.data = file_data;
  model->mmap_data.size = st.st_size;

  // read the model header information
  int *model_header = (int *)file_data;
  if (model_header[0] != 20240326) {
    printf("Bad magic model file\n");
    munmap(file_data, st.st_size);
    close(fd);
    exit(1);
  }
  if (model_header[1] != 1) {
    printf("Bad version in model file\n");
    munmap(file_data, st.st_size);
    close(fd);
    exit(1);
  }

  // read in hyperparameters
  int maxT, V, L, NH, C;
  model->config.max_seq_len = maxT = model_header[2];
  model->config.vocab_size = V = model_header[3];
  model->config.num_layers = L = model_header[4];
  model->config.num_heads = NH = model_header[5];
  model->config.channels = C = model_header[6];

  // allocate space for all the parameters and read them in
  model->param_sizes[0] = V * C;            // wte
  model->param_sizes[1] = maxT * C;         // wpe
  model->param_sizes[2] = L * C;            // ln1w
  model->param_sizes[3] = L * C;            // ln1b
  model->param_sizes[4] = L * (3 * C) * C;  // qkvw
  model->param_sizes[5] = L * (3 * C);      // qkvb
  model->param_sizes[6] = L * C * C;        // attprojw
  model->param_sizes[7] = L * C;            // attprojb
  model->param_sizes[8] = L * C;            // ln2w
  model->param_sizes[9] = L * C;            // ln2b
  model->param_sizes[10] = L * (4 * C) * C; // fcw
  model->param_sizes[11] = L * (4 * C);     // fcb
  model->param_sizes[12] = L * C * (4 * C); // fcprojw
  model->param_sizes[13] = L * C;           // fcprojb
  model->param_sizes[14] = C;               // lnfw
  model->param_sizes[15] = C;               // lnfb

  // cound the number of paramaters
  size_t num_parameters = 0;
  for (size_t i = 0; i < NUM_PARAMETER_TENSORS; i++) {
    num_parameters += model->param_sizes[i];
  }
  model->num_parameters = num_parameters;

  // directly use the mmap memory, avoid copying
  float *params_base = (float *)(file_data + 256 * sizeof(int));
  model->params_memory = malloc_and_point_parameters(
      &model->params, model->param_sizes, params_base);

  // close the file descriptor (the mmap memory is still valid)
  close(fd);

  // other inits
  model->acts_memory = NULL;
  model->grads_memory = NULL;
  model->m_memory = NULL;
  model->v_memory = NULL;
  model->grads_acts_memory = NULL;
  model->inputs = NULL;
  model->targets = NULL;
  model->batch_size = 0;
  model->seq_len = 0;
  model->mean_loss = -1.0f; // -1.0f will designate no loss
}

float *malloc_and_point_activations(ActivationTensors *acts,
                                    size_t *act_sizes) {
  size_t num_activations = 0;
  for (size_t i = 0; i < NUM_ACTIVATION_TENSORS; i++) {
    num_activations += act_sizes[i];
  }
  float *acts_memory = (float *)malloc(num_activations * sizeof(float));
  float **ptrs[] = {
      &acts->encoded,   &acts->ln1,       &acts->ln1_mean, &acts->ln1_rstd,
      &acts->qkv,       &acts->atty,      &acts->preatt,   &acts->att,
      &acts->attproj,   &acts->residual2, &acts->ln2,      &acts->ln2_mean,
      &acts->ln2_rstd,  &acts->fch,       &acts->fch_gelu, &acts->fcproj,
      &acts->residual3, &acts->lnf,       &acts->lnf_mean, &acts->lnf_rstd,
      &acts->logits,    &acts->probs,     &acts->losses};
  float *acts_memory_iterator = acts_memory;
  for (size_t i = 0; i < NUM_ACTIVATION_TENSORS; i++) {
    *(ptrs[i]) = acts_memory_iterator;
    acts_memory_iterator += act_sizes[i];
  }
  return acts_memory;
}
void gpt2_forward(GPT2 *model, int *inputs, int B, int T) {
  // convenience parameters
  int V = model->config.vocab_size;
  int L = model->config.num_layers;
  int NH = model->config.num_heads;
  int C = model->config.channels;
  // record the current B,T as well
  model->batch_size = B;
  model->seq_len = T;
  // and now allocate the space
  model->act_sizes[0] = B * T * C;          // encoded
  model->act_sizes[1] = L * B * T * C;      // ln1
  model->act_sizes[2] = L * B * T;          // ln1_mean
  model->act_sizes[3] = L * B * T;          // ln1_rstd
  model->act_sizes[4] = L * B * T * 3 * C;  // qkv
  model->act_sizes[5] = L * B * T * C;      // atty
  model->act_sizes[6] = L * B * NH * T * T; // preatt
  model->act_sizes[7] = L * B * NH * T * T; // att
  model->act_sizes[8] = L * B * T * C;      // attproj
  model->act_sizes[9] = L * B * T * C;      // residual2
  model->act_sizes[10] = L * B * T * C;     // ln2
  model->act_sizes[11] = L * B * T;         // ln2_mean
  model->act_sizes[12] = L * B * T;         // ln2_rstd
  model->act_sizes[13] = L * B * T * 4 * C; // fch
  model->act_sizes[14] = L * B * T * 4 * C; // fch_gelu
  model->act_sizes[15] = L * B * T * C;     // fcproj
  model->act_sizes[16] = L * B * T * C;     // residual3
  model->act_sizes[17] = B * T * C;         // lnf
  model->act_sizes[18] = B * T;             // lnf_mean
  model->act_sizes[19] = B * T;             // lnf_rstd
  model->act_sizes[20] = B * T * V;         // logits
  model->act_sizes[21] = B * T * V;         // probs
  model->act_sizes[22] = B * T;             // losses
  size_t num_activations = 0;
  for (size_t i = 0; i < NUM_ACTIVATION_TENSORS; i++) {
    num_activations += model->act_sizes[i];
  }
  model->num_activations = num_activations;
  if (model->acts_memory) {
    free(model->acts_memory);
    model->acts_memory = NULL;
  }
  model->acts_memory =
      malloc_and_point_activations(&model->acts, model->act_sizes);
  // also create memory for caching inputs and targets
  if (model->inputs) {
    free(model->inputs);
  }
  model->inputs = (int *)malloc(B * T * sizeof(int));
  // cache the inputs/targets
  memcpy(model->inputs, inputs, B * T * sizeof(int));
  // forward pass
  ParameterTensors params = model->params; // for brevity
  ActivationTensors acts = model->acts;
  float *residual;
  encoder_forward(acts.encoded, inputs, params.wte, params.wpe, B, T,
                  C); // encoding goes into residual[0]
  for (int l = 0; l < L; l++) {
    residual = l == 0 ? acts.encoded : acts.residual3 + (l - 1) * B * T * C;
    // get the pointers of the weights for this layer
    float *l_ln1w = params.ln1w + l * C;
    float *l_ln1b = params.ln1b + l * C;
    float *l_qkvw = params.qkvw + l * 3 * C * C;
    float *l_qkvb = params.qkvb + l * 3 * C;
    float *l_attprojw = params.attprojw + l * C * C;
    float *l_attprojb = params.attprojb + l * C;
    float *l_ln2w = params.ln2w + l * C;
    float *l_ln2b = params.ln2b + l * C;
    float *l_fcw = params.fcw + l * 4 * C * C;
    float *l_fcb = params.fcb + l * 4 * C;
    float *l_fcprojw = params.fcprojw + l * C * 4 * C;
    float *l_fcprojb = params.fcprojb + l * C;
    // get the pointers of the activations for this layer
    float *l_ln1 = acts.ln1 + l * B * T * C;
    float *l_ln1_mean = acts.ln1_mean + l * B * T;
    float *l_ln1_rstd = acts.ln1_rstd + l * B * T;
    float *l_qkv = acts.qkv + l * B * T * 3 * C;
    float *l_atty = acts.atty + l * B * T * C;
    float *l_preatt = acts.preatt + l * B * NH * T * T;
    float *l_att = acts.att + l * B * NH * T * T;
    float *l_attproj = acts.attproj + l * B * T * C;
    float *l_residual2 = acts.residual2 + l * B * T * C;
    float *l_ln2 = acts.ln2 + l * B * T * C;
    float *l_ln2_mean = acts.ln2_mean + l * B * T;
    float *l_ln2_rstd = acts.ln2_rstd + l * B * T;
    float *l_fch = acts.fch + l * B * T * 4 * C;
    float *l_fch_gelu = acts.fch_gelu + l * B * T * 4 * C;
    float *l_fcproj = acts.fcproj + l * B * T * C;
    float *l_residual3 = acts.residual3 + l * B * T * C;
    // now do the forward pass
    layernorm_forward(l_ln1, l_ln1_mean, l_ln1_rstd, residual, l_ln1w, l_ln1b,
                      B, T, C);
    matmul_forward(l_qkv, l_ln1, l_qkvw, l_qkvb, B, T, C, 3 * C);
    attention_forward(l_atty, l_preatt, l_att, l_qkv, B, T, C, NH);
    matmul_forward(l_attproj, l_atty, l_attprojw, l_attprojb, B, T, C, C);
    residual_forward(l_residual2, residual, l_attproj, B * T * C);
    layernorm_forward(l_ln2, l_ln2_mean, l_ln2_rstd, l_residual2, l_ln2w,
                      l_ln2b, B, T, C);
    matmul_forward(l_fch, l_ln2, l_fcw, l_fcb, B, T, C, 4 * C);
    gelu_forward(l_fch_gelu, l_fch, B * T * 4 * C);
    matmul_forward(l_fcproj, l_fch_gelu, l_fcprojw, l_fcprojb, B, T, 4 * C, C);
    residual_forward(l_residual3, l_residual2, l_fcproj, B * T * C);
  }
  residual =
      acts.residual3 + (L - 1) * B * T * C; // last residual is in residual3
  layernorm_forward(acts.lnf, acts.lnf_mean, acts.lnf_rstd, residual,
                    params.lnfw, params.lnfb, B, T, C);
  matmul_forward(acts.logits, acts.lnf, params.wte, NULL, B, T, C, V);
  softmax_forward(acts.probs, acts.logits, B, T, V);
}
void gpt2_zero_grad(GPT2 *model) {
  if (model->grads_memory != NULL) {
    memset(model->grads_memory, 0, model->num_parameters * sizeof(float));
  }
  if (model->grads_acts_memory != NULL) {
    memset(model->grads_acts_memory, 0, model->num_activations * sizeof(float));
  }
}
void gpt2_free(GPT2 *model) {
  // no need to free params_memory, it points to mmap memory
  if (model->mmap_data.data) {
    munmap(model->mmap_data.data, model->mmap_data.size);
  }
  free(model->grads_memory);
  free(model->m_memory);
  free(model->v_memory);
  free(model->acts_memory);
  free(model->grads_acts_memory);
  free(model->inputs);
  free(model->targets);
}
int sample_mult(float *probabilities, int n) {
  // sample index from probabilities (they must sum to 1!)
  // coin can be a random number in [0, 1), usually from random_f32()
  float cdf = 0.0f, coin = 0.5f;
  for (int i = 0; i < n; i++) {
    cdf += probabilities[i];
    if (coin < cdf) {
      return i;
    }
  }
  return n - 1; // in case of rounding errors
}

// the GPT-2 end-of-text token id
#define GPT2_EOT 50256

int main(int argc, char **argv) {
  GPT2 model;
  char *model_path = "./gpt2_124M.bin"; // default model path
  int n = 10;                           // default token limit

  // process command line arguments
  while (argc > 1 && argv[1][0] == '-') {
    if (strcmp(argv[1], "-m") == 0 && argc > 2) {
      model_path = argv[2];
      argc -= 2;
      argv += 2;
    } else if (strcmp(argv[1], "-n") == 0 && argc > 2) {
      n = atoi(argv[2]);
      if (n <= 0) {
        printf("Token limit must be positive.\n");
        exit(1);
      }
      argc -= 2;
      argv += 2;
    } else {
      printf("Usage: %s [-m model_path] [-n token_limit] token1 [token2 ...]\n",
             argv[0]);
      exit(1);
    }
  }
  argc--;
  argv++;

  // initialize the model using the model path
  gpt2_build_from_checkpoint(&model, model_path);

  if (argc == 0) {
    printf("Provide at least one token.\n");
    exit(1);
  }
  if (argc > n) {
    printf("Too many tokens.\n");
    exit(1);
  }

  int *tokens = (int *)malloc(n * sizeof(int)); // dynamically allocate memory
  if (!tokens) {
    printf("Failed to allocate memory for tokens.\n");
    exit(1);
  }

  for (int i = 0; i < n; i++) {
    if (i < argc) {
      tokens[i] = strtol(argv[i], NULL, 10);
    } else {
      tokens[i] = GPT2_EOT;
    }
  }

  for (int t = argc; t < n; t++) {
    gpt2_forward(&model, tokens, 1, t);
    float *probs = model.acts.probs + (t - 1) * model.config.vocab_size;
    int next_token = sample_mult(probs, model.config.vocab_size);
    tokens[t] = next_token;

    printf("%d\n", tokens[t]);
    fflush(stdout);

    if (tokens[t] == GPT2_EOT) {
      break;
    }
  }

  gpt2_free(&model);

  return 0;
}