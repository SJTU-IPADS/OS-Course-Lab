# ICS 第二讲 · 实验设计：`nano-quant`

本讲第五、第六部分讲量化。这个实验把讲过的三种格式落到一个真实模型上：
学生从 Hugging Face 取 `Qwen/Qwen3-VL-2B-Instruct`，自己读它的 safetensors 文件，
把其中 310 个语言模型张量量化成 Q4_0、Q4_1、Q4_K，度量误差，最后让量化后的模型
以自己的学号为输入生成一段文字，提交这段文字的 md5。

与 [LAB.md](LAB.md) 的 `gguf-lens` 的分工：`gguf-lens` 读别人量化好的文件并说明它的
大小，`nano-quant` 生成那样一个文件。前者是观察型，后者是构造型；两者共用同一份
`nano.h` 与同一套字节账目的验收方式，可以合并为一个两周的实验，也可以分开布置。

框架代码、参考实现与测试都已经写好，在 [`nano-quant/`](nano-quant/) 下，
完成情况见第八节。习题课上带学生完成的部分另有一份讲义
[`EXERCISE-q4_0.md`](EXERCISE-q4_0.md)，与本文一同发给学生。

---

## 〇、这一版改了什么

前一版用 `meta-llama/Llama-3.2-1B-Instruct`。换成 `Qwen/Qwen3-VL-2B-Instruct`
消除了门控仓库的排期风险，也换来三处代价与一处必须更正的判定方式，都写在下面：

| 项 | 结论 |
| --- | --- |
| 许可 | Apache-2.0，无门控，无附加使用条款。已核实，见第一节 |
| 下载量 | 4.25 GB，比原来多 1.78 GB，其中 0.81 GB 是本实验不用的视觉塔 |
| 张量筛选 | 文件里 625 个张量，只有 310 个进入产物，筛选本身成为 C 部分的一项判定 |
| q / k 重排 | Qwen 不需要，llama.cpp 的转换脚本对 Qwen3 系列不做旋转编码重排。原 D 部分的这项练习消失，换成张量筛选与绑定权重两项 |
| 种子与温度 | `temperature = 0` 时采样退化为取最大值，随机数发生器不被调用，种子不起作用。输出可复现来自贪心解码与固定的运行环境，不来自种子。实测见第四节 |

框架已经实现，在 [`nano-quant/`](nano-quant/) 下，用 C++17 写成。
第八节记着它的完成情况与剩下的事。

---

## 一、实验对象

| 文件 | 大小 | 用途 |
| --- | --- | --- |
| `model.safetensors` | 4 255 140 312 B | 主对象。`Qwen/Qwen3-VL-2B-Instruct`，625 个张量，全部 BF16 |
| `tiny.safetensors` | 数 KB | 调试。助教用脚本生成，张量少、可整篇打印 |
| `big.safetensors` | 稀疏文件，实占数 KB | 规模验证。头里声明的偏移超过 4 GiB，用 `truncate` 造出 |
| `tiny-be.safetensors` | 数十字节 | 反例。头长度按大端写入，工具必须拒绝并说明理由 |

后两个文件是这个实验里唯二不需要真实权重的对象，作用是把两类错误从「碰巧没触发」
变成「必然触发」。这里有一处新情况：真实文件的最大偏移是 4 255 064 064，
即 `0xFD9F2000`，大于 2^31 而小于 2^32。它已经能让用 `int` 存偏移的写法溢出成负数，
但不能让 `p[i] << (8 * i)` 的移位越界暴露出来，后者仍然只有 `big.safetensors` 能触发。
两个文件都要保留，理由写进实验说明。

### 许可核实

2026-09-07 查得，`Qwen/Qwen3-VL-2B-Instruct` 的仓库元数据里 `gated` 为 false，
模型卡的 front matter 写 `license: apache-2.0`，仓库内没有 LICENSE 文件，
README 全文没有第二处许可相关表述，没有附加使用条款，没有可接受使用政策。

Apache-2.0 允许使用、修改与再分发，条件是保留许可与版权声明，并在派生物里
说明改动。对本实验的含义：

- 学生下载与量化无需申请，不存在审核排期；
- 量化产物是派生物，若要公开分发，需附 Apache-2.0 全文与「量化自
  `Qwen/Qwen3-VL-2B-Instruct`」的说明。**实验仍规定产物不公开分发**，只在课程环境内提交，
  理由是省去逐份检查许可声明的工作，不是许可本身不允许；
- 与前一版的 Llama 3.2 Community License 相比，少了「派生模型名必须以 Llama 开头」
  与「说明里注明 Built with Llama」两条命名要求。

### 模型的获取

无门控，`https://huggingface.co/Qwen/Qwen3-VL-2B-Instruct/resolve/main/model.safetensors`
可直接下载。仍然给两条路径：

