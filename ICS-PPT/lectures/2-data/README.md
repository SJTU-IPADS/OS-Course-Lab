# ICS 第二讲 · 数据的表示

以第一讲引入的模型权重文件为观察对象，回答「文件中的字节表示什么」。

中文是基线（写在 Python 中），英文是 `i18n/en.toml` 翻译覆盖层，一份源码对应两种语言。

## 结构

引言把问题分为四个：什么是字节、多字节如何组成数据对象、整型与浮点如何编码、
如何对浮点数进行量化。前五节回答前三个，第六至第九部分是量化专题，回答第四个，
节与节之间用 `lecture.bridge(...)` 过渡。
前五节的结论都应用于同一个文件，最后一页总结为四个答案。第一讲中 1.9 GB 的来源
原本在第五部分末的「精度决定模型的体积」一页计算，第五部分按 `part-5.md` 重做后这一页已删去。
这组数据留在这里，课上有提问时口头给出：按单一 4 位格式（Q4_0，4.5 位/权重）算是 1.81 GB；
实际文件是 Q4_K_M 混合配方，平均 5.01 位/权重、2.02 GB = 1.88 GiB，
`examples/gguf_bits.py` 能把整个文件统计一遍。

| 节 | 页数 | 内容 |
| --- | --- | --- |
| 回顾与本节的问题 | 5 | 课程信息与课程安排 · Ollama：本机的推理服务 · 第一讲的权重文件 · CPU、内存与磁盘 · 四个问题 |
| 第一部分 · 位与字节 | 6 | `xxd` 的二进制与十六进制视图 · 位串到值 · 十六进制与 C 的进制写法 · 模型格式如何标识自己 · C 数据类型的宽度 · 布尔值的存储 |
| 第二部分 · 字节序 | 7 | 内存即字节 · 字长与地址范围 · 大端与小端 · 读一个字段 · `show_bytes` · 字节序在什么场合可见 · 文本与 token |
| **第三部分 · 整数** | **10（另有 1 个续页）** | 两种整型数的表示 · 取值范围 · 强制转换 · 比较陷阱 · 越界与内核缺陷及其答案 · 大小不同的操作数比较 · 位运算 · 移位 · 运算符优先级与实例 |
| 第四部分 · 浮点数的编码 | 11 | 从十进制小数到二进制小数 · 乘二取整与例子 · 小数转换为二进制位 · IEEE 754 的由来 · 数值形式与三段编码 · 规格化值的阶码与尾数 · 12345 的规格化编码 · 非规格化值 · 特殊值 · 规格化/非规格化/inf/NaN 一览表 |
| 第五部分 · 浮点数的精度特性、舍入机制与现代格式扩展 | 9（另有 8 个续页） | 实数轴上的非均匀分布与相对精度 · 阶码对齐与浮点吸收 · C 常见的四种舍入模式与无偏性 · 位级舍入判决 · 结合律失效与灾难性抵消 · C 的浮点转换与未定义行为 · 爱国者导弹的时钟截断 · 动态范围与相对精度的折衷（FP16 与 BF16） · FP32 到 BF16 的位级映射 |
| 第六部分 · 大模型量化的底层系统原理 | 11（另有 4 个续页） | 为什么要把权重变短 · 换一把短尺子 · 一个数怎么换 · 变短之后快在哪里 · 专题导览 · 高精度神话的破灭 · 计算量、访存量、带宽与算术强度 · 内存带宽决定生成速度上限 · 自回归推理深陷访存受限 · 晶体管能耗与芯片面积核算 · 量化概览 |
| 第七部分 · 数学映射与数据分布 | 7（另有 5 个续页） | 均匀仿射量化映射 · w、q、d、m 的数轴图解 · 偏移引发的交叉项惩罚 · 偏移带来的额外计算量 · 非对称分布下的步长浪费与格点位置图 · m = 0 与 m ≠ 0 的量化练习 · 动态范围与局部精度的零和博弈 |
| 第八部分 · GGUF 家族与二级量化 | 4（另有 4 个续页） | 压缩率与精度的光谱权衡 · GGUF 基础 4 位量化块物理排布 · K-quants 超块与二级量化 · 6 位字段跨越字节边界的重构 |
| 第九部分 · 前沿视野与实验 | 3 | 从通用规范到专用 AI 浮点 · NF4 的正态分位点码本 · 大作业发布 |

共 74 个编号页（含末页的本节小结与自动生成的参考文献页）、9 个衔接页与 1 个封面。

续页与前一页同标题、同页码，由一页放不下的页拆出：图片或代码下方放不下的列表与重点句移到续页。
第一至第四部分只有一个续页（`kernel-bug-answer`），第五部分的续页见下文。量化专题保留的 16 页拆出 9 个续页：
`fp32-myth`、`affine-mapping`、`clipping-tradeoff`、`granularity-spectrum`、
`ai-float-formats`、`nf4-lut`、`lab-release` 七页没有拆，
`kquants-superblock` 拆出 2 个，零点一页重写为两个各占编号的页面（见下文），其余 7 页各拆出 1 个。
专题中另有四页 `roofline-basics`、`quant-overview`、`affine-grid` 与 `quant-example` 不来自 `quant.md`，前三页没有拆，
`quant-example` 拆出 3 个续页；`symmetric-failure` 之后另有一个图解续页 `symmetric-failure-grid`，同样不来自 `quant.md`。
这些续页都不计入上面的 9 个。五者见下文。
只有一页放不下时才拆：留下的每一处拆分都试过合并，合并后要么越过页底
（`symmetric-failure` 超出 125 px 以上，其余 6–75 px），
要么正文压到图片（`topic-cover`、`memory-wall`、`gguf-blocks`、`scale-bits`）。
`fp32-myth` 合并后末行离页脚分隔线约 37 px。
`kquants-superblock` 的拆分按讲师要求而定，不按页高（见第十四处）。
专题的衔接页共四个：第六部分之前的一个由 `quant.md` 第 1 页的标题与副标题拆出，
第七至第九部分之前的三个只写部分名。衔接页不占编号，也不进提纲。

浮点数占两节：第四部分讲解 IEEE 754 的编码规则，第五部分讲解精度特性、舍入机制，
以及 FP16、BF16 等面向深度学习的格式。两节之间的衔接页作为课次的分界。

**第五部分的文案以仓库根目录的 `part-5.md` 为基础，保留原有结构，只按事实审校结果作必要改写。**
`pages.py` 中这一节只做排版：「幻灯片标题」接到 `p.title`；「正文要点」、表格与代码
（连同「量化对比表格」「代码示例与行为剖析」这类标签行）按原有层级接到 `p.slide` / `p.table` / `p.code`；
「章节定位」「核心教学目标」「讲师点拨与过渡」进 `p.notes`。第 4 页用「幻灯片标题」一栏的
「微架构视角：二进制小数的位级舍入判决」，不用小标题。「推荐配图与示意图设计」是给配图的说明，
不上屏，每页按它画了图（见「图表」一节的 10 个生成器），图中的标注是配图自己的文字，不属于 `part-5.md`。
第 7 页的两张图按说明的「左侧 / 右侧」并排放在续页上。
`part-5.md` 的 9 页各占一个编号，拆出 8 个续页：第 1–8 页各 1 个，第 9 页不拆。
拆分点都落在 `part-5.md` 的要点分组之间，不改变文案的先后顺序；「核心教学目标」在拆出的第一面，
「讲师点拨与过渡」在最后一面。第 8 页的规格表缩成 5 列，与「BF16 的设计突破」合为一面；
第 9 页只上屏「位布局天然对齐优势」一组要点与数据通路图，`part-5.md` 中其后的硬件舍入算法、
特殊值兼容、转换代码与本节总结都不上屏（`pages.py` 里的 `BF16_CODE` 目前没有页面引用）。
代码清单沿用 `part-5.md`，注释是中文，个别注释按审校结果改写；这些代码只上屏，
不在 `examples/` 下，也没有 `p.demo`。
重做之前的第五部分（12 页，含「精度决定模型的体积」）已整体删去，它用过的示例与图见下面两节。
审校改写只落在 `pages.py` 中，`part-5.md` 仍是原稿，两者的差异即审校的改动。

