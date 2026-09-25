# Lab 5：虚拟文件系统

虚拟文件系统（Virtual File System，VFS）提供了一个抽象层，使得不同类型的文件系统可以在应用程序层面以统一的方式进行访问。这个抽象层隐藏了不同文件系统之间的差异，使得应用程序和系统内核可以以一致的方式访问各种不同类型的文件系统，如 ext4、tmpfs、 FAT32 等。在 ChCore 中，我们通过 FSM 系统服务以及 FS_Base 文件系统 wrapper 将不同的文件系统整合起来，给运行在 ChCore 上的应用提供了统一的抽象。

本Lab一共分为四个部分：

1. [Posix适配](./Lab5/posix.html)：分析ChCore是如何实现兼容posix的文件接口的。
2. [FSM](./Lab5/FSM.html)：FSM是ChCore的虚拟文件系统的实现层，其主要负责页缓存，挂载点管理，以及路径对接。我们在此部分实现这一文件系统转发层。
3. [FS_Base](./Lab5/base.html): FS_Base是文件系统实现层，由于在微内核系统中文件系统实际由一个个进程实现，所以我们统一包装标准的文件操作到通用库即为FS_Base，我们需要在这一个部分实现它。
4. [Boweraccess](./Lab5/bower.md): FS_Page_Fault是文件系统PageFault的实现，本Lab需要同学们根据一个具体的任务使用页预取出来优化PageFault的触发。

跟先前的Lab相同，本实验代码包含了基础的 ChCore 操作系统镜像，除了练习题相关部分的源码以外（指明需要阅读的代码），其余部分通过二进制格式提供。
在正确完成本实验的练习题之后，你可以在树莓派3B+QEMU或开发板上进入 ChCore shell。与之前的Lab不同的地方是，本Lab不涉及任何内核态的代码编写，你需要将所有的目光聚焦在`user`这个目录下面的文件。注释`/* LAB 5 TODO BEGIN (exercise #) */`和`/* LAB 5 TODO END (exercise #) */`之间代表需要填空的代码部分。
