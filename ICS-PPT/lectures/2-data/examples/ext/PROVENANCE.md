# 权重样本的来源与授权

本目录下两个 `.bf16` 文件是 Meta 的 Llama 3.2 1B Instruct 权重中的两小段，
共 6144 个数、12 KB，用于 `quant_compare.c` 在真实数据上比较几种量化格式。

| 文件 | 张量 | 元素数 | 分布 |
| --- | --- | --- | --- |
| `w-down-proj.bf16` | `model.layers.0.mlp.down_proj.weight` 的前 4096 个 | 4096 | 近似关于 0 对称，绝对值最大 0.0635 |
| `w-final-norm.bf16` | `model.norm.weight` 的前 2048 个 | 2048 | 全为正，范围 0.042 到 2.922，均值 2.35 |

两个文件都是裸的小端 BF16 数组，没有文件头。`../make_sample.py` 用两次
HTTP range 请求从 safetensors 文件里取出这两段，可以随时重新生成。

**来源**：<https://huggingface.co/meta-llama/Llama-3.2-1B-Instruct>，
文件 `model.safetensors`。取数时用的是同一份权重的非门控镜像
<https://huggingface.co/unsloth/Llama-3.2-1B-Instruct>（官方仓库需要先申请授权，
`make_sample.py` 因此不便直接访问它）。

**授权**：Llama 3.2 Community License Agreement。该协议允许再分发权重及其衍生物，
条件是随附协议副本、保留归属声明，且再分发方的月活用户不超过 7 亿。
本目录满足这些条件：

> Built with Llama.
>
> Llama 3.2 is licensed under the Llama 3.2 Community License,
> Copyright © Meta Platforms, Inc. All Rights Reserved.

协议全文见 <https://github.com/meta-llama/llama-models/blob/main/models/llama3_2/LICENSE>。

这两段数据不能还原出任何可用的模型，只作为课堂上的一组真实数值使用。
若课程发布时不希望随附任何权重，删掉这两个文件即可，`make_sample.py`
会在需要时重新取回。
