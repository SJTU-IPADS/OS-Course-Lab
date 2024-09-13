# 物理内存管理

<!-- toc -->

## 伙伴系统

内核初始化过程中，需要对内存管理模块进行初始化（`mm_init` 函数），首先需要把物理内存管起来，从而使内核代码可以动态地分配内存。

ChCore 使用伙伴系统（buddy system）[^buddy]对物理页进行管理，在 `mm_init` 中对伙伴系统进行了初始化。为了使物理内存的管理可扩展，ChCore 在 `mm_init` 的开头首先调用平台特定的 `parse_mem_map` 函数，该函数解析并返回了可用物理内存区域，然后再对各可用物理内存区域初始化伙伴系统。


伙伴系统中的每个内存块都有一个阶（order）表示大小，阶是从 0 到指定上限 `BUDDY_MAX_ORDER` 的整数。一个 $n$ 阶的块的大小为 $2^n \times PAGE\\_SIZE$，因此这些内存块的大小正好是比它小一个阶的内存块的大小的两倍。内存块的大小是 2 次幂对齐，使地址计算变得简单。当一个较大的内存块被分割时，它被分成两个较小的内存块，这两个小内存块相互成为唯一的伙伴。一个分割的内存块也只能与它唯一的伙伴块进行合并（合并成他们分割前的块）。

ChCore 中每个由伙伴系统管理的内存区域称为一个 `struct phys_mem_pool`，该结构体中包含物理页元信息的起始地址（`page_metadata`）、伙伴系统各阶内存块的空闲链表（`free_lists`）等。

> [!CODING] 练习题1
> 完成 `kernel/mm/buddy.c` 中的 `split_chunk`、`merge_chunk`、`buddy_get_pages`、 和 `buddy_free_pages` 函数中的 `LAB 2 TODO 1` 部分，其中 `buddy_get_pages` 用于分配指定阶大小的连续物理页，`buddy_free_pages` 用于释放已分配的连续物理页。

> [!HINT]
>
> - 可以使用 `kernel/include/common/list.h` 中提供的链表相关函数和宏如 `init_list_head`、`list_add`、`list_del`、`list_entry` 来对伙伴系统中的空闲链表进行操作
> - 可使用 `get_buddy_chunk` 函数获得某个物理内存块的伙伴块
> - 更多提示见代码注释

## SLAB分配器

我们希望通过基于伙伴系统的物理内存管理，在内核中进行动态内存分配，也就是可以使用 `kmalloc` 函数（对应用户态的 `malloc`）。ChCore 的 `kmalloc` 对于较小的内存分配需求采用 SLAB 分配器[^slab]，对于较大的分配需求则直接从伙伴系统中分配物理页。动态分配出的物理页被转换为内核虚拟地址（Kernel Virtual Address，KVA），也就是在 LAB 1 中我们映射的 `0xffff_ff00_0000_0000` 之后的地址。我们在练习题 1 中已经实现了伙伴系统，接下来让我们实现 SLAB 分配器吧。


> [!CODING] 练习题2
> 完成 `kernel/mm/slab.c` 中的 `choose_new_current_slab`、`alloc_in_slab_impl` 和 `free_in_slab` 函数中的 `LAB 2 TODO 2` 部分，其中 `alloc_in_slab_impl` 用于在 slab 分配器中分配指定阶大小的内存，而 `free_in_slab` 则用于释放上述已分配的内存。

> [!HINT]
>
> - 你仍然可以使用上个练习中提到的链表相关函数和宏来对 SLAB 分配器中的链表进行操作
> - 更多提示见代码注释

> [!SUCCESS] kmalloc
> 现在内核中已经能够正常使用 `kmalloc` 和 `kfree` 了


> [!SUCCESS]
> 以上为Lab2 Part1的所有内容。
> 正确完成这一部分的练习题后，运行 `make grade`，你应当能够得到 30 分。注意，测试可能会遗漏你代码中的一些问题。因此即使通过这部分测试，代码中的隐藏问题也可能会对后续实验产生影响导致无法通过最终的测试。不过，我们会按照 `make grade` 的结果为你计分。^_^

[^buddy]: 操作系统：原理与实现，5.1.3 伙伴系统原理
[^slab]: 操作系统：原理与实现，5.1.5 SLAB 分配器的基本设计
