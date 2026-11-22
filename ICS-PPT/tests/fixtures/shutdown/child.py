"""A process for tests/test_watch_shutdown.py, which says it lives by a lock.

child.py PATH [ignore] [group]: write its pid to PATH and hold a lock on that
file until it dies. The lock cannot be inherited or outlive it, so the pid is
this process's for exactly as long as the lock is held. ``ignore`` ignores the
signals that ask, ``group`` makes a process group of its own, as GNU timeout
does.
"""

import fcntl
import os
import signal
import sys
import time

path, *how = sys.argv[1:]
if "ignore" in how:
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(signum, signal.SIG_IGN)
if "group" in how:
    os.setpgid(0, 0)
with open(path, "a") as file:
    fcntl.flock(file, fcntl.LOCK_EX)
    file.write(f"{os.getpid()}\n")
    file.flush()
    time.sleep(60)  # and no longer, should the test itself be killed
