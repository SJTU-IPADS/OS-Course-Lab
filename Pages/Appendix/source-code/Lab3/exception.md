本部分内容讲解ChCore异常管理的部分

# 回顾：ARM异常ARM ABI

## ARM 异常分类

这部分内容在缺页管理中首次提到

ARM将异常分为同步异常和异步异常两大类。同步异常是由指令执行直接引发的，例如系统调用、页面错误或非法指令等，这类异常具有**确定性**，每次执行到特定指令时都会触发。而异步异常包括硬件中断（IRQ）、快速中断（FIQ）和错误（ERROR），它们与当前指令无关，通常由**外部事件或硬件故障**引起

- **sync**: 同步异常，如系统调用或页面错误。
- **irq**: 硬件中断请求（IRQ），由外部设备生成的中断。
- **fiq**: 快速中断请求（FIQ），用于更高优先级的中断处理。
- **error**: 处理其他类型的错误，如未定义指令或故障。

## ARM C ABI

### **参数传递规则**

- **寄存器传递**

    前六个整型或指针参数（32/64位）依次通过寄存器x0-x5传递。例如：

  - 第1个参数 → x0
  - 第2个参数 → x1
  - ...
  - 第6个参数 → x5
- **栈传递**

    若参数超过六个，剩余参数按声明顺序从右向左压入栈空间，由调用者分配和释放。


### **返回值传递规则**

- **小型返回值（≤8字节）**

    单个整型、指针或小结构体（≤64位）直接通过x0寄存器返回。

- **中型结构体（≤16字节）**

    通过x0寄存器返回指向该结构体的内存指针，内存由调用者预分配（例如在栈上）。

- **大型结构体（>16字节）**

    调用者需提前在栈中分配内存，并将该内存地址写入x8寄存器。被调用函数通过x8找到目标地址，直接将结构体内容写入此内存区域，同时x0也会返回该地址。


# ChCore异常处理

现在我们再来看ChCore对异常处理的实际实现

Lab文档提到：

> 在 AArch64 中，存储于内存之中的异常处理程序代码被叫做异常向量（exception vector），而所有的异常向量被存储在一张异常向量表（exception vector table）中。
>
>
> AArch64 中的每个异常级别都有其自己独立的异常向量表，其虚拟地址由该异常级别下的异常向量基地址寄存器（`VBAR_EL3`，`VBAR_EL2` 和 `VBAR_EL1`）决定。每个异常向量表中包含 16 个条目，每个条目里存储着发生对应异常时所需执行的异常处理程序代码。
>

由于ChCore仅使用了EL0和EL1两个异常级别，故异常向量表也只有EL1这一张

我们还是先看看源码的实现，再逐步分析

## 源码

重复或者不重要的部分已省略

