# nano-quant

把一个 BF16 的 safetensors 模型量化成 4 位，拼成 GGUF，交给 llama.cpp 跑起来。
配套《计算机系统基础》第二讲「数据的表示」的实验，题面见 [../LAB-quantize.md](../LAB-quantize.md)。

框架负责读文件、排清单、写容器、拼 GGUF；量化本身与其中的位操作由你写在
[student/student.cpp](student/student.cpp) 里。两边的分界是
[include/nano_quant.h](include/nano_quant.h)，判定程序只按那个头文件调用。

## 编译与自查

```
make                      # 生成 nano-quant、nq2gguf、nq-selftest
make fixtures             # 生成判定用的样例文件（约 11 MB，可随时重做）
sh tests/run.sh           # 跑一遍判定
```

`Makefile` 里的编译选项是判定的一部分，不要改：

| 选项 | 作用 |
| --- | --- |
| `-std=c++17 -O2` | 语言版本与优化级别 |
| `-ffp-contract=off` | 禁止把 `a*b+c` 合成一条 FMA 指令 |
| `-fno-fast-math` | 不允许假定浮点可交换、可结合 |

后两条决定了判定能不能跨机器成立。合成 FMA 之后，`a*b` 的中间结果不再舍入到
float，量化边界上的取整会跟着变，同一份代码在两台机器上会写出不同的字节。

## 三个程序

### nano-quant

```
nano-quant plan  <model.safetensors> [--recipe q4_0|q4_1|q4_k|q4_k_m]
nano-quant quant <model.safetensors> [--recipe ...] -o <out.nq> [--limit N]
```

`plan` 只排清单，不读权重，一行一个张量：

```
token_embd.weight Q6_K 2048 151936 255252480
blk.0.attn_norm.weight F32 2048 8192
blk.0.attn_q.weight Q4_K 2048 2048 2359296
```

字段是「GGUF 名字、类型、各维长度、字节数」。汇总走标准错误，不混进管道，
所以账目可以直接接给别的程序：

```
nano-quant plan model.safetensors | awk '{s += $NF} END {print s}'
```

`quant` 逐段读权重、量化、写进 `.nq`，最后把数据区的 md5 打到标准输出。
峰值内存只与段长有关（4 Mi 个元素，16 MB），与最大的张量无关。

### nq2gguf

```
nq2gguf <in.nq> --meta <meta.kv> -o <out.gguf>
```

`meta.kv` 由 [tools/mkmeta.py](tools/mkmeta.py) 从 Hugging Face 仓库里的
`config.json` 与 `tokenizer.json` 生成，里面已经是 GGUF 的键值编码。
`nq2gguf` 把它原样搬进去，另外补 `general.file_type` 与
`general.quantization_version` 两个与量化有关的键。

```
python3 tools/mkmeta.py ~/models/Qwen3-VL-2B-Instruct -o meta.kv
```

### nq-selftest

```
./nq-selftest                 与 tests/reference-blocks.txt 对照
./nq-selftest a q4_0          只跑点到名的组
./nq-selftest --full          f32_to_fp16 改成遍历全部 2^32 个 float
```

不带组名时全跑，任何一个函数没写就在那里停下。带组名时只跑点到的那几组，
组名有 `a`、`scale_min`、`q4_0`、`q4_1`、`q4_k`、`q6_k`（`q4_k` 连带跑
`scale_min`），于是写完一部分就能单独看这一部分过没过。习题课讲义
[`../EXERCISE-q4_0.md`](../EXERCISE-q4_0.md) 用的是 `a q4_0`。

不需要模型文件。检查 A、B 两部分：`rd_u64le` 的三个样例、两个加宽函数在
全部 65536 个位型上与参考一致、`f32_to_fp16` 的取整与往返、
`put_scale_min` 与 `get_scale_min` 在 64×64×8 组取值上互逆，
以及三种格式在 512 个超块上的字节摘要与还原摘要。

那 512 个超块里前 8 个是刻意构造的：全零、全相等、含一个离群值、全负、全正、
量级 1e-7、关于 0 对称、只有一个非零点。这几种情形在真实权重里都出现过，
也是最容易写错的地方。

## 目录

| 路径 | 内容 |
| --- | --- |
| `include/nano_quant.h` | 学生与框架的接口，不要改 |
| `include/nq.h` | 框架自己用的接口 |
| `student/student.cpp` | 你写的代码 |
| `src/st.cpp` | safetensors 头解析与流式读取 |
| `src/q6_k.cpp` | 助教提供的 Q6_K |
| `src/quant.cpp` | 类型表与成行的量化循环 |
| `src/nqfile.cpp` | `.nq` 容器 |
| `src/recipe.cpp` | 配方判据与名字映射表 |
| `tools/mkmeta.py` | 从 HF 文件生成 GGUF 的键值段 |
| `tools/mkfixtures.py` | 生成判定用的样例 safetensors |
| `tools/ggufdump.py` | 读一个 GGUF，检查排布是否自洽 |
| `tools/nq-verify` | 判定三：挂到 ollama 上跑一次生成 |

