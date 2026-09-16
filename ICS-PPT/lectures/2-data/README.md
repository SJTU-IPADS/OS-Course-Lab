# ICS 第二讲 · 数据的表示

以第一讲引入的模型权重文件为观察对象，回答「文件中的字节表示什么」。

中文是基线（写在 Python 中），英文是 `i18n/en.toml` 翻译覆盖层，一份源码对应两种语言。

## 结构

引言把问题分为四个：一段位如何表示一个值、多字节对象如何排列、同一段位有几种解释、
1.9 GB 如何计算。随后六节各回答一部分，节与节之间用 `lecture.bridge(...)` 过渡。
每一节的结论都应用于同一个文件，最后一页总结为四个答案。第一讲中 1.9 GB 的来源
在「精度决定模型的体积」一页按单一 4 位格式计算为 1.81 GB；实际文件是 Q4_K_M 混合配方，
平均 5.01 位/权重、2.02 GB = 1.88 GiB，这组数据放在该页的备注中，课上按需使用。

| 节 | 页数 | 内容 |
| --- | --- | --- |
| 回顾与本节的问题 | 2 | 第一讲的权重文件 · 四个问题 |
| 第一部分 · 位与字节 | 9 | 位串到值 · 十六进制与 C 的进制写法 · `xxd` · 模型格式如何标识自己 · 内存即字节 · 字长与地址范围 · C 数据类型的宽度 · 布尔值的存储 · bitset 与 vector<bool> |
| 第二部分 · 字节序 | 7 | 内存即字节 · 字长 · 大端与小端 · 读一个字段 · `show_bytes` · 在本机上看见字节序 · 文本与 token |
| **第三部分 · 整数** | **16** | 无符号与补码 · 取值范围 · 强制转换 · 比较陷阱 · 越界与内核缺陷 · 扩展与截断 · 一次真实的截断 · 大小不同的操作数比较 · 截断与字节序 · 位运算 · 移位 · 逻辑与算术右移 · 运算符优先级与实例 · 位运算恒等式 |
| 第四部分 · 浮点数与低精度格式 | 12 | 三段结构与偏置 · 二进制科学计数法 · 规格化/非规格化/inf/NaN · 值在数轴上的分布与实测间距 · 舍入 · 浮点不是实数 · FP32/BF16/FP16/TF32/FP8/FP4 · BF16 即截断 · FP16 的编码与次规格化数 · 范围与精度 · 模型尺寸 |
| 第五部分 · 量化：原理与 Q4_0 | 12 | 为什么要量化 · 访存瓶颈的实测 · 量化谁 · 量化的想法 · 仿射映射的三个自由度 · Q4_0 的块结构 · 量化代码 · 4 位打包 · 打包加法 · 硬件上的 4 位运算单元 · 粒度 · 三种粒度的实测 |
| 第六部分 · 量化格式：Q4_1 与 Q4_K | 11 | 偏移与 Q4_1 · Q4_1 的实测 · Q4_K 的超块 · 超块的字节账 · 12 字节里的 16 个 6 位数 · 取出子块系数的代码 · Q4_K 的实测 · 文件里的混合配方 · 量化的代价 · 线性量化以及它之外的做法 · 实验预告 |

共 69 个编号页（含末页的本节小结与自动生成的参考文献页）、6 个衔接页与 1 个封面。

量化占两节，按 1.5–2 次课的讲解时长安排：第五部分讲解一个 4 位格式的设计，
第六部分讲解真实文件中存在多种格式的原因。两节之间的衔接页适合作为下课的分界。

浮点数只在本讲讲解，课程中没有单独的浮点专题，因此 IEEE 754 的编码规则、
非规格化数、特殊值与舍入模式都在第四部分讲解完毕。

第三部分保留了 CS:APP 的完整基线（无符号与补码、强制转换、`copy_from_kernel`
的越界缺陷、扩展与截断、位运算与移位），并在此基础上增加了
4 位量化所需的打包与双通道加法。

