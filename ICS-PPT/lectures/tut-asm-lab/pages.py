"""辅导课「汇编实验准备」的页面。原 PDF（ICS-tutorial-1-env）第 N 页对应函数 sNN，保留第 9–13 页。

这几页的原文是英文，英文原文在 i18n/en.toml，这里是中文译文。原版式把正文的一级条目设为加粗，
这里照样写成加粗。

插图：assets/s012.jpg、s013.jpg 是原 PDF 内嵌的图片；s010.jpg 是内嵌图片按原页的裁剪范围
裁去上部（原页只显示贴纸，不显示上面的标题文字）；s011-2.png（vmware 图标）由内嵌图片和它的
透明蒙版合成。其余裁自 PDF 渲染的原页：s009.png 是由方框和应用图标拼成的示意图，图上的
Various Applications、Serve Applications、Manage Hardware、CPU, Memory, GPU… 属于这张图，
两种语言共用，裁图时抹去了页码；s011-1.png 是用图形画的虚拟机层次图。
"""


def slide(p, md, **kw):
    """原页的正文。原文没有逐行加粗，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def fig(p, name, width=None, height=None):
    """原页的插图 assets/<name>。"""
    return p.image(f"assets/{name}", width_px=width, height_px=height)


def side(p, name, width, place="right"):
    """原页的插图放在整页高的侧栏里，正文在另一栏。"""
    return p.side_image(f"assets/{name}", alt="contain", width=width, side=place)


def s09(p):
    p.title("操作系统")
    fig(p, "s009.png", height=540)


def s10(p):
    p.title("Linux")
    fig(p, "s010.jpg", width=1180)


def s11(p):
    p.title("虚拟机虚拟化")
    side(p, "s011-1.png", "40%")
    slide(p, """
- **共享物理机器**
  - 虚拟机（VM）
- **安装操作系统和应用程序**
  - 就像使用一台真实的机器
- **虚拟机监控器**
  - Xen、KVM、HyperV、VirtualBox
  - 我们在 ICS 中用过 **vmware**
""")
    fig(p, "s011-2.png", width=72)


def s12(p):
    p.title("容器虚拟化")
    fig(p, "s012.jpg", height=560)


def s13(p):
    p.title("容器虚拟化")
    slide(p, "**ICS：我们用 docker 在 Mac 或 Windows 上运行 ubuntu 容器**")
    fig(p, "s013.jpg", height=500)


# The pages below are not from the original PDF. Their commands were run on
# 2026-10-09 (Docker 29.8.2 on x86-64 Ubuntu 26.04) and the outputs are what
# the commands printed then.

def docker_install(p):
    p.title("安装 docker：下载地址")
    p.table([
        ["macOS", "Docker Desktop",
         "[docs.docker.com/desktop/setup/install/mac-install](https://docs.docker.com/desktop/setup/install/mac-install/)"],
        ["Windows", "Docker Desktop",
         "[docs.docker.com/desktop/setup/install/windows-install](https://docs.docker.com/desktop/setup/install/windows-install/)"],
        ["Linux（Ubuntu）", "Docker Engine",
         "[docs.docker.com/engine/install/ubuntu](https://docs.docker.com/engine/install/ubuntu/)"],
    ], headers=["宿主机的系统", "安装的软件", "下载地址"], widths=[18, 18, 64])
    slide(p, """
- **macOS**：Apple 芯片的 Mac 选 Apple silicon，Intel 处理器的 Mac 选 Intel chip；
- **Windows**：安装程序的配置页上选中 Use WSL 2 instead of Hyper-V；
- **安装之后**：启动 Docker Desktop，在终端中执行下面的命令，能够输出版本号时安装完成。
""")
    p.demo("检查 docker 命令", "docker --version",
           output="Docker version 29.8.2, build 7fc2dff")
    p.notes("""
三个地址都是 Docker 官方文档的安装页，页面开头是安装包的下载按钮。2026-10-09 三个地址都能打开。
macOS 的两个安装包在页面上的名字是 Docker Desktop for Mac with Apple silicon 与 Docker Desktop for Mac with Intel chip。
Windows 的安装程序在 Configuration 页上有选项 Use WSL 2 instead of Hyper-V，选中时 Docker Desktop 用 WSL 2 运行 Linux 容器。
Linux 上安装的是 Docker Engine，按安装页的步骤用 apt 安装；x86-64 的 Linux 机器也可以不用 docker，直接在本机完成课程的实验。
docker --version 的输出是本机的版本，版本号随安装的时间不同。
""")


def docker_pull(p):
    p.title("拉取镜像：x86-64 的 ubuntu 26.04")
    slide(p, """
- **镜像（image）**：创建容器所用的文件，从镜像仓库 Docker Hub 下载。`ubuntu` 是镜像的名字，`26.04` 是版本；
- **`--platform linux/amd64`**：选择 x86-64 的镜像，amd64 是 x86-64 的另一个名字。本课程讲解的指令与实验的程序都是 x86-64 的；
- Apple 芯片的 Mac 在不指定 platform 的情况下默认拉取 arm64 的镜像。
""")
    p.demo("拉取 x86-64 的 ubuntu 镜像",
           """docker pull --platform linux/amd64 ubuntu:26.04
docker image inspect --format '{{.Os}}/{{.Architecture}}' ubuntu:26.04""",
           output="""26.04: Pulling from library/ubuntu
