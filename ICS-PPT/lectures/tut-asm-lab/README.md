# ICS 辅导课 · 汇编实验准备

本讲为实验 `solve-bomb` 做准备，有两部分。

- **第 1–13 张**：操作系统、Linux、虚拟机虚拟化与容器虚拟化，结论是 ICS 课程用 docker 在 Mac 或 Windows 上运行
  ubuntu 容器；随后是 docker 的下载地址、拉取 x86-64 的 ubuntu 镜像的命令、运行容器的命令、
  在后台运行容器并用 `docker start`、`docker exec` 进入它的命令，以及用 `docker cp` 把文件复制进容器的命令。
- **第 14–35 张**：`objdump` 的用法；用 `objdump` 阅读实验的课堂程序 `bomb-light`，推出它接受的输入；
  `gdb` 的常用命令与两次调试 `bomb-light` 的过程；静态链接、动态链接与位置无关的可执行文件；实验 `solve-bomb` 的任务。

讲义 id 是 `tut-asm-lab`，与目录名相同（2026-10-11 由 `tut-linux-docker` 改名）。封面与讲义的标题是「汇编实验准备」
（2026-10-11 由 `Docker Intro` 改名），PDF 与 PPTX 的文件名由标题得到，是 `汇编实验准备.pdf`、`汇编实验准备.pptx`。
实验 `solve-bomb` 与 `attack-elf` 的学生说明用这个标题指称本讲（`solve-bomb/handout/README.md`、`attack-elf/dist/HANDOUT.md`）。

## 来源

| 幻灯片 | 来源 |
| --- | --- |
| 1–7 | 由 [`../ICS1/ICS-tutorial-1-env/`](../ICS1/ICS-tutorial-1-env/) 复制后裁剪，保留 LINUX 一节，即原课件 `refs/ppts/ICS1/ICS-tutorial-1-env.pdf` 的第 8–13 页。封面与课程其他各讲（如 `3-asm`）的写法相同，只有标题不同 |
| 8–13 | 不在原课件中：第 9–12 张是 2026-10-09 新写的，第 13 张（`docker cp`）是 2026-10-11 加的 |
| 14–35 | 2026-10-11 按原课件 `ICS-tutorial-5-asm.pptx`（37 页）改写。`refs/ppts/ICS1/` 中有这份课件，改写时用的一份与它只在第 37 页有一行不同；逐页转换的版本在 [`../ICS1/ICS-tutorial-5-asm/`](../ICS1/ICS-tutorial-5-asm/) |

第 14–35 张与原课件 `ICS-tutorial-5-asm.pptx` 各页的对应：

| 原课件的页 | 原标题 | 本讲的页 |
| --- | --- | --- |
| 1、2 | 封面、版权声明 | 没有保留 |
| 3–4 | GNU Tools（objdump） | `objdump`、`objdump-output` |
| 5 | Bomb | `bridge-bomb` |
| 6 | Defuse Bomb | `bomb-what`；原页的图片 `s006.png` 没有保留 |
| 7–8 | Demo | `bomb-what` 与 `bomb-defused` 的两个演示 |
| 9–10 | Step by Step：From machine code to assembly code | `bomb-objdump` |
| 11–13 | Step by Step：Find key functions and parameters | `bomb-main`、`bomb-calls` |
| 14–21 | Step by Step：Find key strings；I/O（`printf`） | `bomb-args`、`bomb-strings` |
| 22–25 | Step by Step：Find key operators | `bomb-branch` |
| 26–29 | I/O（`scanf`） | `bomb-args`、`bomb-defused` |
| 30 | Using GDB | `bridge-gdb` |
| 31 | Can we do it more efficiently and safely? | `gdb` 的右栏 |
| 32 | GNU Tools（GDB） | `gdb` 的左栏与安装命令 |
| 33 | GDB (Gnu DeBugger) Usage | `gdb-commands` |
| 34 | A live demo | `gdb-break`、`gdb-args`、`gdb-step`、`gdb-stop` |
| 35 | Dynamic Link vs. Static Link | `link-static` |
| 36、37 | Tasks | `bridge-tasks`、`lab-solve-bomb` |

改写时改动的地方：

