# Linker Script

在Chcore 构建的最后一步，即链接产生 `build/kernel.img` 时，ChCore 构建系统中指定了使用从 `kernel/arch/aarch64/boot/linker.tpl.ld` 模板产生的 linker script 来精细控制 ELF 加载后程序各分段在内存中布局。

具体地，将 `${init_objects}`（即 `kernel/arch/aarch64/boot/raspi3` 中的代码编成的目标文件）放在了 ELF 内存的 `TEXT_OFFSET`（即 `0x80000`）位置，`.text`（代码段）、 `.data`（数据段）、`.rodata`（只读数据段）和 `.bss`（BSS 段）依次紧随其后。

这里对这些分段所存放的内容做一些解释：

- `init`：内核启动阶段代码和数据，因为此时还没有开启 MMU，内核运行在低地址，所以需要特殊处理
- `.text`：内核代码，由一条条的机器指令组成
- `.data`：已初始化的全局变量和静态变量
- `.rodata`：只读数据，包括字符串字面量等
- `.bss`：未初始化的全局变量和静态变量，由于没有初始值，因此在 ELF 中不需要真的为该分段分配空间，而是只需要记录目标内存地址和大小，在加载时需要初始化为 0

除了指定各分段的顺序和对齐，linker script 中还指定了它们运行时“认为自己所在的内存地址”和加载时“实际存放在的内存地址”。例如前面已经说到 `init` 段被放在了 `TEXT_OFFSET` 即 `0x80000` 处，由于启动时内核运行在低地址，此时它“认为自己所在的地址”也应该是 `0x80000`，而后面的 `.text` 等段则被放在紧接着 `init` 段之后，但它们在运行时“认为自己在” `KERNEL_VADDR + init_end` 也就是高地址。

更多关于 linker script 的细节请参考 [Linker Scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)。
