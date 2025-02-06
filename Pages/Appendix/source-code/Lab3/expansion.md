# 说明

本文仍然基于lab内容进行拓展，包括权限和资源管理 ， 进程调度 ， 异常处理 ， 系统调用。 

# Linux 中的权限和资源管理

在 ChCore 中，每个进程的资源和权限管理围绕 cap_group 进行 ，具体表现在：

1. cap_group 记录该进程拥有的 capability（能力），决定进程可以访问哪些资源（如内存、设备、IPC）。
2. 每个 cap_group 只能访问自己拥有的 capability，这使得 ChCore 具备强隔离性。

接下来我们将讨论Linux中的资源和权限管理：

## 进程的权限和资源管理

在 Linux 内核中，每个进程都由 task_struct 结构体表示，它存储了进程的 **身份信息、权限信息、命名空间信息** 等。

### 代码

**相关代码：task_struct（位于 include/linux/sched.h）**

```c
struct task_struct {
    pid_t pid;                  // 进程 ID
    pid_t tgid;                 // 线程组 ID（主线程 ID）

    struct mm_struct *mm;       // 进程的内存管理信息
    struct files_struct *files; // 进程打开的文件表
    struct cred *cred;          // 进程的权限信息（UID、GID、capabilities）
    struct nsproxy *nsproxy;    // 命名空间信息
    struct signal_struct *signal; // 进程的信号处理信息
    struct cgroup *cgroups;     // 进程所属的 cgroup 组

    struct list_head tasks;     // 进程链表，用于进程调度
    struct sched_entity se;     // 调度实体（CFS 调度使用）
    struct thread_struct thread; // 线程相关信息（寄存器、堆栈等）
    struct task_struct *parent; // 父进程指针
    struct list_head children;  // 子进程链表

    struct mutex alloc_lock;    // 进程资源分配锁
};

```

- **cred**：指向 struct cred，存储进程的 **用户 ID（UID）、组 ID（GID）、capability 权限** 等。
- **nsproxy**：指向 struct nsproxy，存储 **进程所属的命名空间**（pid namespace、mnt namespace 等）。
- **mm**：存储进程的 **虚拟内存信息**，类似 ChCore 中的 cap_group->vmspace。
- **files**：存储进程的 **文件描述符表**。
- **cgroups**（控制组）是 Linux 内核提供的一种机制，用于**限制、隔离和管理**进程对系统资源（CPU、内存、IO、网络等）的使用。它常用于**容器、虚拟化、资源控制**等场景。

### cred 机制

在 Linux 中，cred 结构体管理了 **进程的用户权限和 capabilities**，类似于 ChCore 的 cap_group 机制。

**相关代码：cred（位于 include/linux/cred.h）**

```c
struct cred {
    kuid_t uid;    // 用户 ID
    kgid_t gid;    // 组 ID
    kuid_t euid;   // 有效用户 ID（effective UID）
    kgid_t egid;   // 有效组 ID（effective GID）
    kuid_t suid;   // 保存的用户 ID（saved UID）
    kgid_t sgid;   // 保存的组 ID（saved GID）

    struct group_info *group_info; // 进程所属的组信息

    struct user_struct *user; // 用户的资源信息
    struct key *session_keyring; // 进程的密钥信息

    struct kernel_cap_struct cap_inheritable; // 可继承的 capability
    struct kernel_cap_struct cap_permitted;   // 允许的 capability
    struct kernel_cap_struct cap_effective;   // 当前生效的 capability
};

```

### **namespace（进程的资源隔离）**

Linux 采用 namespace 机制来实现 **进程间的资源隔离**，类似 ChCore 的 cap_group 机制，但粒度更细。

**相关代码：nsproxy（位于 include/linux/nsproxy.h）**

```c
struct nsproxy {
    struct uts_namespace *uts_ns;   // 主机名/域名命名空间
    struct ipc_namespace *ipc_ns;   // 进程间通信（IPC）命名空间
    struct mnt_namespace *mnt_ns;   // 挂载点（文件系统）命名空间
    struct pid_namespace *pid_ns;   // 进程 ID 命名空间
    struct net_namespace *net_ns;   // 网络命名空间
};

```

### **capability（进程的特权管理）**

Linux 使用 capability 机制来控制进程的权限，类似 ChCore 的 capability 机制，但 **Linux 允许部分权限继承**。

**相关代码：capability.h（位于 include/uapi/linux/capability.h）**

```c
#define CAP_CHOWN           0  // 修改文件所有权
#define CAP_DAC_OVERRIDE    1  // 绕过文件权限检查
#define CAP_NET_ADMIN      12  // 管理网络设备
#define CAP_SYS_ADMIN      21  // 执行系统管理操作

```

进程的 cred 结构体包含：

