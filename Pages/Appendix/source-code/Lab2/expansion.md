# 一些说明

本文章以Lab内容为基础进行相关拓展，主要涉及**物理内存分配器**,  内核态与用户态在内存管理上的不同 。本文不涉及伙伴系统 ，页表管理 ，缺页管理 。

# **物理内存分配器**

## 背景

内核中的物理内存由伙伴系统(buddy system)进行管理，它的分配粒度是以物理页帧(4KB)为单位的，但内核中有大量的数据结构只需要若干bytes的空间，倘若仍按页来分配，势必会造成大量的内存被浪费掉。slab分配器的出现就是为了解决内核中这些小块内存分配与管理的难题。

在内核的不断演进过程中，出现了三种物理内存分配器，slab，slob，slub。其中slab是最早的内存的分配器，由于有诸多的问题，后来被slob以及slub取代了。而slob在主要用于内存较小的嵌入式系统。Slub由于支持NUMA架构以及诸多的优点，逐渐的成为了当前内核中的主流内存分配器。我们已经在实验中了解了slab的内存分配功能，下面将介绍slab的另外两个功能以及slub。

## SLAB 的第二使命 ：维护常用对象的缓存 （简称对象缓存）

Slab 分配器的核心优势不仅在于高效地管理小内存块，还在于它能够**缓存常用对象**，使得对象在释放后能够被快速复用，而不需要每次重新初始化和分配。这个机制在内核中极为重要，因为许多数据结构需要频繁分配和释放，而初始化这些结构的开销可能**与分配内存本身的开销相当**，甚至更高。

### 在 Slab 分配器中，对象缓存（Object Caching）指的是：

- 释放的对象不会立即归还给**伙伴系统（Buddy System）**，而是被**保留在 Slab 的内部缓存中**。
- 当同类型的对象再次需要分配时，**优先复用最近释放的对象**，而不是重新向伙伴系统申请内存并进行初始化。
- 由于释放的对象仍然保留在 Slab 缓存中，它们的物理地址通常不会改变，因此仍然可能**驻留在 CPU 的 L1/L2 缓存中**，从而提高访问速度。

这种机制在 Linux 内核中广泛应用于需要**频繁创建和销毁的内核数据结构**，如：

- fs_struct（管理进程的文件系统信息）
- task_struct（进程描述符）
- inode（文件索引节点）

### 那么为什么需要对象缓存:

**（1）减少初始化开销**

某些数据结构的初始化开销**远远大于**分配它们的内存开销。例如，fs_struct 结构用于存储进程的文件系统信息，当一个进程创建时，内核必须为其分配 fs_struct，并初始化其中的多个字段。这个初始化过程可能包括：	

- 设置默认值
- 复制文件系统信息
- 分配多个子结构体

如果每次进程创建时都需要执行完整的初始化，系统开销将会非常大。因此，Slab 分配器允许**已经初始化的对象在释放后保留在缓存中**，下次分配时可以直接复用，大大减少初始化成本。

**（2）提高内存分配效率**

传统的伙伴系统适用于管理**大块内存**，但对**小对象的频繁分配和释放**效率较低。Slab 分配器的对象缓存机制可以避免频繁调用伙伴系统，从而：

- **减少锁争用**（伙伴系统的操作通常需要全局锁（在完成lab2的时候我们经常见到全局锁），而 Slab 允许局部缓存）
- **降低碎片化**（伙伴系统可能因小对象频繁分配和释放导致碎片化）

**（3）提高 CPU 缓存命中率**

当一个对象被释放后，它仍然可能**留在 CPU 的缓存（L1/L2 Cache）中。如果系统很快又需要一个相同类型的对象，那么直接复用这个对象可以避免 CPU 重新加载内存**，从而提高访问速度。例如：

- 当一个 fs_struct 释放后，Slab 分配器不会立即释放其内存，而是保留在缓存中。
- 由于 fs_struct 仍然驻留在 CPU 缓存中，下次再创建进程时，内核可以直接复用该结构，减少访问 DRAM 的延迟。

### **Slab 分配器对象缓存的实现**

**（1）创建 Slab 缓存**

