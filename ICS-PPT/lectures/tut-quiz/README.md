# ICS 辅导课 · Quiz 与实验说明

本讲有三个部分：Quiz 第 1 ~ 5 题的题面，mini-cpu 实验二（组相联缓存）的说明，kvcache 实验（mini-ollama 的 KV Cache）的说明。

讲义 id 是 `tut-quiz`，与目录名相同。封面与讲义的标题是「Quiz 与实验说明」，PDF 与 PPTX 的文件名由标题得到。

## 构建

```bash
python3 -m lecturekit.cli inspect lectures/tut-quiz
python3 -m lecturekit.cli view    lectures/tut-quiz --watch
python3 -m lecturekit.cli render  lectures/tut-quiz --to pdf
python3 -m lecturekit.cli render  lectures/tut-quiz --to pptx
sh lectures/tut-quiz/diagrams/render.sh
```

## 两种语言

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。代码块与演示的命令、输出不翻译，其中的注释是英文。

## 结构

封面之后是三个部分，部分之间用 `lecture.bridge(...)` 过渡。第二、三部分各分三个小节，每个小节以只有标题的衔接页（`s.bridge(...)`）开始，做法与 `3-asm-lab` 相同。共 97 张幻灯片（动画的每一帧各占一张）。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。三个部分的辅助函数与常量分别以 `_quiz_`、`_cache_`、`_kv_` 开头。每个部分在 `pages.py` 中以一段注释开头，写明它的设计与材料的来源。每个页面函数的注释写明它承接上一页的方式。

| 部分 | 幻灯片 | 内容 |
| --- | --- | --- |
| 第一部分 · Quiz 题目 | 2–27 | 第 1 题（4 页）、第 2 题（6 页）、第 3 题（2 页）、第 4 题（7 页，含第（2）题第 1 问的答案一页）、第 5 题（6 页） |
| 第二部分 · mini-cpu 的组相联缓存 | 28–49 | 访存路径：实验一、实验二的访存路径。`cache.c` 的函数与调用者：调用关系 · `cache_read` 与 `cache_write` 的调用者（`frame/mem.c` 的 `walk`）。缓存的结构与访问过程：内存地址的拆分 · 组与路 · 命中时调用的函数 · 命中（3 帧）· 缺失时与替换脏行时调用的函数 · 缺失并写回脏行（5 帧）· 跨两行的访问 |
| 第三部分 · mini-ollama 的 KV Cache | 50–97 | KV Cache 的作用（51–55）：会话与多轮对话 · 一层中的注意力（公式）· 之前 token 的键与值（2 帧）。KV Cache 的结构与访问过程（56–87）：KV Cache 在内存中的结构（2 页：结构、一行的位置）· 第一次请求的写入（16 帧）· 下一次请求的查找与继续计算（13 帧）。内存上限与失效（88–97）：内存与上限 · 内存不足时的 `kv_reserve`（6 帧）· lru 与 smallest 的失效 · 运行时间与内存上限 |

页码随修改变化，以上是 2026-11-22 的页码。各部分的页码包括部分与小节的衔接页。

## 第一部分：题面

题面取自 `../../exames/quiz.md`（`quiz.pdf` 的源文件），题号、分值、文字、代码、表与图照原卷，不加提示，不放答案。唯一的例外是第 4 题（2）第 1 问之后的一页（`q4-rob-answer`）：它把 `../../exames/quiz-answer.md` 中各指令进入、执行、完成与提交的周期，换成周期 1 ~ 9 结束时 ROB 中按程序顺序排列的条目与状态，周期 9 一行是答案；周期 7、8 只有 4 个条目的原因写在讲稿中。

一道题可以分成几页，一页放一个小问，或几个放得下的小问。小问用到的代码、寄存器与内存的值、表、规则、假设与图尽量放在同一页，各页重复它用到的材料，表只留小问用到的行，规则与假设只留小问用到的条目。主题的字号固定，有几页放不下小问用到的全部材料，缺的部分在同一题的前一页或后一页，`pages.py` 第一部分开头的注释逐页列出缺什么。

第 4、5 题各是一个小节，小节名是题目的标题，各页标题是题目中所在部分的标题。

## 第二、三部分：实验说明

