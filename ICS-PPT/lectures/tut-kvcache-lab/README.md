# ICS 辅导课 · Handson-Lab KVCache

本讲是 kvcache 实验（mini-ollama 的 KV Cache）的说明，有两个部分：KV Cache 的作用，KV Cache 的结构与访问过程。

讲义 id 是 `tut-kvcache-lab`，与目录名相同。封面与讲义的标题是「Handson-Lab KVCache」（2026-11-23 起，原为「mini-ollama 的 KV Cache」），PDF 与 PPTX 的文件名由标题得到。2026-11-23 由 `tut-quiz` 的第三部分拆出：原来的前两个小节成为两个部分，页面的 id 不变。原来第二小节的最后一页「下一次请求的查找与继续计算」（`kv-next`，13 帧）与第三小节「内存上限与失效」（衔接页与 `kv-memory`、`kv-reserve`、`kv-policies`、`kv-run-time` 四页）同时删去，页面代码、图的脚本、SVG、英文译文与 README 中只描述它们的段落在 `../../build/removed-tut-quiz-kv-next-limit/`。拆分前的目录在 `../../build/scratch-split/before/tut-quiz/`。

同日实验改为只有一段多轮对话，KV Cache 是一段连续的内存，块 tok 存放第 tok 个 token，没有会话、块表与位图，由你实现的函数是 5 个。各页随之改写：「会话与多轮对话」（`kv-sessions`）改为「多轮对话」（`kv-turns`），第二部分的三页改为连续的内存，动画由 16 帧减为 15 帧。改写前的目录在 `../../build/scratch-kvdeck3/before/tut-kvcache-lab/`，`kv-sessions` 的脚本、图与译文在 `../../build/removed-tut-kvcache-lab/`。

## 构建

```bash
python3 -m lecturekit.cli inspect lectures/tut-kvcache-lab
python3 -m lecturekit.cli view    lectures/tut-kvcache-lab --watch
python3 -m lecturekit.cli render  lectures/tut-kvcache-lab --to pdf
python3 -m lecturekit.cli render  lectures/tut-kvcache-lab --to pptx
sh lectures/tut-kvcache-lab/diagrams/render.sh
```

## 两种语言

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。代码块的内容不翻译，其中的注释是英文。

## 结构

封面之后是两个部分，每个部分以只有标题的衔接页（`lecture.bridge(...)`）开始，衔接页写作「第一部分：KV Cache 的作用」，做法与 `3-asm-lab` 相同。共 24 张幻灯片（动画的每一帧各占一张）。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。辅助函数与常量以 `_kv_` 开头。`pages.py` 以一段注释开头，写明讲义的设计与材料的来源，每个部分另以一段注释开头。每个页面函数的注释写明它承接上一页的方式。

| 部分 | 幻灯片 | 内容 |
| --- | --- | --- |
| 第一部分 · KV Cache 的作用 | 2–6 | 衔接页（2）· 多轮对话（3）· 一层中的注意力（公式，4）· 之前 token 的键与值（2 帧，5–6） |
| 第二部分 · KV Cache 的结构与访问过程 | 7–24 | 衔接页（7）· KV Cache 在内存中的结构（2 页：结构 8、一行的位置 9）· 第一轮对话的写入（15 帧，10–24） |

页码随修改变化，以上是 2026-11-23 的页码。

## 实验说明

材料是实验目录 `../../labs/kvcache`（`README.md`、`include/kvcache.h`、`src/kvcache.c`、`frame/main.c`）。一页是一张为实验画的图，配一两句话或几条短句，解释写在讲稿（`p.notes`）中。「一层中的注意力」（`kv-step`）一页例外：它由符号说明、一个头的注意力公式与一句结论组成，没有图。公式的下标从 0 开始，与 `frame/ops.c` 一致，第 j 个 token 在一层中的一行键与一行值称为这一层的第 j 行，这一对应写在讲稿中。图上标出你需要实现的函数起作用的位置：深色圆角标签是要实现的函数，`compute` 是灰色标签（实验中已写出），白底灰框是框架的函数。`compute` 是供你理解流程的写好的例子，它调用由你实现的函数。kvcache 中由你实现的函数是 5 个：`kv_block_bytes`、`kv_key`、`kv_value`、`kv_append`、`kv_reserve`。页面与讲稿不含参考答案的代码。