在 Linux 内核中，每种需要频繁分配的对象类型，都会有一个专门的 **Slab 缓存（Slab Cache）**，用于存储该类型的对象。例如，Linux 通过 kmem_cache_create() 创建 fs_struct 缓存：

```c
fs_cachep = kmem_cache_create("fs_cache", sizeof(struct fs_struct), 0, SLAB_HWCACHE_ALIGN, NULL);
```

fs_cachep 是 fs_struct 类型的 Slab 缓存指针，该缓存会管理多个 fs_struct 结构，并在释放后保持缓存状态。

**（2）分配对象**

当需要分配 fs_struct 时，内核会调用：

```c
struct fs_struct *fs = kmem_cache_alloc(fs_cachep, GFP_KERNEL);
```

如果 fs_cachep 里有空闲对象，直接返回一个**最近释放的对象，**如果缓存已满，则**分配一个新的 Slab 以此来**避免使用伙伴系统，提高分配效率。

**（3）释放对象**

当 fs_struct 不再需要时，调用：

```c
kmem_cache_free(fs_cachep, fs);
```

fs 并不会被立即释放，而是放入 fs_cachep 的**空闲对象列表。**这样，当下次 kmem_cache_alloc() 需要一个 fs_struct 时，可以直接复用，避免重新分配和初始化。

注：这里“直接复用”的表述不太准确，两次所使用的结构体参数可能不同，而slab也有相应的解决方案，这里为了不过多赘述使用“直接复用”的说法。有关问题可以自行搜索。

## SLAB 的第三使命 ：提高CPU硬件缓存的利用率

### **CPU 缓存与 SLAB 之间的关系**

在现代计算机系统中，**CPU 访问缓存的速度远远快于访问主存（RAM）**。在 CPU 访问数据时，数据首先会被加载到**L1/L2/L3 缓存**中。CPU 读取缓存的速度要比访问主存快 10~100 倍，因此，如果 SLAB 分配器能够优化对象的存储方式，使其尽可能多地驻留在缓存中，就能显著提升系统性能。SLAB 分配器的设计目标之一就是**让分配的对象尽可能驻留在 CPU 缓存中，而不需要频繁访问主存**。

### 优化策略

SLAB 分配器通过以下几种方式优化 CPU 缓存的利用率：

- **分配对齐（Cache Alignment）**
    
    在 SLAB 分配对象时，内核会尝试让对象**按 CPU 缓存行（Cache Line）大小对齐**，从而减少**缓存行拆分（Cache Line Splitting）** 和 **伪共享（False Sharing）**。
    
- **减少缓存污染（Cache Pollution）**
    
    如果内核频繁分配和释放对象，而这些对象的地址在物理内存中分散，那么 CPU 缓存可能会被无用的数据填满，导致缓存命中率下降。这种情况称为**缓存污染（Cache Pollution）**。
    
     **SLAB 如何减少缓存污染？**
    
    1. **优先复用最近释放的对象**，而不是重新分配新内存。
    2. **局部性优化（Locality Optimization）**，让同一类对象尽量存储在相邻的缓存行中，提高访问连续性。
- **提高缓存命中率（Cache Hit Rate）**
    
    SLAB 分配器通过**空间局部性（Spatial Locality）** 和 **时间局部性（Temporal Locality）** 机制，提高 CPU 缓存的命中率。
    
    **空间局部性优化：**SLAB 让**相同类型的对象存储在连续的内存块中**，使得 CPU 在访问一个对象时，能够**预加载**相邻的对象，提高缓存命中率。
    
    例如：
    
    ```c
    struct task_struct *t1 = kmem_cache_alloc(task_cache, GFP_KERNEL);
    struct task_struct *t2 = kmem_cache_alloc(task_cache, GFP_KERNEL);
    ```
    
    如果 t1 和 t2 被分配到**连续的物理地址**，那么当 CPU 读取 t1 时，很可能会**预加载** t2，提高缓存利用率。
    
    **时间局部性优化：SLAB 让最近使用的对象尽可能留在缓存中**，如果短时间内需要再次使用相同的对象，就可以**避免重新加载主存**。
    
    例如，当进程频繁创建/销毁 fs_struct，SLAB 会优先复用**最近释放**的 fs_struct，因为它仍然可能在 L1/L2 缓存中。
    
