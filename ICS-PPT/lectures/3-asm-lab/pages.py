"""实验说明课的全部页面内容。

本节的安排用一张表列出四个实验；第一部分说明课件的下载、安装与使用；第二到
第五部分依次是 nano-quant、bomb-lab、parallel-dot 与 mini-cpu 实验一
（x86-64 指令解释器）。页面函数按这个顺序排列，函数名就是页面 id（连字符写成
下划线）。

四个实验的部分结构相同：「实验框架」用图说明要实现的函数被谁调用、参数是什么、
处理的是哪一部分数据；「课上完成」带着同学完成一部分（nano-quant 的 Q4_0、
bomb-lab 的 phase 1、parallel-dot 的 AVX2 版本、mini-cpu 的 ModRM 与 SIB 的
解析），页面不给出答案代码；「其余部分与评测」说明课后要完成的部分与检查、评测的
方法。bomb-lab 没有「其余部分与评测」一节，phase 1 的课堂演示只有 objdump 反汇编、
地址空间与 lea 三页。parallel-dot 的第三个小节称为「其余部分」，只讲第二部分的
CUDA kernel。mini-cpu 的第三个小节称为「其余部分」，不讲评测与提交。

配图由 diagrams/ 下的同名脚本生成（diagrams/render.sh），脚本与图以 nq_、bomb_、
pd-/pd_、cpu_ 开头；bomb-lab 的四张终端截图由 tools/shots.sh 生成。

第一部分 · 课件

组织方式：先说明课件所在的仓库、要安装的三个工具，以及下载课件与安装 lecturekit
的命令；再说明打开一讲与翻页；最后是运行演示、导出 PowerPoint 与 PDF、同步更新。
页面只列出命令，各系统的安装步骤、截图与常见问题在完整的教程中，页面指向它。

配套材料：教程 ../tut-lecturekit/README.md，它在 GitHub 仓库中的位置是
ICS-PPT/lectures/tut-lecturekit/README.md。页面上的命令取自教程第一到第四部分：
安装 Marp 的一条用教程 1.6 节的 npm 命令，它在三种系统上都可以运行，
版本与 lecturekit/renderers/viewer/marp.py 的 MARP_VERSION 相同。
例子中的讲取 lectures/3-asm。

第二部分 · nano-quant

组织方式：三个小节。「实验框架」说明实验从 safetensors 到 ollama 的流程、框架如何
调用学生的函数、GGUF 文件的布局与学生的函数在其中的位置；「Q4_0 的量化与
还原」从 include/nano_quant.h 的声明出发，把两个参数对应到第二讲的 Q4_0 块图，
说明量化规则与课上的检查；「其余部分与评测」说明课后要完成的 A 部分、
Q4_1、Q4_K 与 put_scale_min。

实验在 ../nano-quant/，实验说明在 ../nano-quant/README.md，每个函数的原型与计算规则
在 ../nano-quant/include/nano_quant.h。页面上的命令都写成学生在 ../nano-quant 中运行
的形式；files= 不能指向讲义目录之外的文件，因此没有源文件预览按钮。

录得的输出的来源（2026-10-19，本机 x86-64 Linux，gcc）：把 ../nano-quant（不含 model/）
复制到 build/scratch-nq/lab/ 下的四个副本：
- student：原样，未实现任何函数；
- solved：impl/nano_quant.cpp 换成 ../nano-quant-ta/nano_quant.cpp；
- classdone：只把 A 部分的四个函数与 q4_0_quantize、q4_0_dequantize 换成参考实现；
- q4only：只把 q4_0_quantize、q4_0_dequantize 换成参考实现。
「课上的检查」第一个演示录自 classdone 的 ./nq-selftest q4_0，第二个录自 q4only；
「评测」的演示录自 solved 的 make -s test 2>&1 | grep -E 'pass|FAIL|failed'，
讲稿中 classdone 的结果录自同一条命令；「A 部分」讲稿中 ./nq-selftest a 的四行录自
solved。真实模型的期望值抄自 ../nano-quant/tests/expected-qwen3vl.txt；GGUF 布局图与
「张量如何变成块」中的张量字节数取自 solved 的
./nano-quant plan ../nano-quant/model/model.safetensors --recipe q4_0。

配图由 diagrams/ 下的 nq_*.py 生成（diagrams/render.sh）。块一级的配色沿用第二讲：
d 橙、m 金、qs 蓝，Q4_K 的 scales 绿；程序一级的配色：框架蓝、你写的代码橙、文件灰、
ollama 绿。

第三部分 · bomb-lab

组织方式：第一小节说明 bomb 的使用：任务（main 对每个阶段的三次调用）、取得 bomb、
阶段的画面与正确、错误的一行、计分板，画面与计分板是终端截图，页面上
文字很少，说明在讲稿中。第二小节是 phase 1 的课堂演示，页面只有三页：用 objdump -d
与 -D 反汇编整个 bomb，bomb 进程的地址空间，lea 的目标地址的算法。

材料的来源：学生说明 ../solve-bomb/handout/README.md、取得 bomb 的 handout/Makefile。
工具的用法在辅导课「汇编实验准备」（../tut-asm-lab）中以 bomb-light 讲过，这里只指向它。
页面与讲稿不写阶段 2 ~ 8 的任何输入、变体或计算内容。

例子程序 examples/bomb：由实验的源码树生成的不连接服务器的 bomb，8 个阶段的变体
全为 0，学号 000000000001（make bomb ID=000000000001 VARIANTS=00000000 SERVER=）。
页面上的地址（phase_1 在 0xab8d，lea 在 0xab99，字符串在 0xcb030）、各段的起始地址
与 objdump 一页的演示输出都取自这个文件，2026-10-19 在本机（x86-64 Linux，binutils
2.46）用 objdump 与 readelf -S 录得。演示从讲义目录出发，先进入 examples/，写出的
bomb.asm、bomb-all.asm 由 examples/.gitignore 忽略。

终端截图 assets/bomb_*.png 由 tools/shots.sh 生成（tools/README.md）：阶段的画面、
错误的一行之后的爆炸、正确的一行之后的阶段 2，都来自 examples/bomb；计分板来自一个在
127.0.0.1 上启动的课程服务器与它发出的 5 个演示 bomb（学号 000000000001 ~
000000000005），计分板上只有这 5 行。

第四部分 · parallel-dot

组织方式：三个小节。「实验框架」说明 mini-ollama 的结构（算子表与三套算子）、
生成一个 Token 时 matvec_q4_0 被调用的位置与矩阵的形状、函数的五个参数在矩阵
向量乘上的位置与 Q4_0 的块，以及作为参照的标量版本与一个块的字节排布；「课上完成」
是 AVX2 版本的做法：按行 8 路累加的流程与 d 提到块内求和之外的公式、一个块进入
寄存器的三步动画、一组 8 个权重的乘加、make test-simd 的检查；「其余部分」
是第二部分的 CUDA kernel：线程块与行的对应、块内 256 个线程的分工、共享内存中的
折半求和。

配套材料：实验总览 ../parallel-dot/README.md，两部分的说明 ../parallel-dot/docs/simd.md
与 ../parallel-dot/docs/cuda.md，算子表 ../parallel-dot/ops.h，三套算子
../parallel-dot/ops/{scalar.c,simd.c,cuda.cu}，主循环 ../parallel-dot/main.c。
页面上的命令都写成学生在 ../parallel-dot 中运行的形式；实验目录在讲义目录之外，
files= 不能指向它，所以演示没有源文件预览按钮。页面与讲稿不含 AVX2 或 CUDA 的
答案代码：intrinsics 的名字取自 ops/simd.c 中给出的 TODO 注释与 docs/simd.md。

配图由 diagrams/pd_*.py 生成，颜色沿用第三讲第五部分的图（权重蓝、x 灰、积橙、
和绿）与第二讲 Q4_0 图的用语；缩放系数 d 用第二讲 Q4_1 图中的金色，以免与积的橙色
混同；权重 0..15 的编码实心蓝，权重 16..31 的编码空心蓝；「每个块的处理」一页的第三帧
中四组编码各用一种颜色（蓝、紫、青、红），由它们得到的 %ymm 用同一颜色；CUDA 的
线程橙色，与第三讲 GPU 图中被展开的线程块与 Warp 相同。

录得的输出（2026-10-18，i9-11900H，gcc 15.2.0，接通电源）：把 ../parallel-dot
（不含模型文件，q4_0.gguf 为符号链接）与 ../parallel-dot-ta 复制到 build/scratch-pd/，
在助教目录的副本中 make test-simd（助教目录的 Makefile 用参考答案编译学生目录的
源码），页面上的演示是 make -s test-simd | tail -n 8 的输出。学生目录原样
make test-simd 的输出（第一组全部 pass 之后 matvec_q4_0 is not implemented yet，
状态 3）只在讲稿中转述。

第五部分 · mini-cpu 实验一

三个小节：实验框架、课上完成的 ModRM 与 SIB 解析、其余部分。
配图由 diagrams/ 下的 cpu_*.py 生成，脚本与图同名；cpu_flow.py 生成三帧。

实验框架一节先说明 printf、libc 与 syscall 的关系，以及 guest/hello 的 syscall 由 mini-cpu 读取、
由 mini-cpu 调用 libc 的 write 代为执行（这一页不提 dram.img、interp_run 与 switch，它们在后面几页讲；
syscall 经 default 交给 sys_dispatch 写在主循环一页的讲稿中）；再讲 guest/hello 装入之后内存在 dram.img、寄存器在
main 的变量 cpu 的 R[] 中。主循环之后一页用中文伪代码写出一条 x86-64 指令的读取顺序（七段的字节数依
Intel SDM 第 2 卷图 2-1 与 2.2.1 节），伪代码以 translate=True 翻译，英文在 i18n/en.toml 中。这一部分只讲实验一，
页面、讲稿与图不提及其他实验。客户机地址取自 objdump -d 与 readelf -l guest/hello；dram.img 中的偏移
与装入后的寄存器值于 2026-10-19 用参考实现运行 guest/hello 之后由 od 与 -v 读出。

用语要求：面向本科课程教学，采用陈述性的技术表述；不使用比喻、口语化措辞。

排版约定：
- 粗体后面不要紧跟全角冒号，写成 `**词**：内容`。
- `==标记==` 只在 `p.slide(...)` 里展开。
- 一页最多一个 `p.highlight`。
- 代码注释统一写英文，中英两版共用。
- slide() 关闭自动加粗，粗体在文案里逐处写明；figure() 放 assets/<name>.svg，默认宽 1120px。
"""


def slide(p, md, **kw):
    """页面文案。粗体在文案里逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def figure(p, name, width=1120):
    """页面的配图 assets/<name>.svg，由 diagrams/ 下的同名脚本生成。"""
    return p.image(f"assets/{name}.svg", width_px=width)


# ==============================================================================
# 本节的安排
# ==============================================================================

def four_labs(p):
    p.title("四个实验")
    p.spacer(100)                      # the table and one line sit mid-page
    p.table(
        headers=["实验", "配合", "要做的事", "计分"],
        rows=[
            ["nano-quant", "第二讲", "把 Qwen3-VL-2B-Instruct 量化为 4 位", "不计分"],
            ["bomb-lab", "第三讲", "从 `bomb` 的机器码推出 8 个阶段的输入", "不计分"],
            ["parallel-dot", "第三讲第五部分", "用 AVX2 与 CUDA 写矩阵向量乘", "不计分"],
            ["mini-cpu 实验一", "第三讲", "完成 x86-64 指令解释器", "计分"],
        ],
    )
    p.spacer(20)
    slide(p, """
鼓励「古法编程」，鼓励同学之间讨论、与 AI 讨论。
""")
    p.notes("""
本讲先说明课件的下载、安装与使用，再说明四个实验，每个实验一部分。前三个实验是课后练习，不计分，每一步都有检查程序，输出与期望值一致就说明这一步正确；mini-cpu 实验一计分，满分 100 分。
nano-quant 配合第二讲「数据的表示」：在 impl/nano_quant.cpp 中实现读字节的函数与三种 4 位格式的编解码，框架把 Qwen3-VL-2B-Instruct 的语言模型量化后组装为 GGUF，由 ollama 加载运行。
bomb-lab 配合第三讲：每个学号一个没有源码的 bomb，阅读它的机器码推出 8 个阶段的输入；bomb 运行时连接课程服务器，每一次爆炸与每一个被拆除的阶段都记在学号下，./bomb -s 显示计分板。
parallel-dot 配合第三讲第五部分的两个提速方案，两部分写同一个算子，即 Q4_0 矩阵与向量的乘积：第一部分用 AVX2 写函数 matvec_q4_0，第二部分写它的 CUDA kernel；没有 NVIDIA 显卡时用 make remote 提交到课程服务器。两部分运行的 mini-ollama 读取 q4_0.gguf，这个文件从交大云盘下载，或由 nano-quant 按 q4_0 配方生成。
mini-cpu 实验一配合第三讲：在三个文件中完成 x86-64 指令解释器，make check 给出得分，make handin 生成的 lab1.zip 提交到 Canvas 平台。
做这四个实验时，鼓励「古法编程」，即代码由自己动手写出；鼓励同学之间讨论，也鼓励与 AI 讨论。
""")


# ==============================================================================
# 课件 · 下载、安装与使用
# ==============================================================================

INSTALL_COMMANDS = """git clone https://github.com/SJTU-IPADS/OS-Course-Lab.git
cd OS-Course-Lab/ICS-PPT
python3 -m pip install -e .
npm install --no-audit --no-fund @marp-team/marp-cli@4.5.0"""

VIEW_COMMAND = """python3 -m lecturekit.cli view lectures/3-asm"""

MORE_COMMANDS = """python3 -m lecturekit.cli view lectures/3-asm --watch
python3 -m lecturekit.cli render lectures/3-asm --to pptx
python3 -m lecturekit.cli render lectures/3-asm --pdf
git pull"""


def kit_install(p):
    p.title("课件的下载与安装")
    p.slide("""
课件在 GitHub 仓库 `SJTU-IPADS/OS-Course-Lab` 的 `ICS-PPT/` 目录中，每一讲是 `lectures/` 下的一个目录，由 lecturekit 渲染为幻灯片，在终端中运行：
""", autobold=False)
    p.code("bash", INSTALL_COMMANDS)
    p.slide("""
