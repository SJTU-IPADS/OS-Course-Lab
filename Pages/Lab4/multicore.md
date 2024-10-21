# 多核支持

> [!NOTE]
> 本部分实验没有代码题，仅有思考题。

---

<!-- toc -->

为了让ChCore支持多核，我们需要考虑如下问题：
- 如何启动多核，让每个核心执行初始化代码并开始执行用户代码？
- 如何区分不同核心在内核中保存的数据结构（比如状态，配置，内核对象等）？
- 如何保证内核中对象并发正确性，确保不会由于多个核心同时访问内核对象导致竞争条件？

在启动多核之前，我们先介绍ChCore如何解决第二个问题。ChCore对于内核中需要每个CPU核心单独存一份的内核对象，都根据核心数量创建了多份（即利用一个数组来保存）。ChCore支持的核心数量为PLAT_CPU_NUM（该宏定义在 `kernel/common/machine.h` 中，其代表可用CPU核心的数量，根据具体平台而异）。 比如，实验使用的树莓派3平台拥有4个核心，因此该宏定义的值为4。ChCore会CPU核心的核心ID作为数组的索引，在数组中取出对应的CPU核心本地的数据。为了方便确定当前执行该代码的CPU核心ID，我们在 `kernel/arch/aarch64/machine/smp.c`中提供了smp_get_cpu_id函数。该函数通过访问系统寄存器tpidr_el1来获取调用它的CPU核心的ID，该ID可用作访问上述数组的索引。

```c
{{#include ../../Lab4/kernel/include/arch/aarch64/plat/raspi3/machine.h:16:20}}
```

## 启动多核
在实验1中我们已经介绍，在QEMU模拟的树莓派中，所有CPU核心在开机时会被同时启动。在引导时这些核心会被分为两种类型。一个指定的CPU核心会引导整个操作系统和初始化自身，被称为CPU主核（primary CPU）。其他的CPU核心只初始化自身即可，被称为CPU从核（backup CPU）。CPU核心仅在系统引导时有所区分，在其他阶段，每个CPU核心都是被相同对待的。

> [!QUESTION] 思考题 1
> 阅读`Lab1`中的汇编代码kernel/arch/aarch64/boot/raspi3/init/start.S。说明ChCore是如何选定主CPU，并阻塞其他其他CPU的执行的。

然而在树莓派真机中，从还需要主C核手动指定每一个CPU核心的的启动地址。这些CPU核心会读取固定地址的上填写的启动地址，并跳转到该地址启动。在kernel/arch/aarch64/boot/raspi3/init/init_c.c中，我们提供了`wakeup_other_cores`函数用于实现该功能，并让所有的CPU核心同在QEMU一样开始执行_start函数。

与之前的实验一样，主CPU在第一次返回用户态之前会在`kernel/arch/aarch64/main.c`中执行main函数，进行操作系统的初始化任务。在本小节中，ChCore将执行enable_smp_cores函数激活各个其他CPU。

> [!QUESTION] 思考题 2
> 阅读汇编代码`kernel/arch/aarch64/boot/raspi3/init/start.S`, init_c.c以及kernel/arch/aarch64/main.c，解释用于阻塞其他CPU核心的secondary_boot_flag是物理地址还是虚拟地址？是如何传入函数`enable_smp_cores`中，又是如何赋值的（考虑虚拟地址/物理地址）？

---

> [!SUCCESS]
> 以上为Lab4 part1 的所有内容
