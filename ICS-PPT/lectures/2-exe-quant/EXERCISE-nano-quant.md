# ICS 习题课一 · 实验题面：`nano-quant`

本讲第五、第六部分讲量化。本实验把讲过的三种格式应用于一个真实模型：
学生从 Hugging Face 获取 `Qwen/Qwen3-VL-2B-Instruct`，自行读取它的 safetensors 文件，
把其中 310 个语言模型张量量化为 Q4_0、Q4_1、Q4_K，度量误差，最后让量化后的模型
以自己的学号为输入生成一段文字，提交这段文字的 md5。

本文是发给学生的题面：实验对象、四个部分的任务、判定方式与分值。
框架代码与测试都已经完成，位于 [`../nano-quant/`](../nano-quant/) 下，
完成情况见第八节。A 部分与 Q4_0 在习题课上由本讲的幻灯片带着完成，
其余部分课后完成。

---

## 〇、本版的修改

前一版使用 `meta-llama/Llama-3.2-1B-Instruct`。改用 `Qwen/Qwen3-VL-2B-Instruct`
消除了门控仓库的排期风险，同时带来三处代价与一处必须更正的判定方式，列于下表：

| 项 | 结论 |
| --- | --- |
| 许可 | Apache-2.0，无门控，无附加使用条款。已核实，见第一节 |
| 下载量 | 4.25 GB，比前一版多 1.78 GB，其中 0.81 GB 是本实验不用的视觉塔 |
| 张量筛选 | 文件中 625 个张量，只有 310 个进入产物，筛选本身成为 C 部分的一项判定 |
| q / k 重排 | Qwen 不需要，llama.cpp 的转换脚本对 Qwen3 系列不做旋转编码重排。原 D 部分的这项练习取消，改为张量筛选与绑定权重两项 |
| 种子与温度 | `temperature = 0` 时采样退化为取最大值，随机数发生器不被调用，种子不起作用。输出可复现来自贪心解码与固定的运行环境。实测见第四节 |

框架已经实现，位于 [`../nano-quant/`](../nano-quant/) 下，用 C++17 编写。
第八节记录了它的完成情况与待办事项。

---

## 一、实验对象

| 文件 | 大小 | 用途 |
| --- | --- | --- |
| `model.safetensors` | 4 255 140 312 B | 主对象。`Qwen/Qwen3-VL-2B-Instruct`，625 个张量，全部 BF16 |
| `tiny.safetensors` | 数 KB | 调试。助教用脚本生成，张量少，可完整打印 |
| `big.safetensors` | 稀疏文件，实占数 KB | 规模验证。头中声明的偏移超过 4 GiB，用 `truncate` 生成 |
| `tiny-be.safetensors` | 数十字节 | 反例。头长度按大端写入，工具必须拒绝并说明理由 |

后两个文件是本实验中仅有的两个不需要真实权重的对象，作用是使两类错误由「可能不触发」
变为「必然触发」。这里有一处新情况：真实文件的最大偏移是 4 255 064 064，
即 `0xFD9F2000`，大于 2^31 而小于 2^32。它已经能使用 `int` 存储偏移的写法溢出为负数，
但不能暴露 `p[i] << (8 * i)` 的移位越界，后者仍然只有 `big.safetensors` 能触发。
两个文件都要保留，理由写入实验说明。

### 许可核实

2026-09-07 查得，`Qwen/Qwen3-VL-2B-Instruct` 的仓库元数据中 `gated` 为 false，
模型卡的 front matter 写 `license: apache-2.0`，仓库内没有 LICENSE 文件，
README 全文没有第二处许可相关表述，没有附加使用条款，没有可接受使用政策。

Apache-2.0 允许使用、修改与再分发，条件是保留许可与版权声明，并在派生物中
说明修改。对本实验的含义：

- 学生下载与量化无需申请，不存在审核排期；
- 量化产物是派生物，若要公开分发，需附 Apache-2.0 全文与「量化自
  `Qwen/Qwen3-VL-2B-Instruct`」的说明。**实验仍规定产物不公开分发**，只在课程环境内提交，
  理由是免去逐份检查许可声明的工作；
- 与前一版的 Llama 3.2 Community License 相比，去掉了「派生模型名必须以 Llama 开头」
  与「说明里注明 Built with Llama」两条命名要求。

### 模型的获取

无门控，`https://huggingface.co/Qwen/Qwen3-VL-2B-Instruct/resolve/main/model.safetensors`
可直接下载。仍然提供两条路径：

1. 助教在校内文件服务上提供一份，公布 sha256。全班每人下载 4.25 GB 的外网流量不可行，
   这是主路径；
2. 学生自行从 Hugging Face 或其镜像下载，用公布的 sha256 校验。

排期上不再需要提前两周，这是更换模型的主要收益。

### 文件的字节布局

```
[0, 8)                          8 字节，小端 uint64，JSON 头的长度 N = 76 240
[8, 76 248)                     JSON 头，每个张量一条
[76 248, 4 255 140 312)         张量数据，长 4 255 064 064
```

