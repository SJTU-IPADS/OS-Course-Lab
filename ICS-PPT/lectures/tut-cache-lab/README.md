# ICS 辅导课 · MiniCPU Lab2: Cache

本讲是 mini-cpu 实验二（组相联缓存）的说明，有三个部分：访存路径，`cache.c` 的函数与调用者，缓存的结构与访问过程。

讲义 id 是 `tut-cache-lab`，与目录名相同。封面与讲义的标题是「MiniCPU Lab2: Cache」（2026-11-23 起，原为「mini-cpu 的组相联缓存」），PDF 与 PPTX 的文件名由标题得到。2026-11-23 由 `tut-quiz` 的第二部分拆出：原来的三个小节成为三个部分，页面、图、讲稿与译文没有改，页面的 id 不变。拆分前的目录在 `../../build/scratch-split/before/tut-quiz/`。

同日按拆分后的 mini-cpu 框架更新：框架分为 `frame/cpu/`（硬件）与 `frame/os/`（软件），`cache_read` 与 `cache_write` 每次只收到一行之内的字节，跨行的访问在到达缓存之前已经按行拆开。各页的路径与函数名随之更新，`cache-walk` 展示的代码由 `walk` 换成 `frame/cpu/mem.c` 的 `mem_access`（页面 id 不变），第二部分加了 `cache-chain` 一页（框架中访存的调用链），`cache-span` 一页按新的约定重写。更新前的目录在 `../../build/scratch-cachedeck/before/tut-cache-lab/`。同日按独立核查修正：`cache-walk` 的标题改为「`mem_access` 对访存的拆分」，`cache-span` 的标题改为「跨两个缓存行的访问」、正文只讲两次行访问，`cache-chain` 图中两条红色箭头改从 `cache.c` 一带的同一处分出，几处「A，不B」的写法只留肯定的一半，修正前的目录在 `../../build/scratch-cachedeck-fix/before/tut-cache-lab/`。同日按你的决定删去 `cache-span`（跨两个缓存行的访问）一页：它与 `cache-chain`、`cache-walk` 重复，讲稿中跨两行的访存计两次命中或缺失一点移入 `cache-walk` 的讲稿，写分配移入 `cache-funcs-miss` 的讲稿，删去的页面、脚本、图与译文在 `../../build/removed-tut-cache-lab-cache-span/`。

## 构建

```bash
python3 -m lecturekit.cli inspect lectures/tut-cache-lab
python3 -m lecturekit.cli view    lectures/tut-cache-lab --watch
python3 -m lecturekit.cli render  lectures/tut-cache-lab --to pdf
python3 -m lecturekit.cli render  lectures/tut-cache-lab --to pptx
sh lectures/tut-cache-lab/diagrams/render.sh
```

## 两种语言

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。代码块的内容不翻译，其中的注释是英文。

## 结构

封面之后是三个部分，每个部分以只有标题的衔接页（`lecture.bridge(...)`）开始，衔接页写作「第一部分：访存路径」，做法与 `3-asm-lab` 相同。共 22 张幻灯片（动画的每一帧各占一张）。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。辅助函数与常量以 `_cache_` 开头。`pages.py` 以一段注释开头，写明讲义的设计与材料的来源，每个部分另以一段注释开头。每个页面函数的注释写明它承接上一页的方式。

| 部分 | 幻灯片 | 内容 |
| --- | --- | --- |
| 第一部分 · 访存路径 | 2–4 | 衔接页（2）· 实验一的访存路径（3）· 实验二的访存路径（4） |
| 第二部分 · `cache.c` 的函数与调用者 | 5–8 | 衔接页（5）· 调用关系（6）· 框架中访存的调用链（`cache-chain`，7）· `mem_access` 对访存的拆分（`frame/cpu/mem.c`，页面 id 是 `cache-walk`，8） |
| 第三部分 · 缓存的结构与访问过程 | 9–22 | 衔接页（9）· 内存地址的拆分（10）· 组与路（11）· 命中时调用的函数（12）· 命中（3 帧，13–15）· 缺失时调用的函数（16）· 替换脏行时调用的函数（17）· 缺失并写回脏行（5 帧，18–22） |

