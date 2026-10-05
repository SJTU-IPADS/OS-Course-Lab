# mini-ollama

`mini-ollama` 读取第二讲实验 `nano-quant` 按 `q4_0` 配方得到的 GGUF 文件，逐个 Token 生成回答。
它与 ollama 读取同一个文件、执行相同的矩阵向量乘，全部运算采用最直接的写法：
单线程，每个权重单独还原、单独相乘、单独累加。第三讲第四部分用它与 ollama 对比生成速度。

## 编译与运行

```bash
make
./mini-ollama ../../../nano-quant/model/q4_0.gguf "用一句话介绍你自己。"
```

```
我是一个虚拟助手，没有实体，但我可以为你提供帮助和解答。
prompt eval rate:     0.94 tokens/s
eval rate:            0.77 tokens/s
matvec rate:          2.64 GFLOPS
```

`make run` 编译后执行上面第二条命令。模型文件与问题是 `Makefile` 中的变量 `MODEL` 与 `PROMPT`，
可以在命令行上改写：`make run PROMPT="1+1等于几？"`。

```
mini-ollama MODEL.gguf PROMPT [MAX_NEW_TOKENS]
```

- 回答写到标准输出，每生成一个 Token 输出一次；三行速率写到标准错误。
- `prompt eval rate` 是读入提示词的速度，`eval rate` 是生成回答的速度，
  两个名字与 `ollama run --verbose` 的输出相同。提示词的最后一个 Token 之前的各轮计入前者，
  此后每一轮生成一个 Token，计入后者。
- `matvec rate` 是全部矩阵向量乘的运算速率，1 次乘加计 2 次运算。
- `MAX_NEW_TOKENS` 是回答的最大 Token 数，默认 64。提示词与回答合计不超过 512 个 Token。

`q4_0.gguf` 的生成方法见 `../../../nano-quant/README.md`「量化与组装」一节，
把其中的 `--recipe q4_k_m` 换为 `--recipe q4_0`。

## 与 ollama 对比

`Modelfile` 让 ollama 加载同一个文件，使用相同的对话格式，每一步取分数最高的 Token，
只使用 CPU（`num_gpu 0`），只用 1 个线程（`num_thread 1`）。`make run-ollama` 执行下面两条命令，
需要 `ollama serve` 已在运行：

```bash
ollama create nq-q4-0 -f Modelfile
ollama run nq-q4-0 --verbose "用一句话介绍你自己。"
```

```
我是一个虚拟助手，没有实体，但我可以用文字与你交流，为你提供帮助和解答。

prompt eval rate:     29.01 tokens/s
eval rate:            10.65 tokens/s
```

`ollama run --verbose` 还输出各阶段的耗时与 Token 数，上面只保留两行速率。
使用其他线程数时通过 ollama 的 HTTP 接口传入 `num_thread`，它覆盖 `Modelfile` 中的数值：

```bash
curl -s http://127.0.0.1:11434/api/generate -d '{"model": "nq-q4-0",
  "prompt": "用一句话介绍你自己。", "stream": false, "options": {"num_thread": 8}}'
```

返回的 JSON 中，`eval_count` 除以 `eval_duration`（纳秒）是生成速度。
检查结束后用 `ollama rm nq-q4-0` 删除这个模型。

2026-10-05 在接通电源的 i9-11900H（8 核）上测得，gcc 15.2.0，ollama 0.33.2。
多次运行中 mini-ollama 在 0.75 ~ 0.77 Token/s 之间，1 个线程的 ollama 在 10.4 ~ 10.9 Token/s 之间：

| 程序 | 线程数 | 生成速度（Token/s） | 相对 mini-ollama |
| --- | --- | --- | --- |
| mini-ollama | 1 | 0.77 | 1 |
| ollama | 1 | 10.7 | 14 倍 |
| ollama | 2 | 18.8 | 24 倍 |
| ollama | 4 | 28.3 | 37 倍 |
| ollama | 8 | 31.4 | 41 倍 |

ollama 在这台机器上加载 `libggml-cpu-icelake.so`。其中计算 Q4_0 内积的函数
`ggml_vec_dot_q4_0_q8_0` 使用 `vpdpbusd` 指令，一条指令完成 32 对 8 位整数的乘法与求和；
`libggml-cpu-haswell.so` 中的同名函数使用 `vpmaddubsw` 与 `vpmaddwd`，
`libggml-cpu-x64.so` 中的同名函数只使用标量的 `imul`。查看方法：

```bash
objdump -d --no-show-raw-insn /usr/local/lib/ollama/libggml-cpu-icelake.so |
  awk '/<ggml_vec_dot_q4_0_q8_0@@Base>:/,/^$/' | grep vpdpbusd
```

