# 实验 4：多核调度与IPC
在本实验中，ChCore将支持在多核处理器上启动（第一部分）；实现多核调度器以调度执行多个线程（第二部分）；最后实现进程间通信IPC（第三部分）。注释`/* LAB 4 TODO BEGIN (exercise #) */`和`/* LAB 4 TODO END (exercise #) */`之间代表需要填空的代码部分。

## 第一部分：多核支持
本部分实验没有代码题，仅有思考题。为了让ChCore支持多核，我们需要考虑如下问题：

如何启动多核，让每个核心执行初始化代码并开始执行用户代码？
如何区分不同核心在内核中保存的数据结构（比如状态，配置，内核对象等）？
如何保证内核中对象并发正确性，确保不会由于多个核心同时访问内核对象导致竞争条件？
在启动多核之前，我们先介绍ChCore如何解决第二个问题。ChCore对于内核中需要每个CPU核心单独存一份的内核对象，都根据核心数量创建了多份（即利用一个数组来保存）。ChCore支持的核心数量为PLAT_CPU_NUM（该宏定义在 kernel/common/machine.h 中，其代表可用CPU核心的数量，根据具体平台而异）。 比如，实验使用的树莓派3平台拥有4个核心，因此该宏定义的值为4。ChCore会CPU核心的核心ID作为数组的索引，在数组中取出对应的CPU核心本地的数据。为了方便确定当前执行该代码的CPU核心ID，我们在 kernel/arch/aarch64/machine/smp.c中提供了smp_get_cpu_id函数。该函数通过访问系统寄存器tpidr_el1来获取调用它的CPU核心的ID，该ID可用作访问上述数组的索引。

### 启动多核
在实验1中我们已经介绍，在QEMU模拟的树莓派中，所有CPU核心在开机时会被同时启动。在引导时这些核心会被分为两种类型。一个指定的CPU核心会引导整个操作系统和初始化自身，被称为主CPU（primary CPU）。其他的CPU核心只初始化自身即可，被称为其他CPU（backup CPU）。CPU核心仅在系统引导时有所区分，在其他阶段，每个CPU核心都是被相同对待的。

> 思考题 1:阅读汇编代码kernel/arch/aarch64/boot/raspi3/init/start.S。说明ChCore是如何选定主CPU，并阻塞其他其他CPU的执行的。

在树莓派真机中，还需要主CPU手动指定每一个CPU核心的的启动地址。这些CPU核心会读取固定地址的上填写的启动地址，并跳转到该地址启动。在kernel/arch/aarch64/boot/raspi3/init/init_c.c中，我们提供了wakeup_other_cores函数用于实现该功能，并让所有的CPU核心同在QEMU一样开始执行_start函数。

与之前的实验一样，主CPU在第一次返回用户态之前会在kernel/arch/aarch64/main.c中执行main函数，进行操作系统的初始化任务。在本小节中，ChCore将执行enable_smp_cores函数激活各个其他CPU。

> 思考题2：阅读汇编代码kernel/arch/aarch64/boot/raspi3/init/start.S, init_c.c以及kernel/arch/aarch64/main.c，解释用于阻塞其他CPU核心的secondary_boot_flag是物理地址还是虚拟地址？是如何传入函数enable_smp_cores中，又是如何赋值的（考虑虚拟地址/物理地址）？

## 第二部分：多核调度
ChCore已经可以启动多核，但仍然无法对多个线程进行调度。本部分将首先实现协作式调度，从而允许当前在CPU核心上运行的线程主动退出或主动放弃CPU时，CPU核心能够切换到另一个线程继续执行。其后，我们将驱动树莓派上的物理定时器，使其以一定的频率发起中断，使得内核可以在一定时间片后重新获得对CPU核心的控制，并基于此进一步实现抢占式调度。

ChCore中与调度相关的函数与数据结构定义在kernel/include/sched/sched.h中。
sched_ops是用于抽象ChCore中调度器的一系列操作。它存储指向不同调度操作的函数指针，以支持不同的调度策略。
cur_sched_ops则是一个sched_ops的实例，其在内核初始化过程中（main函数）调用sched_init进行初始化。
ChCore用在 kernel/include/sched/sched.h 中定义的静态函数封装对cur_sched_ops的调用。sched_ops中定义的调度器操作如下所示：