- **cap_permitted** → 进程 **可以** 使用的 capabilities
- **cap_effective** → 进程 **正在** 使用的 capabilities

## 线程的权限和资源管理

在 Linux 中，线程（Thread）实际上就是一个特殊的进程（Process）。Linux **并没有单独的线程结构**，而是通过 task_struct 来管理所有任务（进程和线程）。进程和线程的主要区别在于它们 **是否共享资源**。

Linux 通过 clone() 来创建线程或进程，参数决定它是**新进程**还是**新线程**。

**相关代码：kernel/fork.c**

```c
long do_fork(unsigned long clone_flags, unsigned long stack_start,
             unsigned long stack_size, int __user *parent_tidptr,
             int __user *child_tidptr)
{
    struct task_struct *p;
    p = copy_process(clone_flags, stack_start, stack_size, parent_tidptr, child_tidptr);
    return p ? task_pid_vnr(p) : -ENOMEM;
}

```

clone_flags → 决定是否共享资源：

- **CLONE_VM** → 共享地址空间（即线程）
- **CLONE_FILES** → 共享文件描述符表
- **CLONE_SIGHAND** → 共享信号处理

当 CLONE_VM 被设置时，线程 **不会** 拷贝进程的地址空间，而是与原进程共享，这样它们就能访问相同的数据，这与 ChCore 中的 cap_group 共享资源类似。

# Linux 的进程调度

在 **Linux** 内核中，与 **ChCore** 线程上下文初始化及内核态切换到用户态的过程相对应的部分，主要涉及 **`fork`、`execve`、`schedule`、`switch_to`、`ret_to_user`** 等函数。

---

## **1.线程上下文初始化 (`init_thread_ctx` 对应)**

在 Linux 中，新进程/线程的初始化类似于 ChCore 的 `init_thread_ctx()`，主要由 `copy_process()` 负责。

**代码位置：**

- `kernel/fork.c` -> `copy_process()`

**核心步骤：**

1. **分配 `task_struct`**
2. **复制父进程的 `mm_struct`（用户地址空间）**
3. **初始化寄存器状态**
4. **加入调度队列**

**对应代码：**

```c
static struct task_struct *copy_process(struct clone_args *args) {
    struct task_struct *p;

    p = dup_task_struct(current); // 复制当前进程的 task_struct
    p->mm = copy_mm(args, current); // 复制内存空间
    p->files = copy_files(args, current); // 复制文件描述符

    thread_copy(args, p); // 初始化线程上下文
    return p;
}

```

这个 `thread_copy()` 就相当于 `init_thread_ctx()`，它会初始化寄存器状态、设置栈指针等。

---

## **2. 调度 (`sched()` 对应)**

ChCore 的 `sched()` 负责选择下一个要运行的线程，在 Linux 中，`schedule()` 负责这个工作。

**代码位置：**

- `kernel/sched/core.c` -> `schedule()`

```c
void __sched schedule(void) {
    struct task_struct *next, *prev;

    prev = current;
    next = pick_next_task(prev); // 选择下一个要执行的进程

    context_switch(prev, next); // 切换到新的进程
}

```

---

## **3. 上下文切换 (`switch_context()` 对应)**

ChCore 用 `switch_context()` 进行线程切换，Linux 采用 `switch_to()`。

**代码位置：**

- `arch/arm64/kernel/process.c` -> `switch_to()`

```c
#define switch_to(prev, next, last) \\
    do { \\
        cpu_switch_to(prev, next); \\
    } while (0)

```

其中 `cpu_switch_to()` 负责：

- **切换寄存器状态**
- **切换栈指针**
- **更新 `task_struct`**

---

## **4. 内核态 → 用户态 (`eret_to_thread()` 对应)**

在 ChCore，`eret_to_thread()` 负责从内核态切换到用户态，Linux 中类似的是 `ret_to_user`。

**代码位置：**

- `arch/arm64/kernel/entry.S` -> `ret_to_user`

```c
ret_to_user:
    mov x0, sp         // 恢复用户态栈指针
    eret              // 从 EL1 返回到 EL0

```

这里 `eret` 指令的作用跟 ChCore 一样，是 ARM64 的指令，负责从 **EL1（内核态）切换到 EL0（用户态）**。

---

## **总结：ChCore vs. Linux 对应关系**

| **ChCore** | **Linux** | **作用** |
| --- | --- | --- |
| `init_thread_ctx()` | `thread_copy()` | 线程上下文初始化 |
| `sched()` | `schedule()` | 选择下一个执行的线程 |
| `switch_context()` | `switch_to()` | 线程/进程上下文切换 |
| `eret_to_thread()` | `ret_to_user` | 从内核态切换到用户态 |

Linux 采用**宏内核架构**，所有的调度、资源管理都在 `task_struct` 中，而 ChCore 采用**微内核架构**，资源管理 (`cap_group`) 和调度 (`thread_t`) 分离。

