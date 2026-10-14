# mini-cpu

你在六个实验中逐步构造一台运行在用户态的 x86-64 机器：先写指令解释器，再把它改成五级流水线，然后在这台机器上装载目标文件，最后依次加入页表与 TLB、高速缓存、系统调用与块设备。后一个实验可能会复用前一个实验的代码，因此请尽量确保前面的实验实现正确。

## 目录结构

| 路径 | 内容 |
| --- | --- |
| `src/labN/` | 你在各实验中要写的源文件与该实验的说明 `README.md`。源文件中是你要写的函数；实验一的源文件另有几个已经写出的示例 |
| `include/mcpu.h` | 全部接口：机器状态、各实验函数的声明与要求（以 `Lab N:` 开头的注释）、框架函数的声明、`todo` |
| `frame/` | 框架代码：主程序、装载器、物理内存、提交日志、反汇编器，以及后面的实验完成之前使用的替代实现 |
| `guest/` | 在模拟机器上运行的客户机程序（C 与汇编源码，`mini.h` 提供系统调用）；`guest/ollama/` 是第三讲的 `mini_ollama`，生成 `guest/mini-ollama` 与 `guest/mini-ollama-simd` |
| `tools/` | `mkmodel.py` 生成 `mini-ollama` 的小模型 |
| `tests/` | 评测脚本 `labN/run.sh`、参考结果、单个函数的检查程序 `unit.c`（生成 `mini-unit`）、生成客户机程序的 `lab1/*.py`、独立计算参考值的 `reference.py`、源文件检查 `srccheck.sh` 与 `symbols.txt`，见“评测” |

仓库中的实验目录只含到已发布的实验为止用到的文件，现在发布的是实验一；后面的实验发布时加入各自的文件，本文件随之更新（见“获取代码与更新”）。

## 六个实验

| 实验 | 内容 |
| --- | --- |
| 1 | 指令解释器 |
| 2 | 五级流水线的 CPU |
| 3 | 目标文件的装载 |
| 4 | 页表与 TLB |
| 5 | 高速缓存 |
| 6 | 系统调用与块设备 |

实验一的源文件是 `src/lab1/modrm.c`、`src/lab1/alu.c` 与 `src/lab1/interp.c`，内容有三项：ModRM、SIB 与位移的解析，解释器与反汇编器 `./mini-cpu --disasm 程序` 共用；整数运算的结果与标志位，包括加、减、逻辑运算、`inc`、`dec`、`neg`、移位、乘法与除法；指令解释器，即一个主循环，按操作码直接修改寄存器与内存。

页表、高速缓存与系统调用在后面的实验中由你实现，此前由 `frame/` 中的替代实现提供，因此实验一的解释器从一开始就能运行带页表访问与系统调用的程序。

`src/labN/` 中的源文件是你在该实验中要写的函数，每个函数的函数体是一段 `TODO` 注释加一个 `todo("名字")` 调用：`TODO` 注释列出实现的步骤，`todo`（`include/mcpu.h`）在 stderr 上打印 `名字 is not implemented yet` 并以状态 3 退出程序。`include/mcpu.h` 是完整的接口：你要写的每个函数都在以 `Lab N:` 开头的注释之下声明，注释写明参数、返回值与要求；框架只调用这些函数，源文件也只需要 `mcpu.h` 声明的内容。实验一的源文件中另有几个函数与 `case` 已经写出，作为示例，见 `src/lab1/README.md`；除此之外，给出的代码全部在 `frame/` 中。每个实验的函数、流程、变量含义与你要完成的内容见对应的 `src/labN/README.md`。

## 运行环境

你在 **x86-64 的 Linux** 上完成实验：

| 机器 | 做法 |
| --- | --- |
| x86-64 Linux | 直接运行 |
| x86-64 Windows | 在 WSL2（Ubuntu）中完成，命令与 Linux 完全相同 |
| arm64 macOS | 用 x86-64 的 Linux 容器（`docker run --platform linux/amd64`）或课程服务器 |

