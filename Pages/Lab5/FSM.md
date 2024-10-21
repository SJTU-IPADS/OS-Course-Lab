# FSM

<!-- toc -->

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

> [!CODING] 练习题 2
> 实现 `user/system-services/system-servers/fsm/fsm.c` 的 `fsm_mount_fs` 函数。


> [!HINT] 提示：
> 你应当回顾Lab4的代码以查看ChCore是怎么基于IPC服务的cap来创建并维护连接的。

当 FSM 收到 Client 的 FSM_REQ_PARSE_PATH 类型的请求时，其首先会尝试解析 IPC 请求中访问文件的路径，通过遍历挂载信息链表，找到对应的最匹配的文件系统以及其挂载点路径。通过匹配的文件系统，获取到该文件系统的客户端 cap。如果 Client 已经获取到了文件系统的 cap，则直接返回解析后的`挂载点路径`；否则 FSM 会把挂载路径以及其对应的文件系统的 cap 也一并返回给 Client，并记录该 Client 已获取的文件系统 cap 的信息（FSM 会记录所有已经发送给某个 Client 的文件系统的 cap，见 `user/system-services/system-servers/fsm/fsm_client_cap.h`）。

> [!CODING] 练习题 3
> 实现 `user/system-services/system-servers/fsm/fsm.c` 的 IPC 请求处理函数。


> [!HINT] 提示：
>   * 完成 `user/system-services/system-servers/fsm/fsm_client_cap.c` 中的相关函数。
>   * 所有关于挂载点有关的helper函数都在 `user/system-services/system-servers/fsm/mount_info.c`
>   * IPC handler 返回的 IPC msg 的数据类型为 `struct fsm_request`，其有关的含义在 `user/chcore-libc/libchcore/porting/overrides/include/chcore-internal/fs_defs.h` 有详细的解释。
>   * 使用 `user/system-services/system-servers/fsm/mount_info.h` 定义的函数来帮助你实现 IPC handler。
>   * 你应当回顾 Lab4 代码以查看 ChCore 是怎么将 cap 对象在进程间收发的，以及 ChCore 中是怎么使用共享内存完成 IPC 调用的。
>   * 由于 printf 并不经过FS所以你可以放心使用。

我们提供了所有需要实现的文件的 Obj 版本，你可以修改 CMakeLists.txt，将编译所需的源文件从未实现的 C 文件替换为包含了正确实现的 Obj 文件，以此验证某一部分练习的正确性。如果你需要调试某一个部分，你可以将 `scripts/grade/cmakelists` 下的CMakeLists对应复制到 `FSM` 以及 `FS_Base` 的目录下覆盖并重新编译，运行 `make qemu` 后你就可以查看到 printf 的调试信息。

---

> [!SUCCESS]
> 以上为Lab5 Part2的所有内容  
> 执行 `make grade`，可以得到 `Scores: 20/100`。