4e07a0f12b2c: Pull complete
06ad70e463aa: Pull complete
Digest: sha256:f144425ff09be612d6d9ad965196e9cdc23dae1f42110a8a11a3e9a8198759f7
Status: Downloaded newer image for ubuntu:26.04
docker.io/library/ubuntu:26.04
linux/amd64""",
           bold=[7], timeout=600)
    p.notes("""
第一条命令从 Docker Hub 下载镜像，输出的前六行属于它：镜像分两层，每层一行，下载并解开后显示 Pull complete；Digest 是镜像内容的 SHA-256 摘要；最后一行是镜像的全名。输出是 2026-10-09 第一次拉取时终端上最后留下的内容，下载过程中每一层的进度（Pulling fs layer、Download complete）没有列出。
镜像已经在本机时，这条命令的输出是 Status: Image is up to date for ubuntu:26.04，层的两行不出现；镜像更新之后摘要也随之变化。
第二条命令打印镜像的系统与处理器架构，linux/amd64 说明拉取到的是 x86-64 的镜像。
镜像约 101 MB。
""")


def docker_run(p):
    p.title("运行容器：在 ubuntu 容器中启动 bash")
    p.demo("运行 ubuntu 容器，在其中执行命令",
           "docker run -it --rm --platform linux/amd64 ubuntu:26.04 bash",
           output="""root@b3bcc8442459:/# uname -m
x86_64
root@b3bcc8442459:/# head -1 /etc/os-release
PRETTY_NAME="Ubuntu 26.04.1 LTS"
root@b3bcc8442459:/# exit
exit""",
           interactive=True, timeout=0)
    slide(p, """
- **`docker run 镜像 命令`**：由镜像创建一个容器，在容器中运行命令，这里的命令是 `bash`；
- **`-it`**：把当前的终端交给容器中的 `bash`，键盘的输入由它读取，它的输出显示在终端上；
- **`--rm`**：容器停止时删除这个容器，在其中写入的文件一并删除；
- **提示符 `root@b3bcc8442459`**：`@` 后面是这个容器的编号。`uname -m` 输出 `x86_64`；
- **`exit`**：结束 `bash`，容器随之停止。
""")
    p.notes("""
演示按钮在终端中运行这条命令，随后输入的每一行由容器中的 bash 执行；页面上的输出是 2026-10-09 的一次运行：输入了 uname -m、head -1 /etc/os-release、exit 三行。容器的编号每次运行都不同。
-i 保持容器的标准输入打开，-t 为容器分配一个终端，两者合写为 -it。
每执行一次 docker run 就创建一个新的容器。带 --rm 时，bash 结束后这个容器被删除，docker ps -a 的列表中没有它，在其中安装的软件与写入的文件随之删除。
Apple 芯片的 Mac 上，x86-64 容器中的指令由 Docker Desktop 翻译为 arm64 指令执行，uname -m 同样输出 x86_64，运行速度比 arm64 的容器慢。
""")


def docker_exec(p):
    p.title("后台运行容器：docker start 与 docker exec")
    p.demo("在后台运行名为 ics 的容器",
           "docker run -dt --name ics --platform linux/amd64 ubuntu:26.04 bash",
           output="fbeaf9a5eea5f980092a5cd7ff52fec9e8c52db0aec3e235dc4df0398d13827d")
    p.demo("启动容器，进入容器执行命令",
           """docker start ics
docker exec -it ics bash""",
           output="""ics
root@fbeaf9a5eea5:/# echo hello > /root/a.txt
root@fbeaf9a5eea5:/# exit
exit""",
           interactive=True, timeout=0)
    slide(p, """
- **`-dt`**：`-d` 使容器在后台运行；`-t` 分配终端，`bash` 等待输入，容器保持运行；
- **`--name ics`**：给容器起名 `ics`。命令不带 `--rm`，容器停止后保留，文件也保留；
- **`docker start ics`**：启动已停止的容器。计算机重启后容器处于停止状态；
- **`docker exec -it ics bash`**：在容器中另启动一个 `bash`，`exit` 之后容器继续运行。
""")
    p.notes("""
两段命令在 2026-10-09 依次执行，页面上的输出是这一次的结果；执行第二段之前用 docker stop ics 停止了容器。
第一段：-d 使 docker run 不等待容器结束，输出容器的 64 位十六进制编号后返回；-t 为 bash 分配一个终端，bash 在终端上等待输入，容器因此保持运行。只写 -d 时 bash 读到输入结束便退出，容器随即停止。--name ics 指定容器的名字，后面的命令用名字指明容器；不写时 docker 起一个随机的名字，用 docker ps -a 查看。
第二段：docker start ics 启动已停止的容器，输出的 ics 是容器的名字；容器已在运行时这条命令同样输出 ics。docker exec -it ics bash 在容器 ics 中再启动一个 bash 并把终端交给它，提示符中的 fbeaf9a5eea5 是容器编号的前 12 位。输入了 echo hello > /root/a.txt 与 exit 两行。exit 结束的是这个 bash，docker run 启动的 bash 仍在运行，容器继续运行。
docker exec ics 命令 不带 -it，执行一条命令后返回。停止容器用 docker stop ics，这条命令约需 10 秒。2026-10-09 核对过：写入 /root/a.txt 之后依次执行 docker stop ics、docker start ics、docker exec ics cat /root/a.txt，输出 hello。
容器处于停止状态时 docker exec 报告 container … is not running。计算机重启或退出 Docker Desktop 之后容器处于停止状态，先执行 docker start ics。
名字 ics 已被占用时，第一段的 docker run 报告 Conflict. The container name "/ics" is already in use。docker rm -f ics 删除这个容器，其中的文件一并删除，之后可以再次创建。
""")


def docker_cp(p):
    p.title("在宿主机与容器之间复制文件：docker cp")
    p.demo("把目录 examples 复制进容器 ics",
           """docker cp examples ics:/root/
