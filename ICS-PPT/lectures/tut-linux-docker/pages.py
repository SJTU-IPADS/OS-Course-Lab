"""Docker Intro 的页面：原 PDF（ICS-tutorial-1-env）第 N 页对应函数 sNN，保留第 9–13 页。

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
