# 线程生命周期管理

本实验的 OS 运行在 AArch64 体系结构，该体系结构采用“异常级别”这一概念定义程序执行时所拥有的特权级别。从低到高分别是 EL0、EL1、EL2 和 EL3。每个异常级别的具体含义和常见用法已在课程中讲述。

ChCore 中仅使用了其中的两个异常级别：EL0 和 EL1。其中，EL1 是内核模式，`kernel` 目录下的内核代码运行于此异常级别。EL0 是用户模式，`user` 目录下的用户库与用户程序代码运行在用户模式下。我们在之前的RTFSC中提到了，在Chcore中内核对用户态提供的所有的资源，如Lab2的内存对象，都围绕着cap_group以及capability展开。同目前所有的主流操作系统一样，ChCore 中的每个进程至少包含一个主线程，也可能有多个子线程，而每个线程则从属且仅从属于一个进程。在 ChCore 中，第一个被创建的进程是 `procmgr`，是 ChCore 核心的系统服务。本实验将以创建 `procmgr` 为例探索在 ChCore 中如何创建进程，以及成功创建第一个进程后如何实现内核态向用户态的切换。

在 ChCore 中，第一个被创建的进程是 `procmgr`，是 ChCore 核心的系统服务。本实验将以创建 `procmgr` 为例探索在 ChCore 中如何创建进程，以及成功创建第一个进程后如何实现内核态向用户态的切换。

## 权利组创建

创建用户程序至少需要包括创建对应的 `cap_group`、加载用户程序镜像并且切换到程序。在内核完成必要的初始化之后，内核将会跳转到创建第一个用户程序的操作中，该操作通过调用 `create_root_thread` 函数完成，本函数完成第一个用户进程的创建，其中的操作包括从`procmgr`镜像中读取程序信息，调用`create_root_cap_group`创建第一个 `cap_group` 进程，并在 `root_cap_group` 中创建第一个线程，线程加载着信息中记录的 elf 程序（实际上就是`procmgr`系统服务）。此外，用户程序也可以通过 `sys_create_cap_group` 系统调用创建一个全新的 `cap_group`。

> [!CODING] 练习题1
> 在 `kernel/object/cap_group.c` 中完善 `sys_create_cap_group`、`create_root_cap_group` 函数。在完成填写之后，你可以通过 Cap create pretest 测试点。

> [!HINT]
> 可以阅读 `kernel/object/capability.c` 中各个与 cap 机制相关的函数以及参考文档。

## ELF加载

然而，完成 `cap_group` 的分配之后，用户程序并没有办法直接运行，因为`cap_group`只是一个资源集合的概念。线程才是内核中的调度执行单位，因此还需要进行线程的创建，将用户程序 ELF 的各程序段加载到内存中。(此为内核中 ELF 程序加载过程，用户态进行 ELF 程序解析可参考`user/system-services/system-servers/procmgr/libs/libchcoreelf/libchcoreelf.c`，如何加载程序可以对`user/system-services/system-servers/procmgr/srvmgr.c`中的`procmgr_launch_process`函数进行详细分析)

> [!CODING] 练习题2
> 在 `kernel/object/thread.c` 中完成 `create_root_thread` 函数，将用户程序 ELF 加载到刚刚创建的进程地址空间中。

> [!HINT]
>
> - 程序头可以参考`kernel/object/thread_env.h`。
> - 内存分配操作使用 `create_pmo`，可详细阅读`kernel/object/memory.c`了解内存分配。
> - 本练习并无测试点，请确保对 elf 文件内容读取及内存分配正确。否则有可能在后续切换至用户态程序运行时出错。

## 进程调度

完成用户程序的内存分配后，用户程序代码实际上已经被映射在`root_cap_group`的虚拟地址空间中。接下来需要对创建的线程进行初始化，以做好从内核态切换到用户态线程的准备。

> [!CODING] 练习题3
> 在 `kernel/arch/aarch64/sched/context.c` 中完成 `init_thread_ctx` 函数，完成线程上下文的初始化。

至此，我们完成了第一个用户进程与第一个用户线程的创建。接下来就可以从内核态向用户态进行跳转了。
回到`kernel/arch/aarch64/main.c`，在`create_root_thread()`完成后，分别调用了`sched()`与`eret_to_thread(switch_context())`。
`sched()`的作用是进行一次调度，在此场景下我们创建的第一个线程将被选择。

`switch_context()`函数的作用则是进行线程上下文的切换，包括vmspace、fpu、tls等。并且将`cpu_info`中记录的当前CPU线程上下文记录为被选择线程的上下文（完成后续实验后对此可以有更深的理解）。`switch_context()` 最终返回被选择线程的`thread_ctx`地址，即`target_thread->thread_ctx`。

`eret_to_thread`最终调用了`kernel/arch/aarch64/irq/irq_entry.S`中的 `__eret_to_thread` 函数。其接收参数为`target_thread->thread_ctx`，将 `target_thread->thread_ctx` 写入`sp`寄存器后调用了 `exception_exit` 函数，`exception_exit` 最终调用 eret 返回用户态，从而完成了从内核态向用户态的第一次切换。

注意此处因为尚未完成`exception_exit`函数，因此无法正确切换到用户态程序，在后续完成`exception_exit`后，可以通过 gdb 追踪 pc 寄存器的方式查看是否正确完成内核态向用户态的切换。

> [!QUESTION] 思考题4
> 思考内核从完成必要的初始化到第一次切换到用户态程序的过程是怎么样的？尝试描述一下调用关系。

> [!BUG] 无法继续执行
> 然而，目前任何一个用户程序并不能正常退出，也不能正常输出结果。这是由于程序中包括了 `svc #0` 指令进行系统调用。由于此时 ChCore 尚未配置从用户模式（EL0）切换到内核模式（EL1）的相关内容，在尝试执行 `svc` 指令时，ChCore 将根据目前的配置（尚未初始化，异常处理向量指向随机位置）执行位于随机位置的异常处理代码，进而导致触发错误指令异常。同样的，由于错误指令异常仍未指定处理代码的位置，对该异常进行处理会再次出发错误指令异常。ChCore 将不断重复此循环，并最终表现为 QEMU 不响应。后续的练习中将会通过正确配置异常向量表的方式，对这一问题进行修复。

---

> [!SUCCESS]
> 以上为Lab3 Part1 的所有内容，完成后你将获得20分