- **NUMA 亲和性优化（NUMA Affinity）**

## SLUB 简介

SLUB 分配器是 Linux 内核中的一种 **优化版 SLAB 分配器**，它的目标是 **提高内存分配的效率，减少锁争用，并降低内存碎片化**。相比 SLAB，SLUB 设计更简单、性能更好，因此是 Linux 内核的默认分配器。下面讲述两者不同点。

### 管理结构

**SLAB**：使用单独的 slab 结构来管理每个 slab 及其中的对象。每个 kmem_cache 对象有一个对应的 slab 列表，用来管理具体的内存分配。

```c
struct kmem_cache {
    const char *name;
    size_t obj_size;
    unsigned int obj_per_slab;
    struct slab *slab_list;  // 存储 slab 列表
};

struct slab {
    struct page *pages;  // 对应的页结构
    struct kmem_cache *cache;
    void *freelist;  // 空闲对象链表
};
```

**SLUB**：SLUB 去除了 slab 结构，直接使用 struct page 来管理 slab 中的对象，简化了内存管理结构。每个 CPU 都有自己的 kmem_cache_cpu，管理空闲对象列表。

```c
struct kmem_cache {
    const char *name;
    size_t obj_size;
    struct page *page_list;  // 使用 page 结构代替 slab
};

struct kmem_cache_cpu {
    void *freelist;  // 每个 CPU 都有自己的空闲对象链表
};

struct page {
    struct kmem_cache *cache;
    void *objects;  // 对象的内存区域
};
```

### **锁机制**

**SLAB**：由于多个 CPU 可能会同时访问同一个 slab，所以需要通过加锁来同步访问。每个 kmem_cache 都有一个全局的锁来管理 slab。

```c
struct kmem_cache {
    spinlock_t lock;  // 用于保护 kmem_cache 的访问
};
```

**SLUB**：SLUB 采用每个 CPU 独立管理空闲对象列表（freelist），因此在大多数情况下，不需要加锁，从而避免了锁争用，提高了并发性能。

```c
struct kmem_cache_cpu {
    void *freelist;  // 每个 CPU 独立管理，不需要加锁
};
```

SLUB 基于每个 CPU 有独立的 freelist，这避免了对共享资源的频繁锁操作，因此在多核系统中，SLUB 具有更好的性能，尤其是在高并发场景下。

### **内存碎片**

**SLAB**：由于每个 slab 内存块的大小是固定的，可能会导致对象大小与 slab 大小不匹配，产生内部碎片。此外，slab 管理结构复杂，容易产生外部碎片。

**SLUB**：SLUB 通过动态调整 slab 的大小，更好地利用内存，减少碎片。此外，由于 SLUB 直接通过 page 来管理内存，内存的布局更加灵活，减少了内存碎片的发生。

### **NUMA 亲和性**

**SLAB**：SLAB 在 NUMA 系统上性能较差，因为它在内存分配时不会考虑 CPU 的亲和性，可能会导致远程 NUMA 节点的内存访问延迟。

**SLUB**：SLUB 提供了更好的 NUMA 亲和性，它会尝试将内存分配限制在当前 CPU 所在的 NUMA 节点内，从而减少跨节点的内存访问，提高性能。

**SLUB 代码：**

```c
struct kmem_cache_cpu {
    void *freelist;  // 每个 CPU 独立管理，支持 NUMA 亲和性
};

struct page {
    unsigned long node_id;  // 页面所属的 NUMA 节点
};
```

### **空闲对象的回收与分配**

**SLAB**：SLAB 使用 slab 结构来回收和分配内存。每个 slab 对象包含多个内存对象，空闲对象被存放在 freelist 中，当需要新的对象时，从 freelist 中分配。

**SLAB 代码：**

```c
void *slab_alloc(struct kmem_cache *cache) {
    struct slab *slab;
    if (slab->freelist) {
        return slab->freelist;  // 从 freelist 中分配
    }
    // 如果 freelist 为空，申请新的 slab
    return slab_get_new(cache);
}
```

