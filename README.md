# 实验5：虚拟文件系统

虚拟文件系统（Virtual File System，VFS）提供了一个抽象层，使得不同类型的文件系统可以在应用程序层面以统一的方式进行访问。这个抽象层隐藏了不同文件系统之间的差异，使得应用程序和系统内核可以以一致的方式访问各种不同类型的文件系统，如 ext4、tmpfs、 FAT32 等。在 ChCore 中，我们通过 FSM 系统服务以及 FS_Base 文件系统 wrapper 将不同的文件系统整合起来，给运行在 ChCore 上的应用提供了统一的抽象。

> 练习1：阅读 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/file.c` 的 `chcore_openat` 函数，分析 ChCore 是如何处理 `openat` 系统调用的，关注 IPC 的调用过程以及 IPC 请求的内容。

Lab5 的所有代码都运行在用户态，不同应用间通过 IPC 进行通信，可调用的 IPC 相关函数定义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore/ipc.h`。

## 第一部分：FSM

FSM 负责管理文件系统，其会处理以下类型的请求：

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

当 FSM 收到 Client FSM_REQ_MOUNT 类型的请求时，其会执行挂载文件系统的操作并**与文件系统建立 IPC 连接**。

> 练习2：实现 `user/system-services/system-servers/fsm/fsm.c` 的 `fsm_mount_fs` 函数。

当 FSM 收到 Client FSM_REQ_PARSE_PATH 类型的请求时，其首先会解析 IPC 请求中访问文件的路径，找到对应的文件系统。如果 Client 已经获取到了文件系统的 cap，则直接返回解析后的文件路径；否则 FSM 会把访问路径对应的文件系统的 cap 返回给 Client，并记录该 Client 已获取的文件系统 cap 的信息（FSM 会记录所有发送给某个 Client 的文件系统的 cap，见 `user/system-services/system-servers/fsm/fsm_client_cap.h`）。

> 练习3：实现 `user/system-services/system-servers/fsm/fsm.c` 的 IPC 请求处理函数。
> 
> 提示：
>   * 完成 `user/system-services/system-servers/fsm/fsm_client_cap.c` 中的相关函数。
>   * IPC handler 返回的 IPC msg 的数据类型为 `struct fsm_request`。
>   * 使用 `user/system-services/system-servers/fsm/mount_info.h` 定义的函数来帮助你实现 IPC handler。

我们提供了所有需要实现的文件的Obj版本，你可以修改CMakeLists.txt，将编译所需的源文件从未实现的C文件替换为包含了正确实现的Obj文件，以此验证某一部分练习的正确性。

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

其中，`private` 表示文件系统特定的私有数据，例如对 inode 的引用。

> 练习4：实现 `user/system-services/system-servers/fs_base/fs_vnode.c` 中 vnode 的 `alloc_fs_vnode`、`get_fs_vnode_by_id`、`inc_ref_fs_vnode`、`dec_ref_fs_vnode`函数。

完成练习4后，执行 `make grade`，可以得到 `Scores: 35/100`。

### server_entry

文件描述符（File Descriptor，简称 fd）是操作系统用于管理文件和其他输入/输出资源（如管道、网络连接等）的一种抽象标识符。在类 Unix 系统（如 Linux、macOS）中，文件描述符是一个非负整数，它指向一个内核中的文件表项，每个表项包含了文件的各种状态信息和操作方法。ChCore 将进程的 fd 保存在 chcore-libc 当中，同时在文件系统中通过 server_entry 维护了各个 Client 的 fd 的信息，把各个 Client 的 fd 和在文件系统侧的 fid 对应起来（(client_badge, fd) -> fid(server_entry)）。

 FS_Base 的 IPC handler 在处理 IPC 请求时，会先把 IPC 消息中包含的文件 fd 转换为 fid。

> 练习5：实现 `user/system-services/system-servers/fs_base/fs_wrapper.c` 中的 `fs_wrapper_set_server_entry` 和 `fs_wrapper_get_server_entry` 函数。
>
> 提示：通过全局变量 `struct list_head server_entry_mapping` 遍历 `server_entry_node`。

完成练习5后，执行 `make grade`，可以得到 `Scores: 50/100`。

### fs_wrapper_ops

接下来，你需要在 FS_Base 中实现基本的文件操作接口。

> 练习6：实现 `user/system-services/system-servers/fs_base/fs_wrapper_ops.c` 中的 `fs_wrapper_open`、`fs_wrapper_close`、`fs_wrapper_read`、`fs_wrapper_pread`、`fs_wrapper_write`、`fs_wrapper_pwrite`、`fs_wrapper_lseek`、`fs_wrapper_fmap` 函数。
>
> 提示：
>   * `user/chcore-libc/libchcore/porting/overrides/include/chcore-internal/fs_defs.h` 中定义了 `struct fs_request`，其中定义了文件系统收到的 IPC 信息所包含的数据。
>   * `user/system-services/system-servers/tmpfs/tmpfs.c` 中定义了 tmpfs 文件系统提供的文件操作接口 server_ops，fs_wrapper 接口会调用到 server_ops 进行实际的文件操作。

完成练习6后，执行 `make grade`，可以得到 `Scores: 100/100`。

> 练习7：思考 ChCore 当前实现 VFS 的方式有何利弊？如果让你在微内核操作系统上实现 VFS 抽象，你会如何实现？