## 课上运行命令

本讲有 22 个 `p.demo(...)`，全部在 `examples/` 下实际运行：

```bash
python3 -m lecturekit.cli view lectures/2-data --watch
```

投影上显示命令与录制的输出，按 ▶ 即在讲义目录中实际运行（`view --watch` 下按钮可用，
渲染出的静态包中按钮不可用）。所有输出都是在
本机上实际执行得到的。`examples/` 中每个 `.c` 都与幻灯片上
显示的清单一致。修改幻灯片上的代码时，源文件要同步修改，然后重新运行并核对输出。

每个 `p.demo(...)` 用 `files=[...]` 声明了它编译的源文件，▶ 旁边因此有一个以
文件名为标签的按钮，按下后在右侧展开该文件的全文（带行号），课上可以直接查看代码。
文件在按下时才读取，修改源文件后再按一次即显示新的内容。

## 跨平台：x86-64 Linux、arm64 macOS 与 x86-64 Windows

本讲讲的是数据表示，命令在三种机器上都能直接执行、结果一致：x86-64 的 Linux、
arm64 的 macOS，以及 Windows 的 WSL2（Ubuntu，命令与 Linux 完全相同）。
为此做过三处调整：

| 页 | 调整 | 原因 |
| --- | --- | --- |
| `recap-weights` | `ls -lhS ... \| sed -n '2p'` + `file "$(ls -dS ... \| head -1)"` | macOS 的 `ls -l` 按 512 字节块打印 `total` 行；`sha256-*` 按字母序会先匹配到 JSON manifest。与第一讲的同一条命令保持一致 |
| `vector-bool` | `grep -oE 'cannot (convert\|initialize).*'` | 同一处错误 gcc 说 cannot convert，clang 说 cannot initialize |
| `shift-kinds` | grep 的字母表加上 `asr` / `lsr` / `rev`，标号加 `^_?` | arm64 的助记符与 x86-64 不同，Mach-O 的符号名前多一个下划线 |

随平台变化、需要在课上说明的三点（都写在对应页的 `p.notes` 里）：

- **`long` 的宽度**：64 位 Linux 与 macOS 是 LP64（8 字节），原生 Windows 是 LLP64（4 字节，
  指针仍是 8 字节）。`c-data-sizes` 表中「64 位」一列按 LP64 写。WSL2 中是 Linux 程序，与表一致。
- **右移的指令名**：x86-64 是 `sar` / `shr`，arm64 是 `asr` / `lsr`。

三种平台都是小端，本讲字节序部分的全部输出一致。`p.demo` 里录的输出来自 x86-64 Linux。

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

