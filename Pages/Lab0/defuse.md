# 二进制炸弹拆除

我们在实验中提供了一个二进制炸弹程序bomb以及它的部分源码bomb.c。在 bomb.c 中，你可以看到一共有 6 个 phase。对每个 phase，bomb程序将从标准中输入中读取一行用户输入作为这一阶段的拆弹密码。若这一密码错误，炸弹程序将异常退出。你的任务是通过 GDB 以及阅读汇编代码，判断怎样的输入可以使得炸弹程序正常通过每个 phase。以下是一次失败尝试的例子：

```console
[user@localhost lab0] $ make qemu
qemu -aarch64 bomb
Type in your defuse password:
1234
BOOM !!!

```

> [!TIP]
> 你需要学习gdb、objdump的使用来查看炸弹程序对应的汇编，并通过断点等方法来查看炸弹运行时的状态（寄存器、内存的值等）。以下是使用gdb来查看炸弹运行状态的例子。在这个例子中，我们在main函数的开头打了一个断点，通过continue让程序运行直至遇到我们设置的断点，使用info查看了寄存器中的值，最终通过x查看了x0寄存器中的地址指向的字符串的内容。以下是输入与输出。

```console
add symbol table from file "bomb"
(y or n) y
Reading symbols from bomb ...
(gdb) break main
Breakpoint 1 at 0x4006a4
(gdb) continue
Continuing.
Breakpoint 1, 0x00000000004006a4 in main ()
(gdb) disassemble
Dump of assembler code for function main:
0x0000000000400694 <+0>:stp0x0000000000400698 <+4>:mov
x29 , x30 , [sp , # -16]!
x29 , sp
0x000000000040069c <+8>:adrpx0 , 0x464000 <free_mem +64>
0x00000000004006a0 <+12>:addx0 , x0 , #0x778
=> 0x00000000004006a4 <+16>:bl0x413b20 <puts >
0x00000000004006a8 <+20>:bl0x400b10 <read_line >
0x00000000004006ac <+24>:bl0x400734 <phase_0 >
0x00000000004006b0 <+28>:bl0x400708 <phase_defused >
0x00000000004006b4 <+32>:bl0x400b10 <read_line >
0x00000000004006b8 <+36>:bl0x400760 <phase_1 >
0x00000000004006bc <+40>:bl0x400708 <phase_defused >
0x00000000004006c0 <+44>:bl0x400b10 <read_line >
0x00000000004006c4 <+48>:bl0x400788 <phase_2 >
0x00000000004006c8 <+52>:bl0x400708 <phase_defused >
0x00000000004006cc <+56>:bl0x400b10 <read_line >
0x00000000004006d0 <+60>:bl0x400800 <phase_3 >
0x00000000004006d4 <+64>:bl0x400708 <phase_defused >
0x00000000004006d8 <+68>:bl0x400b10 <read_line >
0x00000000004006dc <+72>:bl0x4009e4 <phase_4 >
0x00000000004006e0 <+76>:bl0x400708 <phase_defused >
0x00000000004006e4 <+80>:bl0x400b10 <read_line >
0x00000000004006e8 <+84>:bl0x400ac0 <phase_5 >
0x00000000004006ec <+88>:bl0x400708 <phase_defused >
0x00000000004006f0 <+92>:adrpx0 , 0x464000 <free_mem +64>
0x00000000004006f4 <+96>:addx0 , x0 , #0x798
0x00000000004006f8 <+100>:bl0x413b20 <puts >
0x00000000004006fc <+104>:movw0 , #0x0
0x0000000000400700 <+108>:ldpx29 , x30 , [sp], #16
0x0000000000400704 <+112>:ret
// #0
End of assembler dump.
(gdb) info registers x0
x0
0x464778
4605816
(gdb) x /s 0x464778
0x464778:
"Type in your defuse password!"

```

在破解后续阶段时，为了避免每次都需要输入先前阶段的拆弹密码，你可以通过重定向的方式来让炸弹程序读取文件中的密码：

```console
[user@localhost lab0] $ make qemu < ans.txt
qemu -aarch64 bomb
Type in your defuse password:
5 phases to go
4 phases to go
3 phases to go
2 phases to go
1 phases to go
0 phases to go
Congrats! You have defused all phases!

```
