# ICS 第二讲 · 数据的表示

以第一讲留下的模型权重文件为观察对象，回答「文件里的字节表示什么」。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层 —— 一份源码，两种语言。

## 结构

引子把问题拆成四个：一段位怎么变成一个值、多字节对象怎么排列、同一段位有几种解释、
1.9 GB 是怎么算出来的。随后六节各回答一部分，节与节之间用 `lecture.bridge(...)` 过渡。
每一节的结论都落回同一个文件，最后一页收束到四个答案。第一讲那个 1.9 GB 的由来
在「精度决定模型的体积」一页按单一 4 位格式算到 1.81 GB；实际文件是 Q4_K_M 混合配方，
平均 5.01 位/权重、2.02 GB = 1.88 GiB，这组数放在该页的备注里，讲台上按需取用。

| 节 | 页数 | 落点 |
| --- | --- | --- |
| 回顾与本节的问题 | 2 | 第一讲的权重文件 · 四个问题 |
| 第一部分 · 位与字节 | 10 | 二进制 · 位串到值 · 十六进制与 C 的进制写法 · `xxd` · 模型格式如何标识自己 · 内存即字节 · 字长与地址范围 · C 数据类型的宽度 · 布尔值的存储 · bitset 与 vector<bool> |
| 第二部分 · 字节序 | 7 | 大端与小端 · 读一个字段 · `show_bytes` · 在本机上看见字节序 · Linux 中的处理方式 · 转换的编译结果 · 文本与 token |
| **第三部分 · 整数** | **15** | 无符号与补码 · 取值范围 · 强制转换 · 比较陷阱 · 越界与内核缺陷 · 扩展与截断 · 一次真实的截断 · 截断与字节序 · 位运算 · 移位 · 逻辑与算术右移 · 运算符优先级与实例 · 位运算恒等式 |
| 第四部分 · 浮点数与低精度格式 | 11 | 三段结构与偏置 · 二进制科学计数法 · 规格化/非规格化/inf/NaN · 值在数轴上的分布与实测间距 · 舍入 · 浮点不是实数 · FP32/BF16/FP16/TF32/FP8/FP4 · BF16 即截断 · 范围与精度 · 模型尺寸 |
| 第五部分 · 量化：原理与 Q4_0 | 12 | 为什么要量化 · 访存瓶颈的实测 · 量化谁 · 量化的想法 · 仿射映射的三个自由度 · Q4_0 的块结构 · 量化代码 · 4 位打包 · 打包加法 · 硬件上的 4 位运算单元 · 粒度 · 三种粒度的实测 |
| 第六部分 · 量化格式：Q4_1 与 Q4_K | 11 | 偏移与 Q4_1 · Q4_1 的实测 · Q4_K 的超块 · 超块的字节账 · 12 字节里的 16 个 6 位数 · 取出子块系数的代码 · Q4_K 的实测 · 文件里的混合配方 · 量化的代价 · 线性量化以及它之外的做法 · 实验预告 |

共 70 个编号页（含末页的本节小结与自动生成的参考文献页）、6 个衔接页与 1 个封面。

量化占两节，是按 1.5–2 次课的讲解时长安排的：第五部分讲清楚一个 4 位格式怎么来，
第六部分讲真实文件里的格式为什么不止一种。两节之间的衔接页是自然的下课点。

浮点数只在这一讲讲，后面没有单独的浮点专题，因此 IEEE 754 的编码规则、
非规格化数、特殊值与舍入模式都在第四部分讲完。

第三部分保留了 CS:APP 的完整基线（无符号与补码、强制转换、`copy_from_kernel`
那个越界缺陷、扩展与截断、位运算与移位），并在其上接了 AI 时代的内容：
张量索引在 2048×2048×512 处恰好越过 32 位补码的上限，
以及 4 位量化所需的打包与双通道加法。

## 上课时跑命令

本讲有 25 个 `p.demo(...)`，全部在 `examples/` 下真跑：

```bash
python3 -m lecturekit.cli view lectures/2-data --watch
```

投影上显示命令与录下来的输出，按 ▶ 就在讲义目录里真跑一遍（`view --watch` 下按钮即可按，
渲染出来的静态包里按钮是灰的）。所有输出都是在
这台机器上实际执行得到的，不是编造的。`examples/` 里每个 `.c` 都与幻灯片上
显示的清单一致——改幻灯片上的代码时，源文件要一起改，然后重跑一遍核对输出。