1. 助教在校内文件服务上放一份，公布 sha256。4.25 GB × 全班的外网流量不现实，
   这是主路径；
2. 学生自行从 Hugging Face 或其镜像下载，用公布的 sha256 校验。

排期上不再需要提前两周，这是换模型的主要收益。

### 文件的账目

```
[0, 8)                          8 字节，小端 uint64，JSON 头的长度 N = 76 240
[8, 76 248)                     JSON 头，每个张量一条
[76 248, 4 255 140 312)         张量数据，长 4 255 064 064
```

`8 + 76 240 + 4 255 064 064 = 4 255 140 312`，与 `ls -l` 一致。625 个张量在数据区里
首尾相接，没有对齐填充，也没有空洞——这一点由学生自己验证，不由助教告知。

625 个张量按名字前缀分成两部分：

| 前缀 | 张量数 | 参数量 | 字节数 | 本实验 |
| --- | --- | --- | --- | --- |
| `model.language_model.` | 310 | 1 720 574 976 | 3 441 149 952 | 全部处理 |
| `model.visual.` | 315 | 406 957 056 | 813 914 112 | 全部跳过 |

310 = 28 层 × 11 + `embed_tokens` + `norm`。每层的 11 个是
`self_attn` 的 `q_proj`、`k_proj`、`v_proj`、`o_proj`、`q_norm`、`k_norm`，
`mlp` 的 `gate_proj`、`up_proj`、`down_proj`，以及两个 layernorm。

这个模型的 `tie_word_embeddings` 为 true，文件里**没有** `lm_head.weight`：
输出头与输入词嵌入是同一个矩阵。

最大的张量是 `model.language_model.embed_tokens.weight`，形状 151936 × 2048，
单独占 622 329 856 B。

### 磁盘与内存

输入 4.25 GB，产物约 1.10 GB，中间不落盘，准备 6 GB 空间即可。

内存有硬约束：**峰值不超过 256 MiB**，用 `ulimit -v 262144` 运行测试来判定。
最大的张量单独就有 622 MB，一次读进内存必然超限；`ulimit -v` 限制的是地址空间，
因此把整个文件 `mmap` 进来同样超限。学生只能按行或按块 `pread`。
这条约束是这个实验里成本最低、收益最高的一条：它把「文件比内存大」从一句话
变成一个会失败的测试。

---

## 二、safetensors 值不值得手写

值得，而且比 GGUF 简单。整个格式只有三段，第一节已经给出。
唯一的麻烦是 JSON。手写 JSON 解析器不是这门课的内容，因此**头的解析由助教提供**，
接口是：

```c
int st_open(st_file *f, const char *path);          /* 读头长度、解析 JSON、建索引 */
int st_count(const st_file *f);
const st_tensor *st_get(const st_file *f, int i);   /* name, dtype, ndim, shape[], off, len */
size_t st_read(const st_file *f, const st_tensor *t, size_t elem_off,
               uint16_t *dst, size_t n_elem);       /* 从张量内的第 elem_off 个元素起读 n 个 */
```

`st_open` 的实现里留一个洞：头长度的合成调用学生写的 `rd_u64le`，
校验也在学生的代码里。框架负责 JSON，学生负责字节。

safetensors 的二维张量按行主序存放，形状写成 `[输出维, 输入维]`；
GGUF 的同一个张量记成 `ne = [输入维, 输出维]`，两者的字节序列相同。
因此按行量化就是按 GGUF 的行量化，全程不需要转置。这一点要写进实验说明，
否则学生会去找一个并不存在的转置步骤。

---

## 三、学生写什么

框架已经写好，在 [`nano-quant/`](nano-quant/) 下，说明见
[`nano-quant/README.md`](nano-quant/README.md)。一个头文件
`nano_quant.h` 由助教在第一周发布，之后只增不改。学生交一份
`student.cpp`，实现其中的全部函数；其余部分（命令行、遍历、输出、测试）由框架提供。

编译选项由框架固定，其中 `-ffp-contract=off` 是硬性的：
允许编译器把乘加合成 FMA 会改变舍入结果，第四节的逐字节判定就不再跨机器成立。

```
c++ -std=c++17 -O2 -ffp-contract=off -fno-fast-math ...
```

框架另外提供一个 `nq_round`：加上 2^23 + 2^22 再取尾数低位的那个惯用法，
就近舍入、平局取偶。三种格式的取整一律用它。换成 `(int)(x + 0.5f)`
在负数处与平局处都会不同，逐字节判定过不去。

### A 读字节（约 3 小时）

```c
uint64_t rd_u64le(const uint8_t *p);      /* 8 字节，小端，显式合成 */
float    bf16_to_f32(uint16_t h);         /* BF16 是 FP32 的高 16 位 */
float    fp16_to_f32(uint16_t h);         /* 按 1-5-10 的字段拼 */
uint16_t f32_to_fp16(float f);            /* 就近舍入到偶数 */
```