```nasm
// ......
.macro exception_entry label
 /* Each entry of the exeception table should be 0x80 aligned */
 .align 7
 b \label
.endm

/* See more details about the bias in registers.h */
.macro exception_enter
 sub sp, sp, #ARCH_EXEC_CONT_SIZE
 stp x0, x1, [sp, #16 * 0]
 stp x2, x3, [sp, #16 * 1]
 stp x4, x5, [sp, #16 * 2]
 stp x6, x7, [sp, #16 * 3]
 stp x8, x9, [sp, #16 * 4]
 stp x10, x11, [sp, #16 * 5]
 stp x12, x13, [sp, #16 * 6]
 stp x14, x15, [sp, #16 * 7]
  // ...
.endm

.macro exception_exit
 ldp x22, x23, [sp, #16 * 16]
 ldp x30, x21, [sp, #16 * 15] 
 msr sp_el0, x21
 msr elr_el1, x22
 msr spsr_el1, x23
 ldp x0, x1, [sp, #16 * 0]
 ldp x2, x3, [sp, #16 * 1]
 ldp x4, x5, [sp, #16 * 2]
 ldp x6, x7, [sp, #16 * 3]
 ldp x8, x9, [sp, #16 * 4]
  // ...
 add sp, sp, #ARCH_EXEC_CONT_SIZE
 eret
.endm

.macro switch_to_cpu_stack
 mrs     x24, TPIDR_EL1
 add x24, x24, #OFFSET_LOCAL_CPU_STACK
 ldr x24, [x24]
 mov sp, x24
.endm

.macro switch_to_thread_ctx
 mrs     x24, TPIDR_EL1
 add x24, x24, #OFFSET_CURRENT_EXEC_CTX
 ldr x24, [x24]
 mov sp, x24
.endm

/* el1_vector should be set in VBAR_EL1. The last 11 bits of VBAR_EL1 are reserved. */
.align 11
EXPORT(el1_vector)
 exception_entry sync_el1t  // Synchronous EL1t
 exception_entry irq_el1t  // IRQ EL1t
 exception_entry fiq_el1t  // FIQ EL1t
 exception_entry error_el1t  // Error EL1t

 exception_entry sync_el1h  // Synchronous EL1h
 exception_entry irq_el1h  // IRQ EL1h
 exception_entry fiq_el1h  // FIQ EL1h
 exception_entry error_el1h  // Error EL1h

 exception_entry sync_el0_64  // Synchronous 64-bit EL0
 exception_entry irq_el0_64  // IRQ 64-bit EL0
 exception_entry fiq_el0_64  // FIQ 64-bit EL0
 exception_entry error_el0_64  // Error 64-bit EL0

 exception_entry sync_el0_32  // Synchronous 32-bit EL0
 exception_entry irq_el0_32  // IRQ 32-bit EL0
 exception_entry fiq_el0_32  // FIQ 32-bit EL0
 exception_entry error_el0_32  // Error 32-bit EL0

/*
 * The selected stack pointer can be indicated by a suffix to the Exception Level:
 *  - t: SP_EL0 is used
 *  - h: SP_ELx is used
 *
 * ChCore does not enable or handle irq_el1t, fiq_xxx, and error_xxx.
 * The SPSR_EL1 of idle threads is set to 0b0101, which means interrupt
 * are enabled during the their execution and SP_EL1 is selected (h).
 * Thus, irq_el1h is enabled and handled.
 *
 * Similarly, sync_el1t is also not enabled while we simply reuse the handler for
 * sync_el0 to handle sync_el1h (e.g., page fault during copy_to_user and fpu).
 */

irq_el1h:
        /* Simply reusing exception_enter/exit is OK. */
 exception_enter
#ifndef CHCORE_KERNEL_RT
 switch_to_cpu_stack
#endif
 bl handle_irq_el1
 /* should never reach here */
 b .

irq_el1t:
fiq_el1t:
fiq_el1h:
error_el1t:
error_el1h:
sync_el1t:
 bl unexpected_handler

sync_el1h:
 exception_enter
 mov x0, #SYNC_EL1h
 mrs x1, esr_el1
 mrs x2, elr_el1
 bl handle_entry_c
 str     x0, [sp, #16 * 16] /* store the return value as the ELR_EL1 */
 exception_exit

sync_el0_64:
 exception_enter
#ifndef CHCORE_KERNEL_RT
 switch_to_cpu_stack
#endif
 mrs x25, esr_el1
 lsr x24, x25, #ESR_EL1_EC_SHIFT
 cmp x24, #ESR_EL1_EC_SVC_64
 b.eq el0_syscall
 mov x0, SYNC_EL0_64 
 mrs x1, esr_el1
 mrs x2, elr_el1
 bl handle_entry_c
#ifdef CHCORE_KERNEL_RT
 bl do_pending_resched
#else
 switch_to_thread_ctx
#endif
 exception_exit

el0_syscall:

/* hooking syscall: ease tracing or debugging */
#if ENABLE_HOOKING_SYSCALL == ON
 sub sp, sp, #16 * 8
 stp x0, x1, [sp, #16 * 0]
 stp x2, x3, [sp, #16 * 1]
 stp x4, x5, [sp, #16 * 2]
 stp x6, x7, [sp, #16 * 3]
 stp x8, x9, [sp, #16 * 4]
 stp x10, x11, [sp, #16 * 5]
 stp x12, x13, [sp, #16 * 6]
 stp x14, x15, [sp, #16 * 7]
 
 mov x0, x8
 bl hook_syscall

 ldp x0, x1, [sp, #16 * 0]
 ldp x2, x3, [sp, #16 * 1]
 ldp x4, x5, [sp, #16 * 2]
 ldp x6, x7, [sp, #16 * 3]
 ldp x8, x9, [sp, #16 * 4]
 ldp x10, x11, [sp, #16 * 5]
 ldp x12, x13, [sp, #16 * 6]
 ldp x14, x15, [sp, #16 * 7]
 add sp, sp, #16 * 8
#endif

 adr x27, syscall_table  // syscall table in x27
 uxtw x16, w8    // syscall number in x16
 ldr x16, [x27, x16, lsl #3]  // find the syscall entry
 blr x16

 /* Ret from syscall */
 // bl disable_irq
#ifdef CHCORE_KERNEL_RT
 str x0, [sp]
 bl do_pending_resched
#else
 switch_to_thread_ctx
 str x0, [sp]
#endif
 exception_exit

irq_el0_64:
 exception_enter
#ifndef CHCORE_KERNEL_RT
 switch_to_cpu_stack
#endif
 bl handle_irq
 /* should never reach here */
 b .

error_el0_64:
sync_el0_32:
irq_el0_32:
fiq_el0_32:
error_el0_32:
 bl unexpected_handler

fiq_el0_64:
 exception_enter
#ifndef CHCORE_KERNEL_RT
 switch_to_cpu_stack
#endif
 bl handle_fiq
 /* should never reach here */
 b .

// 实现线程切换功能，通过异常返回机制切换到目标线程
/* void eret_to_thread(u64 sp) */
BEGIN_FUNC(__eret_to_thread)
 mov sp, x0
 dmb ish /* smp_mb() */
#ifdef CHCORE_KERNEL_RT
 bl finish_switch
#endif
 exception_exit
END_FUNC(__eret_to_thread)
```