docker exec ics ls /root/examples""",
           output="""Successfully copied 1.18MB (transferred 1.18MB) to ics:/root/
bomb-light""")
    slide(p, """
- **`docker cp 源 目标`**：在宿主机与容器之间复制文件或目录，在宿主机的终端中执行；
- **容器中的路径**写成 `容器名:路径`。`docker cp examples ics:/root/` 把当前目录下的目录 `examples` 复制进容器 `ics`，成为 `/root/examples`；
- **从容器复制到宿主机**：两个参数交换位置。`docker cp ics:/root/a.txt .` 把容器中的 `/root/a.txt` 复制到宿主机的当前目录；
""")
    p.notes("""
页面上的输出是 2026-10-11 在本机（Docker 29.9.0，x86-64 的 Ubuntu 26.04）的终端中执行得到的；容器 ics 由上一页的第一个演示创建。
docker cp 的第一个参数是源，第二个是目标；带「容器名:」的一项是容器中的路径，另一项是宿主机上的路径。源是目录时连同其中的文件一起复制。目标 ics:/root/ 是容器中已有的目录，examples 复制到它之下，成为 /root/examples。
Successfully copied 一行只在终端上输出；输出接到管道或文件时没有这一行。
examples 是本讲义目录中的 examples/，其中只有 bomb-light 一个文件。「使用 objdump」一节起各页的输出都是在容器的 /root/examples 中执行得到的。
2026-10-11 核对过另外两点：docker cp ics:/root/a.txt . 之后，宿主机当前目录下 a.txt 的内容是 hello；容器处于停止状态时 docker cp 同样可以复制。
""")


# The pages below follow the tutorial deck ICS-tutorial-5-asm.pptx (37 slides),
# in its order. The deck showed its listings as screenshots of a small demo
# bomb. Here the bomb is examples/bomb-light, the program of the lab
# solve-bomb that the lecture takes apart, and the listings are p.demo and
# p.code blocks. The outputs were recorded on 2026-10-11 in an ubuntu:26.04
# container (binutils 2.46, gdb 17.1, file 5.46) with bomb-light in
# /root/examples. Tabs in the outputs are written as the spaces a terminal
# shows.

# objdump -d bomb-light | sed -n '/<main>:/,/ret/p'
MAIN = """\
000000000000aa49 <main>:
    aa49:       55                      push   %rbp
    aa4a:       48 89 e5                mov    %rsp,%rbp
    aa4d:       48 83 ec 20             sub    $0x20,%rsp
    aa51:       89 7d ec                mov    %edi,-0x14(%rbp)
    aa54:       48 89 75 e0             mov    %rsi,-0x20(%rbp)
    aa58:       48 8b 55 e0             mov    -0x20(%rbp),%rdx
    aa5c:       8b 45 ec                mov    -0x14(%rbp),%eax
    aa5f:       48 89 d6                mov    %rdx,%rsi
    aa62:       89 c7                   mov    %eax,%edi
    aa64:       e8 ba 01 00 00          call   ac23 <initialize_bomb>
    aa69:       e8 19 05 00 00          call   af87 <read_line>
    aa6e:       48 89 45 f8             mov    %rax,-0x8(%rbp)
    aa72:       48 8b 45 f8             mov    -0x8(%rbp),%rax
    aa76:       48 89 c7                mov    %rax,%rdi
    aa79:       e8 0c 00 00 00          call   aa8a <phase_light>
    aa7e:       e8 b1 05 00 00          call   b034 <phase_defused>
    aa83:       b8 00 00 00 00          mov    $0x0,%eax
    aa88:       c9                      leave
    aa89:       c3                      ret""".splitlines()

# objdump -d bomb-light | sed -n '/<phase_light>:/,/ret/p'
# In the line at aaa9 objdump puts 8 spaces before its own comment; 2 are
# kept, so that the line fits the width of a slide.
PHASE = """\
000000000000aa8a <phase_light>:
    aa8a:       55                      push   %rbp
    aa8b:       48 89 e5                mov    %rsp,%rbp
    aa8e:       48 83 ec 20             sub    $0x20,%rsp
    aa92:       48 89 7d e8             mov    %rdi,-0x18(%rbp)
    aa96:       64 48 8b 04 25 28 00    mov    %fs:0x28,%rax
    aa9d:       00 00
    aa9f:       48 89 45 f8             mov    %rax,-0x8(%rbp)
    aaa3:       31 c0                   xor    %eax,%eax
    aaa5:       48 8d 55 f4             lea    -0xc(%rbp),%rdx
    aaa9:       48 8d 0d 7c 05 0c 00    lea    0xc057c(%rip),%rcx  # cb02c <__rseq_flags+0x28>
    aab0:       48 8b 45 e8             mov    -0x18(%rbp),%rax
    aab4:       48 89 ce                mov    %rcx,%rsi
    aab7:       48 89 c7                mov    %rax,%rdi
    aaba:       b8 00 00 00 00          mov    $0x0,%eax
    aabf:       e8 bc 0d 02 00          call   2b880 <__isoc99_sscanf>
    aac4:       83 f8 01                cmp    $0x1,%eax
    aac7:       74 05                   je     aace <phase_light+0x44>
    aac9:       e8 ea 05 00 00          call   b0b8 <explode_bomb>
    aace:       8b 45 f4                mov    -0xc(%rbp),%eax
    aad1:       3d 95 28 35 01          cmp    $0x1352895,%eax
    aad6:       74 05                   je     aadd <phase_light+0x53>
    aad8:       e8 db 05 00 00          call   b0b8 <explode_bomb>
    aadd:       90                      nop
    aade:       48 8b 45 f8             mov    -0x8(%rbp),%rax
    aae2:       64 48 2b 04 25 28 00    sub    %fs:0x28,%rax
    aae9:       00 00
    aaeb:       74 05                   je     aaf2 <phase_light+0x68>
    aaed:       e8 ce 89 05 00          call   634c0 <__stack_chk_fail>
    aaf2:       c9                      leave
    aaf3:       c3                      ret""".splitlines()


def excerpt(listing, *parts):
    """An excerpt of MAIN or PHASE. A pair (first, last) names lines of the
    listing, counted from 1; a string is put in as it is."""
    out = []
    for part in parts:
        if isinstance(part, str):
            out.append(part)
        else:
            first, last = part
            out.extend(listing[first - 1:last])
    return "\n".join(out)


def asm(p, text, mark=None):
    """A listing from objdump that the page marks line by line. `mark` is
    only available for the `pseudo` language."""
    return p.code("pseudo", text, mark=mark)


def objdump(p):
    p.title("objdump：显示目标文件与可执行文件中的信息")
    slide(p, """
