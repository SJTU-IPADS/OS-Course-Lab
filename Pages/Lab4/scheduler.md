# 多核调度

<!-- toc -->

ChCore已经可以启动多核，但仍然无法对多个线程进行调度。本部分将首先实现协作式调度，从而允许当前在CPU核心上运行的线程主动退出或主动放弃CPU时，CPU核心能够切换到另一个线程继续执行。其后，我们将驱动树莓派上的物理定时器，使其以一定的频率发起中断，使得内核可以在一定时间片后重新获得对CPU核心的控制，并基于此进一步实现抢占式调度。

ChCore中与调度相关的函数与数据结构定义在`kernel/include/sched/sched.h`中。

```c
{{#include ../../Lab4/kernel/include/sched/sched.h:76:84}}
```
sched_ops是用于抽象ChCore中调度器的一系列操作。它存储指向不同调度操作的函数指针，以支持不同的调度策略。
cur_sched_ops则是一个sched_ops的实例，其在内核初始化过程中（main函数）调用sched_init进行初始化。
ChCore用在 `kernel/include/sched/sched.h` 中定义的静态函数封装对cur_sched_ops的调用。sched_ops中定义的调度器操作如下所示：

* sched_init：初始化调度器。
* sched：进行一次调度。即将正在运行的线程放回就绪队列，然后在就绪队列中选择下一个需要执行的线程返回。
* sched_enqueue：将新线程添加到调度器的就绪队列中。
* sched_dequeue：从调度器的就绪队列中取出一个线程。
* sched_top：用于debug,打印当前所有核心上的运行线程以及等待线程的函数。

在本部分将实现一个基本的Round Robin（时间片轮转）调度器，该程序调度在同一CPU核心上运行的线程，因此内核初始化过程调用sched_init时传入了&rr作为参数。该调度器的调度操作（即对于sched_ops定义的各个函数接口的实现）实现在`kernel/sched/policy_rr.c`中，这里简要介绍其涉及的数据结构：

`current_threads`是一个数组，分别指向每个CPU核心上运行的线程。而`current_thread`则利用`smp_get_cpu_id`获取当前运行核心的id，从而找到当前核心上运行的线程。

`struct queue_meta`定义了round robin调度器使用的就绪队列，其中`queue_head`字段是连接该就绪队列上所有等待线程的队列，`queue_len`字段是目前该就绪队列的长度，`queue_lock`字段是用于保证该队列并发安全的锁。 `kernel/sched/policy_rr.c`定义了一个全局变量`rr_ready_queue_meta`,该变量是一个`struct queue_meta`类型的数组，数组大小由`PLAT_CPU_NUM`定义，即代表每个CPU核心都具有一个就绪队列。运行的CPU核心可以通过`smp_get_cpu_id`获取当前运行核心的id，从而在该数组中找到当前核心对应的就绪队列。

## 调度队列初始化
内核初始化过程中会调用`sched_init`初始化调度相关的元数据，`sched_init`定义在`kernel/sched/sched.c`中，该函数首先初始化idle_thread(每个CPU核心拥有一个idle_thread，当调度器的就绪队列中没有等待线程时会切换到idle_thread运行)，然后会初始化`current_threads`数组，最后调用`struct sched_ops rr`中定义的sched_init函数，即`rr_sched_init`。
> [!CODING] 练习题 1
> 在 `kernel/sched/policy_rr.c` 中完善 `rr_sched_init` 函数，对 `rr_ready_queue_meta` 进行初始化。在完成填写之后，你可以看到输出“Scheduler metadata is successfully initialized!”并通过 Scheduler metadata initialization 测试点。

> [!HINT] Tip
> sched_init 只会在主 CPU 初始化时调用，因此 rr_sched_init 需要对每个 CPU 核心的就绪队列都进行初始化。

## 调度队列入队
内核初始化过程结束之后会调用`create_root_thread`来创建第一个用户态进程及线程，在`create_root_thread`最后会调用`sched_enqueue`函数将创建的线程加入调度队列之中。`sched_enqueue`
最终会调用kernel/sched/policy_rr.c中定义的`rr_sched_enqueue`函数。该函数首先挑选合适的CPU核心的就绪队列（考虑线程是否绑核以及各个CPU核心之间的负载均衡），然后调用`__rr_sched_enqueue`将线程插入到选中的就绪队列中。
> [!CODING] 练习 2
> 在 kernel/sched/policy_rr.c 中完善 `__rr_sched_enqueue` 函数，将`thread`插入到`cpuid`对应的就绪队列中。

