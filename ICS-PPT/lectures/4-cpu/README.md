# ICS 第四讲 · 指令如何在 CPU 中重叠执行

接第 3 讲的 `dot_product` 循环体：编译器排出的那些 x86-64 指令，CPU 并不是一条做完再做下一条。这一讲说明重叠能快多少、为什么会停，以及为什么流水线做满之后，第 3 讲算出的带宽上限仍然不变。

中文是基线（写在 Python 里）。例子是第 3 讲 `gcc -Og -fcf-protection=none` 编译出的循环体，不另设一套指令集。

## 结构

开场两页之后分四部分，每部分前有一页「第 N 部分」过渡页；第一、二部分各有两个小节，第二个小节前另有一页小节过渡页（写法同 3-asm：`lecture.bridge` 分部分，`s.bridge` 分小节）。第四部分的对照表是最后一页，没有单独的总结页。

| 部分 · 小节 | 页 | 落点 |
| --- | --- | --- |
| 开场：第 3 讲留下的问题 | `question`、`loop-body` | 第 3 讲算出的两个上限（内存带宽 53 Token/s；每个周期最多 5 条指令开始执行，mini-ollama 1.1 Token/s，实测 0.77）与它留给本讲的两个问题（5 条怎样做到、指令之间等的是什么）；本讲的问题是这些指令一条做完再做下一条，还是几条同时在做。循环体仍是 `movslq`、`movl`、`imull`、`addl`、`addl $1` |
| 第一部分 CPU 的执行模式 · 一条指令的五个阶段与顺序执行 | `stages`、`stages-fig` | 用 `movl (%rsi,%r8,4), %ecx` 走完取指、译码（含读寄存器）、执行、访存、写回。单周期实现里这五步串在同一个长周期里：阶段之间不保存结果，PC、寄存器与内存只在时钟沿写入，所以一条指令要在一个周期内走完，周期按最慢的指令定（`single-cycle` 图） |
| 第一部分 · 流水线执行 | `stage-hw`、`pipeline-fig`、`pipe-limit` | 五个阶段由五组不同的硬件负责，单周期实现中每组硬件只在周期的一小段时间里工作；`stage-hw` 页末的「解决方案」给出流水线的原理：相邻两级之间加级间寄存器，让每组硬件每个周期都为一条指令工作。重叠提高的是吞吐，一条指令的延迟不缩短。周期由最慢的一级决定，级间寄存器还要时间，五级的周期长于单周期的五分之一。`pipe-limit` 只借用书上第 4.4 节的三级算术示意 |
| 第二部分 流水线执行的指令冲突 · data hazard | `forward`、`forward-fig`、`load-use`、`load-use-fig`（四页都是动画） | 没有转发时，后一条指令要等前一条写回之后的周期才能译码：`addl` 停 3 个周期（`forward`）。转发把 `imull` 在执行阶段算出的乘积直接送进 `addl` 的执行阶段，不用停（`forward-fig`）。带内存操作数的 `imull (%rdi,%r8,4), %ecx` 在五级流水线中拆成两条：`movl` 把 `w[i]` 读入 `%r10d`，`imull %r10d, %ecx` 相乘；冲突在 `%r10d`，没有转发停 3 个周期（`load-use`），转发后仍在译码与执行之间停一拍（`load-use-fig`）。`%r10d` 代替处理器内部的临时寄存器，循环体没有用到它 |
| 第二部分 · control hazard | `branch`、`branch-fig`（动画） | `jl` 要到执行阶段读到标志位才知道跳不跳，取指按猜测先取，猜错则作废 |
| 第三部分 拓展：现代 CPU | `modern`、`bandwidth` | 乱序执行、分支预测与推测执行、一个周期开始执行多条指令（第 3 讲的「每个周期最多 5 条」）。IPC 可以大于 1，也会因为停顿、猜错、前后相关远低于每周期 5 条的峰值（0.77 对 1.1）。流水线不减少每个权重的读取，53 Token/s 不变 |
| 第四部分 拓展：大模型推理中的流水线 | `llm-two-phases`、`llm-autoregressive`、`llm-kv-cache`、`llm-batching`、`llm-spec-decode`、`llm-summary` | prefill 在算力受限区、decode 在访存受限区（第 2 讲的 Roofline），`llm_stages.py` 实测相差 8 倍（数字在讲稿里）。自回归生成的公式 $x_t \sim P(x_t \mid x_1, \dots, x_{t-1})$ 与示意图（`llm-autoregressive`：相邻两步，取出的 token 追加到下一步的输入），同一页下方是注意力：$q_i$、$k_i$、$v_i$ 的定义与 $o_t = \mathrm{softmax}(q_t K_t^{\top}/\sqrt{d})\,V_t$，$K_t$、$V_t$ 逐行排列 $k_1 \dots k_t$、$v_1 \dots v_t$；$K$、$V$ 每一步末尾加一行、前面的行不变，$q_t$ 只在第 $t$ 步使用，所以 KV 缓存只存 $K$、$V$ 两个矩阵（这个推理写在讲稿里，正文只有公式）；页底一句说明 $o_t$ 是 $v_1 \dots v_t$ 的加权和、成为下一层的 $h_t$，最后一层位置 $t$ 的结果经输出矩阵与 softmax 得到 $P(x_{t+1} \mid x_1, \dots, x_t)$。注意力公式按「位置 $t$」编号，与自回归公式的「第 $t$ 步」差一位，KV 缓存一页同样写「位置 $t$」。$K_t$、$V_t$ 用 `smallmatrix`，换成 `bmatrix` 后这一页放不下，文字会压到图上。KV 缓存一页给出每个 token 的大小（Qwen3-VL-2B 每个 token 112 KiB），对应转发，连续批处理对应流水线的重叠，投机解码对应分支预测与作废；这两页的页脚注明出处论文（连续批处理：Orca，OSDI 2022；prefill 分块：Sarathi-Serve，OSDI 2024；投机解码：Leviathan 等，ICML 2023，与同期的 Chen 等，2023）。末页是对照表。取自 `4-cpu-bak/bak-2` 的 `llm.py`，回指已改为本讲的页面 |

