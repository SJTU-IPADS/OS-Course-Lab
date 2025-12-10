# 系统调用

## 内核支持

系统调用是系统为用户程序提供的高特权操作接口。在本实验中，用户程序通过 `svc` 指令进入内核模式。在内核模式下，首先操作系统代码和硬件将保存用户程序的状态。操作系统根据系统调用号码执行相应的系统调用处理代码，完成系统调用的实际功能，并保存返回值。最后，操作系统和硬件将恢复用户程序的状态，将系统调用的返回值返回给用户程序，继续用户程序的执行。

通过异常进入到内核后，需要保存当前线程的各个寄存器值，以便从内核态返回用户态时进行恢复。保存工作在`exception_enter` 中进行，恢复工作则由`exception_exit`完成。可以参考`kernel/include/arch/aarch64/arch/machine/register.h`中的寄存器结构，保存时在栈中应准备`ARCH_EXEC_CONT_SIZE`大小的空间。

完成保存后，需要进行内核栈切换，首先从`TPIDR_EL1`寄存器中读取到当前核的`per_cpu_info`（参考`kernel/include/arch/aarch64/arch/machine/smp.h`）,从而拿到其中的`cpu_stack`地址。

> [!CODING] 练习题6
> 填写 `kernel/arch/aarch64/irq/irq_entry.S` 中的 `exception_enter` 与 `exception_exit`，实现上下文保存的功能，以及 `switch_to_cpu_stack` 内核栈切换函数。如果正确完成这一部分，可以通过 Userland 测试点。这代表着程序已经可以在用户态与内核态间进行正确切换。显示如下结果
>
> ```
> Hello userland!
> ```

## 用户态libc支持

在本实验中新加入了 `libc` 文件，用户态程序可以链接其编译生成的`libc.so`,并通过 `libc` 进行系统调用从而进行向内核态的异常切换。在实验提供的 `libc` 中，尚未实现 `printf` 的系统调用，因此用户态程序无法进行正常输出。实验接下来将对 `printf` 函数的调用链进行分析与探索。

`printf` 函数调用了 `vfprintf`，其中文件描述符参数为 `stdout`。这说明在 `vfprintf` 中将使用 `stdout` 的某些操作函数。

在 `../Thirdparty/musl-libc/src/stdio/stdout.c`中可以看到 `stdout` 的 `write` 操作被定义为 `__stdout_write`，之后调用到 `__stdio_write` 函数。

最终 `printf` 函数将调用到 `chcore_stdout_write`。

> [!QUESTION] 思考题7
> 尝试描述 `printf` 如何调用到 `chcore_stdout_write` 函数。

> [!HINT]
> `chcore_write` 中使用了文件描述符，`stdout` 描述符的设置在`user/chcore-libc/libchcore/porting/overrides/src/chcore-port/syscall_dispatcher.c` 中。

`chcore_stdout_write` 中的核心函数为 `put`，此函数的作用是向终端输出一个字符串。

> [!CODING] 练习题8:
> 在其中添加一行以完成系统调用，目标调用函数为内核中的 `sys_putstr`。使用 `chcore_syscallx` 函数进行系统调用。

至此，我们完成了对 `printf` 函数的分析及完善。从 `printf` 的例子我们也可以看到从通用 api 向系统相关 abi 的调用过程，并最终通过系统调用完成从用户态向内核态的异常切换。

---
> [!SUCCESS]
> 以上为Lab3 Part3的所有内容，完成后你可以获得80分
