# Summary

[前言](./Intro.md)
[如何开始实验](./Getting-started.md)
[贡献指南](./Contribute.md)
[CHANGELOG](./Changes.md)
---

- [Lab0：拆炸弹](./Lab0.md)
  - [基本知识](./Lab0/instructions.md)
  - [二进制炸弹拆除](./Lab0/defuse.md)

---

- [Lab1: 机器启动](./Lab1.md)
  - [RTFSC(1)](./Lab1/RTFSC.md)
  - [内核启动](./Lab1/boot.md)
  - [页表映射](./Lab1/pte.md)

- [Lab2: 内存管理](./Lab2.md)
  - [物理内存管理](./Lab2/physical.md)
  - [页表管理](./Lab2/pte.md)
  - [缺页管理](./Lab2/page_fault.md)

- [Lab3: 进程管理](./Lab3.md)
  - [RTFSC(2)](./Lab3/RTFSC.md)
  - [线程管理](./Lab3/thread.md)
  - [异常管理](./Lab3/fault.md)
  - [系统调用](./Lab3/syscall.md)
  - [用户态程序编写](./Lab3/userland.md)

- [Lab4: 多核调度与IPC](./Lab4.md)
  - [多核支持](./Lab4/multicore.md)
  - [多核调度](./Lab4/scheduler.md)
  - [进程间通信(IPC)](./Lab4/IPC.md)
  - [实机运行与IPC性能优化](./Lab4/performance.md)

- [Lab5: 虚拟文件系统](./Lab5.md)
  - [Posix适配](./Lab5/posix.md)
  - [FSM](./Lab5/FSM.md)
  - [VFS(FS_Base)](./Lab5/base.md)
  - [BowerAccess](./Lab5/bower.md)

---

- [Detail: 源码详解](./Appendix/source-code.md)
  - [Lab0：拆炸弹](./Appendix/source-code/Lab0.md)
  - [Lab1: 机器启动](./Appendix/source-code/Lab1.md)
    - [源码解析](./Appendix/source-code/Lab1/code.md)
      - [内核启动](./Appendix/source-code/Lab1/booting.md)
      - [页表映射](./Appendix/source-code/Lab1/pte.md)
    - [教材阅读](./Appendix/source-code/Lab1/book.md)
    - [拓展与思考](./Appendix/source-code/Lab1/thoughts.md)
  - [Lab2: 内存管理](./Appendix/source-code/Lab2.md)
    - [源码解析](./Appendix/source-code/Lab2/code.md)
      - [物理内存管理](./Appendix/source-code/Lab2/physical.md)
      - [虚拟内存管理](./Appendix/source-code/Lab2/virtual.md)
    - [教材阅读](./Appendix/source-code/Lab2/book.md)
    - [拓展与思考](./Appendix/source-code/Lab2/expansion.md)
  - [Lab3: 进程管理](./Appendix/source-code/Lab3.md)
    - [源码解析](./Appendix/source-code/Lab3/code.md)
      - [进程/线程管理](./Appendix/source-code/Lab3/processthread.md)
      - [异常管理](./Appendix/source-code/Lab3/exception.md)
      - [系统调用](./Appendix/source-code/Lab3/sys.md)
    - [教材阅读](./Appendix/source-code/Lab3/book.md)
    - [拓展与思考](./Appendix/source-code/Lab3/expansion.md)
  - [Lab4: 多核调度与IPC](./Appendix/source-code/Lab4.md)
    - [源码解析](./Appendix/source-code/Lab4/code.md)
      - [多核支持](./Appendix/source-code/Lab4/multisupport.md)
      - [多核调度](./Appendix/source-code/Lab4/multisched.md)
      - [ipc通信](./Appendix/source-code/Lab4/ipc.md)
    - [教材阅读](./Appendix/source-code/Lab4/book.md)
    - [拓展与思考](./Appendix/source-code/Lab4/expansion.md)
  - [Lab5: 虚拟文件系统](./Appendix/source-code/Lab5.md)

---

- [附录](./Appendix.md)
  - [Bomb: 工具教程](./Appendix/toolchains.md)
    - [TL;DR Cheatsheet](./Appendix/toolchains/tldr.md)
    - [tmux](./Appendix/toolchains/tmux.md)
    - [gdb](./Appendix/toolchains/gdb.md)
      - [源码级调试 vs 汇编级调试](./Appendix/toolchains/gdb/comparison.md)
      - [使用简介与扩展阅读](./Appendix/toolchains/gdb/usage.md)
    - [objdump](./Appendix/toolchains/objdump.md)
    - [make](./Appendix/toolchains/make.md)
    - [qemu](./Appendix/toolchains/qemu.md)
      - [进程级模拟 vs 系统级模拟](./Appendix/toolchains/qemu/emulation.md)
      - [GDBServer](./Appendix/toolchains/qemu/usage.md)
  - [Kernel: ELF格式](./Appendix/elf.md)
  - [Kernel: Linker Script](./Appendix/linker.md)
  - [Kernel: 调试指北](./Appendix/kernel-debugging.md)
