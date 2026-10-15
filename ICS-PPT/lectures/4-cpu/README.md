# ICS 第四讲 · 指令如何在 CPU 中重叠执行

接第 3 讲的 `dot_product` 循环体：编译器排出的那些 x86-64 指令，CPU 并不是一条做完再做下一条。这一讲说明重叠能快多少、为什么会停，以及为什么流水线做满也解释不了第 3 讲测到的带宽墙。

中文是基线（写在 Python 里）。例子是第 3 讲 `gcc -Og -fcf-protection=none` 编译出的循环体，不另设一套指令集。

## 结构

封面之后四段，每段前有一页过渡。

| 节 | 页 | 落点 |
| --- | --- | --- |
| 第 3 讲留下的问题 | `question`、`loop-body` | 峰值填不满，因为每个权重都要从内存读一次；问题是这些指令是一条做完再做下一条，还是几条同时在做。循环体仍是 `movslq`、`movl`、`imull`、`addl`、`addl $1` |
| 一条指令的五个阶段 | `stages`、`stages-fig` | 用 `movl (%rsi,%r8,4), %ecx` 走完取指、译码、执行、访存、写回。单周期实现里这五步串在同一个长时钟里 |
| 流水线 | `pipeline`、`pipeline-fig`、`pipe-limit` | 重叠提高的是吞吐，一条指令的延迟不缩短，级间寄存器还要时间。时钟由最慢的一段决定。`pipe-limit` 只借用书上第 4.4 节的三段算术示意，不成为一台机器 |
| 两类冒险 | `forward`、`load-use`、`branch` 及各自的图 | `imull` 算出的 `%ecx` 可以转发给下一条 `addl`。`movl` 的结果下一句 `imull` 就要用，访存还没结束，停在译码与执行之间。`jl` 要到执行阶段才知道跳不跳，后面的指令已经被取进来，猜错则作废 |
| 现代的核 | `modern`、`bandwidth` | 乱序执行、分支预测与推测执行、一个周期发射多条指令。后果是 IPC 可以大于 1，也会因为预测失败和访存停顿远低于运算峰值。流水线不减少每个权重的读取 |
| 收束 | `next-lecture` | 这一讲解释不了带宽墙。下一讲是存储层次 |

## 这一讲不讲

Y86 指令编码、HCL、顺序实现的数据通路、五级转发电路、load-use 的停顿控制逻辑、流水线异常、architecture lab。转发只指出结果从执行阶段送到下一条的执行阶段；分支只指出已经取入的指令在猜错时作废，不给一个固定的气泡数。

配图由 `diagrams/*.py` 生成，不要手改 `assets/*.svg`。改完后：

```bash
sh lectures/4-cpu/diagrams/render.sh
python3 -m lecturekit.cli render lectures/4-cpu --png
```