- **objdump（OBJect-file DUMP）**：显示目标文件（`.o`）与可执行文件中的信息。
""")
    p.table([
        ["`objdump -d 文件`", "反汇编：把存放指令的节译成汇编代码"],
        ["`objdump -D 文件`", "反汇编所有的节，数据的字节也按指令译出"],
        ["`objdump -d --disassemble=main 文件`", "只反汇编函数 `main`"],
        ["`objdump -s 文件`", "各节的内容：十六进制的字节与对应的字符"],
        ["`objdump -f 文件`", "文件头：文件的格式、处理器的架构、程序的起始地址"],
        ["`objdump -t 文件`", "符号表：函数与全局变量的名字与地址"],
    ], headers=["命令", "显示的内容"])
    slide(p, """
- **节（section）**：可执行文件中连续的一段。`.text` 存放指令，`.rodata` 存放只读的数据，例如字符串常量。
""")
    p.notes("""
objdump 属于 GNU 的 binutils 软件包。表中各行是 objdump --help 中这些选项的说明：-d 是 --disassemble，反汇编可执行的节；-D 是 --disassemble-all，反汇编所有的节；-s 是 --full-contents；-f 是 --file-headers；-t 是 --syms。另有 -h（--section-headers），显示各节的名字、大小与地址。
-D 把数据节的字节也当作指令译出：机器码一栏是数据本身，汇编指令一栏没有意义。查看数据用 -s。
--disassemble=main 只输出一个函数；函数的名字写在等号之后。
""")


def objdump_output(p):
    p.title("objdump -d 的输出：地址、机器码、汇编指令")
    p.demo("反汇编 bomb-light，输出 main 开头的 6 行",
           """cd examples
objdump -d bomb-light | sed -n '/<main>:/,+5p'""",
           output=excerpt(MAIN, (1, 6)))
    slide(p, """
- **`000000000000aa49 <main>:`**：函数 `main` 从地址 `0xaa49` 开始；
- **每行三栏**：指令的地址、指令的机器码（十六进制的字节）、汇编指令；
- **地址**：`objdump` 显示的是文件中的地址。与程序运行时的地址不完全一致，将在后续说明。
""")
    p.notes("""
bomb-light 是 examples/ 目录中的可执行文件，与实验 solve-bomb 的材料中的 bomb-light 是同一个文件。输出记录于 2026-10-11。
机器码一栏：push %rbp 是 1 个字节 55，mov %rsp,%rbp 是 3 个字节 48 89 e5。下一条指令的地址等于这一条的地址加这一条的字节数：0xaa49 + 1 = 0xaa4a，0xaa4a + 3 = 0xaa4d。
0xaa49 是文件中的地址：bomb-light 运行时被装入内存中的某个位置，main 的地址是装入位置加 0xaa49。「使用 GDB」一节的现场演示显示了运行时的地址。
不带 sed 时，objdump -d bomb-light 输出整个文件的汇编代码，共 196119 行。地址范围中的 +5 是 GNU sed 的写法。
""")


def bomb_objdump(p):
    p.title("从机器码到汇编代码")
    p.demo("使用 objdump 工具反汇编并保存到文件，找到 main",
           """cd examples
objdump -d bomb-light > bomb-light.asm
wc -l bomb-light.asm
grep -n '<main>:' bomb-light.asm""",
           output="""196119 bomb-light.asm
844:000000000000aa49 <main>:""")
    slide(p, """
