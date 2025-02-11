# 目录

- [多核调度](#多核调度)
- [调度API](#调度api)
  - [调度器数据结构](#调度器数据结构)
  - [调度器API实现——以RR为例](#调度器api实现以rr为例)
    - [初始化](#初始化)
    - [入队操作](#入队操作)
    - [出队操作](#出队操作)
    - [rr策略实现](#rr策略实现)
- [协作式调度](#协作式调度)
- [抢占式调度](#抢占式调度)
  - [物理时钟初始化](#物理时钟初始化)
  - [物理时钟中断与抢占](#物理时钟中断与抢占)

# 多核调度

上一部分已经讲解了ChCore对多核支持的实现与多核的启动具体方式逻辑，本部分则来具体讲解ChCore对多核调度的实现——这是我们在ICS中也学过的内容，现在让我们把知识与ChCore的具体实现结合起来看看吧！

# 调度API

## 调度器数据结构

类比我们在 `printf` 的系统调用链中提到的 `fd_ops` 结构体，与调度相关的操作我们也有特定的数据结构 `sched_ops` 来表示：

```c
// /kernel/include/sched.h

struct sched_ops {
        /* 调度器初始化函数
         * 在系统启动时调用，用于：
         * - 初始化调度器数据结构
         * - 设置初始调度参数
         * - 准备调度器运行环境
         * 返回值：成功返回0，失败返回错误码
         */
        int (*sched_init)(void);

        /* 核心调度函数
         * 选择下一个要运行的线程，在以下情况下调用：
         * - 当前线程主动放弃CPU
         * - 时间片用完
         * - 系统调用后
         * 返回值：成功返回0，失败返回错误码
         */
        int (*sched)(void);

        /* 周期性调度函数
         * 在每个时钟中断时被调用，用于：
         * - 更新线程时间片
         * - 检查是否需要重新调度
         * - 维护调度统计信息
         * 返回值：如果需要重新调度返回1，否则返回0
         */
        int (*sched_periodic)(void);

        /* 将线程加入调度队列
         * @param thread: 要加入队列的线程
         * 使用场景：
         * - 线程创建时
         * - 线程从阻塞状态恢复
         * - 线程被唤醒时
         * 返回值：成功返回0，失败返回错误码
         */
        int (*sched_enqueue)(struct thread *thread);

        /* 将线程从调度队列中移除
         * @param thread: 要移除的线程
         * 使用场景：
         * - 线程退出时
         * - 线程进入阻塞状态
         * - 线程被挂起时
         * 返回值：成功返回0，失败返回错误码
         */
        int (*sched_dequeue)(struct thread *thread);

        /* 调试工具：显示调度器状态
         * 用于调试和监控：
         * - 打印当前调度队列状态
         * - 显示线程运行统计信息
         * - 输出调度器性能指标
         * 无返回值
         */
        void (*sched_top)(void);
};
```

它本质上是一个函数指针的集合，里面囊括了我们实现一个调度器所需要的基本函数操作:

- `sched_init` ：初始化调度器
- `sched` ：进行一次调度。即将正在运行的线程放回就绪队列，然后在就绪队列中选择下一个需要执行的线程返回
- `sched_enqueue` ：将新线程添加到调度器的就绪队列中
- `sched_dequeue` ：从调度器的就绪队列中取出一个线程
- `sched_top` ：用于debug,打印当前所有核心上的运行线程以及等待线程的函数

那么不同的调度策略又是如何实现呢？只需要在编译ChCore的时候指定相应的config即可，这会在初始化该结构体的时候体现出来

## 调度器API实现——以RR为例

### 初始化

我们首先会在内核初始化的 `main` 函数中调用一个统一的 `sched_init` 函数：

```c
void main(paddr_t boot_flag, void *info)
{

	// ...

	/* Init scheduler with specified policy */
#if defined(CHCORE_KERNEL_SCHED_PBFIFO)
	sched_init(&pbfifo);
#elif defined(CHCORE_KERNEL_RT) || defined(CHCORE_OPENTRUSTEE)
	sched_init(&pbrr);
#else
	sched_init(&rr);
#endif
	kinfo("[ChCore] sched init finished\n");
	
	// ...
	
}

```

这里传入的调度策略即为一个个 `sched_ops` 结构体的实例，我们将在这里的 `sched_init` （不是结构体里的）将全局变量 `cur_sched_ops` 设置为传入的引用

```c
/* Provided Scheduling Policies */
extern struct sched_ops pbrr; /* Priority Based Round Robin */
extern struct sched_ops pbfifo; /* Priority Based FIFO */
extern struct sched_ops rr; /* Simple Round Robin */

/* Chosen Scheduling Policies */
extern struct sched_ops *cur_sched_ops;
```

```c
// /kernel/sched/sched.c
int sched_init(struct sched_ops *sched_ops)
{
        BUG_ON(sched_ops == NULL);

        init_idle_threads();
        init_current_threads();
        init_resched_bitmaps();

        cur_sched_ops = sched_ops;
        cur_sched_ops->sched_init();

        return 0;
}

// /kernel/sched/policy_rr.c
struct sched_ops rr = {.sched_init = rr_sched_init,
                       .sched = rr_sched,
                       .sched_periodic = rr_sched,
                       .sched_enqueue = rr_sched_enqueue,
                       .sched_dequeue = rr_sched_dequeue,
                       .sched_top = rr_top};
```

所以真正初始化的地方是依据具体的策略而定的，我们这里以rr为例，来到 `rr_sched_init` ：

```c
int rr_sched_init(void)
{
        int i = 0;

        /* Initialize global variables */
        for (i = 0; i < PLAT_CPU_NUM; i++) {
				        // 初始化队列链表头
                init_list_head(&(rr_ready_queue_meta[i].queue_head));
                // 初始化锁
                lock_init(&(rr_ready_queue_meta[i].queue_lock));
                // 初始化队列长度为0
                rr_ready_queue_meta[i].queue_len = 0;
        }

        return 0;
}
```

整体很简单，就是为每个核心初始化其相应的调度队列和锁，其中又涉及到 `queue_meta` 数据结构：

```c
/* 就绪队列元数据结构 */
struct queue_meta {
        struct list_head queue_head;    // 就绪队列链表头
        unsigned int queue_len;         // 队列长度
        struct lock queue_lock;         // 队列锁
        char pad[pad_to_cache_line(sizeof(unsigned int)
                                   + sizeof(struct list_head)
                                   + sizeof(struct lock))];  // 缓存行对齐填充
};

/* 每个CPU的就绪队列数组 */
struct queue_meta rr_ready_queue_meta[PLAT_CPU_NUM];
```

队列依旧使用双向链表实现，初始化之后还需要把队列长度 `queue_len` 设置为0，结束初始化

### 入队操作

我们同样来看rr策略下对入队操作的实现：

```c
/*
 * Sched_enqueue
 * Put `thread` at the end of ready queue of assigned `affinity` and `prio`.
 * If affinity = NO_AFF, assign the core to the current cpu.
 * If the thread is IDEL thread, do nothing!
 */
int rr_sched_enqueue(struct thread *thread)
{
        BUG_ON(!thread);
        BUG_ON(!thread->thread_ctx);
        if (thread->thread_ctx->type == TYPE_IDLE)
                return 0;

        int cpubind = 0;
        unsigned int cpuid = 0;
        int ret = 0;

        cpubind = get_cpubind(thread);
        cpuid = cpubind == NO_AFF ? rr_sched_choose_cpu() : cpubind;

        if (unlikely(thread->thread_ctx->sc->prio > MAX_PRIO))
                return -EINVAL;

        if (unlikely(cpuid >= PLAT_CPU_NUM)) {
                return -EINVAL;
        }

        lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        ret = __rr_sched_enqueue(thread, cpuid);
        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
        return ret;
}

int __rr_sched_enqueue(struct thread *thread, int cpuid)
{
        if (thread->thread_ctx->type == TYPE_IDLE) {
                return 0;
        }

        /* Already in the ready queue */
        if (thread_is_ts_ready(thread)) {
                return -EINVAL;
        }

        thread->thread_ctx->cpuid = cpuid;
        thread_set_ts_ready(thread);
        obj_ref(thread);
        list_append(&(thread->ready_queue_node),
                    &(rr_ready_queue_meta[cpuid].queue_head));
        rr_ready_queue_meta[cpuid].queue_len++;

        return 0;
}
```

发现这两个函数其实只是一个框架，包括的是各种corner case以及参数检查等。考虑到ChCore的多核特性，选择入队的核心才是关键所在，不过在这里我们并没有采用很复杂的算法：

```c
/* A simple load balance when enqueue threads */
unsigned int rr_sched_choose_cpu(void)
{
        unsigned int i, cpuid, min_rr_len, local_cpuid, queue_len;

        local_cpuid = smp_get_cpu_id();
        min_rr_len = rr_ready_queue_meta[local_cpuid].queue_len;

        if (min_rr_len <= LOADBALANCE_THRESHOLD) {
                return local_cpuid;
        }

        /* Find the cpu with the shortest ready queue */
        cpuid = local_cpuid;
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                if (i == local_cpuid) {
                        continue;
                }

                queue_len =
                        rr_ready_queue_meta[i].queue_len + MIGRATE_THRESHOLD;
                if (queue_len < min_rr_len) {
                        min_rr_len = queue_len;
                        cpuid = i;
                }
        }

        return cpuid;
}
```

这里用的选择算法是简单负载均衡，也即：

- 若当前CPU队列长度低于负载阈值，则选择当前CPU，可提高缓存亲和性
- 否则选择最短队列的CPU，这里还要加上迁移开销，具体数量参考宏定义

```c
/* The config can be tuned. */
#define LOADBALANCE_THRESHOLD 5
#define MIGRATE_THRESHOLD     5
```

加锁方面也是简单粗暴地直接加大锁，暂时没有什么优化

### 出队操作

出队不需要选择CPU，所以直接出就行：

```c
/*
 * remove @thread from its current residual ready queue
 */
int rr_sched_dequeue(struct thread *thread)
{
        BUG_ON(!thread);
        BUG_ON(!thread->thread_ctx);
        /* IDLE thread will **not** be in any ready queue */
        BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

        unsigned int cpuid = 0;
        int ret = 0;

        cpuid = thread->thread_ctx->cpuid;
        lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        ret = __rr_sched_dequeue(thread);
        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
        return ret;
}

/* dequeue w/o lock */
int __rr_sched_dequeue(struct thread *thread)
{
        /* IDLE thread will **not** be in any ready queue */
        BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

        if (!thread_is_ts_ready(thread)) {
                kwarn("%s: thread state is %d\n",
                      __func__,
                      thread->thread_ctx->state);
                return -EINVAL;
        }

        list_del(&(thread->ready_queue_node));
        rr_ready_queue_meta[thread->thread_ctx->cpuid].queue_len--;
        obj_put(thread);
        return 0;
}
```

注意各种corner case的判断，参数检查，以及更新对象计数等必须操作

### rr策略实现

上面所述皆为对调度队列的基本操作，现在我们再来看看对Round Robin轮转策略具体实现：

```c
/*
 * Schedule a thread to execute.
 * current_thread can be NULL, or the state is TS_RUNNING or
 * TS_WAITING/TS_BLOCKING. This function will suspend current running thread, if
 * any, and schedule another thread from
 * `(rr_ready_queue_meta[cpuid].queue_head)`.
 * ***the following text might be outdated***
 * 1. Choose an appropriate thread through calling *chooseThread* (Simple
 * Priority-Based Policy)
 * 2. Update current running thread and left the caller to restore the executing
 * context
 */
int rr_sched(void)
{
        /* WITH IRQ Disabled */
        struct thread *old = current_thread;
        struct thread *new = 0;

        if (old) {
                BUG_ON(!old->thread_ctx);

                /* old thread may pass its scheduling context to others. */
                if (old->thread_ctx->type != TYPE_SHADOW
                    && old->thread_ctx->type != TYPE_REGISTER) {
                        BUG_ON(!old->thread_ctx->sc);
                }

                /* Set TE_EXITING after check won't cause any trouble, the
                 * thread will be recycle afterwards. Just a fast path. */
                /* Check whether the thread is going to exit */
                if (thread_is_exiting(old)) {
                        /* Set the state to TE_EXITED */
                        thread_set_exited(old);
                }

                /* check old state */
                if (!thread_is_exited(old)) {
                        if (thread_is_ts_running(old)) {
                                /* A thread without SC should not be TS_RUNNING.
                                 */
                                BUG_ON(!old->thread_ctx->sc);
                                if (old->thread_ctx->sc->budget != 0
                                    && !thread_is_suspend(old)) {
                                        switch_to_thread(old);
                                        return 0; /* no schedule needed */
                                }
                                rr_sched_refill_budget(old, DEFAULT_BUDGET);
                                BUG_ON(rr_sched_enqueue(old) != 0);
                        } else if (!thread_is_ts_blocking(old)
                                   && !thread_is_ts_waiting(old)) {
                                kinfo("thread state: %d\n",
                                      old->thread_ctx->state);
                                BUG_ON(1);
                        }
                }
        }

        BUG_ON(!(new = rr_sched_choose_thread()));
        switch_to_thread(new);

        return 0;
}
```

整体上是根据当前线程是否存在做的判断：

- 若当前线程不存在，则直接快进到choose一个新的
- 若当前线程存在，则继续做进一步检查与操作：
    - 调度上下文检查：除了影子线程和寄存器线程外都必须有调度上下文
    - 若当前线程状态是exited，则给它收尸（`thread_set_exited`）
    - 否则进入时间片（`sc->budget`）检查（运行状态的线程）：
        - 若时间片没用尽，线程也未被挂起，则继续运行
        - 若时间片已经用尽，则重新设置其时间片，并再次入队
    - 对非运行状态的线程作异常处理
- 最后的 `switch_to_thread` 是内核态的切换线程函数，它只负责 current_thread 等变量，并没有设置完整的上下文切换，所以需要搭配其他函数来完成(返回用户态的实例在 `sched_to_thread` ， 内部比较复杂，可能跨核调度，此时需要通过ipi（体系结构特定的处理器间中断）来等待目标cpu核处理好当前的中断等事件释放内核栈， 再进行体系结构特定的上下文切换（寄存器的save/load）和用户态返回)

其中调度上下文是如下数据结构：

```c
typedef struct sched_context {
        unsigned int budget; // 时间片预算
        unsigned int prio; // 线程优先级
} sched_ctx_t;
```

最后再来看看rr策略是如何选择新的可执行的线程吧，这个函数比较长，所以添加了相关的注释：

```c
struct thread *rr_sched_choose_thread(void)
{
        unsigned int cpuid = smp_get_cpu_id();
        struct thread *thread = NULL;

        /* 检查当前CPU的就绪队列是否为空 */
        if (!list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                /* 获取队列锁，保护并发访问 */
                lock(&(rr_ready_queue_meta[cpuid].queue_lock));
                
        again:  /* 重试标签：处理无效线程的情况 */
                /* 双重检查队列是否为空（可能在获取锁的过程中被清空） */
                if (list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;  // 队列为空，返回空闲线程
                }

                /* 尝试找到一个可运行的线程
                 * 处理内核栈被其他核心使用的情况
                 */
                if (!(thread = find_runnable_thread(
                              &(rr_ready_queue_meta[cpuid].queue_head)))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;  // 没有可运行线程，返回空闲线程
                }

                /* 从就绪队列中移除选中的线程 */
                BUG_ON(__rr_sched_dequeue(thread));

                /* 处理退出状态的线程 */
                if (thread_is_exiting(thread) || thread_is_exited(thread)) {
                        thread_set_exited(thread);
                        goto again;  // 重新选择线程
                }

                unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                return thread;  // 返回选中的线程
        }

out:    /* 就绪队列为空，返回对应CPU的空闲线程 */
        return &idle_threads[cpuid];
}

struct thread *find_runnable_thread(struct list_head *thread_list)
{
        struct thread *thread;

        /* 遍历就绪队列中的所有线程 */
        for_each_in_list (
                thread, struct thread, ready_queue_node, thread_list) {
                /* 检查线程是否满足运行条件：
                 * 1. 未被挂起 (!thread_is_suspend)
                 * 2. 内核栈可用 (KS_FREE)
                 *    或者是当前线程 (current_thread)
                 */
                if (!thread_is_suspend(thread)
                    && (thread->thread_ctx->kernel_stack_state == KS_FREE
                        || thread == current_thread)) {
                        return thread;
                }
        }
        return NULL;  // 没有找到合适的线程
}
```

注意一共有两次判空，第一次判空后才拿的锁，以减少轻工作状态下的锁竞争

拿锁后遍历自己核心上的可执行队列，找一个当前状态为free的或者就是当前的thread，如果没找到则返回idle空闲线程

# 协作式调度

协作式调度，即需要用户态程序主动配合"让出CPU"，反映到系统调用上即为 `sys_yield`

```c
/* SYSCALL functions */
void sys_yield(void)
{
        current_thread->thread_ctx->sc->budget = 0;
        sched();
        eret_to_thread(switch_context());
}
```

这里的 sched 即为rr策略下的调度函数，是用结构体模拟的重载：

```c
static inline int sched(void)
{
        return cur_sched_ops->sched();
}
```

注意 `sched` 里的切换线程是不完整的，因此还需要切换上下文并 `eret` ，我们来看源码：

看函数开头前的注释也可以明白其用法

```c
/*
 * This function is used after current_thread is set (a new thread needs to be
 * scheduled).
 *
 * Switch context between current_thread and current_thread->prev_thread:
 * including: vmspace, fpu, tls, ...
 *
 * Return the context pointer which should be set to stack pointer register.
 */
vaddr_t switch_context(void)
{
        struct thread *target_thread;    // 目标线程
        struct thread_ctx *target_ctx;   // 目标线程上下文
        struct thread *prev_thread;      // 前一个线程

        /* 1. 基本检查 */
        target_thread = current_thread;
        if (!target_thread || !target_thread->thread_ctx) {
                kwarn("%s no thread_ctx", __func__);
                return 0;
        }

        target_ctx = target_thread->thread_ctx;
        prev_thread = target_thread->prev_thread;

        /* 2. 特殊情况：切换到自己 */
        if (prev_thread == THREAD_ITSELF)
                return (vaddr_t)target_ctx;

        /* 3. FPU状态处理 */
#if FPU_SAVING_MODE == EAGER_FPU_MODE
        /* 积极模式：立即保存和恢复FPU状态 */
        save_fpu_state(prev_thread);
        restore_fpu_state(target_thread);
#else
        /* 懒惰模式：仅在必要时处理FPU */
        if (target_thread->thread_ctx->type > TYPE_KERNEL)
                disable_fpu_usage();
#endif

        /* 4. 切换线程本地存储(TLS) */
        switch_tls_info(prev_thread, target_thread);

        /* 5. 虚拟内存空间切换 */
#ifndef CHCORE_KERNEL_TEST
        /* 正常情况下的虚拟内存处理 */
        BUG_ON(!target_thread->vmspace);
        /* 记录线程运行的CPU：用于TLB维护 */
        record_history_cpu(target_thread->vmspace, smp_get_cpu_id());
        /* 如果需要，切换虚拟内存空间 */
        if ((!prev_thread) || (prev_thread->vmspace != target_thread->vmspace))
                switch_thread_vmspace_to(target_thread);
#else
        /* 测试模式下的特殊处理 */
        if (target_thread->thread_ctx->type != TYPE_TESTS) {
                BUG_ON(!target_thread->vmspace);
                record_history_cpu(target_thread->vmspace, smp_get_cpu_id());
                switch_thread_vmspace_to(target_thread);
        }
#endif

        /* 6. 架构相关的上下文切换 */
        arch_switch_context(target_thread);

        /* 7. 返回目标上下文地址 */
        return (vaddr_t)target_ctx;
}

void arch_switch_context(struct thread *target)
{
	struct per_cpu_info *info;

	info = get_per_cpu_info();

	info->cur_exec_ctx = (u64)target->thread_ctx; // 设置当前CPU信息
}
```

整体来看需要切换的东西还是很多的：

- `vmspace`  ：这个结构在thread里面(还记得这的核心数据机构是一个rbtree的root吗， 回看内存管理)，但是需要切换页表（页表的地址也在vmspace之中维护）
- `tls` ：(thread local storage， 在arm架构的典型实现之中是TPIDR_EL0寄存器，它存着一个线程特定的标识符)
    - OS其实不知道这里面存的是什么东西，他只是把这个寄存器当成线程特定的标识符，并在自己的线程实现之中维护而已。至于语言层面的tls如何实现，那是编译器开发者或者库开发者的事情，例如存一个特定的空间的指针（例如小的在栈上，大的在堆上）
    - arm compiler的支持文档 https://developer.arm.com/documentation/dui0205/g/arm-compiler-reference/compiler-specific-features/thread-local-storage
    - 对tls的一些讨论 https://forum.osdev.org/viewtopic.php?t=36597
- `fpu` 相关
- 其他想要添加的机制，例如保存和清理TLB的一些数据（history）等
- 把切换的线程相关的寄存器设置到cpu上，见 `arch_switch_context`

# 抢占式调度

协作式调度需要用户态程序自己"体面地"让出CPU，那它要不体面怎么办？我们就来帮它体面，这便是抢占式调度，典型应用场景即如Lab文档所述：

> ChCore启动的第一个用户态线程（执行`user/system-services/system-servers/procmgr/procmgr.c`的`main`函数）将创建一个"自旋线程"，该线程在获得CPU核心的控制权后便会执行无限循环，进而导致无论是该程序的主线程还是ChCore内核都无法重新获得CPU核心的控制权。就保护系统免受用户程序中的错误或恶意代码影响而言，这一情况显然并不理想，任何用户应用线程均可以如该"自旋线程"一样，通过进入无限循环来永久"霸占"整个CPU核心
> 

而抢占式首先要支持的就是时钟中断。时钟中断的支持实际上和其他外设也没什么区别，抽象起来就是对寄存器的读和写，以及配置真正连接cpu引脚的发信号时间等硬件相关的操作

## 物理时钟初始化

回归主线，Lab文档也说了相关寄存器的信息：

> 我们需要处理的系统寄存器如下([Refer](https://developer.arm.com/documentation/102379/0101/The-processor-timers)):
> 
> - CNTPCT_EL0: 它的值代表了当前的 system count。
> - CNTFRQ_EL0: 它的值代表了物理时钟运行的频率，即每秒钟 system count 会增加多少。
> - CNTP_CVAL_EL0: 是一个64位寄存器，操作系统可以向该寄存器写入一个值，当 system count 达到或超过该值时，物理时钟会触发中断。
> - CNTP_TVAL_EL0: 是一个32位寄存器，操作系统可以写入 TVAL，处理器会在内部读取当前的系统计数，加上写入的值，然后填充 CVAL。
> - CNTP_CTL_EL0: 物理时钟的控制寄存器，第0位ENABLE控制时钟是否开启，1代表enble，0代表disable；第1位IMASK代表是否屏蔽时钟中断，0代表不屏蔽，1代表屏蔽。

那么其初始化函数便好理解了：

```c
void plat_timer_init(void)
{
        u64 count_down = 0;
        u64 timer_ctl = 0;
        u32 cpuid = smp_get_cpu_id();

        /* 1. 读取系统计数器和频率*/
        asm volatile ("mrs %0, cntpct_el0":"=r" (cntp_init));    // 获取当前计数值
        kdebug("timer init cntpct_el0 = %lu\n", cntp_init);
        asm volatile ("mrs %0, cntfrq_el0":"=r" (cntp_freq));    // 获取计数器频率
        kdebug("timer init cntfrq_el0 = %lu\n", cntp_freq);

        /* 2. 计算定时器值*/
        cntp_tval = (cntp_freq * TICK_MS / 1000);      // 计算多少个计数对应一个时钟滴答
        tick_per_us = cntp_freq / 1000 / 1000;         // 计算每微秒的计数值
        kinfo("CPU freq %lu, set timer %lu\n", cntp_freq, cntp_tval);

        /* 3. 设置定时器计数值*/
        asm volatile ("msr cntp_tval_el0, %0"::"r" (cntp_tval));  // 设置计数值
        asm volatile ("mrs %0, cntp_tval_el0":"=r" (count_down));  // 读回验证
        kdebug("timer init cntp_tval_el0 = %lu\n", count_down);

        /* 4. 启用定时器中断
         * CNTPNSIRQ: Physical Non-secure timer interrupt
         * CNTVIRQ: Virtual timer interrupt
         */
        put32(core_timer_irqcntl[cpuid],               // 每个CPU核心独立的中断控制
              INT_SRC_TIMER1 | INT_SRC_TIMER3);        // 使能物理定时器和虚拟定时器中断

        /* 5. 配置控制寄存器
         * IMASK = 0: 不屏蔽中断
         * ENABLE = 1: 使能定时器
         * cntp_ctl_el0: Counter-timer Physical Timer Control register
         */
        timer_ctl = 0 << 1 | 1;	/* IMASK = 0 ENABLE = 1 */
        asm volatile ("msr cntp_ctl_el0, %0"::"r" (timer_ctl));   // 设置控制寄存器
        asm volatile ("mrs %0, cntp_ctl_el0":"=r" (timer_ctl));   // 读回验证
        kdebug("timer init cntp_ctl_el0 = %lu\n", timer_ctl);
}
```

## 物理时钟中断与抢占

如何实现中断物理时钟后对CPU的"抢占"？核心便是**在处理时钟中断时递减当前运行线程的时间片，并在当前运行线程的时间片耗尽时进行调度，选取新的线程运行**

来到中断处理的代码：

```c
/* Interrupt handler for interrupts happening when in EL0. */
void handle_irq(void)
{
	plat_handle_irq();
	sched_periodic(); // 在rr策略下即为调度函数，但其他策略下不一样，需要注意
	eret_to_thread(switch_context());// 这个操作即为调度后返回用户态的标准
}
```

继续前进，直接来看处理物理时钟中断的部分：

```c
void plat_handle_irq(void)
{
	// ...	
	switch (irq) {
	case INT_SRC_TIMER1:
		/* CNTPNSIRQ (Physical Non-Secure timer IRQ) */
		handle_timer_irq();
	// ...
	return;
}
```

```c
void handle_timer_irq(void)
{
        u64 current_tick, tick_delta;
        struct time_state *local_time_state;
        struct list_head *local_sleep_list;
        struct lock *local_sleep_list_lock;
        struct sleep_state *iter = NULL, *tmp = NULL;
        struct thread *wakeup_thread;

        /* 获取当前CPU的睡眠队列信息 */
        current_tick = plat_get_current_tick();         // 获取当前时钟计数
        local_time_state = &time_states[smp_get_cpu_id()];
        local_sleep_list = &local_time_state->sleep_list;
        local_sleep_list_lock = &local_time_state->sleep_list_lock;

        /* 遍历并唤醒到期的线程 */
        lock(local_sleep_list_lock);  // 加锁保护睡眠队列
        for_each_in_list_safe (iter, tmp, sleep_node, local_sleep_list) {
                if (iter->wakeup_tick > current_tick) {
                        break;  // 未到唤醒时间，退出循环
                }
                wakeup_thread = container_of(iter, struct thread, sleep_state);
                lock(&wakeup_thread->sleep_state.queue_lock);
                list_del(&iter->sleep_node);
                BUG_ON(wakeup_thread->sleep_state.cb == sleep_timer_cb
                       && !thread_is_ts_blocking(wakeup_thread));
                kdebug("wake up t:%p at:%ld\n", wakeup_thread, current_tick);
                BUG_ON(wakeup_thread->sleep_state.cb == NULL);
                wakeup_thread->sleep_state.cb(wakeup_thread);
                wakeup_thread->sleep_state.cb = NULL;
                unlock(&wakeup_thread->sleep_state.queue_lock);
        }

        /* 设置下一次定时器中断 */
        tick_delta = get_next_tick_delta();  // 计算下一次中断间隔
        unlock(local_sleep_list_lock);

        /* 更新下次过期时间并配置硬件定时器 */
        time_states[smp_get_cpu_id()].next_expire = current_tick + tick_delta;
        plat_handle_timer_irq(tick_delta);

        /* 递减当前线程的时间片 */
        if (current_thread) {
                BUG_ON(!current_thread->thread_ctx->sc);
                BUG_ON(current_thread->thread_ctx->sc->budget == 0);
                current_thread->thread_ctx->sc->budget--;  // 减少时间片
        } else {
                kdebug("Timer: system not runnig!\n");
        }
}
```

为什么两次中断之间的间隔不是固定值而是需要通过计算得到？

- 时钟不只干定时硬中断的活
- 一些其他线程的等待、睡眠之类会导致下一次时钟irq的提前

支持了抢占式调度之后，我们的用户态程序就能打破初始进程procmgr的循环真正运行了

至此，多核调度部分的源码解析结束