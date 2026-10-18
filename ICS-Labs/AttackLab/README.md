# attack-elf

本实验配合课程第三讲「程序的机器级表示」。你会得到三个可执行文件 `attack1`、`attack2`、`attack3`，没有源码。每个程序都是改编自 mini-ollama 的一个聊天程序：它加载模型 `q4_0.gguf`，读入你的一行提示词并回复，直到输入 Ctrl+D 结束。在终端里运行时，提示行用 isocline 绘制，你可以像平常一样输入提示词。而你的攻击提示词需要使用管道传递进去，走的是纯文本读取。两种情形下读入提示的函数都把输入放进一个固定大小的栈缓冲区，且没有长度限制，所以一行足够长的提示会覆盖缓冲区上方的栈内容。

本实验中，你需要构造一行输入，让控制流进入到程序里的 `report_pass` 函数——一个没有任何调用点的过关函数，并传递给它正确的参数。三个程序是同一个聊天程序，通过不同的保护选项编译：

| 程序      | 保护                 | 过关方式                                                                                      |
| --------- | -------------------- | --------------------------------------------------------------------------------------------- |
| `attack1` | 无金丝雀，栈可执行   | 覆盖返回地址，让控制流落到缓冲区里自己写的代码上，由这段代码完成调用                          |
| `attack2` | 有栈金丝雀，栈可执行 | 金丝雀挡在返回地址前：本机这份的值固定为 `0x1122334455667700`，服务器上同样固定但是是另一个值 |
| `attack3` | 无金丝雀，栈不可执行 | 栈上不能放可执行代码，只能利用程序里已有的指令片段拼凑出参数与调用                            |

## 领取你自己的程序

每个学号得到的程序不同，解也不同：程序里的常数与缓冲区大小由你学号的 SHA-256 散列算出。用你的学号领取：

```bash
make attack ID=你的学号
```

命令一览：

| 命令                                          | 做什么                                            |
| --------------------------------------------- | ------------------------------------------------- |
| `make attack ID=…`                            | 领到你学号的三个程序                              |
| `make local1` / `local2` / `local3`           | 把 `answers/attackN.txt` 喂给本机的程序，打印判语 |
| `make local`                                  | 三段都判一遍                                      |
| `make remote1` / `remote2` / `remote3` `ID=…` | 把 `answers/attackN.txt` 发到服务器复核记分       |
| `make remote` `ID=…`                          | 三段都发到服务器进行打分                          |
| `make score`                                  | 记分榜                                            |

## 程序的用法

```
usage: attackN [-q|--quiet] [-h|--help] [-s|--score] [MODEL.gguf]
```

- `-h`、`--help` 打印这一行用法并退出。
- `-q`、`--quiet` 什么都不显示：没有横幅、提示框和回答文字。过关时的那两行仍然打印，那是一次的结果，退出状态也照旧。
- `-s`、`--score` 从课程服务器取回记分榜，按你的学号显示，然后退出。
- `tools/unhex < answers/attack1.txt` 把文件里的十六进制转成原始字节，它的源码就在 `tools/unhex.c`，由本地的 `make` 编出来。
- `tools/asmhex` 反过来：从标准输入读 AT&T 汇编，把汇编出来的机器码按 `unhex` 认识的格式打到标准输出，它调用本机的汇编器得到指令字节。
- 最后的 `MODEL.gguf` 是要加载的模型文件，与设置环境变量 `ATTACK_MODEL` 等价。

如果某一段通过了，程序打印 `phase N passed` 然后退出——它自己并不上报。你需要主动通过 `make remoteN` 把结果记录到服务器上。

程序要加载同一目录下的模型文件 `q4_0.gguf`（约 929 MiB）。你可能在第二讲 `nano-quant` 实验里已经生成过它，复制或
软链接过来即可。如果本地没有这个文件，也可以从云盘上下载：

```bash
cp path/to/q4_0.gguf .            # or: ln -sfn path/to/q4_0.gguf q4_0.gguf
```

云盘链接：<https://pan.sjtu.edu.cn/web/share/a853ce437a176a0064df9a5af1942ecf>

模型文件放好之后，先尝试运行一下：

```bash
./attack1
```

程序画出一个横幅和提示框。回车发送，Ctrl-D 退出，每一轮对话互相独立。用真实模型时一次回答可能需要要跑几十秒，溢出与过关跟模型内容无关，三个阶段都不必等它答完。

