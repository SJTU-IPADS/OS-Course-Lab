# ICS 第一讲 · 计算机系统基础（1）

以一次 `ollama run` 请求为例，自底向上考察计算机系统的各个层次。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层 —— 一份源码，两种语言。

## 结构

结构依照导师对第一讲的修改意见（`1-intro.pptx` 与批注 PDF）：先回答大模型时代
为什么还要学本课程，再以一次 `ollama run` 请求说明 AI 应用仍然通过程序执行实现、
应用与系统的关系，然后分四个部分介绍各层，最后回到这次请求。

| 节 | 页数 | 落点 |
| --- | --- | --- |
| 课程概览 | 6 | 教师与教材（CS:APP、OSTEP）· AI 辅助编程之后学习本课程的必要性 · 改变的内容与不变的系统约束 · 2023 与大模型时代的课程特点 · 课程目标（power **system** programmer）· 六个问题 |
| AI 应用仍然通过程序执行实现 | 5 | AI 应用如何发出请求 · Agent 的请求与工具执行循环 · OpenAI 兼容 API · 推理服务沿用的传统设计（C/S、HTTP、标准 API）· ollama |
| 应用与系统 | 7 | 一条命令背后的系统工作 · 三个进程与 1.9 GB 文件两项观察 · 系统抽象 · 五层框架 · 层次化设计 · 了解系统的程度与本课程的定位 |
| 第 1 部分 · 硬件 | 4 | 目标：计算的软件化 · 存储程序体系结构 · 总线拓扑 · CPU 与 GPU |
| 第 2 部分 · 汇编与指令集 | 3 | 目标：快速开发程序 · ISA · x86 的向后兼容 |
| 第 3 部分 · 工具链与运行时 | 11 | 目标：消除程序绑定（四类绑定）· 字符编码 → 机器指令 → 运行库 → Python → CUDA |
| 第 4 部分 · 操作系统 | 10 | 目标：多个程序共享硬件 · 四类抽象 · 独占使用 · 进程隔离 · llama.cpp 改用 **mmap** 的加载效果 · 设备访问 · 调度（机制与策略）· 演进 |
| 回到例子 | 6 | AI 负载的策略问题 · 自底向上小结 · 分析定位问题 · AI 基础设施中的本课程内容 · 四个主题 · 课程安排 |

工具链放在操作系统之前，与 CS:APP 第 7、8 章的顺序一致。

四个部分的最后一页都是「本课程中的X」：一张「系统方法 | 本课程中的例子 | CS:APP 章节」
的表。与导师原稿不同的两处章节号：总线归到第 6 章（CS:APP §6.1 讲总线），
FP4–FP64 的数值格式归到第 2 章（浮点数表示）。

衔接页写成同一形式——「第 N 部分 · 名称」+「目标：……」。五层框架图与小结图的
层名不带层号。

`p.cite(...)` 会自动生成末尾的参考文献页。讲稿写在 `p.notes(...)` 里，投影不显示，
按 `p` 打开演讲者视图可见。

## 上课时跑命令

本讲所有命令都写成 `p.demo(...)`：投影上照常显示命令与录下来的输出，
`--watch` 起的预览里，按一下 ▶ 就在讲义目录里真跑一遍，输出边跑边出：

```bash
python3 -m lecturekit.cli view lectures/1-intro --watch
```

