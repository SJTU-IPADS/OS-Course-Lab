# Lab 4：多核调度与IPC

在本实验中，我们将逐步实现ChCore的多核支持以及微内核系统的核心：进程间通信，本Lab包含四个部分：]

1. [多核启动支持](./Lab4/multicore.html): 使ChCore通过树莓派厂商所提供的固件唤醒多核执行
2. [多核调度](./Lab4/scheduler.html): 使ChCore实现在多核上进行[round-robin](https://en.wikipedia.org/wiki/Round-robin_scheduling)调度。
3. [IPC](./Lab4/IPC.html): 使ChCore支持进程间通信
4. [IPC调优](./Lab4/performance.html): 为ChCore的IPC针对测试的特点进行调优。

跟先前的Lab相同，本实验代码包含了基础的 ChCore 操作系统镜像，除了练习题相关部分的源码以外（指明需要阅读的代码），其余部分通过二进制格式提供。
在正确完成本实验的练习题之后，你可以在树莓派3B+QEMU或开发板上进入 ChCore shell。
注释`/* LAB 4 TODO BEGIN (exercise #) */`和`/* LAB 4 TODO END (exercise #) */`之间代表需要填空的代码部分。