材料是实验目录 `../../labs/mini-cpu`（`src/lab2/`）与 `../../labs/kvcache`（`src/kvcache.c`）。一页是一张为实验画的图，配一两句话或几条短句，解释写在讲稿（`p.notes`）中。「一层中的注意力」（`kv-step`）一页例外：它由符号说明、一个头的注意力公式与一句结论组成，没有图。公式的下标从 0 开始，与 `frame/ops.c` 一致，第 j 个 token 在一层中的一行键与一行值称为这一层的第 j 行，这一对应写在讲稿中。图上标出你需要实现的函数起作用的位置：深色圆角标签是要实现的函数，`cache_init`、`kv_open` 与 `compute` 是灰色标签（实验中已写出），白底灰框是框架的函数。`compute` 是供你理解流程的写好的例子，它调用由你实现的函数。kvcache 中由你实现的函数是 7 个：`kv_block_bytes`、`kv_key`、`kv_value`、`kv_append`、`kv_invalidate`、`kv_victim`、`kv_reserve`。页面与讲稿不含参考答案的代码。

### 第三部分脱离代码

第三部分的每一页须让一个没有仓库、没有运行过程序、只看投影的学生读懂（2026-11-22）。修改时保持以下几条：

- 一个例子在页上给全它的场景：之前的状态、这次请求，以及其他数字所依据的数；
- 页面、图、讲稿与译文不引用 `make` 目标、`workload.txt`、命令行选项、框架的文件与某次运行的轮次编号，选项按含义写，比如「每次最多生成 2 个 token」「不使用 KV Cache」。小例子中在页上交代过的「A 第 1 轮」「A 第 2 轮」除外；
- 一次实测在页上写明测了什么：模型、请求的先后、缓存的初态、回答的长度。各行按含义命名；
- 保留学生实现的函数名，以及两段动画中画出并能在页上看懂的调用。只有读代码才明白的包装函数不画，写作「框架调用」；
- `kv-layout`、`kv-locate` 的文件按钮在句子之后，去掉按钮时句子仍完整。

### KV Cache 的小例子与动画

第三部分「KV Cache 的结构与访问过程」的四页共用一个小例子：2 层，`kv_dim` 为 10（一个头，d = 10），一块 40 个 float、160 字节，内存池 12 块，`max_answer` 为 2。会话 A（id 0）第 1 轮的消息是 t0 t1 t2，分到块 0–3。会话 B（id 1）运行一轮，消息是 b0，分到块 4、5。A 第 2 轮的新消息是 t5，只再分到块 6–8，结束时 `n_cached` 与 `capacity` 都是 7。`kv_dim` 取 10，使一行的 float 数不与 token 数、块数混淆。`kv-layout` 与 `kv-locate` 取第 2 轮 t4 算完、t5 未算的时刻：A 的块不连续，token 4 在块 6，有效、预留、空闲三种块都有。修改时保持以下几条：

- 先讲结构，再讲函数：`kv-layout` 的图中不标函数；
- 块一律写作「块 n」，token 一律写作 t 或 b 加编号，一块是池中的一列，一列中自上而下是偏移 0、10、20、30 的四行，放大时方向相同；
- 两段动画（`kv-first`、`kv-next`）共不超过 30 帧，每一帧都给出调用者与实参、算出与返回的值、读写的格、会话表与块表及位图的变化。各区域的位置逐帧固定，一帧一件事，与前面重复的 `compute` 合并成一帧，底部一句说明，逐帧的解释写在讲稿中；
- 动画中的字投影时要看得清：一句说明 20 px，面板与表 17–18 px，内存池 16–17 px，表头与标注不小于 13 px。面板只有三行，读写与变化画在表和池中，不在面板中重复；
- 步骤栏中你实现的函数与已给出的函数不淡化，只用橙色框标出当前一步；
- 动画中的符号与 `kv-step` 相同；
- 一轮需要几块只用具体的 token 数出来（标出会作为输入的 token 与只输出的最后一个），页面、图、讲稿与译文不给一般算式，它由学生自己算出；
- 内存不足留给第三小节。它的前三页共用一个例子：96 块的池中 A、B、C 已缓存 42、19、21 个 token，`last_use` 是 2、3、4，B 又收到 17 个 token 的消息，最多回答 8 个。B 缺 25 块，空闲 14 块，三个会话最终需要 107 块，真正超出上限。`kv-memory` 只提出问题，不出现收回与失效。`kv-policies` 在同一起点上比较随后依次到来的三次请求（B、A、C）。

讲义中没有演示：第二部分的 `cache-stats` 与第三部分的 `kv-run`（`make run` 的三次运行）于 2026-11-20 删去。第二部分图与讲稿中的计数、第三部分的运行时间都录自实验目录在 `build/` 下的副本，副本换入了助教目录的参考实现：

