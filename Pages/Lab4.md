# Lab 4：多核调度与IPC

在本实验中，ChCore将支持在多核处理器上启动（第一部分）；实现多核调度器以调度执行多个线程（第二部分）；最后实现进程间通信IPC（第三部分）。注释`/* LAB 4 TODO BEGIN (exercise #) */`和`/* LAB 4 TODO END (exercise #) */`之间代表需要填空的代码部分。

## Preparation

实验 4 与实验 3相同，需要在根目录下拉取 `musl-libc` 代码。

>```bash
> git submodule update --init --recursive
>```

使用 `make build` 检查是否能够正确项目编译。

> [!WARNING]
> 请确保成功拉取`musl-libc`代码后再尝试进行编译。若未成功拉取`musl-libc`就进行编译将会自动创建`musl-libc`文件夹，这可能导致无法成功拉取`musl-libc`代码，也无法编译成功。出现此种情况可以尝试将`user/chcore-libc/musl-libc`文件夹删除，并运行以上指令重新拉取代码。