### 脱离代码

每一页须让一个没有仓库、没有运行过程序、只看投影的学生读懂（2026-11-22）。修改时保持以下几条：

- 一个例子在页上给全它的场景：之前的状态、这一轮的消息，以及其他数字所依据的数；
- 页面、图、讲稿与译文不引用 `make` 目标、`workload.txt`、命令行选项、框架的文件与某次运行的轮次编号，选项按含义写，比如「每轮最多生成 2 个 token」「不使用 KV Cache」。小例子中在页上交代过的「第 1 轮」除外；
- 一次实测在页上写明测了什么：模型、对话的各轮、缓存的初态、回答的长度。各行按含义命名；
- 保留学生实现的函数名，以及动画中画出并能在页上看懂的调用。只有读代码才明白的包装函数不画，写作「框架调用」；
- `kv-locate` 的文件按钮在句子之后，去掉按钮时句子仍完整。

### KV Cache 的小例子与动画

第二部分「KV Cache 的结构与访问过程」的三页共用一个小例子：2 层，`kv_dim` 为 10（一个头，d = 10），一块 40 个 float、160 字节，KV Cache 的内存有 8 块（内存上限中能放下 8 个 token），`max_answer` 为 2。对话第 1 轮的消息是 t0 t1 t2，这一轮预留块 0–3，结束时 `n_cached` 与 `capacity` 都是 4，t4 只输出。`kv_dim` 取 10，使一行的 float 数不与 token 数、块数混淆。`kv-layout` 与 `kv-locate` 取 t2 算完、t3 未算的时刻（`n_cached` 为 3，`capacity` 为 4），已缓存、预留、未使用三种块都有。`kv-locate` 找第 1 层 token 2 的值，地址是 base + 320 + 120 = base + 440，与实验 README 的例子相同。动画 `kv-first` 是第 1 轮，从空的 KV Cache 开始。修改时保持以下几条：

- 先讲结构，再讲函数：`kv-layout` 的图中不标函数；
- 块一律写作「块 n」，token 一律写作 t 加编号，块 n 存放 t_n。一块是内存中的一列，一列中自上而下是第 0 层 K、第 0 层 V、第 1 层 K、第 1 层 V 四行，放大时方向相同；
- 地址一律写作 base 之后的字节数（base+440），块的起点与行在块内的偏移也以字节标出，放大的一块中的数字是块内第几个 float，`kv-locate` 把 30 个 float 换成 120 字节；
- `struct kv_cache` 的六个字段在 `kv-layout` 与动画中画法相同，位于内存的左边；
- 动画的每一帧都给出调用者与实参、算出与返回的值、读写的格，以及 `n_cached`、`capacity` 的变化。各区域的位置逐帧固定，一帧一件事，与前面重复的 `compute` 合并成一帧，底部一句说明，逐帧的解释写在讲稿中；
- 动画中的字投影时要看得清：一句说明 20 px；面板中计算与结果 18 px，调用行 17 px（放不下时逐级缩小，第 11 帧是 16 px），地址行 15–16 px；`struct kv_cache` 的字段名 16 px、值 17 px、含义 14 px；内存中的块名、行名与层名 16–17 px；块的起点、读写标记、状态带、步骤栏与图例 15 px；其余标注不小于 13 px。面板只有三行，给出调用、算出与返回的值，读写与变化只画在 `struct kv_cache` 与内存中，返回一行只写返回值；
- 步骤栏中你实现的函数与已给出的函数不淡化，只用橙色框标出当前一步；
- 动画中的符号与 `kv-step` 相同；
- 一轮需要几块只用具体的 token 数出来（标出会作为输入的 token 与只输出的最后一个），页面、图、讲稿与译文不给一般算式，它由学生自己算出；
- 放不下时 `kv_reserve` 返回 0：这条规则只在动画第 2 帧的比较（4 不大于 `n_blocks` = 8）与它的讲稿中交代，没有一页演示它。

