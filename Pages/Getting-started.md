# 如何开始实验

<!-- toc -->

## 环境准备

对于所有的操作系统，本实验必须依赖Docker环境且仅能在Linux系统上进行运行(我们不支持Mac OS系统)，
请按照Docker官方指示为你运行的操作系统安装对应的Docker发行版。

> [!IMPORTANT] 关于Docker
> 由于中国大陆地区的网络限制，请确保你的docker能够连接到DockerHub，测试方法可以使用 `docker pull nginx:latest`，如果无法访问，您可以依照[该文档](https://docs.docker.com/engine/daemon/)为你的docker daemon添加代理规则。如果你缺少代理，你可以使用这个以下的几个链接：  [百度云(提取uwuv)](https://pan.baidu.com/s/1ipbMZ-C1Qk0S9PGDDMMy6w) [交大云(仅上海交通大学内部可以访问)](https://jbox.sjtu.edu.cn/l/l1Fe9X)。  
> 下载压缩好的Docker镜像，镜像使用`gzip`解压缩镜像文件到标准输出流之后再由docker进行导入（下方的yy.mm请以当前的大版本号替换，如版本tag为24.09.1则请将yy.mm替换成24.09）。
>
> ```
> gzip -cd docker.ipads.oslab.yy.mm.tar.gz | docker load
> ```

> [!IMPORTANT] 关于虚拟机
> 如果你使用的是Windows/MacOS系统，如果不想手动安装docker以及下载镜像，我们也准备了基于VMWare 17的虚拟机镜像，你也可以使用上方的链接下载vmware虚拟机镜像，你可以在解压之后导入vmware即可使用。  
> 用户stu 密码为123456


> [!IMPORTANT] CRLF
> 由于bash/GNU Make强制需要使用LF作为换行符，当你尝试保存脚本时请确保换行符为LF，而非CRLF。


> [!WARNING]
> 小心使用sudo，请注意我们在编译的时候我们会同步的在编译文件夹时写入timestamp文件，如果你使用root权限，会导致后续普通的正常构建返回错误。

### 使用Dev-Container (推荐)

如果你使用的是带有支持Microsoft规范下[Dev-Container](https://vscode.github.net.cn/docs/devcontainers/tutorial)插件的代码编辑器或者集成开发环境，亦或者你使用的是非Linux平台的开发环境，我们**强烈建议**你使用Dev-container直接进行开发，我们已经在其中已经预先安装好了你可能需要使用的所有工具链。并且针对vscode我们在每个Lab的分支目录下都已经配置好了合适的插件配置，简单安装即可以一键启用。安装完之后进入本实验的**根目录**，此时dev-container会识别到容器开发环境，重新进入后就可以直接使用了。

> [!BUG] 关于Windows  
> 在MacOS平台，使用dev-container能够保证兼容性。但在Windows上面由于文件系统并非POSIX兼容，你需要执行以下的方式来正确拉取repo.
>
> 1. 请在设置确保打开Windows 10/11 的开发者模式
> 2. 运行git config --global core.autocrlf false
> 3. 使用git clone -c core.symlinks true 进行clone
> 4. 在完成lab3-5时请将`user/chcore-libc/libchcore/cmake/do_override_dir.sh`中所有的`ln -sf`替换成`cp -r`
> 5. 对于所有`libchcore`下的所有文件，请都运行`make clean`确保其生效

### 本地手动安装

如果你并不想要使用Dev-container，且你使用的是原生Linux平台或者自行创建的Linux虚拟机，我们在下方准备了所有流行发行版的工具链安装命令。同时我们也推荐安装`clangd`，其可以根据`CMake`生成的`compile_commands.json`即编译信息数据库提供增强的LSP的提示功能，用于支撑区分相同函数签名但是不同编译单元参与编译的重名跳转以及根据宏定义用于高显源文件。

以下是本Lab最主要的依赖

- Qemu-system-aarch64
- Qemu-user
- Python >= 3.7
- Make

以下是针对每个主流发行版的除去Docker以外的安装命令。

- Ubuntu/Debian

```console
apt install qemu-system-aarch64 qemu-user python3 python3-pip python3-pexpect make gdb-multiarch

```

- Fedora

```console
dnf install qemu-system qemu-user python3 python3-pip python3-pexpect make gdb

```

- Arch Linux

```console
pacman -Sy qemu-user qemu-system-aarch64 python python-pip python-pexpect base-devel gdb

```

- Gentoo

```console
USE="qemu_mmu_targets_aarch64 qemu_user_targets_aarch64" emerge -1v qemu python dev-python/pexpect python-pip make gdb qemu

```

- OpenSuse Tumbleweed

```console
zypper install qemu qemu-linux-user python3 python3-pexpect make gdb

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

在新实验中，我们特意准备了支持github actions以及gitlab ci的CI配置，你可以在你所使用的代码托管平台上自动运行脚本，为了确保你不会篡改预编译的`.obj`文件，我们会根据本仓库的主线的各个Lab中的`filelist.mk`(详细见贡献指南)的定义自动提取提交中所需要的文件与当前仓库主线的文件树进行合并，最终进行评分。
