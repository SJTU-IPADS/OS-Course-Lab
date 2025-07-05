# 内核启动

<!-- toc -->

## 树莓派启动过程

在树莓派 3B+ 真机上，通过 SD 卡启动时，上电后会运行 ROM 中的特定固件，接着加载并运行 SD 卡上的 `bootcode.bin` 和 `start.elf`，后者进而根据 `config.txt` 中的配置，加载指定的 kernel 映像文件（纯 binary 格式，通常名为 `kernel8.img`）到内存的 `0x80000` 位置并跳转到该地址开始执行。

而在 QEMU 模拟的 `raspi3b`（旧版 QEMU 为 `raspi3`）机器上，则可以通过 `-kernel` 参数直接指定 ELF 格式的 kernel 映像文件，进而直接启动到 ELF 头部中指定的入口地址，即 `_start` 函数（实际上也在 `0x80000`，因为 ChCore 通过 linker script 强制指定了该函数在 ELF 中的位置，如有兴趣请参考[附录](../Appendix/linker.html)）。

## 启动 CPU 0 号核

`_start` 函数（位于 `kernel/arch/aarch64/boot/raspi3/init/start.S`）是 ChCore 内核启动时执行的第一块代码。由于 QEMU 在模拟机器启动时会同时开启 4 个 CPU 核心，于是 4 个核会同时开始执行 `_start` 函数。而在内核的初始化过程中，我们通常需要首先让其中一个核进入初始化流程，待进行了一些基本的初始化后，再让其他核继续执行。

> [!QUESTION] 思考题 1
> 阅读 `_start` 函数的开头，尝试说明 ChCore 是如何让其中一个核首先进入初始化流程，并让其他核暂停执行的。  