讲义中没有演示：`kv-run`（`make run` 的三次运行）于 2026-11-20 删去，页面代码在 `../../build/removed-tut-quiz-kv/`。删去的「框架与 `kvcache.c`」「一轮对话中的调用」两页（`kv-files`、`kv-calls`，2026-11-22）的代码、图与译文在 `../../build/removed-tut-quiz-kv-sec2/`。图中的数字来源如下：

- `kv-turns` 的 token 数取自重构后的实验用真实模型运行 `workload.txt` 的一次记录（2026-11-23，`../../build/scratch-kv-single/run/stderr.txt`）：每轮的 `capacity` 减去 7 是这一轮的 `n_prompt`（13、35、57、81、103），由此得到各轮消息的 token 数 13、14、14、16、14，五轮的回答都是 8 个 token，结束时文本有 111 个 token；
- 小例子的数字由 `../../build/scratch-kvdeck3/tools/check_numbers.py` 核对。它按实验 README 的规则独立重放第 1 轮，再用实验参考解的副本（`../../build/scratch-kvdeck3/lab/`）构建 `tools/driver.c`，运行已给出的 `compute`，记录每次 `attention` 收到的地址，两份记录逐行相同。然后它从生成的 SVG 中读回每帧的地址、`n_cached`、`capacity`、读写的格与各块的状态，与重放比较，`kv-turns` 的数与上面那次运行比较。在本目录运行 `python3 -B ../../build/scratch-kvdeck3/tools/check_numbers.py .`，2026-11-23 的结果是 672 项检查、0 处不一致；
- 拆分前的核对程序 `../../build/scratch-kvdeck2/tools/check_numbers.py` 与 `../../build/scratch-kvblocks/contract/` 针对按块分配的旧实验，对本讲义不再适用。

## 插图

插图的源文件在 `diagrams/`，`diagrams/render.sh` 运行全部脚本，生成 `assets/*.svg`，再用 `check_bounds.py` 检查文字是否超出画布。`assets/` 中的图不手改。全部图共用 `diagrams/svgkit.py`（取自 `9-gpu`）。`save()` 同时按 `svgkit.py` 中 `SCHEMES` 的各张色表替换颜色与淡出的不透明度，为每张图生成画布相同、透明底的 `<图名>.<方案>.svg`（比如 `kv-layout.dark.svg`），增加一个方案只需要增加一张色表。新增的颜色在 `svgkit.py` 中定义并写入每张色表，遇到表中没有的颜色时 `save()` 报错。

| 前缀 | 脚本 | 说明 |
| --- | --- | --- |
| `kv_` | 每个有图的页面一个脚本，公用画法在 `kv_kit.py`。`kv_step.py` 生成 `kv-reuse` 的两张图。`kv_layout.py`、`kv_locate.py`、`kv_first.py` 另外共用 `kv_example_kit.py`：小例子的状态、内存、`struct kv_cache`、动画一帧的版式都在其中，每帧只写改变了什么，各块的状态由这一帧的 `n_cached` 与 `capacity` 得到，变化由相邻两帧的状态相减得到 | 已缓存的行与块浅蓝，写入或改变橙色，读取绿色，重复的计算红色，预留的块白底蓝色虚线，未使用的块灰色，只输出的 token 灰底深色虚线。本步定位的格（动画中取得地址的一行）与 `kv-locate` 中要找的块和一行保持原色，加深色框，橙色只用于写入或改变。`kv-turns` 中之前各轮的文本浅灰，这一轮的消息深灰，回答白色 |