- **`objdump -d bomb-light > bomb-light.asm`**：把汇编代码保存到文件 `bomb-light.asm`，用编辑器阅读与查找；
- **196119 行**：`bomb-light` 自己的函数之外，`sscanf` 等库函数的代码也在这个文件中；
- **从 `main` 读起**：C 程序的入口点是 `main`，位于 `bomb-light.asm` 的第 844 行。
""")
    p.notes("""
原课件这一页列出两条命令：objdump -D bomb > asm 与 objdump -d bomb > asm。-D 连数据节一起反汇编；阅读指令用 -d。
bomb-light.asm 约 10 MB。输出记录于 2026-10-11。演示之后 examples/ 中留有 bomb-light.asm，可以删除。
main 之前的 843 行是 .init 与 .plt 两个节以及 C 库的几个函数；main 之后依次是 phase_light 与 bomb-light 的其余函数，再往后是库函数。
""")


def bomb_main(p):
    p.title("关键函数：main 中出现的四条 call")
    asm(p, excerpt(MAIN, (1, 1),
                   "  ...                                   # 9 instructions: stack frame, argc and argv",
                   (11, 17),
                   "  ...                                   # 3 instructions: return 0"),
        mark=[3, 4, 8, 9])
    slide(p, """
- **`call ac23 <initialize_bomb>`**：`call` 的目标是地址 `0xac23`，尖括号中是这个地址上的符号，这里是函数名；
- **`read_line`** 读入一行，返回值是这一行的地址，在 `%rax` 中。`mov %rax,%rdi` 把它作为第一个参数传给 **`phase_light`**；
- **`phase_defused`** 在 `phase_light` 返回之后被调用：炸弹被拆除；
- `phase_light` 在什么条件下返回？
""")
    p.notes("""
清单取自 objdump -d bomb-light 的输出中 main 的部分（bomb-light.asm 的第 844 行起）。main 共 19 条指令，页面上省略了开头 9 条与结尾 3 条，写成 ...，这两行行尾的英文注释是讲义加的。有底色的是四条 call。
开头 9 条建立栈帧，把 main 的两个参数 argc、argv 存入栈帧，再作为参数传给 initialize_bomb；结尾 3 条是 mov $0x0,%eax、leave、ret，即 return 0。
对应的 C 代码：initialize_bomb(argc, argv); line = read_line(); phase_light(line); phase_defused();。mov %rax,-0x8(%rbp) 与 mov -0x8(%rbp),%rax 把返回值存入局部变量 line，再读出。
函数名来自可执行文件的符号表（objdump -t bomb-light）。bomb-light 保留了符号表，file 的输出中有 not stripped。
""")


def bomb_calls(p):
    p.title("关键函数：phase_light 中的 sscanf 与 explode_bomb")
    p.demo("反汇编 phase_light，取出其中的 call 指令",
           """cd examples
objdump -d --disassemble=phase_light bomb-light | grep call""",
           output=excerpt(PHASE, (16, 16), (19, 19), (23, 23), (29, 29)))
    slide(p, """
- **`__isoc99_sscanf`**：`sscanf` 在 C 库中的名字。`sscanf` 按格式串从一个字符串中读出数据；
- **`explode_bomb`**：使炸弹爆炸。`phase_light` 中有两条 `call explode_bomb`；
- **`__stack_chk_fail`**：金丝雀值被改写时调用，与输入是否正确无关；
- **接下来要确定**：`sscanf` 读出的数据存放在哪里，什么条件下执行到 `call explode_bomb`。
""")
    p.notes("""
--disassemble=phase_light 只反汇编这一个函数，grep call 留下其中含 call 的 4 行。输出记录于 2026-10-11。
__isoc99_sscanf 是 C99 标准的 sscanf：头文件 stdio.h 把 sscanf 换成这个名字。
__stack_chk_fail：phase_light 开头的 mov %fs:0x28,%rax 与 mov %rax,-0x8(%rbp) 把金丝雀值存入栈帧，结尾的 sub %fs:0x28,%rax 检验它，值被改写时调用 __stack_chk_fail。实验的 bomb 的每个阶段都有这几条指令，与答案无关。
原课件这一页的两个问题：scanf，密码存放在哪里；printf/puts，哪条指令输出 bomb。
""")


def bomb_args(p):
    p.title("sscanf 的参数：读出的数据写入 -0xc(%rbp)")
    asm(p, excerpt(PHASE, (5, 5),
                   "  ...                                   # 3 instructions: canary",
                   (10, 16)),
        mark=[3, 6, 7])
    slide(p, """
- **`int sscanf(const char *str, const char *format, ...)`**：按格式串 `format` 从字符串 `str` 中读出数据，写入其后各个参数给出的地址；
- **`%rdi`**：`-0x18(%rbp)` 中的值，即 `0xaa92` 处存入的 `phase_light` 的参数，读入的一行；
- **`%rsi`**：格式串的地址 `0xcb02c`。`0xc057c(%rip)` 是下一条指令的地址 `0xaab0` 加 `0xc057c`，`objdump` 在 `#` 之后写出结果；
- **`%rdx`**：地址 `%rbp - 0xc`，`sscanf` 把读出的数据写入这里。`0xcb02c` 处的格式串是什么？
""")
    p.notes("""