**SLUB**：SLUB 在分配对象时，首先检查当前 CPU 的 freelist，如果没有空闲对象，则从 page 中分配新的对象。当对象被释放时，它会被返回到 freelist。

**SLUB 代码：**

```c
void *slub_alloc(struct kmem_cache *cache) {
    struct kmem_cache_cpu *cpu_cache = &per_cpu(cache->cpu_cache, smp_processor_id());
    if (cpu_cache->freelist) {
        return cpu_cache->freelist;  // 从当前 CPU 的 freelist 中分配
    }
    // 如果 freelist 为空，申请新的 page
    return slub_get_new(cache);
}
```

由于 SLUB 支持每个 CPU 独立管理空闲对象，避免了共享数据结构的锁竞争，在高并发和高负载的环境下，分配和回收对象的效率更高。

参考论文：

- https://github.com/0voice/computer_expert_paper/blob/main/%E6%8E%A5%E8%BF%91%E5%8E%9F%E5%A7%8B%E7%9A%84LinuxOS/%E3%80%8ASlab%20allocators%20in%20the%20Linux%20KernelSLAB%2C%20SLOB%2C%20SLUB%E3%80%8B.pdf
- https://github.com/0voice/computer_expert_paper/blob/main/%E6%8E%A5%E8%BF%91%E5%8E%9F%E5%A7%8B%E7%9A%84LinuxOS/%E3%80%8AThe%20Slab%20AllocatorAn%20Object-Caching%20Kernel%20Memory%20Allocator%E3%80%8B.pdf

# 用户态 VS 内核态

在内存管理方面，**内核态**和**用户态**有很多本质的区别，这些区别源于两者在设计目标、内存访问权限、以及管理方式等方面的差异。下面我将从几个主要角度对比并说明。

## **内存管理的目标与设计**

### **内核态**

内核的主要目的是提供操作系统核心功能，如进程调度、硬件驱动、文件系统等。在内存管理上，内核态更关注 **高效的内存访问** 和 **内存保护**，确保内存的安全性和稳定性。内核通常要处理大量的系统资源管理，因此内存分配机制在内核态上需要具备较强的 **简单性** 和 **高效性**，且往往采用 **固定的粒度**（如页或 slab 级别）来分配内存，以减少碎片问题。

**内核内存管理**的关键：

- **内核通过内存池、缓存优化时间局部性**
    
    内核内存池和缓存的优化机制可以大大提升内存访问的效率。通过 **SLAB 分配器**，内核可以创建缓存池来存储常用的内存对象，以避免频繁的分配和回收。内存池还可以通过维护一些常用对象的缓存，提高内存的 **时间局部性**，从而减少内存访问的延迟。例如，SLAB 分配器通过将内存块组织成一个个对象的缓存池，可以有效地减少碎片，提高分配和回收的效率。
    
    ```c
    struct kmem_cache *create_cache(const char *name, size_t size)
    {
        struct kmem_cache *cache;
        cache = kmem_cache_create(name, size, 0, SLAB_PANIC, NULL);
        return cache;
    }
    ```
    
- **页粒度管理内存，关注内存的局部性**
    
    在内核中，内存的管理通常是基于 **页**（Page）来进行的，操作系统通过 **分页机制** 来将内存划分为一个个固定大小的页（通常为4KB），从而进行内存的管理。分页机制不仅可以提高内存分配的效率，还能实现 **内存保护**，防止不同进程之间的内存干扰。
    
    内核关注 **空间局部性**，即在同一时间，程序对内存的访问通常会集中在一些相邻的内存区域，因此内核会尽量优化内存的页分配策略。例如，通过 **伙伴系统**（Buddy System）或 **SLAB 分配器** 来管理和回收内存，减少内存碎片并提高内存分配的速度。
    
    ```c
    void *kmalloc(size_t size, gfp_t flags)
    {
        struct page *page;
        size_t order;
    
        order = get_order(size);  // 获取内存的页数
        page = alloc_pages(flags, order);  // 分配指定页数的内存
        if (!page)
            return NULL;
    
        return page_address(page);  // 返回实际的内存地址
    }
    ```
    
    在这个代码中，kmalloc 函数通过 alloc_pages 按页分配内存，并通过 page_address 获取到实际的内存地址。
    