- **例子换成 `bomb-light`**。原课件演示的是一个读入密码、输出 `survive!` 或 `bomb!` 的小程序，代码与终端输出都是截图。
  本讲拆除的是实验 `solve-bomb` 的课堂程序 `bomb-light`（`../solve-bomb/src/phase_light.c`、`main_light.c`）：
  `main` 调用 `read_line`、`phase_light`、`phase_defused`，`phase_light` 用 `sscanf` 读一个整数并与 20261013 比较。
  步骤的顺序与名称照原课件，各步骤的内容按 `bomb-light` 写。
- **截图改为 `p.code` 与 `p.demo`**，清单与输出取自 `examples/bomb-light`。
- **查看字符串用 `objdump -s`**。原课件从 `objdump -D` 的输出中读 `.rodata` 的字节，`-D` 把这些字节按指令译出。
- **「A live demo」一页分为两次 `gdb` 会话、四页**，每个演示按钮启动一次 `gdb`。`bomb-light` 是位置无关的程序，
  「运行时的地址 = 装入位置 + 文件中的地址」在第一页演示中说明，断点写成 `b phase_light`、`b *phase_light+53`、`b explode_bomb`。
- **「Dynamic Link vs. Static Link」一页**：原页的提示是演示用的炸弹静态链接、练习用的炸弹动态链接；
  本课程的 `bomb-light` 与实验的 `bomb` 都是静态链接的位置无关程序，动态链接的例子用 `/bin/bash`。
- **任务页是实验 `solve-bomb` 的一页**：不计分，材料在 GitHub 仓库 `SJTU-IPADS/OS-Course-Lab` 的目录 `ICS-Labs/BombLab` 中，是 `Makefile`、`bomb-light`、`README.md` 三个文件。
  原页写的是在 git 分支中取得任务；原页的飞书文档链接（Mac ARM & Docker x86 GDB）已经不用，没有保留。

## 两种语言

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。第 2–7 张的原文是英文，
`en.toml` 里逐字保留原文，Python 里是中文译文；封面的作者与单位是中文原文，`en.toml` 里是英文译文。
第 8–35 张的英文是中文的译文；分节页的标题、拆除炸弹的四个步骤、GDB 的两个清单在原课件中有英文原文，`en.toml` 里用原文。
代码块与演示的命令、输出不翻译，其中的注释是英文。

## 结构

| 幻灯片 | id | 标题 | 内容 |
| --- | --- | --- | --- |
| 1 | `cover` | 汇编实验准备 | 封面：标题、作者（古金宇 · 廖承凡）、单位（上海交通大学 IPADS） |
| 2 | `s08` | LINUX | 分节页 |
| 3 | `s09` | 操作系统 | 操作系统位于应用程序与硬件之间，为应用程序提供服务并管理硬件 |
| 4 | `s10` | Linux | Linux 发行版的标志 |
| 5 | `s11` | 虚拟机虚拟化 | 虚拟机、虚拟机监控器，虚拟机的层次图 |
| 6 | `s12` | 容器虚拟化 | 虚拟机与容器的层次对照图 |
| 7 | `s13` | 容器虚拟化 | docker 的客户端、宿主机与镜像仓库 |
| 8 | `bridge-docker` | 安装 docker，运行 ubuntu 容器 | 分节页 |
| 9 | `docker-install` | 安装 docker：下载地址 | 三行的表（macOS、Windows、Linux 各自安装的软件与 Docker 官方文档的安装页）；三条说明；演示 `docker --version` |
| 10 | `docker-pull` | 拉取镜像：x86-64 的 ubuntu 26.04 | 镜像、`--platform linux/amd64`、Apple 芯片的 Mac 三条说明；演示 `docker pull --platform linux/amd64 ubuntu:26.04` 与查看镜像架构的命令 |
| 11 | `docker-run` | 运行容器：在 ubuntu 容器中启动 bash | 交互式演示 `docker run -it --rm --platform linux/amd64 ubuntu:26.04 bash`；五条说明（`docker run`、`-it`、`--rm`、提示符、`exit`） |
| 12 | `docker-exec` | 后台运行容器：docker start 与 docker exec | 演示 `docker run -dt --name ics --platform linux/amd64 ubuntu:26.04 bash`；交互式演示 `docker start ics` 与 `docker exec -it ics bash`；四条说明（`-dt`、`--name ics`、`docker start`、`docker exec`） |

