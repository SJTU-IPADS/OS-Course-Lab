# ICS 第四讲 · 指令如何在 CPU 中重叠执行

接第 3 讲的 `dot_product` 循环体：编译器排出的那些 x86-64 指令，CPU 并不是一条做完再做下一条。这一讲说明重叠能快多少、为什么会停，以及为什么流水线做满之后，第 3 讲算出的带宽上限仍然不变。

中文是基线（写在 Python 里）。例子是第 3 讲 `gcc -Og -fcf-protection=none` 编译出的循环体，不另设一套指令集。

## 结构

开场两页之后分四部分，每部分前有一页「第 N 部分」过渡页；第一、二、四部分分小节，第一个小节之外的每个小节前另有一页小节过渡页（写法同 3-asm：`lecture.bridge` 分部分，`s.bridge` 分小节）。第四部分的最后一页「同一套设计方法」是全讲的收束。

| 部分 · 小节 | 页 | 落点 |
| --- | --- | --- |
| 开场：第 3 讲留下的问题 | `question`、`loop-body` | 第 3 讲算出的两个上限（内存带宽 53 Token/s；每个周期最多 5 条指令开始执行，mini-ollama 1.1 Token/s，实测 0.77）与它留给本讲的两个问题（5 条怎样做到、指令之间等的是什么）；本讲的问题是这些指令一条做完再做下一条，还是几条同时在做。循环体仍是 `movslq`、`movl`、`imull`、`addl`、`addl $1` |
| 第一部分 CPU 的执行模式 · 一条指令的五个阶段与顺序执行 | `stages`、`stages-fig` | 用 `movl (%rsi,%r8,4), %ecx` 走完取指、译码（含读寄存器）、执行、访存、写回。单周期实现里这五步串在同一个长周期里：阶段之间不保存结果，PC、寄存器与内存只在时钟沿写入，所以一条指令要在一个周期内走完，周期按最慢的指令定（`single-cycle` 图） |
| 第一部分 · 流水线执行 | `stage-hw`、`pipeline-fig`、`pipe-limit` | 五个阶段由五组不同的硬件负责，单周期实现中每组硬件只在周期的一小段时间里工作；`stage-hw` 页末的「解决方案」给出流水线的原理：相邻两级之间加级间寄存器，让每组硬件每个周期都为一条指令工作。重叠提高的是吞吐，一条指令的延迟不缩短。周期由最慢的一级决定，级间寄存器还要时间，五级的周期长于单周期的五分之一。`pipe-limit` 只借用书上第 4.4 节的三级算术示意 |
| 第二部分 流水线执行的指令冲突 · data hazard | `forward`、`forward-fig`、`load-use`、`load-use-fig`（四页都是动画） | 没有转发时，后一条指令要等前一条写回之后的周期才能译码：`addl` 停 3 个周期（`forward`）。转发把 `imull` 在执行阶段算出的乘积直接送进 `addl` 的执行阶段，不用停（`forward-fig`）。带内存操作数的 `imull (%rdi,%r8,4), %ecx` 在五级流水线中拆成两条：`movl` 把 `w[i]` 读入 `%r10d`，`imull %r10d, %ecx` 相乘；冲突在 `%r10d`，没有转发停 3 个周期（`load-use`），转发后仍在译码与执行之间停一拍（`load-use-fig`）。`%r10d` 代替处理器内部的临时寄存器，循环体没有用到它 |
| 第二部分 · control hazard | `branch`、`branch-fig`（动画） | `jl` 要到执行阶段读到标志位才知道跳不跳，取指按猜测先取，猜错则作废 |
| 第三部分 拓展：现代 CPU | `modern`、`bandwidth` | 乱序执行、分支预测与推测执行、一个周期开始执行多条指令（第 3 讲的「每个周期最多 5 条」）。IPC 可以大于 1，也会因为停顿、猜错、前后相关远低于每周期 5 条的峰值（0.77 对 1.1）。流水线不减少每个权重的读取，53 Token/s 不变 |
| 第四部分 拓展：系统里的流水线 · 大模型推理在做什么 | `llm-what`、`llm-layers`、`llm-too-big`、`llm-split` | 面向零基础：大模型是「给定前文、算出下一个词」的函数，第 6 步要等第 5 步取出的词（`llm-autoregressive` 图，例句「流水线提高的是吞吐率」）；算一次是向量穿过 28 层，每层权重 × 向量，17.2 亿个权重 0.968 GB 每个词全部读一遍（`llm-layers`）；Llama 3.1 405B 的 810 GB 对 H100 的 80 GB，至少 11 块（`llm-too-big`）；按层切成 4 份放 4 块 GPU，GPU 间传的向量就是级间寄存器的内容，叫流水线并行（`llm-split`） |
| 第四部分 · 流水线并行与它的气泡 | `llm-pp-single`、`llm-pp-fill`（动画 4 帧）、`llm-pp-ratio`、`llm-pp-decode`、`llm-pp-batch` | 时空图的一行是一块 GPU。一个请求：每拍只有一块 GPU 在算，16 格 12 格停，停格就是气泡。多个请求跟着进来：第 4 拍四块全满，延迟不变、吞吐提高；两头是进管与排空气泡。占比 (K − 1) ÷ (M + K − 1)，K = 4、M = 8 时 27%（GPipe，NeurIPS 2019；页上有 M = 4/8/16/32 的表）。逐词生成：词 2 要等 GPU 4 取出词 1，GPU 1 停三拍，同一请求独自使用时流水线和一块 GPU 一样慢（第 2 部分 data hazard 的对应）。多个用户轮流：请求 2、3、4 填进那三拍，请求 1 的词 2 恰在第 5 拍回到 GPU 1，第 4 拍起没有气泡，独立请求数至少等于 GPU 块数（连续批处理，Orca，OSDI 2022）。实测数字（prefill 1455.8、decode 190.2 token/s）在 `llm-pp-decode` 讲稿里 |
| 第四部分 · 投机推理 | `llm-decode-slow`、`llm-spec-idea`、`llm-spec-verify`、`llm-spec-vs-branch`、`llm-spec-gain` | 逐词生成慢在每个词读一遍全部权重，几个词的输入都已知时读一遍得到几个词（`llm-read-once`，实测 190.2 对 1455.8 词/秒）。小模型先连猜 4 个词（`spec-guess`），对应分支预测；大模型连同前文读一遍权重同时核对 4 个位置，从前往后比对：接受、第一个猜错处换成大模型的词、其后作废，例子接受「吞」「吐」改为「率」作废「，」，一遍得到 3 个词（`spec-decode`）。与分支预测逐行对照的表（谁猜、谁判定、猜对、猜错、最终结果），结论「猜测只影响速度」。划不划算：猜对 k 个前进 k + 1 个，典型 2–3 倍（Leviathan 等，ICML 2023；Chen 等，2023） |
| 第四部分 · 其他系统里的流水线 | `assembly-line`、`video-pipeline`、`pipeline-summary` | 1913 年福特装配线（底盘装配 12 小时 28 分 → 1 小时 33 分），流水线这个词的来历；看视频：下载、解码、显示三级，段与段之间的缓冲区就是级间缓冲，开头「缓冲几秒」是进管，网络慢时缓冲区读空、画面转圈是气泡，自动降清晰度是给最慢的一级减负（`video-pipeline`）。Unix 管道与 GPU 图形管线不讲：前者没有固定节拍、气泡不是格子，后者学生没有体验。末页四行四列的表：一级是什么、级间缓冲是什么、气泡从哪来，加两条总结，再留给下一讲的问题 |