清单是 phase_light 中 0xaa92 处的一条指令与 0xaaa5 到 0xaabf 的 7 条指令，中间省略的 3 条属于金丝雀。有底色的三行写入 sscanf 的三个参数所在的寄存器：第一个参数在 %rdi，第二个在 %rsi，第三个在 %rdx。
call 之前的 mov $0x0,%eax：调用参数个数可变的函数时，%al 给出用于传递参数的向量寄存器的个数，这里是 0。
0xc057c(%rip)：%rip 是下一条指令的地址 0xaab0，0xaab0 + 0xc057c = 0xcb02c。位置无关的程序用这种方式取得数据的地址；实验的 bomb 每个阶段的格式串都这样取得。行尾 # 之后的 cb02c 是 objdump 算出的地址，<__rseq_flags+0x28> 是离这个地址最近的符号加偏移，格式串本身没有符号。这一行 # 之前的空格比 objdump 的输出少 6 个。
lea 计算地址，不访问内存。-0xc(%rbp) 在 phase_light 的栈帧中，是 sub $0x20,%rsp 留出的 32 字节中的 4 个。
""")


def bomb_strings(p):
    p.title("关键字符串：0xcb02c 处的格式串")
    p.demo("显示地址 0xcb02c 起的 3 个字节",
           """cd examples
objdump -s --start-address=0xcb02c --stop-address=0xcb02f bomb-light | tail -1""",
           output=" cb02c 256400                               %d.")
    slide(p, """
- **`objdump -s`**：显示节的内容。一行依次是地址、十六进制的字节、这些字节对应的字符，不能显示的字节写成 `.`；
- **`25 64 00`**：字符 `%`、`d` 与结尾的 0。`0xcb02c` 处的字符串是 `%d`；
- **`%d`**：读出一个十进制整数。`sscanf` 把这个整数写入 `-0xc(%rbp)` 起的 4 个字节，下面把这个局部变量称为 `x`；
- **`sscanf` 的返回值**是成功读出的数据的个数，在 `%eax` 中。
""")
    p.notes("""
objdump -s 显示节的内容，--start-address 与 --stop-address 限定地址的范围，tail -1 只留下输出的最后一行，它之前是文件名、节名 .rodata 与空行。输出记录于 2026-10-11。0xcb02c 同样是文件中的地址。
字符串以值为 0 的字节结束。
原课件用 objdump -D 的输出读这些字节：-D 把 .rodata 也按指令反汇编，机器码一栏就是这些字节，汇编指令一栏没有意义。
局部变量的名字不在可执行文件中，x 是讲义起的名字。输入的一行不以整数开头时，sscanf 的返回值不是 1。
gdb 的 x/s 直接显示一个地址上的字符串，见「使用 GDB」一节。
""")


def bomb_branch(p):
    p.title("关键指令：是否执行 call explode_bomb")
    asm(p, excerpt(PHASE, (16, 24)), mark=[2, 3, 6, 7])
    slide(p, """
- **`jmp`、`jXX`** 改变控制流，**`cmp`、`test`** 设置它们所依据的标志位。`je` 在比较的两个数相等时跳转；
- **`cmp $0x1,%eax`、`je aace`**：`%eax` 是 `sscanf` 的返回值。它等于 1 时跳过 `0xaac9` 处的 `call explode_bomb`；
- **`cmp $0x1352895,%eax`、`je aadd`**：`%eax` 是从 `-0xc(%rbp)` 读出的 `x`。它等于 `0x1352895` 时跳过 `0xaad8` 处的 `call explode_bomb`；
- **正确的输入**：一个整数，等于 `0x1352895`，十进制是 20261013。
""")
    p.notes("""
清单是 phase_light 中 0xaabf 到 0xaadd 的 9 条指令，有底色的是两组 cmp 与 je。
cmp $0x1,%eax 计算 %eax - 1，只设置标志位，不保存结果；je 在 ZF 为 1 时跳转。
je aace <phase_light+0x44>：跳转的目标是 0xaace，尖括号中是函数名加偏移，0xaa8a + 0x44 = 0xaace。
0x1352895 = 1×16⁶ + 3×16⁵ + 5×16⁴ + 2×16³ + 8×16² + 9×16 + 5 = 20261013。换算可以用计算器；gdb 中执行 p 0x1352895 也输出 20261013。
0xaadd 之后是金丝雀值的检验与 leave、ret。两条 je 都跳转时 phase_light 返回，main 接着调用 phase_defused。
原课件这一页的结论是 -0x4(%rbp) == %eax == 0xbeef。
""")


def bomb_defused(p):
    p.title("拆除炸弹：输入 20261013")
    p.demo("运行 bomb-light，输入 20261013",
           """cd examples
BOMB_UI=plain ./bomb-light""",
           output="""bomb-light: 1 phase. One line defuses a phase. A wrong line sets the bomb off.
bomb-light: Enter sends the line, Ctrl-D leaves.
phase 1 of 1> 20261013
phase 1 of 1 defused
The phase is defused.""",
           interactive=True, timeout=0)
    p.table([
        ["从机器码到汇编代码", "`objdump -d bomb-light`，从 `main` 读到 `phase_light`"],
        ["找出关键的函数与参数", "`sscanf` 与两条 `call explode_bomb`；读出的整数在 `-0xc(%rbp)`"],
        ["找出关键的字符串", "`0xcb02c` 处的格式串 `%d`"],
        ["找出关键的指令", "`cmp $0x1,%eax`、`cmp $0x1352895,%eax` 与各自之后的 `je`"],
    ], headers=["步骤", "在 `bomb-light` 中找到的内容"], widths=[28, 72])
    p.notes("""