`8 + 76 240 + 4 255 064 064 = 4 255 140 312`，与 `ls -l` 一致。625 个张量在数据区中
首尾相接，没有对齐填充，也没有空洞，这一点由学生自行验证。

625 个张量按名字前缀分为两部分：

| 前缀 | 张量数 | 参数量 | 字节数 | 本实验 |
| --- | --- | --- | --- | --- |
| `model.language_model.` | 310 | 1 720 574 976 | 3 441 149 952 | 全部处理 |
| `model.visual.` | 315 | 406 957 056 | 813 914 112 | 全部跳过 |

310 = 28 层 × 11 + `embed_tokens` + `norm`。每层的 11 个是
`self_attn` 的 `q_proj`、`k_proj`、`v_proj`、`o_proj`、`q_norm`、`k_norm`，
`mlp` 的 `gate_proj`、`up_proj`、`down_proj`，以及两个 layernorm。

这个模型的 `tie_word_embeddings` 为 true，文件中**没有** `lm_head.weight`：
输出头与输入词嵌入是同一个矩阵。

最大的张量是 `model.language_model.embed_tokens.weight`，形状 151936 × 2048，
单独占 622 329 856 B。

### 磁盘与内存

输入 4.25 GB，产物约 1.10 GB，不产生中间文件，准备 6 GB 空间即可。

内存有硬约束：**峰值不超过 256 MiB**，用 `ulimit -v 262144` 运行测试来判定。
最大的张量单独就有 622 MB，一次读入内存必然超限；`ulimit -v` 限制的是地址空间，
因此 `mmap` 整个文件同样超限。学生只能按行或按块 `pread`。
这条约束是本实验中成本最低、收益最高的一条：它把「文件比内存大」从一句陈述
变为一个会失败的测试。

---

## 二、safetensors 是否值得手写解析

值得，而且比 GGUF 简单。整个格式只有三段，第一节已经给出。
唯一的难点是 JSON。手写 JSON 解析器不属于本课程的内容，因此**头的解析由助教提供**，
接口是：

```c
int st_open(st_file *f, const char *path);          /* 读头长度、解析 JSON、建索引 */
int st_count(const st_file *f);
const st_tensor *st_get(const st_file *f, int i);   /* name, dtype, ndim, shape[], off, len */
size_t st_read(const st_file *f, const st_tensor *t, size_t elem_off,
               uint16_t *dst, size_t n_elem);       /* 从张量内的第 elem_off 个元素起读 n 个 */
```

`st_open` 的实现中预留了学生的部分：头长度的组合调用学生实现的 `rd_u64le`，
校验也在学生的代码中。框架负责 JSON，学生负责字节。

safetensors 的二维张量按行主序存放，形状写作 `[输出维, 输入维]`；
GGUF 的同一个张量记作 `ne = [输入维, 输出维]`，两者的字节序列相同。
因此按行量化就是按 GGUF 的行量化，全程不需要转置。这一点要写入实验说明，
否则学生会寻找一个并不存在的转置步骤。

---

## 三、学生的任务

框架已经完成，位于 [`../nano-quant/`](../nano-quant/) 下，说明见
[`../nano-quant/README.md`](../nano-quant/README.md)。一个头文件
`nano_quant.h` 由助教在第一周发布，之后只增加、不修改。学生在
`impl/nano_quant.cpp` 中实现其中的全部函数；其余部分（命令行、遍历、输出、测试）由框架提供。

编译选项由框架固定，其中 `-ffp-contract=off` 是强制的：
允许编译器把乘加合并为 FMA 会改变舍入结果，第四节的逐字节判定将不再跨机器成立。

```
c++ -std=c++17 -O2 -ffp-contract=off -fno-fast-math ...
```

框架另外提供一个 `nq_round`：即加上 2^23 + 2^22 再取尾数低位的惯用写法，
就近舍入、平局取偶。三种格式的取整一律使用它。改用 `(int)(x + 0.5f)`
在负数处与平局处结果不同，无法通过逐字节判定。

### A 读字节（约 3 小时）

```c
uint64_t rd_u64le(const uint8_t *p);      /* 8 字节，小端，显式组合 */
float    bf16_to_f32(uint16_t h);         /* BF16 是 FP32 的高 16 位 */
float    fp16_to_f32(uint16_t h);         /* 按 1-5-10 的字段组合 */
uint16_t f32_to_fp16(float f);            /* 就近舍入到偶数 */
```

四个函数各配一项可判定的测试：

1. `rd_u64le` 在助教给定的字节向量上逐条相等，其中包含
   `ff ff ff ff ff ff ff ff` 与只有第 7 字节非零的两条。
   **禁止 `memcpy` 到 `uint64_t`**，必须用移位与按位或实现。
   常见错误是写 `p[i] << (8 * i)`：`uint8_t` 提升为 `int`，`i >= 4` 时移位越界，
   结果在小文件上完全正确。`big.safetensors` 的偏移超过 4 GiB，专门用于检出这一错误。
