# 实验 2：内存管理

本实验主要目的在于让同学们熟悉内核启动过程中对内存的初始化和内核启动后对物理内存和页表的管理，包括三个部分：物理内存管理、页表管理、缺页异常处理。

本实验代码包含了基础的 ChCore 操作系统镜像，除了练习题相关部分的源码以外（指明需要阅读的代码），其余部分通过二进制格式提供。
正确完成本实验的练习题之后，你可以在树莓派3B+QEMU或开发板上进入 ChCore shell。

## 第一部分：物理内存管理

内核初始化过程中，需要对内存管理模块进行初始化（`mm_init` 函数），首先需要把物理内存管起来，从而使内核代码可以动态地分配内存。

ChCore 使用伙伴系统（buddy system）[^buddy]对物理页进行管理，在 `mm_init` 中对伙伴系统进行了初始化。为了使物理内存的管理可扩展，ChCore 在 `mm_init` 的开头首先调用平台特定的 `parse_mem_map` 函数，该函数解析并返回了可用物理内存区域，然后再对各可用物理内存区域初始化伙伴系统。

[^buddy]: 操作系统：原理与实现，5.1.3 伙伴系统原理

伙伴系统中的每个内存块都有一个阶（order）表示大小，阶是从 0 到指定上限 `BUDDY_MAX_ORDER` 的整数。一个 $n$ 阶的块的大小为 $2^n \times PAGE\\_SIZE$，因此这些内存块的大小正好是比它小一个阶的内存块的大小的两倍。内存块的大小是 2 次幂对齐，使地址计算变得简单。当一个较大的内存块被分割时，它被分成两个较小的内存块，这两个小内存块相互成为唯一的伙伴。一个分割的内存块也只能与它唯一的伙伴块进行合并（合并成他们分割前的块）。

ChCore 中每个由伙伴系统管理的内存区域称为一个 `struct phys_mem_pool`，该结构体中包含物理页元信息的起始地址（`page_metadata`）、伙伴系统各阶内存块的空闲链表（`free_lists`）等。

> 练习题 1：完成 `kernel/mm/buddy.c` 中的 `split_chunk`、`merge_chunk`、`buddy_get_pages`、 和 `buddy_free_pages` 函数中的 `LAB 2 TODO 1` 部分，其中 `buddy_get_pages` 用于分配指定阶大小的连续物理页，`buddy_free_pages` 用于释放已分配的连续物理页。
>
> 提示：
>
> - 可以使用 `kernel/include/common/list.h` 中提供的链表相关函数和宏如 `init_list_head`、`list_add`、`list_del`、`list_entry` 来对伙伴系统中的空闲链表进行操作
> - 可使用 `get_buddy_chunk` 函数获得某个物理内存块的伙伴块
> - 更多提示见代码注释

我们希望通过基于伙伴系统的物理内存管理，在内核中进行动态内存分配，也就是可以使用 `kmalloc` 函数（对应用户态的 `malloc`）。ChCore 的 `kmalloc` 对于较小的内存分配需求采用 SLAB 分配器[^slab]，对于较大的分配需求则直接从伙伴系统中分配物理页。动态分配出的物理页被转换为内核虚拟地址（Kernel Virtual Address，KVA），也就是在 LAB 1 中我们映射的 `0xffff_ff00_0000_0000` 之后的地址。我们在练习题 1 中已经实现了伙伴系统，接下来让我们实现 SLAB 分配器吧。

[^slab]: 操作系统：原理与实现，5.1.5 SLAB 分配器的基本设计

> 练习题 2：完成 `kernel/mm/slab.c` 中的 `choose_new_current_slab`、`alloc_in_slab_impl` 和 `free_in_slab` 函数中的 `LAB 2 TODO 2` 部分，其中 `alloc_in_slab_impl` 用于在 slab 分配器中分配指定阶大小的内存，而 `free_in_slab` 则用于释放上述已分配的内存。
>
> 提示：
>
> - 你仍然可以使用上个练习中提到的链表相关函数和宏来对 SLAB 分配器中的链表进行操作
> - 更多提示见代码注释

有了伙伴系统和 SLAB 分配器，就可以实现 `kmalloc` 了。

> 练习题 3：完成 `kernel/mm/kmalloc.c` 中的 `_kmalloc` 函数中的 `LAB 2 TODO 3` 部分，在适当位置调用对应的函数，实现 `kmalloc` 功能
>
> 提示：
>
> - 你可以使用 `get_pages` 函数从伙伴系统中分配内存，使用 `alloc_in_slab` 从 SLAB 分配器中分配内存
> - 更多提示见代码注释

现在内核中已经能够正常使用 `kmalloc` 和 `kfree` 了！

正确完成这一部分的练习题后，运行 `make grade`，你应当能够得到 30 分。注意，测试可能会遗漏你代码中的一些问题。因此即使通过这部分测试，代码中的隐藏问题也可能会对后续实验产生影响导致无法通过最终的测试。不过，我们会按照 `make grade` 的结果为你计分。^_^

## 第二部分：页表管理

在 LAB 1 中我们已经详细介绍了 AArch64 的地址翻译过程，并介绍了各级页表和不同类型的页表描述符，最后在内核启动阶段配置了一个粗粒度的启动页表。现在，我们需要为用户态应用程序准备一个更细粒度的页表实现，提供映射、取消映射、查询等功能。