# **Linux 的异常处理机制及其与 ChCore 的对比**

在 AArch64 架构中，异常（Exception）是 CPU 在运行过程中，由于软件或硬件事件触发的特殊情况，可能需要操作系统内核介入处理。例如：

- **同步异常（Synchronous Exception）**：由于指令执行引起，如非法指令、访问非法地址、系统调用（`svc` 指令）。
- **异步异常（Asynchronous Exception）**：与当前执行指令无关，如 **IRQ（普通中断）、FIQ（快速中断）、SError（系统错误）**。

Linux 内核必须实现完整的异常处理机制，而 ChCore 作为微内核，采用较精简的异常处理方式。下文对比两者在 **异常向量表、异常处理流程、系统调用、上下文切换** 等方面的不同。

---

## **1. 异常向量表（Exception Vector Table）**

AArch64 处理器在异常发生时，会跳转到 **异常向量表** 执行异常处理。Linux 和 ChCore 均需要初始化异常向量表。

- **Linux 的异常向量表** 位于 `arch/arm64/kernel/entry.S`，并由 `__exception_vectors` 统一管理不同异常。
- **ChCore 的异常向量表** 位于 `kernel/arch/aarch64/irq/irq_entry.S`，仅针对 EL1 进行初始化。

Linux 支持完整的 EL0-EL3 异常处理，而 ChCore 仅涉及 EL0 和 EL1。

---

## **2. 异常处理流程（Exception Handling Flow）**

### **Linux 异常处理流程**

1. **CPU 触发异常**（同步异常或中断）。
2. **跳转到异常向量表**，进入 `__exception_vectors` 处理。
3. **调用异常处理函数**：
    - `el1_sync` 处理 EL1 级别同步异常。
    - `el1_irq` 处理 EL1 级别中断。
    - `el0_sync` 处理用户态（EL0）的异常，如系统调用。
4. **解析异常原因**（`do_sync` 解析同步异常，`do_irq` 处理中断）。
5. **调用具体处理逻辑**，如 `handle_syscall` 处理系统调用。
6. **恢复 CPU 状态**，执行 `eret` 返回。

### **ChCore 异常处理流程**

1. **进入异常向量表**（如 `vector_sync_el1h`）。
2. **调用 `handle_entry_c`** 解析异常类型。
3. **针对 `svc` 指令调用 `handle_syscall` 处理系统调用**。
4. **异常处理完毕后返回用户态**。

ChCore 的异常处理逻辑比 Linux 更加简洁，主要是因为其微内核设计减少了内核的职责。

---

## **3. 系统调用（Syscall）**

### **Linux 系统调用流程**

1. 用户态程序执行 `svc #0`。
2. 触发 **同步异常**，进入 `el0_sync` 处理。
3. 调用 `do_el0_svc` 解析 **系统调用号**（存储在 `x8` 寄存器）。
4. 调用相应的系统调用处理函数（如 `sys_write`）。
5. 处理完成后返回用户态。

```c
el0_sync:
    save_context
    bl do_el0_svc
    restore_context
    eret

```

### **ChCore 系统调用流程**

1. 用户态程序执行 `svc` 指令。
2. 进入 `vector_sync_el1h` 处理异常。
3. 调用 `handle_syscall` 解析系统调用号。
4. 调用相应的系统调用函数（如 `sys_write`）。
5. 处理完成后返回用户态。

```c
vector_sync_el1h:
    save_context
    bl handle_syscall
    restore_context
    eret

```

| **对比项** | **Linux** | **ChCore** |
| --- | --- | --- |
| **系统调用入口** | `el0_sync` | `vector_sync_el1h` |
| **处理函数** | `do_el0_svc` | `handle_syscall` |
| **系统调用解析** | `sys_call_table` | `syscall_table` |
| **异常返回** | `ret_from_exception` | `eret_to_thread` |

Linux 和 ChCore 的系统调用机制类似，但 Linux 由于支持更多特性（如 seccomp、安全沙盒），其实现更加复杂。

---

## **4. 上下文切换（Context Switch）**

上下文切换涉及进程/线程调度，需要保存和恢复 CPU 状态。

### **Linux 上下文切换**

1. `context_switch` 负责保存当前进程的寄存器状态。
2. `switch_mm` 切换进程的地址空间（MMU 切换）。
3. `restore_context` 恢复新进程的 CPU 状态。
4. 继续执行新进程。

```c
switch_to:
    save_context
    load_new_task
    restore_context

```

### **ChCore 上下文切换**

1. `switch_context` 负责保存当前线程状态。
2. 切换到新的线程（`thread_ctx`）。
3. 恢复新线程状态，执行 `eret` 返回。