| 文件 | 使用页 | 说明 |
| --- | --- | --- |
| `make_gguf.py` | — | 生成 `tiny.gguf` |
| `make_formats.py` | — | 生成 `tiny.safetensors` / `tiny.onnx` / `tiny.pkl` / `tiny.zip`（ONNX 文件需要 onnx 包） |
| `tiny.safetensors`、`tiny.onnx`、`tiny.pkl`、`tiny.zip` | `other-formats` | 四种格式的开头各 16 字节，都按各自规则写出，作为源码提交 |
| `tiny.gguf` | `hexdump`、`endianness-visible`、`read-the-header` | 224 字节的真实 GGUF v3 文件，作为源码提交 |
| `gguf_head.c` | `endianness-visible` | 按两种字节序读同一个 `version` 字段 |
| `show_bytes.c` | `show-bytes` | CS:APP 的 `show_bytes`，两参数版本 |
| `sizes.c` | `c-data-sizes` | 各类型的 `sizeof` |
| `bitset_demo.cpp` | `vector-bool` | 一百万个布尔值在 `vector<bool>` / `bitset` / `deque<bool>` 中占的字节数 |
| `vector_bool_bad.cpp` | `vector-bool` | 有意无法编译：`&v[0]` 的类型不是 `bool *` |
| `bool_size.c` | `bool-storage` | 同一份代码按 C 与 C++ 编译，比较 `bool` 的宽度与取值（需要 g++） |
| `compare.c` | `comparison-trap` | 有符号与无符号混用的四个比较 |
| `truncate.c` | `truncation-in-practice` | 扩展、截断与同一段字节的两种读法 |
| `truncate_endian.c` | `truncation-and-endianness` | 按值截断与按字节取前缀，在两种排列下的结果 |
| `shift_kind.c` | `shift-kinds` | 有符号与无符号右移，以及编译出的 `sar` / `shr` |
| `precedence.c` | `precedence-in-practice` | 三个缺少括号的表达式与 `-Wall` 的三条警告 |
| `bit_rules.c` | —（不占幻灯片） | 穷举 65536 组字节验证 `bit-identities` 那页的恒等式，课上有提问时可以当场运行 |
| `binary_point.c` | `binary-scientific` | 十进制值写为二进制并移动小数点，得到阶码与尾数 |
| `float_spacing.c` | `float-spacing-measured` | 用 `nextafterf` 取相邻 FP32，输出步长与相对步长（需要 `-lm`） |
| `gguf_bits.py` | —（不占幻灯片） | 读整个权重文件，按张量类型统计位数与字节数；`model-size` 的备注引用了它的结论 |
| `float_law.c` | `float-not-real` | 浮点结合律失效与 `0.1 + 0.2` 的舍入 |
| `rounding.c` | `float-rounding` | 舍入到最近的偶数，以及 `0.1` 的近似值 |
| `fp16_range.c` | `range-and-precision` | 实际的 `_Float16` / `__bf16` 转换（需要 gcc 12+，x86-64） |
| `bf16.c` | `bf16-truncation` | 逐位打印 FP32 与 BF16 |
| `fp16_classes.c` | `fp16-classes` | 把七个位模式复制进 `_Float16` 输出，覆盖次规格化、规格化、无穷与 NaN（需要 gcc 12+，x86-64） |
| `int4.c` | —（不占幻灯片） | 打包加法，穷举 65536 对输入；`nibble-add` 的备注中引用了它的结论 |
| `quantize.c` | `quantize-code` | Q4_0 的量化与反量化，输出误差 |
| `quant_compare.c` | `granularity-measured`、`q4-1-measured` | 五种方案（per-tensor / per-256 / Q4_0 / Q4_1 / Q4_K）在真实权重上的字节数与误差；`q4-k-measured` 的对照表数据也来自它（需要 gcc 12+，x86-64） |
| `k_scales.c` | —（不占幻灯片） | Q4_K 的 12 字节打包，100 万组随机值验证 `put`/`get` 互逆；`k-scales-code` 的备注中提到它，课上有提问时可以当场运行 |
| `make_sample.py` | — | 用两次 HTTP Range 请求获取下面两段权重；也是 safetensors 头的最小读法示例 |
| `ext/w-down-proj.bf16`、`ext/w-final-norm.bf16` | 同 `quant_compare.c` | Llama-3.2-1B 的两段真实权重，各数 KB，出处与授权见 `ext/PROVENANCE.md` |

`fp16_range.c`、`fp16_classes.c` 用了 `_Float16`（前者还用了 `__bf16`），这两个类型在旧编译器上不存在，
源文件头部已注明。其余例子只用 C99。

## 图表

`diagrams/` 是自制图的源文件，`assets/` 中的同名 `.svg` 是产物，不要手工修改：

```bash
lectures/2-data/diagrams/render.sh    # 重新生成全部图表
```