| 13 | `docker-cp` | 在宿主机与容器之间复制文件：docker cp | 演示 `docker cp examples ics:/root/` 与 `docker exec ics ls /root/examples`；四条说明（命令的两个参数、容器中的路径、从容器复制到宿主机、`examples` 与实验材料） |
| 14 | `bridge-objdump` | 使用 objdump | 分节页 |
| 15 | `objdump` | objdump：显示目标文件与可执行文件中的信息 | 六行的表（`-d`、`-D`、`-d --disassemble=main`、`-s`、`-f`、`-t` 各显示什么）；节的定义 |
| 16 | `objdump-output` | objdump -d 的输出：地址、机器码、汇编指令 | 演示 `objdump -d bomb-light` 中 `main` 开头的 6 行；四条说明（函数的起始地址、每行的三栏、文件中的地址、`sed` 的范围） |
| 17 | `bridge-bomb` | 炸弹 | 分节页 |
| 18 | `bomb-what` | 拆除炸弹 | 炸弹是什么、要做什么、可以用什么；交互式演示 `BOMB_UI=plain ./bomb-light`，输入 224，输出 `phase 1 of 1 exploded` |
| 19 | `bomb-objdump` | 从机器码到汇编代码：objdump -d bomb-light > bomb-light.asm | 演示把反汇编保存到 `bomb-light.asm`（196119 行）并用 `grep -n` 找到 `main`（第 844 行） |
| 20 | `bomb-main` | 找出关键的函数：main 中的四条 call | `main` 的清单，开头 9 条与结尾 3 条指令写成 `...`，四条 `call` 有底色；`read_line` 的返回值传给 `phase_light`，`phase_light` 返回之后调用 `phase_defused` |
| 21 | `bomb-calls` | 找出关键的函数：phase_light 中的 sscanf 与 explode_bomb | 演示从 `phase_light` 中取出 4 条 `call`；`__isoc99_sscanf`、`explode_bomb`、`__stack_chk_fail` 各是什么；接下来要确定的两件事 |
| 22 | `bomb-args` | sscanf 的参数：读出的数据写入 -0xc(%rbp) | `0xaa92` 一条与 `0xaaa5` 到 `0xaabf` 的 7 条指令，写入三个参数寄存器的三行有底色；`sscanf` 的原型与三个参数，`0xc057c(%rip)` 的含义 |
| 23 | `bomb-strings` | 找出关键的字符串：0xcb02c 处的格式串是 %d | 演示 `objdump -s` 显示 `0xcb02c` 起的 3 个字节；`%d` 的含义，局部变量 `x`，`sscanf` 的返回值 |
| 24 | `bomb-branch` | 找出关键的指令：cmp 与 je 决定是否执行 call explode_bomb | `0xaabf` 到 `0xaadd` 的 9 条指令，两组 `cmp` 与 `je` 有底色；两处比较的条件；正确的输入 20261013 |
| 25 | `bomb-defused` | 拆除炸弹：输入 20261013 | 交互式演示 `BOMB_UI=plain ./bomb-light`，输入 20261013，输出 `phase 1 of 1 defused`；四个步骤各找到了什么 |
| 26 | `bridge-gdb` | 使用 GDB | 分节页 |
| 27 | `gdb` | GDB：运行程序，在指定的位置停下，查看寄存器与内存 | 两栏：GDB 的功能、用 GDB 拆除炸弹；在容器中安装 `gdb`、`binutils`、`file` 的两条命令 |
| 28 | `gdb-commands` | GDB 的常用命令 | 九行的表（命令、缩写、作用）；参考资料 `gdb-ref.txt` 的链接 |
| 29 | `gdb-break` | 现场演示：在 phase_light 设置断点，运行到断点处 | 交互式演示 `BOMB_UI=plain gdb -q bomb-light`：`b phase_light`、`r`、输入 224；断点的位置；运行时的地址 = 装入位置 + 文件中的地址 |
| 30 | `gdb-args` | 现场演示：在 call sscanf 之前停下，查看参数 | 接着上一页的会话：`x/s $rdi`、`b *phase_light+53`、`c`、`x/s $rsi`；三条说明 |
| 31 | `gdb-step` | 现场演示：单步执行，查看返回值与比较的两个数 | 接着上一页的会话：`ni`、`p $eax`、`x/dw $rbp-0xc`、三次 `ni`、`p $eax`、`p 0x1352895`；两条说明 |
| 32 | `gdb-stop` | 现场演示：在 explode_bomb 设置断点，在爆炸之前退出 | 交互式演示：`b explode_bomb`、`r`、输入 224、`quit`；三条说明 |
| 33 | `link-static` | 静态链接与动态链接 | 演示 `file bomb-light /bin/bash`；`gcc -static`、动态链接、`file`、`pie executable`、实验的 `bomb` 五条说明 |
| 34 | `bridge-tasks` | 任务 | 分节页 |
| 35 | `lab-solve-bomb` | 实验 solve-bomb：拆除你的学号对应的 bomb | 不计分；`bomb` 与服务器的连接；实验材料在 GitHub 上的位置；七条命令（取得仓库、`docker cp`、进入容器、安装工具、`make bomb`、运行）；完成标准 |

