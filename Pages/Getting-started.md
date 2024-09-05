# 如何开始实验

<!-- toc -->

## 环境准备

对于所有的操作系统，本实验必须依赖Docker环境，请按照Docker官方指示为你运行的操作系统安装对应的Docker发行版。

> [!IMPORTANT] 关于Docker
> 由于中国大陆地区的网络限制，请确保你的docker能够连接到docker-hub，测试方法可以使用 `docker pull nginx:latest`，如果无法访问，您可以依照[该文档](https://docs.docker.com/engine/daemon/)为你的docker daemon添加代理规则。

### 使用Dev-Container (推荐)

如果你使用的是带有支持Microsoft规范下Dev-Container插件的代码编辑器或者集成开发环境，亦或者你使用的是非Linux平台的开发环境，我们强烈建议你使用Dev-container直接进行开发，我们已经在其中已经预先安装好了你可能需要使用的所有工具链。并且针对vscode我们在每个Lab的分支目录下都已经配置好了合适的插件配置，简单安装即可以一键启用。

### 本地手动安装(推荐)

如果你并不想要使用Dev-container，且你使用的是原生Linux平台或者自行创建的Linux虚拟机，我们在下方准备了所有流行发行版的工具链安装命令。同时我们也推荐安装`clangd`，其可以根据`CMake`生成的`compile_commands.json`即编译信息数据库提供增强的LSP的提示功能，用于支撑区分相同函数签名但是不同编译单元参与编译的重名跳转以及根据宏定义用于高显源文件。

以下是本Lab最主要的依赖

- Qemu-system-aarch64
- Qemu-user
- Python >= 3.7
- Make

以下是针对每个主流发行版的除去Docker以外的安装命令。

- Ubuntu/Debian

```console
apt install qemu-system-aarch64 qemu-user python3 python3-pip python3-psutil make gdb-multiarch

```

- Fedora

```console
dnf install qemu-system qemu-user python3 python3-pip python3-psutil make gdb

```

- Arch Linux

```console
pacman -Sy qemu-user qemu-system-aarch64 python python-pip python-psutil base-devel gdb

```

- Gentoo

```console
USE="qemu_mmu_targets_aarch64 qemu_user_targets_aarch64" emerge -1v qemu python dev-python/psutil python-pip make gdb qemu

```

- OpenSuse Tumbleweed

```console
zypper install qemu qemu-linux-user python3 python3-psutil make gdb

```

## 文档说明

各实验文档除开`lab0`为单独的实验内容，其他都包含了以下的几种习题，请根据下方的指示完成对应实验报告以及对应的编程题。

> [!QUESTION] 思考题
> 思考题为需要在实验报告中书面回答的问题，需要用文字或者是图片来描述

> [!CODING] 练习题
> 练习题需在 ChCore 代码中填空，并在实验报告阐述实现过程，完成即可获得实验大多数的分数。

> [!CHALLENGE] 挑战题
> 挑战题为难度稍高的练习题，作为实验的附加题用于加深你对代码结构以及系统设计的理解。

## CI评分

在新实验中，我们特意准备了支持github actions以及gitlab ci的CI配置，你可以在你所使用的代码托管平台上自动运行脚本，为了确保你不会篡改预编译的`.obj`文件，我们会根据本仓库的主线的各个Lab中的`filelists.mk`(详细见贡献指南)的定义自动提取提交中所需要的文件与仓库主线的文件树进行合并，最终进行评分。