```c
switch_context:
    save_thread_context
    load_new_thread
    eret

```

| **对比项** | **Linux** | **ChCore** |
| --- | --- | --- |
| **上下文切换函数** | `switch_to` | `switch_context` |
| **进程调度** | 复杂（涉及 CFS、优先级等） | 简单（仅切换线程） |
| **地址空间切换** | `switch_mm` | 轻量化切换 |
| **恢复方式** | `restore_context` | `eret` 返回 |

ChCore 的上下文切换较 Linux 更加轻量，因为它是微内核，仅切换 **线程上下文**，而 Linux 需要切换整个 **进程上下文**（包括 MMU、地址空间等）。

---

## **总结**

| **特性** | **Linux** | **ChCore** |
| --- | --- | --- |
| **异常向量表** | `__exception_vectors` | `irq_entry.S` |
| **异常处理流程** | 复杂，支持 EL0-EL3 | 仅处理 EL0、EL1 |
| **系统调用** | `do_el0_svc` | `handle_syscall` |
| **上下文切换** | `switch_to` | `switch_context` |
| **进程调度** | 完整进程管理 | 仅线程切换 |
| **异常返回** | `ret_from_exception` | `eret_to_thread` |

**Linux 采用完整的异常处理机制，适用于通用 OS，而 ChCore 采用轻量化设计，更适合微内核架构。**

# Linux 与 ChCore 的系统调用机制对比

## 1. 系统调用概述

系统调用是操作系统提供给用户程序访问内核功能的接口，通常用于执行特权操作，如文件管理、进程控制和内存操作。

- **ChCore**：用户程序通过 `svc` 指令进入内核模式。
- **Linux**：用户程序通常使用 `syscall` 指令（x86）或 `svc` 指令（AArch64）进入内核。

两者都需要保存当前执行状态，以便系统调用执行完毕后能正确返回用户态。

## 2. 进入内核模式的流程

### **ChCore**

1. 用户态程序执行 `svc` 指令触发同步异常。
2. 进入异常向量表的 `sync_el1h` 处理代码。
3. `exception_enter` 负责保存当前线程的寄存器状态。
4. 切换到内核栈 `switch_to_cpu_stack`。
5. 根据系统调用编号调用相应的系统调用处理函数。
6. 执行完毕后，通过 `exception_exit` 恢复寄存器状态并返回用户态。

### **Linux**

1. 用户态程序执行 `syscall` 指令（x86）或 `svc` 指令（AArch64）。
2. 进入异常向量表 `entry.S`，调用 `do_syscall_64`（x86）或 `el0_svc`（AArch64）。
3. `syscall_enter_from_user_mode` 负责保存用户态寄存器。
4. 通过 `sys_call_table` 查找并调用具体的系统调用函数。
5. `syscall_exit_to_user_mode` 负责恢复用户态寄存器并返回。

## 3. 上下文切换与寄存器管理

### **ChCore**

- `exception_enter` 保存 **通用寄存器** 和 **程序状态**。
- `exception_exit` 恢复寄存器，并执行 `eret` 返回用户态。
- 需要手动管理栈切换，读取 `TPIDR_EL1` 获取 `cpu_stack`。

### **Linux**

- `syscall_enter_from_user_mode` 负责 **用户态寄存器** 备份。
- `syscall_exit_to_user_mode` 负责 **恢复用户态寄存器**。
- 使用 `task_struct` 维护进程状态，避免手动栈切换。

## 4. 系统调用表与调用机制

### **ChCore**

- `syscall_dispatcher` 解析系统调用号。
- 使用 `chcore_syscallx` 进行系统调用。
- 例如，`sys_putstr` 用于 `printf` 实现字符输出。

### **Linux**

- `sys_call_table` 存储系统调用函数指针。
- `sys_write` 用于 `printf` 输出，调用 `do_write` 处理文件描述符。
- 通过 `glibc` 提供 `write()`，封装 `syscall(SYS_write, ...)`。

## 5. 主要区别

| 机制 | ChCore | Linux |
| --- | --- | --- |
| 进入方式 | `svc` | `syscall` / `svc` |
| 系统调用表 | `syscall_dispatcher` | `sys_call_table` |
| 进程管理 | `cap_group` 机制 | `task_struct` 结构体 |
| 寄存器保存 | `exception_enter` | `syscall_enter_from_user_mode` |
| 栈管理 | 需要手动切换 | 自动管理 |

## 7. 结论

- **ChCore** 由于是微内核架构，系统调用管理较为精简，需手动处理内核栈和寄存器切换。
- **Linux** 由于是宏内核，提供完整的 `sys_call_table`，更强大的进程管理，用户态库（如 `glibc`）隐藏了底层细节。

以上即为 Linux 与 ChCore 在系统调用机制上的核心对比。