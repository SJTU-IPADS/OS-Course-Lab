#include "pthread_impl.h"
#include <chcore/syscall.h>

int __pthread_mutex_lock(pthread_mutex_t *m)
{
	int cur_prio = 0, r;
	bool set_ceil = false;

	if (m->ceil) {
		cur_prio = usys_get_prio(0);

		if (cur_prio < m->ceil) {
			set_ceil = true;
			usys_set_prio(0, m->ceil);
		}
	}

	if ((m->_m_type&15) == PTHREAD_MUTEX_NORMAL
	    && !a_cas(&m->_m_lock, 0, EBUSY))
		r = 0;
	else
		r = __pthread_mutex_timedlock(m, 0);

	if (set_ceil && !r)
		m->old_prio = cur_prio;
	return r;
}

weak_alias(__pthread_mutex_lock, pthread_mutex_lock);