2. `bf16_to_f32` 与期望值逐位相等，覆盖 0、±inf、NaN、次正规数各一例。
3. `fp16_to_f32` 在全部 65 536 个 fp16 位模式上与 `_Float16` 的转换一致。
4. `f32_to_fp16` 与框架自带的 `nq_f32_to_fp16` 逐个比对，NaN 只比对是否为 NaN，另外要求
   半精度能精确表示的每一个值往返转换后不变。默认按质数步长抽样 1700 万个位模式，
   `nq-selftest --full` 遍历全部 2^32 个，约一分钟。

第 4 项的难点是就近舍入到偶数、次正规数与上溢到 inf，本讲第四部分已经讲解。
它不是附加项：三种格式的缩放系数都用它写出，它出错会导致整个产物的字节与期望值不一致。
框架的 `nq_f32_to_fp16` 已在全部 2^32 个非 NaN 的 float 上与 GCC 的
`_Float16` 转换核对过，自查以它为比对基准。

### B 三种量化格式（约 6 小时）

```c
void q4_0_quantize(const float *x, uint8_t *blk);    /* 32 个权重 -> 18 字节 */
void q4_0_dequantize(const uint8_t *blk, float *x);
void q4_1_quantize(const float *x, uint8_t *blk);    /* 32 个权重 -> 20 字节 */
void q4_1_dequantize(const uint8_t *blk, float *x);
void q4_k_quantize(const float *x, uint8_t *blk);    /* 256 个权重 -> 144 字节 */
void q4_k_dequantize(const uint8_t *blk, float *x);
void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m);   /* 已给出 */
void put_scale_min(int j, uint8_t *q, uint8_t sc, uint8_t m);   /* 16 个 6 位数 -> 12 字节 */
```

`get_scale_min` 在讲义中给出（`k-scales-code` 页），实验中提供的也是它。
**`put_scale_min` 由学生实现**：读取格式决定了写入格式，这是这一部分设计上的关键：
学生已有解码器，需要实现与之互逆的编码器。判定采用穷举：
64 × 64 组取值、每组 8 个位置，`put` 之后 `get` 的结果必须全部一致
（讲义的 `examples/k_scales.c` 是同一个测试的随机版本，可以直接提供给学生作为自测）。

Q4_K 每个子块的步长与偏移由迭代搜索确定，讲义没有讲解这个搜索。
**这个搜索的完整算法写入实验说明**，与 llama.cpp 的 `make_qkx2_quants` 等价，
包括迭代次数、初值、比较的方向与「偏移为正时置为 0」这一条。
书面描述在 [`../nano-quant/README.md`](../nano-quant/README.md) 的「Q4_K 子块的搜索」一节。
理由见第四节：第二层判定要求产物与期望值逐字节相同，搜索有一步不同即无法满足。
搜索算法本身不计分，按说明实现即可；计分的是格式的布局与打包。

其余判定：

| 项 | 判定 | 检出的错误 |
| --- | --- | --- |
| 误差上界 | 每个权重 `|w − dequant(quant(w))| <= |d|`，未落在极值编码上的满足 `<= |d|/2` | 缩放系数取错方向 |
| 幂等 | quantize → dequantize → quantize 后码字不变 | 舍入写成截断 |
| 逐字节比对 | 对助教给定的一组权重，18 / 20 / 144 字节与期望的块完全相同 | 见下 |
| 相对误差 | 在 `w-down-proj.bf16` 上，三种格式的相对 RMSE 与讲义表格相差不超过 0.01 个百分点 | 使用了不同的极值定义 |

逐字节比对约束了四个只影响布局、不影响单点误差的细节，前三项测试均无法检出它们：

- Q4_0 的 `d` 取 `extreme(x) / -8`；
- Q4_0 / Q4_1 的 `qs[j]` 对应第 `j` 与第 `j+16` 个权重；
- Q4_K 的 `qs` 以 64 个权重为一组，第 `e` 与第 `e+32` 个权重共用一个字节；
- Q4_K 的偏移取 `min(0, lo)`，即偏移不取正值。

最后一条使 Q4_K 在全为正数的张量上明显差于 Q4_1。该模型中
`model.language_model.layers.0.input_layernorm.weight` 的 2048 个权重全为正，
取值在 0.0537 与 1.1016 之间，是一个实例。实现正确才会观察到这一现象，因此
**要求学生在报告中解释这一行数据**，这是 B 部分唯一一道文字题。

### C 全模型量化与字节统计（约 3 小时）

```
$ ./nano-quant plan  model/model.safetensors --recipe q4_k_m
$ ./nano-quant quant model/model.safetensors --recipe q4_k_m -o qwen3vl-2b-q4km.nq
```

