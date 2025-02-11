# Lab 4 的实验笔记
# 目录

- [说明](#说明)
- [多核调度](#多核调度)
  - [Linux调度的底层依赖](#linux调度的底层依赖)
    - [task_struct（任务结构体）](#task_struct任务结构体)
    - [runqueue（运行队列）](#runqueue运行队列)
    - [sched_entity（调度实体）](#sched_entity调度实体)
    - [调度器核心操作（核心调度函数）](#调度器核心操作核心调度函数)
    - [时间片与抢占](#时间片与抢占)
    - [总结](#总结)
  - [Linux的调度策略及其实现](#linux的调度策略及其实现)
    - [CFS（完全公平调度器）](#cfs完全公平调度器)
    - [实时调度策略（Real-time scheduling）](#实时调度策略real-time-scheduling)
    - [SCHED_IDLE](#sched_idle)
    - [SCHED_BATCH](#sched_batch)
- [进程间通信（IPC)](#进程间通信ipc)
  - [1. 管道（Pipe）](#1-管道pipe)
  - [2. 命名管道（FIFO）](#2-命名管道fifo)
  - [3. 消息队列（Message Queue）](#3-消息队列message-queue)
  - [4. 共享内存（Shared Memory）](#4-共享内存shared-memory)
  - [5. 信号量（Semaphore）](#5-信号量semaphore)
  - [6. 套接字（Socket）](#6-套接字socket)
  - [7. 信号（Signal）](#7-信号signal)
- [相关阅读](#相关阅读)
  - [Linux的调度](#linux的调度)
    - [论文](#论文)
    - [博客](#博客)
  - [Linux的IPC 机制](#linux的ipc-机制)
    - [论文](#论文-1)
    - [博客](#博客-1)


# 说明

本文包括 Linux 调度策略的介绍 ， Linux IPC机制的介绍，并为读者提供相关阅读材料。读者阅读之后可以思考Linux 与 Chcore 在调度以及IPC 方面的不同之处以及why。

# 多核调度

## Linux调度的底层依赖

Linux调度器是一个基于抢占和时间片轮转机制的复杂调度系统。它的底层依赖主要集中在以下几个关键的数据结构和机制中。

### task_struct（任务结构体）

task_struct是Linux内核中表示一个进程或线程的核心数据结构，它包含了进程的所有信息，调度器使用它来决定调度哪些进程或线程执行，跟踪进程的状态，管理进程的优先级、时间片等。例如task_struct不仅包含进程的执行状态（如TASK_RUNNING、TASK_SLEEPING），还包含任务优先级、调度实体等信息，调度器利用这些信息来决定哪个任务在何时运行。

```c
struct task_struct {
    ...
    unsigned int state;            // 任务状态（在调度中判断是否可运行）
    int priority;                  // 任务的优先级（影响调度顺序）
    int time_slice;                // 分配的时间片长度（用于控制每个任务的执行时间）
    struct list_head run_list;     // 在运行队列中的链表（调度器用来排序）
    struct sched_entity se;        // 调度实体，包含虚拟运行时间等调度信息
    ...
};
```
相关注释：

**state**: 进程的当前状态，调度器通过state来判断任务是否可以被调度（例如TASK_RUNNING表示进程可以调度）。

**priority**: 进程的优先级，调度器根据优先级来决定任务的调度顺序。

**run_list**: 链接任务在各个调度队列中的位置，调度器通过它来遍历队列并选择任务执行。

**sched_entity**: sched_entity包含任务的调度实体，决定任务如何进入调度队列以及如何被调度。

### **runqueue（运行队列）**

runqueue是每个CPU核心的运行队列，用来管理该核心上所有准备执行的任务。Linux调度器通过runqueue来组织和调度任务。

```c
struct rq {
    struct list_head queue;        // 就绪队列，保存当前CPU核心上的所有可调度任务
    int nr_running;                // 当前运行队列中可运行的任务数
    struct task_struct *curr;      // 当前正在运行的任务
    ...
};
```

相关注释：

**queue**: queue是一个双向链表，用来按优先级或其他策略存储任务。调度器从队列中挑选最合适的任务执行。

**nr_running**: 记录队列中正在运行的任务数量，调度器可以使用它来确定是否有足够的任务在队列中等待调度。

**curr**: 指向当前正在运行的任务，调度器需要使用它来了解当前CPU核心正在运行哪个任务。

### **sched_entity（调度实体）**

sched_entity是调度器内部的核心数据结构之一，代表一个任务在调度过程中的实体，它包含了该任务的运行时间、调度权重、时间片等信息。

```c
struct sched_entity {
    unsigned int exec_start;        // 任务开始执行的时间
    unsigned int sum_exec_runtime;  // 任务已执行的总时间
    unsigned int period;            // 任务的时间周期
    unsigned int weight;            // 任务的权重（优先级等）
    ...
};
```

相关注释：

**exec_start**: 任务开始执行的时间戳，用于计算任务的执行时间。

**sum_exec_runtime**: 任务已执行的累计时间，调度器通过这个信息来判断任务是否需要调度。

**weight**: 任务的权重，决定任务在调度中的优先级，调度器根据weight来决定任务执行的频率和顺序。

### **调度器核心操作（核心调度函数）**

调度器的核心函数用于管理任务的调度流程。这些函数通过操作上述数据结构来实现任务的入队、出队、选择执行任务等功能。

**schedule()**：这是调度器的核心函数，负责选择下一个任务并执行任务切换。在此函数中，调度器根据当前CPU核心的runqueue选择下一个要执行的任务，并通过switch_to()函数切换到该任务。

```c
void schedule(void) {
    struct task_struct *next_task = pick_next_task();  // 选择下一个要执行的任务
    switch_to(next_task);  // 切换到选中的任务
}
```

相关注释：

- task_struct：task_struct是Linux内核中表示一个进程或线程的核心数据结构，包含进程的状态、上下文信息、优先级、时间片等。pick_next_task()函数返回的next_task即为下一个要运行的任务，switch_to()函数会利用这个task_struct来进行上下文切换。
- pick_next_task()：该函数从就绪队列中选择下一个任务，因此 schedule() 函数依赖于 pick_next_task() 来进行任务选择。

**pick_next_task()**：用于从runqueue中挑选出一个任务，在时间片到期或其他事件发生时进行任务切换。它是任务调度过程的第一步，决定了当前CPU要执行哪个任务。

```c
struct task_struct *pick_next_task(struct rq *rq) {
    struct task_struct *next = list_first_entry(&rq->queue, struct task_struct, run_list);
    return next;  // 返回队列中第一个任务
}
```

相关注释：

- rq（**runqueue**）：runqueue是每个CPU核心的就绪队列，存储所有可以运行的任务。它是一个结构体，包含一个双向链表queue来存储任务。在pick_next_task()函数中，rq->queue表示当前CPU上所有可调度任务的队列。
- list_first_entry(&rq->queue, struct task_struct, run_list)：该宏从runqueue中的任务链表（queue）获取第一个任务。run_list是task_struct中的链表节点，它指向任务在就绪队列中的位置。

**enqueue_task()**：将任务加入到队列中，使任务能够被调度器选中。任务可能是在创建时添加到队列中，或者在任务从阻塞状态恢复时重新加入队列。

```c
void enqueue_task(struct rq *rq, struct task_struct *p) {
    list_add_tail(&p->run_list, &rq->queue);  // 将任务加入到队列的尾部
}
```

相关注释：

- rq（**runqueue**）：runqueue表示当前CPU的就绪队列，rq->queue是一个双向链表，存储所有当前CPU上可运行的任务。
- list_add_tail(&p->run_list, &rq->queue)：这是Linux内核提供的一个宏，用于将一个任务（p）添加到队列（rq->queue）的尾部。run_list是task_struct中的链表节点，它指向任务在就绪队列中的位置。
- task_struct：每个任务都包含run_list成员，用于在队列中挂载任务。enqueue_task()函数将任务节点（run_list）添加到队列的尾部，表示任务已经准备好执行，等待调度。

**dequeue_task()**：将一个任务（p）从当前CPU的就绪队列中移除，通常在任务完成或阻塞时进行。

```c
void dequeue_task(struct rq *rq, struct task_struct *p) {
    list_del(&p->run_list);  // 将任务从队列中移除
}
```

相关注释：

- rq（**runqueue**）：rq表示当前CPU的就绪队列，rq->queue是一个双向链表，存储当前CPU上的所有可调度任务。
- list_del(&p->run_list)：这是Linux内核提供的宏，用于从链表中删除一个任务节点。run_list是task_struct中的链表成员，表示任务在就绪队列中的位置。dequeue_task()函数通过这个宏将任务从队列中移除，表示该任务已经不再可调度（如执行完毕或被阻塞）。
- task_struct：每个任务都包含run_list成员，用来在调度队列中定位任务。dequeue_task()会通过list_del()将task_struct中的run_list节点从就绪队列中移除。

### **时间片与抢占**

时间片和抢占是调度器的核心概念，用来确保公平调度和响应实时任务。每个任务被分配一定的时间片，时间片耗尽后任务会被重新调度。

**时间片**：每个任务都会分配一个时间片，调度器使用它来控制任务的执行时长。

**抢占**：当一个任务的时间片耗尽时，调度器会抢占任务并调度下一个任务。

**scheduler_tick()**：此函数每次时钟中断触发时检查当前任务的时间片，若时间片耗尽则调用schedule()执行任务切换。

```c
void scheduler_tick(void) {
    if (--current->time_slice == 0) {
        schedule();  // 时间片耗尽，执行调度
    }
}
```

### 总结

在Linux调度系统中，底层数据结构（如task_struct、runqueue、sched_entity）起到了组织和管理任务的作用，它们帮助调度器高效地进行任务的选择和切换。调度核心函数（如schedule()、pick_next_task()）操作这些数据结构来实现不同的调度策略，保证操作系统可以公平且高效地管理多任务环境。

## L**inux的调度策略及其实现**

### **CFS（完全公平调度器）**

CFS（Completely Fair Scheduler）是Linux的默认调度策略，旨在提供每个进程公平的CPU时间。CFS采用红黑树来管理就绪队列，每个任务按其"虚拟运行时间"（vruntime）排序。虚拟运行时间是根据任务的优先级和实际运行时间计算出来的，目的是让每个任务获得一个公平的时间片。

**关键特性：**

- **虚拟运行时间**：每个任务有一个vruntime值，它衡量了任务消耗CPU的时间。CFS会优先选择虚拟运行时间最小的任务。
- **红黑树**：CFS使用红黑树数据结构来管理就绪队列，能够高效地选择和调整下一个执行的任务。
- **公平性**：任务的vruntime会随着运行而增加，vruntime越小的任务越早被调度。

**原理：**

在 CFS 中，vruntime 的计算公式大致是将任务的实际运行时间和任务的优先级相结合来动态调整任务的调度顺序，具体来说，vruntime 随着任务的实际运行时间增加而递增，但任务的优先级会影响该增量的大小，优先级较高的任务（vruntime 较小）会被调度得更频繁，而优先级较低的任务则会相对延迟调度，这种方式保证了每个任务按照公平的原则来获得 CPU 时间，避免了长时间占用 CPU 的情况，进而提高了系统的响应性和公平性。

CFS 使用红黑树来管理就绪队列。任务按照 vruntime 值存储在红黑树中，红黑树的特点是可以在对数时间内进行插入、删除和查找操作，这对于调度器非常重要，因为调度需要快速找到 vruntime 最小的任务。

相关代码：

```c
// CFS调度器：选择下一个要执行的任务
// 通过从红黑树中选取vruntime最小的任务，来决定下一个执行的任务
struct task_struct *pick_next_task_cfs(struct rq *rq) {
    // 获取红黑树中的第一个节点（即 vruntime 最小的任务）
    struct rb_node *n = rb_first(&rq->cfs_tasks);
    if (!n) return NULL;  // 如果红黑树为空，返回 NULL
    struct task_struct *next = rb_entry(n, struct task_struct, rb_node);
    return next;  // 返回最小 vruntime 的任务
}

// CFS调度器：将任务加入 CFS 队列（红黑树）
// 新任务根据其 vruntime 值被插入到红黑树中的合适位置
void enqueue_task_cfs(struct rq *rq, struct task_struct *p) {
    // 插入任务时使用任务的 vruntime 作为键，红黑树自动按此排序
    struct rb_node **new = &(rq->cfs_tasks.rb_node), *parent = NULL;
    
    // 寻找插入位置，保持红黑树的顺序
    while (*new) {
        struct task_struct *this = rb_entry(*new, struct task_struct, rb_node);
        parent = *new;
        // 根据任务的 vruntime 值来决定是插入左子树还是右子树
        if (p->vruntime < this->vruntime)
            new = &(*new)->rb_left;
        else
            new = &(*new)->rb_right;
    }
    // 将新任务的红黑树节点链接到父节点
    rb_link_node(&p->rb_node, parent, new);
    // 将新节点插入红黑树
    rb_insert_color(&p->rb_node, &rq->cfs_tasks);
}

// CFS调度器：将任务从 CFS 队列（红黑树）移除
// 任务不再就绪时，移除其在红黑树中的节点
void dequeue_task_cfs(struct rq *rq, struct task_struct *p) {
    // 从红黑树中删除任务的节点
    rb_erase(&p->rb_node, &rq->cfs_tasks);
}

// CFS调度器：更新任务的虚拟运行时间
// 任务的 vruntime 会根据实际运行时间增加，目的是让运行时间较长的任务，vruntime 增加得更快
void update_task_runtimes(struct task_struct *p) {
    // 更新任务的 vruntime，通常情况下 vruntime 会根据任务的实际运行时间增加
    // 假设每次调度时的时间增量为10
    p->vruntime += 10;  // 这里假设每次调度时的时间增量为10
}
```

### **实时调度策略（Real-time scheduling）**

实时调度策略用于保证某些任务能够按时完成。Linux提供了两个主要的实时调度策略：SCHED_FIFO和SCHED_RR。

- **SCHED_FIFO**：先进先出调度策略，任务按它们到达的顺序执行，不会被其他任务抢占，直到它们完成或者主动放弃CPU。
- **SCHED_RR**：轮转调度策略，SCHED_FIFO的变种，任务在获得CPU后运行一个时间片，然后强制切换到下一个任务。

**关键特性：**

- **SCHED_FIFO**：实时任务在运行时不会被抢占，直到其时间片用完或任务完成。
- **SCHED_RR**：实时任务会轮流执行，每个任务有一个固定的时间片。

相关代码：

**SCHED_FIFO：先进先出调度策略:**

```c
// SCHED_FIFO调度策略：选择下一个要执行的任务
// 任务按它们到达的顺序执行，不会被其他任务抢占
struct task_struct *pick_next_task_fifo(struct rq *rq) {
    struct task_struct *next = NULL;
    
    // 检查队列是否为空
    if (!list_empty(&rq->fifo_tasks)) {
        // 选择队列中的第一个任务，即最先到达的任务
        next = list_first_entry(&rq->fifo_tasks, struct task_struct, run_list);
    }
    return next;
}

// SCHED_FIFO调度策略：将任务加入到 FIFO 队列
// 新任务按到达的顺序加入队列，队列是先进先出的
void enqueue_task_fifo(struct rq *rq, struct task_struct *p) {
    // 将任务加入到队列的尾部，保持队列的顺序
    list_add_tail(&p->run_list, &rq->fifo_tasks);
}

// SCHED_FIFO调度策略：将任务从 FIFO 队列中移除
// 当任务执行完成或者被主动放弃时，将其从队列中移除
void dequeue_task_fifo(struct rq *rq, struct task_struct *p) {
    // 将任务从队列中移除
    list_del(&p->run_list);
}
```

**SCHED_RR：轮转调度策略**

```c
// SCHED_RR调度策略：选择下一个要执行的任务
// 轮转调度策略，任务在获得CPU后运行一个时间片，然后强制切换到下一个任务
struct task_struct *pick_next_task_rr(struct rq *rq) {
    struct task_struct *next = NULL;
    
    // 检查队列是否为空
    if (!list_empty(&rq->rr_tasks)) {
        // 选择队列中的第一个任务（轮转调度，首先执行队列最前面的任务）
        next = list_first_entry(&rq->rr_tasks, struct task_struct, run_list);
    }
    return next;
}

// SCHED_RR调度策略：将任务加入到 RR 队列
// 新任务加入队列时，按轮转顺序进行调度
void enqueue_task_rr(struct rq *rq, struct task_struct *p) {
    // 将任务加入到队列的尾部，保证轮转调度的顺序
    list_add_tail(&p->run_list, &rq->rr_tasks);
}

// SCHED_RR调度策略：将任务从 RR 队列中移除
// 任务完成或被主动放弃时，从队列中移除
void dequeue_task_rr(struct rq *rq, struct task_struct *p) {
    // 将任务从队列中移除
    list_del(&p->run_list);
}

// SCHED_RR调度策略：更新任务的时间片
// 每个任务在运行时间片后会被强制切换，重新加入队列等待下次运行
void update_task_rr(struct task_struct *p) {
    // 假设时间片为 10ms，任务运行结束后，时间片重置
    p->time_slice = 10;  // 为任务分配一个新的时间片
}
```

### **SCHED_IDLE**

SCHED_IDLE策略是用于CPU空闲时的调度。它的主要作用是当系统空闲时，调度一个低优先级的任务，防止系统空闲过长时间，浪费CPU资源。

**关键特性：**

- **CPU空闲时运行**：仅在没有其他任务需要执行时，系统才会调度此类任务。
- **最低优先级**：SCHED_IDLE任务的优先级非常低，因此只有在没有其他任务时才会被调度。

相关代码：

```c
// SCHED_IDLE调度策略：选择下一个要执行的任务
// 空闲调度策略主要用于处理 CPU 空闲的情况。当系统没有其他高优先级任务时，选择 SCHED_IDLE 任务
struct task_struct *pick_next_task_idle(struct rq *rq) {
    struct task_struct *next = NULL;

    // 如果没有其他任务等待执行，选择空闲任务
    if (list_empty(&rq->idle_tasks)) {
        next = NULL;  // 如果没有任务，则返回空
    } else {
        // 如果有空闲任务，选择队列中的第一个空闲任务
        next = list_first_entry(&rq->idle_tasks, struct task_struct, run_list);
    }

    return next;
}

// SCHED_IDLE调度策略：将任务加入到空闲队列
// 如果系统进入空闲状态时，将任务加入到空闲队列中
void enqueue_task_idle(struct rq *rq, struct task_struct *p) {
    // 将任务加入到空闲队列中
    list_add_tail(&p->run_list, &rq->idle_tasks);
}

// SCHED_IDLE调度策略：将任务从空闲队列中移除
// 当任务不再是空闲状态时，将其从队列中移除
void dequeue_task_idle(struct rq *rq, struct task_struct *p) {
    // 将任务从空闲队列中移除
    list_del(&p->run_list);
}

// SCHED_IDLE调度策略：更新任务的运行时间
// 空闲任务的运行时间并不会增加过多，主要目的是避免长时间占用 CPU
void update_task_idle(struct task_struct *p) {
    // 空闲任务的虚拟运行时间不会增加太多，以避免占用 CPU
    p->vruntime += 1;  // 空闲任务的虚拟运行时间增加非常少
}
```

### **SCHED_BATCH**

SCHED_BATCH策略用于批量处理任务，通常用于长时间运行的计算密集型任务。与CFS不同，SCHED_BATCH没有精细的时间片管理，任务可以连续运行较长时间。

**关键特性：**

- **长时间运行**：SCHED_BATCH任务可以长时间占用CPU，不会频繁地进行时间片轮转。
- **不需要高响应性**：适用于长时间运行的计算任务，对响应时间的要求不高

相关代码：

```c
// SCHED_BATCH调度策略：选择下一个要执行的任务
// 批处理任务调度会选择就绪队列中下一个待执行的任务
struct task_struct *pick_next_task_batch(struct rq *rq) {
    struct task_struct *next = NULL;

    // 选择就绪队列中第一个任务作为下一个要执行的任务
    if (!list_empty(&rq->batch_tasks)) {
        next = list_first_entry(&rq->batch_tasks, struct task_struct, run_list);
    }

    return next;
}

// SCHED_BATCH调度策略：将任务加入到批处理队列
// 当一个任务被标记为批处理任务时，加入批处理队列
void enqueue_task_batch(struct rq *rq, struct task_struct *p) {
    // 将任务加入到批处理任务队列
    list_add_tail(&p->run_list, &rq->batch_tasks);
}

// SCHED_BATCH调度策略：将任务从批处理队列中移除
// 当批处理任务不再需要执行时，移除它
void dequeue_task_batch(struct rq *rq, struct task_struct *p) {
    // 将任务从批处理任务队列中移除
    list_del(&p->run_list);
}

// SCHED_BATCH调度策略：更新任务的虚拟运行时间
// 批处理任务的虚拟运行时间增量比其他任务更慢，以减少其对 CPU 资源的争夺
void update_task_batch(struct task_struct *p) {
    // 批处理任务的 vruntime 增加得相对较慢，减少对 CPU 的占用
    p->vruntime += 10;  // 增加较小的虚拟运行时间增量
}
```

注：SCHED_BATCH 调度策略与 SCHED_IDLE 相似，都是为了确保 CPU 资源的合理利用，不过批处理任务通常适用于运行较长时间且对响应时间要求不高的任务。

# 进程间通信（IPC)

在 Linux 系统中，常见的 IPC（进程间通信）机制包括：

1. **管道（Pipe）**
2. **命名管道（FIFO）**
3. **消息队列（Message Queue）**
4. **共享内存（Shared Memory）**
5. **信号量（Semaphore）**
6. **套接字（Socket）**
7. **信号（Signal）**

接下来，我将介绍每种 IPC 机制的原理、设计、优化方向、应用场景，以及底层实现和依赖的数据结构。

## 1. 管道（Pipe）

### 原理和设计，优化方向，应用场景

- **原理和设计**：管道是一种半双工的通信方式，数据只能单向流动。通常用于父子进程之间的通信。管道是基于内核的缓冲区实现的。
- **优化方向**：可以通过增加缓冲区大小来提高数据传输效率，或者使用多个管道实现双向通信。
- **应用场景**：常用于 shell 命令中的管道操作，如 `ls | grep "txt"`。

### 底层实现和底层依赖

- **底层实现**：管道是通过 `pipe()` 系统调用创建的，内核会维护一个环形缓冲区。
- **底层依赖**：依赖于文件描述符和内核缓冲区。

相关代码：

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd[2];
    pid_t pid;
    char buf[128];

    // 创建管道
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    // 创建子进程
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {  // 子进程
        close(fd[1]);  // 关闭写端
        read(fd[0], buf, sizeof(buf));  // 从管道读取数据
        printf("Child received: %s\n", buf);
        close(fd[0]);
    } else {  // 父进程
        close(fd[0]);  // 关闭读端
        const char *msg = "Hello from parent!";
        write(fd[1], msg, strlen(msg) + 1);  // 向管道写入数据
        close(fd[1]);
    }

    return 0;
}
```

## 2. 命名管道（FIFO）

### 原理和设计，优化方向，应用场景

- **原理和设计**：命名管道是一种特殊的文件，允许无亲缘关系的进程通过文件系统路径进行通信。
- **优化方向**：可以通过调整缓冲区大小或使用非阻塞模式来提高性能。
- **应用场景**：适用于需要持久化通信的场景，如日志收集。

### 底层实现和底层依赖

- **底层实现**：通过 `mkfifo()` 系统调用创建，内核维护一个 FIFO 队列。
- **底层依赖**：依赖于文件系统和内核缓冲区。

相关代码：

```c
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char *fifo_path = "/tmp/my_fifo";

    // 创建命名管道
    mkfifo(fifo_path, 0666);

    int fd = open(fifo_path, O_WRONLY);  // 打开写端
    const char *msg = "Hello from writer!";
    write(fd, msg, strlen(msg) + 1);  // 写入数据
    close(fd);

    return 0;
}
```

## 3. 消息队列（Message Queue）

### 原理和设计，优化方向，应用场景

- **原理和设计**：消息队列是一个消息的链表，允许进程通过消息类型进行通信。
- **优化方向**：可以通过调整消息大小和队列长度来优化性能。
- **应用场景**：适用于需要按优先级处理消息的场景，如任务调度。

### 底层实现和底层依赖

- **底层实现**：通过 `msgget()`、`msgsnd()` 和 `msgrcv()` 系统调用实现。
- **底层依赖**：依赖于内核维护的消息队列数据结构。

相关代码：

```c
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

struct msg_buffer {
    long msg_type;
    char msg_text[128];
};

int main() {
    key_t key = ftok("msg_queue", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    struct msg_buffer message;
    message.msg_type = 1;
    strcpy(message.msg_text, "Hello from sender!");

    // 发送消息
    msgsnd(msgid, &message, sizeof(message), 0);

    // 接收消息
    msgrcv(msgid, &message, sizeof(message), 1, 0);
    printf("Received: %s\n", message.msg_text);

    // 删除消息队列
    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}
```

## 4. 共享内存（Shared Memory）

### 原理和设计，优化方向，应用场景

- **原理和设计**：共享内存允许多个进程共享同一块内存区域，是最高效的 IPC 方式。
- **优化方向**：可以通过调整共享内存大小和使用同步机制（如信号量）来优化。
- **应用场景**：适用于需要高效传输大量数据的场景，如数据库系统。

### 底层实现和底层依赖

- **底层实现**：通过 `shmget()`、`shmat()` 和 `shmdt()` 系统调用实现。
- **底层依赖**：依赖于内核维护的共享内存段。

相关代码：

```c
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

int main() {
    key_t key = ftok("shm_file", 65);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    // 附加共享内存
    char *str = (char *)shmat(shmid, (void *)0, 0);

    // 写入数据
    strcpy(str, "Hello from writer!");

    // 读取数据
    printf("Data read from memory: %s\n", str);

    // 分离共享内存
    shmdt(str);

    // 删除共享内存
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
```

## 5. 信号量（Semaphore）

### 原理和设计，优化方向，应用场景

- **原理和设计**：信号量用于进程间的同步，通常用于控制对共享资源的访问。
- **优化方向**：可以通过调整信号量的初始值和操作方式（如非阻塞）来优化。
- **应用场景**：适用于需要同步的场景，如生产者-消费者问题。

### 底层实现和底层依赖

- **底层实现**：通过 `semget()`、`semop()` 和 `semctl()` 系统调用实现。
- **底层依赖**：依赖于内核维护的信号量集合。

相关代码：

```c
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int main() {
    key_t key = ftok("sem_file", 65);
    int semid = semget(key, 1, 0666 | IPC_CREAT);

    union semun arg;
    arg.val = 1;  // 初始化信号量值为 1
    semctl(semid, 0, SETVAL, arg);

    struct sembuf sb = {0, -1, 0};  // P 操作
    semop(semid, &sb, 1);

    printf("Critical section\n");

    sb.sem_op = 1;  // V 操作
    semop(semid, &sb, 1);

    // 删除信号量
    semctl(semid, 0, IPC_RMID, arg);

    return 0;
}
```

## 6. 套接字（Socket）

### 原理和设计，优化方向，应用场景

- **原理和设计**：套接字是一种网络通信机制，支持不同主机间的进程通信。
- **优化方向**：可以通过调整缓冲区大小和使用非阻塞模式来优化。
- **应用场景**：适用于网络通信，如 Web 服务器和客户端。

### 底层实现和底层依赖

- **底层实现**：通过 `socket()`、`bind()`、`listen()` 和 `accept()` 等系统调用实现。
- **底层依赖**：依赖于网络协议栈和内核缓冲区。

相关代码：

```c
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    const char *hello = "Hello from server";

    // 创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // 绑定套接字
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // 监听
    listen(server_fd, 3);

    // 接受连接
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

    // 发送数据
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    // 关闭套接字
    close(new_socket);
    close(server_fd);

    return 0;
}
```

## 7. 信号（Signal）

### 原理和设计，优化方向，应用场景

- **原理和设计**：信号是一种异步通信机制，用于通知进程发生了某个事件。
- **优化方向**：可以通过合理设计信号处理函数来避免竞态条件。
- **应用场景**：适用于进程控制，如终止进程或处理异常。

### 底层实现和底层依赖

- **底层实现**：通过 `signal()` 或 `sigaction()` 系统调用实现。
- **底层依赖**：依赖于内核的信号处理机制。

相关代码：

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int signum) {
    printf("Received signal %d\n", signum);
}

int main() {
    // 注册信号处理函数
    signal(SIGINT, signal_handler);

    while (1) {
        printf("Running...\n");
        sleep(1);
    }

    return 0;
}
```

# 相关阅读

## Linux的调度

### 论文

"**The Completely Fair Scheduler" - Ingo Molnar**

https://dl.acm.org/doi/fullHtml/10.5555/1594371.1594375

### 博客

https://www.geeksforgeeks.org/cpu-scheduling-in-operating-systems/

https://tldp.org/LDP/tlk/kernel/processes.html

## Linux的IPC 机制

### 论文

**"Communications in Operating Systems" - William Stallings**

[https://dl.acm.org/doi/pdf/10.1145/359340.359342](https://ris.utwente.nl/ws/portalfiles/portal/5605915/mullender92interprocess.pdf)

### 博客

https://www.geeksforgeeks.org/inter-process-communication-ipc/

https://tldp.org/LDP/lpg/node7.html