量化是一个独立专题，分四个部分：第六部分给出系统动机，第七部分给出数学映射，
第八部分讲真实文件中的格式，第九部分是前沿与大作业。部分与部分之间的衔接页适合作为下课的分界。
`quant.md` 分五幕，按 1.5–2 次课的讲解时长安排；第 4 幕「底层硬件算术与位级并行技术」
（第 16–20 张：INT32 累加器、点积指令、4 位打包与符号扩展、SWAR 加法、SmoothQuant）、
第 3 幕末的混合精度配方（第 15 张），以及第 5 幕中的 Cache 实测、PTQ 流水线与结语
（第 23、24、26 张）已按讲师要求删去，第 1–3 幕对应第六至第八部分，第 5 幕余下的三页是第九部分。
专题封面续页的第 4 个核心问题（无分支解包与通道隔离加法）只在第 4 幕回答，随之删去。

**专题的文案以仓库根目录的 `quant.md` 为基础，保留原有结构，只按事实审校结果作必要改写。**
`quant_pages.py` 只做排版：把「页面标题」接到 `p.title`，把「屏幕核心文案」按原有层级接到
`p.slide` / `p.table` / `p.code` / `p.demo`，「讲师讲稿与互动」进 `p.notes`，「文献出处」进页脚。
一页放不下时拆成同名的两页（`*_cont`）——同名页在提纲里合并为一行、共用一个页码，
因此拆页既不改文案也不改编号。`quant.md` 的 26 张 slide 各占一个编号，例外有五处：
插入的 `roofline-basics`、`quant-overview`、`affine-grid` 与 `quant-example` 各占一个编号；零点一页重写为 `zero-point-cost` 与 `zero-point-compute`，
各占一个编号；第 10 张（KL 散度标定算法，原 `kl-calibration`）按讲师要求删去，不占编号。
第 16–20 张与第 23、24 张也已删去，不占编号。
因此 `bandwidth-ledger` 起各页的编号比按 `quant.md` 顺序数出的多 1，`affine-mapping` 起多 2，
`affine-grid` 之后多 3，`zero-point-compute` 之后多 4，`quant-example` 之后多 5，
`granularity-spectrum` 起又回到多 4，`ai-float-formats` 起少 1，`lab-release` 起少 3。
修改这一部分之前请先改 `quant.md`。

不来自 `quant.md` 的内容有十六处，都按讲师要求改动，`quant.md` 都没有同步，
对比两份文案时不要把它们算进去。

第一处在 `bandwidth-ledger-cont`（第 54 页的续页）：补了 RTX 5090 的照片和「对比 RTX 5090」一条，
给「算术强度极低（BF16 权重约为 1 FLOP/Byte）」一个参照——
BF16 张量峰值算力 209.5 TFLOPS ÷ 显存带宽 1792 GB/s ≈ 117 FLOP/Byte，
取自 NVIDIA RTX Blackwell 白皮书 V1.1 表 3 的稠密值（419 TFLOPS 是稀疏值，不适用）。
白皮书以 `p.cite` 列在参考文献页。

第二处是 `affine-mapping` 之后的整页 `affine-grid`（第 59 页）：只有一张自制图 `affine-grid.svg`，
用 4 位无符号量化的算例（权重范围 [−1.0, 2.0]，d = 0.2，m = −1.0）把 w、q、d、m 画在数轴上。

第三处是零点的两页 `zero-point-cost`（第 60 页）与 `zero-point-compute`（第 61 页），整体重写：
`quant.md` 原文直接给出 $S_X$、$Q_{X,k}$ 等符号的展开式而不加定义，讲师要求改写。
第 60 页从第 58 页的反量化公式出发，把一个长度为 K 的点积代入展开，得到编号 ①–④ 的四项；
第 61 页说明每一项在推理时还是离线计算，并用一张表比较对称量化与展开后计算两种做法多出的运算。
原文把「$Z_W$」称作激活的零点；重写后第七部分的公式统一为 GGUF Q4_1 的 $\hat{w} = d\,q + m$，
这一项写作权重的偏移 $m_w$，两页标题中的「零点 $Z$」随之改为「偏移 $m$」。
讲稿中给出 Llama-3.2-1B 一个 2048 × 2048 投影的运算次数，以及 GGUF Q4_1 × Q8_1
（`block_q8_1` 中的 `s = d * sum(qs)`）对这一项的处理。

第四处是 `symmetric-failure` 与其实测续页之间的图解续页 `symmetric-failure-grid`（仍是第 62 页）：
只有一张自制图 `symmetric-waste.svg`，数据是实测续页读取的同一个文件 `examples/ext/w-final-norm.bf16`。
图中对称量化的格点按讲师要求与其后的练习采用同一约定：绝对值最大的元素对应 $q = 7$，
$d$ = 最大绝对值 / 7 ≈ 0.417，格点 $-8d$ 到 $7d$，即 $[-3.34, +2.92]$：负半轴 8 个与 0 处 1 个共 9 个
在数据范围外，范围内 7 个。`symmetric-failure` 的文案与讲稿采用同一组数值，这一行称为「对称量化」，
不称 Q4_0；Q4_1 的步长约 0.192，不到对称量化的一半。`quant_compare.c` 实测的 Q4_0 按 llama.cpp 的实现
取 $d$ = 最大绝对值元素 / −8，格点 $(q - 8)d$，最大值为正时是 $-7|d|$ 到 $8|d|$，这一差别写在实测续页的讲稿中。
改为 / 7 之前的页面、图与生成脚本保存在 `quant_pages.py.pass13`、`assets/symmetric-waste.svg.orig`
与 `diagrams/symmetric_waste.py.orig`。

第五处是实测续页 `symmetric-failure-cont`（仍是第 62 页）的控制台输出：
`quant.md` 给出的输出与 `quant_compare` 的实际输出不一致，页面改成照录实际输出
（`cd examples && ./quant_compare norm`），共 7 行，含 per-tensor、per-256 与 Q4_K 三行。
页面讨论的只是 Q4_0 与 Q4_1 两行，这两行用 `p.demo` 的 `bold=[5, 6]` 加粗。
这一页的讲稿说明实测的 Q4_0 按块计算步长、取 / −8，与图解续页的 / 7 不同。
改动 `quant_compare.c` 或样本文件之后，需要重新运行并同步这段输出与加粗的行号。

第六处是 `bandwidth-ledger`（第 54 页）的硬件数据：`quant.md` 用的是双通道 DDR5-5600（89.6 GB/s）
与 RTX 4090（1008 GB/s），讲师认为过旧，改为双通道 DDR5-7200（115.2 GB/s）与 RTX 5090（1792 GB/s）。
DDR5-7200 是 Intel Core Ultra 200S Plus（2026 年 3 月发布）官方支持的内存速率，以 `p.cite` 列在参考文献页；
1792 GB/s 与续页的算术强度对比取自同一份白皮书。表中速度按「峰值带宽 ÷ 模型体积」重新计算，
模型体积用 Llama-3.2-1B 的 1,235,814,400 个参数算出（2.4716 / 1.3131 / 0.6951 GB），
这一算法能逐项复现 `quant.md` 原表的六个数。讲稿中的两个速度随之改为 47 与 166 token/s，
其余叙述未改。