`plan` 读取头，输出每个张量的类型、元素数、量化后字节数与累计字节数；
`quant` 执行量化。强制验收条件是
**`plan` 预测的总字节数与 `quant` 产物的 `ls -l` 完全相等**。
对齐填充可以精确计算，因此这里不接受近似：任何一个字段的宽度或字节序读错，
总字节数就不一致，学生需要自行找出出错的字段。

本版新增一条：`plan` 必须排除 315 个 `model.visual.` 张量，
并在末尾单独报告被跳过的张量数与字节数。跳过的判据由学生实现。
清单输出到标准输出，汇总输出到标准错误，因此清单可以直接作为其他程序的输入。

配方由助教给出，是一张「张量名模式 → 格式」的表，学生只实现按表分派：

| 张量 | 格式 | 位/权重 | 张量数 |
| --- | --- | --- | --- |
| 一维张量（各 norm，含 `q_norm` / `k_norm`） | F32 | 32 | 113 |
| `embed_tokens.weight` | Q6_K | 6.5625 | 1 |
| 14 个指定层的 `v_proj` 与 `down_proj` | Q6_K | 6.5625 | 28 |
| 其余二维张量 | Q4_K | 4.5 | 168 |

「14 个指定层」是 llama.cpp 对 Q4_K_M 采用的选择规则，层号
0、1、2、5、8、11、14、17、20、23、24、25、26、27，即前 1/8、后 1/8 与其间每三层取一层。
选择规则由助教给出。`embed_tokens` 取 Q6_K 的理由是它同时充当输出头。

按这张表计算的字节数：

| 方案 | 字节数 | 位/权重 |
| --- | --- | --- |
| 全 Q4_0 | 968 249 344 | 4.502 |
| 全 Q4_K | 968 249 344 | 4.502 |
| 全 Q4_1 | 1 075 777 536 | 5.002 |
| `q4_k_m` 配方 | 1 101 457 408 | 5.121 |

全 Q4_0 与全 Q4_K 的字节数完全相同，两者都是每权重 4.5 位；讲义第 61 页给出了计算。
BF16 的 3 441 149 952 B 对 `q4_k_m` 的 1 101 457 408 B，压缩比 3.12。

这四个数由本文的统计脚本计算，已经与完整实现在真实模型上的实际输出核对：
`q4_0` 与 `q4_k_m` 两个配方的 `plan` 预测值与 `quant` 产物数据区精确相等。

也与已公开的 `unsloth/Qwen3-VL-2B-Instruct-GGUF` 做了对照。它的 Q4_K_M 是
1 107 410 624 B，本框架的同名产物是 1 107 404 384 B，两者差 6 240 字节。
两份元数据段的字符串长度不同，差值的量级与之相符。前一版本文中给出的
247 552 B 差值来自对元数据大小的估算。

三条要求：

1. `plan` 的输出可以直接由 `awk` 求和，一行一个张量，字段用空格分隔，
   最后一个字段是字节数：`nano-quant plan m.safetensors | awk '{s += $NF} END {print s}'`；
2. 报告中给出四种方案的体积与加权平均位宽，并用讲义 `why-quantize` 页的
   带宽数据估算各自的生成速度；
3. 全程峰值内存不超过 256 MiB。

### D 加载运行（约 3 小时）

产物要能被真实推理程序加载。完整的 GGUF 写入涉及分词器等与本讲无关的元数据，
因此分为两步：助教提供 `nq2gguf`，把一份预先准备的元数据块与学生的张量数据区组装为
一个合法的 `.gguf`；学生负责让张量数据区的内容与顺序完全符合它的预期。

```
$ python3 tools/mkmeta.py model -o model/meta.kv
$ ./nq2gguf qwen3vl-2b-q4km.nq --meta model/meta.kv -o qwen3vl-2b-q4km.gguf
$ tools/nq-verify qwen3vl-2b-q4km.gguf 523030910000
```

元数据块由 `tools/mkmeta.py` 从 Hugging Face 仓库中的 `config.json` 与
`tokenizer.json` 直接生成，架构是 `qwen3vl`，
其中三个键与文本无关，但必须正确：`qwen3vl.rope.dimension_sections` 为 `[24, 20, 20, 0]`，
`qwen3vl.n_deepstack_layers` 为 3，`qwen3vl.rope.freq_base` 为 5 000 000。
前两个来自视觉塔的配置，纯文本输入时它们参与的计算步骤加的是 0。

学生负责框架不提供的三项工作：

- **名字映射**：`model.language_model.layers.0.self_attn.q_proj.weight` 对应
  `blk.0.attn_q.weight`，映射表由助教给出（见
  [`../nano-quant/README.md`](../nano-quant/README.md) 的「名字映射」一节），
  另外要从张量名中取出层号；写出顺序就是 safetensors 头中的顺序，
  顺序错误时文件仍然合法、仍能加载，但数据区的 md5 与期望值不一致；
