# 多核支持
本节内容负责解析ChCore关于多核支持方面的源码，包括多核原理，多核启动等部分
## 目录
- [知识回顾](#知识回顾)
  - [CPU信息](#cpu信息)
  - [如何按需启动多核](#如何按需启动多核)
- [多核启动](#多核启动)
  - [初始调度](#初始调度)
  - [唤醒从核](#唤醒从核)

# 知识回顾

支持多核，首先要有多核，这部分内容需要的知识其实在之前的源码解析中也提到过，我们先复习复习以前的内容

## CPU信息

我们在讲解系统调用的时候曾经提到：

> `TPIDR_EL1`（Thread Process ID Register for EL1）是ARM架构中一个特殊的寄存器，**用于存储当前执行线程或进程的上下文信息**。在操作系统内核中，这个寄存器经常被用来存储指向`per_cpu_data`结构的指针，该结构包含了特定于CPU的数据，比如CPU的局部变量和栈指针
> 

那么多核，自然就会有多个这样的CPU信息块，具体数量又依硬件设备而定，以树莓派3为例：

```c
#include <common/vars.h>

/* raspi3 config */
#define PLAT_CPU_NUM    4
#define PLAT_RASPI3
```

好的知道了，是4个核心。至于CPU_info，我们在讲解系统调用的内核栈切换时候也讲过：以结构体形式存在，通过指针+偏移量的形式访问结构体成员

```c
#define OFFSET_CURRENT_EXEC_CTX		0
#define OFFSET_LOCAL_CPU_STACK		8
#define OFFSET_CURRENT_FPU_OWNER	16
#define OFFSET_FPU_DISABLE		24

struct per_cpu_info {
	/* The execution context of current thread */
	u64 cur_exec_ctx;

	/* Per-CPU stack */
	char *cpu_stack;

	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;

	char pad[pad_to_cache_line(sizeof(u64) +
				   sizeof(char *) +
				   sizeof(void *) +
				   sizeof(u32))];
} __attribute__((packed, aligned(64)));
```

## 如何按需启动多核

回忆在"机器启动"部分的内容：

主核恒为cpu 0, 在 `start.S` 之中我们比较当前cpu id和0，如果是0核就跳进primary执行init_c

而从核则是先循环等待bss段清零，再循环等待smp enable

```nasm
primary:

	/* Turn to el1 from other exception levels. */
	bl 	arm64_elX_to_el1

	/* Prepare stack pointer and jump to C. */
	adr 	x0, boot_cpu_stack
	add 	x0, x0, #INIT_STACK_SIZE
	mov 	sp, x0

	b 	init_c

	/* Should never be here */
	b	.
	
		/* Wait for bss clear */
wait_for_bss_clear:
	// ...
wait_until_smp_enabled:
	// ...
```

主核在 `init_c` 初始化uart之后用 `sev` 指令唤醒其他核（树莓派真机需求，在QEMU模拟器中是直接启动的），之后主核进入 `start_kernel` ，初始化cpu内核栈、清空页表和TLB设置后进入 `main`

```c
void init_c(void)
{
	/* Clear the bss area for the kernel image */
	clear_bss();

	/* Initialize UART before enabling MMU. */
	early_uart_init();
	uart_send_string("boot: init_c\r\n");

	wakeup_other_cores();

	/* Initialize Kernell Page Table. */
	uart_send_string("[BOOT] Install kernel page table\r\n");
	init_kernel_pt();

	/* Enable MMU. */
	el1_mmu_activate();
	uart_send_string("[BOOT] Enable el1 MMU\r\n");

	/* Call Kernel Main. */
	uart_send_string("[BOOT] Jump to kernel main\r\n");
	start_kernel(secondary_boot_flag);

	/* Never reach here */
}
```

在 `main` 中，则依次按照顺序：

- 初始化锁
- 初始化uart
- 初始化cpu info
- 初始化内存管理模块
- 初始化内核页表
- 初始化调度器
- 启动smp

此时其他核通过 `secondary_init` 初始化自己的cpu info和kernel stack之后让出cpu, 进入调度

之后由主核负责创建第一个用户态线程（即 `create_root_thread` ），完毕后全部进入调度

```c
/*
 * @boot_flag is boot flag addresses for smp;
 * @info is now only used as board_revision for rpi4.
 */
void main(paddr_t boot_flag, void *info)
{
	// ...
	/* Other cores are busy looping on the boot_flag, wake up those cores */
	enable_smp_cores(boot_flag);

	// ...

	smc_init();

	// ...

	/* Create initial thread here, which use the `init.bin` */
	create_root_thread();
	kinfo("[ChCore] create initial thread done\n");
	
	/* Leave the scheduler to do its job */
	sched();

	// ...
}
```

# 多核启动

## 初始调度

第一个问题来了：在创建第一个线程时，所有内核均已启动，而这时候并没有等待的别线程，那调度给谁呢？

答案是自己调度给自己，并且会有idle优化（空闲线程优化），这部分内容在Linux中亦有记载：

(ref: https://www.cnblogs.com/doitjust/p/13307378.html)

我们以rr调度策略为例，来看看ChCore的实现（具体是哪种策略会在构建时决定，参考 `main` 函数的源代码）：

```c
struct thread *rr_sched_choose_thread(void)
{
        unsigned int cpuid = smp_get_cpu_id();
        struct thread *thread = NULL;

        if (!list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        again:
                if (list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;
                }
                /*
                 * When the thread is just moved from another cpu and
                 * the kernel stack is used by the origina core, try
                 * to find another thread.
                 */
                if (!(thread = find_runnable_thread(
                              &(rr_ready_queue_meta[cpuid].queue_head)))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;
                }

                BUG_ON(__rr_sched_dequeue(thread));
                if (thread_is_exiting(thread) || thread_is_exited(thread)) {
                        /* Thread need to exit. Set the state to TE_EXITED */
                        thread_set_exited(thread);
                        goto again;
                }
                unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                return thread;
        }
out:
        return &idle_threads[cpuid];
}
```

注意到在等待队列为空的时候，会来到标签 `out` ，返回一个 `idle_thread` ，即空闲线程

它的ctx会在初始化的时候被放在 `idle_thread_routine` 处，这个函数是体系结构相关的，旨在防止cpu空转降低功耗

```c
/* Arch-dependent func declaraions, which are defined in assembly files */
extern void idle_thread_routine(void);
```

进一步阅读汇编代码，这个函数在arm架构中是wfi指令，让cpu进入低功耗状态，在某些版本中的实现是关闭几乎所有的时钟

```nasm
BEGIN_FUNC(idle_thread_routine)
idle:   wfi
        b idle
END_FUNC(idle_thread_routine)
```

## 唤醒从核

在"机器启动"栏目，我们只是简单的讲解了主核通过设置 `secondary_boot_flag` 来唤醒处于轮询状态的从核，这里我们细致分析这一过程：

首先看主核 `main` 函数的参数：

```c
void main(paddr_t boot_flag, void *info)
```

这里的 `boot_flag` 即是之前在 init_c 中传入的 `secondary_boot_flag`

再来看看 secondary_boot_flag 自己是什么东西：

```c
// kernel/arch/aarch64/boot/raspi3/init/init_c.c
/*
 * Initialize these varibles in order to make them not in .bss section.
 * So, they will have concrete initial value even on real machine.
 *
 * Non-primary CPUs will spin until they see the secondary_boot_flag becomes
 * non-zero which is set in kernel (see enable_smp_cores).
 *
 * The secondary_boot_flag is initilized as {NOT_BSS, 0, 0, ...}.
 */
#define NOT_BSS (0xBEEFUL)
long secondary_boot_flag[PLAT_CPU_NUMBER] = {NOT_BSS}; // 0xBEEFUL
volatile u64 clear_bss_flag = NOT_BSS;
```

`secondary_boot_flag`实际上是作为kernel .data 段的一个地址被加载的

毫无疑问，这时候内核页表都还没初始化，那它本身指的必然是物理地址

而它在什么时候发挥作用？是在 `main` 中：

```c
/*
 * @boot_flag is boot flag addresses for smp;
 * @info is now only used as board_revision for rpi4.
 */
void main(paddr_t boot_flag, void *info)
{
	// ...
	
	/* Other cores are busy looping on the boot_flag, wake up those cores */
	enable_smp_cores(boot_flag);

	// ...

	/* Create initial thread here, which use the `init.bin` */
	create_root_thread();

	/* Leave the scheduler to do its job */
	sched();
	
	// ...
}
```

从 `main` 函数的签名以及 `enable_smp_cores` 函数的实现也可以看出来，我们需要先进行一次转换得到虚拟地址，再进行后续的操作：

```c
void enable_smp_cores(paddr_t boot_flag)
{
        int i = 0;
        long *secondary_boot_flag;

        /* 设置当前CPU（主核）的状态为运行状态 */
        cpu_status[smp_get_cpu_id()] = cpu_run;

        /* 将启动标志数组的物理地址转换为虚拟地址 */
        secondary_boot_flag = (long *)phys_to_virt(boot_flag);

        /* 遍历所有CPU核心 */
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                /* 设置当前CPU的启动标志
                 * 这个标志会被对应的CPU核心检测到，从而开始其启动流程
                 */
                secondary_boot_flag[i] = 1;

                /* 刷新数据缓存区域
                 * 确保启动标志的更新对所有CPU核心可见
                 * 防止缓存一致性问题导致其他核心看不到更新
                 */
                flush_dcache_area((u64) secondary_boot_flag,
                                (u64) sizeof(u64) * PLAT_CPU_NUM);

                /* 数据同步屏障
                 * 确保在继续执行前，所有的内存操作都已完成
                 * 这是多核系统中保证内存一致性的关键步骤
                 */
                asm volatile ("dsb sy");

                /* 等待目标CPU改变其状态
                 * 通过轮询检查cpu_status数组来确认CPU已经启动
                 * cpu_hang表示CPU尚未启动完成
                 * 当CPU完成初始化后，会将其状态改为非cpu_hang值
                 */
                while (cpu_status[i] == cpu_hang)
                        ;

                /* 打印CPU激活信息，用于调试和状态跟踪 */
                kinfo("CPU %d is active\n", i);
        }

        /* 所有CPU启动完成
         * 打印总结信息，标志着多核初始化的完成
         */
        kinfo("All %d CPUs are active\n", PLAT_CPU_NUM);

        /* 初始化处理器间中断（IPI）数据
         * 这是多核系统中进行核间通信的必要步骤
         * 必须在所有CPU都启动完成后才能初始化
         */
        init_ipi_data();
}
```

为什么这时候又需要转换为虚拟地址？因为这个函数是在主核中被调用的，主核已经完成初始化页表的工作了，自然需要虚拟地址

我在主核改的flag，你从核又怎么看得见？通过刷新数据缓存，即 `flush_dcache_area` 函数，而这又和硬件设计联系在一起了

```nasm
BEGIN_FUNC(flush_dcache_area)
	dcache_by_line_op civac, sy, x0, x1, x2, x3
	ret
END_FUNC(flush_dcache_area)
```

至此，多核支持部分源码解析到此结束