第七处是 `memory-wall`（第 55 页）与其续页：`quant.md` 用三条文字比较 BERT 与 GPT-2 的计算量与时延，
讲师要求补上参数量与访存量，再把对比改为一张表（模型、参数量、计算量、访存量、算术强度、
端到端相对时延），算术强度一列留空，由学生用计算量除以访存量算出；左图中两者的位置可供对照，
续页的「根源剖析」给出答案。讲稿末段补了一段，提示这一练习、两个除法的结果，以及访存量约 45 倍、
时延约 28 倍的差距。
数值读自 Gholami 等人论文
（arXiv 2403.14123）的图 3：序列长度 4096 时 BERT-Base 为 1324 GFLOPs、11.2 GMOPs，
GPT-2 为 1012 GFLOPs、507.8 GMOPs。论文按 batch size 1、参数与激活均为 8 位统计，
因此 GMOPs 即 GB，访存量之比 507.8 ÷ 11.2 ≈ 45。论文没有注明 GPT-2 的规模，只说它与
BERT-Base 的配置基本相同，图中的计算量也与 12 层、768 维的 GPT-2 相符，因此参数量取该规模
公开权重的 124M（GPT-2 原论文表 2 写作 117M）；BERT-Base 的 110M 取自 BERT 原论文。
讲稿给出两个除法：1324 ÷ 11.2 ≈ 118（论文图中标注 117，差在访存量的舍入）与 1012 ÷ 507.8 ≈ 2；
续页的 117 ~ 266 是 BERT-Base 与 BERT-Large 在序列长度 128 ~ 4096 下的范围。
续页原有的「计算方法」一条已由讲师删去。
续页的页底是 `roofline-shift.svg`，由上一页的图中移来：RTX 5090 的 Roofline（平衡点 ≈ 117 FLOP/Byte）上，
单请求生成的工作点从 BF16 的 1 经 8 位的 2 移到 4 位的 4，箭头为 BF16 → 4 位，与「量化的作用」一条
的「权重数据量降至约 1/4（BF16 → 4 位）」对应；三个点都在平衡点左侧的访存受限区。
为给表格腾出位置，第 55 页的图宽从 770 px 缩到 660 px；「序列长度 4096」这一前提写在表格上方的标题中。

第八处是 `alu-energy`（第 56 页）右上角的台积电商标 `assets/ext/tsmc-wordmark.svg`，
出处与授权见下文「外部图片」一节，页脚注明来源。

第九处是同一页表中的数据，按讲师要求统一为 45nm。`quant.md` 的能耗列（3.7 / 0.9 / 1.1 / 0.2 / 0.03 pJ）
本来就是 Horowitz（ISSCC 2014）的 45nm 数据，标题却写「台积电 7nm」；面积列（770 / 260 / 280 / 180 / 8 μm²）
与任何出处都不一致。标题改为「45nm 工艺」，能耗列不变；面积列改为 Song Han 在 Stanford CS231n（2017）
第 15 讲中给出的台积电 45nm 综合结果（7700 / 4184 / 1640 / 282 / 36 μm²，Design Compiler，
浮点单元用 DesignWare），以 `p.cite` 列在参考文献页。Horowitz 的能耗数据没有写明代工厂，
因此标题与表头都不写台积电。相对倍数按原数据重算：FP16 乘法 1.1 ÷ 0.03 ≈ 37（原为 36），
INT8 乘法 0.2 ÷ 0.03 ≈ 6.7（原为 6.6），其余不变。续页 `alu-energy-cont` 的改动见第十一处。

第十处是 `fp32-myth` 与 `bandwidth-ledger` 之间插入的 `roofline-basics`（第 53 页），讲师要求
在给出硬件数据之前定义后面几页用到的量与单位。页上一张表定义计算量 $W$、访存量 $Q$、峰值算力 $P$、
带宽 $B$ 与算术强度 $I = W/Q$ 及其单位，表下说明 G/T 前缀与 GFLOPs / TFLOPS 的写法。

第十一处是插入第十处时核对第 52–56 页后修正的表述，逐条如下：

- `fp32-myth` 的系统结论原为「更是造成自回归推理被“显存带宽”卡死的物理根源」。访存受限的根源
  是自回归生成每个权重只参与一次乘加、算术强度低，FP32 只是使搬运的字节数成倍增加，
  改为「还会成倍增加自回归推理需要搬运的字节数，加剧“显存带宽”的限制」。
- `bandwidth-ledger-cont` 的「算术强度极低（≈ 1 FLOP/Byte）」只对 BF16 权重成立，补注
  $2\text{ FLOP} \div 2\text{ Byte}$；「权重的物理位宽，几乎线性决定了模型的生成吞吐量」
  方向相反，速度上限是带宽除以体积，改为「与权重的物理位宽近似成反比」。
- `memory-wall` 的讲稿原为 BERT「采用批量前向并行处理」「计算强度超过 100 FLOP/Byte」。
  论文按 batch size 1 统计，BERT 的权重复用来自整段序列在一次前向传播中并行处理，
  讲稿照此改写，「计算强度」统一为「算术强度」。访存量相差 45 倍只在序列长度 4096 时成立，
  页面把这一前提写在表格的标题中。
- `memory-wall-cont` 的「BERT（预填充/批量计算）」改为「整段序列一次前向」，BERT 是 Encoder，
  没有预填充阶段；「GPT-2（自回归单字生成）」改为「逐 token 生成」；「将数据体积削减至 1/4」
  补上前提「BF16 → 4 位」。
- `alu-energy-cont` 的末条原为「8 位整数加法器仅需极简的布斯编码进位链，不仅静态漏电微弱，
  硅片利用率也高出十余倍」。布斯编码用于乘法器，「十余倍」也与上一页的面积不符，
  改为按表中面积算出的两个比值（加法器 1/116，乘法器 1/27），页脚补上面积的出处。
- 图 `roofline-latency.svg`：BERT 原画作 200 FLOP/Byte 的一个点，论文中没有这个值，
  改为 117 ~ 266 的区间，两者都不标数值，留给学生计算；量化箭头原从 2 指到 8，相当于 8 位降到 2 位，
  已从这张图中去掉，改画在续页的 `roofline-shift.svg` 中（BF16 → 4 位，见第七处）；
  BERT 柱下的「Encoder 预填充」改为「Encoder 整段并行」。

第十二处是 `fp32-myth`（第 52 页）「23 位尾数带来约 7 位有效十进制精度」之后补的量级估算：
尾数 $M$ 的间隔为 $2^{-23} \approx 1.2 \times 10^{-7}$。原先写的是
$2^{23} \approx 8.4 \times 10^{6}$「量级为 $10^{7}$」，按科学计数法读是 $10^{6}$，改为尾数的间隔：
与第四部分的科学计数法一致，有效数字由尾数决定，阶码只决定小数点的位置。
讲稿末段补了算上隐含首位的 $\log_{10} 2^{24} \approx 7.2$，
以及 C 标准的 `FLT_DIG` = 6 与 `FLT_DECIMAL_DIG` = 9；原讲稿中按 $2^{23}$ 个值计数的估算删去。

第十三处是 `granularity-spectrum`（第 65 页）：原续页的三条实测数据（Llama-3.2-1B `ffn_down`，
整张量 4.00 位/权重 13.50%、256 元素分组 4.06 位/权重 11.15%、32 元素分组 4.50 位/权重 8.42%）
按讲师要求改为表格并入本页页底，续页删去。页底只剩约 150 px，因此三种粒度作列、位宽写进表头，
表格只有「相对均方根误差」一行；页脚的出处随之移到正文块上。
全文审校后，表头改为「ffn_down 前 4096 个」「整段共用（4.00 位）」：数据是 `ffn_down` 的前 4096 个权重
（见 `examples/ext/PROVENANCE.md`），不是整张矩阵；页脚原引的 Hubara（讲低位宽训练，不讨论量化粒度）
换为 Krishnamoorthi 的白皮书（2018）与 Dettmers、Zettlemoyer 的 ICML 2023 论文。

第十四处是 `kquants-superblock`（第 67 页）：按讲师要求，第一面只讲工程痛点，
超块图 `q4k-superblock.svg` 移到续页 `kquants-superblock-cont`，与「Q4_K 的破局之道」放在同一面，
方便对着图讲解。图与原续页的全部文字放不下，「超块开销结算」移到第二个续页
`kquants-superblock-cont2`，k-quants 的出处（Kawrakow，llama.cpp PR #1684）随之留在最后一面。其后按讲师要求又改了四处：