每个 `p.demo(...)` 用 `files=[...]` 声明了它编译的源文件，▶ 旁边因此有一个以
文件名为标签的按钮，按下后在右侧展开该文件的全文（带行号），课上可以直接看代码。
文件在按下时才读取，改完源文件再按一次就是新的内容。

## 构建

```bash
python3 -m lecturekit.cli view   lectures/2-data --watch      # 边改边看
python3 -m lecturekit.cli render lectures/2-data --pdf        # PDF
python3 -m lecturekit.cli render lectures/2-data --to pptx    # 可编辑的 PPTX
python3 -m lecturekit.cli render lectures/2-data --png        # 逐页 PNG，用来查版面溢出
python3 -m lecturekit.cli view   lectures/2-data --watch --lang en   # 英文
```

## 例子

`examples/` 是所有可运行代码的目录。编译产物已在 `.gitignore` 里。

| 文件 | 用在 | 说明 |
| --- | --- | --- |
| `make_gguf.py` | — | 生成 `tiny.gguf` |
| `make_formats.py` | — | 生成 `tiny.safetensors` / `tiny.onnx` / `tiny.pkl` / `tiny.zip`（ONNX 那个需要 onnx 包） |
| `tiny.safetensors`、`tiny.onnx`、`tiny.pkl`、`tiny.zip` | `other-formats` | 四种格式的开头各 16 字节，都按各自规则写出，作为源码提交 |
| `tiny.gguf` | `hexdump`、`endianness-visible`、`read-the-header` | 224 字节的真实 GGUF v3 文件，作为源码提交 |
| `gguf_head.c` | `endianness-visible` | 按两种字节序读同一个 `version` 字段 |
| `show_bytes.c` | `show-bytes` | CS:APP 的 `show_bytes`，两参数版本 |
| `sizes.c` | `c-data-sizes` | 各类型的 `sizeof` |
| `bitset_demo.cpp` | `vector-bool` | 一百万个布尔值在 `vector<bool>` / `bitset` / `deque<bool>` 中占的字节数 |
| `vector_bool_bad.cpp` | `vector-bool` | 故意编译不过：`&v[0]` 得不到 `bool *` |
| `bool_size.c` | `bool-storage` | 同一份代码按 C 与 C++ 编译，比较 `bool` 的宽度与取值（需要 g++） |
| `compare.c` | `comparison-trap` | 有符号与无符号混用的四个比较 |
| `truncate.c` | `truncation-in-practice` | 扩展、截断与同一段字节的两种读法 |
| `endian_host.c` | `endianness-in-linux` | `__BYTE_ORDER__` 与 `htole32` / `htobe32` 各自的结果 |
| `endian_calls.c` | `endianness-conversion-cost` | 两个转换函数编译出的指令（配合 `gcc -S`） |
| `truncate_endian.c` | `truncation-and-endianness` | 按值截断与按字节取前缀，在两种排列下的结果 |
| `shift_kind.c` | `shift-kinds` | 有符号与无符号右移，以及编译出的 `sar` / `shr` |
| `precedence.c` | `precedence-in-practice` | 三个漏括号的表达式与 `-Wall` 的三条警告 |
| `bit_rules.c` | —（不占幻灯片） | 穷举 65536 组字节验证 `bit-identities` 那页的恒等式，课上有人问起可以现跑 |
| `binary_point.c` | `binary-scientific` | 十进制值写成二进制并移动小数点，得到阶码与尾数 |
| `float_spacing.c` | `float-spacing-measured` | 用 `nextafterf` 取相邻 FP32，报告步长与相对步长（需要 `-lm`） |
| `gguf_bits.py` | —（不占幻灯片） | 读整个权重文件，按张量类型统计位数与字节数；`model-size` 的备注引用了它的结论 |
| `float_law.c` | `float-not-real` | 整数溢出与浮点结合律失效 |
| `rounding.c` | `float-rounding` | 舍入到最近的偶数，以及 `0.1` 的近似值 |
| `fp16_range.c` | `range-and-precision` | 真的 `_Float16` / `__bf16` 转换（需要 gcc 12+，x86-64） |
| `bf16.c` | `bf16-truncation` | 逐位打印 FP32 与 BF16 |
| `int4.c` | —（不占幻灯片） | 打包加法，穷举 65536 对输入；`nibble-add` 的备注里引用了它的结论 |
| `quantize.c` | `quantize-code` | Q4_0 的量化与反量化，报告误差 |
| `quant_compare.c` | `granularity-measured`、`q4-1-measured` | 五种方案（per-tensor / per-256 / Q4_0 / Q4_1 / Q4_K）在真实权重上的字节数与误差；`q4-k-measured` 那张对照表的数据也出自它（需要 gcc 12+，x86-64） |
| `k_scales.c` | —（不占幻灯片） | Q4_K 的 12 字节打包，100 万组随机值验证 `put`/`get` 互逆；`k-scales-code` 的备注里提到它，课上有人问起可以现跑 |
| `make_sample.py` | — | 用两次 HTTP Range 请求取下面两段权重；也是 safetensors 头的最小读法示例 |
| `ext/w-down-proj.bf16`、`ext/w-final-norm.bf16` | 同 `quant_compare.c` | Llama-3.2-1B 的两段真实权重，各数 KB，出处与授权见 `ext/PROVENANCE.md` |