## 这一讲不讲

Y86 指令编码、HCL、顺序实现的数据通路、五级转发电路、load-use 的停顿控制逻辑、流水线异常、architecture lab。转发只指出结果（执行阶段算出的、或访存阶段读出的）直接送进下一条的执行阶段，不画电路；分支只指出已经取进来的指令在猜错时作废，不给一个固定的作废条数（图上画的是五级模型在判定那一拍的状态）。

## 术语

与第 3 讲一致：
- 正文计数用「周期」；「拍」只在「停一拍」和讲稿口语里；「时钟」只指时钟信号或周期长短。
- 「阶段」说一条指令经过的五步，「级」说流水线的硬件分级（五级、级间寄存器、最慢的一级），不用「段」。
- 名词「停顿」，动词「停一拍」，图格子「停」；「气泡」只在讲稿提一次。
- 一律「作废」。
- 「转发」在 `forward-fig` 页正文定义；「开始执行」沿用第 3 讲，「发射」只在讲稿里。
- 「标志位」不写「条件码」；正文不裸用 load / store。
- data hazard、control hazard 用英文，不译成「数据冒险」「控制冒险」；统称 hazard 与 load-use data hazard 只在讲稿里，正文写「读内存后马上使用」。

每页讲稿三件事：要讲的点、学生易错处、页末引出下一页的问题。讲稿只回指讲过的内容；要提前用到的术语，写一句点明讲它的那一节。

`question` 与 `bandwidth` 两页的正文里，`1.1 Token/s` 的数字与单位之间、`51.2 GB/s ÷ 0.968 GB ≈ 53 Token/s` 这个算式内部用的是不换行空格（U+00A0），免得渲染时把数字与单位、算式拆到两行；改这两句时保留它。

## 配图

配图由 `diagrams/*.py` 生成，不要手改 `assets/*.svg`。时空图（pipeline、no-forward、forward、stall、branch）共用 `diagrams/timegrid.py` 的格子规格（96×52 的格、五阶段配色、周期号、行标签位置），翻页时格子不跳位置；`timegrid.py` 只定义函数，不生成图，`render.sh` 跳过它。no-forward、forward、stall、branch 是动画（`p.frames`），每帧推进一个周期：脚本输出 `forward-1.svg` 起的若干帧，最后一帧是整图，`timegrid.row` 的 `upto` 只画到某个周期；动画的每一帧在 deck 里算一页。`no_forward.py` 画 `forward`、`load-use` 两页的「没有转发」动画，两套时序相同（前一条第 5 周期写回，后一条在第 3 至 5 周期停、第 6 周期译码），前 5 帧每帧一个周期，最后一帧补齐周期 7 至 9 并用红括号标出停的 3 个周期；冲突的周期 3 在图顶用红字写明，写回格到译码格的深色箭头朝右。`stall.py` 的行距比 `forward.py` 小（那一页图上方有一段正文）。第 6 页的 `single-cycle` 不是等宽的时空图：格子宽度按各阶段用时示意（取指、执行、访存 2 份，译码、写回 1 份），配色与行高取自 `timegrid.py`。第 8 页的 `stage-hw` 与它用同一条时间轴，按硬件分行：`stage_hw.py` 从 `single_cycle.py` 取阶段宽度与指令，改一处两张图一起变。约定：图顶不写与页标题同义的整句；图底不写结论句；`forward-fig`、`branch-fig` 用 `p.highlight` 收口，写在 `p.frames` 之后，只在最后一帧出现；`forward`、`load-use`、`load-use-fig` 三页不加 highlight；写回格用深色实心，取指与写回不同色。改完后：

```bash
sh lectures/4-cpu/diagrams/render.sh
python3 -m lecturekit.cli render lectures/4-cpu --png
```

`llm-two-phases` 讲稿里的数字是 2026-10-07 在 RTX 3060 Laptop GPU 上的一次运行（prefill 1455.8 token/s，decode 190.2 token/s）。重测在 `examples/` 下运行 `python3 llm_stages.py`，需要 `ollama serve` 在运行，并已 `ollama create nq-qwen3vl-2b -f ../../nano-quant/model/Modelfile`；换了数字要同步 `llm-two-phases` 的讲稿。
