// Original Author: Andrej Karpathy
// https://github.com/karpathy/llm.c

#include "gpt2.h"
#include "task.h"
#include "thread-sync.h"
#include "timer_utils.h"
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

extern TaskQueue task_queue;
extern pthread_t worker_threads[NUM_WORKERS];

// ----------------------------------------------------------------------------
// all the individual layers' forward passes
// B = batch_size, T = sequence_length, C = channels, V = vocab_size

void encoder_forward(float *out, int *inp, float *wte, float *wpe, int B, int T, int C)
{
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

void layernorm_forward(float *out, float *mean, float *rstd, float *inp, float *weight, float *bias, int B, int T,
                       int C)
{
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

void matmul_forward(float *out, float *inp, float *weight, float *bias, int B, int T, int C, int OC)
{
    // reset the task queue state
    task_queue.stop = 0;
    task_queue.completed_tasks = 0;

    // calculate the total number of tasks
    int tasks_per_position = (OC + BLOCK_SIZE - 1) / BLOCK_SIZE;
    task_queue.total_tasks = B * T * tasks_per_position;

    // calculate the total number of tasks
    for (int b = 0; b < B; b++) {
        for (int t = 0; t < T; t++) {
            for (int block = 0; block < tasks_per_position; block++) {
                int start_o = block * BLOCK_SIZE;
                int end_o = start_o + BLOCK_SIZE;
                if (end_o > OC)
                    end_o = OC;

                MatMulTask task = {.out = out,
                                   .inp = inp,
                                   .weight = weight,
                                   .bias = bias,
                                   .b = b,
                                   .t = t,
                                   .start_o = start_o,
                                   .end_o = end_o,
                                   .B = B,
                                   .T = T,
                                   .C = C,
                                   .OC = OC,
                                   .valid = 1};
                enqueue_task(task);
            }
        }
    }

    // wait for all tasks to complete
    P(&task_queue.done);
}

void attention_forward(float *out, float *preatt, float *att, float *inp, int B, int T, int C, int NH)
{
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
                    float *key_t2 = inp + b * T * C3 + t2 * C3 + h * hs + C; // +C because it's key

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
                    float *value_t2 = inp + b * T * C3 + t2 * C3 + h * hs + C * 2; // +C*2 because it's value
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
void gelu_forward(float *out, float *inp, int N)
{
    // (approximate) GeLU elementwise non-linearity in the MLP block of
    // Transformer
    for (int i = 0; i < N; i++) {
        float x = inp[i];
        float cube = 0.044715f * x * x * x;
        out[i] = 0.5f * x * (1.0f + tanhf(GELU_SCALING_FACTOR * (x + cube)));
    }
}

void residual_forward(float *out, float *inp1, float *inp2, int N)
{
    for (int i = 0; i < N; i++) {
        out[i] = inp1[i] + inp2[i];
    }
}

void softmax_forward(float *probs, float *logits, int B, int T, int V)
{
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

float *malloc_and_point_parameters(ParameterTensors *params, size_t *param_sizes, float *params_base)
{
    size_t num_parameters = 0;
    for (size_t i = 0; i < NUM_PARAMETER_TENSORS; i++) {
        num_parameters += param_sizes[i];
    }

    // use the passed params_base instead of allocating new memory
    float *params_memory = params_base;

    // assign all the tensors
    float **ptrs[] = {&params->wte,      &params->wpe,      &params->ln1w, &params->ln1b, &params->qkvw, &params->qkvb,
                      &params->attprojw, &params->attprojb, &params->ln2w, &params->ln2b, &params->fcw,  &params->fcb,
                      &params->fcprojw,  &params->fcprojb,  &params->lnfw, &params->lnfb};
    float *params_memory_iterator = params_memory;
    for (size_t i = 0; i < NUM_PARAMETER_TENSORS; i++) {
        *(ptrs[i]) = params_memory_iterator;
        params_memory_iterator += param_sizes[i];
    }
    return params_memory;
}

float *malloc_and_point_activations(ActivationTensors *acts, size_t *act_sizes)
{
    size_t num_activations = 0;
    for (size_t i = 0; i < NUM_ACTIVATION_TENSORS; i++) {
        num_activations += act_sizes[i];
    }
    float *acts_memory = (float *)malloc(num_activations * sizeof(float));
    float **ptrs[] = {&acts->encoded, &acts->ln1,       &acts->ln1_mean, &acts->ln1_rstd, &acts->qkv,
                      &acts->atty,    &acts->preatt,    &acts->att,      &acts->attproj,  &acts->residual2,
                      &acts->ln2,     &acts->ln2_mean,  &acts->ln2_rstd, &acts->fch,      &acts->fch_gelu,
                      &acts->fcproj,  &acts->residual3, &acts->lnf,      &acts->lnf_mean, &acts->lnf_rstd,
                      &acts->logits,  &acts->probs,     &acts->losses};
    float *acts_memory_iterator = acts_memory;
    for (size_t i = 0; i < NUM_ACTIVATION_TENSORS; i++) {
        *(ptrs[i]) = acts_memory_iterator;
        acts_memory_iterator += act_sizes[i];
    }
    return acts_memory;
}

void gpt2_build_from_checkpoint(GPT2 *model, char *checkpoint_path)
{
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

    // save mmap data for later cleanup
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
    model->params_memory = malloc_and_point_parameters(&model->params, model->param_sizes, params_base);

    // close the file descriptor (mmap memory is still valid)
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

void gpt2_forward(GPT2 *model, int *inputs, int B, int T)
{
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
    model->acts_memory = malloc_and_point_activations(&model->acts, model->act_sizes);

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
        layernorm_forward(l_ln1, l_ln1_mean, l_ln1_rstd, residual, l_ln1w, l_ln1b, B, T, C);
        matmul_forward(l_qkv, l_ln1, l_qkvw, l_qkvb, B, T, C, 3 * C);
        attention_forward(l_atty, l_preatt, l_att, l_qkv, B, T, C, NH);
        matmul_forward(l_attproj, l_atty, l_attprojw, l_attprojb, B, T, C, C);
        residual_forward(l_residual2, residual, l_attproj, B * T * C);
        layernorm_forward(l_ln2, l_ln2_mean, l_ln2_rstd, l_residual2, l_ln2w, l_ln2b, B, T, C);
        matmul_forward(l_fch, l_ln2, l_fcw, l_fcb, B, T, C, 4 * C);
        gelu_forward(l_fch_gelu, l_fch, B * T * 4 * C);
        matmul_forward(l_fcproj, l_fch_gelu, l_fcprojw, l_fcprojb, B, T, 4 * C, C);
        residual_forward(l_residual3, l_residual2, l_fcproj, B * T * C);
    }
    residual = acts.residual3 + (L - 1) * B * T * C; // last residual is in residual3
    layernorm_forward(acts.lnf, acts.lnf_mean, acts.lnf_rstd, residual, params.lnfw, params.lnfb, B, T, C);
    matmul_forward(acts.logits, acts.lnf, params.wte, NULL, B, T, C, V);
    softmax_forward(acts.probs, acts.logits, B, T, V);
}

void gpt2_free(GPT2 *model)
{
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

int sample_mult(float *probabilities, int n)
{
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

int main(int argc, char **argv)
{
    GPT2 model;
    char *model_path = "./gpt2_124M.bin"; // default model path
    int n = 10;                           // default token limit
    int timer_show = 0;                   // default not show timer
    int use_kv_cache = 0;                 // default not use KV cache

    // process command line arguments
    while (argc > 1 && argv[1][0] == '-') {
        if (strcmp(argv[1], "-m") == 0 && argc > 2) {
            model_path = argv[2];
            argc -= 2;
            argv += 2;
        } else if (strcmp(argv[1], "-t") == 0 && argc > 2) {
            timer_show = atoi(argv[2]);
            argc -= 1;
            argv += 1;
        } else if (strcmp(argv[1], "-n") == 0 && argc > 2) {
            n = atoi(argv[2]);
            if (n <= 0) {
                printf("Token limit must be positive.\n");
                exit(1);
            }
            argc -= 2;
            argv += 2;
        } else if (strcmp(argv[1], "-kvcache") == 0) {
            use_kv_cache = 1; // enable KV cache
            argc -= 1;
            argv += 1;
        } else {
            printf("Usage: %s [-m model_path] [-n token_limit] [-kvcache] token1 [token2 ...]\n", argv[0]);
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

    int *tokens = (int *)malloc(n * sizeof(int));
    if (!tokens) {
        printf("Failed to allocate memory for tokens.\n");
        exit(1);
    }

    init_task_queue();
    // if use kv cache, initialize the kv cache
    if (use_kv_cache) {
        init_kv_cache(&model);
    }

    for (int i = 0; i < argc; i++) {
        tokens[i] = strtol(argv[i], NULL, 10);
    }

    if (use_kv_cache) {
        // use kv cache: forward the input tokens once
        double token_start = get_time();
        gpt2_forward_with_kvcache(&model, tokens, 1, argc);
        double token_time;

        // add: sample the next token from the probability distribution of the last input token
        if (argc < n) { // ensure not exceed the token limit
            float *probs = model.acts.probs + (argc - 1) * model.config.vocab_size;
            int next_token = sample_mult(probs, model.config.vocab_size);
            tokens[argc] = next_token;
            token_time = get_time() - token_start;

            if (timer_show) {
                printf("%d (%.2fms)\n", tokens[argc], token_time * 1000);
                fflush(stdout);
            } else {
                printf("%d\n", tokens[argc]);
                fflush(stdout);
            }

            // if generated EOT, end directly
            if (tokens[argc] == GPT2_EOT) {
                n = argc + 1; // end early
            }
        }
    }

    double start_time = get_time();

    int generated_count = use_kv_cache ? 1 : 0; // If using KV cache, one token is already generated

    int start_pos = argc + (use_kv_cache ? 1 : 0);

    for (int t = start_pos; t < n; t++) {
        double token_start = get_time();

        if (use_kv_cache) {
            // incremental generation: only process the latest token
            // copy the full token sequence instead of only the new token
            int *full_tokens = (int *)malloc(t * sizeof(int));
            if (full_tokens == NULL) {
                printf("Failed to allocate memory for full tokens.\n");
                exit(1);
            }
            memcpy(full_tokens, tokens, t * sizeof(int));

            // forward the full token sequence instead of only the new token
            gpt2_forward_with_kvcache(&model, tokens, 1, t);

            // sample the next token from the probability distribution of the last token
            // ensure t > 0 to avoid out-of-bounds access
            float *probs = model.acts.probs + (t - 1) * model.config.vocab_size;
            int next_token = sample_mult(probs, model.config.vocab_size);
            tokens[t] = next_token;

            free(full_tokens);
        } else {
            // not use kv cache: process the full sequence each time
            gpt2_forward(&model, tokens, 1, t);
            float *probs = model.acts.probs + (t - 1) * model.config.vocab_size;
            int next_token = sample_mult(probs, model.config.vocab_size);
            tokens[t] = next_token;
        }

        double token_time = get_time() - token_start;
        if (timer_show) {
            printf("%d (%.2fms)\n", tokens[t], token_time * 1000);
            fflush(stdout);
        } else {
            printf("%d\n", tokens[t]);
            fflush(stdout);
        }

        generated_count++;

        if (tokens[t] == GPT2_EOT) {
            break;
        }
    }

    double total_time = get_time() - start_time;
    if (generated_count > 0 && timer_show) {
        printf("\nGenerated %d tokens, total time %.2f seconds, average %.2f ms per token\n", generated_count,
               total_time, (total_time * 1000) / generated_count);
    }

    if (timer_show) {
        print_timers();
    }

    free(tokens);

    gpt2_free(&model);
    clear_kv_cache(&model);

    stop_task_queue();

    return 0;
}