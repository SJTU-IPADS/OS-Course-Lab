# ICS 实验说明 · nano-quant、bomb-lab、attack-elf、parallel-dot、mini-cpu 实验一

第二讲与第三讲的配套实验的说明课：第一部分说明课件的下载、安装与使用，此后五个实验各一部分，按 nano-quant、bomb-lab、attack-elf、parallel-dot、mini-cpu 实验一的顺序排列。每个实验的部分分三个小节：「实验框架」用图说明要实现的函数被谁调用、参数是什么、处理的是哪一部分数据；「课上完成」带着同学完成实验的一部分，页面不给出答案代码；「其余部分与评测」说明课后要完成的部分、检查与评测的方法。bomb-lab 与 attack-elf 只有前两个小节，parallel-dot 的第三个小节称为「其余部分」，只讲第二部分的 CUDA kernel，mini-cpu 的第三个小节称为「其余部分」，不讲评测与提交。约定与操作细节在各实验目录的 README 中，页面只举例，解释写在讲稿中。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。

## 结构

| 节 | 页数 | 小节与落点 |
| --- | --- | --- |
| 本节的安排 | 1 | 五个实验：配合的讲次、要做的事、是否计分 |
| 第一部分 · 课件 | 3 | 下载与安装 · 打开一讲并翻页 · 运行演示、导出与更新 |
| 第二部分 · nano-quant | 10 | 实验框架（4）：流程、框架对函数的调用、GGUF 的布局、张量变成块 · 课堂演示：Q4_0 的量化与还原（3）：声明与第二讲块图的对应、量化与打包的规则、`nq-selftest q4_0` 与在 ollama 中运行 q4_0 模型 · 其余部分与评测（3）：A 部分、Q4_1、Q4_K 与 `put_scale_min` |
| 第三部分 · bomb-lab | 7 | bomb 的使用（4）：`main` 的结构、取得 bomb、画面与正确、错误的一行、计分板 · 课堂演示：phase 1（3）：用 `objdump -d`、`-D` 反汇编整个 bomb、进程的地址空间、`lea` 的目标 |
| 第四部分 · attack-elf | 6 | attack 的使用（3）：任务与三个程序、领取程序与模型与用法（终端界面、`-q`、`-s`、攻击走管道）、提交与记分 · 课堂演示：attack1 的攻破（3）：`chat_turn` 的栈帧、载荷的字节、字节填入栈的六帧动画 |
| 第五部分 · parallel-dot | 11 | 实验框架（4）：算子表、一个 Token 的调用、参数与 Q4_0 的块、标量版本与块的字节排布 · 课堂演示：AVX2 版本的 `matvec_q4_0`（4）：按行 8 路累加与 `d` 提到块内求和之外的公式、一个块进入寄存器（三帧）、8 路乘加、`make test-simd` · 其余部分（3）：Grid 与行、线程块内的分工、折半求和 |
| 第六部分 · mini-cpu 实验一 | 17 | 实验框架（6）：`printf`、libc 与 `syscall`、guest/hello 的 `syscall` 由 mini-cpu 代为执行、装入之后内存与寄存器的位置、主循环、x86-64 指令的七段与读取顺序（伪代码）、指令的字节与 op（以 `add` 为例，一次循环调用的函数） · 课堂演示：ModRM 与 SIB 的解析（6）：ModRM、SIB、REX、`modrm_decode` 的判断顺序（三帧）、`modrm_t`、三条指令的解析结果 · 其余部分（5）：alu.c 与标志位、移位乘除、整数指令的五组、cond 与条件跳转、向量指令 |

共 84 张幻灯片：1 个封面、19 个衔接页（六个部分各一个，五个实验的 13 个小节各一个）、64 张正文（55 页，其中三页是动画）。打印的页码连续计数，PDF 的第 N + 1 页是打印的第 N 页。

页面函数都在 `pages.py`，按部分排列，函数名就是页面 id（连字符写成下划线）；第一到第六部分的页面 id 分别以 `kit-`、`nq-`、`bomb-`、`attack-`、`pd-`、`cpu-` 开头。`pages.py` 开头的说明按部分写明内容、演示输出与配图的来源。

讲义的六节称为「第一部分」到「第六部分」，小节不编号。nano-quant 的两个部分写作「A 部分」「B 部分」，parallel-dot 的两个部分写作「第一部分」「第二部分」（在第五部分之内指实验的两个部分）。

