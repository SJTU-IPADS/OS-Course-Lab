/* 配方判据与名字映射表。两张表都是数据，规则写在 README 里。 */
#include <string.h>

#include "nq.h"

/* llama.cpp 的 Q4_K_M：第 i 层（共 n 层）的 attn_v 与 ffn_down 是否升到 Q6_K。
   源出 llama.cpp 的 use_more_bits。 */
bool nq_use_more_bits(int i, int n) {
    return i < n / 8 || i >= 7 * n / 8 || (i - n / 8) % 3 == 2;
}

const nq_name_rule NQ_NAME_RULES[] = {
    /* 不属于某一层的两个张量 */
    { "model.language_model.embed_tokens.weight",                     "token_embd.weight"           },
    { "model.language_model.norm.weight",                             "output_norm.weight"          },
    /* 层内张量，%d 是层号 */
    { "model.language_model.layers.%d.input_layernorm.weight",        "blk.%d.attn_norm.weight"     },
    { "model.language_model.layers.%d.self_attn.q_proj.weight",       "blk.%d.attn_q.weight"        },
    { "model.language_model.layers.%d.self_attn.k_proj.weight",       "blk.%d.attn_k.weight"        },
    { "model.language_model.layers.%d.self_attn.v_proj.weight",       "blk.%d.attn_v.weight"        },
    { "model.language_model.layers.%d.self_attn.o_proj.weight",       "blk.%d.attn_output.weight"   },
    { "model.language_model.layers.%d.self_attn.q_norm.weight",       "blk.%d.attn_q_norm.weight"   },
    { "model.language_model.layers.%d.self_attn.k_norm.weight",       "blk.%d.attn_k_norm.weight"   },
    { "model.language_model.layers.%d.post_attention_layernorm.weight","blk.%d.ffn_norm.weight"     },
    { "model.language_model.layers.%d.mlp.gate_proj.weight",          "blk.%d.ffn_gate.weight"      },
    { "model.language_model.layers.%d.mlp.up_proj.weight",            "blk.%d.ffn_up.weight"        },
    { "model.language_model.layers.%d.mlp.down_proj.weight",          "blk.%d.ffn_down.weight"      },
};
const int NQ_NAME_RULE_COUNT = (int)(sizeof(NQ_NAME_RULES) / sizeof(NQ_NAME_RULES[0]));

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