- 第一面页底补了 `q4-dilemma.svg`：位宽与误差的平面上 Q4_0（4.50 位，4.25%）与
  Q4_1（5.00 位，0.94%）两个点，数据取自 `symmetric-failure-cont` 中 `./quant_compare norm` 的输出（图中不写页码），
  4.50 位、约 1% 处画一个带问号的虚线圆，对应页上的问题。讲稿随之改写，
  原讲稿（二级量化的巧思）移到第一个续页。
- 第一个续页的二级量化改为两条：步长以 `d`、偏移以 `dmin` 为单位量化为 6 位整数 `sc` 与 `m`；
  `sc` 与 `m` 共 12 字节，`d` 与 `dmin` 为 FP16，共 4 字节。原文把 `d`、`dmin` 称为「全局基准」，
  没有写它们与 16 个子块参数的关系。这与 llama.cpp 的 `block_q4_K` 一致：
  `d` 是 8 个子块步长的最大值除以 63，`sc` = round(步长 ÷ `d`)，还原时子块步长 = `d` × `sc`。
  讲稿末段补了这一点。
- 图中的「还原：ŵ = d · sc · q − dmin · m」改为「子块的步长 = d × sc，偏移 = dmin × m」，
  三段中 scales 与 d, dmin 两格的说明改为「8 个 sc 与 8 个 m」「sc 与 m 的量化步长」；
  完整的还原两步写在第二个续页的页首。结算中的「全局基准」写作「`d` 与 `dmin`」。
- 第二个续页页底的表格照录 `quant_compare` 对两段权重的输出，只取 Q4_0、Q4_1、Q4_K 三行。
  `w-down-proj` 上 Q4_K（7.70%）与 Q4_1（7.69%）几乎相同；`w-final-norm` 上 Q4_K 为 2.16%，
  介于 Q4_0 与 Q4_1 之间，原因是 Q4_K 的偏移只向下平移（子块最小值大于 0 时按 0 处理），
  讲稿写明了这一点。

第十五处是 `alu-energy` 与第七部分之间插入的 `quant-overview`（第 57 页），讲师要求在进入公式之前
从整体上说明量化做什么：把浮点数映射为少量位的整数存储，推理时再映射回近似值参与计算。
页上依次是量化、存储、计算三条，一个 4 个权重的示例（按最大绝对值 0.97 对应 7 的比例映射到 −7 ~ 7，
映射回的值按 0.97 ÷ 7 的精确比例计算），以及收益与代价。这一页位于第七部分的公式之前，
页面与讲稿只用文字与数值，缩放系数、步长等概念留给第七部分。

第十六处是 `symmetric-failure-cont` 之后插入的练习页 `quant-example`（第 63 页，含三个续页），讲师要求在课上
让学生计算 $m = 0$ 与 $m \neq 0$ 两种量化。这一页原在 `affine-grid` 之后，按讲师要求移到对称量化的不足之后，
先讲完偏移的代价与对称量化的不足，再做练习；页标题随之由「示例」改为「练习」。
同一组 6 个权重（−0.40、0.00、0.31、0.58、0.87、1.40）量化为 4 位。
第一面只出题：一张只画实数轴与 6 个权重的图（`quant-example-task.svg`），以及两种方式的条件与要求的量，
讲稿提示留几分钟给学生计算。这张图与解答面的图由同一段代码画出、高度相同，翻到解答面时实数轴的位置不变。
第一个续页 $m = 0$，$q$ 取 −8 ~ 7，$d$ 取最大绝对值 ÷ 7 = 0.2（NVIDIA 量化白皮书与 TFLite 的取法），
量化范围 [−1.6, 1.4]；图中画出 −8（对应 −1.6），没有权重映射到它的原因按讲师要求只在讲稿中说明；第二个续页 $m \neq 0$，$q$ 取 0 ~ 15，
量化范围即数据范围，$d = 0.12$，$m = -0.4$。这两面各有一张图（`quant-example-zero.svg`、
`quant-example-offset.svg`，上为实数轴、下为整数轴，格点只画在整数轴上）与一张逐个权重的 $q$、$\hat{w}$、误差表，各权重换算后的商都不恰好落在 .5 上，四舍五入没有歧义；
第三个续页用一张表对比两种做法的格点数、$d$、误差上界、最大误差与 0.00 的还原值。
页面与讲稿只写 $m = 0$ 与 $m \neq 0$，不用对称、非对称这两个名称（这一页最初位于这两个概念之前）。

`../2-data-bak/` 是替换之前的整份讲义，原样保留，用来对比两份量化文案的优劣。
它自带旧量化部分用到的全部图、生成器与译文，定稿之前不要删。

浮点数只在本讲讲解，课程中没有单独的浮点专题，因此 IEEE 754 的编码规则、
非规格化数与特殊值在第四部分、舍入模式在第五部分讲解完毕。

第三部分保留了 CS:APP 的完整基线（无符号数与有符号数、强制转换、`copy_from_kernel`
的越界缺陷、扩展与截断、位运算与移位）。专题中 GGUF 的半字节打包（`gguf-blocks`）
与 6 位字段的跨字节拼接（`scale-bits`）以第三部分的位运算与移位为前置知识。

## 课上运行命令

本讲有 18 个 `p.demo(...)`。`ollama-intro` 与 `recap-weights` 两页的命令操作本机的 Ollama 及其权重文件，其余全部在 `examples/` 下实际运行：

```bash
python3 -m lecturekit.cli view lectures/2-data --watch
```

投影上显示命令与录制的输出，按 ▶ 即在讲义目录中实际运行（`view --watch` 下按钮可用，
渲染出的静态包中按钮不可用）。固定的数值输出来自本机实际运行；地址等非确定内容在录制输出中明确标为可变。
`examples/` 中每个 `.c` 都与幻灯片上
显示的清单一致（第五部分的清单沿用 `part-5.md`，只上屏，不在 `examples/` 下）。修改幻灯片上的代码时，源文件要同步修改，然后重新运行并核对输出。

每个 `p.demo(...)` 用 `files=[...]` 声明了它编译的源文件，▶ 旁边因此有一个以
文件名为标签的按钮，按下后在右侧展开该文件的全文（带行号），课上可以直接查看代码。
文件在按下时才读取，修改源文件后再按一次即显示新的内容。

`ollama-intro` 一页由第一讲移来。`ollama serve` / `ollama pull` / `ollama run` 写了 `timeout=0`，
不会自行结束，用抽屉里的 ■ 停止。每按一次 ▶ 新开一个运行标签页，`ollama serve` 占着一个
标签页时，`ollama pull` / `ollama run` 在旁边的标签页中可以连上它。页面右侧的
`assets/ollama-logo.png` 随这一页一起从第一讲移来。

## 跨平台：x86-64 Linux、arm64 macOS 与 x86-64 Windows

本讲讲的是数据表示，大部分源码可在 x86-64 Linux、arm64 macOS 与 Windows WSL2 上编译；
涉及 `__bf16`、Linux 头文件、预编译二进制、地址值或汇编的示例依赖具体编译器与平台，
结果不保证逐字节一致。为减少差异做过三处调整：

| 页 | 调整 | 原因 |
| --- | --- | --- |
| `recap-weights` | `ls -lhS ... \| sed -n '2p'` + `file "$(ls -dS ... \| head -1)"` | macOS 的 `ls -l` 按 512 字节块打印 `total` 行；`sha256-*` 按字母序会先匹配到 JSON manifest。与第一讲的同一条命令保持一致 |
| `vector-bool`（该页已不在本讲中） | `grep -oE 'cannot (convert\|initialize).*'` | 同一处错误 gcc 说 cannot convert，clang 说 cannot initialize |

随平台变化、需要在课上说明的一点（写在对应页的 `p.notes` 里）：

- **`long` 的宽度**：64 位 Linux 与 macOS 是 LP64（8 字节），原生 Windows 是 LLP64（4 字节，
  指针仍是 8 字节）。`c-data-sizes` 表中明确标为常见 64 位 LP64。WSL2 中是 Linux 程序，与该列一致。