* sche_init：初始化调度器。
* sched：进行一次调度。即将正在运行的线程放回就绪队列，然后在就绪队列中选择下一个需要执行的线程返回。
* sched_enqueue：将新线程添加到调度器的就绪队列中。
* sched_dequeue：从调度器的就绪队列中取出一个线程。
* sched_top：用于debug,打印当前所有核心上的运行线程以及等待线程的函数。

在本部分将实现一个基本的Round Robin（时间片轮转）调度器，该程序调度在同一CPU核心上运行的线程，因此内核初始化过程调用sched_init时传入了&rr作为参数。该调度器的调度操作（即对于sched_ops定义的各个函数接口的实现）实现在kernel/sched/policy_rr.c中，这里煎药介绍其涉及的数据结构：

`current_threads`是一个数组，分别指向每个CPU核心上运行的线程。而`current_thread`则利用`smp_get_cpu_id`获取当前运行核心的id，从而找到当前核心上运行的线程。

`struct queue_meta`定义了round robin调度器使用的就绪队列，其中`queue_head`字段是连接该就绪队列上所有等待线程的队列，`queue_len`字段是目前该就绪队列的长度，`queue_lock`字段是用于保证该队列并发安全的锁。 kernel/sched/policy_rr.c定义了一个全局变量`rr_ready_queue_meta`,该变量是一个`struct queue_meta`类型的数组，数组大小由`PLAT_CPU_NUM`定义，即代表每个CPU核心都具有一个就绪队列。运行的CPU核心可以通过`smp_get_cpu_id`获取当前运行核心的id，从而在该数组中找到当前核心对应的就绪队列。

### 调度队列初始化
内核初始化过程中会调用`sched_init`初始化调度相关的元数据，`sched_init`定义在kernel/sched/sched.c中，该函数首先初始化idle_thread(每个CPU核心拥有一个idle_thread，当调度器的就绪队列中没有等待线程时会切换到idle_thread运行)，然后会初始化`current_threads`数组，最后调用`struct sched_ops rr`中定义的sched_init函数，即`rr_sched_init`。
> 练习 1：在 kernel/sched/policy_rr.c 中完善 `rr_sched_init` 函数，对 `rr_ready_queue_meta` 进行初始化。在完成填写之后，你可以看到输出“Scheduler metadata is successfully initialized!”并通过 Scheduler metadata initialization 测试点。
> 
> 提示：sched_init 只会在主 CPU 初始化时调用，因此 rr_sched_init 需要对每个 CPU 核心的就绪队列都进行初始化。

### 调度队列入队
内核初始化过程结束之后会调用`create_root_thread`来创建第一个用户态进程及线程，在`create_root_thread`最后会调用`sched_enqueue`函数将创建的线程加入调度队列之中。`sched_enqueue`
最终会调用kernel/sched/policy_rr.c中定义的`rr_sched_enqueue`函数。该函数首先挑选合适的CPU核心的就绪队列（考虑线程是否绑核以及各个CPU核心之间的负载均衡），然后调用`__rr_sched_enqueue`将线程插入到选中的就绪队列中。
> 练习 2：在 kernel/sched/policy_rr.c 中完善 `__rr_sched_enqueue` 函数，将`thread`插入到`cpuid`对应的就绪队列中。在完成填写之后，你可以看到输出“Successfully enqueue root thread”并通过 Schedule Enqueue 测试点。

### 调度队列出队
内核初始化过程结束并调用`create_root_thread`创建好第一个用户态进程及线程之后，在第一次进入用户态之前，会调用`sched`函数来挑选要返回到用户态运行的线程（虽然此时就绪队列中只有root thread一个线程）。`sched`最终会调用kernel/sched/policy_rr.c中定义的`rr_sched`函数。
该调度函数的操作非常直观，就是将现在正在运行的线程重新加入调度器的就绪队列当中，并从就绪队列中挑选出一个新的线程运行。
由于内核刚刚完成初始化，我们还没有设置过`current_thread`，所以`rr_sched`函数中`old`为`NULL`，后面的练习中我们会考虑`old`不为`NULL`的情况。紧接着`rr_sched`会调用`rr_sched_choose_thread`函数挑选出下一个运行的线程，并切换到该线程。

