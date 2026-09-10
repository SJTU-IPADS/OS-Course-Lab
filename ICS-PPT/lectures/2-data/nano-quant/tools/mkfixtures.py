#!/usr/bin/env python3
"""生成判定用的 safetensors 样例。

    python3 tools/mkfixtures.py tests/fixtures

三个文件：
  tiny.safetensors     一个四层的小模型，张量名与 Qwen3-VL 完全一致，
                       另外放了几个 model.visual. 的张量用来试过滤。
  big.safetensors      稀疏文件，第二个张量的偏移超过 4 GiB，
                       用来试 64 位偏移。占的磁盘只有几百字节。
  tiny-be.safetensors  头长度按大端写，读的时候必须被拒。

内容是定死的伪随机数，换台机器生成的字节完全一样。
"""
import json
import os
import struct
import sys


def bf16_bytes(vals):
    """把一串 float 截断成 BF16 的小端字节。"""
    out = bytearray()
    for v in vals:
        u = struct.unpack('<I', struct.pack('<f', v))[0]
        out += struct.pack('<H', u >> 16)
    return bytes(out)


class Lcg:
    """与 nq-selftest 里同一个发生器。"""
    def __init__(self, seed):
        self.s = seed & 0xffffffffffffffff

    def next32(self):
        self.s = (self.s * 6364136223846793005 + 1442695040888963407) & 0xffffffffffffffff
        return self.s >> 32

    def uniform(self):
        return (self.next32() >> 8) / float(1 << 24)

    def weight(self, scale):
        return scale * (self.uniform() * 2.0 - 1.0)


HIDDEN, INTER, VOCAB, LAYERS = 256, 512, 1024, 4
HEAD_DIM, HEADS, KV_HEADS = 64, 4, 2


def tiny_tensors():
    """[(名字, 形状)]，顺序就是文件里的顺序。"""
    t = [("model.language_model.embed_tokens.weight", [VOCAB, HIDDEN])]
    for i in range(LAYERS):
        p = "model.language_model.layers.%d." % i
        t += [
            (p + "input_layernorm.weight",          [HIDDEN]),
            (p + "self_attn.q_proj.weight",         [HEADS * HEAD_DIM, HIDDEN]),
            (p + "self_attn.k_proj.weight",         [KV_HEADS * HEAD_DIM, HIDDEN]),
            (p + "self_attn.v_proj.weight",         [KV_HEADS * HEAD_DIM, HIDDEN]),
            (p + "self_attn.o_proj.weight",         [HIDDEN, HEADS * HEAD_DIM]),
            (p + "self_attn.q_norm.weight",         [HEAD_DIM]),
            (p + "self_attn.k_norm.weight",         [HEAD_DIM]),
            (p + "post_attention_layernorm.weight", [HIDDEN]),
            (p + "mlp.gate_proj.weight",            [INTER, HIDDEN]),
            (p + "mlp.up_proj.weight",              [INTER, HIDDEN]),
            (p + "mlp.down_proj.weight",            [HIDDEN, INTER]),
        ]
    t += [("model.language_model.norm.weight", [HIDDEN])]
    # 视觉塔的几个张量，只用来试 tensor_selected
    t += [
        ("model.visual.patch_embed.proj.weight", [HIDDEN, HIDDEN]),
        ("model.visual.blocks.0.attn.qkv.weight", [HIDDEN, HIDDEN]),
        ("model.visual.merger.linear_fc2.weight", [HIDDEN, HIDDEN]),
    ]
    return t


def write_safetensors(path, entries, big_endian_len=False):
    """entries: [(名字, 形状, 字节)]。"""
    header, off = {}, 0
    for name, shape, blob in entries:
        header[name] = {"dtype": "BF16", "shape": shape, "data_offsets": [off, off + len(blob)]}
        off += len(blob)
    js = json.dumps(header, separators=(',', ':')).encode()
    pad = (-len(js)) % 8                      # 头补到 8 的倍数，safetensors 允许
    js += b' ' * pad
    fmt = '>Q' if big_endian_len else '<Q'
    with open(path, 'wb') as f:
        f.write(struct.pack(fmt, len(js)))
        f.write(js)
        for _, _, blob in entries:
            f.write(blob)
    return os.path.getsize(path)