这里列出的三种运行环境都是小端，确定性的整数与浮点字节输出一致；指针地址受 ASLR 影响，
每次运行都可能不同。`p.demo` 里录的固定数值输出来自 x86-64 Linux。

## 构建

```bash
python3 -m lecturekit.cli view   lectures/2-data --watch      # 修改时实时预览
python3 -m lecturekit.cli render lectures/2-data --pdf        # PDF
python3 -m lecturekit.cli render lectures/2-data --to pptx    # 可编辑的 PPTX
python3 -m lecturekit.cli render lectures/2-data --png        # 逐页 PNG，用于检查版面溢出
python3 -m lecturekit.cli view   lectures/2-data --watch --lang en   # 英文
```

## 示例

`examples/` 是所有可运行代码的目录。编译产物已列入 `.gitignore`。
「使用页」一列由 `pages.py` / `quant_pages.py` 中的实际引用得出；写「—（当前不占幻灯片）」
的文件仍然能编译运行，只是没有页面用它。`bit_rules.c`、`shift_kind.c`、`truncate.c`、
`truncate_endian.c` 不在下表中，它们同样不占幻灯片。

| 文件 | 使用页 | 说明 |
| --- | --- | --- |
| `make_gguf.py` | — | 生成 `tiny.gguf` |
| `make_formats.py` | — | 生成 `tiny.safetensors` / `tiny.onnx` / `tiny.pkl` / `tiny.zip`（ONNX 文件需要 onnx 包） |
| `tiny.safetensors`、`tiny.onnx`、`tiny.pkl`、`tiny.zip` | `other-formats` | 四种格式的开头各 16 字节，都按各自规则写出，作为源码提交 |
| `tiny.gguf` | `hexdump`、`other-formats` | 224 字节的真实 GGUF v3 文件，作为源码提交 |
| `gguf_head.c` | `read-the-field` | 按两种字节序读同一个 `version` 字段 |
| `show_bytes.c` | `show-bytes` | CS:APP 的 `show_bytes`，两参数版本 |
| `sizes.c` | `c-data-sizes` | 各类型的 `sizeof` |
| `bitset_demo.cpp` | —（当前不占幻灯片） | 一百万个布尔值在 `vector<bool>` / `bitset` / `deque<bool>` 中占的字节数 |
| `vector_bool_bad.cpp` | —（当前不占幻灯片） | 有意无法编译：`&v[0]` 的类型不是 `bool *` |
| `bool_size.c` | `bool-storage` | 同一份代码按 C 与 C++ 编译，比较 `bool` 的宽度与取值（需要 g++） |
| `compare.c` | `comparison-trap` | 有符号与无符号混用的四个比较 |
| `endian_host.c` | —（当前不占幻灯片） | `__BYTE_ORDER__` 与 `htole32` / `htobe32` 各自的结果 |
| `endian_calls.c` | —（当前不占幻灯片） | 两个转换函数编译出的指令（配合 `gcc -S`） |
| `precedence.c` | `precedence-in-practice` | 三个缺少括号的表达式与 `-Wall` 的三条警告 |
| `frac_bits.c` | `frac-to-bits` | 幻灯片上的乘二取整循环，转换 0.75、0.625、0.2 |
| `normalized.c` | `normalized-example` | 把 12345 编码为单精度浮点数，逐步输出二进制、阶码、尾数与 `0x4640E400` |
| `denormalized.c` | `denormalized` | 解码三个阶码字段全 0 的位模式：$-0$、$2^{-127}$ 与最小正数 $2^{-149}$ |
| `gguf_bits.py` | —（不占幻灯片） | 读整个权重文件，按张量类型统计位数与字节数；1.9 GB 的来源（见「结构」一节）由它统计得出 |
| `float_law.c` | —（当前不占幻灯片） | 浮点结合律失效与 `0.1 + 0.2` 的舍入 |
| `float_casts.c` | —（当前不占幻灯片） | `int` / `float` / `double` 之间的转换、向零截断与越界转换 |
| `rounding.c` | —（当前不占幻灯片） | 舍入到最近的偶数，以及 `0.1` 的近似值 |
| `rounding_modes.c` | —（当前不占幻灯片） | 用 `fesetround` 切换四种舍入模式，对五个值取整（链接时需要 `-lm`） |
| `patriot.c` | —（当前不占幻灯片） | 0.1 截断到 23 位小数后的误差，以及 100 小时后的累积误差与距离 |
| `fp16_range.c` | —（当前不占幻灯片） | 实际的 `_Float16` / `__bf16` 转换（需要 gcc 12+，x86-64） |
| `bf16_round.c` | —（当前不占幻灯片） | 保留前 16 位再舍入到最近的偶数，与编译器的 `__bf16` 转换逐个对照，含进位到阶码的情形（需要 gcc 12+，x86-64） |
| `bf16.c` | —（当前不占幻灯片） | 只截断、不舍入，逐位打印 π 的 FP32 与 BF16 以及被舍去的部分 |
| `bf16_classes.c` | —（当前不占幻灯片） | 把八个位模式复制进 `__bf16` 输出，覆盖非规格化、规格化、无穷与 NaN（需要 gcc 12+，x86-64） |
| `int4.c` | —（不占幻灯片） | 打包加法，穷举 65536 对输入；原 `swar-add-demo` 页已随第 4 幕删去 |
| `quantize.c` | —（不占幻灯片） | Q4_0 的量化与反量化，输出误差；专题改写后不再有对应页，课上有提问时可以当场运行 |
| `quant_compare.c` | `symmetric-failure-cont`、`kquants-superblock` | 五种方案（per-tensor / per-256 / Q4_0 / Q4_1 / Q4_K）在真实权重上的字节数与误差；`symmetric-failure-cont` 上的 `p.demo` 只取 `norm` 一档，对比 Q4_0 与 Q4_1；`kquants-superblock` 第一面的图取 `norm` 的 Q4_0 与 Q4_1 两点，第三面的表取两段权重的 Q4_0、Q4_1、Q4_K 三行（需要 gcc 12+，x86-64） |
| `k_scales.c` | —（不占幻灯片） | Q4_K 的 12 字节打包，100 万组随机值验证 `put`/`get` 互逆；`scale-bits` 页上的解码片段就是它验证的那套位拼接，课上有提问时可以当场运行 |
| `make_sample.py` | — | 用两次 HTTP Range 请求获取下面两段权重；也是 safetensors 头的最小读法示例 |
| `ext/w-down-proj.bf16`、`ext/w-final-norm.bf16` | 同 `quant_compare.c` | Llama-3.2-1B 的两段真实权重，各数 KB，出处与授权见 `ext/PROVENANCE.md` |

`float_law.c` 至 `bf16_classes.c` 这九个文件原属重做之前的第五部分，页面删去后仍能编译运行，
课上讲到对应的现象时可以当场演示（`patriot.c` 算出的 0.343 秒与 575.4 米即第 7 页的数据）。

`fp16_range.c` 用了 `_Float16` 与 `__bf16`，`bf16_classes.c`、`bf16_round.c` 用了 `__bf16`，这两个类型在旧编译器上不存在，
源文件头部已注明。其余例子只用 C99。

## 图表

`diagrams/` 是自制图的源文件，`assets/` 中的同名 `.svg` 是产物，不要手工修改：

```bash
lectures/2-data/diagrams/render.sh    # 重新生成全部图表
```