页面上的输出是 2026-10-11 在 ubuntu:26.04 的容器中的一次运行，输入的是 20261013，程序以状态 0 退出。
phase_light 的 C 代码：int x; if (sscanf(line, "%d", &x) != 1) explode_bomb(); if (x != 20261013) explode_bomb();。
表中的四个步骤是原课件「Step by Step」各页累积的清单：From machine code to assembly code、Find key functions and parameters、Find key strings、Find key operators。
""")


def gdb(p):
    p.title("GDB：运行程序，在指定的位置停下，查看寄存器与内存")
    slide(p, """
阅读汇编代码时，寄存器与内存中的值要自己推算；`gdb` 运行程序并显示这些值。
""")
    p.columns("""
**GDB（GNU DeBugger）的功能**

- 启动程序
- 使程序在指定的条件下停止
- 检查程序停下时的状态
- 修改程序中的内容
""", """
**用 GDB 拆除炸弹**

- 在关键的路径上设置断点
- 查看寄存器与内存
- 单步执行，每次一条指令
- 在炸弹爆炸之前终止程序
""", autobold=False)
    slide(p, """
- **安装**：在 ubuntu 容器中执行下面两条命令。容器中的用户是 root，命令之前不写 `sudo`。
""")
    p.code("bash", """apt update
apt install -y gdb binutils file     # objdump is in the package binutils""")
    p.notes("""
左栏是原课件「GNU Tools」一页的四项：Start your program、Stop on special conditions、Examine what has happened、Change things in your program。右栏是原课件「Can we do it more efficiently and safely?」一页的四项。
2026-10-11 在 ubuntu:26.04 的容器中执行过这两条命令，装上的是 gdb 17.1、binutils 2.46 与 file 5.46。file 用在「静态链接与动态链接」一页。原课件写的是 sudo apt install gdb，用于普通用户登录的 Ubuntu。
""")


def gdb_commands(p):
    p.title("GDB 的常用命令")
    p.table([
        ["`gdb 文件`", "", "启动 `gdb`，读入可执行文件"],
        ["`break 函数名`、`break *地址`", "`b`", "设置断点：函数的开头，或这个地址上的指令"],
        ["`run`", "`r`", "运行程序，到断点处停下"],
        ["`print/x $寄存器`", "`p`", "输出寄存器的值，`/x` 表示十六进制"],
        ["`x/s 地址`", "", "显示内存中的内容，`/s` 表示按字符串显示"],
        ["`continue`", "`c`", "继续运行，到下一个断点处停下"],
        ["`stepi`、`nexti`", "`si`、`ni`", "执行一条指令；遇到 `call`，`nexti` 执行到函数返回"],
        ["`layout asm`", "", "打开汇编窗口，显示当前指令前后的汇编代码"],
        ["`quit`", "`q`", "退出 `gdb`，被调试的程序随之结束"],
    ], headers=["命令", "缩写", "作用"], widths=[30, 12, 58])
    p.notes("""
表中各行取自原课件「GDB (Gnu DeBugger) Usage」一页与「A live demo」一页用到的命令。
stepi 遇到 call 时进入被调用的函数，停在它的第一条指令上；nexti 执行到这个函数返回，停在 call 的下一条指令上。
print 与 x 的斜线之后是显示的格式：x 十六进制，d 十进制，s 字符串，i 指令。x/4xb 地址 显示 4 个字节的十六进制，x/i $pc 显示将要执行的指令。
break *地址 中的地址可以写成函数名加偏移，如 break *phase_light+53，见现场演示。
gdb-ref.txt 是课程网站上的命令列表，2026-10-11 能够打开。
""")


def gdb_break(p):
    p.title("演示：在 phase_light 设置断点，运行到断点处")
    p.demo("用 gdb 运行 bomb-light",
           """cd examples
BOMB_UI=plain gdb -q bomb-light""",
           output="""...
(gdb) b phase_light
Breakpoint 1 at 0xaa92
(gdb) r
...
phase 1 of 1> 224

Breakpoint 1, 0x000070cf753d6a92 in phase_light ()""",
           bold=[3, 8], interactive=True, timeout=0)
    slide(p, """
- **`b phase_light`**：断点在 `0xaa92`，`phase_light` 开头建立栈帧的 3 条指令之后；
- **`r`**：运行程序。输入一行 `224` 之后程序调用 `phase_light`，停在断点处，`0xaa92` 处的指令还没有执行；
- **运行时的地址 = 装入位置 + 文件中的地址**：`0x70cf753d6a92 = 0x70cf753cc000 + 0xaa92`。装入位置在程序启动时确定，在容器中每次运行都不同。
""")
    p.notes("""
演示按钮在终端中启动 gdb，之后的命令从键盘输入。页面上的输出是 2026-10-11 在 ubuntu:26.04 的容器中的一次会话，bomb-light 在 /root/examples 中；-q 使 gdb 启动时不输出版本信息，BOMB_UI=plain 使 bomb-light 只输出文字。
开头的 ... 处是 gdb 读入符号时输出的两行：Reading symbols from bomb-light... 与 (No debugging symbols found in bomb-light)。bomb-light 没有用 -g 编译，没有调试信息；函数名来自符号表，b phase_light 仍然可用。
r 之后的 ... 处是 4 行：Starting program: /root/examples/bomb-light；warning: Error disabling address space randomization: Operation not permitted；bomb-light 输出的两行说明。
warning 一行：gdb 启动程序时要关闭地址空间随机化，容器不允许这项操作，装入位置因此每次运行都不同；同一天的另一次会话中这个断点在 0x00007ce123c9da92。直接在 Linux 主机上运行 gdb 时没有这一行，装入位置每次相同。
b phase_light 的断点在函数开头之后 8 个字节处（phase_light+8）：gdb 跳过了 push %rbp、mov %rsp,%rbp、sub $0x20,%rsp 三条指令。装入位置 0x70cf753cc000 由 0x70cf753d6a92 - 0xaa92 得到。
原课件的演示在 r 之后执行 layout asm，打开汇编窗口，显示当前指令前后的汇编代码；页面上的输出是文字，没有这个窗口。
""")