四个函数各配一项可判定的测试：

1. `rd_u64le` 在助教给定的字节向量上逐条相等，其中包含
   `ff ff ff ff ff ff ff ff` 与只有第 7 字节非零的两条。
   **禁止 `memcpy` 到 `uint64_t`**，必须写成移位与或。
   常见错误是写 `p[i] << (8 * i)`：`uint8_t` 提升为 `int`，`i >= 4` 时移位越界，
   结果在小文件上完全正确。`big.safetensors` 的偏移超过 4 GiB，专治这一条。
2. `bf16_to_f32` 与助教的参考值逐位相等，覆盖 0、±inf、NaN、次正规数各一例。
3. `fp16_to_f32` 在全部 65 536 个 fp16 位模式上与 `_Float16` 的转换一致。
4. `f32_to_fp16` 与助教的参考实现比对，NaN 只比对是否为 NaN，另外要求
   半精度能精确表示的每一个值原样往返。默认按质数步长抽样 1700 万个位模式，
   `nq-selftest --full` 遍历全部 2^32 个，约一分钟。

第 4 项的难点是就近舍入到偶数、次正规数与上溢到 inf，本讲第四部分讲过。
它不是附加项：三种格式的缩放系数都用它写出，它错了整个产物都对不上。
助教的 `nq_f32_to_fp16` 已在全部 2^32 个非 NaN 的 float 上与硬件的
`_Float16` 转换核对过，可以放心当作参考。

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

`get_scale_min` 讲义上给过（`k-scales-code` 页），实验里给的也是它。
**`put_scale_min` 由学生自己写**：读法确定了写法，这是这一部分设计上的关键一步——
学生手里有解码器，要造出与之互逆的编码器。判定是穷举意义上的：
64 × 64 组取值、每组 8 个位置，`put` 之后 `get` 回来必须一个不差
（讲义的 `examples/k_scales.c` 是同一个测试的随机版本，可以直接给学生当作自测）。

Q4_K 每个子块的步长与偏移用一个迭代搜索定出来，讲义没有讲这个搜索。
**这个搜索的完整算法写进实验说明**，与 llama.cpp 的 `make_qkx2_quants` 等价，
包括迭代次数、初值、比较的方向与「偏移取正时压回 0」这一句。
书面描述在 [`nano-quant/README.md`](nano-quant/README.md) 的「Q4_K 子块的搜索」一节。
理由见第四节：第二层判定要求产物与参考实现逐字节相同，搜索差一步就不可达。
搜索算法本身不计分，照着写即可；计分的是格式的布局与打包。

其余判定：

| 项 | 判定 | 治什么错 |
| --- | --- | --- |
| 误差上界 | 每个权重 `|w − dequant(quant(w))| <= |d|`，未落在极值编码上的满足 `<= |d|/2` | 缩放系数取错方向 |
| 幂等 | quantize → dequantize → quantize 后码字不变 | 舍入写成截断 |
| 逐字节比对 | 对助教给定的一组权重，18 / 20 / 144 字节与参考块完全相同 | 见下 |
| 相对误差 | 在 `w-down-proj.bf16` 上，三种格式的相对 RMSE 与讲义表格相差不超过 0.01 个百分点 | 用了别的极值定义 |

逐字节比对锁住四个只影响布局、不影响单点误差的细节，前三项测试对它们全部无效：

- Q4_0 的 `d` 取 `extreme(x) / -8` 而非 `absmax / 7`；
- Q4_0 / Q4_1 的 `qs[j]` 配的是第 `j` 与第 `j+16` 个权重；
- Q4_K 的 `qs` 以 64 个权重为一组，第 `e` 与第 `e+32` 个权重共用一个字节；
- Q4_K 的偏移取 `min(0, lo)`，即偏移不取正值。

最后一条会让全为正数的张量上的 Q4_K 明显差于 Q4_1。这个模型里
`model.language_model.layers.0.input_layernorm.weight` 的 2048 个权重全为正，
取值在 0.0537 与 1.1016 之间，是现成的例子。写对了才会看到这个现象，因此
**要求学生在报告里解释这一行数据**，这是 B 部分唯一一道文字题。

### C 全模型量化与账目（约 3 小时）

```
$ ./nano-quant plan  model.safetensors --recipe q4_k_m
$ ./nano-quant quant model.safetensors --recipe q4_k_m -o qwen3vl-2b-q4km.nq
```

`plan` 只读头，不读权重，输出每个张量的类型、元素数、量化后字节数与累计字节数；
`quant` 真正执行。硬性验收条件与 `gguf-lens` 相同：
**`plan` 预测的总字节数与 `quant` 产物的 `ls -l` 完全相等**。

这一版新增一条：`plan` 必须把 315 个 `model.visual.` 张量排除在外，
并在末尾单独报出被跳过的张量数与字节数。跳过的判据由学生自己写，不给现成的表。
清单走标准输出，汇总走标准错误，于是账目可以直接接给别的程序。

