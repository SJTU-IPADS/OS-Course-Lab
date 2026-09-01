# ICS 第一讲 · 计算机系统导论

以一次 `ollama run` 请求为例，自底向上考察计算机系统的各个层次。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层 —— 一份源码，两种语言。

## 结构

引子将一次请求分解为五个层次，随后每一节考察一层，节与节之间用
`lecture.bridge(...)` 过渡。**第 3 层（操作系统）为本讲重点**：篇幅最长，
并贯穿使用同一组具体数据（三个进程、4.7 GB 权重、缺页装入、页缓存）。

| 节 | 页数 | 落点 |
| --- | --- | --- |
| 课程概览 | 2 | 教师、教材、课程讨论的问题 |
| 问题的提出 | 7 | AI 应用如何发出请求 · OpenAI 兼容 API · ollama · 三个进程与 4.7 GB 文件两项观察 · 五层框架 |
| 第 1 层 · 硬件 | 3 | 存储程序体系结构、两条总线的互连拓扑、访存带宽的限制 |
| 第 2 层 · 指令集 | 2 | ISA 作为软硬件接口；386 分页作为下一层的硬件前提 |
| **第 3 层 · 操作系统** | **12** | 进程隔离 · 地址空间 · **mmap** 与 llama.cpp 的实测效果 · 页缓存 · 设备访问 · 调度 |
| 第 4 层 · 工具链与运行时 | 10 | 字符编码 → 四阶段编译 → 机器指令 → 运行库 → Python → CUDA |
| 第 5 层 · 应用与 Agent | 2 | Agent 执行循环；AI 负载复用的机制与新的策略问题 |
| 总结 | 4 | 自底向上小结、故障定位表、四个主题、课程安排 |

开头三页先交代 AI 应用是怎么把请求发出去的（Agent → HTTP → 推理后端）、
OpenAI 兼容 API 的端点，以及 ollama 在本机提供的正是这组接口；随后才是
`ollama run` 那条命令和它的实际输出。学生在第一节课不需要预先知道 ollama 是什么。

五页衔接页写成同一形式——层号与名称 + 该层涉及的对象——连起来就是一份
自底向上的目录。

`p.cite(...)` 会自动生成末尾的参考文献页。讲稿写在 `p.notes(...)` 里，投影不显示，
按 `p` 打开演讲者视图可见。

## 上课时跑命令

本讲所有命令都写成 `p.demo(...)`：投影上照常显示命令与录下来的输出，
加上 `--demo` 起的预览里，按一下 ▶ 就在讲义目录里真跑一遍，输出边跑边出：

```bash
python3 -m lecturekit.cli view lectures/1-intro --watch --demo
```