`rr_sched_choose_thread`内部会调用`find_runnable_thread`从当前CPU核心的就绪队列中选取一个可以运行的线程并调用`__rr_sched_dequeue`将其从就绪队列中移除。
> 练习 3：在 kernel/sched/policy_rr.c 中完善 `find_runnable_thread` 函数，在就绪队列中找到第一个满足运行条件的线程并返回。 在 kernel/sched/policy_rr.c 中完善 `__rr_sched_dequeue` 函数，将被选中的线程从就绪队列中移除。在完成填写之后，运行 ChCore 将可以成功进入用户态，你可以看到输出“Enter Procmgr Root thread (userspace)”并通过 Schedule Enqueue 测试点。

### 协作式调度
顾名思义，协作式调度需要线程主动放弃CPU。为了实现该功能，我们提供了`sys_yield`这一个系统调用(syscall)。该syscall可以主动放弃当前CPU核心，并调用上述的`sched`接口完成调度器的调度工作。kernel/sched/policy_rr.c中定义的`rr_sched`函数中，如果当前运行线程的状态为`TS_RUNNING`，即还处于可以运行的状态，我们应该将其重新加入到就绪队列当中，这样该线程在之后才可以被再次调度执行。

> 练习 4：在kernel/sched/sched.c中完善系统调用`sys_yield`，使用户态程序可以主动让出CPU核心触发线程调度。
> 此外，请在kernel/sched/policy_rr.c 中完善`rr_sched`函数，将当前运行的线程重新加入调度队列中。在完成填写之后，运行 ChCore 将可以成功进入用户态并创建两个线程交替执行，你可以看到输出“Cooperative Schedluing Test Done!”并通过 Cooperative Schedluing 测试点。

### 抢占式调度

使用刚刚实现的协作式调度器，ChCore能够在线程主动调用`sys_yield`系统调用让出CPU核心的情况下调度线程。然而，若用户线程不想放弃对CPU核心的占据，内核便只能让用户线程继续执行，而无法强制用户线程中止。 因此，在这一部分中，本实验将实现抢占式调度，以帮助内核定期重新获得对CPU核心的控制权。

ChCore启动的第一个用户态线程（执行user/system-services/system-servers/procmgr/procmgr.c的`main`函数）将创建一个“自旋线程”，该线程在获得CPU核心的控制权后便会执行无限循环，进而导致无论是该程序的主线程还是ChCore内核都无法重新获得CPU核心的控制权。就保护系统免受用户程序中的错误或恶意代码影响而言，这一情况显然并不理想，任何用户应用线程均可以如该“自旋线程”一样，通过进入无限循环来永久“霸占”整个CPU核心。

为了处理“自旋线程”的问题，ChCore内核必须具有强行中断一个正在运行的线程并夺回对CPU核心的控制权的能力，为此我们必须扩展ChCore以支持处理来自物理时钟的外部硬件中断。

**物理时钟初始化**