| 源文件 | 产物 | 使用页 |
| --- | --- | --- |
| `svgkit.py` | — | 共用的图元：`cells`、`brace`、`text`、`rect`、`esc` |
| `check_bounds.py` | — | 检查图中是否有文字超出画布，`render.sh` 末尾自动运行 |
| `byte_order.py` | `byte-order.svg` | `endianness`，同一个值在两种排列下的四个地址 |
| `memory_bytes.py` | `memory-bytes.svg` | `memory-as-bytes`，12 个地址格与跨 4 格的 `int` |
| `address_space.py` | `address-space.svg` | `word-size`，4 GB 的横条与占用近一半空间的权重文件 |
| `same_bits.py` | `same-bits.svg` | `casting`，同一串 16 位的两种解释 |
| `expand_truncate.py` | `expand-truncate.svg` | `expand-truncate`，补入的字节与丢弃的字节 |
| `shifts.py` | `shifts.svg` | `shifts`，一个位串的三种移位与补入的位 |
| `bit_fields.py` | `bit-fields.svg` | `float-structure`，3.1415927 的 FP32 三段分解 |
| `float_formats.py` | `float-formats.svg` | `precision-formats`，七种格式的三段分配 |
| `float_spacing.py` | `float-spacing.svg` | `float-distribution`，可表示的值在 0–8 上随阶码增大而变稀疏 |
| `quantize_line.py` | `quantize-line.svg` | `quantization-idea`，16 个等距级与半步长误差 |
| `q4_block.py` | `q4-block.svg` | `q4-block`，18 字节的块结构与一个字节中的两个权重 |
| `int4_accumulate.py` | `int4-accumulate.svg` | `int4-hardware`，共用字节的进位与 32 位累加器的对照 |
| `granularity.py` | `granularity.svg` | `granularity`，同一组权重按整张量 / 256 / 32 分组的位宽与误差 |
| `zero_point.py` | `zero-point.svg` | `zero-point`，Q4_0 与 Q4_1 的可表示级在同一段数轴上的分布 |
| `q4_k_block.py` | `q4-k-block.svg` | `superblock`，144 字节按比例绘制的 `block_q4_K` |
| `k_scales.py` | `k-scales.svg` | `k-scales-layout`，16 个 6 位数在 12 字节中的位置 |

`float-formats.svg` 有意不绘制 FP64（它在深度学习中很少使用，改由页上的 `p.aside` 简要说明），
并把 FP32 与 BF16 排在相邻两行，以便绘制 bit 16 处的截断线。

编写这类生成器时注意四点：SVG 文本中的 `&`、`<`、`>` 必须转义，否则整张图会因为
不是良构 XML 而被静默丢弃（`svgkit.py` 中的 `esc()` 用于此目的）；CJK 字符的
字宽按 13.2 px 估算，按 8.2 估算会使图例互相重叠；`<text>` 中连续的空格会被 XML
合并为一个，需要留出间隔时应分为两个 `<text>` 元素（见 `bit_fields.py`）；SVG 带有
显式的 `width`/`height`，页面不会缩小它，图的高度超过版面预留的空间时会遮挡正文，
上限约为 260 px（正文三四行时）到 330 px（整页以图为主时）。

画布宽度与页面上的 `width_px` 取同一个值时，图不被缩放，生成器中设定的字号就是它在
页面上的实际字号；两者不等时字号按比例缩小。量化部分的三张图（`granularity`、
`zero-point`、`q4-k-block`）原先以 920 的画布按 790–880 的宽度显示，字号偏小，
后来统一改为画布 1000、显示 1000。1280 宽的版面中图的宽度上限约 1150。

## 外部图片

`assets/ext/` 是从外部获取的图片，已逐张核对授权，页面上用 `.footnote(...)` 标注来源。

| 文件 | 使用页 | 来源与授权 |
| --- | --- | --- |
| `ariane-501.jpg` | `truncation-in-practice` | Wikimedia Commons，阿丽亚娜 501 残骸，公有领域 |
| `kahan.jpg` | `float-rounding` | Wikimedia Commons，摄影 George Bergman，CC BY-SA 4.0 |
| `llm-int8-fig2.svg` | `quantization-cost` | Dettmers et al., LLM.int8()（NeurIPS 2022）图 2，CC BY 4.0 |
| `memory-wall-profile.png` | `memory-bound-measured` | Gholami et al., AI and Memory Wall（IEEE Micro 2024）图 3(b)(d)，CC BY 4.0；裁去两幅子图的标题后横向拼合 |