配方由助教给出，是一张「张量名模式 → 格式」的表，学生只实现按表分派：

| 张量 | 格式 | 位/权重 | 张量数 |
| --- | --- | --- | --- |
| 一维张量（各 norm，含 `q_norm` / `k_norm`） | F32 | 32 | 113 |
| `embed_tokens.weight` | Q6_K | 6.5625 | 1 |
| 14 个指定层的 `v_proj` 与 `down_proj` | Q6_K | 6.5625 | 28 |
| 其余二维张量 | Q4_K | 4.5 | 168 |

「14 个指定层」是 llama.cpp 对 Q4_K_M 用的挑法，层号
0、1、2、5、8、11、14、17、20、23、24、25、26、27，即前 1/8、后 1/8 与其间每三层取一层。
挑法由助教给出，不要求学生推导。`embed_tokens` 取 Q6_K 的理由是它同时充当输出头。

按这张表算出的账目：

| 方案 | 字节数 | 位/权重 |
| --- | --- | --- |
| 全 Q4_0 | 968 249 344 | 4.502 |
| 全 Q4_K | 968 249 344 | 4.502 |
| 全 Q4_1 | 1 075 777 536 | 5.002 |
| `q4_k_m` 配方 | 1 101 457 408 | 5.121 |

全 Q4_0 与全 Q4_K 的字节数完全相同，两者都是每权重 4.5 位；讲义第 61 页算过这件事。
BF16 的 3 441 149 952 B 对 `q4_k_m` 的 1 101 457 408 B，压缩比 3.12。

这四个数由本文的账目脚本算出，已经与参考实现在真实模型上的输出核对：
`q4_0` 与 `q4_k_m` 两个配方的 `plan` 预测值与 `quant` 产物数据区精确相等。

与已公开的 `unsloth/Qwen3-VL-2B-Instruct-GGUF` 的对照也做了。它的 Q4_K_M 是
1 107 410 624 B，本框架的同名产物是 1 107 409 760 B，两者差 864 字节。
两份元数据段的字符串长度不同，这个量级的差与之相称。前一版本文里写的
247 552 B 缺口来自对元数据大小的估算，不来自配方表。

三条要求：

1. `plan` 的输出可以直接交给 `awk` 求和，一行一个张量，字段用空格分隔，
   最后一个字段是字节数：`nano-quant plan m.safetensors | awk '{s += $NF} END {print s}'`；
2. 报告里给出四种方案的体积与加权平均位宽，并用讲义 `why-quantize` 页的
   带宽数据估算各自的生成速度；
3. 全程峰值内存不超过 256 MiB。

### D 让它跑起来（约 3 小时）

产物要能被真实推理程序加载。完整的 GGUF 写入涉及分词器等与本讲无关的元数据，
因此拆成两步：助教提供 `nq2gguf`，把一份预先备好的元数据块与学生的张量数据区拼成
一个合法的 `.gguf`；学生负责让张量数据区的内容与顺序完全符合它的预期。

```
$ python3 tools/mkmeta.py ~/models/Qwen3-VL-2B-Instruct -o meta.kv
$ ./nq2gguf qwen3vl-2b-q4km.nq --meta meta.kv -o qwen3vl-2b-q4km.gguf
$ tools/nq-verify qwen3vl-2b-q4km.gguf 523030910000
```

元数据块由 `tools/mkmeta.py` 从 Hugging Face 仓库里的 `config.json` 与
`tokenizer.json` 直接生成，架构是 `qwen3vl`，
其中三个键与文本无关却必须写对：`qwen3vl.rope.dimension_sections` 为 `[24, 20, 20, 0]`，
`qwen3vl.n_deepstack_layers` 为 3，`qwen3vl.rope.freq_base` 为 5 000 000。
前两个来自视觉塔的配置，纯文本输入时它们参与的那几步加的是 0。

学生负责三件框架不替他们做的事：

- **名字映射**：`model.language_model.layers.0.self_attn.q_proj.weight` 对应
  `blk.0.attn_q.weight`，映射表由助教给出（见
  [`nano-quant/README.md`](nano-quant/README.md) 的「名字映射」一节），
  另外要从张量名里取出层号；写出顺序就是 safetensors 头里的顺序，
  顺序错了文件仍然合法、仍能加载、数据区的 md5 却对不上；
- **张量筛选**：315 个视觉张量一个都不能写进去，多写一个就与元数据块里的清单对不上；
- **绑定权重**：这个模型没有 `lm_head.weight`，产物里也**不能**有 `output.weight`。
  llama.cpp 读不到它时会把 `token_embd.weight` 再用一次。自己补一个进去，
  产物会多出 255 252 480 B，`nq2gguf` 的张量清单对不上并报错。

