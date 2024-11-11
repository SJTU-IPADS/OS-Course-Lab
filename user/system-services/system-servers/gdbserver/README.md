This project creates a tiny debugger based on `ptrace`.
It implements the [GDB Remote Serial Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html), works like a real `gdbserver` and can be connected by `gdb` client.

## Features
* No extra dependencies. Only repies on `glibc`
* Support basic debugging functions and some advance features, such as [Host I/O](https://sourceware.org/gdb/onlinedocs/gdb/Host-I_002fO-Packets.html) and debugging multithreaded programs
* Support multiple architectures (i386, x86_64, ARM and PowerPC)

## Usage
You can compile it by
```sh
$ make
```
or even
```sh
$ gcc -std=gnu99 *.c -o gdbserver
```

And run it by
```sh
$ ./gdbserver 127.0.0.1:1234 a.out
```
or you can attach an existed process
```sh
$ ./gdbserver --attach 127.0.0.1:1234 23456
```

Then you can run normal `gdb` (Only GDB 8.x is tested) and connect it by
```
target remote 127.0.0.1:1234
```