依赖 x86-64 Linux 的四处：客户机程序是 x86-64 的 ELF 文件；`Makefile` 用到 GNU ld 的 `--wrap`；`frame/loader.c` 用 `<elf.h>`；`make check` 把生成的客户机程序在本机运行一次，以它的输出作为期望值（`tests/lab1/*.py`）。macOS 上没有其中任何一项，因此不提供原生做法。

## 编译与运行

```bash
make              # 编译 mini-cpu、mini-unit 与客户机程序
make LAB=n        # 只编译第 1 到第 n 个实验
make check        # 评测第 1 到第 N 个实验，打印每一项的得分与总分
make handin       # 把 src/lab1 .. src/labN 打包为 labN.zip，用于提交
make clean
```

`LAB` 默认取 `src/` 下编号最大的 `labN`，编译时以 `-DLAB=N` 传给所有源文件，框架据此选择替代实现与命令行选项。

```bash
./mini-cpu guest/hello
./mini-cpu --stats guest/sum
./mini-cpu guest/mini-ollama "hello" 8 < guest/tiny.gguf    # 第三讲的 mini_ollama，模型从标准输入给出
./mini-cpu -h                 # 列出当前实验可用的选项
```

`guest/mini-ollama` 与 `guest/mini-ollama-simd` 也能读入第二讲实验生成的 `q4_0.gguf`，在模拟机器上生成完整的回答，模型文件的下载地址、命令与所需的时间见 [`src/lab1/README.md`](src/lab1/README.md) 的“第五部分”一节，这一部分不计分。

物理内存是当前目录下的文件 `dram.img`，程序运行期间加锁。你在同一目录下同时运行两个模拟器时，后启动的一个报错退出；这时用 `--ram 文件名` 给其中一个指定另一个文件。

## 评测

```bash
make check
```

`make check` 先编译，再依次运行 `tests/lab1/run.sh` 到 `tests/labN/run.sh`。每个实验满分 100 分，分为若干项，每项全部通过才得到该项的分数。你运行的测试与判定提交时运行的测试相同。随机参数的种子每次不同，正确的实现在任意种子下得分相同。

第一行是随机参数的种子（见“随机参数”），之后每项一行，`PASS` 或 `FAIL`、得分/满分、项目名。未通过的项目在下面列出期望的结果、实际的结果与程序在 stderr 上的前几行。实验一计分的部分有四个，每个部分的项目之前有一行以 `--` 开头的标题。你在实验一的第四部分中漏掉 `vpmulld` 时的输出：

```
seed 31274
== lab 1
-- part 1: modrm.c, 30 points
PASS  10/10  unit: modrm_decode, every form and every truncated length
PASS  10/10  disasm: the guests
PASS  10/10  disasm: random memory operands (SIB, displacement)
-- part 2: alu.c, 30 points
PASS  10/10  unit: alu.c, x86_szp to x86_neg
PASS  10/10  unit: alu.c, x86_shift
PASS  10/10  unit: alu.c, x86_mul and x86_div
-- part 3: interp.c, the integer instructions, 30 points
PASS   3/3   unit: cond, 16 conditions under 32 combinations of flags
PASS   3/3   hello, sum: output
PASS   3/3   matmul, heap, pattern, block: output
PASS   3/3   page faults: past the stack, store to the code
PASS   3/3   commit logs: hello, sum
PASS   3/3   commit logs: matmul, heap, pattern, block
PASS   3/3   flags: random operands, against this machine
PASS   3/3   conditions: jcc, setcc, cmovcc under all flags
PASS   3/3   random instructions: registers and memory, against this machine
PASS   3/3   mini-ollama: two small models, against this machine
-- part 4: interp.c, the vector instructions, 10 points
FAIL   0/2   simd: scalar and vector loops, n = 19 and 1000
        | expected: scalar = -28
        | simd   = -28
        | got:      scalar = -28
        | simd   = 
        | 
        | fault (illegal instruction) after 667 instructions:
        | rax  0000000000000005  rcx  0000000000000013  rdx  0000000000000005  rbx  0000000000000013
FAIL   0/2   tinytf: seed 1 and two random seeds
        | expected: prompt = 14 9 2 11
        | tokens = 1 8 15 9 8 11 2 7
        | logits = 7685
        | got:      
        | 
        | fault (illegal instruction) after 69488 instructions:
        | rax  0000000000000000  rcx  0000000000000010  rdx  0000000000000000  rbx  0000000000000000
        | (guest/tinytf 1 8)
FAIL   0/2   simd, tinytf: commit logs (vector registers)
        | expected: 2635356908 13470
        | got:      4261807836 11045
        | (cksum of the commit log of guest/simd: checksum and size in bytes)
FAIL   0/2   random vector instructions: registers and memory
        | (tests/lab1/vecops.py 16351 60)
        | 
        | fault (illegal instruction) after 24 instructions:
        | rax  0000000000000000  rcx  0000000000000000  rdx  0000000000000000  rbx  0000000000000000
        | the output of mini-cpu ends before the record of block 0
FAIL   0/2   mini-ollama-simd: dot_q4 on ymm registers
        | (python3 tools/mkmodel.py 7370 > m.gguf; mini-cpu  guest/mini-ollama-simd "the cat in the hat" 10 < m.gguf)
        | expected: jjjjjjjjjj
        | got:      
        | < prompt tokens:    19
        | < generated tokens: 10
        | < logits checksum:  2903821179
        | > 
lab 1 score 90/100
total score 90/100
```

