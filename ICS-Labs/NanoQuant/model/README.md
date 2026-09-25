# model/

本目录存放实验用的模型文件。模型不随实验框架发布，需要自行下载后放入本目录。
`nano-quant/README.md` 中的命令都按下面的路径书写，文件名与位置保持不变。

## 需要的三个文件

| 文件 | 字节数 | 用途 |
| --- | --- | --- |
| `model.safetensors` | 4 255 140 312 | 权重，625 个张量，全部 BF16。`nano-quant plan` 与 `quant` 的输入 |
| `config.json` | 1 505 | 模型结构参数。`tools/mkmeta.py` 的输入 |
| `tokenizer.json` | 7 032 403 | 词表与合并规则。`tools/mkmeta.py` 的输入 |

三个文件均取自 Hugging Face 仓库 `Qwen/Qwen3-VL-2B-Instruct` 的 `main` 分支，
许可为 Apache-2.0，无需申请即可下载。放好之后目录如下：

```
nano-quant/model/
├── README.md
├── SHA256SUMS
├── config.json
├── model.safetensors
└── tokenizer.json
```

`config.json` 与 `tokenizer.json` 是生成 GGUF 元数据所必需的两个文件。
仓库中的 `tokenizer_config.json`、`generation_config.json`、`chat_template.json`
为可选文件：`mkmeta.py` 在它们存在时读取对话模板与特殊词元，缺少时使用默认值。
`tests/expected-qwen3vl.txt` 中 GGUF 的字节数按缺少这三个文件的情形给出，
因此本目录中只放上面三个文件。

## 下载

在 `nano-quant/` 下执行：

```bash
sh tools/fetch-model.sh
```

脚本把 `SHA256SUMS` 中列出的三个文件逐个下载到本目录，每个文件下载之后校验 sha256，
一个文件输出一行 `文件名: OK` 或 `文件名: FAILED`，退出码为失败的文件数。

- **下载源**：默认是 Hugging Face 的镜像 `https://hf-mirror.com`。
  环境变量 `HF_ENDPOINT` 有值时改用它的值；从 Hugging Face 本站下载时写作

  ```bash
  HF_ENDPOINT=https://huggingface.co sh tools/fetch-model.sh
  ```

- **中断之后**：已下载的部分保存在 `文件名.part` 中，再次运行同一条命令即从断点继续。
- **已有的文件**：本目录中已有、摘要正确的文件不再下载。
  因此从校内文件服务或其他途径取得这三个文件之后，放入本目录，运行一次脚本即完成校验。
- **需要的工具**：`curl`，以及 `sha256sum`（Linux、WSL2、Windows 的 Git Bash）或 `shasum`（macOS）。

课程在校内文件服务上也提供这三个文件。模型文件约 4.3 GB。`q4_0` 与 `q4_k_m` 两个配方的
`.nq` 与 `.gguf` 合计约 4.2 GB，同时保留两个配方的结果时准备 10 GB 的磁盘空间。

## 校验

三个文件的摘要写在 `SHA256SUMS` 中，`fetch-model.sh` 每次运行都按它校验。
不使用脚本时，也可以直接校验：

```bash
cd nano-quant/model
sha256sum -c SHA256SUMS          # macOS: shasum -a 256 -c SHA256SUMS
```

三行都输出 `OK` 之后再开始实验。`model.safetensors` 的摘要与 Hugging Face 上
该文件的 LFS 记录一致；另外两个文件的摘要于 2026-09-13 从同一仓库下载后计算。
文件不完整时，真实模型上的 md5 必然与期望值不一致，且无法从量化器的输出中看出原因。

## 本目录中生成的文件

按 `nano-quant/README.md` 的流程，下面几个文件也写在本目录：

| 文件 | 来源 |
| --- | --- |
| `meta.kv` | `python3 tools/mkmeta.py model -o model/meta.kv` |
| `*.nq`、`*.gguf` | `./nano-quant quant …`、`./nq2gguf …` |
| `Modelfile` | 由 ollama 加载运行时写出 |
| `*.part` | `tools/fetch-model.sh` 下载中断时留下的部分文件，下载完成后改名为原文件名 |

本目录下除 `README.md`、`SHA256SUMS` 与 `.gitignore` 以外的文件都不纳入版本管理。