- **张量筛选**：315 个视觉张量都不能写入，多写一个就与元数据块中的清单不一致；
- **绑定权重**：这个模型没有 `lm_head.weight`，产物中也**不能**有 `output.weight`。
  llama.cpp 读取不到它时会复用 `token_embd.weight`。若自行添加，
  产物会多出 255 252 480 B，`nq2gguf` 检查张量清单不一致并报错。

前一版在这一步要求学生发现 q / k 的旋转编码重排。Qwen3 系列不需要这个重排，
llama.cpp 的转换脚本对它们不做重排，本版取消了这项练习。
上面第二、第三条是替代内容：判定同样不可伪造，定位同样要从现象反推。

---

## 四、判定：三层

三层判定按可复现性从强到弱排列。**每一层通过是下一层通过的前提**，
因此首先失败的一层指出了错误所在的阶段。

### 第一层：块级逐字节（与机器无关）

助教给出一组固定的 256 个权重，学生的三种格式各输出 18 / 20 / 144 字节，
与期望的块逐字节相同。这一层只用整数与四则运算，固定 `-ffp-contract=off` 后
在任何机器、任何编译器上结果相同。

### 第二层：产物 md5（与机器无关，与期望值相同）

`quant` 产物的张量数据区的 md5 与助教公布的期望值相同。
`q4_k_m` 配方是 `6b7031b4ff51880111226bca94d681da`，全 Q4_0 是
`fa4f64a2e2be8d8bdb7dad8c0ed06e27`，两者的字节数分别是 1 101 457 408 与
968 249 344。期望值与生成条件记录在 `../nano-quant/tests/expected-qwen3vl.txt`。
这一层覆盖全部 310 个张量、配方分派、跳过视觉张量、写出顺序，
是本实验中最强的一项判定。它成立的前提是学生量化器的输出与 llama.cpp 的算法**逐字节相同**，
所以第三节 B 部分要把 Q4_K 的搜索算法完整写入实验说明。

### 第三层：生成文本的 md5（每人一个值，需要固定后端与版本）

学生以自己的学号为输入运行一次生成，提交输出文本的 md5。
助教为每位同学公布该 md5 的**前 8 位**，学生据此自查。

输入是一段固定的字节，其中只有学号因人而异。Qwen3-VL 的对话模板在没有系统消息
且没有工具时只产生下面三行，提示词按原样使用即可（措辞的理由见本节末尾）：

```
<|im_start|>user
把这串数字逐位用中文写出来：523030910000<|im_end|>
<|im_start|>assistant
```

运行由助教提供的 `nq-verify` 完成。使用 ollama 即可，学生无需自行编译 llama.cpp：

```
ollama create nq-<学号> -f Modelfile      # Modelfile 只有一行 FROM ./qwen3vl-2b-q4km.gguf
curl -s http://localhost:11434/api/generate -d @req.json | jq -r .response | md5sum
```

`req.json` 中 `"raw": true`，提示词就是上面三行的字节，模板不参与；
`options` 固定为 `{"temperature": 0, "num_predict": 128, "num_gpu": 0, "num_ctx": 4096}`。
`"raw": true` 不能省略：ollama 会按 GGUF 中的模板组装提示词，模板随版本变化，raw 模式不使用模板。
`nq-verify` 在生成结束后查询 `/api/ps`，模型占用的显存不为 0 时报错退出，
因此判定结果一定来自纯 CPU 推理。

**关于种子的更正。** `temperature = 0` 让采样退化为取最大 logit，随机数发生器不被调用，
种子取 2026 或其他值都不影响输出。写明 `"seed": 2026` 只是记录参数，
它不是可复现性的来源。可复现性来自贪心解码与固定的数值流程。

### 实测一（2026-09-07，i9-11900H + RTX 3060，ollama 0.33.2，llama3.2，贪心，128 token）

| 改动的量 | 输出的 md5 |
| --- | --- |
| 同一条命令重复运行 | 相同 |
| `seed` 取 2026 / 1 / 99999 | 相同 |
| `num_thread` 取 1 / 2 / 4 / 8 / 16 | 相同 |
| `num_ctx` 取 4096 / 2048 | 相同 |
| `num_batch` 取 512 / 128 | 相同 |
| `num_gpu` 取 0 / 99（CPU 与 GPU） | 一段提示词下**不同**，更换提示词后相同 |

三点结论：

1. **线程数不影响结果。** ggml 按行切分矩阵乘，一个输出元素的整个点积在同一个线程中
   按同一顺序累加，线程数改变时累加顺序不变。原先「线程数会改变 token」的说法不成立。
2. **同一后端下未测出任何差异。** 固定模型文件、固定 ollama 版本、固定 `num_gpu: 0`，
   重复运行结果逐字节相同。这一层在单一环境中可以保证。
3. **更换后端之后输出可能相同，也可能不同。** 同一台机器、同一个模型，
   只更换提示词，CPU 与 GPU 的输出一次不同、一次相同。原因是贪心解码只在
   前两名 logit 的间距小于数值误差时才会改变结果，是否改变取决于具体的输入。
   对判分而言，「多数学生一致、少数学生无法解释地不一致」与「全部不一致」同样难以处理。