## safetensors 的排布

文件是三段：8 字节的小端头长度 N、N 字节的 UTF-8 JSON 头、其余是张量数据。
JSON 头的每个键是张量名，值给出 `dtype`、`shape` 与 `data_offsets`；
`data_offsets` 相对数据区起点（也就是 8 + N），左闭右开。

`shape` 是行优先的，`shape[0]` 变化最慢。GGUF 的 `ne` 相反，`ne[0]` 变化最快。
两者的字节序列完全相同，改的只是这一串长度的书写顺序，不需要转置。

框架读头长度时调的是你的 `rd_u64le`。这一步写反了，得到的数会大得离谱，
`nano-quant` 会报出来并提示检查移位方向。

## `.nq` 容器

```
[0,4)    "NQ01"
[4,8)    uint32 张量数
[8,16)   uint64 数据区起点
[16,24)  uint64 数据区字节数
[24,..)  逐个张量：名字长度、名字、类型、维数、各维长度、区内偏移、字节数
```

数据区起点对齐到 32，区内每个张量也对齐到 32，空隙留零。
判定二用的是数据区这一段的 md5，与文件头无关。

## 配方

一维张量（各种归一化系数）一律留 F32。二维张量按配方定：

| 配方 | 规则 |
| --- | --- |
| `q4_0` | 全部 Q4_0 |
| `q4_1` | 全部 Q4_1 |
| `q4_k` | 全部 Q4_K |
| `q4_k_m` | 词表 Q6_K；部分层的 `v_proj` 与 `down_proj` Q6_K；其余 Q4_K |

`q4_k_m` 里「部分层」的判据取自 llama.cpp 的 `use_more_bits`：共 n 层时，
第 i 层满足下面任一条就升到 Q6_K。

```
i < n/8        或        i >= 7n/8        或        (i - n/8) % 3 == 2
```

n = 28 时符合的是 0、1、2、5、8、11、14、17、20、23、24、25、26、27 共 14 层。
两头的层各留一段整数除法的余量，中间每三层挑一层。

## 名字映射

Qwen3-VL 的语言模型张量前缀是 `model.language_model.`，视觉塔是 `model.visual.`。
本实验只处理前者，625 个张量里选中 310 个。

| safetensors | GGUF |
| --- | --- |
| `model.language_model.embed_tokens.weight` | `token_embd.weight` |
| `model.language_model.norm.weight` | `output_norm.weight` |
| `…layers.N.input_layernorm.weight` | `blk.N.attn_norm.weight` |
| `…layers.N.self_attn.q_proj.weight` | `blk.N.attn_q.weight` |
| `…layers.N.self_attn.k_proj.weight` | `blk.N.attn_k.weight` |
| `…layers.N.self_attn.v_proj.weight` | `blk.N.attn_v.weight` |
| `…layers.N.self_attn.o_proj.weight` | `blk.N.attn_output.weight` |
| `…layers.N.self_attn.q_norm.weight` | `blk.N.attn_q_norm.weight` |
| `…layers.N.self_attn.k_norm.weight` | `blk.N.attn_k_norm.weight` |
| `…layers.N.post_attention_layernorm.weight` | `blk.N.ffn_norm.weight` |
| `…layers.N.mlp.gate_proj.weight` | `blk.N.ffn_gate.weight` |
| `…layers.N.mlp.up_proj.weight` | `blk.N.ffn_up.weight` |
| `…layers.N.mlp.down_proj.weight` | `blk.N.ffn_down.weight` |

Qwen3-VL 的 `config.json` 里 `tie_word_embeddings` 为真，词表矩阵同时充当
输出层，文件里没有 `lm_head.weight`，产物里也不要写 `output.weight`：
llama.cpp 见到缺这一项会自己复用 `token_embd`。

产物里张量的顺序就是 safetensors 头里的顺序。顺序变了，数据区的 md5 就变了。

## Q4_K 子块的搜索

Q4_0 与 Q4_1 的步长有闭式解，Q4_K 的没有。它在 32 个元素的子块上找一对
（缩放 s、偏移 m），使加权平方误差最小：

```
E = Σ w_i (s·q_i + m − x_i)²        w_i = sqrt(Σx²/32) + |x_i|
```

权重 `w_i` 让绝对值大的元素说话更响，同时给整块一个与量级有关的底。
搜索分两步。