`ollama serve` / `ollama pull` / `ollama run` 写了 `timeout=0`——它们不会自己结束，
用抽屉里的 ■ 停。每按一次 ▶ 都会新开一个运行标签页，同一页上的命令并排跑着，
所以 `ollama serve` 占着一个标签页时，`ollama pull` / `ollama run` 在旁边照样能连上它；
标签页上的 ✕ 关掉这一次运行（还在跑的会一并停掉），抽屉右上角的 ▾ 只是把抽屉收起来、
什么都不停，翻到下一页才会把这一页跑着的东西全部停掉。`runtime-libraries` 一页会在 `examples/`
下真的编译，产物已在 `.gitignore` 里。
这一页的 demo 还用 `files=[...]` 声明了编译的源文件，▶ 旁边多一个以文件名为标签的
按钮，按下后在右侧展开该文件的全文，课上可以直接看代码。
详见 [docs/usage.md](../../docs/usage.md#running-a-demo-from-the-deck)。

## 跨平台：x86-64 Linux、arm64 macOS 与 x86-64 Windows

课上演示与同学自己复现时的机器有三种：x86-64 的 Linux、arm64 的 Mac、x86-64 的
Windows。本讲的命令统一写成前两者都能直接执行的形式，输出的**结构**一致；Windows
以 WSL2（Ubuntu）为课程环境，命令与 x86-64 Linux 完全相同。平台之间确实不同的部分
写在 `p.notes(...)` 里，讲的时候可以直接说明。

| 页 | 命令 | macOS（arm64） | Windows（原生，不经 WSL2） |
| --- | --- | --- | --- |
| `ai-app-request` | `curl .../v1/chat/completions` | 相同 | `curl.exe` 相同；PowerShell 里 `curl` 是 `Invoke-WebRequest` 的别名，要写全 `curl.exe` |
| `one-command` / `request-path` | `ollama serve` / `pull` / `run` | 相同 | 相同（有 Windows 版） |
| `three-processes` | `ps -eo pid,comm,args \| grep '[o]llama'` | 进程一致；`comm` 列显示完整路径 | `Get-Process ollama*` |
| `weights-are-data` | `ls -lhS ... \| sed -n '2p'` + `file "$(ls -dS ... \| head -1)"` | 相同 | 权重在 `%USERPROFILE%/.ollama/models/blobs`；`Get-ChildItem <dir> \| Sort-Object Length -Descending \| Select-Object -First 1` |
| `machine-code` | 展示的是 x86-64 汇编 | `ldr` / `fmul` / `fadd`（或 `fmadd`）/ `cmp` + `b.ne`，六步结构一致 | 指令集相同；调用约定为 Microsoft x64，传参寄存器与 System V 不同 |
| `runtime-libraries` | `ldd ./cpp_demo 2>/dev/null \|\| otool -L ./cpp_demo` | 没有 `ldd`，`otool -L` 承担同样作用；列出 `libc++.1.dylib` 与 `libSystem.B.dylib` | 共享库是 DLL；MSYS2 中同样有 `ldd`，原生工具链用 `objdump -p` 或 `dumpbin /dependents`，列出 `libstdc++-6.dll`、`msvcrt.dll`、`KERNEL32.dll` |
| `python-and-pytorch` | `file "$(command -v python3)"` + `ls .../lib \| grep -E 'libtorch_(cpu\|cuda)'` | `file` 报 Mach-O arm64，算子库为 `libtorch_cpu.dylib`，没有 CUDA 版本 | 没有 `file`，用 `Get-Command python` 定位；算子库为 `torch_cpu.dll` / `torch_cuda.dll`（没有 `lib` 前缀） |
| `failures-between-layers` | 表格里的排查手段 | 内存一栏取 `vm_stat` | 内存用任务管理器或 `Get-Counter` 的内存计数器，内核日志是事件查看器 |

三条约定，改命令时一并维持：

1. **不要依赖只在一个平台存在的命令。** 需要用到时写成 `linux_cmd 2>/dev/null || mac_cmd`，
   并在同一行加英文注释说明谁是谁（见 `runtime-libraries`）。Windows 的等价写法放在
   `p.notes(...)` 里：命令行里再加一层回退会让投影上的命令变得无法阅读。
2. **不要依赖输出的行数与列宽。** 用 `sed -n '2p'`、`head -1`、`grep -E` 把要看的那一行挑出来，
   而不是让听众去数行（`ls -l` 的 `total` 行在 Linux 与 macOS 上单位不同）。
3. **不要依赖文件名后缀与排序。** 共享库在 Linux 上是 `.so`、macOS 上是 `.dylib`、
   Windows 上是 `.dll`；`ls -dS` 按大小排序比按字母序稳定。

录在 `output=` 里的输出来自 x86-64 Linux，投影上显示的是这一份；按 ▶ 在别的平台上真跑时，
输出会按上表的差别变化。GPU 相关的 `cuda-kernel` 一页按内容本身就是 NVIDIA 平台的，
不在跨平台之列。

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
   | `hardware-bus.svg` | `machine-parts` |
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
  `hardware_bus.py`（内存总线与 I/O 总线的拓扑）

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
  已经因此把英文的层次图标签缩成了 `OS` / `ISA`。
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