前一版在这一步让学生自己发现 q / k 的旋转编码重排。Qwen3 系列不需要这个重排，
llama.cpp 的转换脚本对它们不做重排，这项练习在这一版里没有了。
上面第二、第三条是它的替代：判定同样不可伪造，定位同样要从现象倒推。

---

## 四、判定：三层

三层判定从可复现性最强的一层排到最弱的一层。**每一层通过是下一层通过的前提**，
因此哪一层先挂就指出了错在哪个阶段。

### 第一层：块级逐字节（与机器无关）

助教给一组固定的 256 个权重，学生的三种格式各产出 18 / 20 / 144 字节，
与参考块逐字节相同。这一层只用整数与四则运算，钉死 `-ffp-contract=off` 后
在任何机器、任何编译器上结果相同。

### 第二层：产物 md5（与机器无关，全班同一个值）

`quant` 产物的张量数据区的 md5 与助教公布的参考值相同。
`q4_k_m` 配方是 `6b7031b4ff51880111226bca94d681da`，全 Q4_0 是
`fa4f64a2e2be8d8bdb7dad8c0ed06e27`，两者的字节数分别是 1 101 457 408 与
968 249 344。参考值与生成条件记在 `nano-quant/tests/reference-qwen3vl.txt`。
这一层覆盖全部 310 个张量、配方分派、跳过视觉张量、写出顺序，
是这个实验里最强的一项判定。它成立的前提是学生的量化器与参考实现**逐字节相同**，
所以第三节 B 部分要把 Q4_K 的搜索算法完整写进实验说明。

### 第三层：生成文本的 md5（每人一个值，需要固定后端与版本）

学生以自己的学号为输入跑一次生成，提交输出文本的 md5。
助教为每位同学公布该 md5 的**前 8 位**，学生据此自查。

输入是一段固定的字节，其中只有学号因人而异。Qwen3-VL 的对话模板在没有系统消息
且没有工具时只产生下面三行，提示词照抄即可（措辞的理由见本节末尾）：

```
<|im_start|>user
把这串数字逐位用中文写出来：523030910000<|im_end|>
<|im_start|>assistant
```

运行由助教提供的 `nq-verify` 完成。用 ollama 就够了，学生不必自己编译 llama.cpp：

```
ollama create nq-<学号> -f Modelfile      # Modelfile 只有一行 FROM ./qwen3vl-2b-q4km.gguf
curl -s http://localhost:11434/api/generate -d @req.json | jq -r .response | md5sum
```

`req.json` 里 `"raw": true`，提示词就是上面那三行的字节，模板不参与；
`options` 固定为 `{"temperature": 0, "num_predict": 128, "num_gpu": 0, "num_ctx": 4096}`。
`"raw": true` 不能省：ollama 会按 GGUF 里的模板拼提示词，模板随版本变，raw 模式绕开它。

**关于种子的更正。** `temperature = 0` 让采样退化为取最大 logit，随机数发生器不被调用，
种子取 2026 还是别的值都不影响输出。写上 `"seed": 2026` 只是把参数记录下来，
它不是可复现性的来源。可复现性来自贪心解码与固定的数值流程。

### 实测一（2026-09-07，i9-11900H + RTX 3060，ollama 0.33.2，llama3.2，贪心，128 token）

| 改动的量 | 输出的 md5 |
| --- | --- |
| 同一条命令重复跑 | 相同 |
| `seed` 取 2026 / 1 / 99999 | 相同 |
| `num_thread` 取 1 / 2 / 4 / 8 / 16 | 相同 |
| `num_ctx` 取 4096 / 2048 | 相同 |
| `num_batch` 取 512 / 128 | 相同 |
| `num_gpu` 取 0 / 99（CPU 与 GPU） | 一段提示词下**不同**，换一段提示词后相同 |

三点结论：

1. **线程数不影响结果。** ggml 按行切分矩阵乘，一个输出元素的整个点积在同一个线程里
   按同一个顺序累加，线程数变了累加顺序不变。原先「线程数会换掉 token」的说法不成立。
2. **同一后端下没测出任何差异。** 固定模型文件、固定 ollama 版本、固定 `num_gpu: 0`，
   重复跑就是逐字节相同。这一层在单一环境里是可以保证的。
3. **换后端是「不保证相同」，不是「一定不同」。** 同一台机器、同一个模型，
   只把提示词换掉，CPU 与 GPU 的输出一次不同、一次相同。原因是贪心解码只在
   前两名 logit 的间距小于数值误差时才会翻转，翻不翻取决于具体的输入。
   对判分来说，「大部分人对、少数人无缘无故不对」与「都不对」一样难处理。