## 这一讲不讲

Y86 指令编码、HCL、顺序实现的数据通路、五级转发电路、load-use 的停顿控制逻辑、流水线异常、architecture lab。转发只指出结果（执行阶段算出的、或访存阶段读出的）直接送进下一条的执行阶段，不画电路；分支只指出已经取进来的指令在猜错时作废，不给一个固定的作废条数（图上画的是五级模型在判定那一拍的状态）。

## 术语

与第 3 讲一致：
- 正文计数用「周期」；「拍」只在「停一拍」和讲稿口语里；「时钟」只指时钟信号或周期长短。
- 「阶段」说一条指令经过的五步，「级」说流水线的硬件分级（五级、级间寄存器、最慢的一级），不用「段」。
- 名词「停顿」，动词「停一拍」，图格子「停」。第四部分正文用「气泡」指时空图上的停格（第一次出现在 `llm-pp-single`），讲稿里也可以用。
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

`llm-pp-decode` 与 `llm-decode-slow` 讲稿里的数字是 2026-10-07 在 RTX 3060 Laptop GPU 上的一次运行（prefill 1455.8 token/s，decode 190.2 token/s）；`llm-read-once` 图上也印着这两个数。重测在 `examples/` 下运行 `python3 llm_stages.py`，需要 `ollama serve` 在运行，并已 `ollama create nq-qwen3vl-2b -f ../../nano-quant/model/Modelfile`；换了数字要同步这两页的讲稿和 `diagrams/llm_read_once.py`。

第四部分的时空图（`llm_pp_single.py`、`llm_pp_fill.py`、`llm_pp_decode.py`、`llm_pp_batch.py`）也用 `timegrid.py` 的格子，行标签是四块 GPU，格子里是请求号或词号（`TONE` 里有「请求 1」至「请求 4」、「词 1」「词 2」）；`llm_pp_ratio.py` 要画 11 列，用更小的格子自己画。`spec_decode.py` 一次生成 `spec-guess.svg`（只到小模型猜的那一行）和 `spec-decode.svg`（整图），两张图坐标相同，翻页时格子不动。`llm_autoregressive.py` 仍在用（`llm-what` 页）；`llm_phases.py`、`llm_kv_cache.py`、`llm_batching.py` 现在没有页面引用。
