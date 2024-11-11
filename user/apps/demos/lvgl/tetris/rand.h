#ifndef __RAND_H__
#define __RAND_H__
#include <time.h>

static unsigned long long state[1] = {0};

unsigned long long xorshift64star() {
	if (state[0] == 0) {
		struct timespec t;
		clock_gettime(0, &t);
		state[0] = t.tv_sec + t.tv_nsec;
	}
	unsigned long long x = state[0];	/* The state must be seeded with a nonzero value. */
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c
	state[0] = x;
	return x * 0x2545F4914F6CDD1D;
}
#endif