## 解析

### 异常向量表

向量表内容本身即和文档中图片所示结构一致，注意一下它是内存对齐的，通过宏定义实现：

```c
.macro exception_entry label
 /* Each entry of the exeception table should be 0x80 aligned */
 .align 7
 b \label
.endm
```

这里的 `.align 7` 便把内存对齐到了 `0x80` 字节

### 关键宏定义

- `exception_enter` : 保存CPU上下文，包括通用寄存器和系统寄存器
- `exception_exit` : 恢复CPU上下文并返回
- `switch_to_cpu_stack` : 切换到CPU栈
- `switch_to_thread_ctx` : 切换到线程上下文

注意保存CPU上下文的这两个宏，它们采用的方式是直接把寄存器的值保存在了内核栈上。如果你看过xv6的代码，就会发现这不同于xv6的trampoline在内核页表上规定了一个特殊的位置（trampoline page）用于保存寄存器

两种设计的权衡（选读）：

> trampoline page实现起来较易，但如果要支持多线程和抢占式调度，则相对麻烦
>
>
> 早期 Unix 选择 trampoline page 主要是因为当时的硬件限制，而现代 ARM OS 选择内核栈是因为现代硬件的进步和对系统安全性和性能的更高要求。两种方法各有优劣，选择哪种方法需要根据具体的硬件架构和操作系统设计目标进行权衡。
>
> 一些早期的操作系统，例如 xv6-riscv，仍然使用 trampoline page 来处理用户态到内核态的转换。[2](https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/trampoline.S) Linux 内核中也使用了 trampoline 的概念，但其作用和实现方式与早期 Unix 中的 trampoline page 不同。Linux 中的 trampoline 主要用于处理内核地址空间布局随机化 (KASLR) 等安全特性。[1](https://mxatone.medium.com/kernel-memory-randomization-and-trampoline-page-tables-9f73827270ab)
>
> 总而言之，选择 trampoline page 还是内核栈是一个 trade-off 的过程。Trampoline page 实现简单，节省内存，但安全性较低，难以支持多处理器和抢占式调度。内核栈实现复杂，占用内存较多，但安全性更高，更易于支持多处理器和抢占式调度。现代操作系统大多选择内核栈，是因为现代硬件的性能提升使得内核栈的开销可以接受，并且内核栈带来的安全性提升和功能扩展更加重要。
>

### 异常处理流程（C ABI兼容）

这部分又分为两大类：系统调用和普通异常处理

- 系统调用：

在AArch64中，系统调用由 `svc` 指令执行，这时触发 `sync_el0_64` 异常，进入向量表：

```nasm
sync_el0_64:
    exception_enter                      // 保存上下文
#ifndef CHCORE_KERNEL_RT
    switch_to_cpu_stack                 // 非RT模式下切换到CPU栈
#endif
    mrs x25, esr_el1                   // 读取异常综合寄存器(ESR)
    lsr x24, x25, #ESR_EL1_EC_SHIFT    // 获取异常类别
    cmp x24, #ESR_EL1_EC_SVC_64        // 判断是否为系统调用
    b.eq el0_syscall                    // 如果是系统调用，跳转处理
```

随后进入系统调用的核心逻辑（这里的钩子是用来调试+监控的）

```nasm
el0_syscall:
    // 如果启用了系统调用钩子
#if ENABLE_HOOKING_SYSCALL == ON
    // 保存寄存器x0-x15
    sub sp, sp, #16 * 8
    stp x0, x1, [sp, #16 * 0]
    // ... 保存其他寄存器 ...
    
    mov x0, x8           // 系统调用号作为参数
    bl hook_syscall      // 调用钩子函数
    
    // 恢复寄存器
    ldp x0, x1, [sp, #16 * 0]
    // ... 恢复其他寄存器 ...
    add sp, sp, #16 * 8
#endif
    // 系统调用处理核心逻辑
    adr x27, syscall_table              // 获取系统调用表地址
    uxtw x16, w8                        // 系统调用号（保存在x8中）
    ldr x16, [x27, x16, lsl #3]         // 查找系统调用处理函数
    blr x16                             // 调用对应的处理函数
```

从系统调用表返回后，进行返回处理：

```nasm
    // 系统调用返回值处理
#ifdef CHCORE_KERNEL_RT
    str x0, [sp]                        // 保存返回值
    bl do_pending_resched               // RT模式下检查是否需要重新调度
#else
    switch_to_thread_ctx                // 切换回线程上下文
    str x0, [sp]                        // 保存返回值
#endif
    exception_exit                      // 恢复上下文并返回用户态
```

- 普通异常处理：以 `sync_el1h` 为例

```nasm
sync_el1h:
 exception_enter
 mov x0, #SYNC_EL1h
 mrs x1, esr_el1
 mrs x2, elr_el1
 bl handle_entry_c
 str     x0, [sp, #16 * 16] /* store the return value as the ELR_EL1 */
 exception_exit
```

大体上即为保存上下文——调用函数——恢复上下文的流程

注意这里的C ABI的体现，`x0, x1, x2` 就是c函数的args, 返回值置于 `x0`

而对于其他异常/还没有实现的异常，则直接调用 `unexpected_handler`

```nasm
error_el0_64:
sync_el0_32:
irq_el0_32:
fiq_el0_32:
error_el0_32:
 bl unexpected_handler
```

至此，异常管理部分源码解析结束