尚未完成的函数使整项不通过，`got:` 之后是 `名字 is not implemented yet`。

以 `unit:` 开头的项目运行 `mini-unit`（`tests/unit.c`，`make` 与 `mini-cpu` 一起生成）：它不运行客户机程序，直接以固定的参数调用一个文件中的函数，把返回值与留在内存中的结果与 `mcpu.h` 的要求比较，因此一个函数在依赖它的函数完成之前你就可以检查。实验一的组是 `modrm`、`alu`、`shift`、`muldiv` 与 `cond`：

```bash
./mini-unit alu
```

每个失败的检查打印一行，说明参数、期望值与实际值（每组最多 12 行），最后一行是 `组名: N checks, M failed`，没有失败时退出状态为 0。被检查的函数使程序收到信号（除以零、访问越界、`abort`）时，打印一行 `组名: ended by signal 编号 (名称)`，已知是哪一个调用时一并写出，退出状态为 1。`--ram 文件` 指定物理内存文件（默认 `dram.img`）。

你只运行一个实验的测试时，在本目录中运行：

```bash
sh tests/lab1/run.sh
```

你单独运行 `run.sh` 时，它不做源文件检查，也不打印种子；检查源文件：`sh tests/srccheck.sh src 1`。

### 测试检查的内容

| 类别 | 检查方式 |
| --- | --- |
| 单个函数 | `mini-unit` 直接调用一个文件中的函数，没有失败的检查（见上文） |
| 程序输出 | 客户机程序的 stdout 与期望值逐字相同；`sum`、`matmul`、`simd` 的期望值由 `python3 tests/reference.py` 独立计算 |
| 生成的程序 | `tests/lab1/flagops.py`、`condops.py`、`randops.py` 生成的程序在本机 CPU 与模拟器上输出相同；`vecops.py` 的期望输出在 Python 中计算；`memops.py` 的反汇编与 `objdump -d` 相同 |
| 缺页 | `guest/deep`（越过栈）与 `guest/readonly`（写代码页）在固定的指令之后、固定的地址缺页 |
| 提交日志 | `--commit` 写出的日志的 `cksum`（校验和与字节数）与 `tests/lab1/expected.txt` 相同 |
| 随机参数 | 见下一节 |

### 随机参数

一部分项目使用随机参数，参数由一个种子决定。`make check` 的第一行打印种子，你用同一个种子可以重复一次评测：

```bash
SEED=31274 make check
```

实验一中随机的内容：

| 随机的内容 | 期望值的来源 |
| --- | --- |
| `guest/tinytf` 的种子与生成的 token 数 | `python3 tests/reference.py tinytf 种子 个数` |
| `tests/lab1/memops.py` 生成的内存操作数 | 汇编后 `objdump -d` 的输出 |
| `flagops.py` 生成的操作数与输入标志，`randops.py` 的随机整数指令 | 同一程序在本机 CPU 上运行的输出 |
| `vecops.py` 的随机向量指令 | `vecops.py --want` 在 Python 中计算的输出 |
| `tools/mkmodel.py` 生成的小模型的种子，`mini-ollama` 的提示词与 token 数 | 同一程序在本机 CPU 上运行的输出 |

