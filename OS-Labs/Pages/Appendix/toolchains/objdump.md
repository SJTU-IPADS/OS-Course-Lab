# Objdump

<!-- toc -->

## 简介

objdump是GNU binutils中的标准工具之一。它的主要作用是显示二进制程序的相关信息，包括可执行
文件、静态库、动态库等各种二进制文件。因此，它在逆向工程和操作系统开发等底层领域中非常常
用，因为在这些领域中，我们所接触到的程序很可能不提供源代码，只有二进制可执行文件；抑或是所
面临的问题在高级语言的抽象中是不存在的，只有深入到机器指令的层次才能定位和解决。在这些领域
中，objdump最常见的应用是反汇编，即将可执行文件中的二进制指令，根据目标架构的指令编码规
则，还原成文本形式的汇编指令，以供用户从汇编层面分析和理解程序。例如，在拆弹实验中，我们只
提供了炸弹程序的极少一部分源代码，同学们需要通过objdump等工具进行逆向工程，从汇编层级理解
炸弹程序的行为，进而完成实验。

## GNU实现 vs LLVM实现

objdump最初作为GNU binutils的一部分发布的。由于它直接处理二进制指令，因此它的功能与CPU架
构等因素强相关。在常见Linux发行版中，通过包管理安装的objdump均为GNU binutils提供的实现，
且它一般被编译为用于处理当前CPU架构的二进制指令。例如，在x86_64 CPU上运行的Linux中安装
objdump，它一般只能处理编译到x86_64架构的二进制文件。对于GNU binutils提供的实现，如果需要
处理非当前CPU架构的二进制可执行文件，则需要额外安装为其他架构编译的objdump，例如，如果需
要在x86_64 CPU上处理aarch64架构的二进制文件，一般需要使用 aarch64-linux-gnu-objdump 。
LLVM是另一个被广泛应用的模块化编译器/工具链项目集合。LLVM项目不仅提供了另一个非常常见的
C/C++编译器 clang ，也提供了一组自己的工具链实现，对应GNU binutils中的相关工具。例如，LLVM
也提供了自己的objdump实现，在许多Linux发行版上，你需要安装和使用 llvm-objdump 命令。LLVM
所提供的工具链实现与GNU binutils中的相应工具是基本兼容的，这意味着它们的命令行参数/语法等是
相同的，但LLVM工具链也提供了其他扩展功能。从用户的角度来说，LLVM工具链与GNU binutils的一
个主要区别是LLVM工具链不需要为每个能处理的架构编译一个单独的版本，以 llvm-objdump 为例，不
论处理x86_64还是aarch64架构中的二进制文件，均可以使用 llvm-objdump 命令（需要在编译 llvm-
objdump 时开启相应的支持，从发行版包管理中安装的版本通常有包含）。

## 可执行文件格式

为了更好地加载和执行程序，操作系统在一定程度上需要了解程序的内部结构和一系列元数据。因此，
一个二进制可执行文件并不仅仅是将所有指令和数据按顺序写到文件中即可，通常它需要以一种特定的
可执行文件格式存储，使用的格式是由它将要运行在的操作系统决定的。例如，对于Unix/Linux操作系
统，ELF格式是目前最主流的可执行文件格式。而Windows下主要的可执行文件格式是PE/COFF。除了
反汇编之外，objdump还可以显示与可执行文件格式相关的众多信息，但需要用户对可执行文件格式自
身的理解。详细分析可执行文件格式超出了本教程的范围，感兴趣的同学建议查阅与ELF相关的资料。在
后续ChCore实验中，也会涉及部分关于ELF的知识。

## objdump使用

本节只介绍拆弹实验中可能用到的基础用法。详细用法感兴趣的同学可以参考扩展阅读部分。

- `objdump -dS a.out a.S` : 反汇编`a.out`中的可执行section（不含数据
sections），并保存到输出文件`a.S`中。在可能的情况下（如有调试信息和源文件），还会输出汇编指
令所对应的源代码。

- `objdump -dsS a.out a.S` : 将`a.out` sections的内容全部导出，但
仍然只反汇编可执行sections，且在可能情况下输出源代码。

## 扩展阅读

- [objdump手册](https://man7.org/linux/man-pages/man1/objdump.1.html)
- <https://en.wikipedia.org/wiki/Executable_and_Linkable_Format>
- <https://maskray.me/blog/>
- <https://fourstring.github.io/ya-elf-guide/guide_zh>