共 35 张幻灯片：1 个封面、6 个分节页、5 个原课件 `ICS-tutorial-1-env` 的编号页、5 个 docker 的页、18 个按 `ICS-tutorial-5-asm` 改写的页。
第 6、7 张标题相同，在大纲里合为一行。

## 目录

| 路径 | 内容 |
| --- | --- |
| `lecture.py` | 页序：封面、LINUX、「安装 docker，运行 ubuntu 容器」、「使用 objdump」、「炸弹」、「使用 GDB」、「任务」六节 |
| `pages.py` | 每页一个函数。原课件 `ICS-tutorial-1-env` 第 N 页是 `sNN`；其余各页的函数名是页面 id 把 `-` 换成 `_`（`docker_cp`、`bomb_main`、`gdb_break`、……） |
| `assets/` | 插图，文件名以所在页的 `sNNN` 开头 |
| `examples/bomb-light` | 第 13–33 张的演示使用的程序，见「例子程序」一节 |
| `i18n/en.toml` | 英文覆盖层 |

## 例子程序

`examples/` 中只有一个文件 `bomb-light`，是 [`../solve-bomb/bomb-light`](../solve-bomb/) 的副本（sha256 以 `b1f6a2a80052b653` 开头），
即实验 `solve-bomb` 发给学生的材料中的同一个文件。它是实验为课堂讲解准备的程序：只有一个阶段，所有人的相同，
不连接服务器，爆炸不被记录；正确的输入是 20261013。

页面上的地址（`main` 在 `0xaa49`，`phase_light` 在 `0xaa8a`，格式串在 `0xcb02c`）、行数（196119、844）与各个演示的输出都取自这个文件。
实验重新构建 `bomb-light` 之后，把新的文件复制过来，并重新记录：`pages.py` 中的 `MAIN` 与 `PHASE`、各页标题与正文中的地址、
`p.demo` 的 `output=`、三页 `gdb` 会话的代码块、讲稿中的地址。`phase_light` 的内容改变时，「炸弹」一节各页的正文要重写。

`bomb-objdump` 一页的演示在 `examples/` 中写出 `bomb-light.asm`（约 10 MB），演示之后可以删除。

## 插图

插图都取自原课件 `ICS-tutorial-1-env`，是位图，本讲没有 `diagrams/`。图里的文字是英文，两种语言共用一张图。第 8–35 张没有插图。

| 文件 | 页 | 来源 |
| --- | --- | --- |
| `s009.png` | `s09` | 裁自 PDF 渲染的原页，裁图时抹去了页码 |
| `s010.jpg` | `s10` | 原 PDF 内嵌的图片，按原页的裁剪范围裁去上部 |
| `s011-1.png` | `s11` | 裁自 PDF 渲染的原页 |
| `s011-2.png` | `s11` | vmware 图标，原 PDF 内嵌的图片与它的透明蒙版合成 |
| `s012.jpg` | `s12` | 原 PDF 内嵌的图片 |
| `s013.jpg` | `s13` | 原 PDF 内嵌的图片 |