`guest/tinytf [种子 [个数]]` 是一个整数运算的小型 Transformer（一层因果自注意力与一层 MLP），全部矩阵乘法调用 `guest/linear.s` 的 `linear_simd`，按贪心方式生成 1 到 12 个 token：

```
$ ./mini-cpu guest/tinytf 7 5
prompt = 13 4 8 4
tokens = 4 6 10 11 7
logits = 1124
```

### 源文件检查

`make check` 在运行测试之前执行 `tests/srccheck.sh`，用 `make` 的编译选项逐个编译 `src/lab1` .. `src/labN` 中的 `.c` 文件，检查目标文件：

| 检查 | 规则 |
| --- | --- |
| 使用的函数与变量 | 你在每个实验中只能使用 `tests/symbols.txt` 中该实验一行列出的框架接口、`libc` 一行列出的库函数，以及该实验自己的文件中定义的名字 |
| 定义的名字 | 不能与 libc、libm 中的名字相同（这样的定义会替换整个程序中的库函数），不能以 `__` 开头 |
| 节 | 只能有代码、数据与调试信息的节，代码只能在 `.text` 节；不能有 `.init_array` 等在 `main` 之前执行的节 |
| 指令 | 代码中不能有 `syscall`、`sysenter` 与 `int $0x80` |

例如实验一的源文件不能直接调用 `mem_read` 与 `dram_ptr`，读写客户机的内存要经过 `load`、`store`、`rm_get`、`rm_set` 等辅助函数。检查不通过时打印每一个问题，不运行测试，总分为 0。

测试运行期间，`tests/` 与 `tools/` 中的文件被设为只读，测试结束后比较这些文件的校验和；文件被改动时总分为 0。

每次运行的时间上限为 120 秒，超过时该项不通过；环境变量 `LIMIT` 可以改变这个上限，评测时使用默认值。被测程序写出的每个文件不超过 256 MiB（`tests/lib.sh` 中的 `FSIZE`，以 `ulimit -f` 设置），输出不停止的程序由系统终止。测试运行时物理内存使用临时文件，不影响当前目录的 `dram.img`。每个实验的测试项与分值见 `src/labN/README.md` 的“评测”一节。

## 获取代码与更新

整个实验放在 GitHub 仓库 `SJTU-IPADS/OS-Course-Lab` 的 `ICS-Labs/MiniCPU` 目录下：

<https://github.com/SJTU-IPADS/OS-Course-Lab/tree/main/ICS-Labs/MiniCPU>

```bash
git clone https://github.com/SJTU-IPADS/OS-Course-Lab.git
cd OS-Course-Lab/ICS-Labs/MiniCPU
```

后面的实验与框架的修改都在这个目录中更新，不另外发布压缩包。有更新时，你在仓库目录中重新拉取代码：

```bash
git pull
make check
```

发布实验 N 的更新加入 `src/labN/`，并更新 `Makefile`、`README.md`、`frame/`、`include/`、`guest/`、`tests/` 等框架文件；它不改动 `src/lab1` .. `src/lab(N-1)` 中的源文件，因此你已经写好的代码保持不变。拉取之后 `LAB` 自动变为新的编号，`make` 重新编译。

## 提交

```bash
make handin
```

生成 `labN.zip`，其中只有 `src/lab1` 到 `src/labN` 下的 `.c` 与 `.h` 文件。请把生成的压缩包提交到 Canvas 平台。测评时把这个压缩包解压到一份新的框架中，再运行 `make check`，因此：

- 你对 `src/` 以外的修改（包括 `frame/`、`include/mcpu.h`、`Makefile`）不会生效，你的代码只依赖发布的框架；
- 你新增的辅助文件放在 `src/labN/` 中，`Makefile` 会编译其中所有的 `.c` 文件；
- 你提交前先运行 `make check`，并用 `unzip -l labN.zip` 确认文件齐全；判定时在新的框架中运行同样的测试，随机参数的种子不同，因此你的实现对任意参数都要正确。