> [!HINT]
> 可以在 [Arm Architecture Reference Manual](https://documentation-service.arm.com/static/61fbe8f4fa8173727a1b734e) 找到 `mpidr_el1` 等系统寄存器的详细信息。

## 切换异常级别

AArch64 架构中，特权级被称为异常级别（Exception Level，EL），四个异常级别分别为 EL0、EL1、EL2、EL3，其中 EL3 为最高异常级别，常用于安全监控器（Secure Monitor），EL2 其次，常用于虚拟机监控器（Hypervisor），EL1 是内核常用的异常级别，也就是通常所说的内核态，EL0 是最低异常级别，也就是通常所说的用户态。

QEMU `raspi3b` 机器启动时，CPU 异常级别为 EL3，我们需要在启动代码中将异常级别降为 EL1，也就是进入内核态。具体地，这件事是在 `arm64_elX_to_el1` 函数（位于 `kernel/arch/aarch64/boot/raspi3/init/tools.S`）中完成的。

为了使 `arm64_elX_to_el1` 函数具有通用性，我们没有直接写死从 EL3 降至 EL1 的逻辑，而是首先判断当前所在的异常级别，并根据当前异常级别的不同，跳转到相应的代码执行。

```asm
{{#include ../../Lab1/kernel/arch/aarch64/boot/raspi3/init/tools.S:70:143}}
```

> [!CODING] 练习题 2
> 在 `arm64_elX_to_el1` 函数的 `LAB 1 TODO 1` 处填写一行汇编代码，获取 CPU 当前异常级别。

> [!HINT]
> 通过 `CurrentEL` 系统寄存器可获得当前异常级别。通过 GDB 在指令级别单步调试可验证实现是否正确。注意参考文档理解 `CurrentEL` 各个 bits 的[意义](https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/CurrentEL--Current-Exception-Level)。

`eret`指令可用于从高异常级别跳到更低的异常级别，在执行它之前我们需要设置
设置 `elr_elx`（异常链接寄存器）和 `spsr_elx`（保存的程序状态寄存器），分别控制`eret`执行后的指令地址（PC）和程序状态（包括异常返回后的异常级别）。

> [!CODING] 练习题 3
> 在 `arm64_elX_to_el1` 函数的 `LAB 1 TODO 2` 处填写大约 4 行汇编代码，设置从 EL3 跳转到 EL1 所需的 `elr_el3` 和 `spsr_el3` 寄存器值。

> [!HINT]
> `elr_el3` 的正确设置应使得控制流在 `eret` 后从 `arm64_elX_to_el1` 返回到 `_start` 继续执行初始化。 `spsr_el3` 的正确设置应正确屏蔽 DAIF 四类中断，并且将 [SP](https://developer.arm.com/documentation/ddi0500/j/CHDDGJID) 正确设置为 `EL1h`. 在设置好这两个系统寄存器后，不需要立即 `eret`.

练习完成后，可使用 GDB 跟踪内核代码的执行过程，由于此时不会有任何输出，可通过是否正确从 `arm64_elX_to_el1` 函数返回到 `_start` 来判断代码的正确性。

## 跳转到第一行 C 代码

降低异常级别到 EL1 后，我们准备从汇编跳转到 C 代码，在此之前我们先设置栈（SP）。因此，`_start` 函数在执行 `arm64_elX_to_el1` 后，即设置内核启动阶段的栈，并跳转到第一个 C 函数 `init_c`。

```
{{#include ../../Lab1/kernel/arch/aarch64/boot/raspi3/init/start.S:23:82}}

```

> [!QUESTION] 思考题 4
> 说明为什么要在进入 C 函数之前设置启动栈。如果不设置，会发生什么？

进入 `init_c` 函数后，第一件事首先通过 `clear_bss` 函数清零了 `.bss` 段，该段用于存储未初始化的全局变量和静态变量（具体请参考[附录](../Appendix/elf.html)）。

> [!QUESTION] 思考题 5
> 在实验 1 中，其实不调用 `clear_bss` 也不影响内核的执行，请思考不清理 `.bss` 段在之后的何种情况下会导致内核无法工作。

## 初始化串口输出

到目前为止我们仍然只能通过 GDB 追踪内核的执行过程，而无法看到任何输出，这无疑是对我们写操作系统的积极性的一种打击。因此在 `init_c` 中，我们启用树莓派的 UART 串口，从而能够输出字符。

在 `kernel/arch/aarch64/boot/raspi3/peripherals/uart.c` 已经给出了 `early_uart_init` 和 `early_uart_send` 函数，分别用于初始化 UART 和发送单个字符（也就是输出字符）。

```
{{#include ../../Lab1/kernel/arch/aarch64/boot/raspi3/peripherals/uart.c:140:146}}

```

> [!CODING] 练习题6
> 在 `kernel/arch/aarch64/boot/raspi3/peripherals/uart.c` 中 `LAB 1 TODO 3` 处实现通过 UART 输出字符串的逻辑。

> [!SUCCESS] 第一个字符串
> 恭喜！我们终于在内核中输出了第一个字符串！  
> 感兴趣的同学请思考`early_uart_send`究竟是怎么输出字符的。

## 启用 MMU

在内核的启动阶段，还需要配置启动页表（`init_kernel_pt` 函数），并启用 MMU（`el1_mmu_activate` 函数），使可以通过虚拟地址访问内存，从而为之后跳转到高地址作准备（内核通常运行在虚拟地址空间 `0xffffff0000000000` 之后的高地址）。

关于配置启动页表的内容由于包含关于页表的细节，将在本实验下一部分实现，目前直接启用 MMU。

在 EL1 异常级别启用 MMU 是通过配置系统寄存器 `sctlr_el1` 实现的（Arm Architecture Reference Manual D13.2.118）。具体需要配置的字段主要包括：

- 是否启用 MMU（`M` 字段）
- 是否启用对齐检查（`A` `SA0` `SA` `nAA` 字段）
- 是否启用指令和数据缓存（`C` `I` 字段）

> [!CODING] 练习题7
> 在 `kernel/arch/aarch64/boot/raspi3/init/tools.S` 中 `LAB 1 TODO 4` 处填写一行汇编代码，以启用 MMU。

由于没有配置启动页表，在启用 MMU 后，内核会立即发生地址翻译错误（Translation Fault），进而尝试跳转到异常处理函数（Exception Handler），
该异常处理函数的地址为异常向量表基地址（`vbar_el1` 寄存器）加上 `0x200`。
此时我们没有设置异常向量表（`vbar_el1` 寄存器的值是0），因此执行流会来到 `0x200` 地址，此处的代码为非法指令，会再次触发异常并跳转到 `0x200` 地址。
使用 GDB 调试，在 GDB 中输入 `continue` 后，待内核输出停止后，按 Ctrl-C，可以观察到内核在 `0x200` 处无限循环。

---

> [!IMPORTANT]
> 以上为Lab1 Part1 的内容
