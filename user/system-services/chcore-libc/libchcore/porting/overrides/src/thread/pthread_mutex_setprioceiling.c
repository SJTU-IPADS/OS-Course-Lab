#include "pthread_impl.h"

int pthread_mutex_setprioceiling(pthread_mutex_t *restrict m, int ceiling, int *restrict old)
{
	if (old)
		*old = m->ceil;
	if (ceiling < MIN_PRIO || ceiling > MAX_PRIO)
		return -EINVAL;
	m->ceil = ceiling;
	return 0;
}
