#!/usr/bin/env python3
"""Timing the two halves of one inference request.

Serving a prompt happens in two phases with different shapes.  Prefill runs the
whole prompt through the network at once, so every weight that is read is used
for many tokens.  Decode produces one token at a time, and each token reads the
same weights again for a single use.  The rates below are what that difference
costs, measured on whatever model ollama is already serving.  Each run uses
a fresh prompt so the server's prefix cache does not hide the prefill work.

    python3 llm_stages.py [model]

Needs `ollama serve` running.  The model from the second lecture's lab is the
default, since its weights are the ones that were taken apart there.
"""

import json
import sys
import time
import urllib.request

HOST = "http://localhost:11434"
DEFAULT = "nq-qwen3vl-2b"


def generate(model, prompt, predict):
    body = json.dumps({"model": model, "prompt": prompt, "stream": False,
                       "options": {"num_predict": predict}}).encode()
    req = urllib.request.Request(HOST + "/api/generate", body,
                                 {"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=600) as r:
        return json.load(r)


def size_bytes(model):
    body = json.dumps({"model": model}).encode()
    req = urllib.request.Request(HOST + "/api/show", body,
                                 {"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            return json.load(r)["details"].get("parameter_size", "?")
    except Exception:
        return "?"


def main():
    model = sys.argv[1] if len(sys.argv) > 1 else DEFAULT
    filler = "The processor overlaps the execution of several instructions. "
    # A nonce keeps the prompt out of the server's prefix cache, so prefill is
    # measured rather than skipped.
    prompt = ("Note %d. " % time.time_ns() + filler * 40
              + "\nSummarize the paragraph above in one sentence.")

    generate(model, "hi", 1)                       # load the weights first
    d = generate(model, prompt, 64)

    pn, pt = d["prompt_eval_count"], d["prompt_eval_duration"] / 1e9
    en, et = d["eval_count"], d["eval_duration"] / 1e9
    print("模型 %s，%s 参数" % (model.split(":")[0], size_bytes(model)))
    print("阶段      token 数    用时 ms   token/s")
    print("prefill   %7d   %8.1f   %8.1f" % (pn, pt * 1e3, pn / pt))
    print("decode    %7d   %8.1f   %8.1f" % (en, et * 1e3, en / et))
    print("\nprefill 的速率是 decode 的 %.0f 倍；一次 decode 平均 %.1f ms"
          % ((pn / pt) / (en / et), et / en * 1e3))


if __name__ == "__main__":
    main()
