# ICS 辅导课 · Docker Intro

介绍操作系统、Linux、虚拟机虚拟化与容器虚拟化，结论是 ICS 课程用 docker 在 Mac 或 Windows 上运行
ubuntu 容器；最后一节给出 docker 的下载地址、拉取 x86-64 的 ubuntu 镜像的命令与运行容器的命令。

本讲由 [`../ICS1/ICS-tutorial-1-env/`](../ICS1/ICS-tutorial-1-env/) 复制后裁剪而来，保留
LINUX 一节，即原课件 `refs/ppts/ICS1/ICS-tutorial-1-env.pdf` 的第 8–13 页。
封面与课程其他各讲（如 `3-asm`）的写法相同，只有标题不同。
讲义 id 是 `tut-linux-docker`，与目录名相同。
「安装 docker，运行 ubuntu 容器」一节的 4 张幻灯片是 2026-10-09 新写的，不在原课件中。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。第 8–13 页的原文是英文，
`en.toml` 里逐字保留原文，Python 里是中文译文；封面的作者与单位是中文原文，`en.toml` 里是英文译文。

## 结构

| 幻灯片 | id | 标题 | 内容 |
| --- | --- | --- | --- |
| 1 | `cover` | Docker Intro | 封面：标题、作者（古金宇 · 陈榕）、单位（上海交通大学 IPADS） |
| 2 | `s08` | LINUX | 分节页 |
| 3 | `s09` | 操作系统 | 操作系统位于应用程序与硬件之间，为应用程序提供服务并管理硬件 |
| 4 | `s10` | Linux | Linux 发行版的标志 |
| 5 | `s11` | 虚拟机虚拟化 | 虚拟机、虚拟机监控器，虚拟机的层次图 |
| 6 | `s12` | 容器虚拟化 | 虚拟机与容器的层次对照图 |
| 7 | `s13` | 容器虚拟化 | docker 的客户端、宿主机与镜像仓库 |
| 8 | `bridge-docker` | 安装 docker，运行 ubuntu 容器 | 分节页 |
| 9 | `docker-install` | 安装 docker：下载地址 | 三行的表（macOS、Windows、Linux 各自安装的软件与 Docker 官方文档的安装页）；三条说明；演示 `docker --version` |
| 10 | `docker-pull` | 拉取镜像：x86-64 的 ubuntu 26.04 | 镜像、`--platform linux/amd64`、Apple 芯片的 Mac 三条说明；演示 `docker pull --platform linux/amd64 ubuntu:26.04` 与查看镜像架构的命令 |
| 11 | `docker-run` | 运行容器：在 ubuntu 容器中启动 bash | 交互式演示 `docker run -it --platform linux/amd64 ubuntu:26.04 bash`；四条说明（`docker run`、`-it`、提示符、`exit` 之后再次进入） |

共 11 张幻灯片：1 个封面、2 个分节页、5 个原课件的编号页、3 个新写的页。第 6、7 张标题相同，在大纲里合为一行。

## 目录

| 路径 | 内容 |
| --- | --- |
| `lecture.py` | 页序：封面、LINUX 一节、「安装 docker，运行 ubuntu 容器」一节 |
| `pages.py` | 每页一个函数，原课件第 N 页是 `sNN`，新写的页是 `docker_install`、`docker_pull`、`docker_run` |
| `assets/` | 插图，文件名以所在页的 `sNNN` 开头 |
| `i18n/en.toml` | 英文覆盖层 |

## 插图

插图都取自原课件，是位图，本讲没有 `diagrams/`。图里的文字是英文，两种语言共用一张图。

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

**封面**：`lecture.cover(标题, author="古金宇 · 陈榕", time="上海交通大学 IPADS")`，不带 `id` 和 `logo`，与课程其他各讲一致。

**用语**：本课件用于本科课程教学，一律采用陈述性的技术表述。不使用比喻、口语化措辞，
以及「不是……而是……」「是 A，不是 B」一类对比句式；结论写成可以直接复述的判断句。

**保留的各页**沿用 [`../ICS1/README.md`](../ICS1/README.md)「转换约定」一节：

- **页面 id**：原课件第 N 页的 id 是 `sNN`，裁剪后 id 不重排，从 `s08` 开始。
- **标题与正文**：照原页翻译，保留原文的层级。原版式把正文的一级条目设为加粗，这里照样写成加粗。
- **自动加粗**：`pages.py` 的 `slide(...)` 以 `autobold=False` 调用 `p.slide(...)`，加粗全部手写。
- **分节页**：只有一行标题的原页用 `lecture.bridge(...)`，其后各页放进同名的 `section`。

**新写的三页**（`docker-install`、`docker-pull`、`docker-run`）：

- **页面 id** 用内容命名，不用 `sNN`；标题写这一页的内容，正文的写法与课程其他各讲相同（`**词**：说明`）。
- **命令都用 `p.demo`**，`output=` 是 2026-10-09 在本机（Docker 29.8.2，x86-64 的 Ubuntu 26.04）执行这条命令得到的输出。`docker-pull` 的输出是第一次拉取结束时终端上留下的内容，下载过程中每一层的进度行没有列出；镜像已在本机时输出是 `Status: Image is up to date for ubuntu:26.04`。
- **`docker-run` 是交互式演示**（`interactive=True, timeout=0`）：演示按钮打开一个终端，输入的每一行由容器中的 bash 执行；页面上的输出是输入 `uname -m`、`head -1 /etc/os-release`、`exit` 三行的一次运行。每按一次按钮创建一个新的容器，`exit` 之后它处于停止状态，用 `docker rm` 删除。
- **镜像固定为 `ubuntu:26.04`**，与讲义各页输出所用的系统相同；命令中的 `--platform linux/amd64` 使 Apple 芯片的 Mac 也得到 x86-64 的镜像。
- **下载地址**是 Docker 官方文档的三个安装页，表中显示不带 `https://` 的地址，点击打开；2026-10-09 核对过三个地址都能打开。

**排版**：

- 粗体后面的全角冒号写在 `**` 之外：`**词**：内容`；
- `==标记==` 只在 `p.slide(...)` 里展开；
- 一页最多一个 `p.highlight`；
- 修改之后渲染 PNG 逐页查看，1280×720 下内容不越过 y=700。

**英文**：中文改动之后同步 `i18n/en.toml`，`i18n check` 应报告 0 missing、0 changed、0 orphaned。

## 渲染

```bash
python3 -m lecturekit.cli render lectures/tut-linux-docker --pdf
```

```bash
python3 -m lecturekit.cli render lectures/tut-linux-docker --lang en --strict --pdf
```

输出在 `build/tut-linux-docker-viewer/`，PDF 的文件名由标题得到，是 `docker-intro.pdf`。

```bash
python3 -m lecturekit.cli i18n check lectures/tut-linux-docker --lang en
```
