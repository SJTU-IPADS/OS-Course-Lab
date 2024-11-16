# 页表管理

<!-- toc -->

在[LAB 1](../Lab1/pte.html) 中我们已经详细介绍了 AArch64 的地址翻译过程，并介绍了各级页表和不同类型的页表描述符，最后在内核启动阶段配置了一个粗粒度的启动页表。现在，我们需要为用户态应用程序准备一个更细粒度的页表实现，提供映射、取消映射、查询等功能。

> [!CODING] 练习题4
> 完成 `kernel/arch/aarch64/mm/page_table.c` 中的 `query_in_pgtbl`、`map_range_in_pgtbl_common`、`unmap_range_in_pgtbl` 和 `mprotect_in_pgtbl` 函数中的 `LAB 2 TODO 4` 部分，分别实现页表查询、映射、取消映射和修改页表权限的操作，以 4KB 页为粒度。

> [!HINT]
>
> - 需要实现的函数内部无需刷新 TLB，TLB 刷新会在这些函数的外部进行
> - 实现中可以使用 `get_next_ptp`、`set_pte_flags`、`virt_to_phys`、`GET_LX_INDEX` 等已经给定的函数和宏
> - 更多提示见代码注释

> [!NOTE] 页表配错了怎么办？  
> 在Aarch64的架构中，每当系统进入异常处理流程，寄存器`ELR_EL1`将保存错误发生的指令地址，而对于出错的虚拟内存地址，你可以通过查询`FAR_EL1`找到。

> [!QUESTION] 思考题5
> 阅读 Arm Architecture Reference Manual，思考要在操作系统中支持写时拷贝（Copy-on-Write，CoW）[^cow]需要配置页表描述符的哪个/哪些字段，并在发生页错误时如何处理。（在完成第三部分后，你也可以阅读页错误处理的相关代码，观察 ChCore 是如何支持 Cow 的）

> [!QUESTION] 思考题6
> 为了简单起见，在 ChCore 实验 Lab1 中没有为内核页表使用细粒度的映射，而是直接沿用了启动时的粗粒度页表，请思考这样做有什么问题。

> [!CHALLENGE] 挑战题7
> 使用前面实现的 `page_table.c` 中的函数，在内核启动后的 `main` 函数中重新配置内核页表，进行细粒度的映射。

---

> [!SUCCESS]
> 以上为Lab2 Part2的所有内容
> 
> 正确完成该练习题后，运行 `make grade`，你应当能够得到 70 分。同样的，正确实现功能是通过测试的充分非必要条件。

[^cow]: 操作系统：原理与实现，12.4 原子更新技术：写时拷贝
