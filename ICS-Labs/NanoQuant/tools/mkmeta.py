#!/usr/bin/env python3
"""Make the GGUF key-value section from the config.json and tokenizer.json
of a Hugging Face repository.

    python3 tools/mkmeta.py <model dir> -o meta.kv

meta.kv is:
    'NQKV' | uint32 key-value pairs | uint64 bytes | the encoded pairs
nq2gguf copies the last part into the GGUF as is and adds two keys about the
quantization, general.file_type and general.quantization_version.

Required: config.json and tokenizer.json.
Optional: tokenizer_config.json, generation_config.json, chat_template.json;
when present, the special tokens and the chat template are read from them,
otherwise defaults are used.
"""
import argparse
import json
import os
import struct
import sys

# GGUF value type numbers
U8, I8, U16, I16, U32, I32, F32, BOOL, STR, ARR, U64, I64, F64 = range(13)


class KV:
    def __init__(self):
        self.buf = bytearray()
        self.n = 0

    def _str(self, s):
        b = s.encode('utf-8')
        return struct.pack('<Q', len(b)) + b

    def _key(self, k):
        self.buf += self._str(k)
        self.n += 1

    def u32(self, k, v):   self._key(k); self.buf += struct.pack('<II', U32, v)
    def i32(self, k, v):   self._key(k); self.buf += struct.pack('<Ii', I32, v)
    def f32(self, k, v):   self._key(k); self.buf += struct.pack('<If', F32, v)
    def boolean(self, k, v): self._key(k); self.buf += struct.pack('<IB', BOOL, 1 if v else 0)
    def string(self, k, v): self._key(k); self.buf += struct.pack('<I', STR) + self._str(v)

    def arr_i32(self, k, vals):
        self._key(k)
        self.buf += struct.pack('<IIQ', ARR, I32, len(vals))
        self.buf += struct.pack('<%di' % len(vals), *vals)

    def arr_u32(self, k, vals):
        self._key(k)
        self.buf += struct.pack('<IIQ', ARR, U32, len(vals))
        self.buf += struct.pack('<%dI' % len(vals), *vals)

    def arr_str(self, k, vals):
        self._key(k)
        self.buf += struct.pack('<IIQ', ARR, STR, len(vals))
        out = bytearray()
        for s in vals:
            b = s.encode('utf-8')
            out += struct.pack('<Q', len(b)) + b
        self.buf += out

    def dump(self, path):
        with open(path, 'wb') as f:
            f.write(b'NQKV')
            f.write(struct.pack('<I', self.n))
            f.write(struct.pack('<Q', len(self.buf)))
            f.write(self.buf)
        return self.n, len(self.buf)


def load(d, name, required=True):
    p = os.path.join(d, name)
    if not os.path.exists(p):
        if required:
            sys.exit("mkmeta: %s is missing" % p)
        return None
    with open(p, encoding='utf-8') as f:
        return json.load(f)


