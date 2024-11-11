#include <chcore/syscall.h>

__thread int lock_level = 0;

/* the default return value of intLock, set by intLockLevelSet */
#define DEFAULT_LOCK_LEVEL	0xf

int intLock(void)
{
	if (lock_level == 0)
		usys_disable_irq();
	lock_level++;
	return DEFAULT_LOCK_LEVEL;
}

int intUnlock(int oldSR)
{
	lock_level--;
	if (lock_level == 0)
		usys_enable_irq();

	return 0;
}
