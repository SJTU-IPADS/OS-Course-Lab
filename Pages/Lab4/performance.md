# 实机运行与IPC性能优化

<!-- toc -->

在本部分，你需要对IPC的性能进行优化。为此，你首先需要在树莓派3B实机上运行ChCore。

> [!CODING] 练习题 8
> 请在树莓派3B上运行ChCore，并确保此前实现的所有功能都能正确运行。

在ChCore启动并通过测试后，在命令行运行
```shell
$ ./test_ipc_perf.bin
```

你会得到如下输出结果
```shell
[TEST] test ipc with 32 threads, time: xxx cycles
[TEST] test ipc with send cap, loop: 100, time: xxx cycles
[TEST] test ipc with send cap and return cap, loop: 100, time: xxx cycles
[TEST] Test IPC Perf finished!
```

> [!CODING] 练习题 9
> 尝试优化在第三部分实现的IPC的性能，降低test_ipc_perf.bin的三个测试所消耗的cycle数

IPC性能测试程序的测试用例包括：
1. 创建多个线程发起IPC请求（不传递cap），Server收到IPC后直接返回。记录从创建线程到所有线程运行结束的时间。
2. Client创建多个PMO对象，并发起IPC请求（传递PMO）；Server收到IPC后读取PMO，并依据读出的值算出结果，将结果写回随IPC传递的PMO中并返回；Client在IPC返回后读取PMO中的结果。将上述过程循环多次并记录运行时间。
3. Client创建多个PMO对象，并发起IPC请求（传递PMO）；Server收到IPC后读取PMO，并依据读出的值算出结果，然后创建新的PMO对象，将结果写入新创建的PMO中，并通过`ipc_return_with_cap`返回；Client在IPC返回后读取返回的PMO中的结果。将上述过程循环多次并记录运行时间。

在测试能够顺利通过的前提下，你可以修改任意代码。（测试程序所调用的函数位于 `user/chcore-libc/libchcore/porting/overrides/src/chcore-port/ipc.c`）


> [!SUCCESS]
> 以上为Lab4 的所有内容