因此这一层的判定条件写成：**`num_gpu` 必须为 0，ollama 版本由助教指定。**
固定这两项之后，剩下的唯一变量是 CPU 指令集：ollama 随安装包提供 14 个
`libggml-cpu-*.so`，按 CPU 特性选择一个，AVX2 与 AVX-512 的点积累加宽度不同。
**这一项本文没有实测**，因为系统的模型目录属于 root，无法启动第二个服务端来指定变体。
需要时可以使用 `GGML_BACKEND_PATH`：把单个变体放入一个目录并将该变量指向它，
全班即使用同一个内核。

两条实施路径，二选一，在发布前确定：

1. 助教指定 ollama 版本、`num_gpu: 0`，并公布期望值时说明所用 CPU 变体；
   若发现指令集确实造成差异，再用 `GGML_BACKEND_PATH` 统一。这条工作量最小；
2. 学生在课程统一的容器中运行 `nq-verify`，助教的期望值也在该容器中生成。这条最可靠。

### 实测二（2026-09-08，同一台机器，ollama 0.33.2，本实验的产物，贪心，128 token）

| 项 | 结果 |
| --- | --- |
| `q4_k_m` 产物重复运行三次 | md5 相同，`82d486311862c3f312562b014cd89acc` |
| 全 Q4_0 产物重复运行两次 | md5 相同，`1abc06d2d92ad0d5390da2ac6556b93a` |
| 四个不同学号 | 四个 md5 两两不同 |
| 两种配方，同一学号 | md5 不同，输出的措辞也不同 |

**助教的工作量。** 期望值每人一份：一次加载 1.10 GB 的模型、生成 128 个 token，
CPU 上约十几秒，200 人约一小时，可以装载一次模型后循环生成。
生成脚本要断言每份输出非空且长度不少于 8 个 token，以排除空输出：
空输出的 md5 对所有人相同，这一层判定将失去意义。

**输入要让输出因人而异。** 对于纯数字的用户消息，模型很可能对所有人给出同一句固定回复，
此时 md5 也对所有人相同。提示词要使学号出现在输出中，措辞定为「把这串数字逐位用中文写出来：
523030910000」。同时这类任务的 logit 间距大，比开放式回答更不容易因数值误差改变结果。

这条措辞已在真实产物上验证。`q4_k_m` 的产物对 523030910000 输出
「五二三零三零九一零零零零」，更换四个学号得到四个两两不同的 md5，
同一条命令重复三次 md5 均相同。发布前仍要按第八节的方法抽取 20 个学号复核。

同一段提示词在全 Q4_0 的产物上输出「5 2 3 0 3 0 9 1 0 0 0 0」：数字识别正确，
但没有执行「用中文写出来」。4.502 与 5.121 位/权重的差别在这条提示词上
直接可见，这一现象已写入习题课讲义。

**这一层的作用。** 它不用于检出实现错误（第一、二层已经检出），
它用于防止抄袭：每人的输入不同，输出不同，md5 不同，前 8 位也不同。
它同时是一个不可伪造的终点：模型或者输出通顺的文本，或者不能。

---

## 五、诊断：六个已知缺陷

助教给出六个能编译、能运行、不崩溃、结果错误的版本，
学生提交第一处分歧、证据链、修复用的 prompt 与最终 diff。

| 编号 | 现象 | 第一处分歧 | 讲义页 |
| --- | --- | --- | --- |
| q1 | 小模型结果正常，`big.safetensors` 的偏移变为一个很小的数 | `rd_u64le` 写成 `p[i] << (8 * i)`，`uint8_t` 提升为 `int` 后移位越界 | `endianness`、`expand-truncate` |
| q2 | 所有权重都偏小，误差约为正确值的 256 倍 | BF16 被当作 FP16 解释 | `precision-formats`、`range-and-precision` |
| q3 | Q4_0 的误差比 Q4_1 大得多，且总有一个编码从不出现 | `d` 取 `absmax / 7`，16 个编码只用到 15 个 | `quantize-code`、`granularity-measured` |
| q4 | 每个权重单独检查误差正常，但整块的顺序错误 | `qs` 的两个半字节对应第 `j` 与第 `j+1` 个权重 | `nibble-packing` |
| q5 | Q4_K 的前四个子块正确，后四个的缩放系数偏大 | `put_scale_min` 写高两位时用 `=` 覆盖了低位 | `k-scales-layout`、`k-scales-code` |
| q6 | `plan` 与 `quant` 都自洽，产物比正确的产物大 255 252 480 B，`nq2gguf` 报张量清单不匹配 | 发现张量表中没有 `lm_head`，就把 `token_embd` 复制一份写为 `output.weight` | 本文第一节的绑定权重、D 部分 |

