# Posix 适配

无论我们采用的是什么样的操作系统，如果我们希望能够对上用户态的程序的话，我们都希望其采用同一套的调用规范。相同的在我们开发用户态程序的时候，我们也希望下层的libc提供的接口保持一致，以便于开发者进行移植。而在现代操作系统中`Posix`是一个非常重要的规范。我们都可以在Windows, MacOS, Linux以及其他的衍生系统上找到它的身影，`Posix`针对文件系统提出了一系列的API规范。下面是一个简要的描述


1. mount, umount API：用于文件系统的挂载以及
2. open, close：用于打开以及关闭文件描述符
3. write, read：用于文件的读写
4. mkdir, rmdir, creat, unlink, link, symlink：用于文件以及目录的创建与删除
5. fcntl (byte range locks, etc.)：用于修改文件描述符的具体属性
6. stat, utimes, chmod, chown, chgrp：用于修改文件的属性
7. 所有的文件路径都以及'/'开始

例如当我们在Linux系统中使用`strace`去追踪`cat`指令的系统调用时我们可以得到如下的系统调用序

```bash
openat(AT_FDCWD, "foo", O_RDONLY)       = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=4, ...}) = 0
fadvise64(3, 0, 0, POSIX_FADV_SEQUENTIAL) = 0
mmap(NULL, 139264, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7cb8b6fd3000
read(3, "foo\n", 131072)                = 4
write(1, "foo\n", 4foo
)                    = 4
read(3, "", 131072)                     = 0

```

撇去一些无关紧要的系统调用后，我们可以看到其首先调用了`openat`这个系统指令，其负责打开一个系统路径下的文件，并返回其一个文件描述符号。

> [!CODING] 练习题 1
> 阅读 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/file.c` 的 `chcore_openat` 函数，分析 ChCore 是如何处理 `openat` 系统调用的，关注 IPC 的调用过程以及 IPC 请求的内容。

Lab5 的所有代码都运行在用户态，不同应用间通过 IPC 进行通信，可调用的 IPC 相关函数定义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore/ipc.h`。

如果你感兴趣的话，你也可以继续阅读`read`以及`write`这些文件系统调用的实现，来看看`chcore-libc`是怎么将Chcore文件系统的实现对齐在Posix的API之上的。

---

> [!SUCCESS]
> 以上为Lab5 Part1的所有内容