- Windows 上把 `python3` 写成 `python`
- Ubuntu 和 macOS 上先运行 `python3 -m venv .venv && source .venv/bin/activate` 创建虚拟环境并激活，后续打开终端只需要 `source .venv/bin/activate`
- 完整的教程参考仓库中的 `ICS-PPT/lectures/tut-lecturekit/README.md`，PDF 版：[https://github.com/SJTU-IPADS/OS-Course-Lab/blob/main/ICS-PPT/lectures/tut-lecturekit/tut-lecturekit.pdf](https://github.com/SJTU-IPADS/OS-Course-Lab/blob/main/ICS-PPT/lectures/tut-lecturekit/tut-lecturekit.pdf)
""", autobold=False)
    p.notes("""
课件放在 GitHub 仓库 SJTU-IPADS/OS-Course-Lab 中，ICS-PPT 目录是课件本身和把它渲染成幻灯片的工具 lecturekit。课件是一组 Python 源文件，lectures 下每个子目录是一讲。
需要三个工具：Git 下载课件并在之后同步更新，Python 运行 lecturekit，版本在 3.12 及以上，Node.js 提供 npm，用来安装把幻灯片编译成网页的 Marp。Windows 与 macOS 从各自的官网下载安装程序，Ubuntu 用 apt 一次装完。Windows 上安装 Python 时要勾选 Add python.exe to PATH。装好之后重新打开终端，新的终端才能找到这些命令。
四条命令依次是：下载整个仓库，约 900 MB；进入 ICS-PPT 目录；安装 lecturekit 与它依赖的 Python 库；把 Marp 安装到 ICS-PPT/node_modules 下。后两条只需运行一次。
Ubuntu 不允许往系统自带的 Python 中直接安装库，macOS 上用 Homebrew 安装的 Python 也是这样，所以先在课件目录中建立一个独立的 Python 环境 .venv，lecturekit 装在这个环境中；启用后提示符前面多出 (.venv)。用 python.org 的安装程序装的 Python 没有这个限制，教程的 macOS 部分直接安装，不建 .venv，两种做法都可以。
下载慢时，git clone 加 --depth 1 只下载最新版本，不含历史记录；pip install 加 -i https://pypi.tuna.tsinghua.edu.cn/simple，npm install 加 --registry=https://registry.npmmirror.com，改用国内的镜像源。
教程按 Windows、macOS、Ubuntu 分成三部分，每一步有截图与应当看到的输出，最后是常见问题；同一个目录中的 tut-lecturekit.pdf 是它编译出的 PDF，页面上的链接指向这个文件。
""")


def kit_view(p):
    p.title("打开一讲并翻页")
    p.code("bash", VIEW_COMMAND)
    p.slide("""
命令在 `ICS-PPT/` 目录中运行，浏览器自动打开这一讲的大纲，通过点击一行标题进入那一页。把 `lectures/3-asm` 换成 `lectures/` 下的其他目录即打开其他讲。
""", autobold=False)
    p.table(
        headers=["操作", "按键"],
        rows=[
            ["下一页、上一页", "→、←"],
            ["全屏", "f"],
            ["演讲者视图", "p"],
            ["回到大纲", "右上角的「≡ 大纲」"],
        ],
    )
    p.slide("""
命令末尾加 `--lang en` 打开英文版。
""", autobold=False)
    p.notes("""
view 把这一讲渲染为网页，终端最后一行打印网页的路径，随后浏览器自动打开大纲。大纲是一棵章节树，每行前面的数字是那一页在整个幻灯片中的页码。
以后每次看课件：打开终端，进入课件的 ICS-PPT 目录，运行 view；建了 .venv 的，先运行 source .venv/bin/activate。ls lectures 列出当前有哪些讲。
翻页也可以用空格与 Shift + 空格；f 再按一次退出全屏；p 打开一个新窗口，显示讲稿与下一页。
浏览器没有自动打开时，打开终端最后一行打印的 index.html。
""")


def kit_more(p):
    p.title("运行演示、导出与更新")
    p.code("bash", MORE_COMMANDS)
    p.slide("""
- `--watch`：终端保持运行，浏览器打开 `http://localhost:3030`。此时演示方块的演示功能被启用，点击 ▶，命令将在本机运行，输出显示在页面底部 drawer 中
- `--to pptx`：导出可编辑的 PowerPoint
- `--pdf`：导出 PDF，需要本机装有 Chrome
- 导出的文件在 `ICS-PPT/build/` 下，完整的路径打印在终端中
- `git pull`：同步课件的更新
""", autobold=False)
    p.notes("""
课件中带 ▶ 按钮的方块是一条真实的命令，页面上印着的是某次运行录得的输出。用 --watch 打开课件时，终端不退出，作为本机的服务器；点 ▶ 后命令在你的电脑上运行，输出实时显示在页面底部的面板中，面板右上角显示退出状态与用时。演示方块旁的文件名按钮在页面右侧打开那个源文件。有些演示依赖额外的程序，例如 C 编译器，没有安装时面板报告找不到命令，页面上已经印着输出。
--to pptx 导出的是真实的文本框与图片，可以在 PowerPoint 或 WPS 中编辑，不需要浏览器；--pdf 用 Chrome 打印，PDF 的第一页是可以点击的大纲。两者的文件都在 ICS-PPT/build 下，终端打印完整的路径。
课件更新后，在仓库中运行 git pull。git pull 报告本地有修改时，说明你改动过课件目录中的文件；不需要保留这些改动时，先运行 git restore . 再 git pull。更新后 view 报错时，重新运行一次「课件的下载与安装」一页的后两条命令。
""")


# ==============================================================================
# 第二部分 · nano-quant
# ==============================================================================

# ==============================================================================
# 共享清单
# ==============================================================================

Q4_0_DECL = """void q4_0_quantize(const float *x, uint8_t *blk);    /* 32 floats in, 18 bytes out */
void q4_0_dequantize(const uint8_t *blk, float *x);  /* 18 bytes in, 32 floats out */"""

SELFTEST_Q4_0 = """q4_0  relative rms error 0.08408, max abs error 8
q4_0_back    f444b647e38b04afee71056cd0b1acac  matches
q4_0_bytes   942530b9c1c6ec1c41790c63de4317f8  matches"""

Q4_0_MODEL = """cd ../nano-quant
sh tools/fetch-model.sh                      # model.safetensors, config.json, tokenizer.json
python3 tools/mkmeta.py model -o model/meta.kv
./nano-quant quant model/model.safetensors --recipe q4_0 -o model/q4_0.nq
./nq2gguf model/q4_0.nq --meta model/meta.kv -o model/q4_0.gguf
ollama create nq-q4-0 -f model/Modelfile     # README.md's Modelfile, FROM ./q4_0.gguf
ollama run nq-q4-0"""


# ==============================================================================
# 实验框架
# ==============================================================================

def nq_pipeline(p):
    p.title("实验的流程：从 safetensors 到 ollama 加载的 GGUF")
    figure(p, "nq_pipeline", 1120)
    slide(p, """
- 对象：`model.safetensors` 中语言模型的 310 个 BF16 张量，量化为 4 位
- 你的函数在 `nano-quant quant` 中被调用，每个量化块调用一次
""")
    p.notes("""
nano-quant 是第二讲的配套实验：把 Qwen3-VL-2B-Instruct 的 model.safetensors（4.25 GB，625 个 BF16 张量）中语言模型的 310 个张量量化为 4 位，组装为 GGUF，由 ollama 加载运行；视觉塔的 315 个张量全部跳过。
四个程序接力：nano-quant quant 读 safetensors，按配方逐块量化，写出 .nq（量化后的张量数据区加一个目录）；tools/mkmeta.py 从 config.json 与 tokenizer.json 生成 GGUF 的键值段 meta.kv，24 个键值对；nq2gguf 把两者组装为 GGUF；ollama create 与 ollama run 加载它回答提示词。
框架给出命令行、文件读写、张量的选择、每个张量的格式与 GGUF 名字、按行量化的循环、Q6_K 格式、GGUF 组装与全部检查程序。你写的只有 impl/nano_quant.cpp 中与数据表示有关的函数：字节的组合、浮点格式的转换、三种量化格式的编码与还原。三个程序 nano-quant、nq2gguf、nq-selftest 都链接这个文件，调用其中函数的是 nano-quant 与 nq-selftest。
实验是课后练习，不计分；每一部分配有检查程序，输出与期望值一致说明这一部分正确。图中的大小是 q4_k_m 配方的：数据区 1 101 457 408 字节，GGUF 1 107 404 384 字节。
""")


def nq_call_graph(p):
    p.title("框架如何调用实现的函数")
    figure(p, "nq_call_graph", 1120)
    slide(p, """
`include/nano_quant.h` 是框架与你的代码之间的全部接口。
""")
    p.notes("""
图左是 nano-quant quant 的主循环（src/main_quant.cpp），图右是 include/nano_quant.h 声明、你在 impl/nano_quant.cpp 中实现的函数。框架只通过这个头文件调用你的代码，你的代码也只需要头文件中的两个辅助函数 nq_round 与 nq_q4_k_fit，完成实验不需要阅读 src/ 下的框架代码。
调用的位置有三处。st_open 打开文件时读前 8 个字节，交给 rd_u64le 组合为头长度 N，再按 N 读出 JSON 头并解析张量目录。之后对语言模型的每个张量按整行分段读入，每段至多 4 Mi 个权重，所以峰值内存只与段长有关，约 48 MB；st_read_f32 把每个 BF16 权重的 16 位交给 bf16_to_f32 得到 float；nq_quantize 按张量的格式把这一段切成块，对第 b 个块调用 q4_0_quantize(x, blk)，x 指向 src 中这一块的 32 个 float，blk 指向 dst 中这一块的 18 个字节（Q4_1 是 20 字节，Q4_K 是 256 个权重、144 字节）；nq_write_chunk 把 dst 写入 .nq 的数据区。
每个函数都是纯函数：只读参数、只写输出，不分配内存、不打印、不退出。一块只取决于它自己的 32 个输入，块与块之间没有联系。q4_0_dequantize 在 nano-quant 中没有调用点，nq-selftest 用它把编码还原为 float，与期望的 md5 比较。
""")


def nq_gguf(p):
    p.title("GGUF 文件布局")
    figure(p, "nq_gguf_layout", 1000)
    slide(p, """
- 头：魔数、版本、键值对、张量目录，由 `nq2gguf` 写出
- 数据区：310 个张量首尾相接，每个张量是它的块首尾相接，`nq2gguf` 原样复制
- 视觉部分的 315 个张量不在 GGUF 中：量化后的模型只接受文字输入
""")
    p.notes("""
GGUF 文件分两段。头由 nq2gguf 写出：魔数 GGUF、版本 3、张量数、键值对数，然后是键值对与张量目录，最后填充到 32 的倍数。键值对来自 meta.kv：mkmeta.py 从 config.json 与 tokenizer.json 生成架构参数、词表、合并规则与特殊词元共 24 个，nq2gguf 再加 general.file_type 与 general.quantization_version 两个。目录中每个张量一项：GGUF 名字、维数、各维长度、格式与它在数据区内的偏移。真实模型的头是 5 946 976 字节。
数据区是 310 个张量的字节首尾相接，顺序与 safetensors 的头相同：token_embd.weight 在前，然后是 28 层，每层 11 个张量，最后是 output_norm.weight；每个张量对齐到 32。张量的字节就是它的块首尾相接，这些字节由你的函数写出；.nq 的数据区与 GGUF 的数据区逐字节相同，nq2gguf 原样复制。
图按 q4_0 配方画：二维张量全部 Q4_0，数据区 968 249 344 字节，整个文件 974 196 320 字节；一维的归一化系数保持 F32，由框架原样复制。一层的 11 个张量是注意力的 attn_q、attn_k、attn_v、attn_output，前馈的 ffn_gate、ffn_up、ffn_down，以及 4 个归一化系数 attn_norm、ffn_norm（各 8 192 字节）、attn_q_norm、attn_k_norm（各 512 字节）；4 个合计 17 KB，在图中是灰线，名字标在下方。q4_k_m 配方下 embed_tokens 与部分层的 v_proj、down_proj 用 Q6_K，其余二维张量用 Q4_K，数据区 1 101 457 408 字节。
tools/ggufdump.py 读一个 GGUF 检查三项：目录中的偏移都对齐到 general.alignment、相邻张量首尾相接、最后一个张量的末端与文件长度相符。
""")


def nq_blocks(p):
    p.title("张量和量化块之间的关系")
    figure(p, "nq_tensor_blocks", 1120)
    slide(p, """
- 张量按行存放，每行从头切成 32 个权重一块，每块独立量化
- 每一块调用一次 `q4_0_quantize(x, blk)`：输入 32 个 float，输出 18 个字节，输出总字节数 = 行数 × 每行块数 × 18
""")
    p.notes("""
以 blk.0.attn_q.weight 为例：2048 行，每行 2048 个权重，safetensors 中按行存放。框架按整行读入 float，每行从头切成 32 个权重一块，共 64 块；对每一块调用一次 q4_0_quantize(x, blk)，x 指向这 32 个 float，blk 指向输出缓冲区中这一块的 18 个字节。行与行、块与块之间没有联系，所以每个块只取决于自己的输入，分段处理得到相同的字节。
张量在数据区的字节数 = 行数 × 每行块数 × 每块字节数 = 2048 × 64 × 18 = 2 359 296，与 nano-quant plan 为这个张量输出的最后一列相同；plan 各行的总和就是 quant 报告的数据区字节数。行长必须是块长的倍数，框架在 plan 阶段检查，不满足时报错退出。
nq-selftest 的调用方式相同：它构造 131072 个权重，经 nq_quantize 切成 4096 个块逐块调用 q4_0_quantize，再逐块调用 q4_0_dequantize 还原。
""")


# ==============================================================================
# 课上完成：Q4_0 的量化与还原
# ==============================================================================

def nq_q4_decl(p):
    p.title("Q4_0 的声明")
    p.code("c", Q4_0_DECL)
    figure(p, "nq_q4_params", 1000)
    slide(p, """
`d` 由 `f32_to_fp16` 写出、`fp16_to_f32` 读回，这两个函数属于 A 部分，先于 Q4_0 完成。
""")
    p.notes("""
两个函数的原型在 include/nano_quant.h 中，头文件保持不变。q4_0_quantize 的 x 指向一块的 32 个 float（128 字节），blk 指向这一块输出的 18 个字节；q4_0_dequantize 反过来，blk 指向 18 个字节，x 指向写回的 32 个 float。
图把两个参数放在第二讲的 Q4_0 块图上：块的前 2 个字节 blk[0..1] 是 fp16 的 d，低字节在前；后 16 个字节 blk[2..17] 是 qs，第 j 个编码字节的低 4 位存第 j 个权重的编码 q[j]，高 4 位存第 j + 16 个权重的编码 q[j + 16]。编码 q 取 0..15，4 位。
d 用 f32_to_fp16 写出、用 fp16_to_f32 读回，这两个函数是 A 部分的：q4_0 组的检查先于编码调用 f32_to_fp16，A 部分未完成时在这里停下。bf16_to_f32 与 rd_u64le 不参与 Q4_0。
""")


def nq_q4_rule(p):
    p.title("Q4_0 的量化规则：|x| 最大的权重对应编码 0")
    slide(p, r"""
- $\mathrm{amax}$：$|x_i|$ 最大的 $x_i$，带符号，并列时取第一个
- $d = \mathrm{amax} / (-8)$，$\mathrm{id} = 1/d$；$d = 0$ 时 $\mathrm{id} = 0$
- 编码：$q_i = \min\left(15,\ ⌊ x_i \cdot \mathrm{id} + 8.5 ⌋\right)$，$i = 0, 1, \dots, 31$，用舍入到 fp16 之前的 $d$
- 还原：$x_i = d \cdot (q_i - 8)$，$d$ 从块中读回
- 打包：`blk[2 + j] = q[j] | q[j + 16] << 4`，$j = 0, 1, \dots, 15$
""")
    figure(p, "nq_q4_grid", 1120)
    p.notes("""
规则按头文件逐步写出，每一步取整都已固定，一个输入只有一个正确的输出。amax 是 32 个权重中绝对值最大的一个，带符号，绝对值并列时取第一个；d = amax / −8，所以 d 的符号与 amax 相反；id 是 d 的倒数，d 为 0 时取 0。每个权重先乘以 id 落在 [−8, 8]，加 8.5 后落在 [0.5, 16.5]，强制转换为 int 截断小数部分，相当于对 x·id + 8 四舍五入，页面上的 ⌊ ⌋ 写的就是这个 (int)：被转换的数不小于 0.5，截断与向下取整相同；min 把唯一可能的 16 截到 15。编码用的是舍入到 fp16 之前的 float d。
图用第二讲练习里的六个权重：amax = 1.40 为正，d = −0.175，id ≈ −5.71。编码 0 对应 amax 本身，编码 8 对应 0，格点 d·(q − 8) 从 1.40 到 −1.225，q 增大时格点向负方向走；数据范围 [−0.4, 1.4] 内有 11 个格点，范围外 5 个。六个权重的编码依次为 10、8、6、5、3、0，还原值 −0.35、0、0.35、0.525、0.875、1.40。
第二讲练习中 m = 0 的做法取 d = 最大绝对值 / 7、编码 −8..7；Q4_0 取 / −8、编码 0..15 存为无符号 4 位，绝对值最大的权重总落在编码 0，最大值为正时格点是 −7|d| 到 8|d|。
还原时 x[i] = d · (float)(q[i] − 8)，d 从块中读回，是 fp16 往返之后的值。
打包：第 j 个编码字节 blk[2 + j] = q[j] | q[j + 16] << 4，j 取 0..15；d 的两个字节由 f32_to_fp16 给出，低字节在 blk[0]、高字节在 blk[1]。
两条全局约定：浮点运算全部用 float，按头文件的次序计算，不经过 double；Makefile 中的 -ffp-contract=off 与 -fno-fast-math 保持不变，它们保证同一份代码在不同机器上写出相同的字节。
""")


def nq_q4_check(p):
    p.title("课上的检查：nq-selftest q4_0，再由 ollama 运行 q4_0 模型")
    p.demo("A 部分与 Q4_0 完成之后", """cd ../nano-quant
make -s && ./nq-selftest q4_0""", output=SELFTEST_Q4_0)
    slide(p, """
- 编码字节与还原值的 md5 都 `matches` 即正确
""")
    p.demo("生成 model/q4_0.gguf，在 ollama 中输入问题", Q4_0_MODEL, interactive=True, timeout=0)
    p.notes("""
make 生成 nano-quant、nq2gguf 与 nq-selftest 三个程序。nq-selftest 按组检查，组名 a、scale_min、q4_0、q4_1、q4_k、q6_k；指定组名时只运行这些组，完成一组涉及的函数即可运行这一组。
q4_0 组在 512 个超块、131072 个权重上量化，即 4096 个 Q4_0 块：先把全部权重编码，比较编码字节的 md5（q4_0_bytes），再逐块还原，比较还原值的 md5（q4_0_back），两个 md5 都与 tests/expected-blocks.txt 一致时输出 matches，否则 DIFFERS，退出码 1。标准错误上另有一行相对均方根误差与最大绝对误差，正确时为 0.08408 与 8。
前 8 个超块是专门构造的边界输入：全零、全相等、含一个离群值、全负、全正、量级 1e-7、关于 0 对称、只有一个非零点；其余 504 个超块是随机数，量级从 1e-4 到 1e2，全部先截断为 BF16 再量化，即真实模型中权重的取值。
尚未实现的函数被调用时输出 nano_quant.cpp: <函数名> is not implemented yet 并以状态 3 退出。q4_0 组先于编码调用 f32_to_fp16 写出 d，所以要先完成 A 部分的 fp16 两个函数，再写 Q4_0。
q4_0 模型由你的代码生成。配方 q4_0 把二维张量全部量化为 Q4_0，一维张量保持 F32，只调用 A 部分与 Q4_0 的函数，B 部分的其余函数未完成时也能生成。tools/fetch-model.sh 下载约 4.3 GB 的三个文件，在课前完成；mkmeta.py 生成 GGUF 的键值段；quant 在标准输出写一行：数据区的 md5、输出文件、数据区字节数，与 tests/expected-qwen3vl.txt 中的 q4_0_nq_md5（fa4f64a2e2be8d8bdb7dad8c0ed06e27）与 q4_0_nq_bytes（968249344）相同即正确；nq2gguf 写出 model/q4_0.gguf，974196320 字节。
ollama 一侧先启动 ollama（ollama serve，或安装时注册的系统服务），model/Modelfile 按 README 写出，FROM 一行改为 ./q4_0.gguf；ollama create 把它导入为 nq-q4-0。ollama run 进入对话，在底部的终端中输入问题，/bye 或 Ctrl-D 结束。问「用一句话介绍你自己。」时，只完成 A 部分与 Q4_0 的实现得到的模型回答「我是一个AI助手，没有实体，但可以为你提供知识、解答问题、创作内容等，随时准备协助你。」检查结束后用 ollama rm nq-q4-0 删除这个模型。
""")


# ==============================================================================
# 其余部分与评测
# ==============================================================================

def nq_rest_a(p):
    p.title("课后 · A 部分：读字节的四个函数")
    figure(p, "nq_safetensors", 1120)
    slide(p, """
- `rd_u64le` 读头长度，字节顺序写反时 `nano-quant` 报告 `header length ... is impossible`
- `f32_to_fp16` 就近舍入、平局取偶，三种格式的 fp16 字段都由它写出
- 检查：`./nq-selftest a`，四项各一行 `ok  matches`
""")
    p.notes("""
A 部分是四个函数，它们在图中各有调用的位置。
rd_u64le 把 8 个字节按小端组合为 64 位整数，p[0] 是最低字节，用移位与按位或；uint8_t 参与移位之前提升为 int，左移 32 位及以上时要先转换为 64 位类型。框架用它读 safetensors 的头长度 N，本实验的模型文件 N = 76240；字节顺序写反时读出的数值大于文件长度，nano-quant 报告 header length ... is impossible 并提示检查移位方向。
bf16_to_f32 是一次移位：BF16 的 16 位就是 FP32 的高 16 位，符号与阶码相同，7 位尾数是 FP32 尾数的高 7 位。框架读入每个权重时调用它。
fp16_to_f32 按 1-5-10 三个字段组合，输入分为正规数、次正规数、零、无穷与 NaN 四类。f32_to_fp16 就近舍入、平局取偶，超出范围的值变为无穷，过小的值变为次正规数或零。三种量化格式的 fp16 字段都由 f32_to_fp16 写出，它的一处舍入错误会使整个量化结果与期望值不一致，所以检查程序把它与框架自带的实现逐个对照：默认按步长 251 抽取 float，--full 遍历全部 2^32 个 float，约 15 秒；并要求半精度能精确表示的每个值往返后不变。
检查：./nq-selftest a，四项各一行 ok matches，依次是 bf16_to_f32、f32_to_fp16、fp16_to_f32、rd_u64le。make test 的第 2 项用 rd_u64le 读两个样例文件的头：小端写入的必须被接受，大端写入的必须被拒绝。
""")


def nq_rest_q4_1(p):
    p.title("课后 · Q4_1：格点从最小值开始")
    figure(p, "nq_q4_1", 1120)
    slide(p, r"""
- $\mathrm{lo}$、$\mathrm{hi}$：最小与最大的 $x_i$，$m = \mathrm{lo}$
- $d = (\mathrm{hi} - \mathrm{lo}) / 15$，$\mathrm{id} = 1/d$；$d = 0$ 时 $\mathrm{id} = 0$
- 编码：$q_i = \min\left(15,\ ⌊ (x_i - \mathrm{lo}) \cdot \mathrm{id} + 0.5 ⌋\right)$，还原：$x_i = d \cdot q_i + m$
- 检查：`./nq-selftest q4_1`
""")
    p.notes("""
Q4_1 与 Q4_0 的差别是块里多存一个 fp16 的 m，共 20 字节、5.00 位/权重。lo 与 hi 是块中最小与最大的权重，d = (hi − lo) / 15，id = d ≠ 0 ? 1/d : 0，m = lo；编码 q[i] = min(15, (int)((x[i] − lo) · id + 0.5))，用舍入到 fp16 之前的 float d 与 lo；还原 x[i] = d · q[i] + m，d 与 m 从块中读回。(x[i] − lo) · id 不小于 0，(int) 的截断与向下取整相同，页面上写作 ⌊ ⌋。字节：blk[0..1] = d，blk[2..3] = m，blk[4 + j] = q[j] | q[j + 16] << 4。
图用同一组六个权重：lo = −0.40，hi = 1.40，d = 0.12，格点 d·q + m 从 lo 到 hi，16 个全部在数据范围内，与第二讲 m ≠ 0 的练习相同；编码依次为 0、3、6、8、11、15。
全相等的块：hi = lo，d = 0，id = 0，每个编码都是 0，还原值都是 m。
检查：./nq-selftest q4_1，两行 q4_1_back、q4_1_bytes 各 matches。
""")


def nq_rest_q4_k(p):
    p.title("课后 · Q4_K：256 个权重组成一个超块进行二次量化")
    figure(p, "nq_q4_k", 1120)
    figure(p, "nq_scale_bytes", 1120)
    slide(p, """
- 三步的公式在 `q4_k_quantize` 的注释，通过 `./nq-selftest q4_k` 检查正确性
""")
    p.notes("""
Q4_K 以 256 个权重为一个超块，分 8 个子块各 32 个。子块 j 的权重表示为 x ≈ D[j]·q − M[j]，与 Q4_1 的形式相同；D[j] 与 M[j] 再量化一次，写作超块共用的 fp16 系数与子块的 6 位整数之积：D[j] = d·sc[j]，M[j] = dmin·m[j]。一个超块 144 字节：d、dmin 4 字节，缩放段 12 字节，编码 128 字节，4.5 位/权重，与 Q4_0 相同。
q4_k_quantize 分三步，编号与头文件相同。第 1 步对每个子块调用框架提供的 nq_q4_k_fit(x + 32·j, &s[j], &o[j])，它用加权最小二乘在 21 个候选步长中搜索，求出 s[j] ≥ 0 与 o[j] ≥ 0，公式在 README 的 B 部分，这一步只需调用。第 2 步取 8 个 s[j] 中最大的 S 与 8 个 o[j] 中最大的 O，d = S/63，dmin = O/63，is = S > 0 ? 63/S : 0，io 同理，sc[j] = nq_round(is·s[j])，m[j] = nq_round(io·o[j])，都落在 0..63；d 与 dmin 用 f32_to_fp16 写入 blk[0..3]，8 对 (sc, m) 用 put_scale_min 写入 blk[4..15]。第 3 步按还原时读到的值重新计算编码：D = fp16_to_f32(f32_to_fp16(d))·sc[j]，M 同理，q[i] = D ≠ 0 ? clamp(nq_round((x[i] + M) / D), 0, 15) : 0。
编码段以 64 个权重为一组，每组 32 字节：第 g 组的第 l 个字节低 4 位存子块 2g 的第 l 个编码，高 4 位存子块 2g + 1 的第 l 个编码，与 Q4_0 的 j、j + 16 配对不同。还原 x[i] = (d·sc[j])·q[i] − dmin·m[j]，按括号的次序计算。
round 一律用 nq_round（就近舍入，平局取偶），(int)(x + 0.5f) 在负数与平局处与它不同。检查：./nq-selftest q4_k，同时运行 scale_min 组。
缩放段 12 个字节放 8 个 sc 与 8 个 m，各 6 位，共 96 位，没有空位。前 4 对各占一个字节的低 6 位：scales[k] 的位 5..0 是 sc[k]，scales[k+4] 的位 5..0 是 m[k]。后 4 对各拆成两段：高 2 位放在前 8 个字节的位 7..6，scales[k] 的位 7..6 是 sc[k+4] 的位 5..4，scales[k+4] 的位 7..6 是 m[k+4] 的位 5..4；低 4 位放在 scales[k+8]，位 3..0 是 sc[k+4] 的、位 7..4 是 m[k+4] 的。k 取 0..3。
impl/nano_quant.cpp 中给出的 get_scale_min 按这个排布读出第 j 对；你实现与它互逆的 put_scale_min：写入第 j 对之后，get_scale_min 读出第 j 对得到刚写入的值，其余 7 对保持原值。检查时 12 个字节的初值是任意的，8 对的写入次序每轮不同，共 64 × 64 轮，因此写入第 j 对时只改动属于这一对的位：先清掉这些位，再按位或写入。
检查：./nq-selftest scale_min，一行 scale_min ok matches；q4_k 组自动包含它。
""")


# ==============================================================================
# 第三部分 · bomb-lab
# ==============================================================================

# ==============================================================================
# bomb 的使用
# ==============================================================================

def bomb_task(p):
    p.title("实验 bomb-lab：阅读 bomb 的机器码，推出 8 个阶段的输入")
    p.image("assets/bomb_main_flow.svg", width_px=1000)
    slide(p, """
- `main` 函数对每个阶段依次调用三个函数：`read_line` 读取一行输入，`phase_k(line)` 检查输入是否正确，`phase_defused` 报告这个阶段已拆除；
- **要做的事**：阅读 `phase_1` ~ `phase_8` 的机器码，推出每个阶段接受的那一行，每个学号的 `bomb` 不同。
""")
    p.notes("""
实验 bomb-lab 配合第三讲「程序的机器级表示」，不计分。实验给出一个可执行文件 bomb，没有源码。
图是 main 的结构：对每个阶段 k，read_line 读一行输入，返回这一行的地址；phase_k 以这个地址为唯一的参数（%rdi），检查这一行；phase_k 返回时 main 调用 phase_defused，向服务器报告阶段 k 已拆除，然后进入下一个阶段。8 个阶段都返回之后程序输出 All phases are defused.，退出状态 0。
phase_k 认为这一行错误时调用 explode_bomb：它向服务器报告阶段 k 爆炸，显示爆炸的画面，程序以状态 1 退出，不返回。
任务是阅读 phase_1 到 phase_8 的机器码，推出每个阶段接受的那一行。每个学号对应一个 bomb，各同学的 bomb 接受的输入不同。
objdump 与 gdb 的用法在辅导课「汇编实验准备」中以课堂程序 bomb-light 讲过，本节不重复。
""")


def bomb_get(p):
    p.title("获取 bomb 二进制程序")
    p.image("assets/bomb_fetch.svg", width_px=980)
    slide(p, """
- **`make bomb ID=学号`**：在实验材料的目录中执行，需要安装 `curl` 或 `wget` 工具；
- **`bomb` 运行时保持与服务器的连接**：每个被拆除的阶段、每一次爆炸都会被记录，可通过 `./bomb -s` 查看计分板。
""")
    p.notes("""
实验材料是三个文件：取得 bomb 的 Makefile、实验说明 README.md、课堂演示用的 bomb-light（「汇编实验准备」中拆除的那个程序）。材料的位置与复制进容器的步骤在「汇编实验准备」的最后一页。
make bomb ID=学号 把学号发给课程服务器，服务器按学号链接出一个 bomb 并传回，保存为当前目录下的文件 bomb。ID= 之后写自己的学号。同一个学号每次得到的 bomb 相同；文件丢失或换了一台机器时再执行一次这条命令。
bomb 启动时连接课程服务器，运行期间保持连接：每个被拆除的阶段、每一次爆炸都记在学号下；连接不上服务器时程序以状态 3 退出。
./bomb -s 从服务器读取计分板并显示。计分板上没有学号，每个 bomb 以 8 位十六进制的编号代替，编号由服务器从学号算出，从编号不能得到学号。
bomb 的输入框上方显示它所属的学号，开始之前核对。
""")


def bomb_screen(p):
    p.title("运行 bomb：正确的一行进入下一个阶段，错误的一行爆炸")
    p.side_image("assets/bomb_phase.png", width="52%", alt="contain", side="left")
    slide(p, """
中上方是当前的阶段与进度，输入框右上角是这个 `bomb` 所属的学号，在输入框中输入一行，按 Enter 提交，`Ctrl-D` 退出。
""").under_side_image()
    p.image("assets/bomb_defused.png", width_px=420, caption="正确：进度 1/8，进入阶段 2")
    p.image("assets/bomb_boom.png", width_px=420, caption="错误：爆炸，程序退出，状态 1")
    p.notes("""
左边的截图是 ./bomb 启动后的画面（演示用的 bomb，学号 000000000001，终端 80 列 20 行）。程序启动时连接课程服务器，连接成功之后占据整个终端窗口。
从上往下：BOMB 与当前的阶段 PHASE 1；8 个方格是进度，已拆除的阶段显示为绿色，右边是已拆除的阶段数 0/8；一行规则：一行输入拆除一个阶段，输入错误即爆炸；输入框的标题是 phase 1 of 8，右侧是这个 bomb 所属的学号 ID 000000000001，开始之前核对它；框中 > 之后输入一行，按 Enter 提交；Ctrl-C 清空这一行，Ctrl-D 退出。
窗口少于 59 列或 15 行时，程序改为依次输出，不占据整个窗口；终端显示不正常时的环境变量见 README.md「终端显示不正常时」一节。
右上图：输入正确的一行之后，进度变为 1/8，第一个方格为绿色，中间一行 phase 1 defused，输入框的标题变为 phase 2 of 8，程序等待阶段 2 的一行。
右下图：输入错误的一行之后，爆炸的画面显示在窗口中央，phase 1 exploded，随后程序退出，退出状态 1。再次运行 bomb 从阶段 1 开始，已经拆除的阶段要重新输入，因此把推出的行写在答案文件中，每行一个阶段：./bomb answers.txt 先依次读文件中的行，空行跳过，文件读完之后从键盘读。
两种结果 bomb 都报告给服务器：被拆除的阶段记在学号下，爆炸的次数也记在学号下，每一次都计。
三张截图都来自演示用的 bomb，不连接服务器。
""")


def bomb_board(p):
    p.title("计分板：./bomb -s")
    p.image("assets/bomb_board.png", width_px=840, framed=True)
    slide(p, """
每行代表一个 `bomb` 的解题进度，其中自己的那一行以 `>` 标出。已拆除的阶段显示为绿色。
""")
    p.notes("""
截图是 ./bomb -s 的画面，来自一个本地的演示服务器，上面有 5 个演示用的 bomb。
每行依次是名次、bomb 的编号、已拆除的阶段数、爆炸的总次数，以及 8 个阶段各自的爆炸次数。已拆除的阶段，它的次数显示为绿色；尚未到达的阶段显示为 .。第 2 行的 bomb 在阶段 3 爆炸过 1 次，之后拆除了它，因此这一格是绿色的 1。
自己的一行以 > 标出并加亮；底部的一行给出 bomb 的总数与自己所在的行。按键：上下箭头移动一行，左右箭头或 PgUp、PgDn 翻页，Home、End 到第一页与最后一页，r 重新从服务器读取，q 退出。
名次的排列：已拆除的阶段多的在前；阶段数相同时，爆炸次数少的在前；仍相同时，先完成的在前。
输出接到管道或文件时，计分板是同样的表格，没有颜色：已拆除的阶段在次数之后带 *，自己的一行以 > 开头；./bomb -s | grep '^>' 取出自己的一行。
计分板上不显示学号：bomb 的编号是 8 位十六进制数字，由服务器从学号算出。
""")


# ==============================================================================
# 课上完成：phase 1
# ==============================================================================

def bomb_objdump(p):
    p.title("反汇编 bomb：objdump -d 与 -D 输出全部汇编代码")
    p.demo("反汇编整个 bomb，保存到文件，找到 phase_1", """cd examples
objdump -d bomb > bomb.asm
objdump -D bomb > bomb-all.asm
grep -c '^Disassembly of section' bomb.asm bomb-all.asm
grep -n '<phase_1>:' bomb.asm""",
           output="""bomb.asm:5
bomb-all.asm:28
935:000000000000ab8d <phase_1>:""")
    slide(p, """
- **`objdump -d`**：反汇编存放指令的 5 个节（`.init`、`.plt`、`.plt.got`、`.text`、`.fini`），`bomb` 的全部汇编代码保存在 `bomb.asm` 中；
- **`objdump -D`**：反汇编全部 28 个节，`.rodata`、`.data` 等数据节的字节也按指令译出；
- **`phase_1` 在 `bomb.asm` 的第 935 行**：用编辑器打开 `bomb.asm`，按函数名查找。
""")
    p.notes("""
-d 是 --disassemble，反汇编可执行的节；-D 是 --disassemble-all，反汇编所有的节。两者的用法在「汇编实验准备」中以 bomb-light 讲过。演示在 examples/bomb 上运行，输出 2026-10-19 在本机录得（binutils 2.46）。
objdump 在每个节之前输出一行 Disassembly of section 节名:，grep -c 数出这样的行：-d 输出 5 个节，-D 输出 28 个节。-D 多出的是数据节，例如 .rodata、.data、.dynsym：它们的字节也按指令译出，机器码一栏是数据本身，汇编指令一栏没有意义。查看数据用 objdump -s，或 gdb 的 x/s。
bomb 是静态链接的程序，C 库的函数也在文件中，因此 bomb.asm 有 196847 行、10.7 MB，bomb-all.asm 有 299932 行、15.9 MB（wc -l 与 ls -l 的结果）。phase_1 在 bomb.asm 的第 935 行；在 bomb-all.asm 中是第 17453 行，-D 把排在 .text 之前的 .dynsym 等数据节也译了出来。
只看一个函数时用 objdump -d --disassemble=phase_1 bomb，它只输出 phase_1。学生自己的 bomb 用同样的命令反汇编。演示之后 examples/ 中留有 bomb.asm 与 bomb-all.asm。
""")


def bomb_space(p):
    p.title("bomb 进程的地址空间：指令在 .text，字符串在 .rodata")
    p.side_image("assets/bomb_address_space.svg", width="40%", alt="contain", side="left")
    p.spacer(120)                      # the text sits level with the middle of the figure
    slide(p, """
- **代码段 `.text`**：程序的指令。`objdump -d` 反汇编的是这一段，`phase_1` 从 `0xab8d` 开始；
- **只读数据 `.rodata`**：字符串常量等只读的数据，`0xcb030` 在其中；
- **数据段与栈**：`read_line` 读入的一行在 `.bss` 中，`phase_1` 的栈帧中保存了它的地址。

地址空间的完整机制在讲虚拟内存时说明。
""")
    p.notes("""
图是 bomb 进程的地址空间，高地址在上，与第三讲「运行时栈」一页的图相同，加上了只读数据一段。各段左侧的地址是文件中的地址，由 readelf -S bomb 得到：.text 从 0xa1c0 开始，.rodata 从 0xcb000 开始，.data 从 0x103000 开始。
代码段 .text 存放指令，objdump -d 反汇编的就是这一段。phase_1 从 0xab8d 开始，lea 在 0xab99。
只读数据 .rodata 存放字符串常量等只读的数据，运行时不能写入。0xcb030 在 0xcb000 之后，属于这一段。
数据段 .data 与 .bss 存放全局变量。read_line 把读入的一行放在一个全局的缓冲区中（.bss），返回它的地址；phase_1 把这个地址存在自己的栈帧中（-0x8(%rbp)）。栈在高地址一端，向低地址生长。
bomb 是位置无关的程序：文件中的地址是相对装入位置的偏移，运行时的地址等于装入位置加这个偏移，装入位置在程序启动时由内核决定，每次运行都不同。程序启动之前 gdb 读的是文件，x/s 0xcb030 按文件中的地址读；启动之后要用运行时的地址或符号。「汇编实验准备」的 gdb 演示讲过这一点。
地址翻译、页表与装入位置的选择属于虚拟内存，在后面的讲次中说明；本节把图中的地址当作内存地址使用。
""")


def bomb_lea(p):
    p.title("lea 的目标：下一条指令的地址加偏移量，objdump 在行尾算出")
    p.image("assets/bomb_lea.svg", width_px=1040)
    slide(p, """
- **`%rip` 相对寻址**：位置无关的程序用「下一条指令的地址 + 偏移量」取得数据的地址；装入位置变化时两者同时变化，和仍然指向同一个字节；
- **`# cb030 <__rseq_flags+0x2c>`**：`objdump` 算出的文件地址，与离它最近的符号加偏移；字符串常量没有自己的符号。
""")
    p.notes("""
lea 0xc0490(%rip),%rdx：操作数是以 %rip 为基址的内存操作数。执行这条指令时 %rip 已经指向下一条指令 0xaba0，有效地址是 0xaba0 + 0xc0490 = 0xcb030。lea 只算地址，不读内存，把 0xcb030 写入 %rdx。
机器码 48 8d 15 90 04 0c 00 的后 4 个字节是偏移量 0xc0490，小端存放。
位置无关的程序中，指令与数据一起装入，两者的距离不随装入位置变化，因此用「下一条指令的地址 + 偏移量」取得数据的地址：装入位置变化时 %rip 随之变化，和仍然指向同一个字节。第三讲的「用已有的指令实现调用与返回」一页与「汇编实验准备」的 sscanf 参数一页都出现过这种写法。
objdump 在行尾的注释中写出了这个和：# cb030；尖括号中的 __rseq_flags+0x2c 是离 0xcb030 最近的符号加偏移，与字符串无关，字符串常量在符号表中没有名字。
.rodata 中 0xcb030 起的字节是 54 68 65 20 …，即字符 T h e 空格 …，到值为 0 的字节结束。
""")


# ==============================================================================
# 第四部分 · parallel-dot
# ==============================================================================

# ==============================================================================
# 共享清单
# ==============================================================================

MATVEC_SIGNATURE = "static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols);"

KERNEL_LAUNCH = """__global__ void matvec_q4_0_kernel(float *y, const uint8_t *w, const float *x, int cols);

matvec_q4_0_kernel<<<rows, 256>>>(y, w, x, cols);   /* in the host function matvec_q4_0, given */"""

SCALAR_MATVEC = """size_t row_bytes = cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES;   /* cols / 32 * 18 */
for (size_t r = 0; r < rows; r++) {
    const uint8_t *blk = w + r * row_bytes;                      /* the first block of row r */
    float sum = 0;
    for (size_t c = 0; c < cols; c += 32, blk += 18) {           /* one block per step */
        float d = q4_0_scale(blk);
        for (int j = 0; j < 16; j++) {                           /* one code byte per step */
            int lo = (blk[2 + j] & 15) - 8, hi = (blk[2 + j] >> 4) - 8;
            sum += d * lo * x[c + j] + d * hi * x[c + j + 16];
        }
    }
    y[r] = sum;
}"""

SIMD_TEST_TAIL = """  matrix           rows x cols          MB  scalar ms  GB/s    simd ms  GB/s  speed-up
  attn_q           2048 x 2048         2.4    3.134     0.8    0.479     4.9      6.5x
  ffn_gate, ffn_up 6144 x 2048         7.1    9.246     0.8    1.517     4.7      6.1x
  ffn_down         2048 x 6144         7.1    9.161     0.8    1.474     4.8      6.2x
  output           151936 x 2048     175.0  243.557     0.7   40.167     4.4      6.1x
  pass  speed-up on attn_q 6.5x, at least 4x required

all passed"""


# ==============================================================================
# 实验框架
# ==============================================================================

def pd_frame_ops(p):
    p.title("mini-ollama 的结构：main.c 经算子表调用算子")
    figure(p, "pd-frame-ops")
    p.slide("""
- **两部分修改同一个算子 `matvec_q4_0`**：第一部分写 `ops/simd.c` 中的函数（AVX2），第二部分写 `ops/cuda.cu` 中由它启动的 kernel `matvec_q4_0_kernel`。
""", autobold=False)
    p.notes("""
parallel-dot 配合第三讲第五部分「性能瓶颈与向量化」给出的两个提速方案：一条指令完成多次乘加（SIMD），使用更多的运算单元（GPU）。实验分两部分，依次对应两个方案，按顺序完成；两部分都是课后练习，不计分。两部分的程序都是第三讲的 mini-ollama：读取 Q4_0 格式的模型文件 q4_0.gguf，逐个 Token 生成回答。
图的左边是框架：model.c 把 GGUF 文件载入内存并按名字找到每个张量；main.c 是逐个 Token 生成的循环；selftest.c 是检查程序。tokenizer.c 把提示词转换为 Token 编号、把编号转换为文字，图中省略。中间是 ops.h 中的算子表 struct ops：每个成员是一个函数指针，十个算子与五项内存操作（init、alloc、upload、download、finish）；main.c 与 selftest.c 只经由这张表调用算子，表的 matvec 一项就是两部分要写的算子。
右边是算子表的三种实现，每种一个文件：scalar.c 是第三讲 mini_ollama.c 中的运算，在 CPU 的一个线程上每次处理一个元素，以 -fno-tree-vectorize 编译，是两部分比较速度的基准与检查程序的参照；simd.c 用 AVX2 intrinsics，每次处理 8 个 float，其中只有 matvec_q4_0 没有写，第一部分写它；cuda.cu 中每个算子是一个 kernel，其中只有 matvec_q4_0_kernel 的函数体没有写，第二部分写它，启动它的主机端函数 matvec_q4_0 已给出。
make 把 main.c 与 selftest.c 按每套算子各编译一次，用宏 -DOPS=ops_scalar、ops_simd、ops_cuda 指明程序使用哪一张表，目标文件放在 build/scalar、build/simd、build/cuda 下。一个程序只含一套算子：三个 mini-ollama 读取模型文件、回答 prompt.txt 中的问题、输出生成速度；两个 ops-selftest 以 scalar.c 为参照检查另一套算子的每个函数，再比较 matvec 的速度，不需要模型文件。两部分的命令都在 parallel-dot/ 目录下执行。
""")


def pd_frame_token(p):
    p.title("生成一个 Token：197 次 matvec_q4_0")
    figure(p, "pd-frame-token")
    p.slide("""
- **生成速度由 `matvec_q4_0` 决定**：每次调用读取一个矩阵的全部权重，一个 Token 共 1.7 × 10⁹ 个。实测在 CPU 上运行时，时间占用在 99% 以上、在 GPU 上运行时，kernel 的时间占用也在 94% 左右。
""", autobold=False)
    p.notes("""
main.c 生成一个 Token 的过程：embed 取 token_embd 中这个 Token 的一行，还原为 2048 个 float，作为 x；x 依次经过 28 层，每层先是注意力部分，再是前馈部分；最后 next_token 用输出层算出每个 Token 的分数，argmax 取分数最高的一个作为下一个 Token。
每层的注意力部分调用 4 次 matvec：attn_q、attn_k、attn_v 把归一化后的 x 变为查询、键、值，attn_output 把各头的结果变回 2048 维；前馈部分调用 3 次：ffn_gate 与 ffn_up 把 2048 维变为 6144 维，swiglu 之后 ffn_down 变回 2048 维。28 层共 196 次，输出层 1 次，合计 197 次。
图中每个矩阵按同一个比例画出：attn_q 与 attn_output 是 2048 × 2048；attn_k 与 attn_v 是 1024 × 2048，模型有 16 个查询头、8 个键值头，每头 128 维；ffn_gate 与 ffn_up 是 6144 × 2048；ffn_down 是 2048 × 6144，它乘的向量长 6144。输出层与词嵌入共用 token_embd，151936 × 2048，按这个比例高度是 attn_q 的 74 倍，图中截断。
因此 mini-ollama 中 matvec_q4_0 收到的 cols 只有 2048 与 6144 两种，rows 有 1024、2048、6144 与 151936 四种。生成一个 Token 要算 28 × 20480 + 151936 = 725376 行的内积，全部矩阵的每个权重各参与一次乘加，约 1.7 × 10⁹ 次。检查程序另用 32 与 96 列的矩阵检查块的边界。
其他算子处理的是向量：embed、rmsnorm、rope、swiglu、add 每次 1024 到 6144 个 float，argmax 每个 Token 一次、151936 个 float；attn_scores 与 attn_values 读取注意力的缓存，每个已处理的 Token 1024 个 float，随 Token 数增长。2026-10-19 在本机给算子表的每一项计时（参考答案，prompt.txt：13 个 Token 的提示词，回答 16 个 Token）：matvec 占标量版本与 AVX2 版本运行时间的 99.7%；GPU 上占 nsys 统计的 kernel 时间的 93.9%。提示词为 400 个 Token 时，注意力的两个算子增长到 AVX2 版本运行时间的 3.4%、GPU kernel 时间的 5.8%，matvec 仍占 96.3% 与 89.2%。
""")


def pd_frame_args(p):
    p.title("matvec_q4_0 的参数：w 的 rows 行各与 x 相乘，写入 y")
    p.code("c", MATVEC_SIGNATURE)
    figure(p, "pd-frame-args")
    p.slide("""
- **以 float 计算，相加的次序不限**。
- **每个块与实验 nano-quant 中 q4_0 相同**：2 字节 fp16 的缩放系数 `d`，16 字节各存两个 4 位编码。
""", autobold=False)
    p.notes("""
两部分写的都是这个算子：Q4_0 格式的矩阵 W 与一个 float 向量 x 的乘积 y = W x。第一部分的函数签名如页面所示，第二部分的 kernel 以 (y, w, x, cols) 为参数，行号由线程块的编号给出。
五个参数在图上的位置：w 指向 rows 行权重的第一个字节，矩阵按行存放，每行 cols 个权重占 cols / 32 × 18 个字节，第 r 行从 w + r × (cols / 32 × 18) 开始；x 指向 cols 个 float，与矩阵的一行一样长；y 指向 rows 个 float，与矩阵一样高。cols 是 32 的正倍数，rows 不小于 1。
函数的规则（ops.h 与 docs/simd.md）：对每一行 r 写入 y[r] = Σ W[r][c] · x[c]，c 从 0 到 cols − 1，W[r][c] 是第 r 行第 c 个权重，以 float 计算；乘积相加的次序不限，标量版本按 c 的次序累加，向量版本分 8 路、按块累加，两者的结果只在最后几位有差别，检查程序允许这种差别。函数只写 y 的 rows 个 float，只读 w 的 rows × cols / 32 个块与 x 的 cols 个 float，不分配内存、不输出、不退出。
w 与 x 的地址都没有对齐的保证：一个块的编码从块的第 2 个字节开始，地址一般不是 16 的倍数；x 由 calloc 或 malloc 分配，地址不保证是 32 的倍数。
Q4_0 的块与第二讲实验 nano-quant 写出的相同：第 0、1 字节是缩放系数 d，半精度浮点数（1 位符号、5 位指数、10 位尾数），低字节在前；ops.h 给出的 q4_0_scale(blk) 读出块 blk 的 d，返回 float，CPU 与 GPU 上都可以调用。第 2 + j 字节（j = 0..15）是两个编码：低 4 位是第 j 个权重的编码，高 4 位是第 j + 16 个权重的编码，与第二讲 Q4_0 图中的画法相同。编码 q 取 0 到 15，表示的权重是 d × (q − 8)。cols = 2048 时一行是 64 个块首尾相接，共 1152 字节；第 b 个块从行首第 18b 个字节开始，存第 32b 到 32b + 31 个权重。
x 的 32 个 float 与一个块的 32 个权重对应：第 c 列在第 c / 32 个块中，它的编码在这个块的第 2 + c % 16 个字节，c % 32 小于 16 时是低 4 位，否则是高 4 位。本节的图中，权重 0 到 15 的编码画成实心的蓝色格，权重 16 到 31 的编码画成空心的蓝色格。
""")


def pd_frame_scalar(p):
    p.title("matvec_q4_0 的标量版本")
    p.code("c", SCALAR_MATVEC)
    figure(p, "pd-frame-block")
    p.notes("""
这是 ops/scalar.c 中已给出的标量版本，逐行计算内积：外层循环取每一行的第一个块，中层循环每步处理一个块，先读出 d，内层循环每步处理一个编码字节，还原出两个权重、分别乘以 x 中对应的元素、累加到 sum。一行的全部块处理完后写入 y[r]。
下方的图是 blk 指向的一个块的 18 个字节：blk[0]、blk[1] 是缩放系数 d，fp16，低字节在前，q4_0_scale(blk) 读出它；blk[2 + j] 的低 4 位是第 j 个权重的编码 q[j]，即代码中的 lo + 8，高 4 位是第 j + 16 个权重的编码 q[j + 16]，即 hi + 8。内层循环的第 j 步处理的就是这一个字节中的两个编码，分别乘以 x[c + j] 与 x[c + j + 16]。
两部分的实现要得到同样的结果：AVX2 版本把中层循环的一步改为对 32 个权重同时操作，CUDA 版本让 256 个线程分担一行的列。两者相加的次序与标量版本不同，各项结果的最后几位有差别，检查程序按定义用 double 计算期望值，允许这种差别。
scalar.c 以 -O2 -fno-tree-vectorize 编译：GCC 12 起在 -O2 下自动向量化部分循环，这个选项使标量算子保持每次处理一个元素，它是两部分比较速度的基准。ops-selftest 的第一组用它检查另一套算子中已给出的函数，第二组按 ops.h 的定义检查 matvec，第三组比较 matvec 的速度。
""")


# ==============================================================================
# 课上完成：AVX2 版本的 matvec_q4_0
# ==============================================================================

def pd_class_plan(p):
    p.title("AVX2 版本的做法：8 路累加")
    figure(p, "pd-class-plan", width=980)
    p.slide(r"""
- **一个块的 32 个权重共用 $d_b$**，$d_b$ 可以提到块内的求和之外：在标量版本中，先还原权重，再与 $x$ 相乘，而 AVX2 版本中，先与 $x$ 相乘、累加到 bsum，最后才乘以 $d_b$。
  $$
  y_r=\sum_{b=0}^{63}\sum_{i=0}^{31} d_b\,(q_{b,i}-8)\,x_{32b+i}=\sum_{b=0}^{63} d_b\sum_{i=0}^{31}(q_{b,i}-8)\,x_{32b+i}
  $$
""", autobold=False)
    p.notes("""
AVX2 版本对每一行用一个 256 位寄存器保存 8 路累加器 acc，每一步处理一个块的 32 个权重：把块的 16 个字节拆出 32 个编码，减 8，转为 float；分 4 组、每组 8 路，与 x 中对应的 32 个 float 相乘，乘积累加到块的 8 路部分和 bsum；bsum 乘以这个块的 d，累加到 acc；然后处理下一个块。一行的 64 个块（cols = 2048 时）处理完后，hsum8 把 acc 的 8 个 float 相加为一个 float，写入 y[r]。
页面上的公式：b 是块的编号，i 是块内的下标，第 b 个块的缩放系数是 d_b、第 i 个编码是 q_{b,i}，与它相乘的是 x[32b + i]。还原的权重是 d_b · (q_{b,i} − 8)，即「matvec_q4_0 的标量版本」一页块图下方的一行；y_r 是这一行 2048 个权重与 x 的乘积之和，即左边。标量版本按左边计算，sum += d * lo * x[c + j] 每个权重乘一次 d，一个块 32 次。一个块的 32 个权重乘的是同一个 d_b，乘法对加法分配，d_b 可以提到块内的求和之外，得到右边：块内先把 (q_{b,i} − 8) · x[32b + i] 相加，再乘一次 d_b。AVX2 版本按右边计算：块内的和分在 bsum 的 8 路中，第 k 路是 i = k、8 + k、16 + k、24 + k 四项之和，8 路之和就是右边块内的和；_mm256_mul_ps 把 8 路各乘以 d_b，一条指令，加到 acc 的 8 路上；hsum8 把 acc 的 8 路相加，得到对全部块的和。先还原权重的写法每个块要 4 次 8 路的乘法乘 d，这种写法只要 1 次。
两边在实数上相等；在 float 中，乘法与加法的次序不同，舍入不同，两种写法的结果最后几位有差别。实验的规则是「以 float 计算，相加的次序不限」：检查程序用 double 按定义计算每一行的期望值，差不超过这一行 Σ |W[r][c] · x[c]| 的 10⁻⁵ 即通过，不同的次序带来的差别在 10⁻⁷ 量级。
hsum8 在 simd.c 的开头，已给出。图的下半部分每一行是一个 %ymm 的 8 路，通道 0 在最右边，与第三讲的图相同；虚线的几路中的数据不再需要。第一步把高 128 位的 4 个 float 加到低 128 位的 4 个上（_mm256_extractf128_ps 取出高 128 位，_mm_add_ps 相加），结果是一个 128 位的值，高 4 路不再使用；然后两次 _mm_hadd_ps(s, s) 把相邻的两个相加：第一次之后第 0、1 路各是 4 个之和，第 2、3 路是这两个数的重复；第二次之后第 0 路是 8 个之和，_mm_cvtss_f32 取出最低的 32 位，写入 y[r]。第一步把两半相加，原因是 AVX 的 _mm256_hadd_ps 只在每个 128 位之内把相邻两个相加，高 128 位与低 128 位之间的相加要先用 _mm256_extractf128_ps 取出高 128 位，再用 _mm_add_ps 逐路相加，一条指令做 4 次加法；两半相加之后是一个 128 位的值，_mm_hadd_ps 一条指令把相邻两个相加，两次得到 8 个之和。各个数相加的次序不改变实数上的和，hsum8 的次序由可用的指令决定。这一步称为水平相加，每一行只在全部块处理完之后做一次。
8 路累加器的含义与第三讲「一轮向量循环」一页相同：s₀ 累加下标模 8 为 0 的那些积，s₁ 累加模 8 为 1 的，依此类推，相加之后才是内积；第三讲用 vpaddd 累加 8 个 int，本节用 _mm256_add_ps 累加 8 个 float。
同一文件中已给出的 dot（rmsnorm 与 attn_scores 调用的 float 内积）就是 8 路累加、最后 hsum8 的写法，可以当作例子阅读。
simd.c 中 matvec_q4_0 上方的 TODO 注释写出一种做法与各步用到的 intrinsics，docs/simd.md「函数的规则」一节把它列为六步；课上按这个顺序把函数写出来。第一、二步与同一文件中的 embed 相同。第三步得到 q − 8 的 float，不乘 d：embed 在这一步用 decode8 乘以 d，把还原的权重写入内存；matvec_q4_0 把 q − 8 直接与 x 相乘，d 在第五步对整个块乘一次，即公式的右边。第三步中 lo 与 hi 的后 8 个先用 _mm_srli_si128 右移 8 个字节；第四步是第三讲向量循环的 float 版本；第六步的 hsum8 在文件开头，已给出。
函数体初始时是两行：(void) 一行使未使用的参数不产生警告，todo("matvec_q4_0") 输出 matvec_q4_0 is not implemented yet 并以状态 3 退出程序；把这两行换成实现。
两条规则：只用非对齐加载 _mm_loadu_si128 与 _mm256_loadu_ps，w 与 x 的地址都没有对齐的保证，对齐加载可能使程序以段错误终止；只读每个块的 16 个编码字节，用 256 位加载读编码会多读 16 字节，mini-ollama-simd 把模型文件映射到内存，最后一行之后可能没有可读的内存。
simd.c 以 -mavx2 编译，不加 -mfma，可以使用 <immintrin.h> 中的 AVX2 intrinsics，其他文件不以 -mavx2 编译。每条 intrinsic 对应的指令与用法见 Intel Intrinsics Guide（https://www.intel.com/content/www/us/en/docs/intrinsics-guide/）。
""")


def pd_class_decode(p):
    p.title("每个块的处理：16 字节拆成 32 个编码，每 8 个转为 8 个 float")
    p.slide("""
- **三步**：用 128 位加载指令取出 16 个编码字节；按位与、右移拆出低 4 位与高 4 位；每 8 个编码扩展、减 8、转换为 float 类型。
""", autobold=False)
    p.frames("assets/pd-class-decode-1.svg", "assets/pd-class-decode-2.svg",
             "assets/pd-class-decode-3.svg", width_px=1120)
    p.notes("""
这一页三帧，是 simd.c 中 TODO 注释给出的前三步。前两步与同一文件中已给出的 embed 相同。第三步得到的是 q − 8 的 float，不乘 d：embed 在这一步乘以 d，把还原的权重写入内存；matvec_q4_0 把 q − 8 直接与 x 相乘，d 在第五步对整个块乘一次，即上一页公式的右边。
第一帧：块的 18 个字节在内存中，q4_0_scale(blk) 读出 d；_mm_loadu_si128(blk + 2) 一条 128 位的加载取出 16 个编码字节，放入一个 128 位寄存器（%xmm）。只读 16 字节：一个块的编码只有 16 字节，用 256 位加载读编码会多读 16 字节，mini-ollama-simd 把模型文件映射到内存，最后一行之后可能没有可读的内存，多读的字节会引起段错误。用非对齐加载：编码从块的第 2 个字节开始，地址一般不是 16 的倍数，对齐加载 _mm_load_si128 会使程序以段错误终止。右上角放大了 qs0：一个字节含两个 4 位编码，低 4 位是第 0 个权重的编码 q0，高 4 位是第 16 个权重的编码 q16，写作 q0-4 与 q16-4，后缀是位数。
第二帧：与 0x0f 按位与（_mm_and_si128）得到每个字节的低 4 位，即第 0 到 15 个权重的编码 lo；每个 16 位右移 4 位（_mm_srli_epi16）再按位与得到高 4 位，即第 16 到 31 个权重的编码 hi。右移按 16 位进行，相邻字节的位会移入，所以右移之后仍要按位与。
第三帧：每 8 个编码扩展为 8 个 32 位整数（_mm256_cvtepu8_epi32，取 128 位寄存器的低 8 个字节），减 8（_mm256_sub_epi32），转换为 8 个 float（_mm256_cvtepi32_ps）；lo 与 hi 的后 8 个先把寄存器右移 8 个字节（_mm_srli_si128，embed 中的写法），移到低 8 个字节。图中四组各用一种颜色，每组得到的 %ymm 画在这 8 个编码的下方，颜色相同，两条虚线连接两者；hi 的两组与前面的图一样画成空心。一个 %ymm 是 32 字节，%xmm 是 16 字节，图中 %ymm 画得比 %xmm 长，没有按比例。四组依次是权重 0..7、8..15、16..23、24..31，分别与 x[c..c+7]、x[c+8..c+15]、x[c+16..c+23]、x[c+24..c+31] 对应，c = 32b。第三帧中编码后的数字同样是位数：lo 与 hi 中每个编码占一个字节，写作 q0-8；%ymm 中每个编码是一个 32 位 float，写作 q0-32，它的值是 q − 8。
""")


def pd_class_mac(p):
    p.title("一组 8 个权重与 x 的 8 个 float：乘，累加到 bsum")
    p.spacer(72)                       # the figure alone sits mid-page
    figure(p, "pd-class-mac")
    p.notes("""
这是 TODO 注释的第四步，第三讲「一轮向量循环：8 个通道同时乘、同时加」一页的 float 版本。上面两行是操作数：一个 %ymm 中的 8 个 float，即一组 8 个编码减 8 的结果，与上一页相同写作 q0-32，后缀是位数；x 中对应的 8 个 float，由 _mm256_loadu_ps(x + c + 8g) 读入，x 的地址没有对齐的保证，用非对齐加载。_mm256_mul_ps 一条指令完成 8 次乘法，8 个积放在一个寄存器中；_mm256_add_ps 一条指令把 8 个积加到 bsum 的 8 路部分和上。通道 0 在最右边，与第三讲的图相同。
一个块有 4 组，这两条指令各做 4 次：bsum 的每一路累加 4 个积，第 k 路累加权重 k、8 + k、16 + k、24 + k 的积，块的 32 个权重全部计入。bsum 在处理一个块之前清零（_mm256_setzero_ps），或者第一组直接用乘积初始化。
之后是第五步：bsum 乘以 d（_mm256_set1_ps 把 d 复制到 8 路，_mm256_mul_ps 相乘），_mm256_add_ps 累加到行的 8 路累加器 acc。第三讲的向量循环每轮处理 8 个 int；这里每个块处理 32 个权重，用 4 次乘、4 次加、再 1 次乘、1 次加。
Makefile 以 -O2 -mavx2 编译 simd.c，不加 -mfma：讲义讲到 AVX2 为止，融合乘加在这些算子中的收益很小。
""")


def pd_class_check(p):
    p.title("检查 attn_q 上的加速比")
    p.demo("检查每个算子的结果，比较 matvec 的速度", """cd ../parallel-dot
make -s test-simd | tail -n 8""",
           output=SIMD_TEST_TAIL, bold=[2, 6, 8])
    p.notes("""
make test-simd 编译三个程序，然后运行 ./ops-selftest-simd，不需要模型文件。程序分三组，每一项输出一行 pass 或 FAIL，最后输出 all passed 或 N failed，退出码为失败的项数。页面上是 tail -n 8 取出的第三组的表与最后一行，毫秒数与 GB/s 因机器而异。
第一组「operators」检查已给出的算子，每个算子与 scalar.c 中的同名算子处理同一组输入，这一组在开始之前就全部通过。第二组「matvec」检查你的函数：矩阵为 64 × 32、64 × 96、1000 × 2048（两组，其中一组 x 的量级为 64）与 64 × 6144，32 与 96 检查只有一个块与块数不是 2 的幂的行；缩放系数有正有负，16 个编码值都出现，另有 d 为 0 与次正规数的行。每一行的期望值按 ops.h 中的定义用 double 计算，结果与它的差不超过这一行 Σ |W[r][c] · x[c]| 的 10⁻⁵ 即通过；不同的累加次序带来的差别在 10⁻⁷ 量级，漏掉 2048 个权重中的一个，差别是 1/2048，远大于这个界限。
第三组「speed」用模型中的四种矩阵比较 simd.c 与 scalar.c 的 matvec_q4_0：每种至少运行 0.2 秒，取 3 次中最快的一次，输出每次矩阵向量乘的毫秒数、每秒读取的权重字节数与加速比。第一种矩阵 attn_q（2048 × 2048，2.4 MB，在高速缓存之内）上的加速比不低于 4 倍即通过：仍然每次处理一个权重的实现在 -mavx2 下也只有 1.6 倍，每步处理 32 个权重的 256 位实现有 6 到 7 倍。页面上的输出是参考答案在 i9-11900H 上的结果（2026-10-18），四种矩阵的加速比 6.1 到 6.5 倍。
尚未实现时，第一组全部 pass 之后，程序在第二组第一次调用 matvec_q4_0 时输出 matvec_q4_0 is not implemented yet 并以状态 3 退出，make 报告 Error 3。最后一行是 all passed 时第一部分完成。
""")


# ==============================================================================
# 其余部分
# ==============================================================================

def pd_rest_grid(p):
    p.title("cuda 版本的做法：一个线程块计算矩阵的一行")
    p.code("cuda", KERNEL_LAUNCH)
    p.spacer(72)                       # the figure mid-way between code and page foot
    figure(p, "pd-rest-grid")
    p.notes("""
第二部分写 ops/cuda.cu 中 kernel matvec_q4_0_kernel 的函数体，它初始时是一个 todo() 调用。启动它的主机端函数 matvec_q4_0 已写好：以 rows 个线程块、每块 THREADS = 256 个线程启动，参数是 y、w、x 与 cols；主机端函数与启动配置不修改。
图中：第 blockIdx.x 个线程块计算 y[blockIdx.x]，即矩阵的这一行与 x 的内积；这一行从 w + row × (cols / 32 × 18) 开始，是 cols / 32 个 Q4_0 块，排布与第一部分相同。rows 个线程块各算一行，全部行由它们分担；一个块的输出只有 y[row] 一个 float。一个线程块的 256 个线程分为 8 个 Warp，每个 Warp 32 个线程同时执行同一条指令，与第三讲「GPU 架构」一页相同；线程在线程块内的编号是 threadIdx.x。
与第一部分的对照：第一部分用一个循环依次处理每一行，一行之内 8 路累加器、最后由 hsum8 相加；这里 rows 个线程块各算一行，一行之内 256 个线程各自累加、最后在共享内存中折半相加。
mini-ollama-cuda 在生成第一个 Token 之前用算子表的 upload 把全部权重（974 MB）拷入显存，用 alloc 在显存中分配键值缓存与各个向量；此后每一项计算的输入与输出都在显存中，kernel 只读写显存，每生成一个 Token，CPU 只从 GPU 取回 argmax 得到的一个整数。
""")


def pd_rest_threads(p):
    p.title("线程块内的分工：线程 t 累加第 t、t + 256、… 列")
    figure(p, "pd-rest-threads")
    p.slide("""
- **每个线程在寄存器中累加 8 个乘积**：第 c 列的权重在第 c / 32 个 Q4_0 块的第 2 + c % 16 个字节，权重 = `q4_0_scale(块) · (编码 − 8)`。
""", autobold=False)
    p.notes("""
这一页有两种「块」：线程块是 256 个线程，计算矩阵的一行；Q4_0 块是 32 个权重，一行 2048 列是 64 个 Q4_0 块。一个线程块的 256 个线程分为 8 个 Warp，每个 Warp 32 个线程，一轮处理 256 列，即 8 个 Q4_0 块，每个 Warp 读其中一个，每个线程读一列。
docs/cuda.md 规定的分工：线程 t 在寄存器中累加第 t、t + 256、t + 512、… 列的乘积 W[row][c] · x[c]。图中把一行的 2048 列分为 8 轮，每轮 256 列，即 8 个 Q4_0 块；第一轮展开：线程 t 读第 t 列，第 c 列在第 c / 32 个 Q4_0 块中，它的编码在这个 Q4_0 块的第 2 + c % 16 个字节，c % 32 小于 16 时是低 4 位，否则是高 4 位，权重是 q4_0_scale(块) × (编码 − 8)。q4_0_scale 在 GPU 上也可以调用；embed_kernel 中从 Q4_0 块中取出一个编码的写法，这里也要用到。
第二轮同样的 256 个线程处理列 256 到 511，依此类推，8 轮之后每个线程累加了 8 个乘积；cols = 6144 时是 24 轮。
同一个 Warp 的 32 个线程处理相邻的 32 列，即同一个 Q4_0 块：它们读的是相邻的字节与相邻的 float，这是显存访问合并的条件。
每个线程的部分和在自己的寄存器中；256 个部分和如何相加为 y[row]，由共享内存中的折半求和完成。
""")


def pd_rest_fold(p):
    p.title("块内折半求和：256 个部分和 8 步相加为 1 个")
    p.spacer(96)                       # the figure alone sits mid-page
    figure(p, "pd-rest-fold")
    p.notes("""
每个线程把自己的部分和写入共享数组 part 的第 t 个元素，__syncthreads() 之后开始折半：第一步线程 t < 128 把 part[t + 128] 加到 part[t] 上，__syncthreads()；第二步线程 t < 64 把 part[t + 64] 加到 part[t] 上；每一步前一半的线程把后一半的元素加到自己的元素上，参与相加的元素个数依次是 256、128、…、2，共 8 步，之后 part[0] 是 256 个部分和之和，线程 0 把它写入 y[row]。
__syncthreads() 使线程块内的每个线程在这一行等待，直到线程块内全部线程都到达这一行：写入 part 之后的一次保证 256 个部分和都已存入，每一步之后的一次保证这一步的加法都已完成，下一步读到的是新值。
与第三讲 dot_cuda.cu 中 dot_kernel 的折半相加相同；cuda.cu 中已给出的 rmsnorm_kernel 先让每个线程累加每第 256 个元素的平方、再在共享内存中折半求和，正是这个 kernel 要用的写法，softmax_kernel 用同样的写法折半两次。
与第一部分的对照：第一部分的 hsum8 把 8 路累加器相加为一个数，这里把 256 个部分和相加为一个数，两者都是每一步两两相加，使数的个数减半。
""")


# ==============================================================================
# 第五部分 · mini-cpu 实验一
# ==============================================================================

# ==============================================================================
# 1 实验框架
# ==============================================================================

MAIN_LOOP = """x86 x;
while (!c->halted && !(opt_max && c->icount >= opt_max)) {
    unsigned op = x86_begin(c, &x);
    switch (op) {
    case 0x89:                  /* mov: rm = reg */
        modrm(&x);
        rm_set(&x, get(&x, x.reg));
        break;
    ...
    default:
        x86_other(&x);          /* the frame's cases */
        break;
    }
    if (x86_end(&x) < 0)
        break;
}"""

# pseudocode in the lecture's language: translated, see p.code(..., translate=True)
INSN_READ = """b = 下一个字节
while (b 是传统前缀) { 记下它的作用; b = 下一个字节 }
if (b 是 REX 前缀 40 .. 4f) { rex = b; b = 下一个字节 }
if (b 是 VEX 前缀 c4 或 c5) { vex = b 与其后 2 或 1 个字节; op = 下一个字节 }
else if (b == 0f) op = 0f 与其后 1 或 2 个字节
else op = b
if (op 带 ModRM) {
    modrm = 下一个字节
    if (modrm 之后有 SIB) sib = 下一个字节
    disp = 其后 0、1 或 4 个字节，个数由 modrm 与 sib 决定
}
imm = 其后 0、1、2、4 或 8 个字节，个数由 op 与操作数宽度决定"""


def cpu_libc(p):
    p.title("printf, libc 和 syscall")
    p.spacer(56)                       # figure and text mid-page
    figure(p, "cpu_libc")
    p.spacer(16)
    slide(p, """
- **`printf` 是 C 标准库 libc 中的函数**：它与程序自己的函数一样由普通指令组成，按格式把参数转为字节，再调用 libc 的 `write`。
- **进入操作系统的只有 `syscall` 指令**：`write` 把编号与参数放入寄存器，执行 `syscall`，由操作系统完成输出。guest 程序不链接 libc，没有 `printf`，输出由 `guest/mini.h` 的 `write` 直接执行 `syscall`。
""")
    p.notes("""
PC 上一个普通的 C 程序调用 printf 时，printf 的代码来自 C 标准库 libc（Linux 上通常是 glibc），链接时与程序放在一起，运行时与程序自己的函数一样是 CPU 执行的普通指令。printf 按格式串把 argc 转为字符 1，得到 "argc = 1\n" 这 9 个字节，放入 stdout 的缓冲区；需要输出时，它调用 libc 中的 write 函数。
向终端输出、结束程序这类操作，程序自己的指令不能完成，要请求操作系统完成。请求用的是 syscall 指令：write 函数把编号 1 放入 %rax，把文件描述符、缓冲区地址、字节数放入 %rdi、%rsi、%rdx，执行 syscall，由 Linux 把这 9 个字节输出到终端。系统调用的机制在讲异常时说明。
printf 不一定每次都调用 write：stdout 是终端时按行输出，遇到换行调用一次 write；重定向到文件时，缓冲区满或程序结束时才调用。在本机上用 strace -e trace=write 运行一个程序，可以看到它执行的每一次 write。
guest/ 中的程序不链接 libc，printf、malloc 这些库函数都不能使用。guest/mini.h 自己写出了用到的几个函数：puts 求出字符串的长度，putint 把整数转为十进制数字，write 用内联汇编直接执行 syscall。图中两条路径都在 PC 上运行，两处 syscall 由同一个操作系统 Linux 执行；guest 程序在 mini-cpu 上运行时，syscall 由 frame/ 中的代码代为完成。
""")


def cpu_hello_syscall(p):
    p.title("syscall 指令：mini-cpu 代为执行")
    p.spacer(64)
    figure(p, "cpu_hello_syscall")
    slide(p, """
guest/hello 的 `syscall` 只被 mini-cpu 读取，PC 的 CPU 不执行它。mini-cpu 读到它之后调用 libc 的 `write`，PC 的 CPU 执行 `write` 中的 `syscall`：mini-cpu 代为执行了这次系统调用。
""")
    p.notes("""
guest/hello 是 guest/ 中最小的客户机程序，输出 hello from the guest 与 argc = 1 两行。图左是它输出第一行的几条指令：%eax 置 1，即 write 的编号；%esi 置字符串的地址 0x402000；%rdi 置 1，即标准输出；%rdx 此前已经是字节数 21。然后在 40112a 执行 syscall，这条指令的字节是 0f 05。
guest/hello 在 mini-cpu 上运行时，PC 的 CPU 执行的是 mini-cpu 的机器码；guest/hello 的机器码是 mini-cpu 读取的数据，PC 的 CPU 不执行它。mini-cpu 逐条读取客户机的指令，按指令的规定修改客户机的寄存器与内存。读到 0f 05 时，它从客户机的 %rax 得知这是一次 write，把客户机内存中 0x402000 处的 21 个字节复制到自己的数组中。
然后 mini-cpu 调用 libc 的 write：mini-cpu 是链接 libc 的 C 程序，write 最终执行 syscall，%rax = 1，字节同样是 0f 05。这一条由 PC 的 CPU 执行，进入 Linux，Linux 把 21 个字节输出到终端。返回值 21 写回客户机的 %rax，mini-cpu 接着读取 40112c 处的下一条指令。
这就是代为执行：guest/hello 的 syscall 只被读取，它要求的 write 由 mini-cpu 中的 syscall 完成。"argc = " 与 putint 输出的 "1\\n" 以同样的方式输出，共三次 write。guest/hello 最后以编号 60 执行 syscall，即 exit；mini-cpu 读到它时停止执行客户机程序，把客户机给出的退出码作为自己的退出码。
""")


def cpu_hello_mem(p):
    p.title("内存与寄存器：mini-cpu 在内存中模拟")
    figure(p, "cpu_hello_mem")
    slide(p, """
`frame/loader.c` 把 guest/hello 的指令与字符串写入 dram.img，在栈顶写入 argc 与 argv，设置 `R[RSP]` 与 `R[RIP]`。`interp_run` 从 `R[RIP]` 开始逐条执行。
""")
    p.notes("""
./mini-cpu guest/hello 启动的 mini-cpu 是本机上的一个普通进程，guest/hello 的内存与寄存器都是这个进程中的数据。
内存：frame/main.c 先打开当前目录下的文件 dram.img，把它的大小设为 64 MiB，用 mmap 映射到 mini-cpu 自己的内存中，作为客户机的内存。装载器 loader_exec（frame/loader.c）读 guest/hello 的程序头，把 .text 与 .rodata 两段复制进去；再准备 64 页的栈，在栈顶依次写入参数字符串 "guest/hello"、argv[0] 的地址与 argc。
guest/hello 的指令中出现的地址是 0x401000、0x402000、0x7fffffffefc0 这样的客户机地址。frame/ 以 4 KiB 的页为单位，为每一页在 dram.img 中分配一个位置，读写时把客户机地址换算为文件中的偏移：0x401000 一页在偏移 0x101000，0x402000 一页在 0x102000，栈顶 0x7fffffffefc0 在 0x142fc0。运行结束后，od -A x -c -j $((0x102000)) -N 32 dram.img 在文件中显示这两个字符串。按页对应的方式在讲虚拟内存时说明；解释器只使用客户机地址，换算由 load、store 这些辅助函数完成。
寄存器：frame/main.c 的 main 中有局部变量 mcpu cpu，客户机的寄存器就是它的成员 R[18]：R[RAX] 到 R[R15] 是 16 个通用寄存器，R[RFLAGS] 是标志寄存器，R[RIP] 是 rip。interp_run(c) 的参数 c 指向 cpu，interp.c 用宏把 R 定义为 c->R，所以 R[RSP] 就是客户机的 %rsp。装载器把 R[RSP] 置为栈顶 argc 所在的地址 0x7fffffffefc0，R[RIP] 置为入口 0x401000，标志寄存器置为 0x2（第 1 位恒为 1），其余为 0。
之后 interp_run 每次循环按 R[RIP] 从客户机内存取一条指令，执行它，修改 R[] 与客户机内存。guest/hello 的第一条指令在 0x401000，字节是 48 8b 3c 24，即 mov (%rsp),%rdi，把 argc 读入 R[RDI]。
""")


def cpu_loop(p):
    p.title("主循环：不断读取、解析、执行")
    p.side_image("assets/cpu_loop.svg", width="36%", alt="contain", side="left")
    p.code("c", MAIN_LOOP)
    slide(p, """
`x86_begin` 读到操作码为止，其后的字节由 `case` 读取，读多少由操作码与 ModRM 决定。
""")
    p.notes("""
一次循环执行一条指令。x86_begin 从 R[RIP] 取字节，跳过前缀，记下 REX 与操作数宽度，读出操作码并返回它的编号 op；此时 R[RIP] 指向操作码之后的第一个字节。
switch 按 op 进入一个 case：case 读操作码之后的字节，计算，写寄存器与内存。没有 case 的操作码进入 default，由框架的 x86_other 执行。syscall 的操作码 0f 05 就没有 case：x86_other 把它交给 frame/sysbase.c 的 sys_dispatch，sys_dispatch 按 R[RAX] 代为执行系统调用（write 调用本机 libc 的 write，exit 把 c->halted 置 1，x86_end 随后返回 -1），所以 interp.c 不需要为 syscall 写 case。
x86_end 计数并写提交日志；指令中途出错（缺页、非法指令）时它把寄存器恢复到指令开始前的值，返回 -1，循环结束并打印报告。
case 0x89 是 mov 的一种形式，rm = reg：先 modrm 解析操作数，再把 reg 寄存器的值写到 rm。
""")


def cpu_insn_format(p):
    p.title("x86-64 指令的组成与读取顺序")
    figure(p, "cpu_insn_format")
    p.code("pseudo", INSN_READ, translate=True)
    p.notes("""
一条 x86-64 指令最多由图中的七段组成，按从左到右的顺序排列，最长 15 字节；除操作码外，每一段都可以没有。图中各段的宽度不按比例。
66、f2、f3、f0、67 与段前缀称为传统前缀，分四类，一条指令中每一类最多用一个，因此前缀最多 4 个字节：66 把操作数宽度改为 2 字节；f2 与 f3 是重复前缀，也用来区分一部分向量指令；f0 是 lock 前缀；67 改变地址的宽度；2e、3e、26、36、64、65 是段前缀。REX 前缀是 40 到 4f 中的一个字节，紧挨在操作码之前：它的 W 位把操作数宽度改为 8 字节，另外三位给寄存器编号加第 4 位，在下一小节讲。
操作码有 1 到 3 个字节：一个字节；0f 开头的两个字节；0f 38、0f 3a 开头的三个字节。AVX 指令以 VEX 前缀开头：c4 之后的 2 个字节带有 REX 的各位、操作码所在的表（0f、0f 38 或 0f 3a）与 66、f2、f3 的选择，c5 之后的 1 个字节是它的简写，表固定为 0f。VEX 指令因此没有 REX 与 0f，VEX 之后是一个字节的操作码。
伪代码按读取的顺序写出，每一段有没有、有几个字节，由它之前的字节决定：操作码决定有没有 ModRM、有没有立即数；ModRM 决定有没有 SIB、偏移有几个字节，ModRM 与 SIB 的各个字段在下一小节讲。立即数的字节数由操作码决定，部分操作码再看操作数宽度：操作码 81 的立即数在有 66 前缀时是 2 字节，否则是 4 字节，有 REX.W 时仍是 4 字节，读出后符号扩展为 8 字节。8 字节的立即数只出现在两种 mov 中：REX.W 与 b8 到 bf 的组合把一个 8 字节常数写入寄存器，a0 到 a3 以一个 8 字节的地址读写内存；objdump 把两者都写作 movabs。
图下方的括号是 mini-cpu 中的分工。x86_begin 读前缀、REX 或 VEX 与操作码，返回编号 op：它识别 66、f2、f3、六个段前缀、REX 与 VEX，读到 0f 时再读一个字节。其后的字节由 case 读取：modrm(&x) 调用你的 modrm_decode，读 ModRM、SIB 与偏移；立即数由 fetch_imm、fetch8 这些函数读取。
图中各段的颜色在后面每一页都表示同一段字节：灰色前缀（包括 REX 与 VEX）、橙色操作码、绿色 ModRM、紫色 SIB、青色偏移、蓝色立即数。
""")


def cpu_bytes(p):
    p.title("一条指令的字节与 op 的编号")
    p.spacer(48)
    figure(p, "cpu_bytes")
    p.notes("""
除第二条外，各条指令都取自客户机程序的 objdump 输出。
第一条有 REX 前缀 4c、操作码 03、ModRM 04、SIB d5 与 4 字节偏移；op 是 0x03。
第二条 add %rax,%r8 与第一条的 REX 前缀和操作码都相同，op 也是 0x03，只有 ModRM 字节不同。ModRM 04 表示另一个操作数 rm 在内存中，后面还有 SIB 与 4 字节偏移；ModRM c0 表示 rm 是寄存器 %rax，指令到此结束。两条指令都把和写入 %r8：第一条加的是内存中的 8 字节，第二条加的是 %rax。同一个操作码的指令，另一个操作数是寄存器还是内存、后面还有几个字节，由 ModRM 决定；ModRM 的各个字段在下一小节讲。guest/sum 中有同一条指令 add %rax,%r8，汇编器把它编码为 49 01 c0，用的是操作码 01；这里的 4c 03 c0 是用 {load} add %rax,%r8 让汇编器改用操作码 03 得到的，以便与第一条比较。
第三条的操作码 81 之后是 ModRM ec 与 4 字节立即数；op 是 0x81。
两字节操作码 0f 88 的编号是 0x100 加第二个字节，即 0x188；之后是 4 字节偏移。
VEX 指令的编号是 0x1000 | map << 8 | 操作码字节：c4 e2 7d 是三字节的 VEX 前缀，map 2 即 0f 38，操作码 40，编号 0x1240。
下半部分是第一条 add 在 interp_run 的一次循环中依次调用的函数。x86_begin 读前缀 4c 与操作码 03，记下 x.rex = 0x4c 与操作数宽度 x.size = 8，返回 op = 0x03；switch 进入 case 0x03。操作码 03 是 add 的一种形式，reg = reg + rm：reg 是寄存器，rm 是寄存器或内存，和写入 reg；这一条的 reg 是 %r8，rm 是内存，即把内存中的 8 字节加到 %r8 上。modrm 调用你的 modrm_decode，读 ModRM、SIB 与 4 字节偏移，得出 x.reg = 8 即 %r8，另一个操作数在内存中，地址是 0x403020 + %rdx × 8；这几个字节怎样解析在下一小节讲。get 读 %r8，rm_get 从这个地址读 8 字节；x86_alu 的运算编号是 0x03 >> 3 = 0，即 add，它返回两数之和并设置标志位；set 把和写入 %r8。第二条 add %rax,%r8 经过同样的调用，只是 modrm 得出 x.rm = 0，rm_get 读的是 %rax。最后 x86_end 返回 0，此时 R[RIP] 已经越过偏移，指向下一条指令。
""")


# ==============================================================================
# 2 课上完成：ModRM 与 SIB 的解析
# ==============================================================================

def cpu_modrm_byte(p):
    p.title("ModRM 字节：mod、reg、rm")
    figure(p, "cpu_modrm")
    slide(p, """
操作码之后的第一个字节。**mod** 说明另一个操作数是寄存器还是内存，**reg** 是寄存器编号或扩展的操作码，**rm** 是寄存器编号或基址寄存器的编号。
""")
    p.notes("""
ModRM 分三个字段：高 2 位 mod，中间 3 位 reg，低 3 位 rm。
mod 为 11 时 rm 是寄存器；00、01、10 时另一个操作数在内存中，mod 同时决定偏移的宽度：0、1、4 字节。
reg 是寄存器操作数的编号；0x81、0x83 这类组指令没有寄存器操作数，reg 字段扩展操作码，modrm_t 中叫 sub。
rm 在内存操作数的情况下是基址寄存器的编号，有两个例外：rm 100 表示随后有 SIB 字节，rm 101 且 mod 00 表示没有基址，随后 4 字节偏移，地址相对于下一条指令。
例子是 hello 的第一条指令：3c 是 00 111 100，内存操作数，没有偏移，reg 是 %rdi，rm 100 读 SIB 字节 24。
操作码 8b 规定 reg 是目标，所以 %rdi = 内存中的 8 字节。
""")


def cpu_sib_byte(p):
    p.title("SIB 字节：scale、index、base")
    figure(p, "cpu_sib")
    slide(p, """
rm 为 100 时 ModRM 之后是 SIB 字节：地址 = 偏移 + base + index × scale。**index** 得到 4 表示没有变址；**base** 字段为 101 且 mod 为 00 表示没有基址，随后 4 字节偏移。
""")
    p.notes("""
SIB 的三个字段与 ModRM 同样分 2、3、3 位：scale 两位是比例因子的对数，index 与 base 是寄存器编号。
index 加上 REX.X 之后等于 4 表示没有变址寄存器，因为 %rsp 不能作变址；此时 scale 记为 1。
base 字段 101 且 mod 00 表示没有基址，随后 4 字节偏移，matmul 的这条指令就是这种形式：
地址 = 0x403020 + %rdx × 8。
hello 的 (%rsp) 是另一种常见形式：%rsp 作基址必须经过 SIB，SIB 24 是 00 100 100，没有变址，base 是 %rsp。
""")


def cpu_rex(p):
    p.title("REX 前缀给三个字段加第 4 位")
    figure(p, "cpu_rex")
    slide(p, """
`x86_begin` 把 REX 记在 `x.rex` 中，`modrm_decode` 的参数 `rex` 就是它。两个例外只看 3 位的字段，判断在加 REX.B 之前。
""")
    p.notes("""
REX 前缀是 0100 W R X B：W 为 1 时操作数 64 位；R、X、B 各给一个字段加第 4 位，R 给 reg，X 给 index，B 给 rm（mod 11 时）或 base。rex 的位 2、1、0 依次是 R、X、B。
两个例外只看 3 位的字段：rm 100 加上 REX.B 是 %r12，它作基址仍然经过 SIB；rm 101 且 mod 00 加上 REX.B 仍然是 %rip 相对。index 相反，在加 REX.X 之后判断是否为 4，%r12 可以作变址。
block 的 lea -0x1(%r12),%eax 是 %r12 作基址的例子：REX 41 的 B 为 1，ModRM 44 的 rm 100 读 SIB 24，base 100 加 8 得 12。
""")


def cpu_modrm_flow(p):
    p.title("modrm_decode 的判断顺序")
    slide(p, """
`unsigned modrm_decode(const uint8_t *p, unsigned n, uint8_t rex, modrm_t *m)`：`p` 指向 ModRM 字节，`n` 是可读的字节数，`rex` 是前缀（没有时为 0），结果填入 `*m`。
""")
    p.frames("assets/cpu_flow-1.svg", "assets/cpu_flow-2.svg", "assets/cpu_flow-3.svg", width_px=1120)
    p.notes("""
第一帧：读 ModRM 字节，拆出 mod、sub 与 reg，reg 加 REX.R。mod 为 11 时 rm 是寄存器，其余字段取没有的值，返回 1。
第二帧：内存操作数。rm 字段为 100 时读 SIB 字节，拆出 scale、index 与 base，index 加 REX.X 后等于 4 表示没有变址，base 字段 101 且 mod 00 表示没有基址。
rm 字段不是 100 时，rm 101 且 mod 00 是 %rip 相对，其余 base = rm 字段加 REX.B。
第三帧：偏移的宽度由 mod 决定，mod 00 没有偏移，没有基址的两种情况例外，4 字节；偏移按小端读出并符号扩展到 64 位。返回用掉的字节数，1 到 6；读每个字节之前检查它在 n 个字节之内，否则返回 0。
课上照这张图写 modrm_decode；函数体写在 modrm.c 中，开头注释的例子表就是检查用例的一部分。
""")


def cpu_modrm_out(p):
    p.title("modrm_t 的字段与解释器对它的使用")
    figure(p, "cpu_modrm_t")
    slide(p, """
`modrm_decode` 只读字节、填字段，不访问寄存器；`frame/x86.c` 的 `modrm()` 由这些字段算出 `x.addr`，`case` 中用 `get`、`rm_get` 与 `ea` 取操作数即可。
""")
    p.notes("""
modrm_t 有十个字段，全部由 modrm_decode 填写：mod、sub、reg、rm、base、index、scale、rip、dsize、disp。
没有的寄存器记 -1，没有变址时 scale 记 1，这样 modrm() 的地址公式对每种形式都成立：
x.addr = disp + R[base] + R[index] * scale，没有的项不加。
rip 为 1 时 ea 再把 R[RIP] 加上去，此时 R[RIP] 已经指向下一条指令，所以立即数要在 ea 之前读完。
同一个 modrm_decode 还被反汇编器 frame/translate.c 调用（./mini-cpu --disasm），反汇编器只给它指令的字节，因此函数里不能读机器状态。
""")


def cpu_examples_1(p):
    p.title("三条指令的解析结果：hello 的 mov、lea 与 REX.B")
    figure(p, "cpu_examples")
    slide(p, """
尝试写出 `modrm_t` 的每个字段与返回值。
""")
    p.notes("""
第一条 mov (%rsp),%rdi：ModRM 3c 是 mod 0、reg 7、rm 100，读 SIB 24，index 100 没有变址，base 4，用掉 2 字节。
第二条 lea 0x8(%rsp),%rsi：ModRM 74 的 mod 是 01，1 字节偏移 08；其余与第一条相同，用掉 3 字节。
第三条 mov %rdi,%r9：REX 49 的 B 为 1，ModRM f9 的 mod 11，rm 是寄存器 001 加 8 得 9，用掉 1 字节。
三条的 reg 都不加 REX.R，因为 REX.R 为 0。
""")


# ==============================================================================
# 3 其余部分
# ==============================================================================

def cpu_alu_flags(p):
    p.title("alu.c：十个函数与五个标志位")
    p.spacer(64)
    figure(p, "cpu_flags")
    p.spacer(16)
    slide(p, """
每个函数返回 `size` 字节的结果，只改 `*f` 的这五位。`x86_add` 是写出的例子，`x86_sub` 与 `x86_logic` 照它写，`inc`、`dec`、`neg` 可以由它们组合。
""")
    p.notes("""
alu.c 的函数只做算术与标志位，不读指令、不访问内存，所以 mini-unit 可以单独检查它们。
x86_szp 由结果设置 ZF、SF、PF，其余函数都调用它：ZF 是低 size 字节为 0，SF 是结果的最高位，PF 是低 8 位中 1 的个数为偶数。
x86_add 已经写出，是 CF 与 OF 的样板；x86_sub 的 CF 是借位，OF 是 a 与 b 异号且结果与 a 异号；x86_logic 清零 CF 与 OF。
x86_inc 已经写出：调用 x86_add 但保留 CF；x86_dec 与 x86_neg 同样由 x86_sub 得到。
x86_shift 中 shl 已经写出，shr 与 sar 要写；x86_mul 要写，x86_div 已经写出。
调用者有两个：x86_alu 把运算编号 0 到 7 分派到前四个函数；其余六个由框架的 x86_other 调用，它取出 inc、dec、neg、移位、乘除法的操作数并写回结果。
""")


def cpu_shift(p):
    p.title("移位、乘法与除法")
    figure(p, "cpu_shift")
    slide(p, """
移位的 **CF** 是最后移出的那一位；`shr` 与 `sar` 的差别是高位补 0 还是补符号位。`x86_mul` 把 2 × `size` 字节的乘积分成 `*hi` 与 `*lo`，乘积超出 `size` 字节时 CF 与 OF 为 1。
""")
    p.notes("""
x86_shift 的 sub 是 ModRM 的 reg 字段：4 是 shl，5 是 shr，6 是 sal 同 shl，7 是 sar。
位数先取低 5 位，size 为 8 时取低 6 位；位数为 0 时标志位不变。
三行都移动 1 位：shl 0x81 得 0x02，移出的最高位 1 进 CF，OF 是结果最高位与 CF 不同；shr 0x81 得 0x40，移出的最低位 1 进 CF，OF 是 v 的最高位；sar 0x81 得 0xc0，高位补符号位，OF 为 0。
x86_mul 的 mul 无符号、imul 有符号，乘积 2 × size 字节分成 hi 与 lo，size 为 8 时用 __int128；CF 与 OF 都表示乘积超出 size 字节。x86_div 已经写出，除数为 0 或商放不下时返回 -1，框架报告 divide error。
""")


def cpu_insn_groups(p):
    p.title("interp.c 的整数指令：按操作码之后的字节分五组")
    figure(p, "cpu_groups")
    slide(p, """
`interp.c` 开头的注释按这五组列出每个操作码的语义，括号中是第一个用到它的客户机程序。
""")
    p.notes("""
只有操作码：push、pop、ret。push 已经写出，pop 与 ret 照它写，寄存器编号在操作码的低 3 位加 REX.B。
操作数字节：mov 的两种形式、lea、八种运算的两种形式、test。rm = rm OP reg 的八个操作码01、09、11、19、21、29、31、39 的运算编号是 op >> 3；reg = reg OP rm 的八个是 03、0b 到 3b。
操作数字节加立即数：c7 是 mov，81 与 83 已经写出，运算编号在 x.sub。
立即数：b8 到 bf 是 mov 到寄存器，x.size 为 8 时立即数 8 字节。
跳转的偏移：call e8、jmp eb 与 e9、jcc 70 到 7f 与 180 到 18f；偏移读完后 R[RIP] 是下一条指令，加上 rel。
每组写好一条，同组的其余指令只换运算或宽度。
""")


def cpu_cond_jump(p):
    p.title("cond 与条件跳转")
    figure(p, "cpu_cond")
    slide(p, """
`cond(f, cc)` 读 `f` 的五个标志位，按条件码 `cc` 的低 4 位返回 1 或 0；16 个条件码的表在 `interp.c` 的注释中。`jcc` 的 `case` 先读偏移，`cond` 为 1 时 `R[RIP] += rel`。
""")
    p.notes("""
条件码是操作码的低 4 位，jcc 的 case 直接把 op 传给 cond。
e 是 ZF，ne 是 !ZF，l 是 SF != OF，ge 是 SF == OF；其余 12 个在 interp.c 开头的注释与 mcpu.h 中。
例子是 sum 的 jge：操作码 7d 的条件码 d 是 ge，偏移 f4 是 -12。
偏移读完后 R[RIP] 是 40112c，加 -12 得 401120，循环的开头。
框架的 x86_other 处理 cmovcc 与 setcc 时也调用 cond，所以评测的 conditions 一项同时检查三种指令。
""")


def cpu_vector(p):
    p.title("向量指令：VEX 前缀与 Y[] 的通道")
    figure(p, "cpu_vector")
    slide(p, """
`x86_begin` 从 VEX 前缀读出第三个操作数 `x.v` 与宽度 `x.vlen`；`case` 逐通道算到局部的 `u256`，最后一步 `vset` 写目标寄存器。
""")
    p.notes("""
五条要写的向量指令是 vpxor、vmovdqu、vpaddd、vpmulld、vmovd，vphaddd 与 vextracti128 已经写出。
VEX 前缀多出一个寄存器操作数 vvvv，x86_begin 记在 x.v 中；L 位决定 ymm 还是 xmm，x.vlen 是 32 或 16。
整数通道是 32 位：u256 的 d[i]，vlen / 4 个通道。
vpaddd 的 case：modrm 解析 rm，rm_vget 读第二个源，与 Y[x.v] 逐通道相加到局部的 r，vset 写 x.reg。
目标寄存器同时是源，所以先算到局部变量，最后一步才写出；写 xmm 时 vset 清零高 128 位。
vmovdqu 的 rm 可以是内存；vmovd 的目标是 rm，用 rm_set 写 4 字节。
""")
