#!/usr/bin/env python3
"""Make the safetensors test files that tests/run.sh uses.

    python3 tools/mkfixtures.py tests/fixtures

Two files:
  tiny.safetensors     a four-layer model with the tensor names of Qwen3-VL,
                       plus a few model.visual. tensors, which nano-quant
                       leaves out as it does the vision tower of the real model
  tiny-be.safetensors  tiny with its header length written big endian, which
                       a reader must refuse
and tiny-meta/, a config.json and a minimal vocabulary for tiny.

The contents come from a fixed-seed generator: the same bytes on every machine.
"""
import json
import os
import struct
import sys


def bf16_bytes(vals):
    """Truncate floats to BF16, little-endian bytes."""
    out = bytearray()
    for v in vals:
        u = struct.unpack('<I', struct.pack('<f', v))[0]
        out += struct.pack('<H', u >> 16)
    return bytes(out)


class Lcg:
    """The generator of nq-selftest."""
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
    """[(name, shape)] in file order."""
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
    # a few vision tower tensors, which nano-quant leaves out
    t += [
        ("model.visual.patch_embed.proj.weight", [HIDDEN, HIDDEN]),
        ("model.visual.blocks.0.attn.qkv.weight", [HIDDEN, HIDDEN]),
        ("model.visual.merger.linear_fc2.weight", [HIDDEN, HIDDEN]),
    ]
    return t


def write_safetensors(path, entries, big_endian_len=False):
    """entries: [(name, shape, bytes)]."""
    header, off = {}, 0
    for name, shape, blob in entries:
        header[name] = {"dtype": "BF16", "shape": shape, "data_offsets": [off, off + len(blob)]}
        off += len(blob)
    js = json.dumps(header, separators=(',', ':')).encode()
    pad = (-len(js)) % 8                      # pad the header to a multiple of 8, as safetensors allows
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
            vals = [0.5 + r.uniform() for _ in range(n)]      # norm weights are all positive
        else:
            vals = [r.weight(0.05) for _ in range(n)]
        entries.append((name, shape, bf16_bytes(vals)))
    return write_safetensors(path, entries, big_endian_len=be)


def make_meta_dir(d):
    """A minimal config and vocabulary for tiny.safetensors, so the whole
    pipeline runs offline."""
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
                     ("tiny-be.safetensors", lambda p: make_tiny(p, be=True))):
        p = os.path.join(out, name)
        print("%-22s %8d bytes" % (name, fn(p)))
    make_meta_dir(os.path.join(out, "tiny-meta"))
    print("%-22s config.json and a minimal vocabulary" % "tiny-meta/")


if __name__ == "__main__":
    main()