> [!SUCCESS]
> 在完成填写之后，你可以看到输出“Successfully enqueue root thread”并通过 Schedule Enqueue 测试点。

## 调度队列出队
内核初始化过程结束并调用`create_root_thread`创建好第一个用户态进程及线程之后，在第一次进入用户态之前，会调用`sched`函数来挑选要返回到用户态运行的线程（虽然此时就绪队列中只有root thread一个线程）。`sched`最终会调用kernel/sched/policy_rr.c中定义的`rr_sched`函数。
该调度函数的操作非常直观，就是将现在正在运行的线程重新加入调度器的就绪队列当中，并从就绪队列中挑选出一个新的线程运行。
由于内核刚刚完成初始化，我们还没有设置过`current_thread`，所以`rr_sched`函数中`old`为`NULL`，后面的练习中我们会考虑`old`不为`NULL`的情况。紧接着`rr_sched`会调用`rr_sched_choose_thread`函数挑选出下一个运行的线程，并切换到该线程。

`rr_sched_choose_thread`内部会调用`find_runnable_thread`从当前CPU核心的就绪队列中选取一个可以运行的线程并调用`__rr_sched_dequeue`将其从就绪队列中移除。
> [!CODING] 练习 3
> 在 kernel/sched/sched.c 中完善 `find_runnable_thread` 函数，在就绪队列中找到第一个满足运行条件的线程并返回。 在 `kernel/sched/policy_rr.c` 中完善 `__rr_sched_dequeue` 函数，将被选中的线程从就绪队列中移除。

> [!SUCCESS]
> 在完成填写之后，运行 ChCore 将可以成功进入用户态，你可以看到输出“Enter Procmgr Root thread (userspace)”并通过 Schedule Enqueue 测试点。

## 协作式调度
顾名思义，协作式调度需要线程主动放弃CPU。为了实现该功能，我们提供了`sys_yield`这一个系统调用(syscall)。该syscall可以主动放弃当前CPU核心，并调用上述的`sched`接口完成调度器的调度工作。`kernel/sched/policy_rr.c`中定义的`rr_sched`函数中，如果当前运行线程的状态为`TS_RUNNING`，即还处于可以运行的状态，我们应该将其重新加入到就绪队列当中，这样该线程在之后才可以被再次调度执行。

> [!CODING] 练习 4
> 在`kernel/sched/sched.c`中完善系统调用`sys_yield`，使用户态程序可以主动让出CPU核心触发线程调度。
> 此外，请在`kernel/sched/policy_rr.c` 中完善`rr_sched`函数，将当前运行的线程重新加入调度队列中。

> [!SUCCESS]
> 在完成填写之后，运行 ChCore 将可以成功进入用户态并创建两个线程交替执行，你可以看到输出“Cooperative Schedluing Test Done!”并通过 Cooperative Schedluing 测试点。

## 抢占式调度

使用刚刚实现的协作式调度器，ChCore能够在线程主动调用`sys_yield`系统调用让出CPU核心的情况下调度线程。然而，若用户线程不想放弃对CPU核心的占据，内核便只能让用户线程继续执行，而无法强制用户线程中止。 因此，在这一部分中，本实验将实现抢占式调度，以帮助内核定期重新获得对CPU核心的控制权。

ChCore启动的第一个用户态线程（执行`user/system-services/system-servers/procmgr/procmgr.c`的`main`函数）将创建一个“自旋线程”，该线程在获得CPU核心的控制权后便会执行无限循环，进而导致无论是该程序的主线程还是ChCore内核都无法重新获得CPU核心的控制权。就保护系统免受用户程序中的错误或恶意代码影响而言，这一情况显然并不理想，任何用户应用线程均可以如该“自旋线程”一样，通过进入无限循环来永久“霸占”整个CPU核心。

