# 实验5：虚拟文件系统

虚拟文件系统（Virtual File System，VFS）提供了一个抽象层，使得不同类型的文件系统可以在应用程序层面以统一的方式进行访问。这个抽象层隐藏了不同文件系统之间的差异，使得应用程序和系统内核可以以一致的方式访问各种不同类型的文件系统，如 ext4、tmpfs、 FAT32 等。在 ChCore 中，我们通过 FSM 系统服务以及 FS_Base 文件系统 wrapper 将不同的文件系统整合起来，给运行在 ChCore 上的应用提供了统一的抽象。

> 练习1：阅读 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/file.c` 的 `chcore_openat` 函数，分析 ChCore 是如何处理 `openat` 系统调用的，关注 IPC 的调用过程以及 IPC 请求的内容。

Lab5 的所有代码都运行在用户态，不同应用间通过 IPC 进行通信，可调用的 IPC 相关函数定义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore/ipc.h`。

## 第一部分：FSM

只要实现了 FSBase 和 FSWrapper 的接口的 IPC 服务，都可以成为一个文件系统示例。FSM 负责管理文件系统，为用户态建立文件系统连接并创建 IPC 的客户端，由于文件系统与其挂载点密切相关，所以 FSM 会处理以下类型的请求：

```C
/* Client send fsm_req to FSM */
enum fsm_req_type {
        FSM_REQ_UNDEFINED = 0,

        FSM_REQ_PARSE_PATH,
        FSM_REQ_MOUNT,
        FSM_REQ_UMOUNT,

        FSM_REQ_SYNC,
};
```

当 FSM 收到 Client 的FSM_REQ_MOUNT 类型的请求时，其会执行挂载文件系统的操作，**增加挂载的文件系统数量**，创建对应的 `mount_info_node` 添加到挂载信息表中，直至最后**与文件系统建立 IPC 连接**，并将创建完的 IPC 客户端保存在挂载信息中，总而言之，FSM 仅负责挂载和文件系统同步有关的工作，剩下的其他功能由每个具体的FS服务进行处理。

```c
struct mount_point_info_node {
        cap_t fs_cap;
        char path[MAX_MOUNT_POINT_LEN + 1];
        int path_len;
        ipc_struct_t *_fs_ipc_struct; // fs_client
        int refcnt;
        struct list_head node;
};

```

> 练习2：实现 `user/system-services/system-servers/fsm/fsm.c` 的 `fsm_mount_fs` 函数。
>
> 提示：你应当回顾Lab4的代码以查看ChCore是怎么基于IPC服务的cap来创建并维护连接的。

当 FSM 收到 Client 的 FSM_REQ_PARSE_PATH 类型的请求时，其首先会尝试解析 IPC 请求中访问文件的路径，通过遍历挂载信息链表，找到对应的最匹配的文件系统以及其挂载点路径。通过匹配的文件系统，获取到该文件系统的客户端 cap。如果 Client 已经获取到了文件系统的 cap，则直接返回解析后的`挂载点路径`；否则 FSM 会把挂载路径以及其对应的文件系统的 cap 也一并返回给 Client，并记录该 Client 已获取的文件系统 cap 的信息（FSM 会记录所有已经发送给某个 Client 的文件系统的 cap，见 `user/system-services/system-servers/fsm/fsm_client_cap.h`）。

> 练习3：实现 `user/system-services/system-servers/fsm/fsm.c` 的 IPC 请求处理函数。
>
> 提示：
>   * 完成 `user/system-services/system-servers/fsm/fsm_client_cap.c` 中的相关函数。
>   * 所有关于挂载点有关的helper函数都在 `user/system-services/system-servers/fsm/mount_info.c`
>   * IPC handler 返回的 IPC msg 的数据类型为 `struct fsm_request`，其有关的含义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore-internal/fs_defs.h` 有详细的解释。
>   * 使用 `user/system-services/system-servers/fsm/mount_info.h` 定义的函数来帮助你实现 IPC handler。
>   * 你应当回顾 Lab4 代码以查看 ChCore 是怎么将 cap 对象在进程间收发的，以及 ChCore 中是怎么使用共享内存完成 IPC 调用的。
>   * 由于 printf 并不经过FS所以你可以放心使用。

我们提供了所有需要实现的文件的 Obj 版本，你可以修改 CMakeLists.txt，将编译所需的源文件从未实现的 C 文件替换为包含了正确实现的 Obj 文件，以此验证某一部分练习的正确性。如果你需要调试某一个部分，你可以将 `scripts/grade/cmakelists` 下的CMakeLists对应复制到 `FSM` 以及 `FS_Base` 的目录下覆盖并重新编译，运行 `make qemu` 后你就可以查看到 printf 的调试信息。

完成第一部分后，执行 `make grade`，可以得到 `Scores: 20/100`。

## 第二部分：FS_Base

在 ChCore 中，FS_Base 是文件系统的一层 wrapper，IPC 请求首先被 FS_Base 接收，再由 FS_Base 调用实际的文件系统进行处理。

### vnode

在 FS_Base wrapper 中，ChCore 实现了 vnode 抽象，为文件系统中的对象（如文件、目录、符号链接等）提供一个统一的表示方式。

ChCore 中 vnode 的定义为：
```C
struct fs_vnode {
        ino_t vnode_id; /* identifier */
        struct rb_node node; /* rbtree node */