| 源文件 | 产物 | 使用页 |
| --- | --- | --- |
| `svgkit.py` | — | 共用的图元：`cells`、`brace`、`text`、`rect`、`esc`，以及混排斜体变量与下标的 `rich` |
| `check_bounds.py` | — | 检查图中是否有文字超出画布，`render.sh` 末尾自动运行 |
| `byte_order.py` | `byte-order.svg` | `endianness`，同一个值在两种排列下的四个地址 |
| `cpu_memory_disk.py` | `cpu-memory-disk.svg` | `machine-model`，CPU 与磁盘夹着内存，内存里每个字节有地址、存的是 8 个 0/1 |
| `quant_path.py` | `quant-path.svg` | `quant-plain-speed`，权重的路径：磁盘 → 主存 → 显存 → 计算单元，量化少搬的是最后一段 |
| `memory_bytes.py` | `memory-bytes.svg` | `memory-as-bytes`，12 个地址格与跨 4 格的 `int` |
| `address_space.py` | `address-space.svg` | `word-size`，4 GB 的横条与占用近一半空间的权重文件 |
| `same_bits.py` | `same-bits.svg` | `casting`，同一串 16 位的两种解释 |
| `shifts.py` | `shifts.svg` | `shifts`，一个位串的三种移位与补入的位 |
| `bit_fields.py` | `bit-fields.svg` | `ieee-form`，3.1415927 的 FP32 三段分解 |
| `float_formats.py` | `float-formats.svg` | —（当前不占幻灯片；原 `precision-formats`），七种格式的三段分配 |
| `float_spacing.py` | `float-spacing.svg` | —（当前不占幻灯片；原 `float-distribution`），可表示的值在 0–8 上随阶码增大而变稀疏 |
| `ulp_spacing.py` | `ulp-spacing.svg` | `ulp-distribution-cont`，左半是 3 位尾数的简化格式在 [0.5, 4) 上的刻度与三段间距，右半是 FP32 在 [2²⁴, 2²⁵) 内间距为 2、奇数落在缝隙中 |
| `fp_absorb.py` | `fp-absorb.svg` | `fp-absorption`，2²⁴ 加 1.0：对阶后形成精确中点，向最近偶数舍入的结果仍是 2²⁴ |
| `round_bias.py` | `round-bias.svg` | `ieee-rounding-modes-cont`，中点 1.5–8.5 依次舍入：四舍五入的累积误差一路升到 +4，向偶数舍入在 0 与 0.5 之间振荡 |
| `round_grs.py` | `round-grs.svg` | `round-bits-cont`，尾数末位与 G、R、S 的排布，以及判定精确中点与进位的逻辑门 |
| `cancellation.py` | `cancellation.svg` | `cancellation`，前 21 位相同的两个 24 位有效数相减，规格化后只剩 3 位有效精度 |
| `cast_paths.py` | `cast-paths.svg` | `casts-ub-cont`，`int`、`float`、`double` 之间的六条转换：绿色精确、黄色舍入、红色截断或越界，附 `cvttsd2si` 的 `0x80000000` |
| `patriot_drift.py` | `patriot-drift.svg` | `patriot-cont` 左图，时钟误差随运行时间线性增长，100 小时 0.3433 秒 |
| `patriot_gate.py` | `patriot-gate.svg` | `patriot-cont` 右图，按 GAO 报告给出的约 3750 mph 估算，跟踪窗口与导弹实际位置相差约 575 米（示意，不按比例） |
| `fp16_bf16_fields.py` | `fp16-bf16-fields.svg` | `bf16-tradeoff`，FP32、FP16、BF16 按同一位宽比例排开，FP16 的 5 位阶码以红框标出，BF16 与 FP32 的高 16 位对齐 |
| `bf16_datapath.py` | `bf16-datapath.svg` | `bf16-mapping`，左边 FP32 → BF16 的数据通路（高 16 位直通，低 16 位一次加法，进位加到高半部），右边 FP32 → FP16 的五个步骤 |
| `expand_truncate.py` | `expand-truncate.svg` | —（当前不占幻灯片） |
| `affine_grid.py` | `affine-grid.svg` | `affine-grid`，上图是实数轴与整数轴上一一对应的 16 个格点（d 为间距，m 为 q = 0 对应的实数值，含一个截断的例子），左下是放大的一格与误差上界 d/2，右下是 d 加倍与 m 改为 0 时网格的变化 |
| `symmetric_waste.py` | `symmetric-waste.svg` | `symmetric-failure-grid`，上图是 2048 个最终 RMSNorm 权重的真实直方图（读取 `examples/ext/w-final-norm.bf16`），下面两行是整个张量共用一个步长时对称量化（$d$ = 最大绝对值 / 7，格点 $-8d$ ~ $7d$）与 Q4_1 的 16 个格点在同一条实数轴上的位置，数据范围以底色标出 |
| `quant_example.py` | `quant-example-task.svg`、`quant-example-zero.svg`、`quant-example-offset.svg` | `quant-example` 的出题面只画实数轴与 6 个权重；续页 `quant-example-cont` 与 `quant-example-cont2` 画同一组 6 个权重在 $m = 0$（$q$ 取 −8 ~ 7，$d = 0.2$）与 $m = -0.4$（$q$ 取 0 ~ 15，$d = 0.12$）两种格点上的位置：上为实数轴 $w$，每 0.2 一个刻度，权重画在轴上；下为整数轴 $q$，每个编号画在它还原出的实数值 $d q + m$ 处，格点只画在整数轴上；两张图的实数轴相同，$m \neq 0$ 时 0.0 落在 $q = 3$ 与 4 之间；每个权重以箭头指向舍入到的格点，数据范围以底色标出，范围外的格点为灰色，图顶部标出数据范围内的格点数，灰色括号标出范围外的格点数 |
| `roofline_latency.py` | `roofline-latency.svg` | `memory-wall`，左图是 GPT-2 的 2 FLOP/Byte 与 BERT 的 117 ~ 266 FLOP/Byte 在 Roofline 上的位置（不标数值，页面的表格要学生算出），右图是它解释的端到端时延差 |
| `roofline_shift.py` | `roofline-shift.svg` | `memory-wall-cont` 页底，RTX 5090 的 Roofline 上单请求生成从 BF16（I = 1）经 8 位（2）到 4 位（4）的移动 |
| `gguf_q4_blocks.py` | `gguf-q4-blocks.svg` | `gguf-blocks`，Q4_0 的 18 字节与 Q4_1 的 20 字节块，逐字段按比例排开 |
| `q4_dilemma.py` | `q4-dilemma.svg` | `kquants-superblock` 第一面，位宽与误差的平面上 Q4_0 与 Q4_1 两个实测点，以及两者都达不到的位置（Q4_0 的体积、Q4_1 的精度） |
| `q4k_superblock.py` | `q4k-superblock.svg` | `kquants-superblock-cont`，144 字节的 Q4_K 超块分为三段，12 字节的 `sc`、`m` 管 8 个子块，底部一行写明子块步长 = d × sc、偏移 = dmin × m |
| `q4k_scale_bits.py` | `q4k-scale-bits.svg` | `scale-bits`，一个 6 位步长跨在 Byte 0 与 Byte 8 上，以及解码时的拼接 |

`float-formats.svg`（当前不占幻灯片）有意不绘制 FP64（它在深度学习中很少使用，改由页上的 `p.aside` 简要说明），
并把 FP32 与 BF16 排在相邻两行，以便绘制 bit 16 处的分界线（转换保留左侧 16 位，再按右侧舍入）。

编写这类生成器时注意四点：SVG 文本中的 `&`、`<`、`>` 必须转义，否则整张图会因为
不是良构 XML 而被静默丢弃（`svgkit.py` 中的 `esc()` 用于此目的）；CJK 字符的
字宽按 13.2 px 估算，按 8.2 估算会使图例互相重叠；`<text>` 中连续的空格会被 XML
合并为一个，需要留出间隔时应分为两个 `<text>` 元素（见 `bit_fields.py`）；SVG 带有
显式的 `width`/`height`，页面不会缩小它，图的高度超过版面预留的空间时会遮挡正文，
上限约为 260 px（正文三四行时）到 330 px（整页以图为主时）。

画布宽度与页面上的 `width_px` 取同一个值时，图不被缩放，生成器中设定的字号就是它在
页面上的实际字号；两者不等时字号按比例缩小。1280 宽的版面中图的宽度上限约 1150。
专题的四张图按 1000 的画布绘制，页面上按 770–960 显示：这几页图下方还有三到五行正文，
按 1000 显示会把正文挤到页底以外。