`fp16_range.c` 用了 `_Float16` 与 `__bf16`，这两个类型在旧编译器上不存在，
源文件头部已注明。其余例子只用 C99。

## 图表

`diagrams/` 是自制图的源文件，`assets/` 里的同名 `.svg` 是产物，不要手改：

```bash
lectures/2-data/diagrams/render.sh    # 重新生成全部图表
```

| 源文件 | 产物 | 用在 |
| --- | --- | --- |
| `svgkit.py` | — | 共用的图元：`cells`、`brace`、`text`、`rect`、`esc` |
| `check_bounds.py` | — | 检查图里有没有文字跑出画布，`render.sh` 末尾自动跑 |
| `byte_order.py` | `byte-order.svg` | `endianness`，同一个值在两种排列下的四个地址 |
| `memory_bytes.py` | `memory-bytes.svg` | `memory-as-bytes`，12 个地址格与跨 4 格的 `int` |
| `address_space.py` | `address-space.svg` | `word-size`，4 GB 的横条与占掉近一半的权重文件 |
| `same_bits.py` | `same-bits.svg` | `casting`，同一串 16 位的两种读法 |
| `expand_truncate.py` | `expand-truncate.svg` | `expand-truncate`，补入的字节与丢弃的字节 |
| `shifts.py` | `shifts.svg` | `shifts`，一个位串的三种移位与补入的位 |
| `bit_fields.py` | `bit-fields.svg` | `float-structure`，3.1415927 的 FP32 三段分解 |
| `float_formats.py` | `float-formats.svg` | `precision-formats`，七种格式的三段分配 |
| `float_spacing.py` | `float-spacing.svg` | `float-distribution`，可表示的值在 0–8 上随阶码变稀 |
| `quantize_line.py` | `quantize-line.svg` | `quantization-idea`，16 个等距级与半步长误差 |
| `q4_block.py` | `q4-block.svg` | `q4-block`，18 字节的块结构与一个字节里的两个权重 |
| `int4_accumulate.py` | `int4-accumulate.svg` | `int4-hardware`，共用字节的进位与 32 位累加器的对照 |
| `granularity.py` | `granularity.svg` | `granularity`，同一组权重按整张量 / 256 / 32 分组的位宽与误差 |
| `zero_point.py` | `zero-point.svg` | `zero-point`，Q4_0 与 Q4_1 的可表示级落在同一段数轴上 |
| `q4_k_block.py` | `q4-k-block.svg` | `superblock`，144 字节按比例画出的 `block_q4_K` |
| `k_scales.py` | `k-scales.svg` | `k-scales-layout`，16 个 6 位数在 12 字节里的位置 |

`float-formats.svg` 有意不画 FP64（它在深度学习里很少出现，改用页上的 `p.aside` 一句带过），
并把 FP32 与 BF16 排在相邻两行，好画出 bit 16 处那道截断线。

写这类生成器时注意四点：SVG 文本里的 `&`、`<`、`>` 必须转义，否则整张图会因为
不是良构 XML 而被静默丢弃（`svgkit.py` 里的 `esc()` 就是为此）；CJK 字符的
字宽按 13.2 px 估算，用 8.2 会让图例互相压住；`<text>` 里连续的空格会被 XML
折叠成一个，要留空隙就分成两个 `<text>` 元素（见 `bit_fields.py`）；SVG 带着
显式的 `width`/`height`，页面不会把它缩小，图比版面留给它的空间高就会压住正文，
上限约为 260 px（正文三四行时）到 330 px（整页以图为主时）。

画布宽度与页面上的 `width_px` 取同一个值时，图不被缩放，生成器里写的字号就是它在
页面上的实际字号；两者不等时字号按比例缩小。量化部分的三张图（`granularity`、
`zero-point`、`q4-k-block`）原先按 920 的画布放在 790–880 的位置上显示，字偏小，
后来一律改成 1000 对 1000。1280 宽的版面留给图的宽度上限约 1150。

