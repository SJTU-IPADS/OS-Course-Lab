# Capability 机制

## 介绍

设计与实现简洁而易用的资源抽象能够为开发面向不同软硬件的系统服务提供很大便利，从而增强 ChCore 微内核操作系统的向新硬件、新场景的可扩展能力。

在资源管理方面，ChCore 微内核采用“Everything is an object”的设计理念，它将用户态能够进行操作的资源统一抽象成**内核对象**（kernel object）。这和UNIX操作系统中经典抽象“Everything is a file”类似，都是旨在提供简洁而统一的资源抽象。

ChCore微内核中共提供了7种类型的内核对象，分别是 cap 组对象（cap_group）、线程对象（thread）、物理内存对象（pmo）、地址空间对象（vmspace）、通信对象（connection 和 notification）、中断对象（irq）。每种内核对象定义了若干可以被用户态调用的操作方法，比如为线程对象设置优先级等调度信息、将一个物理内存对象映射到一个地址空间对象中等。

为了能够使用户态调用内核对象定义的方法，ChCore 微内核需要提供内核对象命名机制，即为一个内核对象提供在用户态相应的标识符。类似地，宏内核操作系统中的文件对象（file）在用户态的识别符是 fd，这就是一种命名机制。ChCore 采用 **capability** 作为内核对象在用户态的标识符。在 ChCore 微内核操作系统上运行的应用程序在内核态对应一个 cap 组对象，该对象中记录着该应用程序能够操作的全部内核对象。

![](./assets/capability.jpg)

如图所示，每个 cap 组可以被认为是存储内核对象指针的数组，当且仅当该数组中某一项指向某个内核对象，这个对象才能被该 cap 组（即对应的用户态进程）访问；作为用户态进程的系统服务和应用程序使用 capability 访问与之对应的内核对象，而 capability 在用户态看来就是一个整数，实际上是 cap 组的索引，操作系统内核通过 cap 组中存储的指针就可以找到相应的内核对象；当授权某个进程访问一个内核对象时，操作系统内核首先在这个进程对应的 cap 组中分配一个空闲的索引，然后把内核对象的指针填写到该索引位置，最后把索引值作为 capability 返回给用户态即可。用户态进程中的所有线程隶属于同一个 cap 组，而该 cap 组中非空闲的索引值即为该进程拥有的所有 capability，也就是该进程中所有线程能够操作的内核对象、即所有能够访问的资源。

通过 capability 的设计，每个进程拥有独立的内核对象命名空间。

每个进程获取 capability 的方式有三种。

1. 在被创建时由父进程赋予；
2. 在运行时向微内核申请获得；
3. 由其他进程授予。

不同的进程可以拥有相同内核对象的 capability，比如在上图中，进程-1 的 cap-3 和 进程-2 的 cap-5 就对应着同一个内核对象，这为进程间协同工作提供基础能力。比如说，若该内核对象是一个物理内存对象，则两个进程可以通过把它映射到各自的地址空间中从而建立共享内存；若该内核对象是一个通信对象，则两个进程可以通过调用其提供的方法进行交互。

不过，对于应用程序而言，ChCore 并不要求把 capability 抽象直接暴露给它们，而是可以通过系统库加以封装。

## Allocation

A typical allocation includes the following three steps:

1. `obj_alloc` allocates an object and returns a data area with user-defined length.
2. Init the object within the data area.
3. `cap_alloc` allocate a cap for the inited object.

Cap should be allocated at last which guarantees corresponding object is observable after initialized.

Here is an example of initializing a pmobject with error handling:

```c
	struct pmobject *pmo;
	int cap, ret;

	/* step 1: obj_alloc allocates an object with given type and length */
	pmo = obj_alloc(TYPE_PMO, sizeof(*pmo));
	if (!pmo) {
		ret = -ENOMEM;
		goto out_fail;
	}

	/* step 2: do whatever you like to init the object */
	pmo_init(pmo);

	/*
	 * step 3: cap_alloc allocates an cap on a cap_group
	 * for the object whith given rights
	 */
	cap = cap_alloc(current_cap_group, pmo, 0);
	if (cap < 0) {
		ret = cap;
		goto out_free_obj;
	}

	......
	/* failover example in the following code to free cap */
	if (failed)
		goto out_free_cap;
	......

out_free_cap_pmo:
	/* cap_free frees both the object and cap */
	cap_free(cap_group, stack_pmo_cap);
	/* set pmo to NULL to avoid double free in obj_free */
	pmo = NULL;
out_free_obj_pmo:
	/* obj_free only frees the object */
	obj_free(pmo);
out_failed:
	return ret;
```

## Search and Use

Object can be get and used by `obj_get` and `obj_put` must be invoked after use. This pair of get/put solves data race of object use and free (see [Destruction](#destruction) for explaination).

Here is an example of getting and using an cap:

```c
	struct pmobject *pmo;

	/* get the object by its cap and type */
	pmo = obj_get(current_cap_group, cap, TYPE_PMO);
	if (!pmo) {
		/* if cap does not map to any object or the type is not TYPE_PMO */
		r = -ECAPBILITY;
		goto out_fail;
	}

	/* do whatever you like to the object */
	......

	obj_put(pmo);
```

## Destruction

Every object contains a reference counter, which counts it is pointed by how many caps and get by `obj_get`. The counter is substracted when `cap_free` or `obj_put` is invoked. When the counter reaches 0, the object is freed.