本部分我们将通过配置ARM提供的Generic Timer来使能物理时钟并使其以固定的频率发起中断。
我们需要处理的系统寄存器如下(Refer：https://developer.arm.com/documentation/102379/0101/The-processor-timers):
* CNTPCT_EL0: 它的值代表了当前的 system count。
* CNTFRQ_EL0: 它的值代表了物理时钟运行的频率，即每秒钟 system count 会增加多少。
* CNTP_CVAL_EL0: 是一个64位寄存器，操作系统可以向该寄存器写入一个值，当 system count 达到或超过该值时，物理时钟会触发中断。
* CNTP_TVAL_EL0: 是一个32位寄存器，操作系统可以写入 TVAL，处理器会在内部读取当前的系统计数，加上写入的值，然后填充 CVAL。
* CNTP_CTL_EL0: 物理时钟的控制寄存器，第0位ENABLE控制时钟是否开启，1代表enble，0代表disable；第1位IMASK代表是否屏蔽时钟中断，0代表不屏蔽，1代表屏蔽。

对物理时钟进行初始化的代码位于kernel/arch/aarch64/plat/raspi3/irq/timer.c的`plat_timer_init`函数。
> 练习 5：请根据代码中的注释在kernel/arch/aarch64/plat/raspi3/irq/timer.c中完善`plat_timer_init`函数，初始化物理时钟。需要完成的步骤有：
> * 读取 CNTFRQ_EL0 寄存器，为全局变量 cntp_freq 赋值。
> * 根据 TICK_MS（由ChCore决定的时钟中断的时间间隔，以ms为单位，ChCore默认每10ms触发一次时钟中断）和cntfrq_el0 （即物理时钟的频率）计算每两次时钟中断之间 system count 的增长量，将其赋值给 cntp_tval 全局变量，并将 cntp_tval 写入 CNTP_TVAL_EL0 寄存器！
> * 根据上述说明配置控制寄存器CNTP_CTL_EL0。
> 
> 由于启用了时钟中断，但目前还没有对中断进行处理，所以会影响评分脚本的评分，你可以通过运行ChCore观察是否有"[TEST] Physical Timer was successfully initialized!: OK"输出来判断是否正确对物理时钟进行初始化。

**物理时钟中断与抢占**

我们在lab3中已经为ChCore配置过异常向量表（kernel/arch/aarch64/irq/irq_entry.S），当收到来自物理时钟的外部中断时，内核会进入`handle_irq`中断处理函数，该函数会调用平台相关的`plat_handle_irq`来进行中断处理。`plat_handle_irq`内部如果判断中断源为物理时钟，则调用`handle_timer_irq`。

ChCore记录每个线程所拥有的时间片（`thread->thread_ctx->sc->budget`），为了能够让线程之间轮转运行，我们应当在处理时钟中断时递减当前运行线程的时间片，并在当前运行线程的时间片耗尽时进行调度，选取新的线程运行。

> 练习 6：请在kernel/arch/aarch64/plat/raspi3/irq/irq.c中完善`plat_handle_irq`函数，当中断号irq为INT_SRC_TIMER1（代表中断源为物理时钟）时调用`handle_timer_irq`并返回。 请在kernel/irq/irq.c中完善`handle_timer_irq`函数，递减当前运行线程的时间片budget，并调用sched函数触发调度。 请在kernel/sched/policy_rr.c中完善`rr_sched`函数，在将当前运行线程重新加入就绪队列之前，恢复其调度时间片budget为DEFAULT_BUDGET。
> 在完成填写之后，运行 ChCore 将可以成功进入用户态并打断创建的“自旋线程”让内核和主线程可以拿回CPU核心的控制权，你可以看到输出"Hello, I am thread 3. I'm spinning."和“Thread 1 successfully regains the control!”并通过 Preemptive Scheduling 测试点。

## 第三部分：进程间通信（IPC）

在本部分，我们将实现ChCore的进程间通信，从而允许跨地址空间的两个进程可以使用IPC进行信息交换。

### ChCore进程间通讯概览
![](./docs/assets/IPC-overview.png)

ChCore的IPC接口不是传统的send/recv接口。其更像客户端/服务器模型，其中IPC请求接收者是服务器，而IPC请求发送者是客户端。 服务器进程中包含三类线程:
* 主线程：该线程与普通的线程一样，类型为`TYPE_USER`。该线程会调用`ipc_register_server`将自己声明为一个IPC的服务器进程，调用的时候会提供两个参数:服务连接请求的函数client_register_handler和服务真正IPC请求的函数server_handler（即图中的`ipc_dispatcher`），调用该函数会创建一个注册回调线程;
* 注册回调线程：该线程的入口函数为上文提到的client_register_handler，类型为`TYPE_REGISTER`。正常情况下该线程不会被调度执行，仅当有Client发起建立IPC连接的请求时，该线程运行并执行client_register_handler，为请求建立连接的Client创建一个服务线程（即图中的IPC handler thread）并在服务器进程的虚拟地址空间中分配一个可以用来映射共享内存的虚拟地址。
* 服务线程：当Client发起建立IPC连接请求时由注册回调线程创建，入口函数为上文提到的server_handler，类型为`TYPE_SHADOW`。正常情况下该线程不会被调度执行，仅当有Client端线程使用`ipc_call`发起IPC请求时，该线程运行并执行server_handler（即图中的`ipc_dispatcher`），执行结束之后会调用`ipc_return`回到Client端发起IPC请求的线程。

Note：注册回调线程和服务线程都不再拥有调度上下文（Scheduling Context），也即不会主动被调度器调度到。其在客户端申请建立IPC连接或者发起IPC请求的时候才会被调度执行。为了实现该功能，这两种类型的线程会继承IPC客户端线程的调度上下文（即调度时间片budget），从而能被调度器正确地调度。

### ChCore IPC具体流程
为了实现ChCore IPC的功能，首先需要在Client与Server端创建起一个一对一的IPC Connection。该Connection保存了IPC Server的服务线程（即上图中IPC handler Thread）、Client与Server的共享内存（用于存放IPC通信的内容）。同一时刻，一个Connection只能有一个Client接入，并使用该Connection切换到Server的处理流程。ChCore提供了一系列机制，用于创建Connection以及创建每个Connection对应的服务线程。下面将以具体的IPC注册到调用的流程，详细介绍ChCore的IPC机制：

1. IPC服务器的主线程调用`ipc_register_server`（user/chcore-libc/musl-libc/src/chcore-port/ipc.c中）来声明自己为IPC的服务器端。
    * 参数包括server_handler和client_register_handler，其中server_handler为服务端用于提供服务的回调函数（比如上图中IPC handler Thread的入口函数`ipc_dispatcher`）；client_register_handler为服务端提供的用于注册的回调函数，该函数会创建一个注册回调线程。
    * 随后调用ChCore提供的的系统调用：`sys_register_server`。该系统调用实现在kernel/ipc/connection.c当中，该系统调用会分配并初始化一个`struct ipc_server_config`和一个`struct ipc_server_register_cb_config`。之后将调用者线程（即主线程）的general_ipc_config字段设置为创建的`struct ipc_server_config`，其中记录了注册回调线程和IPC服务线程的入口函数（即图中的`ipc_dispatcher`）。将注册回调线程的general_ipc_config字段设置为创建的`struct ipc_server_register_cb_config`，其中记录了注册回调线程的入口函数和用户态栈地址等信息。
2. IPC客户端线程调用`ipc_register_client`（定义在user/chcore-libc/musl-libc/src/chcore-port/ipc.c中）来申请建立IPC连接。
    * 该函数仅有一个参数，即IPC服务器的主线程在客户端进程cap_group中的capability。该函数会首先通过系统调用申请一块物理内存作为和服务器的共享内存（即图中的Shared Memory）。
    * 随后调用`sys_register_client`系统调用。该系统调用实现在kernel/ipc/connection.c当中，该系统调用会将刚才申请的物理内存映射到客户端的虚拟地址空间中，然后调用`create_connection`创建并初始化一个`struct ipc_connection`类型的内核对象，该内核对象中的shm字段会记录共享内存相关的信息（包括大小，分别在客户端进程和服务器进程当中的虚拟地址和capability）。
    * 之后会设置注册回调线程的栈地址、入口地址和第一个参数，并切换到注册回调线程运行。
3. 注册回调线程运行的入口函数为主线程调用`ipc_register_server`是提供的client_register_handler参数，一般会使用默认的`DEFAULT_CLIENT_REGISTER_HANDLER`宏定义的入口函数，即定义在user/chcore-libc/musl-libc/src/chcore-port/ipc.c中的`register_cb`。
    * 该函数首先分配一个用来映射共享内存的虚拟地址，随后创建一个服务线程。
    * 随后调用`sys_ipc_register_cb_return`系统调用进入内核，该系统调用将共享内存映射到刚才分配的虚拟地址上，补全`struct ipc_connection`内核对象中的一些元数据之后切换回客户端线程继续运行，客户端线程从`ipc_register_client`返回，完成IPC建立连接的过程。
4. IPC客户端线程调用`ipc_create_msg`和`ipc_set_msg_data`向IPC共享内存中填充数据，然后调用`ipc_call`（user/chcore-libc/musl-libc/src/chcore-port/ipc.c中）发起IPC请求。
    * `ipc_call`中会发起`sys_ipc_call`系统调用（定义在kernel/ipc/connection.c中），该系统调用将设置服务器端的服务线程的栈地址、入口地址、各个参数，然后迁移到该服务器端服务线程继续运行。由于当前的客户端线程需要等待服务器端的服务线程处理完毕，因此需要更新其状态为TS_WAITING，且不要加入等待队列。
5. IPC服务器端的服务线程在处理完IPC请求之后使用`ipc_return`返回。
    * `ipc_return`会发起`sys_ipc_return`系统调用，该系统调用会迁移回到IPC客户端线程继续运行，IPC客户端线程从`ipc_call`中返回。

> 练习 7：在user/chcore-libc/musl-libc/src/chcore-port/ipc.c与kernel/ipc/connection.c中实现了大多数IPC相关的代码，请根据注释补全kernel/ipc/connection.c中的代码。之后运行ChCore可以看到 “[TEST] Test IPC finished!” 输出，你可以通过 Test IPC 测试点。
