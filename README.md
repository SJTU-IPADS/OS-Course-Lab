# IPADS ChCore MicroKernel Repository

## Prerequisities(build with docker)

* docker

## Build

Generate build config files:

```shell
./chbuild defconfig raspi3 # raspi3 can be changed to other platforms (options: raspi3 raspi4)
```

Build according to `.config`:

```shell
./chbuild build
```

Clean and rebuild all:

```shell
./chbuild clean
./chbuild build
```

Full usage of `chbuild` script:

```shell
./chbuild --help
```

## Run in QEMU

```shell
./build/simulate.sh
```

## Coding Style

Basic Requirements:

1. Please use #pragma once (instead of #ifndef) in header files.
2. Put all header files in kernel/include directory and, especially,
   put the architecture-dependent ones in kernel/include/arch/xxx/arch/
   (e.g., kernel/include/arch/aarch64/arch/) directory.

## Debugging

* `TRACE_SYSCALL` (disabled by default) in `user/chcore-libc/src/chcore-port/syscall_dispatcher.c` controls whether to print all syscall invocations in libc.
    Not all syscalls are printed. Exceptions include:
    1. SYS_sched_yield (generated too frequently);
    2. SYS_writev, SYS_ioctl and SYS_read (used in printf);
    3. SYS_io_submit (used in printf?).

* `kernel/include/common/debug.h` defines the kernel debugging options which can ease the procedure of bug hunting.

---
LICENSE: Relicensed under Mulan PSL v2.