第一步取起点。令 `min = min(x)`（若为正则取 0）、`max = max(x)`，
`iscale = 15/(max − min)`，`q_i = round(iscale·(x_i − min))` 截到 `[0, 15]`，
`s = 1/iscale`，算出此时的 E 作为当前最好值。`min > 0` 时压到 0，
是为了让 0 一定落在某个级上——权重矩阵里 0 出现得很多。

第二步在 21 个候选上试。第 is 个候选（is 从 0 到 20）取

```
iscale = (−1 + 0.1·is + 15) / (max − min)
```

即把满量程从 14 逐步放宽到 16。每个候选先按 `iscale` 定出一组 `q_i`，
再对固定的 `q_i` 解最小二乘，得到该组下最优的 s 与 m：

```
D  = Σw · Σwq² − (Σwq)²
s  = (Σw · Σwqx − Σwx · Σwq) / D
m  = (Σwq² · Σwx − Σwq · Σwqx) / D
```

`D <= 0` 的候选跳过。解出的 m 若为正则压到 0，此时 `s = Σwqx / Σwq²`。
算出该候选的 E，比当前最好值小就换过去。

八个子块各自跑完之后，八个 s 与八个 m 再各自量化成 6 位：超块存
`d = max(s)/63` 与 `dmin = max(m)/63` 两个 fp16，每个子块存 `round(63·s/max(s))`
与 `round(63·m/max(m))`。这 16 个 6 位数用 `put_scale_min` 压进 12 字节。
最后按定下来的 `d·sc` 与 `dmin·m` 重新算一遍 `q_i`，写进 `qs`。

`qs` 的 128 字节按 64 个元素一组排：低 4 位放元素 e，高 4 位放元素 e+32。

取整一律用 `nano_quant.h` 里的 `nq_round`，就近舍入、平局取偶。
换成 `(int)(x + 0.5f)` 在负数处与平局处都会不同，字节对不上。

## 判定的三层

1. **逐块**：`nq-selftest`，与 `tests/reference-blocks.txt` 对照。
2. **整个产物**：`.nq` 数据区的 md5，全班同一个值。样例文件的参考值在
   `tests/reference-tiny-nq.md5`。
3. **跑起来**：`tools/nq-verify <model.gguf> <学号>`，报输出文本的 md5，
   前 8 位对得上即为通过。

第三层的条件定死为贪心解码、128 个词、纯 CPU、上下文 4096。温度为 0 时
采样不查随机数，种子取什么都一样。`num_gpu 0` 是必须的：CPU 与 GPU 后端
把同一串加法排成不同的次序，长文本上可能分岔。

## 已经验证过的

- 三种格式加 Q6_K 在 137216 个权重上与 llama.cpp 的 `quantize_row_*_ref`
  逐字节相同，样本含全零块、离群值块、量级 1e-7 的块与真实模型的两段切片。
- `nq_f32_to_fp16` 在全部 2^32 个非 NaN 的 float 上与硬件的 `_Float16`
  转换一致；`nq_fp16_to_f32` 在全部 65536 个位型上一致。
- 用小样例走完 `plan → quant → nq2gguf` 之后，ollama 里的 llama.cpp
  完整加载了产物：46 个张量识别为 17 个 F32、24 个 Q4_K、5 个 Q6_K，
  架构、张量名与 `rope.dimension_sections` 都被接受。
- 真实的 `Qwen/Qwen3-VL-2B-Instruct` 走完了全程，两种配方各一次，
  数值见 `tests/reference-qwen3vl.txt`。`plan` 预测的字节数与产物数据区
  精确相等；`ulimit -v 262144` 下 `quant` 与 `nq2gguf` 都跑得完，
  产物 md5 与不设限时相同；峰值常驻内存 48 MB。
- 两个产物都被 ollama 0.33.2 加载并生成了文字。同一条命令重复三次，
  输出的 md5 逐次相同；换四个学号，四个 md5 两两不同。

### 真实模型上的数

| 配方 | 张量数据 | 位/权重 | 耗时 | GGUF 文件 |
| --- | --- | --- | --- | --- |
| `q4_0` | 968 249 344 B | 4.502 | 8 秒 | 974 201 696 B |
| `q4_k_m` | 1 101 457 408 B | 5.121 | 88 秒 | 1 107 409 760 B |

已公开的 `unsloth/Qwen3-VL-2B-Instruct-GGUF` 的 Q4_K_M 是 1 107 410 624 B，
与本框架的 `q4_k_m` 产物差 864 字节。两份元数据段的字符串长度不同，
这个量级的差与之相称，配方表的张量类型分派与 llama.cpp 一致。
