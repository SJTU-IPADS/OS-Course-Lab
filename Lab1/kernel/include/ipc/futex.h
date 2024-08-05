#ifndef IPC_FUTEX_H
#define IPC_FUTEX_H

#include <common/types.h>
#include <common/hashtable.h>
#include <ipc/notification.h>

#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9

#define FUTEX_PRIVATE 128

#define FUTEX_CLOCK_REALTIME 256


struct futex_entry {
        struct notification *notific;
        int *uaddr;
        int waiter_count;
        struct hlist_node hash_node;
};

struct cap_group;
void futex_deinit(struct cap_group *cap_group);
void futex_init(struct cap_group *cap_group);

/* Syscalls */
int sys_futex_wait(int *uaddr, int futex_op, int val, struct timespec *timeout);
int sys_futex_wake(int *uaddr, int futex_op, int val);
int sys_futex_requeue(int *uaddr, int *uaddr2, int nr_wake, int nr_requeue);
int sys_futex(int *uaddr, int futex_op, int val, struct timespec *timeout, int *uaddr2, int val3);

#endif /* IPC_FUTEX_H */