- **内存的分配与回收由内核控制，操作相对简单、直接**
    
    在内核中，内存的分配与回收是由内核来管理的。为了减少内存分配时的复杂度和开销，内核采用了简化的分配策略，比如通过 **kmalloc** 进行内存分配，内存回收则使用 **kfree** 函数。在内核内存管理中，回收内存通常不需要依赖于垃圾回收机制，而是通过明确的函数调用进行。
    
    内核空间的内存分配采用的策略是：通过 **伙伴系统**（Buddy System）进行物理内存块的管理，内核直接控制内存的分配与回收，因此操作简单、直接。
    

### **用户态**

用户态的内存管理更注重 **灵活性** 和 **适应性**。用户进程的内存管理需要考虑到不同应用程序的内存访问模式、生命周期管理、缓存机制等。由于 **用户空间** 的内存不受内核直接管理，用户程序通常通过 **动态分配** 和 **缓存策略** 来优化内存的使用效率，最大化性能。

**用户内存管理**的关键：

- **用户内存管理更加灵活，使用堆进行内存分配**
    
    在用户空间，内存管理通常更加 **灵活**，最常见的方式就是通过 **堆**（Heap）来分配内存。堆内存的分配非常灵活，可以在程序运行时动态地申请和释放内存。与内核态的内存分配不同，用户空间通过标准库函数（如 malloc）来管理堆上的内存，并且可以在程序运行过程中对内存进行管理。malloc 提供了灵活的内存管理策略，例如通过 **内存池** 和 **缓存机制** 来避免频繁的分配与释放操作，优化程序的性能。
    
    **相关代码（malloc 内存池）：**
    
    ```c
    void *malloc(size_t size)
    {
        void *ptr;
        ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);  // 使用 mmap 进行内存分配
        return ptr;
    }
    ```
    
    这里，malloc 通过 mmap 系统调用分配内存，mmap 可以提供更灵活的内存映射方式，同时允许分配大块内存。
    
- 使用如 malloc 等函数进行内存分配，通常会处理分配的生命周期和缓存等问题。
    
    在用户空间，内存的生命周期管理通常由开发者或库函数来处理。malloc 和 free 等函数会自动管理内存的生命周期，开发者可以通过使用缓存机制来提升程序的性能(CSAPP 中的cache lab即考验此点)。
    

## **内存分配的效率与策略**

### **内核态**

由于内核空间的内存分配需要考虑到实时性和简洁性，内核一般采用 **固定大小的内存块** 来管理内存，例如使用 kmalloc，并采用一些内存分配算法（如伙伴系统、SLAB 分配器）来降低分配开销。内核态内存管理的效率通常较高，但灵活性相对较低。

kmalloc 是内核空间的内存分配函数。其主要特点是提供快速分配小块内存，且内存对齐要求严格。在分配时会检查是否能够满足物理内存的连续性要求，若不满足会触发页面分配。通常情况下，kmalloc 会尝试高效地为内核分配连续的内存区域。

### **用户态**

用户空间的内存管理更侧重于 **灵活性**，通过如 malloc 这样的分配函数，用户可以按需分配内存。用户内存分配的性能往往与分配器的设计、使用模式等因素密切相关， malloc 通过内部的缓存池、合并空闲内存块等技术来提升分配效率。

malloc 是用户态的内存分配函数，背后通常由操作系统的动态链接库（如 **glibc**）来实现，采用的是 **堆管理** 和 **分配策略**，如使用 **双向链表** 管理空闲内存块（CSAPP 中的 malloc lab即采用不同管理策略）。

## **内存释放与回收**

### **内核态**

内核通过 kfree 函数来释放 kmalloc 分配的内存。内核会更严格地控制内存的回收，避免内存泄漏，并且使用相对简单的回收机制来确保内存的及时回收。

### **用户态**

用户态通过 free 来释放 malloc 分配的内存。内核空间和用户空间的回收机制不同，内核需要手动回收，而用户态一般依赖于 **垃圾回收** 或 **内存池管理** 来自动回收内存。