原页没有标注这些图片的出处，页面上因此没有 `.footnote(...)`。`s010.jpg`、`s012.jpg`、`s013.jpg`
的授权尚未核对，记在 `lectures/PENDING.md`。

## 用语与排版约定

**封面**：`lecture.cover(标题, author="古金宇 · 廖承凡", time="上海交通大学 IPADS")`，不带 `id` 和 `logo`，与课程其他各讲一致。

**用语**：本课件用于本科课程教学，一律采用陈述性的技术表述。不使用比喻、口语化措辞，
以及「不是……而是……」「是 A，不是 B」一类对比句式；结论写成可以直接复述的判断句。

**保留的各页**沿用 [`../ICS1/README.md`](../ICS1/README.md)「转换约定」一节：

- **页面 id**：原课件第 N 页的 id 是 `sNN`，裁剪后 id 不重排，从 `s08` 开始。
- **标题与正文**：照原页翻译，保留原文的层级。原版式把正文的一级条目设为加粗，这里照样写成加粗。
- **自动加粗**：`pages.py` 的 `slide(...)` 以 `autobold=False` 调用 `p.slide(...)`，加粗全部手写。
- **分节页**：只有一行标题的原页用 `lecture.bridge(...)`，其后各页放进同名的 `section`。

**docker 的五页**（`docker-install`、`docker-pull`、`docker-run`、`docker-exec` 是 2026-10-09 新写的，`docker-cp` 是 2026-10-11 加的）：

- **页面 id** 用内容命名，不用 `sNN`；标题写这一页的内容，正文的写法与课程其他各讲相同（`**词**：说明`）。
- **命令都用 `p.demo`**，`output=` 是 2026-10-09 在本机（Docker 29.8.2，x86-64 的 Ubuntu 26.04）执行这条命令得到的输出。`docker-pull` 的输出是第一次拉取结束时终端上留下的内容，下载过程中每一层的进度行没有列出；镜像已在本机时输出是 `Status: Image is up to date for ubuntu:26.04`。
- **`docker-run` 是交互式演示**（`interactive=True, timeout=0`）：演示按钮打开一个终端，输入的每一行由容器中的 bash 执行；页面上的输出是输入 `uname -m`、`head -1 /etc/os-release`、`exit` 三行的一次运行。命令带 `--rm`：每按一次按钮创建一个新的容器，`exit` 之后这个容器被删除。
- **`docker-exec` 有两个演示，按顺序运行**：第一个 `docker run -dt --name ics …` 创建名为 `ics` 的容器并让它在后台运行；第二个是交互式演示，先 `docker start ics`，再 `docker exec -it ics bash`，页面上的输出是输入 `echo hello > /root/a.txt`、`exit` 两行的一次运行。容器 `ics` 在演示之后保留；名字已被占用时第一个演示报告 `Conflict`，再次演示之前执行 `docker rm -f ics`。记录页面上的输出之后，本机的容器 `ics` 已删除。
- **`docker-cp` 的演示在 `docker-exec` 的第一个演示之后运行**：它把讲义目录下的 `examples` 复制进容器 `ics`，成为 `/root/examples`。页面上的输出是 2026-10-11 在本机（Docker 29.9.0）的终端中执行得到的；`Successfully copied` 一行只在终端上输出。记录之后本机的容器 `ics` 已删除。
- **镜像固定为 `ubuntu:26.04`**，与讲义各页输出所用的系统相同；命令中的 `--platform linux/amd64` 使 Apple 芯片的 Mac 也得到 x86-64 的镜像。
- **下载地址**是 Docker 官方文档的三个安装页，表中显示不带 `https://` 的地址，点击打开；2026-10-09 核对过三个地址都能打开。

**第 14–35 张**（「使用 objdump」「炸弹」「使用 GDB」「任务」四节）：

- **顺序与用词照原课件**：四节的标题、拆除炸弹的四个步骤（从机器码到汇编代码、找出关键的函数与参数、找出关键的字符串、找出关键的指令）、
  GDB 的两个清单都取自原课件。讲稿在每页写明它对应原课件的哪些条目。