`.lk-figure--sized img` 的 `max-height` 是 `none`，给了 `width_px` 的图不会被它的
`<figure>` 限制高度，图长出来的部分会直接画在后面的列表上，而 `<figure>` 自身的
`getBoundingClientRect()` 看不出这一点。检查重叠时要取元素及其后代的并集
（跳过 `position: absolute` 的角标），否则这类遮挡查不出来。

## AI 生成的配图

`assets/` 下的四张 `.jpg` 由本机命令行上的 `codex` 调用其图像生成模型绘制，
提示词要求画面中不出现任何文字。页面上一律用 `.footnote(...)` 标注
「配图由 OpenAI Codex 图像生成模型绘制，仅作概念示意。」，与外部图片同等对待。

| 文件 | 使用页 | 画面内容 |
| --- | --- | --- |
| `quant-cover.jpg` | `topic-cover` | 专题封面：连续波形经过芯片后变为阶梯，输出成一格一格的离散立方体 |
| `alu-floorplan.jpg` | `alu-energy-cont` | 芯片版图：一个 FP32 乘法器的面积上铺满 4×4 共 16 个 INT8 乘加单元 |
| `smoothquant-lens.jpg` | —（原 `smoothquant-cont` 已随第 4 幕删去） | 带离群尖峰的激活波形穿过透镜后变平，流入整型计算核 |
| `closing-bits.jpg` | —（原 `closing` 已按讲师要求删去） | 结语：二进制立方体汇成大模型的注意力拓扑，沿芯片导线铺成的立交桥流向星空 |

生成命令的形式（`codex` 的 stdin 必须关闭，否则它会停在读取输入上）：

```bash
codex exec --skip-git-repo-check --sandbox workspace-write "$PROMPT" < /dev/null
```

提示词末尾都要写明 no text / no lettering / no numerals：模型在图里写出来的字母基本不成词，
留在讲义上不合适。产物按 1600 px 宽转成 JPEG 收进 `assets/`。
这四张图只作概念示意，页面上的数值结论都来自正文与 `examples/` 中实测得到的输出。

## 外部图片

`assets/ext/` 是从外部获取的图片，已逐张核对授权，页面上用 `.footnote(...)` 标注来源。

| 文件 | 使用页 | 来源与授权 |
| --- | --- | --- |
| `zang-binyu.jpg` | `course-info` | 上海交通大学并行与分布式系统研究所成员页，https://ipads.se.sjtu.edu.cn/zh/pub/members/binyu_zang/ |
| `core-memory.jpg` | —（当前不占幻灯片） | Wikimedia Commons，摄影 Mister rf，CC BY-SA 4.0 |
| `ariane-501.jpg` | —（当前不占幻灯片） | **出处与授权未记录**，本表补写时已不在任何页面上使用；重新使用前需要重新核对 |
| `kahan.jpg` | `ieee-history` | Wikimedia Commons，摄影 George Bergman，CC BY-SA 4.0 |
| `patriot-launch.jpg` | —（当前不占幻灯片；原 `patriot-missile`） | Wikimedia Commons（File:Patriot_missile_launch_b.jpg），美国陆军拍摄，公有领域 |
| `rtx5090.jpg` | `bandwidth-ledger-cont` | Wikimedia Commons（File:RTX 5090 - duża wydajność dużym kosztem (2160p 30fps VP9 LQ-96kbit AAC)-00.02.15.468.png），ZMASLO 的评测视频截帧，CC BY 3.0；裁去四周背景，只留显卡与两侧手部 |
| `llm-int8-fig2.svg` | —（当前不占幻灯片） | Dettmers et al., LLM.int8()（NeurIPS 2022）图 2，CC BY 4.0 |
| `tsmc-wordmark.svg` | `alu-energy` | Wikimedia Commons（File:TSMC wordmark.svg），台积电文字商标，公有领域（未达独创性门槛），商标权归台积电所有；原样使用 |
| `memory-wall-profile.png` | —（当前不占幻灯片） | Gholami et al., AI and Memory Wall（IEEE Micro 2024）图 3(b)(d)，CC BY 4.0；裁去两幅子图的标题后横向拼合 |

`memory-wall-profile.png` 取自论文的 arXiv 源码包（`arxiv.org/e-print/2403.14123`）中的
`figs/hardware/mops_new.pdf` 与 `latency_new.pdf`，转换为位图、裁去各自的标题后横向拼合。
CC BY 4.0 允许修改，修改内容在上表与页面脚注中均已注明。

授权分为三类：可自由改编（CC BY、公有领域）、可用但有附加条件（CC BY-SA 需同协议共享，
CC BY-NC-SA 另限非商业，课堂教学符合）、以及版权所有仅课堂合理使用。
页面上用到的图只属于前两类，对外发布的版本无需删除图片（`ariane-501.jpg` 的授权没有记录，
但它不出现在任何页面上）。NVIDIA 与 Google 博客上的位分配图授权属于第三类，
经核对，均未采用，位分配改用自制的 `fp16-bf16-fields.svg`。

`assets/ext/` 下现在有六张不占幻灯片的图（`gguf-spec.jpg`、`core-memory.jpg`、
`ariane-501.jpg`、`llm-int8-fig2.svg`、`memory-wall-profile.png`、`patriot-launch.jpg`）：`gguf-spec.jpg` 与
`core-memory.jpg` 在早先的精简中退场，`ariane-501.jpg` 没有留下使用记录，
`llm-int8-fig2.svg` 与 `memory-wall-profile.png` 随旧量化部分一起退场，
`patriot-launch.jpg` 随重做之前的第五部分退场。除 `ariane-501.jpg` 外，授权都已核对并记在上表中，
重新使用时按原授权标注即可，因此文件保留而不删除。`assets/` 下随旧量化部分退场的六张自制图（`quantize-line`、`q4-block`、
`granularity`、`zero-point`、`q4-k-block`、`k-scales`）连同生成器已删除，
需要时从 `../2-data-bak/` 取回。

## 实验

本讲的实验是 `nano-quant`：读取 `Qwen/Qwen3-VL-2B-Instruct` 的 safetensors，
自行量化为 4 位并运行。题面在习题课目录下，
见 [../2-exe-quant/EXERCISE-nano-quant.md](../2-exe-quant/EXERCISE-nano-quant.md)，
其中有四个部分的任务、三层判定、六个诊断缺陷与考核方式。
框架已完成并在真实模型上验证，见 [../nano-quant/](../nano-quant/)。

`../nano-quant/` 是发给学生的目录：框架、待实现的骨架与判定脚本，C++17，无外部依赖。
`make && make fixtures && sh tests/run.sh` 能在不下载模型的情况下完成一整轮测试，
说明与评分方式见 [../nano-quant/README.md](../nano-quant/README.md)。真实模型上的期望值在
`../nano-quant/tests/expected-qwen3vl.txt`。

`../nano-quant-ta/` 只供助教使用，发布时不包含：其中是完整实现、用它编译三个程序的
`Makefile`，以及重新生成期望值的方法，见 [../nano-quant-ta/README.md](../nano-quant-ta/README.md)。

A 部分的四个转换函数与 Q4_0 的两个块函数在习题课上完成，
见 [../2-exe-quant/](../2-exe-quant/)。这六个函数完成之后 `./nq-selftest a q4_0`
的六项全部与期望值一致，工作量为一次课。Q4_1、Q4_K 与整模型的步骤在题面中。

幻灯片末尾的 `lab-release` 页已与 `nano-quant` 对齐：实验对象为
`Qwen/Qwen3-VL-2B-Instruct`，量化格式包括 Q4_0、Q4_1 与 Q4_K，最后组装并校验 GGUF。

## 英文覆盖层

中文写在 `pages.py` 与 `quant_pages.py` 中，英文放在 `i18n/en.toml`，共 797 条。修改中文之后：

```bash
python3 -m lecturekit.cli i18n extract lectures/2-data --lang en   # 合并新增/变化的条目
python3 -m lecturekit.cli i18n check   lectures/2-data --lang en   # 上课前检查遗漏
```