因此这一层的判定条件写成：**`num_gpu` 必须为 0，ollama 版本由助教指定。**
钉住这两项之后，剩下的唯一变量是 CPU 指令集：ollama 随包发 14 个
`libggml-cpu-*.so`，按 CPU 特性挑一个，AVX2 与 AVX-512 的点积累加宽度不同。
**这一项本文没有实测**，因为系统的模型目录属 root，起不了第二个服务端来强制变体。
需要时的办法是现成的：ollama 认 `GGML_BACKEND_PATH`，把单个变体放进一个目录再指过去，
全班就落在同一个内核上。

两条落地路径，二选一，在发布前定：

1. 助教指定 ollama 版本、`num_gpu: 0`，并公布参考值时说明所用 CPU 变体；
   若发现指令集确实造成差异，再用 `GGML_BACKEND_PATH` 统一。这条最省事；
2. 学生在课程统一的容器里跑 `nq-verify`，助教的参考值也在那里生成。这条最稳。

### 实测二（2026-09-08，同一台机器，ollama 0.33.2，本实验的产物，贪心，128 token）

| 项 | 结果 |
| --- | --- |
| `q4_k_m` 产物重复跑三次 | md5 相同，`82d486311862c3f312562b014cd89acc` |
| 全 Q4_0 产物重复跑两次 | md5 相同，`1abc06d2d92ad0d5390da2ac6556b93a` |
| 四个不同学号 | 四个 md5 两两不同 |
| 两种配方，同一学号 | md5 不同，输出的措辞也不同 |

**助教侧的工作量。** 参考值要一人一份：一次加载 1.10 GB 的模型、生成 128 个 token，
CPU 上约十几秒，200 人约一小时，可以一次装载模型循环跑完。
生成脚本要断言每份输出非空且长度不少于 8 个 token，避免出现空输出——
空输出的 md5 人人相同，那一层就失去了意义。

**输入要让输出因人而异。** 一个纯数字的用户消息，模型很可能对所有人回同一句套话，
那样 md5 也人人相同。提示词要把学号逼进输出里，措辞定为「把这串数字逐位用中文写出来：
523030910000」。同时这类任务的 logit 间距大，比开放式回答更不容易在边界上翻转。

这条措辞在真实产物上验过。`q4_k_m` 的产物对 523030910000 回
「五二三零三零九一零零零零」，换四个学号得到四个两两不同的 md5，
同一条命令重复三次 md5 逐次相同。发布前仍要按第八节的办法抽 20 个学号复核一遍。

同一段提示词在全 Q4_0 的产物上回「5 2 3 0 3 0 9 1 0 0 0 0」：数字认对了，
「用中文写出来」这一句没有照做。4.502 与 5.121 位/权重的差别在这条提示词上
直接可见，这个现象写进了习题课讲义。

**这一层能挡住什么。** 它挡不住实现错误（第一、二层已经挡住了），
它挡住的是抄袭：每人的输入不同，输出不同，md5 不同，前 8 位也不同。
它同时是一个不可伪造的终点：模型说人话，或者不说。

---

## 五、诊断：六个已知缺陷

与 `gguf-lens` 的 C 部分同样的形式，给六个能编译、能运行、不崩溃、结果错误的版本，
学生提交第一处分歧、证据链、修复用的 prompt 与最终 diff。

| 编号 | 表现 | 第一处分歧 | 讲义页 |
| --- | --- | --- | --- |
| q1 | 小模型一切正常，`big.safetensors` 的偏移变成一个小数 | `rd_u64le` 写成 `p[i] << (8 * i)`，`uint8_t` 提升为 `int` 后移位越界 | `endianness`、`expand-truncate` |
| q2 | 所有权重都偏小，误差约为正确值的 256 倍 | BF16 当成 FP16 解释 | `precision-formats`、`range-and-precision` |
| q3 | Q4_0 的误差比 Q4_1 大得多，且总有一个编码从不出现 | `d` 取 `absmax / 7`，16 个编码只用了 15 个 | `quantize-code`、`granularity-measured` |
| q4 | 每个权重单独看误差正常，整块的顺序是错的 | `qs` 的两个半字节配成第 `j` 与第 `j+1` 个权重 | `nibble-packing` |
| q5 | Q4_K 的前四个子块正确，后四个的缩放系数偏大 | `put_scale_min` 写高两位时用 `=` 覆盖而非 `|=` 合并 | `k-scales-layout`、`k-scales-code` |
| q6 | `plan` 与 `quant` 都自洽，产物比参考大 255 252 480 B，`nq2gguf` 报张量清单不匹配 | 见到词表里没有 `lm_head`，就把 `token_embd` 复制一份写成 `output.weight` | 本文第一节的绑定权重、D 部分 |

六个缺陷的定位手段各不相同：q1 只在大偏移下出现，q2 看一眼数量级就能发现，
q3 要统计编码分布，q4 只有逐字节比对能判定，q5 要打印那 12 个字节，
q6 要把产物的张量清单与元数据块里的清单排序后 `comm`。

---

## 六、考核

沿用 `gguf-lens` 的办法：产物给出分数，面谈给出门槛。