def build(d, arch, name):
    cfg = load(d, 'config.json')
    tok = load(d, 'tokenizer.json')
    tcfg = load(d, 'tokenizer_config.json', required=False) or {}
    gcfg = load(d, 'generation_config.json', required=False) or {}
    ctpl = load(d, 'chat_template.json', required=False) or {}

    text = cfg.get('text_config', cfg)
    vis = cfg.get('vision_config', {})

    kv = KV()
    kv.string('general.architecture', arch)
    kv.string('general.type', 'model')
    kv.string('general.name', name)
    kv.u32('general.alignment', 32)

    n_layer = int(text['num_hidden_layers'])
    n_head = int(text['num_attention_heads'])
    n_head_kv = int(text.get('num_key_value_heads', n_head))
    head_dim = int(text.get('head_dim', text['hidden_size'] // n_head))

    kv.u32('%s.block_count' % arch, n_layer)
    kv.u32('%s.context_length' % arch, int(text['max_position_embeddings']))
    kv.u32('%s.embedding_length' % arch, int(text['hidden_size']))
    kv.u32('%s.feed_forward_length' % arch, int(text['intermediate_size']))
    kv.u32('%s.attention.head_count' % arch, n_head)
    kv.u32('%s.attention.head_count_kv' % arch, n_head_kv)
    kv.u32('%s.attention.key_length' % arch, head_dim)
    kv.u32('%s.attention.value_length' % arch, head_dim)
    kv.f32('%s.attention.layer_norm_rms_epsilon' % arch, float(text['rms_norm_eps']))
    kv.f32('%s.rope.freq_base' % arch, float(text.get('rope_theta', 10000.0)))

    # multimodal RoPE splits the rotary dimensions into sections; pad with 0 to four
    sec = list((text.get('rope_scaling') or {}).get('mrope_section') or [])
    if sec:
        while len(sec) < 4:
            sec.append(0)
        kv.arr_i32('%s.rope.dimension_sections' % arch, [int(v) for v in sec[:4]])

    ds = vis.get('deepstack_visual_indexes')
    if ds:
        kv.u32('%s.n_deepstack_layers' % arch, len(ds))

    # ---- vocabulary ----
    vocab = tok['model']['vocab']
    n_vocab = int(text['vocab_size'])
    tokens = [None] * max(n_vocab, len(vocab))
    types = [1] * len(tokens)                       # 1 = NORMAL
    for s, i in vocab.items():
        tokens[i] = s
    for a in tok.get('added_tokens', []):
        i = int(a['id'])
        while i >= len(tokens):
            tokens.append(None)
            types.append(1)
        tokens[i] = a['content']
        types[i] = 3 if a.get('special') else 4     # 3 = CONTROL, 4 = USER_DEFINED
    for i, t in enumerate(tokens):
        if t is None:                               # the vocabulary is shorter than the embeddings: fill the rest
            tokens[i] = '[PAD%d]' % i
            types[i] = 5                            # 5 = UNUSED
    if len(tokens) != n_vocab:
        print('mkmeta: vocabulary has %d entries, config says %d' % (len(tokens), n_vocab), file=sys.stderr)

    merges = tok['model']['merges']
    if merges and not isinstance(merges[0], str):
        merges = ['%s %s' % (a, b) for a, b in merges]

    kv.string('tokenizer.ggml.model', 'gpt2')
    kv.string('tokenizer.ggml.pre', 'qwen2')
    kv.arr_str('tokenizer.ggml.tokens', tokens)
    kv.arr_i32('tokenizer.ggml.token_type', types)
    kv.arr_str('tokenizer.ggml.merges', merges)

    def tid(*names):
        for n in names:
            v = gcfg.get(n, tcfg.get(n))
            if isinstance(v, list):
                v = v[0]
            if isinstance(v, int):
                return v
        return None

    eos = tid('eos_token_id')
    pad = tid('pad_token_id')
    if eos is None:
        for i, t in enumerate(tokens):
            if t == '<|im_end|>':
                eos = i
    if eos is not None:
        kv.u32('tokenizer.ggml.eos_token_id', eos)
    if pad is not None:
        kv.u32('tokenizer.ggml.padding_token_id', pad)
    kv.boolean('tokenizer.ggml.add_bos_token', False)
    kv.boolean('tokenizer.ggml.add_eos_token', False)

    tpl = ctpl.get('chat_template') or tcfg.get('chat_template')
    if isinstance(tpl, list):
        tpl = tpl[0].get('template')
    if tpl:
        kv.string('tokenizer.chat_template', tpl)

    return kv


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('dir')
    ap.add_argument('-o', required=True)
    ap.add_argument('--arch', default='qwen3vl')
    ap.add_argument('--name', default='Qwen3-VL-2B-Instruct')
    a = ap.parse_args()
    kv = build(a.dir, a.arch, a.name)
    n, nb = kv.dump(a.o)
    print('%s: %d key-value pairs, %d bytes' % (a.o, n, nb))


if __name__ == '__main__':
    main()