页码随修改变化，以上是 2026-11-23 删去 `cache-span` 之后的页码。

`cache-walk` 的代码最长一行 75 列。页面的高度放不下整个 `mem_access`，页面只列出它的循环（18 行，去掉一级缩进），换算失败时的两行写作一行 `...`，函数头、循环之前的长度检查与之后的 `return 0` 不列出，讲稿写明略去的内容。代码从 `../../labs/mini-cpu/frame/cpu/mem.c` 复制，那里改动后需要重新核对。

## 实验说明

材料是实验目录 `../../labs/mini-cpu`（`src/lab2/`）。一页是一张为实验画的图，配一两句话或几条短句，解释写在讲稿（`p.notes`）中。图上标出你需要实现的函数起作用的位置：深色圆角标签是要实现的函数，`cache_init` 是灰色标签（实验中已写出），白底灰框是框架的函数。页面与讲稿不含参考答案的代码。

讲义中没有演示：`cache-stats` 于 2026-11-20 删去。图与讲稿中的计数录自实验目录在 `build/` 下的副本，副本换入了助教目录的参考实现：

- mini-cpu：`../../build/scratch-cachedeck/mini-cpu/`，2026-11-23 按拆分后的框架复制，同日换入实验此后修改的 `frame/cpu/mem.c`、`frame/cpu/dram.c`、`frame/main.c`、`frame/jit.c`、`include/mcpu.h` 与两个 README 并重新构建，`src/lab1/` 换成 `../../labs/mini-cpu-ta/lab1/`，`src/lab2/cache.c` 换成 `../../labs/mini-cpu-ta/lab2/cache.c`，按 `make LAB=2` 构建。`cache-funcs-hit` 讲稿中的 99.43% 录自其中的 `./mini-cpu --stats guest/rowcol rows`（`cache accesses 1619325  hits 1610053  misses 9272  hit rate 99.43%  writebacks 1035`，与 2026-11-18 在 `../../build/scratch-tutquiz-cache/mini-cpu/` 中按旧框架录得的相同），行访问含取指，讲稿中以文字写明程序做什么、矩阵与缓存多大。讲义中没有其他录自运行的数字：`cache-hit`、`cache-miss` 中组 13 的状态与计数器的值是为说明过程设的。原来的 `cache-stats` 一页（有缓存与没有缓存的计数与运行时间的两个演示）于 2026-11-20 删去，页面代码在 `../../build/removed-tut-quiz-stats/`。

## 插图

插图的源文件在 `diagrams/`，`diagrams/render.sh` 运行全部脚本，生成 `assets/*.svg`，再用 `check_bounds.py` 检查文字是否超出画布。`assets/` 中的图不手改。全部图共用 `diagrams/svgkit.py`（取自 `9-gpu`）。`save()` 同时按 `svgkit.py` 中 `SCHEMES` 的各张色表替换颜色与淡出的不透明度，为每张图生成画布相同、透明底的 `<图名>.<方案>.svg`（比如 `cache-path.dark.svg`），增加一个方案只需要增加一张色表。新增的颜色在 `svgkit.py` 中定义并写入每张色表，遇到表中没有的颜色时 `save()` 报错。

| 前缀 | 脚本 | 说明 |
| --- | --- | --- |
| `cache_` | 每页一个脚本，公用画法在 `cache_kit.py` | 缓存的行浅蓝，内存绿色，命中绿色，内存读写（经 `clflush` 访问宿主的内存）红色。地址的三段 tag 紫、组号蓝、行内偏移橙。`cache_funcs.py` 另外生成三种情况的调用路径图，路径上的函数与箭头保持原色、箭头加粗，其余部分的不透明度降到 20%（深色图中为 36%）。`cache_chain.py` 画出框架中访存的调用链：灰底的两块是硬件（`frame/cpu/`、实验一的解释器、实验二的缓存）与软件（`frame/os/`），每个文件是一条白色的带，左边或上方写它的路径，你实现的文件边框深色（`实验一`、`TODO` 两个标签），`cache.c` 的边框最粗，箭头上的「每页一次」「每行一次」标出拆分的位置，读写内存的调用红色，两条红色箭头从 `cache.c` 一带的同一处分出（读与写都可能写回脏行、读入新行） |