nano-quant、parallel-dot、mini-cpu 的目录都在讲义目录之外（`../nano-quant` 等），框架的 `files=` 与 `[label](file:...)` 只接受讲义目录之内的路径，因此这三部分的页面没有源文件预览按钮，只写文件名。

nano-quant 与 parallel-dot 的演示的第一行是 `cd "$LECTUREKIT_ROOT/lectures/nano-quant"` 或 `cd "$LECTUREKIT_ROOT/lectures/parallel-dot"`，进入实验目录；`$LECTUREKIT_ROOT` 是框架运行演示时设置的项目根目录，即 `lecturekit` 包所在的目录（`docs/usage.md`，「Running a demo from the deck」）。

代码块不翻译，注释用英文。例外是「x86-64 指令的组成与读取顺序」一页的伪代码：它用中文写，以 `p.code(..., translate=True)` 进入翻译覆盖层，英文版显示 `i18n/en.toml` 中的英文伪代码。

## 配图、截图与例子程序

- `diagrams/` 下的脚本生成 `assets/` 中的 SVG，脚本与图同名，以 `nq_`、`bomb_`、`attack_`、`pd_`（图名 `pd-`）、`cpu_` 开头；`diagrams/render.sh` 运行全部脚本并用 `check_bounds.py` 检查越界。`cpu_kit.py` 是 mini-cpu 各图共用的配色与绘制函数，`nq_q4_1.py` 用 `nq_q4_grid.py` 的绘制函数。生成的 SVG 不手改。
- 图在页面上的排布跟着图的长宽比：横向的长条图用 `p.side_image(..., alt="contain", side="left")` 放在页面左侧、文字在右侧（`chat_turn` 的栈帧一页，图的 SVG 只画内容，窗口用 `left=` 裁掉左边的空白），其余图横跨页面；动画用 `p.frames`，一帧一张。
- `assets/bomb_*.png` 是 bomb 在终端中的截图，由 `tools/shots.sh` 生成，方法在 `tools/README.md`：阶段的画面、爆炸、拆除之后的画面来自 `examples/bomb`，计分板来自一个在 127.0.0.1 上启动的课程服务器与它发出的演示 bomb，计分板上只有演示学号。
- `examples/bomb` 是不连接服务器的演示 bomb，学号 000000000001，由实验的源码树生成（`tools/README.md` 第 2 步）；第三部分的截图、页面上的地址与 `objdump` 的演示都来自它。演示命令从讲义目录出发，先进入 `examples/`。它是生成的文件，`examples/.gitignore` 忽略它与演示中写出的 `bomb.asm`、`bomb-all.asm`。
- `examples/attack1` 是 attack1 演示学号 000000000001 的构建，`examples/attack1.txt` 是这个学号的答案（代码写在缓冲区低端，返回地址写成它的地址），`examples/attack1.bin` 是答案的原始字节（由实验的 `tools/unhex` 写出），`examples/q4_0.gguf` 是指向 nano-quant 模型的符号链接；三者由 `tools/make-attack-demo.sh` 生成（`tools/README.md`），第四部分的地址与反汇编都来自它们。

## 演示与内容来源