`memory-wall-profile.png` 取自论文的 arXiv 源码包（`arxiv.org/e-print/2403.14123`）中的
`figs/hardware/mops_new.pdf` 与 `latency_new.pdf`，转换为位图、裁去各自的标题后横向拼合。
CC BY 4.0 允许修改，修改内容在上表与页面脚注中均已注明。

授权分为三类：可自由改编（CC BY、公有领域）、可用但有附加条件（CC BY-SA 需同协议共享，
CC BY-NC-SA 另限非商业，课堂教学符合）、以及版权所有仅课堂合理使用。
本讲只使用前两类，对外发布的版本无需删除图片。NVIDIA 与 Google 博客上的位分配图授权属于第三类，
经核对，均未采用，位分配改用自制的 `float-formats.svg`。

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

幻灯片末尾的 `lab-preview` 页是 `nano-quant` 的预告，只介绍学生需要完成的四项任务，
细节以实验说明为准。`assets/ext/gguf-spec.jpg` 与 `read-the-header` 页的逐字段表
在早先的一次精简中删除，图片文件仍在 `assets/ext/` 下，重新使用时按原授权标注即可。

## 英文覆盖层

中文写在 `pages.py` 中，英文放在 `i18n/en.toml`，共 636 条。修改中文之后：

```bash
python3 -m lecturekit.cli i18n extract lectures/2-data --lang en   # 合并新增/变化的条目
python3 -m lecturekit.cli i18n check   lectures/2-data --lang en   # 上课前检查遗漏
```

`extract` 只做合并，已翻译的条目原样保留；基线修改后会标记 `# CHANGED`。
`--strict` 让任何未翻译的条目直接拒绝渲染。

框架不翻译三类内容，因此本讲的英文版中仍有中文：

1. **代码清单**（`p.code` 的正文，含注释）。本讲的注释一律写英文，中英两版共用，
   `examples/` 下 30 个源文件中没有中文。唯一的例外是十六进制页的
   `HEX_GROUPS`：它是一张对照表，两行标签「十六进制 / 二进制」
   在英文版中仍是中文。demo 的 `name` 与 `description` 是普通文本，照常翻译。
2. **图片路径**（`p.image` 的 `src`）。`assets/` 下 16 张手写 SVG 的标注是中文，
   英文版中仍显示中文：`memory-bytes`、`byte-order`、`same-bits`、`shifts`、
   `expand-truncate`、`bit-fields`、`float-formats`、`float-spacing`、
   `quantize-line`、`q4-block`、`zero-point`、`q4-k-block`、`k-scales`、
   `granularity`、`int4-accumulate`、`address-space`。
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
- 修改之后运行 `--png`，检查是否有内容越过页底（1280×720 下 y≈690 是边界）；
  版面偏空的页用 `p.gap(n)` 下移内容，本讲的正文页大多占页高的 60%–90%。

## 与参考资料的关系

- `refs/advice/1. 数据类型和位运算.md` 提供了本讲的取材范围：
  字符编码与 token 的类比、低精度浮点格式表、量化方法、int4 的位运算例子。
  其中 int4 加法的示例代码 `overflow = a & b & 0b1000` 不正确（它不等于 bit 3 的进位），
  本讲改用 `s = (a & 0x77) + (b & 0x77); s ^ ((a ^ b) & 0x88)`，
  并用 `int4.c` 的穷举测试验证：该式在全部 65536 对输入上错误数为 0。
- `refs/advice/ICS课程现况和改革建议.md` 提供了实验设计的框架：实验按观察型 / 构造型 /
  诊断修复型 / 优化型 / 对抗取证型分类，允许学生使用 AI 但把评价锚点移回学生本人。
- CS:APP 第 2 章是内容基线。整数与浮点的完整定理证明、IEEE 754 的舍入规则细节
  有意留给教材，本讲只选取能应用于该文件的部分。
