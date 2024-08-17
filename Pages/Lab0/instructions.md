# 基本知识

> [!INFO]
> 本部分旨在熟悉 ARM 汇编语言，以及使用 QEMU 和 QEMU/GDB调试

---

<!-- toc -->

## 熟悉Aarch64汇编

AArch64 是 ARMv8 ISA 的 64 位执行状态。《ARM 指令集参考指
南》是一个帮助入门 ARM 语法的手册。在 ChCore 实验中，只
需要在提示下可以理解一些关键汇编和编写简单的汇编代码即可。

## 使用 QEMU 运行炸弹程序

我们在实验中提供了bomb二进制文件，但该文件只能运行在基于 AArch64
的 Linux 中。通过 QEMU，我们可以在其他架构上模拟运行。同时，QEMU
可以结合 GDB 进行调试（如打印输出、单步调试等）

> [!TIP]
> QEMU 不仅可以模拟运行用户态程序，也可以模拟运行在内核态的操作系统。在前一种模式下，QEMU 会模拟运行用户态的汇编代码，同时将系统调用等翻译为对宿主机的调用。在后一种模式下，QEMU 将在虚拟硬件上模拟一整套计算机启动的过程。

在lab0目录下，输入以下命令可以在 QEMU 中运行炸弹程序

```console
[user@localhost Lab0]$ make qemu

```

炸弹程序的标准输出将会显示在 QEMU 中：

```console
Type in your defuse password:

```

## QEMU 与 GDB

在实验中，由于需要在 x86-64 平台上使用 GDB 来调试 AArch64 代
码，因此使用gdb-multiarch代替了普通的gdb。使用 GDB 调试的原理是，
QEMU 可以启动一个 GDB 远程目标（remote target）
（使用-s或-S参数
启动），QEMU 会在真正执行镜像中的指令前等待 GDB 客户端的连接。开
启远程目标之后，可以开启 GDB 进行调试，它会在某个端口上进行监听。

打开两个终端，在bomb-lab目录下，输入make qemu-gdb和make gdb命
令可以分别打开带有 GDB 调试的 QEMU 以及 GDB，在 GDB 中将会看
到如下的输出：

```console
...
0x0000000000400540 in ?? ()
...
(gdb)

```