## 阅读机器代码

三个程序里都有一个过关函数 `report_pass(unsigned long phase, unsigned long key)`：两个参数，第一个是阶段号，第二个是口令，两个都对就打印 `phase N passed` 然后退出。它在程序里，但没有任何地方调用它。过关要做的就是**把这两个数放到参数寄存器中，再让控制流转移到函数开头**。读入提示的函数是 `chat_turn`，缓冲区就在它的栈帧里。

```bash
objdump -d --disassemble=chat_turn   attack1
objdump -d --disassemble=report_pass attack1
```

程序是静态链接的，`objdump -d attack1` 很长，用 `--disassemble=函数名` 只看一个函数。程序不是位置无关的，所以 `objdump` 显示的地址就是运行时的地址，`report_pass` 每次运行都在同一处。

## 阶段 2：金丝雀是固定值

三个程序里只有 `attack2` 带栈金丝雀：`chat_turn` 把它放在 `%rbp-0x8`。本实验中金丝雀行为不与 libc 完全一致，libc 中金丝雀的值会在每次运行时随机生成，但我们为了实验方便，程序会在 `main` 之前把确定的八字节值写到金丝雀的位置。而服务器和本地二进制 `attack2` 中使用的值不同：

| 哪份               | 金丝雀                                    | 怎么知道                                                   |
| ------------------ | ----------------------------------------- | ---------------------------------------------------------- |
| 你本地这份         | 固定 `0x1122334455667700`，每个学号都一样 | 它是编译进去的常数，`objdump -s -j .data attack2` 里能看到 |
| 服务器运行的那一份 | 服务器从你学号生成的值，不同学号值不同    | 通过一字节一字节测出来                                     |

两份程序除此之外一模一样：指令、地址、缓冲区距离都不变，所以本机找到的 gadget 和偏移，服务器那份照用。

服务器的金丝雀值只能一字节一字节地试，本机这个也照样能试：尝试把一行提示写成「填充到金丝雀之前 + 已经解出的字节 + 一个候选字节」，候选对了，这一行会跑完（`exit 0`）；错了，金丝雀的检查先失败（`signal 6`）。一个字节 256 个候选，只有一个能够让这一行跑完，8 轮就有了全部 8 字节。

我们保证金丝雀的字节里不会出现 `0a`（对应回车，这可能会中断输入）。另外，为了方便，读字符的函数 `Gets` 只在没读满缓冲区时补结尾的 NUL，溢出的一行原样留在栈上，所以一行正好停在候选字节处，这样不会让多补的 NUL 覆盖金丝雀字节导致失败。

这里提供 `Gets` 函数的源码供解题参考：

```c
static char *Gets(char *buf, size_t size)
{
    int c;
    char *p = buf;

    /* At a terminal, read the line with isocline. This way, the NUL and non-
       newline bytes an exploit needs cannot be typed, so an attack should
       arrive through a pipe and takes the plain read below. */
    if (ui_interactive() && !ui_is_quiet()) {
        char *line = ui_readline();

        if (line == NULL)
            return NULL;
        strcpy(buf, line);
        ic_free(line);
        return buf;
    }
    if (isatty(STDIN_FILENO) && !ui_is_quiet()) {
        fputs("> ", stderr);
        fflush(stderr);
    }

    /* -q reads the line the plain way too: nothing is drawn, and what the
       terminal echoes of the typing is the terminal's doing. */
    while ((c = getchar()) != EOF && c != '\n') {
        *p++ = (char)c;
    }
    if (c == EOF && p == buf)
        return NULL;

    if ((size_t)(p - buf) < size)
        *p = '\0';

    return buf;
}
```

## 提交与记分榜

把答案写进 `answers/attackN.txt` 之后：

```bash
make remote1 ID=你的学号      # 阶段 1，改成 2、3 就是另外两阶段
make remote  ID=你的学号      # 三阶段一起提交
```

`tools/submit.py` 读目录、调用 `unhex` 转成字节、发送到服务器。服务器用你的程序复核，回你一句 `phase N: pass (exit 0); you have cleared K/3`——是否通过由 `pass/fail` 标识，而程序的退出码由 `exit/signal` 标识，后者在 `attack2` 阶段尝试金丝雀值时被用到。

查看记分榜：

```bash
make score          # or: ./attack1 -s
```

榜上你自己那一行标出完整学号与 `Y`，其他人只显示匿名代号。
