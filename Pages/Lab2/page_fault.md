# 缺页异常处理

<!-- toc -->

缺页异常（page fault）是操作系统实现延迟内存分配的重要技术手段。当处理器发生缺页异常时，它会将发生错误的虚拟地址存储于 `FAR_ELx` 寄存器中，并触发相应的异常处理流程。ChCore 对该异常的处理最终实现在 `kernel/arch/aarch64/irq/pgfault.c` 中的 `do_page_fault` 函数。本次实验暂时不涉及前面的异常初步处理及转发相关内容，我们仅需要关注操作系统是如何处缺页异常的。

> [!CODING] 练习题8
> 完成 `kernel/arch/aarch64/irq/pgfault.c` 中的 `do_page_fault` 函数中的 `LAB 2 TODO 5` 部分，将缺页异常转发给 `handle_trans_fault` 函数。

在 ChCore 中，一个进程的虚拟地址空间由多段“虚拟地址区域”（VMR，又称 VMA）组成，一段 VMR 记录了这段虚拟地址对应的“物理内存对象”（PMO），而 PMO 中则记录了物理地址相关信息。因此，想要处理缺页异常，首先需要找到当前进程发生页错误的虚拟地址所处的 VMR，进而才能得知其对应的物理地址，从而在页表中完成映射。

> [!CODING] 练习题9
> 完成 `kernel/mm/vmspace.c` 中的 `find_vmr_for_va` 函数中的 `LAB 2 TODO 6` 部分，找到一个虚拟地址找在其虚拟地址空间中的 VMR。

> [!HINT]
>
> - 一个虚拟地址空间所包含的 VMR 通过 rb_tree 的数据结构保存在 `vmspace` 结构体的 `vmr_tree` 字段
> - 可以使用 `kernel/include/common/rbtree.h` 中定义的 `rb_search`、`rb_entry` 等函数或宏来对 rb_tree 进行搜索或操作

缺页处理主要针对 `PMO_SHM` 和 `PMO_ANONYM` 类型的 PMO，这两种 PMO 的物理页是在访问时按需分配的。缺页处理逻辑为首先尝试检查 PMO 中当前 fault 地址对应的物理页是否存在（通过 `get_page_from_pmo` 函数尝试获取 PMO 中 offset 对应的物理页）。若对应物理页未分配，则需要分配一个新的物理页，再将页记录到 PMO 中，并增加页表映射。若对应物理页已分配，则只需要修改页表映射即可。

> [!CODING] 练习题10
> 完成 `kernel/mm/pgfault_handler.c` 中的 `handle_trans_fault` 函数中的 `LAB 2 TODO 7` 部分（函数内共有 3 处填空，不要遗漏），实现 `PMO_SHM` 和 `PMO_ANONYM` 的按需物理页分配。你可以阅读代码注释，调用你之前见到过的相关函数来实现功能。

> [!CHALLENGE] 挑战题 11
> 我们在`map_range_in_pgtbl_common`、`unmap_range_in_pgtbl` 函数中预留了没有被使用过的参数`rss` 用来来统计map映射中实际的物理内存使用量[^rss]，
> 你需要修改相关的代码来通过`Compute physical memory`测试，不实现该挑战题并不影响其他部分功能的实现及测试。如果你想检测是否通过此部分测试，需要修改`kernel/config.cmake`中`CHCORE_KERNEL_PM_USAGE_TEST`为ON

---

> [!CHALLENGE]
> 为了防止你通过尝试在打印scores.json里的内容来逃避检查，我们在每一次评分前都会修改elf序列号段的信息并让其chcore在评分点进行打出，当且仅当评分程序捕捉到序列号输出之后，我们才会对检查进行打分，如果没有通过序列号验证，你的评分将为0分。

> [!SUCCESS]
> 以上为Lab2 Part3。
> 正确完成上述练习题后，运行 `make qemu` 后 ChCore 应当能正常进入 Shell；运行 `make grade`，你应当能够得到 100 分。如果你无法通过测试，请考虑到也有可能是你前面两个部分的实现存在漏洞。

[^rss]: <https://en.wikipedia.org/wiki/Resident_set_size>