## 外部图片

`assets/ext/` 是从外部取来的图片，逐张核对过授权，页面上用 `.footnote(...)` 标注来源。

| 文件 | 用在 | 来源与授权 |
| --- | --- | --- |
| `core-memory.jpg` | `why-binary` | Wikimedia Commons，摄影 Mister rf，CC BY-SA 4.0 |
| `ariane-501.jpg` | `truncation-in-practice` | Wikimedia Commons，阿丽亚娜 501 残骸，公有领域 |
| `kahan.jpg` | `float-rounding` | Wikimedia Commons，摄影 George Bergman，CC BY-SA 4.0 |
| `llm-int8-fig2.svg` | `quantization-cost` | Dettmers et al., LLM.int8()（NeurIPS 2022）图 2，CC BY 4.0 |
| `memory-wall-profile.png` | `memory-bound-measured` | Gholami et al., AI and Memory Wall（IEEE Micro 2024）图 3(b)(d)，CC BY 4.0；裁去两幅子图的标题后横向拼合 |

`memory-wall-profile.png` 取自论文的 arXiv 源码包（`arxiv.org/e-print/2403.14123`）里的
`figs/hardware/mops_new.pdf` 与 `latency_new.pdf`，转成位图、裁去各自的标题后横向拼合。
CC BY 4.0 允许修改，改动在上表与页面脚注里都写明了。

授权分三档：可自由改编（CC BY、公有领域）、可用但有附加条件（CC BY-SA 需同协议共享，
CC BY-NC-SA 另限非商业，课堂教学符合）、以及版权所有仅课堂合理使用。
本讲只用前两档，对外发布的版本不必删图。NVIDIA 与 Google 博客上的位分配图授权属第三档，
已核对过，一律没有采用，位分配改用自制的 `float-formats.svg`。

## 实验

本讲有两份实验设计文档与一份习题课讲义：

| 文档 | 实验 | 方向 | 状态 |
| --- | --- | --- | --- |
| [LAB.md](LAB.md) | `gguf-lens` | 读一个已经量化好的 `.gguf`，说明它为什么是这个大小 | 只有设计 |
| [LAB-quantize.md](LAB-quantize.md) | `nano-quant` | 读 `Qwen/Qwen3-VL-2B-Instruct` 的 safetensors，自己量化成 4 位并跑起来 | 框架已写并在真实模型上跑通，见 [nano-quant/](nano-quant/) |
| [EXERCISE-q4_0.md](EXERCISE-q4_0.md) | `nano-quant` 的 A 部分与 Q4_0 | 习题课上带着写完的那六个函数 | 可以发 |

两者共用同一份 `dequantize` 与同一套「字节账目精确相等」的验收方式，
可以合并成一个两周的实验，也可以分开布置。`LAB.md` 里还有考核方式、
与 nano-ollama 各层的衔接，以及不做贯穿项目时的取证型备选方案。

`nano-quant/` 下是框架、助教的参考实现与判定脚本，C++17，无外部依赖。
`make && make fixtures && sh tests/run.sh` 能在不下载模型的情况下跑完一整轮，
说明见 [nano-quant/README.md](nano-quant/README.md)。真实模型上的参考值在
`nano-quant/tests/reference-qwen3vl.txt`。

`EXERCISE-q4_0.md` 与 `LAB-quantize.md` 一同发给学生。它只写 A 部分的四个转换函数
与 Q4_0 的两个块函数，这六个写完之后 `./nq-selftest a q4_0` 的六项全部与参考一致，
一次课的量。Q4_1、Q4_K 与整模型那几步留在实验说明里。

幻灯片末尾的 `lab-preview` 页是 `nano-quant` 的预告，只讲学生要做的四件事，
细节以实验说明为准。`assets/ext/gguf-spec.jpg` 与 `read-the-header` 那张逐字段表
在早先的一次精简里撤掉了，图片文件仍在 `assets/ext/` 下，重新上页时按原授权标注即可。

## 英文覆盖层

中文写在 `pages.py` 里，英文放在 `i18n/en.toml`，共 629 条。改完中文之后：

```bash
python3 -m lecturekit.cli i18n extract lectures/2-data --lang en   # 合并出新增/变化的条目
python3 -m lecturekit.cli i18n check   lectures/2-data --lang en   # 上课前查缺
```

`extract` 只合并不覆盖，已翻译的条目不会丢；基线改了会标 `# CHANGED`。
`--strict` 让任何未翻译的条目直接拒绝渲染，而不是带着橙色底色上投影。

框架不翻译三类内容，本讲因此在英文版里仍有中文：