- **课件**：页面依据 [`../tut-lecturekit/README.md`](../tut-lecturekit/README.md)（它在 GitHub 仓库中是 `ICS-PPT/lectures/tut-lecturekit/README.md`），只列出命令，各系统的步骤、截图、镜像源与常见问题由页面指向教程；第 4 页同时给出教程编译出的 PDF 在 GitHub 上的链接（`ICS-PPT/lectures/tut-lecturekit/tut-lecturekit.pdf`）。安装 Marp 的一条用教程 1.6 节的 `npm install` 形式，它在三种系统上都可以运行；其中的版本 `4.5.0` 与 `lecturekit/renderers/viewer/marp.py` 的 `MARP_VERSION` 相同，框架升级 Marp 时这一页与教程一起改。例子中的讲取 `lectures/3-asm`。三页没有演示。
- **nano-quant**：页面依据 [`../nano-quant/README.md`](../nano-quant/README.md) 与 `include/nano_quant.h`；课上 Q4_0 一页的公式与打包方式逐字取自头文件的注释。演示输出于 2026-10-19 录自学生目录（不含 `model/`）的副本，其中 `impl/nano_quant.cpp` 只换上 `../nano-quant-ta/` 实现中的 A 部分与 Q4_0：「课上的检查」第一个演示来自它，第二个演示是交互式的，没有录输出。GGUF 布局图的字节数取自 `./nano-quant plan model/model.safetensors --recipe q4_0`；「课上的检查」一页讲稿中 q4_0 模型的回答于 2026-10-19 录得：用只含 A 部分与 Q4_0 的实现量化整个模型，md5 与字节数与 `tests/expected-qwen3vl.txt` 相同，再由本机的 ollama 0.33.2 运行；流程图讲稿中 q4_k_m 的字节数抄自该文件，没有用它量化。
- **bomb-lab**：页面依据发给学生的材料 [`../solve-bomb/handout/README.md`](../solve-bomb/handout/README.md) 与 `handout/Makefile`，工具的用法指向辅导课「汇编实验准备」（`../tut-asm-lab`）。第三部分的一个演示（`objdump -d`、`-D` 反汇编整个 bomb）与页面上的地址（`phase_1`、`lea` 与字符串的地址，各段的起始地址）于 2026-10-19 用 binutils 2.46 从 `examples/bomb` 录得，地址由 `objdump -d` 与 `readelf -S` 读出。页面与讲稿不写阶段 2 ~ 8 的任何输入或计算内容。
- **attack-elf**：页面依据发给学生的材料 [`../attack-elf/dist/README.md`](../attack-elf/dist/README.md) 与教师的 [`../attack-elf/README.md`](../attack-elf/README.md)（编译选项、三个阶段的构造、记分与服务器），源码取自 [`../attack-elf/src/chat.c`](../attack-elf/src/chat.c) 的 `chat_turn`、`Gets` 与 `report_pass`。演示的输出与页面上的数字（缓冲区 304 字节、返回地址在 +312、口令 `0x27d6fbbe5c7d230b`、`report_pass` 在 `0x01528000`、返回地址 `0x00007fffffffd550`、载荷 320 字节）于 2026-10-20 在本机（x86-64 Linux，binutils 2.46）用 `objdump`、`nm`、`readelf`、`grep` 与运行 `examples/attack1` 录得，全部取自演示学号 000000000001 的构建。页面与讲稿不写阶段 2、3 的载荷，只说明它们各自的防护；页面上的攻击把代码写进缓冲区、返回地址写成它的地址（演示关掉 ASLR），实验室参考答案里不依赖栈地址的做法不在课上出现。
- **parallel-dot**：页面依据 [`../parallel-dot/README.md`](../parallel-dot/README.md)、`docs/simd.md`、`docs/cuda.md` 与 `ops/` 下的源码；intrinsics 的名字只取自 `ops/simd.c` 的 TODO 注释与 `docs/simd.md`，页面与讲稿不含 AVX2 或 CUDA 的答案代码。演示输出于 2026-10-18 在本机录得（i9-11900H）：把 `../parallel-dot` 与 `../parallel-dot-ta` 复制到临时目录，在助教目录的副本中运行 `make -s test-simd | tail -n 8`。「生成一个 Token」一页讲稿中各算子的时间占比于 2026-10-19 测得：在同样的副本上给算子表的每一项计时（`prompt.txt` 与一个 400 个 Token 的提示词），GPU 上另用 nsys 统计各 kernel 的时间。
- **mini-cpu 实验一**：页面依据 `../mini-cpu/src/lab1/README.md`、`include/mcpu.h` 与学生目录的源码；图中的指令都取自客户机程序的 `objdump -d`。「内存与寄存器：mini-cpu 在内存中模拟」一页的客户机地址取自 `objdump -d` 与 `readelf -l guest/hello`，`dram.img` 中的偏移与装入后的寄存器值于 2026-10-19 用参考实现运行 `guest/hello` 之后由 `od` 与 `-v` 读出。mini-cpu 一部分的页面、讲稿与图只讲实验一，不提及其他实验。

## mini-cpu 的运行环境

实验在 **x86-64 的 Linux** 上完成。

| 机器 | 做法 |
| --- | --- |
| x86-64 Linux | 直接运行 |
| x86-64 Windows | 在 WSL2（Ubuntu）中完成，命令与 Linux 完全相同 |
| arm64 macOS | 用 x86-64 的 Linux 容器（`docker run --platform linux/amd64`）或课程服务器 |

mini-cpu 装载并解释 x86-64 的 ELF 程序，Makefile 用到 GNU ld 的 `--wrap`，框架代码用 `<elf.h>`，评测脚本用 binutils 的 `readelf`、`objdump`。这几项在 macOS 上都不存在，因此实验不提供 macOS 的原生做法。

## 渲染

```bash
python3 -m lecturekit.cli render lectures/3-asm-lab --pdf
```

```bash
python3 -m lecturekit.cli render lectures/3-asm-lab --lang en --pdf
```
