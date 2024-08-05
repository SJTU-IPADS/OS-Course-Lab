# Multicore synchronization in chcore

## Lock List

**Locks should be required from higher level to lower level.**

```
                                      big_kernel_lock (0x1000)
              /
sched_lock (not used) (0x100)
              |
    ready_queue_lock (0x10)
```



| Lock             | Description         | Level  | File                 |
| ---------------- | ------------------- | ------ | -------------------- |
| big_kernel_lock  | Big Kernel Lock     | 0x1000 | main.c               |

sched:
| ready_queue_lock | Protect Ready Queue | 0x10   | kernel/sched/sched.c |
|                  |                     |        |                      |

mm:
| buddy_lock       | Protect physical page allocation | 0x10  | kernel/mm/buddy.c     |
| slabs_lock(s)    | Protect each slab allocator      | 0x20  | kernel/mm/slab.c      |
| vmspace_lock(s)  | Protect each vmspace mapping     | 0x100 | kernel/mm/vmregion.c  |
| pgtbl_lock(s)    | Protect each page table          | 0x100 | kernel/mm/vmregion.c  | 
							| kernel/mm/pgfault_handler.c |
| heap_lock        | Protect the heap of each process (cap_group) |