> 练习题 4：完成 `kernel/arch/aarch64/mm/page_table.c` 中的 `query_in_pgtbl`、`map_range_in_pgtbl_common`、`unmap_range_in_pgtbl` 和 `mprotect_in_pgtbl` 函数中的 `LAB 2 TODO 4` 部分，分别实现页表查询、映射、取消映射和修改页表权限的操作，以 4KB 页为粒度。
>
> 提示：
>
> - 需要实现的函数内部无需刷新 TLB，TLB 刷新会在这些函数的外部进行
> - 实现中可以使用 `get_next_ptp`、`set_pte_flags`、`virt_to_phys`、`GET_LX_INDEX` 等已经给定的函数和宏
> - `map_range_in_pgtbl_common`、`unmap_range_in_pgtbl`、`get_next_ptp` 中的 `rss` 参数是用于统计物理内存使用量的[^rss]，在本实验中你不必实现该功能，该功能的正确实现与否不影响本题得分。在调用 `get_next_ptp` 时可以直接使用 `NULL` 作为 `rss` 参数。（当然你也可以选择正确实现这一功能）
> - 更多提示见代码注释

[^rss]: https://en.wikipedia.org/wiki/Resident_set_size

正确完成该练习题后，运行 `make grade`，你应当能够得到 70 分。同样的，正确实现功能是通过测试的充分非必要条件。

> 思考题 5：阅读 Arm Architecture Reference Manual，思考要在操作系统中支持写时拷贝（Copy-on-Write，CoW）[^cow]需要配置页表描述符的哪个/哪些字段，并在发生页错误时如何处理。（在完成第三部分后，你也可以阅读页错误处理的相关代码，观察 ChCore 是如何支持 Cow 的）

[^cow]: 操作系统：原理与实现，12.4 原子更新技术：写时拷贝

> 思考题 6：为了简单起见，在 ChCore 实验 Lab1 中没有为内核页表使用细粒度的映射，而是直接沿用了启动时的粗粒度页表，请思考这样做有什么问题。

> 挑战题 7：使用前面实现的 `page_table.c` 中的函数，在内核启动后的 `main` 函数中重新配置内核页表，进行细粒度的映射。

## 第三部分：缺页异常处理

缺页异常（page fault）是操作系统实现延迟内存分配的重要技术手段。当处理器发生缺页异常时，它会将发生错误的虚拟地址存储于 `FAR_ELx` 寄存器中，并触发相应的异常处理流程。ChCore 对该异常的处理最终实现在 `kernel/arch/aarch64/irq/pgfault.c` 中的 `do_page_fault` 函数。本次实验暂时不涉及前面的异常初步处理及转发相关内容，我们仅需要关注操作系统是如何处缺页异常的。

> 练习题 8: 完成 `kernel/arch/aarch64/irq/pgfault.c` 中的 `do_page_fault` 函数中的 `LAB 2 TODO 5` 部分，将缺页异常转发给 `handle_trans_fault` 函数。

在 ChCore 中，一个进程的虚拟地址空间由多段“虚拟地址区域”（VMR，又称 VMA）组成，一段 VMR 记录了这段虚拟地址对应的“物理内存对象”（PMO），而 PMO 中则记录了物理地址相关信息。因此，想要处理缺页异常，首先需要找到当前进程发生页错误的虚拟地址所处的 VMR，进而才能得知其对应的物理地址，从而在页表中完成映射。

> 练习题 9: 完成 `kernel/mm/vmspace.c` 中的 `find_vmr_for_va` 函数中的 `LAB 2 TODO 6` 部分，找到一个虚拟地址找在其虚拟地址空间中的 VMR。
>
> 提示：
>
> - 一个虚拟地址空间所包含的 VMR 通过 rb_tree 的数据结构保存在 `vmspace` 结构体的 `vmr_tree` 字段
> - 可以使用 `kernel/include/common/rbtree.h` 中定义的 `rb_search`、`rb_entry` 等函数或宏来对 rb_tree 进行搜索或操作

缺页处理主要针对 `PMO_SHM` 和 `PMO_ANONYM` 类型的 PMO，这两种 PMO 的物理页是在访问时按需分配的。缺页处理逻辑为首先尝试检查 PMO 中当前 fault 地址对应的物理页是否存在（通过 `get_page_from_pmo` 函数尝试获取 PMO 中 offset 对应的物理页）。若对应物理页未分配，则需要分配一个新的物理页，将将页记录到 PMO 中，并增加页表映射。若对应物理页已分配，则只需要修改页表映射即可。

> 练习题 10: 完成 `kernel/mm/pgfault_handler.c` 中的 `handle_trans_fault` 函数中的 `LAB 2 TODO 7` 部分（函数内共有 3 处填空，不要遗漏），实现 `PMO_SHM` 和 `PMO_ANONYM` 的按需物理页分配。你可以阅读代码注释，调用你之前见到过的相关函数来实现功能。

正确完成上述练习题后，运行 `make qemu` 后 ChCore 应当能正常进入 Shell；运行 `make grade`，你应当能够得到 100 分。如果你无法通过测试，请考虑到也有可能是你前面两个部分的实现存在漏洞。

> 挑战题 11: 由于没有磁盘，因此我们采用一部分内存模拟磁盘。内存页是可以换入换出的，请设计一套换页策略（如 LRU 等），并在 `kernel/mm/pgfault_handler.c` 中的的适当位置简单实现你的换页方法。