**产物（100 分）**，标记为核心系统操作的项合计 60 分：

| 项 | 分 | 核心 |
| --- | --- | --- |
| A 四个转换函数，含 `f32_to_fp16` | 10 | ● |
| A 显式合成，无 `memcpy` 转换，`tiny-be` 被正确拒绝 | 5 | ● |
| B 三种格式的四项测试全过（第一层判定） | 20 | ● |
| B `put_scale_min` 穷举互逆 | 10 | ● |
| B 关于 Q4_K 偏移的那道文字题 | 5 | |
| C `plan` 与 `ls -l` 精确相等，视觉张量正确排除 | 10 | ● |
| C 产物 md5 与参考相同（第二层判定） | 10 | ● |
| C 峰值内存不超过 256 MiB | 5 | |
| C 四种方案的体积、位宽与速度估算 | 5 | |
| D 生成文本的 md5 前 8 位相符（第三层判定） | 15 | |
| 诊断部分六处定位与证据链 | 5 | |
| 附加：`nq-selftest --full` 下 `f32_to_fp16` 穷举通过 | +5 | |

若第四节两条路径都落实不了，这 15 分改判「产物能被 ollama 加载并生成连贯的中文」，
md5 只作自查不计分。这个取舍在实验发布前定下来，不在批改时定。

**面谈（随机抽查 12%）**，每人 5 分钟，两件事：

1. 当面读 12 个字节的 hexdump，说出其中第 5 个子块的 scale 是多少；
2. 从该生报告里挑一条追问，例如「你的 `plan` 里 Q6_K 那几个张量的字节数是怎么算的」，
   或「为什么第 0 层的 `input_layernorm` 上 Q4_1 比 Q4_K 好」。

抽中且无法解释的，核心系统操作 60 分归零，并进入下一个实验的必抽名单。
抽签用全班可验证的公开种子。

**AI 的使用**允许并要求记录，规则与 `gguf-lens` 一致：报告里写明哪些部分由 AI 生成、
改了它哪里、为什么改。诊断部分的提交物本身就是 prompt。

**提交物**：`student.cpp`、可重跑的测试日志、一份报告、产物张量数据区的 md5、
生成文本的 md5。**不提交模型权重与量化产物。**

**时间预算**：A 3 小时，B 6 小时，C 3 小时，D 3 小时，诊断 3 小时，周期两周。

---

## 七、与 nano-ollama 的衔接

`nano-quant` 与 `gguf-lens` 同属 L1，方向相反，接口相同：

| 工具 | 输入 | 输出 | 在 L1 里的位置 |
| --- | --- | --- | --- |
| `gguf-lens` | `.gguf` | 元数据、张量清单、字节账目 | 读 |
| `nano-quant` | `.safetensors` | 量化后的张量数据 | 写 |
| L1 的交付 | 上面两者共用的 `dequantize` | 内存里的 `float` 权重与形状 | 交给 L2 的 `matmul` |

两个工具共用同一份 `dequantize`：`gguf-lens` 用它解开别人的文件，`nano-quant` 用它做
误差度量，L2 的 `matmul` 吃的也是它的输出。一处写错，三处都不过。

如果课时只够一个实验，建议保留 `nano-quant` 而把 `gguf-lens` 缩成它的 A 部分：
`nano-quant` 覆盖的讲义页更多（多出第五、第六两部分），并且有 D 部分这个
不可伪造的终点。

---

## 八、给助教的实现清单

框架在 [`nano-quant/`](nano-quant/)，C++17，无外部依赖。已经完成的：

| 项 | 位置 |
| --- | --- |
| `nano_quant.h`：全部学生函数的原型与文档注释 | `include/nano_quant.h` |
| 固定的编译选项，`-ffp-contract=off` 与 `-fno-fast-math` | `Makefile` |
| safetensors 头的 JSON 解析与索引，头长度合成留给学生的 `rd_u64le` | `src/st.cpp`、`src/json.cpp` |
| 流式读取：按元素区间 `pread`，峰值内存与张量大小无关 | `src/st.cpp`、`src/main_quant.cpp` |
| Q6_K 的量化与反量化 | `src/q6_k.cpp` |
| 配方判据与名字映射表 | `src/recipe.cpp` |
| Q4_K 子块系数搜索算法的完整书面描述 | `nano-quant/README.md` |
| `.nq` 容器、`plan` 与 `quant` | `src/nqfile.cpp`、`src/main_quant.cpp` |
| `nq2gguf` 与元数据生成 | `src/main_nq2gguf.cpp`、`tools/mkmeta.py` |
| `nq-verify`：固定 Modelfile 与请求体，`num_gpu` 为 0，`raw` 为 true | `tools/nq-verify` |
| 助教参考实现 | `ref/reference.cpp` |
| 三个样例 safetensors 的生成脚本 | `tools/mkfixtures.py` |
| 参考块摘要与参考产物 md5 | `tests/reference-blocks.txt`、`tests/reference-tiny-nq.md5` |
| 判定脚本 | `tests/run.sh`、`src/main_selftest.cpp` |
| GGUF 排布检查 | `tools/ggufdump.py` |
| 习题课讲义，只覆盖 A 部分与 Q4_0 | [`EXERCISE-q4_0.md`](EXERCISE-q4_0.md) |
| `nq-selftest` 的分组运行，写完一部分就能单独判定 | `src/main_selftest.cpp` |