def make_tiny(path, be=False):
    r = Lcg(20260907)
    entries = []
    for name, shape in tiny_tensors():
        n = 1
        for d in shape:
            n *= d
        if name.endswith("norm.weight") and len(shape) == 1:
            vals = [0.5 + r.uniform() for _ in range(n)]      # 归一化系数全正
        else:
            vals = [r.weight(0.05) for _ in range(n)]
        entries.append((name, shape, bf16_bytes(vals)))
    return write_safetensors(path, entries, big_endian_len=be)


def make_big(path):
    """两个张量，第二个落在 4 GiB 之后。文件是稀疏的。"""
    r = Lcg(2026)
    a = bf16_bytes([r.weight(0.1) for _ in range(256)])
    b = bf16_bytes([r.weight(0.1) for _ in range(256)])
    gap = 4 * 1024 * 1024 * 1024 + 1024          # 第二个张量的起点
    header = {
        "model.language_model.embed_tokens.weight":
            {"dtype": "BF16", "shape": [1, 256], "data_offsets": [0, 512]},
        "model.language_model.norm.weight":
            {"dtype": "BF16", "shape": [256], "data_offsets": [gap, gap + 512]},
    }
    js = json.dumps(header, separators=(',', ':')).encode()
    js += b' ' * ((-len(js)) % 8)
    with open(path, 'wb') as f:
        f.write(struct.pack('<Q', len(js)))
        f.write(js)
        f.write(a)
        f.seek(8 + len(js) + gap)
        f.write(b)
    return os.path.getsize(path)


def make_meta_dir(d):
    """给 tiny.safetensors 配一套最小的 config 与词表，好让整条链路能离线跑通。"""
    os.makedirs(d, exist_ok=True)
    cfg = {
        "architectures": ["Qwen3VLForConditionalGeneration"],
        "model_type": "qwen3_vl",
        "tie_word_embeddings": True,
        "text_config": {
            "hidden_size": HIDDEN, "intermediate_size": INTER,
            "num_hidden_layers": LAYERS, "num_attention_heads": HEADS,
            "num_key_value_heads": KV_HEADS, "head_dim": HEAD_DIM,
            "max_position_embeddings": 4096, "rms_norm_eps": 1e-6,
            "rope_theta": 5000000.0, "vocab_size": VOCAB,
            "rope_scaling": {"mrope_section": [16, 8, 8], "rope_type": "default"},
        },
        "vision_config": {"deepstack_visual_indexes": [1, 2, 3]},
    }
    with open(os.path.join(d, "config.json"), "w") as f:
        json.dump(cfg, f, indent=1)

    vocab = {}
    for i in range(VOCAB - 3):
        vocab["t%d" % i] = i
    tokens = {
        "version": "1.0",
        "added_tokens": [
            {"id": VOCAB - 3, "content": "<|endoftext|>", "special": True},
            {"id": VOCAB - 2, "content": "<|im_start|>", "special": True},
            {"id": VOCAB - 1, "content": "<|im_end|>", "special": True},
        ],
        "model": {"type": "BPE", "vocab": vocab, "merges": ["t0 t1", "t2 t3"]},
    }
    with open(os.path.join(d, "tokenizer.json"), "w") as f:
        json.dump(tokens, f)
    with open(os.path.join(d, "tokenizer_config.json"), "w") as f:
        json.dump({"eos_token_id": VOCAB - 1, "chat_template": "{{ messages }}"}, f)
    return d


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "tests/fixtures"
    os.makedirs(out, exist_ok=True)
    for name, fn in (("tiny.safetensors", lambda p: make_tiny(p)),
                     ("tiny-be.safetensors", lambda p: make_tiny(p, be=True)),
                     ("big.safetensors", make_big)):
        p = os.path.join(out, name)
        size = fn(p)
        used = os.stat(p).st_blocks * 512
        print("%-22s %12d 字节，占盘 %d 字节" % (name, size, used))
    make_meta_dir(os.path.join(out, "tiny-meta"))
    print("%-22s config.json 与最小词表" % "tiny-meta/")


if __name__ == "__main__":
    main()