`extract` 只做合并，已翻译的条目原样保留；基线修改后会标记 `# CHANGED`，
不再属于本讲的条目移到文件末尾的 `# orphaned` 段，不会被删除。
`--strict` 让任何未翻译的条目直接拒绝渲染。

**第五部分与量化专题目前没有完整英文译文**：783 条中有 351 条未翻译，
文件末尾另有 469 条孤立条目，包括旧量化部分、重做之前第五部分，以及
合并续页、重写或删页时留下的条目。未翻译的条目在英文版中回落到中文，
`--strict` 会直接拒绝渲染，因此英文版在翻译补齐之前不要用 `--strict` 出片。

改写专题时给页面换过四个 id（`superblock` → `kquants-superblock`、
`nibble-packing` → `nibble-pack-cpp`、`mixed-recipe` → `mixed-recipe-llama` 及其续页）：
这几个 id 旧量化部分用过，沿用会让旧译文悄悄套在新页面上。第五部分重做时同理，
页面 id 全部换新（`float-distribution` 等旧 id 的译文在孤立段里），节的 id 由 `float-formats` 改为 `float-precision`。衔接页的 id 按位置生成，
换不掉，`bridge-6`、`bridge-7` 的旧译文已清空。**新增页面时先确认 id 没被用过。**

框架不翻译三类内容，因此本讲的英文版中仍有中文：

1. **代码清单**（`p.code` 的正文，含注释）。第一至第四部分的注释一律写英文，中英两版共用，
   `examples/` 下 34 个源文件中，凡是上幻灯片的都没有中文（两个生成脚本 `make_gguf.py`
   与 `make_sample.py` 的注释有中文，它们只在命令行里跑）。两处例外：十六进制页的
   `HEX_GROUPS` 是一张对照表，两行标签「十六进制 / 二进制」在英文版中仍是中文；
   第五部分与量化专题的清单分别沿用 `part-5.md` 与 `quant.md`，其中的注释是中文，两种语言下都显示中文。
   demo 的 `name` 与 `description` 是普通文本，照常翻译。
2. **图片路径**（`p.image` 的 `src`）。`assets/` 下 30 张手写 SVG 的标注是中文，
   英文版中仍显示中文：`memory-bytes`、`byte-order`、`same-bits`、`shifts`、
   `bit-fields`、`float-formats`、`float-spacing`、`address-space`、`expand-truncate`、
   `ulp-spacing`、`fp-absorb`、`round-bias`、`round-grs`、`cancellation`、`cast-paths`、
   `patriot-drift`、`patriot-gate`、`fp16-bf16-fields`、`bf16-datapath`、
   `roofline-latency`、`roofline-shift`、`affine-grid`、`quant-example-task`、`quant-example-zero`、`quant-example-offset`、
   `symmetric-waste`、`gguf-q4-blocks`、
   `q4-dilemma`、`q4k-superblock`、`q4k-scale-bits`。
   `assets/` 下四张 AI 生成的 `.jpg` 里没有文字，两种语言共用。
   与第一讲同样的限制：要让英文版彻底英文，需要框架支持按语言选图。
3. **`p.cite(...)` 与 `p.news(...)`**。参考文献本身不翻译。

第一讲记录的两条经验在本讲同样成立：英文比中文长，修改中文后要运行 `--lang en`
检查是否超出边界；`i18n/en.toml` 中不要在一个列表项内部换行。本讲初稿翻译完成之后有
23 页超出页底，其中「位运算恒等式」一页的 `p.highlight` 完全位于画面之外，
按下面三条修改后才全部回到版面内（1280×720 下，正文区的下沿在 y≈700）：

- **`p.highlight` 不超过 66 个字符**。66 字符为一行，73 字符即为两行，增加的一行占 48 px，
  英文长句最容易在此处超出；本讲有 31 条因此重写。
- **`p.slide` 的列表项不超过 88 个字符**，含行内代码时按等宽字体计算，需要更短。
- **表格单元格按列宽计算**，「位运算恒等式」那张表的说明列约 48 字符一行，名称列约 22 字符。

检查一页是否超出，比查看 PNG 更准确的方法是在渲染出的 `slides.html` 中测量元素位置：
位于画面之外的内容在 PNG 上不显示，只有通过 `getBoundingClientRect()` 才能检测到。

## 用语与排版约定

与第一讲相同，修改本讲时请一并遵守，`pages.py` 顶部也有一份记录。
量化专题以 `quant.md` 为基础，保留其整体用语风格；事实审校只作必要的最小改写。

**用语**：本课件用于本科课程教学，一律采用陈述性的技术表述。不使用比喻、
口语化或生活化的措辞，以及「不是……而是……」与「只做 A，不做 B」一类先肯定再否定的对比句式。结论写成可以直接复述的判断句。

**排版**：

- **粗体后面不要紧跟全角冒号**，写成 `**词**：内容`；
- **尖括号要放在行内代码里**：`p.cite(title=...)`、表格与正文中的 `vector<bool>` 若不加
  反引号会被当作 HTML 标签丢弃；`p.title(...)` 中加反引号也无效，标题中避免尖括号；
- **`==标记==` 只在 `p.slide(...)` 中展开**，写入 `p.highlight(...)`、图注、
  表格单元格会成为字面量；标记内部也**不能包含行内代码**，
  `` ==仍落在 `int` 的范围内== `` 会原样输出两对等号；
- **`p.slide(...)` 中的换行就是换行**，一段话不要在源码中手动折行；
- **一页最多一个 `p.highlight`**；
- 代码清单控制在 **10 行以内**。本讲有五页因为清单过长超出页底，
  最终把 `show_bytes` 从 12 行缩减到 6 行、`quantize` 从 21 行缩减到 10 行；
- **`**` 前面是全角标点时闭不上**：CommonMark 的 flanking 规则里，
  `采用**标定集（Calibration Set）**进行` 中后一对 `**` 左边是 `）`、右边是汉字，
  既不能开也不能闭，两个星号会原样打在屏幕上。文案不能改时写成 `<strong>…</strong>`；
- **`p.code(..., mark=[...])` 只对 `language="pseudo"` 有效**，
  其他语言由渲染器着色，没有地方叠高亮，`inspect` 会直接报错；
- 修改之后运行 `--png`，检查是否有内容越过页底（1280×720 下 y≈690 是边界）；
  版面偏空的页用 `p.gap(n)` 下移内容，本讲的正文页大多占页高的 60%–90%。
  量化专题的续页普遍偏空（35%–50%），因为拆页的位置由文案的层级决定，不能为了填满而调整。

## 与参考资料的关系

- `refs/advice/1. 数据类型和位运算.md` 提供了本讲的取材范围：
  字符编码与 token 的类比、低精度浮点格式表、量化方法、int4 的位运算例子。
  其中 int4 加法的示例代码 `overflow = a & b & 0b1000` 不正确（它不等于 bit 3 的进位），
  `examples/int4.c` 改用 `s = (a & 0x77) + (b & 0x77); s ^ ((a ^ b) & 0x88)`，
  穷举测试验证该式在全部 65536 对输入上错误数为 0。讲这一例子的 SWAR 页已随第 4 幕删去。
- `refs/advice/ICS课程现况和改革建议.md` 提供了实验设计的框架：实验按观察型 / 构造型 /
  诊断修复型 / 优化型 / 对抗取证型分类，允许学生使用 AI 但把评价锚点移回学生本人。
- `quant.md`（仓库根目录）是量化专题的文案来源，26 张 slide 中保留 16 张（第 10、15、16–20、23、24、26 张删去）。
  它自带每页的「配图要求」「配套可运行演示」「讲师讲稿与互动」「文献出处」，
  分别落到 `p.image`、`p.demo`、`p.notes` 与页脚。2026-09 的本轮审校已复核课堂上使用的
  标准、论文、硬件规格与 llama.cpp 格式说明，并修正了不受原出处支持的绝对化表述。
- CS:APP 第 2 章是内容基线。整数与浮点的完整定理证明、IEEE 754 的舍入规则细节
  有意留给教材，本讲只选取能应用于该文件的部分。