`ollama serve` / `ollama pull` / `ollama run` 写了 `timeout=0`——它们不会自己结束，
用抽屉里的 ■ 停。每按一次 ▶ 都会新开一个运行标签页，同一页上的命令并排跑着，
所以 `ollama serve` 占着一个标签页时，`ollama pull` / `ollama run` 在旁边照样能连上它；
标签页上的 ✕ 关掉这一次运行（还在跑的会一并停掉），抽屉右上角的 ▾ 只是把抽屉收起来、
什么都不停，翻到下一页才会把这一页跑着的东西全部停掉。`compile-pipeline` 与 `runtime-libraries` 两页会在 `examples/`
下真的编译，产物（`.i` / `.s` / `.o` 与两个可执行文件）已在 `.gitignore` 里。
详见 [docs/usage.md](../../docs/usage.md#running-a-demo-from-the-deck)。

## 构建

```bash
python3 -m lecturekit.cli view   lectures/1-intro --watch          # 中文，边改边看
python3 -m lecturekit.cli view   lectures/1-intro --watch --lang en # 英文
python3 -m lecturekit.cli render lectures/1-intro --pdf            # PDF
python3 -m lecturekit.cli render lectures/1-intro --to pptx        # 可编辑的 PPTX
```

## 多语言

中文写在 `pages.py` / `lecture.py` 里，英文放在 `i18n/en.toml`。改完中文之后：

```bash
python3 -m lecturekit.cli i18n extract lectures/1-intro --lang en   # 合并出新增/变化的条目
python3 -m lecturekit.cli i18n check   lectures/1-intro --lang en   # 上课前查缺
python3 -m lecturekit.cli render       lectures/1-intro --lang en --strict
```

`extract` 只合并不覆盖，已翻译的条目不会丢；基线改了会标 `# CHANGED`。
`--strict` 让任何未翻译的条目直接拒绝渲染，而不是带着橙色底色上投影。

### 覆盖层管不到的三处

框架只替换 DSL 里的文本。这三类东西对两种语言是同一份，改动时要留意：

1. **代码块**（`p.code(...)`）与**命令**（`p.demo(...)` 的 `command` / `output`）
   —— 框架不翻译代码：翻译过的清单已经是另一份清单。因此本讲所有代码注释统一
   写成**英文**，中英两版共用。demo 的 `name` 与 `description` 是普通文本，照常翻译。
2. **图片路径**（`p.image` / `p.frames` 的 `src`）—— 同样不翻译。因此凡是能用
   `p.architecture(...)` 表达的层次图都改成了 architecture 块（它的层名和模块名
   在覆盖层里，会跟着翻译）：`five-layers`、`isa-contract`、`os-services`、
   `nvcc-compilation`、`request-recap`。
3. **仍然写死中文的图** —— 下面这些是真正的有向图，architecture 块表达不了，
   英文版里它们仍然显示中文：

   | 图 | 用在 |
   | --- | --- |
   | `mmap-overview.svg` | `model-loading`（本讲最重要的一页） |
   | `hardware-bus.svg` | `machine-parts` |
   | `address-space.svg` | `virtual-address-space` |
   | `agent-request.svg` | `ai-app-request` |
   | `os-timeline.svg` | `os-evolution` |
   | `mini-boundaries.svg` | `mini-ollama-boundaries` |
   | `interpreter-path.svg` | `python-and-pytorch` |
   | `agent-loop.svg` | `agent-loop` |

   要让英文版彻底英文，需要框架支持按语言选图（目前 `src` 不进覆盖层）。

**PPTX 的已知缺口**：PPTX 渲染器不画 `p.architecture(...)`，所以上面第 2 条列的
五页导出后会丢掉层次图（其余内容正常）。要发 PPTX 时先单独导出这几张图片：

```bash
python3 -m lecturekit.cli render lectures/1-intro --pages five-layers,isa-contract,os-services,nvcc-compilation,request-recap --png
```

## 图表

`diagrams/` 是所有自制图的源文件，`assets/` 里的同名 `.svg` 是产物，不要手改：

```bash
lectures/1-intro/diagrams/render.sh    # 重新生成全部图表
```

- `*.dot` — graphviz 流程图（`agent-request.dot` 是开头那张请求路径图）
- `*.py` — 手工排布的 SVG，各自写出自己的产物：
  `mmap_figure.py`（mmap 的三条编号关系）、`hardware_bus.py`（内存总线与 I/O 总线的拓扑）、
  `address_space.py`（进程虚拟地址空间，替换了原先英文的 CS:APP 截图）

`isa-boundary.dot`、`os-services.dot`、`nvcc-pipeline.dot` 已经不再被幻灯片引用
（改用 architecture 块了），留在目录里只是备份，可以随时删掉。

## 用语与排版约定

改这一讲时请一并维持，`pages.py` 顶部也记了一份。

**用语**：本课件用于本科课程教学，一律采用陈述性的技术表述。不使用比喻
（「合同」「显微镜」「那道门」）、口语化措辞（「秒开」「玩意儿」「谁来分」），
以及「不是……而是……」一类的对比句式。结论写成可以直接复述的判断句。

**排版**：

- **粗体后面不要紧跟全角冒号。** `**词：**内容` 不符合 CommonMark 的闭合规则，
  会把 `**` 原样打到投影上；写成 `**词**：内容`。
- **`==标记==` 只在 `p.slide(...)` 里展开。** 写进 `p.highlight(...)`、图注、
  表格单元格会变成字面量。
- **一页最多一个 `p.highlight`**，只给真正的结论用。
- 图配文优先用 `.image_right(...)` 分两栏，而不是一路竖着堆；成组的图用 `p.row(...)`
  （见 `os-evolution-people`）。版面不要长时间保持「标题 + 项目符号 + 结论框」一种形态。
- 版面偏空的页用 `p.gap(52)` 匀开；`p.gap("fill")` 在只有三四块的页上会撑出
  夸张的空洞，不要用。
- **英文比中文长。** 改完中文记得跑一遍 `--lang en` 看有没有撑出边界 ——
  已经因此把英文的层次图标签缩成了 `3 · OS` / `2 · ISA`。
- **`i18n/en.toml` 里不要在一个列表项内部换行。** slide 文本中的换行会渲染成硬
  换行，而且自动加粗只作用于第一个物理行；一个 bullet 写成一行。

## 与参考课件的关系

原始的 59 页课件在 `refs/ppts/1-intro.pptx`，是内容范围的基准。C 语言标准版本
列表、ASCII 控制字符表、操作系统截图集这类打断叙事的目录式材料，有意留在
参考课件里没有搬过来。

图片来源：

- llama.cpp PR #613 页面截图：<https://github.com/ggml-org/llama.cpp/pull/613>
- Intel Core Ultra 200S 平台结构图：<https://cdrdv2-public.intel.com/832586/832586_007.pdf>
- Intel Core Ultra 200S 芯粒封装图：<https://download.intel.com/newsroom/2024/client-computing/Intel-Core-Ultra-200S-Series-Presentation.pdf>
- NVIDIA Rubin 显存带宽对比图：<https://developer.nvidia.com/blog/inside-the-nvidia-rubin-platform-six-new-chips-one-ai-supercomputer/>