| 程序 | 内积循环每轮的指令条数 | 每轮处理的权重个数 | 每次乘加的指令条数 |
| --- | --- | --- | --- |
| mini-ollama 的 `matvec` | 20 | 2 | 10 |
| ollama 的 `ggml_vec_dot_q4_0_q8_0`（icelake） | 22 | 32 | 0.69 |

## 程序结构

| 文件 | 内容 |
| --- | --- |
| `mini_ollama.c` | 主循环与一层之内的全部运算 |
| `model.c` | 把 GGUF 文件载入内存（`mmap`，Windows 上是 `fread`），解析键值段与张量目录，按名字找到每个张量 |
| `tokenizer.c` | 提示词转换为 Token 编号，Token 编号转换为文字 |
| `mini_ollama.h` | 三个文件共用的结构体与函数原型 |
| `Modelfile` | ollama 加载同一个模型文件所用的配置 |
| `Makefile` | 编译；`make run` 与 `make run-ollama` 用两个程序回答同一个问题 |

主循环在 `mini_ollama.c` 的末尾，每一轮让位置 `pos` 上的 Token 经过全部 28 层：

```c
for (pos = 0; pos + 1 < end; pos++) {
    embed(m, tok[pos], x);                      /* x = row tok[pos] of token_embd */
    for (int l = 0; l < m->n_layer; l++) {      /* 28 layers */
        attention(m, l, pos, x);                /* 4 matrix-vector products */
        feed_forward(m, l, x);                  /* 3 matrix-vector products */
    }
    if (pos + 1 < n)
        continue;                               /* the prompt gives the next token */
    tok[pos + 1] = next_token(m, x);            /* 1 matrix-vector product */
    if (tok[pos + 1] == m->eos)
        break;
    print_token(m, tok[pos + 1]);
}
```

| 函数 | 矩阵向量乘 | 权重个数 |
| --- | --- | --- |
| `attention` | `attn_q`、`attn_k`、`attn_v`、`attn_output` | 12 582 912 |
| `feed_forward` | `ffn_gate`、`ffn_up`、`ffn_down` | 37 748 736 |
| `next_token` | `token_embd` | 311 164 928 |

生成一个 Token 共 28 × 7 + 1 = 197 次矩阵向量乘，1 720 451 072 次乘加。
这些乘加全部在 `matvec` 中完成，它占运行时间的 99.7%。

`model.c` 用 `mmap` 把模型文件只读地映射到进程的地址空间，张量、词表都是指向映射区的指针，文件内容没有复制。
文件的每一页在程序第一次访问时进入内存，第一轮循环因此包含读入权重的时间；
文件已在系统的页缓存中时，这部分时间可以忽略（0.94 Token/s 的 `prompt eval rate` 即在这种情形下测得）。
Windows 没有 `mmap`：`model.c` 在定义了 `_WIN32` 时改用 `fread` 把整个文件读入 `malloc` 得到的缓冲区，
文件须小于 2 GB。两种方式由同一个函数 `load_file` 提供，其余代码相同。

`Makefile` 使用 `-O2 -fno-tree-vectorize`。GCC 12 起在 `-O2` 下自动向量化部分循环，
`-fno-tree-vectorize` 使 `matvec` 保持每次处理一个权重。

## 范围

- 矩阵只支持 Q4_0，一维张量为 F32。其他配方得到的文件在加载时报告
  `... is not Q4_0` 并退出。
- 每一步取分数最高的 Token，相当于 ollama 的 `temperature 0`。
- 分词器省略了按正则表达式切分单词的步骤，只保留其中「每个数字单独成为一个 Token」一条。
  普通句子的结果与模型自带的分词器相同；单词前有连续多个空格时，空格的切分位置不同。
- 整数按小端读取，程序在小端机器上运行。

## 核对

- 按 Hugging Face 的模型定义用 numpy 写出的参考实现在同一组 Q4_0 权重上整句计算，
  对上面的提示词生成的 17 个 Token 与 `mini-ollama` 相同。
- ollama（CPU）对同一提示词的回答是
  「我是一个虚拟助手，没有实体，但我可以用文字与你交流，为你提供帮助和解答。」
  前 9 个 Token 与 `mini-ollama` 相同。第 10 个 Token 上分数最高的两个候选
  「可以」与「可以用」相差 0.008；ollama 把激活量化为 8 位整数再计算，
  数值误差改变了这一步的选择。
- `model.c` 的 `fread` 分支在 Linux 上对调条件编译后编译并运行，回答与 `mmap` 分支相同。
  程序没有在 Windows 上运行过。
- 14 条中英文提示词的 Token 编号与按 `tokenizer.json` 的正则表达式实现的参考分词器比较，
  11 条相同，其余 3 条都含有单词前的连续空格。