- mini-cpu：一个副本按 `make LAB=1` 构建，即实验一的机器，另一个副本换入 `../../labs/mini-cpu-ta/lab2/cache.c` 后构建。两个副本的 `src/lab1/` 都换成了 `../../labs/mini-cpu-ta/lab1/`。`cache-funcs-hit` 讲稿中的 99.43% 录自后一个副本中的 `./mini-cpu --stats guest/rowcol rows`，行访问含取指，讲稿中以文字写明程序做什么、矩阵与缓存多大。原来的 `cache-stats` 一页（有缓存与没有缓存的计数与运行时间的两个演示）于 2026-11-20 删去，页面代码在 `../../build/removed-tut-quiz-stats/`。
- kvcache 的运行时间一页：时间与计数取自 `../../build/scratch-kvblocks/handoff/runs.md`（`STATUS: final`），是参考实现在真实模型上的四次运行，各一次：96 块时的 `lru` 与 `smallest`、不使用 KV Cache、182 块。页上按含义命名各行，并画出 7 次请求的先后 A A B C B A C。重录时更新 `diagrams/kv_run_time.py`、`kv_run_time` 的讲稿与译文，再运行核对程序。删去的 `kv-run` 与 `kv-check`（`make test` 的检查）两页的代码、图与译文在 `../../build/removed-tut-quiz-kv/`。
- kvcache 的其他数字：`../../build/scratch-kvblocks/contract/` 是按块存储的契约，其中 `simulate.py` 独立于实验代码重放这些规则，`expected.txt` 是它对小例子与 `make run`（21M，96 块）的完整输出。`../../build/scratch-kvdeck2/tools/check_numbers.py` 从生成的 SVG 中读回每帧的位、块表项、字段、地址、读写的格与第三小节的各个数，与它们和 `runs.md` 逐项比较：`python3 -B ../../build/scratch-kvdeck2/tools/check_numbers.py .`。删去的「框架与 `kvcache.c`」「一轮对话中的调用」两页（`kv-files`、`kv-calls`，2026-11-22）的代码、图与译文在 `../../build/removed-tut-quiz-kv-sec2/`。


## 插图

插图的源文件在 `diagrams/`，`diagrams/render.sh` 运行全部脚本，生成 `assets/*.svg`，再用 `check_bounds.py` 检查文字是否超出画布。`assets/` 中的图不手改。全部图共用 `diagrams/svgkit.py`（取自 `9-gpu`）。`save()` 同时按 `svgkit.py` 中 `SCHEMES` 的各张色表替换颜色与淡出的不透明度，为每张图生成画布相同、透明底的 `<图名>.<方案>.svg`（比如 `cache-path.dark.svg`），增加一个方案只需要增加一张色表。新增的颜色在 `svgkit.py` 中定义并写入每张色表，遇到表中没有的颜色时 `save()` 报错。

| 前缀 | 脚本 | 说明 |
| --- | --- | --- |
| `q_` | `q_rob.py`、`q_cpu_memory.py`、`q_gpu_memory.py`、`q_gpu_disk.py` | 由 `../../exames/diagrams/` 复制，只改了导入的 `svgkit.py` 与输出路径，生成的图与原卷的图相同 |
| `cache_` | 每页一个脚本，公用画法在 `cache_kit.py` | 缓存的行浅蓝，内存绿色，命中绿色，内存读写（经 `clflush` 访问宿主的内存）红色。地址的三段 tag 紫、组号蓝、行内偏移橙。`cache_funcs.py` 另外生成三种情况的调用路径图，路径上的函数与箭头保持原色、箭头加粗，其余部分的不透明度降到 20%（深色图中为 36%） |
| `kv_` | 每个有图的页面一个脚本，公用画法在 `kv_kit.py`。`kv_step.py` 生成 `kv-reuse` 的两张图。`kv_layout.py`、`kv_locate.py`、`kv_first.py`、`kv_next.py` 另外共用 `kv_example_kit.py`：小例子的状态、内存池、动画一帧的版式都在其中，每帧只写改变了什么，块号一律由这一帧的块表得到，变化由相邻两帧的状态相减得到。`kv_memory.py`、`kv_reserve.py`、`kv_policies.py` 共用 `kv_run_kit.py` 中第三小节的例子：96 块的内存池、起点与随后的三次请求 | 有效的行与块浅蓝，写入或改变橙色，读取绿色，重复的计算与失效释放的块红色，预留的块白底蓝色虚线，空闲的块灰色，只输出的 token 灰底深色虚线。块下方标出所属的会话：A 蓝、B 紫、C 棕。96 块的池按比例画，一格是一块，即一个 token 的 224 KiB |