六个缺陷的定位方法各不相同：q1 只在大偏移下出现，q2 检查数量级即可发现，
q3 需要统计编码分布，q4 只有逐字节比对能够判定，q5 需要打印这 12 个字节，
q6 需要把产物的张量清单与元数据块中的清单排序后用 `comm` 比较。

---

## 六、考核

产物决定分数，面谈决定资格。

**产物（100 分）**，标记为核心系统操作的项合计 60 分：

| 项 | 分 | 核心 |
| --- | --- | --- |
| A 四个转换函数，含 `f32_to_fp16` | 10 | ● |
| A 显式组合，无 `memcpy` 转换，`tiny-be` 被正确拒绝 | 5 | ● |
| B 三种格式的四项测试全部通过（第一层判定） | 20 | ● |
| B `put_scale_min` 穷举互逆 | 10 | ● |
| B 关于 Q4_K 偏移的文字题 | 5 | |
| C `plan` 与 `ls -l` 精确相等，视觉张量正确排除 | 10 | ● |
| C 产物 md5 与期望值相同（第二层判定） | 10 | ● |
| C 峰值内存不超过 256 MiB | 5 | |
| C 四种方案的体积、位宽与速度估算 | 5 | |
| D 生成文本的 md5 前 8 位相符（第三层判定） | 15 | |
| 诊断部分六处定位与证据链 | 5 | |
| 附加：`nq-selftest --full` 下 `f32_to_fp16` 穷举通过 | +5 | |

若第四节两条路径都无法实施，这 15 分改为判定「产物能被 ollama 加载并生成连贯的中文」，
md5 只用于自查，不计分。这一取舍在实验发布前确定。

**面谈（随机抽查 12%）**，每人 5 分钟，包括两项：

1. 当面阅读 12 个字节的 hexdump，说出其中第 5 个子块的 scale；
2. 从该生报告中选取一处追问，例如「你的 `plan` 中 Q6_K 张量的字节数如何计算」，
   或「为什么第 0 层的 `input_layernorm` 上 Q4_1 比 Q4_K 好」。

抽中且无法解释的，核心系统操作 60 分归零，并进入下一个实验的必抽名单。
抽签用全班可验证的公开种子。

**AI 的使用**允许并要求记录：报告中写明哪些部分由 AI 生成、
修改了哪些部分、修改的原因。诊断部分的提交物本身就是 prompt。

**提交物**：`impl/nano_quant.cpp`、可重复运行的测试日志、一份报告、产物张量数据区的 md5、
生成文本的 md5。**不提交模型权重与量化产物。**

**时间预算**：A 3 小时，B 6 小时，C 3 小时，D 3 小时，诊断 3 小时，周期两周。

---

## 七、与 nano-ollama 的衔接

`nano-quant` 是 L1 的实验：

| 环节 | 输入 | 输出 | 在 L1 中的位置 |
| --- | --- | --- | --- |
| `nano-quant` | `.safetensors` | 量化后的张量数据 | 写 |
| `nq2gguf` | `.nq` 与一段元数据 | 可被 llama.cpp 加载的 `.gguf` | 写 |
| L1 的交付 | 本实验中的 `dequantize` | 内存中的 `float` 权重与形状 | 交给 L2 的 `matmul` |

`dequantize` 在本实验中用于误差度量，L2 的 `matmul` 的输入也是它的输出，
一处出错两处均无法通过。D 部分要求量化后的模型真的生成文字，是一个不可伪造的终点。

---

## 八、给助教的实现清单

框架在 [`../nano-quant/`](../nano-quant/)，C++17，无外部依赖。已完成的内容：

| 项 | 位置 |
| --- | --- |
| `nano_quant.h`：全部学生函数的原型与文档注释 | `include/nano_quant.h` |
| 固定的编译选项，`-ffp-contract=off` 与 `-fno-fast-math` | `Makefile` |
| safetensors 头的 JSON 解析与索引，头长度组合由学生的 `rd_u64le` 完成 | `src/st.cpp`、`src/json.cpp` |
| 流式读取：按元素区间 `pread`，峰值内存与张量大小无关 | `src/st.cpp`、`src/main_quant.cpp` |
| Q6_K 的量化与反量化 | `src/q6_k.cpp` |
| 配方判据与名字映射表 | `src/recipe.cpp` |
| Q4_K 子块系数搜索算法的完整书面描述 | `../nano-quant/README.md` |
| `.nq` 容器、`plan` 与 `quant` | `src/nqfile.cpp`、`src/main_quant.cpp` |
| `nq2gguf` 与元数据生成 | `src/main_nq2gguf.cpp`、`tools/mkmeta.py` |
| `nq-verify`：固定 Modelfile 与请求体，`num_gpu` 为 0，`raw` 为 true | `tools/nq-verify` |
| 三个样例 safetensors 的生成脚本 | `tools/mkfixtures.py` |
| 块摘要与样例产物 md5 的期望值 | `tests/expected-blocks.txt`、`tests/expected-tiny-nq.md5` |
| 判定脚本 | `tests/run.sh`、`src/main_selftest.cpp` |
| GGUF 排布检查 | `tools/ggufdump.py` |
| 习题课的讲解，只覆盖 A 部分与 Q4_0 | 本讲的幻灯片，见 [README.md](README.md) |
| `nq-selftest` 的分组运行，完成一部分即可单独判定 | `src/main_selftest.cpp` |