        enum fs_vnode_type type; /* regular or directory */
        int refcnt; /* reference count */
        off_t size; /* file size or directory entry number */
        struct page_cache_entity_of_inode *page_cache;
        cap_t pmo_cap; /* fmap fault is handled by this */
        void *private;

        pthread_rwlock_t rwlock; /* vnode rwlock */
};
```

其中，`private` 表示文件系统特定的私有数据，例如对 inode 的引用，refcnt 代表该 vnode 被引用的次数，在下文的 `server_entry` 中会提到。

> 练习4：实现 `user/system-services/system-servers/fs_base/fs_vnode.c` 中 vnode 的 `alloc_fs_vnode`、`get_fs_vnode_by_id`、`inc_ref_fs_vnode`、`dec_ref_fs_vnode` 函数。
>
> 提示：
>
> - 你可能需要回顾Lab2中的代码去了解红黑树的操作方法。

完成练习4后，执行 `make grade`，可以得到 `Scores: 35/100`。

### server_entry

文件描述符（File Descriptor，简称 fd）是操作系统用于管理文件和其他输入/输出资源（如管道、网络连接等）的一种抽象标识符。我们来回顾一下计算机系统基础课中学习的unix文件系统抽象。在类 Unix 系统（如 Linux、macOS）中，文件描述符是一个非负整数，它指向一个内核中的文件表项，每个表项包含了文件的各种状态信息和操作方法。ChCore 将进程的 fd 保存在 chcore-libc 当中，同时在文件系统中通过 server_entry 维护了各个 Client 的 fd 的信息，把各个 Client 的 fd 和在文件系统侧的 fid 对应起来（(client_badge, fd) -> fid(server_entry)），也就是说 server_entry 对应着每个文件系统实例所对应的文件表项，其包含了对应文件表项的文件 offset 以及 `vnode` 引用。由于一个 `vnode` 可能会对应多个文件表项，所以 `vnode` 的引用数需要进行维护。

FS_Base 的 IPC handler 在处理 IPC 请求时，会先把 IPC 消息中包含的文件 fd 转换为 fid，所以我们需要把进程的 fd 和实际所对应的文件表项的映射建立起来，而在 ChCore 中对应的就是 `server_mapping` 链表。每当处理 IPC 请求时，文件系统都会通过进程发起的 badge 号找到与之对应的映射表，最终得到文件表项的 ID。

> 练习5：实现 `user/system-services/system-servers/fs_base/fs_wrapper.c` 中的 `fs_wrapper_set_server_entry` 和 `fs_wrapper_get_server_entry` 函数。
>
> 提示：
>
> - 通过全局变量 `struct list_head server_entry_mapping` 遍历 `server_entry_node`。
> - 你可以参考 `fs_wrapper_clear_server_entry` 来理解每一个变量的含义。

完成练习5后，执行 `make grade`，可以得到 `Scores: 50/100`。

### fs_wrapper_ops

当我们拥有了文件表项和VNode抽象后，我们便可以实现真正的文件系统操作了。

我们可以将 FS_Base 以及 FS_Wrapper 的所有逻辑看成一个 VFS 的通用接口，其暴露出的接口定义为 `strcut fs_server_ops`。对于每一个文件系统实例，其都需要定义一个全局的名为 `server_ops` 的全局句柄，并将实际的文件系统操作的实现注册到该句柄中。你可以通过查看 `user/system-services/system-servers/tmpfs/tmpfs.c` 中查看 ChCore 的默认 `tmpfs` 文件系统是怎么将其注册到 `FS_Wrapper` 中的。而到了实际处理文件请求时，上层的 FS_Wrapper 在响应 IPC 请求的时候，只需要调用 `server_ops` 中的函数指针即可，不需要实际真正调用每一个文件系统实现的操作函数， 这样便完成了一个统一的文件操作逻辑。例如在 `tmpfs` 中实际的读命令为 `tmpfs_read` 但在上层的 `fs_wrapper` 看来其调用只需要调用 `server_ops->read` 即可而不需要真正知晓 `tmpfs` 中的函数签名。

对于本 Lab 你只需要实现最基本的 Posix 文件操作即可，即 Open，Close，Read, Write 以及 LSeek 操作。而其下层每个文件系统除了 `Open` 操作，每当 FS_Base 尝试处理 Posix 文件请求时，其都会调用 `translate_fd_to_fid` 将对应的 `fd`  翻译成 `fid` 并重新写回 `struct fs_request` 中的 `fd`，所以请注意**不需要在实际的fs_wrapper_函数中再次调用该函数**。下面将简述一下每一个函数的语义。

对于 Open 以及 Close 来说，其主要的目的就是创建以及回收 Server Entry 即文件表项。由于在 VFS 中 VNode 的创建是动态的，所以当进程尝试发出 `Open` 中，我们需要调用与之对应的 `server_ops` 并同时分配对应的文件表项。对于每一个新增的文件表项，我们需要将其关联到对应的内存 `VNode` 中。由于文件表项所对应的 `VNode` 可能不在内存中，所以当文件系统返回 inode 号时我们需要尝试查找相应的 `vnode`，如果不存在则尝试分配并将其添加至对应的红黑树中。当完成 `VNode` 关联后，我们需要使用上一步实现的映射函数，将 `server_entry` 与用户 `fd` 映射，完成文件表项的创建。对于 `Close`，我们需要采取类似的逻辑，即回退所有的文件表项操作，减少引用计数，并尝试回收对应的系统资源。

针对 `Read/Write/Lseek` 操作，你需要参考 `man` 以及对应的 `tests/fs_test` 下的所有测试文件，按照 `Posix` 语义相应地维护 `server_entry` 以及 `vnode` 信息，并将数据返回给用户进程。

针对 mmap 操作，我们知道针对文件的 mmap 操作是采取 Demand Paging 的内存映射来实现的，当用户进程调用 `mmap` 时，FS 会首先为用户新增一个 `pmo` 即内存对象，并将其对应的类型设置为 `PMO_FILE`，并为其创建 `Page_Fault` 映射(`user/system-services/system-servers/fs_base/fs_page_fault.c`)，最后将该 `pmo` 对象发回用户进程并让其进行映射。当用户尝试访问该内存对象，并发生缺页异常时，内核会根据 pmo 的所有者(badge)将异常地址调用到对应FS处理函数进行处理，处理函数为每一个文件系统中的 `user_fault_handler`，此时 FS 服务器会根据缺页地址分配新的内存页，填充文件内容并完成缺页的处理，最终返回内核态，从而递交控制权到原来的用户进程。	

> 练习6：实现 `user/system-services/system-servers/fs_base/fs_wrapper_ops.c` 中的 `fs_wrapper_open`、`fs_wrapper_close`、`fs_wrapper_read`、`fs_wrapper_pread`、`fs_wrapper_write`、`fs_wrapper_pwrite`、`fs_wrapper_lseek`、`fs_wrapper_fmap` 函数。
>
> 提示：
>   * `user/chcore-libc/libchcore/porting/overrides/include/chcore-internal/fs_defs.h` 中定义了 `struct fs_request`，其中定义了文件系统收到的 IPC 信息所包含的数据。
>   * 针对文件表项的helper函数如 `alloc_entry` 和 `free_entry` 在 `user/system-services/system-servers/fs_base/fs_vnode.c` 中定义。
>   * `user/system-services/system-servers/tmpfs/tmpfs.c` 中定义了 tmpfs 文件系统提供的文件操作接口 server_ops，fs_wrapper 接口会调用到 server_ops 进行实际的文件操作。
>   * 用户态的所有针对文件的请求，首先会被路由到 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/file.c` 中，该文件包含了在调用 `ipc` 前后的预备和收尾工作。
>   * 你应当回顾 Lab2 的代码，去了解针对 PMO_FILE，内核是怎么处理缺页并将其转发到FS中的。同时你需要查看 `user/system-services/system-servers/fs_base/fs_page_fault.c` 中的 `page_fault` 处理函数，了解 FS 是如何处理 mmap 缺页异常的。

完成练习6后，执行 `make grade`，可以得到 `Scores: 100/100`。

> 练习7：思考 ChCore 当前实现 VFS 的方式有何利弊？如果让你在微内核操作系统上实现 VFS 抽象，你会如何实现？
