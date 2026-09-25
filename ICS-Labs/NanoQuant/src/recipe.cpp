/* Which tensors go into the output, the type of each under a recipe, and its
   GGUF name. README.md states the rules. */
#include <stdio.h>
#include <string.h>

#include "nq.h"

namespace {

const char LM_PREFIX[] = "model.language_model.";

bool ends_with(const char *s, const char *suf) {
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && strcmp(s + a - b, suf) == 0;
}

/* llama.cpp's use_more_bits: whether attn_v and ffn_down of layer i, in a
   model of n layers, go up to Q6_K under Q4_K_M. */
bool use_more_bits(int i, int n) {
    return i < n / 8 || i >= 7 * n / 8 || (i - n / 8) % 3 == 2;
}

/* The two tensors outside the layers. */
const struct { const char *hf; const char *gguf; } FLAT[] = {
    { "model.language_model.embed_tokens.weight", "token_embd.weight"  },
    { "model.language_model.norm.weight",         "output_norm.weight" },
};

/* Tensors inside layer L: model.language_model.layers.L.<hf> is blk.L.<gguf>.weight. */
const struct { const char *hf; const char *gguf; } PER_LAYER[] = {
    { "input_layernorm.weight",          "attn_norm"   },
    { "self_attn.q_proj.weight",         "attn_q"      },
    { "self_attn.k_proj.weight",         "attn_k"      },
    { "self_attn.v_proj.weight",         "attn_v"      },
    { "self_attn.o_proj.weight",         "attn_output" },
    { "self_attn.q_norm.weight",         "attn_q_norm" },
    { "self_attn.k_norm.weight",         "attn_k_norm" },
    { "post_attention_layernorm.weight", "ffn_norm"    },
    { "mlp.gate_proj.weight",            "ffn_gate"    },
    { "mlp.up_proj.weight",              "ffn_up"      },
    { "mlp.down_proj.weight",            "ffn_down"    },
};

/* The part of a layer tensor's name after "layers.L.", or nullptr. */
const char *layer_suffix(const char *name) {
    const char *p = strstr(name, ".layers.");
    if (!p) return nullptr;
    p += 8;
    if (*p < '0' || *p > '9') return nullptr;
    while (*p >= '0' && *p <= '9') p++;
    return *p == '.' ? p + 1 : nullptr;
}

} /* namespace */

bool nq_tensor_selected(const char *name) {
    return strncmp(name, LM_PREFIX, sizeof LM_PREFIX - 1) == 0;
}

int nq_layer_of(const char *name) {
    const char *p = strstr(name, ".layers.");
    if (!p) return -1;
    p += 8;
    if (*p < '0' || *p > '9') return -1;
    int n = 0;
    while (*p >= '0' && *p <= '9') n = n * 10 + (*p++ - '0');
    return n;
}

int nq_gguf_name(const char *hf_name, char *buf, size_t buf_size) {
    for (const auto &e : FLAT) {
        if (strcmp(hf_name, e.hf) != 0) continue;
        if (strlen(e.gguf) + 1 > buf_size) return -1;
        strcpy(buf, e.gguf);
        return 0;
    }
    if (!nq_tensor_selected(hf_name)) return -1;
    const char *suf = layer_suffix(hf_name);
    if (!suf) return -1;
    for (const auto &e : PER_LAYER) {
        if (strcmp(suf, e.hf) != 0) continue;
        int n = snprintf(buf, buf_size, "blk.%d.%s.weight", nq_layer_of(hf_name), e.gguf);
        return n > 0 && (size_t)n < buf_size ? 0 : -1;
    }
    return -1;
}

nq_type nq_recipe_pick(nq_recipe r, const char *name, int n_dims, int layer, int n_layers) {
    if (n_dims == 1) return NQ_TYPE_F32;          /* every norm stays F32 */
    switch (r) {
        case NQ_RECIPE_Q4_0: return NQ_TYPE_Q4_0;
        case NQ_RECIPE_Q4_1: return NQ_TYPE_Q4_1;
        case NQ_RECIPE_Q4_K: return NQ_TYPE_Q4_K;
        case NQ_RECIPE_Q4_K_M:
            if (ends_with(name, "embed_tokens.weight")) return NQ_TYPE_Q6_K;
            if (layer >= 0 && use_more_bits(layer, n_layers) &&
                (ends_with(name, "self_attn.v_proj.weight") ||
                 ends_with(name, "mlp.down_proj.weight"))) return NQ_TYPE_Q6_K;
            return NQ_TYPE_Q4_K;
    }
    return NQ_TYPE_Q4_K;
}

const char *nq_recipe_name(nq_recipe r) {
    switch (r) {
        case NQ_RECIPE_Q4_0:   return "q4_0";
        case NQ_RECIPE_Q4_1:   return "q4_1";
        case NQ_RECIPE_Q4_K:   return "q4_k";
        case NQ_RECIPE_Q4_K_M: return "q4_k_m";
    }
    return "?";
}

int nq_recipe_parse(const char *s, nq_recipe *out) {
    if (!strcmp(s, "q4_0"))   { *out = NQ_RECIPE_Q4_0;   return 0; }
    if (!strcmp(s, "q4_1"))   { *out = NQ_RECIPE_Q4_1;   return 0; }
    if (!strcmp(s, "q4_k"))   { *out = NQ_RECIPE_Q4_K;   return 0; }
    if (!strcmp(s, "q4_k_m")) { *out = NQ_RECIPE_Q4_K_M; return 0; }
    return -1;
}