已经核对的五项：

- 三种格式加 Q6_K 在 137 216 个权重上与 llama.cpp 的 `quantize_row_*_ref`
  逐字节相同，样本含全零块、离群值块、量级 1e-7 的块与讲义的两段真实切片；
- `nq_f32_to_fp16` 在全部 2^32 个非 NaN 的 float 上与硬件 `_Float16` 转换一致，
  `nq_fp16_to_f32` 在全部 65 536 个位型上一致；
- 用小样例完成 `plan → quant → nq2gguf` 之后，ollama 内的 llama.cpp 完整加载了产物，
  46 个张量识别为 17 个 F32、24 个 Q4_K、5 个 Q6_K，架构与
  `rope.dimension_sections` 都被接受，没有报错；
- 真实的 Qwen3-VL-2B-Instruct 完成了全流程，两种配方各一次。第二层判定的
  两个 md5 已经确定（见第四节），`plan` 的预测与产物精确相等，
  `ulimit -v 262144` 下 `quant` 与 `nq2gguf` 都能完成且 md5 不变，
  峰值常驻内存 48 MB；
- 两个产物都被 ollama 0.33.2 加载并生成了文字，可复现性与因人而异均已验证，
  数值见第四节的实测二与 `../nano-quant/tests/expected-qwen3vl.txt`。

尚未完成的：

- 每位学生的期望 md5 前 8 位，用于第三层判定；生成脚本要断言输出非空；
- 六个缺陷版本，从完整实现修改生成，每个版本只有一处差异，与完整实现一样不随实验发布；
- 抽签脚本，种子公开；
- 补充测试 CPU 指令集变体的影响：用 `GGML_BACKEND_PATH` 分别指向只含
  `libggml-cpu-haswell.so` 与只含 `libggml-cpu-icelake.so` 的目录，比对两次的 md5。
  这一项决定是否采用第四节的第 2 条路径。

组织部分：

- 习题课讲解之后，A 部分的 15 分与 B 部分中 Q4_0 的部分不再是独立完成的内容，
  第六节的分数表是否调整，在发布前确定。一种方案是把这部分的分值降为 10 分，
  把减少的分值加到 Q4_K 与第二层判定上；
- 校内镜像的分发路径与 sha256；
- 第四节第三层判定的两条路径二选一，在发布前确定；
- 实验说明里写清 Apache-2.0 的条款与「产物不公开分发」。

讲义中已有的、可以直接提供给学生的材料：

| 材料 | 位置 |
| --- | --- |
| `get_scale_min` 与 12 字节打包的自测 | `examples/k_scales.c` |
| 五种方案在真实权重上的误差对照 | `examples/quant_compare.c` |
| 两段真实权重样本与它们的出处 | `examples/ext/`、`examples/make_sample.py` |
| 取样脚本（也是 safetensors 头的最小读法示例） | `examples/make_sample.py` |

讲义的两段样本 `w-down-proj.bf16` 与 `w-final-norm.bf16` 取自 Llama-3.2-1B，
实验更换模型不影响它们：它们只用于对照误差数值。若希望实验与讲义只依赖
Apache-2.0 的权重，可以把两段样本替换为 Qwen 的对应张量，代价是讲义
`granularity-measured`、`q4-1-measured`、`q4-k-measured`、`zero-point` 四页的
实测数字全部需要重新计算。本文不做这个假设。

---

## 九、实验与讲义的对应

| 实验内容 | 讲义页 |
| --- | --- |
| 小端组合与显式宽度 | `endianness`、`read-the-field`、`show-bytes` |
| 整数提升与移位越界 | `expand-truncate`、`shifts` |
| BF16 / FP16 的字段与精度 | `precision-formats`、`bf16-truncation`、`range-and-precision` |
| FP16 的次规格化数与舍入规则 | `fp16-classes`、`float-rounding` |
| 为什么要量化、带宽估算 | `why-quantize`、`memory-bound-measured`、`what-to-quantize` |
| 仿射映射的三个自由度 | `quantization-map`、`granularity`、`granularity-measured` |
| Q4_0 的块与打包 | `q4-block`、`quantize-code`、`nibble-packing` |
| Q4_1 的偏移 | `zero-point`、`q4-1-measured` |
| Q4_K 的超块与两级缩放 | `superblock`、`q4-k-budget`、`k-scales-layout`、`k-scales-code`、`q4-k-measured` |
| 混合配方与体积统计 | `mixed-recipe`、`quantization-cost`、`model-size` |