为了处理“自旋线程”的问题，ChCore内核必须具有强行中断一个正在运行的线程并夺回对CPU核心的控制权的能力，为此我们必须扩展ChCore以支持处理来自物理时钟的外部硬件中断。

**物理时钟初始化**

本部分我们将通过配置ARM提供的Generic Timer来使能物理时钟并使其以固定的频率发起中断。
我们需要处理的系统寄存器如下([Refer](https://developer.arm.com/documentation/102379/0101/The-processor-timers)):
* CNTPCT_EL0: 它的值代表了当前的 system count。
* CNTFRQ_EL0: 它的值代表了物理时钟运行的频率，即每秒钟 system count 会增加多少。
* CNTP_CVAL_EL0: 是一个64位寄存器，操作系统可以向该寄存器写入一个值，当 system count 达到或超过该值时，物理时钟会触发中断。
* CNTP_TVAL_EL0: 是一个32位寄存器，操作系统可以写入 TVAL，处理器会在内部读取当前的系统计数，加上写入的值，然后填充 CVAL。
* CNTP_CTL_EL0: 物理时钟的控制寄存器，第0位ENABLE控制时钟是否开启，1代表enble，0代表disable；第1位IMASK代表是否屏蔽时钟中断，0代表不屏蔽，1代表屏蔽。

对物理时钟进行初始化的代码位于`kernel/arch/aarch64/plat/raspi3/irq/timer.c`的`plat_timer_init`函数。

> [!CODING] 练习 5
> 请根据代码中的注释在`kernel/arch/aarch64/plat/raspi3/irq/timer.c`中完善`plat_timer_init`函数，初始化物理时钟。需要完成的步骤有：
> * 读取 CNTFRQ_EL0 寄存器，为全局变量 cntp_freq 赋值。
> * 根据 TICK_MS（由ChCore决定的时钟中断的时间间隔，以ms为单位，ChCore默认每10ms触发一次时钟中断）和cntfrq_el0 （即物理时钟的频率）计算每两次时钟中断之间 system count 的增长量，将其赋值给 cntp_tval 全局变量，并将 cntp_tval 写入 CNTP_TVAL_EL0 寄存器！
> * 根据上述说明配置控制寄存器CNTP_CTL_EL0。

> [!HINT]
> 由于启用了时钟中断，但目前还没有对中断进行处理，所以会影响评分脚本的评分，你可以通过运行ChCore观察是否有`"[TEST] Physical Timer was successfully initialized!: OK"`输出来判断是否正确对物理时钟进行初始化。

**物理时钟中断与抢占**

我们在lab3中已经为ChCore配置过异常向量表（`kernel/arch/aarch64/irq/irq_entry.S`），当收到来自物理时钟的外部中断时，内核会进入`handle_irq`中断处理函数，该函数会调用平台相关的`plat_handle_irq`来进行中断处理。`plat_handle_irq`内部如果判断中断源为物理时钟，则调用`handle_timer_irq`。

ChCore记录每个线程所拥有的时间片（`thread->thread_ctx->sc->budget`），为了能够让线程之间轮转运行，我们应当在处理时钟中断时递减当前运行线程的时间片，并在当前运行线程的时间片耗尽时进行调度，选取新的线程运行。

> [!CODING] 练习 6
> 请在`kernel/arch/aarch64/plat/raspi3/irq/irq.c`中完善`plat_handle_irq`函数，当中断号irq为INT_SRC_TIMER1（代表中断源为物理时钟）时调用`handle_timer_irq`并返回。 请在kernel/irq/irq.c中完善`handle_timer_irq`函数，递减当前运行线程的时间片budget，并调用sched函数触发调度。 请在`kernel/sched/policy_rr.c`中完善`rr_sched`函数，在将当前运行线程重新加入就绪队列之前，恢复其调度时间片budget为DEFAULT_BUDGET。

> [!SUCCESS]
> 在完成填写之后，运行 ChCore 将可以成功进入用户态并打断创建的“自旋线程”让内核和主线程可以拿回CPU核心的控制权，你可以看到输出`"Hello, I am thread 3. I'm spinning."`和`“Thread 1 successfully regains the control!”`并通过 `Preemptive Scheduling` 测试点。

--- 

> [!SUCCESS]
> 以上为Lab4 Part2的所有内容
