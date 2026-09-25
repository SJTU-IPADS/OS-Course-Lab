# TL;DR Cheatsheet

> [!INFO]
> <参数> 代表需要被替换的参数，请将其替换为你需要的实际值（包括左右两侧尖括号）。

---

<!-- toc -->

## tmux

- 创建新会话： tmux new -s <会话名>
- 进入会话： tmux attach -t <会话名>
- 临时退出会话（会话中程序保持在后台继续运行）： Ctrl-b d
- 关闭会话及其中所有程序： tmux kill-session -t <会话名>
- 水平分屏： Ctrl-b "
- 垂直分屏： Ctrl-b %

## gdb

- 不输入任何命令，直接按下回车键：重复上一条命令
- break *address : 在地址 address 处打断点
- break <函数名> : 在函数入口处打断点
- continue : 触发断点后恢复执行
- info breakpoints : 列出所有断点
- delete <NUM> : 删除编号为 NUM 的断点
- stepi : 触发断点后单步执行一条指令
- print <expr> : 打印表达式 <expr> 的求值结果，可以使用部分C语法，例如 `print*(int*) 0x1234` 可将地址 0x1234 开始存储的4个字节按32位有符号整数解释输出
- print/x <expr> : 以16进制打印 <expr> 的求值结果
- lay asm : 使用TUI汇编视图
- lay src : 使用TUI源代码视图（要求被调试可执行文件包含调试信息，且本地具有相应源代
- 码文件）
- tui disable : 退出TUI

## objdump操作

- objdump -dS <可执行文件> > <输出文件> : 反汇编可执行文件中的可执行section（不含数据
- sections），并保存到输出文件中。在可能的情况下（如有调试信息和源文件），还会输出汇
- 编指令所对应的源代码。
- objdump -dsS <可执行文件> > <输出文件> : 将可执行文件中的所有sections的内容全部导
- 出，但仍然只反汇编可执行sections，且在可能情况下输出源代码
