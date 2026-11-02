/* Time one load from a working set of a given size.

   The working set is a ring of 64-byte nodes in random order. Each node holds
   the address of the next one, so every load needs the result of the previous
   load and the loads cannot overlap.

     gcc -O2 latency.c -o latency && ./latency */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/mman.h>

#define NODE 64                       /* one node per 64-byte cache line */

typedef struct node { struct node *next; char pad[NODE - sizeof(void *)]; } node;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static double ns_per_load(size_t bytes) {
    size_t count = bytes / NODE;
    node *ring = aligned_alloc(1 << 21, (bytes + (1 << 21) - 1) & ~(size_t)((1 << 21) - 1));
    madvise(ring, bytes, MADV_HUGEPAGE);
    size_t *order = malloc(count * sizeof *order);
    for (size_t i = 0; i < count; i++) order[i] = i;
    srand(1);
    for (size_t i = count - 1; i > 0; i--) {         /* one cycle through all nodes */
        size_t j = rand() % i;
        size_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }
    for (size_t i = 0; i < count; i++)
        ring[order[i]].next = &ring[order[(i + 1) % count]];
    free(order);

    long loads = 1L << 23;
    double best = 1e30;
    for (int rep = 0; rep < 3; rep++) {
        node *p = &ring[0];
        double start = now();
        for (long i = 0; i < loads; i++) p = p->next;
        double ns = (now() - start) / loads;
        if (p == NULL) puts("");                      /* keep the loop alive */
        if (ns < best) best = ns;
    }
    free(ring);
    return best;
}

int main(void) {
    size_t sizes[] = {16 << 10, 256 << 10, 2 << 20, 64 << 20};
    const char *names[] = {"16 KB", "256 KB", "2 MB", "64 MB"};
    printf("%10s  %12s\n", "data", "ns per load");
    for (int i = 0; i < 4; i++)
        printf("%10s  %12.1f\n", names[i], ns_per_load(sizes[i]));
    return 0;
}