def gdb_stop(p):
    p.title("演示：在 explode_bomb 设置断点，在爆炸之前退出")
    p.demo("断点设在 explode_bomb，输入 224",
           """cd examples
BOMB_UI=plain gdb -q bomb-light""",
           output="""...
(gdb) b explode_bomb
Breakpoint 1 at 0xb0bc
(gdb) r
...
phase 1 of 1> 224

Breakpoint 1, 0x00007789a7e9d0bc in explode_bomb ()
(gdb) quit
...""",
           interactive=True, timeout=0)
    slide(p, """
- **`b explode_bomb`**：两条 `call explode_bomb` 都进入这个函数，程序在爆炸之前停下；
- **停下之后**可以查看寄存器与内存，找出输入在哪一处比较中没有通过；
- **`quit`**：退出 `gdb`，`bomb-light` 随之结束，没有输出 `phase 1 of 1 exploded`。
""")
    p.notes("""
页面上的输出是 2026-10-11 在 ubuntu:26.04 的容器中的另一次会话。前两处 ... 与上一个演示相同：开头是 gdb 读入符号的两行，r 之后是 Starting program、warning 与 bomb-light 的两行说明。末尾的 ... 处，gdb 输出 A debugging session is active.、Inferior 1 [process 3067] will be killed. 与 Quit anyway? (y or n)，回答 y 之后 gdb 退出；终端上没有出现 phase 1 of 1 exploded。
b explode_bomb 的断点在 0xb0bc，explode_bomb（0xb0b8）开头之后 4 个字节处。
这次会话中在断点处执行过 bt：#1 一行是 0x00007789a7e9cadd in phase_light ()，减去装入位置 0x7789a7e92000 得 0xaadd，是 0xaad8 处那条 call 的下一条指令，即第二处比较没有通过。
原课件「Can we do it more efficiently and safely?」一页的四项对应本节的三页现场演示：在关键的路径上设置断点、查看寄存器与内存、单步执行、按需终止程序。原课件的演示是一次会话：b main、r、layout asm、x/s、ni、ni、x/s $rdi、p/x $rdi、b *ADDR、quit；这里分成两次会话，每个演示按钮启动一次 gdb。
""")


def lab_solve_bomb(p):
    p.title("实验 solve-bomb：拆除你的学号对应的 bomb")
    slide(p, """
- **本实验不计分**。每个学号一个 `bomb`，共 8 个阶段，每个阶段读一行输入；
- **`bomb` 运行时连接课程服务器**，每一次爆炸、每一个被拆除的阶段都记在你的学号下；
- **实验材料**在 GitHub 仓库 [SJTU-IPADS/OS-Course-Lab](https://github.com/SJTU-IPADS/OS-Course-Lab/tree/main/ICS-Labs/BombLab) 的目录 `ICS-Labs/BombLab` 中：`Makefile`、`bomb-light`、`README.md` 三个文件，说明在 `README.md` 中。
""")
    p.code("bash", """git clone https://github.com/SJTU-IPADS/OS-Course-Lab.git     # on the host
docker cp OS-Course-Lab/ICS-Labs/BombLab ics:/root/           # on the host
docker exec -it ics bash                                      # the rest runs in the container
apt update && apt install -y make curl gdb binutils
cd /root/BombLab
make bomb ID=525xxxxxxxxx                                     # fetch the bomb of your student ID
./bomb                                                        # 8 phases""")
    slide(p, """
- **完成标准**：答案写在 `answers.txt` 中，每行一个阶段，`./bomb answers.txt` 显示 8 个阶段全部拆除。
""")
    p.notes("""
实验 solve-bomb 不计分。实验材料是三个文件，在 GitHub 仓库 SJTU-IPADS/OS-Course-Lab 的 main 分支的目录 ICS-Labs/BombLab 中；其中的 bomb-light 就是本讲拆除的程序。
代码块的前两行在宿主机上执行：取得仓库，把目录 BombLab 复制进「后台运行容器」一页建立的容器 ics，成为 /root/BombLab。第三行进入容器，其余各行在容器中执行。
make bomb ID=学号 把学号发给课程服务器，服务器生成这个学号的 bomb 并传回；同一个学号每次得到的 bomb 相同。ID= 之后写自己的学号。
2026-10-11 在 ubuntu:26.04 的容器中执行过：用 apt 安装这四个软件包，用测试学号执行 make bomb 并取得 bomb。2026-10-11 仓库中还没有目录 ICS-Labs/BombLab，前两行没有执行过。
bomb 的画面占据整个终端窗口；在 gdb 中运行时要注意的事项见 README.md「在 gdb 中运行时的几件事」一节。
""")