- **例子只有 `bomb-light` 一个**，不另写程序。用词与实验的学生说明一致：阶段、输入的一行、拆除、爆炸。
- **命令与它的输出用 `p.demo`**，带 `output=`；只供阅读的汇编清单用 `p.code`，省略处写 `...`。
  `output=` 是 2026-10-11 在 ubuntu:26.04 的容器（`--platform linux/amd64`，gdb 17.1，binutils 2.46，file 5.46，`bomb-light` 在 `/root/examples`）中执行得到的输出。
- **汇编清单**：`pages.py` 的 `MAIN` 与 `PHASE` 是 `objdump -d bomb-light` 输出中 `main` 的 20 行与 `phase_light` 的 31 行。
  各页用 `excerpt(MAIN, (起, 止), ...)` 取其中的几段，省略处是一行 `  ...`，行尾带英文注释说明省略了什么。
  清单用 `asm(p, text, mark=[...])` 显示，即 `p.code("pseudo", ...)`：`mark=` 只支持 `pseudo`，汇编清单因此是单色的。
  `PHASE` 中 `0xaaa9` 一行，`objdump` 在自己的注释之前输出 8 个空格，这里留 2 个，使这一行不超出页面的宽度。
- **`bomb-light` 用 `BOMB_UI=plain` 运行**：页面上的输出是文字。不写这一项时程序的画面占据整个终端窗口，不能作为文字放在页面上。
- **交互式演示**（`interactive=True, timeout=0`）是 `bomb-what`、`bomb-defused`、`gdb-break`、`gdb-stop` 四页：
  演示按钮在终端中启动 `bomb-light` 或 `gdb`，之后的各行从键盘输入；页面上的输出是记录下来的一次会话。
  `gdb` 的输出中省略的各行写成 `...`，省略了什么写在讲稿里。
- **`gdb-args`、`gdb-step`** 的代码块接着 `gdb-break` 的会话，用 `p.code("text", ...)` 显示，这两页没有演示按钮。
- **地址**：`bomb-light` 是位置无关的程序。正文中 `0xaa92` 这样的地址是文件中的地址，即 `objdump` 显示的地址；
  `gdb` 会话中 `0x70cf753d6a92` 这样的地址是那一次运行时的地址。容器中每次运行的装入位置不同，现场演示时地址的高位与页面上的不同，低 12 位相同。
- **演示按钮在讲义所在的机器上执行命令**，各演示的第一条命令是 `cd examples`。这台机器要是 x86-64 的 Linux，装有 `objdump`、`file`、`gdb`。
  直接在 Linux 主机上运行 `gdb` 时，没有容器中的那一行 `warning: Error disabling address space randomization`，装入位置每次相同。
- **任务页**是实验 `solve-bomb` 的一页，写明不计分；内容取自实验的学生说明（`../solve-bomb/handout/README.md`），截止日期没有写。
  实验材料的位置是 `https://github.com/SJTU-IPADS/OS-Course-Lab/tree/main/ICS-Labs/BombLab`（2026-10-11 给定，当时仓库中还没有这个目录）；
  页面上的 `docker cp OS-Course-Lab/ICS-Labs/BombLab ics:/root/` 与 `cd /root/BombLab` 用的是这个目录名。

**排版**：

- 粗体后面的全角冒号写在 `**` 之外：`**词**：内容`；
- `==标记==` 只在 `p.slide(...)` 里展开；
- 一页最多一个 `p.highlight`；
- 修改之后渲染 PNG 逐页查看，1280×720 下内容不越过 y=700。

**英文**：中文改动之后同步 `i18n/en.toml`，`i18n check` 应报告 0 missing、0 changed、0 orphaned。

## 渲染

```bash
python3 -m lecturekit.cli render lectures/tut-asm-lab --pdf
```

```bash
python3 -m lecturekit.cli render lectures/tut-asm-lab --lang en --strict --pdf
```

输出在 `build/tut-asm-lab-viewer/`，PDF 的文件名由标题得到，是 `汇编实验准备.pdf`。

```bash
python3 -m lecturekit.cli render lectures/tut-asm-lab --to pptx
```

输出是 `build/tut-asm-lab-pptx/汇编实验准备.pptx`。

```bash
python3 -m lecturekit.cli i18n check lectures/tut-asm-lab --lang en
```