已经核对过的五件事：

- 三种格式加 Q6_K 在 137 216 个权重上与 llama.cpp 的 `quantize_row_*_ref`
  逐字节相同，样本含全零块、离群值块、量级 1e-7 的块与讲义的两段真实切片；
- `nq_f32_to_fp16` 在全部 2^32 个非 NaN 的 float 上与硬件 `_Float16` 转换一致，
  `nq_fp16_to_f32` 在全部 65 536 个位型上一致；
- 用小样例走完 `plan → quant → nq2gguf` 之后，ollama 内的 llama.cpp 完整加载了产物，
  46 个张量识别为 17 个 F32、24 个 Q4_K、5 个 Q6_K，架构与
  `rope.dimension_sections` 都被接受，没有报错；
- 真实的 Qwen3-VL-2B-Instruct 走完了全程，两种配方各一次。第二层判定的
  两个 md5 已经定下（见第四节），`plan` 的预测与产物精确相等，
  `ulimit -v 262144` 下 `quant` 与 `nq2gguf` 都跑得完且 md5 不变，
  峰值常驻内存 48 MB；
- 两个产物都被 ollama 0.33.2 加载并生成了文字，可复现性与因人而异都验过，
  数值见第四节的实测二与 `nano-quant/tests/reference-qwen3vl.txt`。

还没做的：

- 每位学生的参考 md5 前 8 位，用于第三层判定；生成脚本要断言输出非空；
- 六个缺陷版本，从 `ref/reference.cpp` 打补丁生成，每个版本只差一处；
- 抽签脚本，种子公开；
- 补测 CPU 指令集变体的影响：用 `GGML_BACKEND_PATH` 分别指向只含
  `libggml-cpu-haswell.so` 与只含 `libggml-cpu-icelake.so` 的目录，比对两次的 md5。
  这一项决定要不要走第四节的第 2 条路径。

组织部分：

- 习题课讲下来之后，A 部分的 15 分与 B 部分里 Q4_0 的那一份不再是独立完成的内容，
  第六节的分数表要不要重排，在发布前定。一种改法是把这部分的分值压到 10 分，
  把腾出来的分加到 Q4_K 与第二层判定上；
- 校内镜像的分发路径与 sha256；
- 第四节第三层判定的两条路径二选一，在发布前定下来；
- 实验说明里写清 Apache-2.0 的条款与「产物不公开分发」。

讲义里已有的、可以直接给学生的材料：

| 材料 | 位置 |
| --- | --- |
| `get_scale_min` 与 12 字节打包的自测 | `examples/k_scales.c` |
| 五种方案在真实权重上的误差对照 | `examples/quant_compare.c` |
| 两段真实权重样本与它们的出处 | `examples/ext/`、`examples/make_sample.py` |
| 取样脚本（也是 safetensors 头的最小读法示例） | `examples/make_sample.py` |

讲义的两段样本 `w-down-proj.bf16` 与 `w-final-norm.bf16` 取自 Llama-3.2-1B，
实验换模型不影响它们：它们只用来对照误差数值。若希望实验与讲义只依赖
Apache-2.0 的权重，可以把两段样本换成 Qwen 的对应张量，代价是讲义
`granularity-measured`、`q4-1-measured`、`q4-k-measured`、`zero-point` 四页的
实测数字全部要重算。本文不做这个假设。

---

## 九、实验与讲义的对应

| 实验内容 | 讲义页 |
| --- | --- |
| 小端合成与显式宽度 | `endianness`、`read-the-field`、`show-bytes` |
| 整数提升与移位越界 | `expand-truncate`、`shifts` |
| BF16 / FP16 的字段与精度 | `precision-formats`、`range-and-precision`、`bf16-truncation` |
| 为什么要量化、带宽估算 | `why-quantize`、`memory-bound-measured`、`what-to-quantize` |
| 仿射映射的三个自由度 | `quantization-map`、`granularity`、`granularity-measured` |
| Q4_0 的块与打包 | `q4-block`、`quantize-code`、`nibble-packing` |
| Q4_1 的偏移 | `zero-point`、`q4-1-measured` |
| Q4_K 的超块与两级缩放 | `superblock`、`q4-k-budget`、`k-scales-layout`、`k-scales-code`、`q4-k-measured` |
| 混合配方与体积账目 | `mixed-recipe`、`quantization-cost`、`model-size` |