1. **代码清单**（`p.code` 的正文，含注释）。本讲的注释一律写英文，中英两版共用，
   `examples/` 下 28 个源文件里没有中文。剩下的一处是十六进制那页的
   `HEX_GROUPS`：它是一张对照表而不是注释，两行标签「十六进制 / 二进制」
   在英文版里照旧是中文。demo 的 `name` 与 `description` 是普通文本，照常翻译。
2. **图片路径**（`p.image` 的 `src`）。`assets/` 下 16 张手写 SVG 的标注是中文，
   英文版里照旧显示中文：`memory-bytes`、`byte-order`、`same-bits`、`shifts`、
   `expand-truncate`、`bit-fields`、`float-formats`、`float-spacing`、
   `quantize-line`、`q4-block`、`zero-point`、`q4-k-block`、`k-scales`、
   `granularity`、`int4-accumulate`、`address-space`。
   与第一讲同样的限制：要让英文版彻底英文，需要框架支持按语言选图。
3. **`p.cite(...)` 与 `p.news(...)`**。参考文献本身不翻译。

第一讲记下的两条经验在本讲同样成立：英文比中文长，改完中文要跑一遍 `--lang en`
看有没有撑出边界；`i18n/en.toml` 里不要在一个列表项内部换行。本讲初稿翻完之后有
23 页顶出页底，其中「位运算恒等式」一页的 `p.highlight` 整条落在画面之外，
按下面三条改完才全部收进来（1280×720 下，正文区的下沿在 y≈700）：

- **`p.highlight` 不超过 66 个字符**。66 字符是一行，73 字符就是两行，多出的一行是 48 px，
  英文长句最容易在这里超；本讲有 31 条因此重写。
- **`p.slide` 的列表项不超过 88 个字符**，行内代码按等宽字体算要再短一些。
- **表格单元格按列宽算**，「位运算恒等式」那张表的说明列约 48 字符一行，名称列约 22 字符。

量一页有没有超，比看 PNG 更准的办法是在渲染出的 `slides.html` 里量元素位置：
落在画面之外的内容在 PNG 上根本不显影，只有量 `getBoundingClientRect()` 才看得见。

## 用语与排版约定

与第一讲相同，改这一讲时请一并维持，`pages.py` 顶部也记了一份。

**用语**：本课件用于本科课程教学，一律采用陈述性的技术表述。不使用比喻、
口语化措辞，以及「不是……而是……」一类的对比句式。结论写成可以直接复述的判断句。

**排版**：

- **粗体后面不要紧跟全角冒号**，写成 `**词**：内容`；
- **尖括号要放在行内代码里**：`p.cite(title=...)`、表格与正文中的 `vector<bool>` 若不加
  反引号会被当成 HTML 标签丢掉；`p.title(...)` 里连反引号也不行，标题中避免尖括号；
- **`==标记==` 只在 `p.slide(...)` 里展开**，写进 `p.highlight(...)`、图注、
  表格单元格会变成字面量；标记内部也**不能包含行内代码**，
  `` ==仍落在 `int` 的范围内== `` 会原样打出两对等号；
- **`p.slide(...)` 里的换行就是换行**，一段话不要在源码里手动折行；
- **一页最多一个 `p.highlight`**；
- 代码清单控制在 **10 行以内**。本讲有五页因为清单过长顶到页底，
  最后把 `show_bytes` 从 12 行压到 6 行、`quantize` 从 21 行压到 10 行；
- 改完之后跑一遍 `--png`，检查有没有内容越过页底（1280×720 下 y≈690 是边界）；
  版面偏空的页用 `p.gap(n)` 往下压，本讲的正文页大多落在页高的 60%–90% 之间。

## 与参考资料的关系

- `refs/advice/1. 数据类型和位运算.md` 提供了本讲的取材范围：
  字符编码与 token 的类比、低精度浮点格式表、量化方法、int4 的位运算例子。
  其中 int4 加法的示例代码 `overflow = a & b & 0b1000` 不正确（它不是 bit 3 的进位），
  本讲改用 `s = (a & 0x77) + (b & 0x77); s ^ ((a ^ b) & 0x88)`，
  并用 `int4.c` 的穷举测试验证：该式在全部 65536 对输入上错误数为 0。
- `refs/advice/ICS课程现况和改革建议.md` 提供了实验设计的框架，见 `LAB.md`。
- CS:APP 第 2 章是内容基线。整数与浮点的完整定理证明、IEEE 754 的舍入规则细节
  有意留在教材里，本讲只取能落到这个文件上的部分。
