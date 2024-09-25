# Lab 5：虚拟文件系统

虚拟文件系统（Virtual File System，VFS）提供了一个抽象层，使得不同类型的文件系统可以在应用程序层面以统一的方式进行访问。这个抽象层隐藏了不同文件系统之间的差异，使得应用程序和系统内核可以以一致的方式访问各种不同类型的文件系统，如 ext4、tmpfs、 FAT32 等。在 ChCore 中，我们通过 FSM 系统服务以及 FS_Base 文件系统 wrapper 将不同的文件系统整合起来，给运行在 ChCore 上的应用提供了统一的抽象。

> [!CODING] 练习1
> 阅读 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/file.c` 的 `chcore_openat` 函数，分析 ChCore 是如何处理 `openat` 系统调用的，关注 IPC 的调用过程以及 IPC 请求的内容。

Lab5 的所有代码都运行在用户态，不同应用间通过 IPC 进行通信，可调用的 IPC 相关函数定义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore/ipc.h`。
