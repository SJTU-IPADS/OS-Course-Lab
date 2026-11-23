"""Handson-Lab KVCache：页面函数。

kvcache 实验的说明，两个部分：KV Cache 的作用，KV Cache 的结构与访问过程。
页面的顺序与 lecture.py 相同，函数名把下划线换成连字符就是页面的 id。辅助函数与常量以 _kv_ 开头。
下面的一段注释写明讲义的设计，每个部分另以一段注释开头。
"""


# ==============================================================================
# mini-ollama 的 KV Cache
# ==============================================================================
#
# 本讲义回答一个问题：mini-ollama 怎样为一段多轮对话保存键与值，kvcache.c 中的函数各在何时
# 被调用、算什么、改什么。compute 已给出，它是写好的例子，供理解流程，它调用由你实现的函数。
# 其余 5 个函数由你实现：kv_block_bytes、kv_key、kv_value、kv_append、kv_reserve。
#
# 各页脱离代码也能读懂：学生手里没有仓库，也没有运行过程序。一个例子在页上给全它的场景（之前的状态、
# 这一轮的消息、其他数字所依据的数），选项按含义写（「每轮最多生成 8 个 token」），实测在页上写明测了什么。
# 页面与讲稿不引用实验的文件、make 目标、命令行选项与某次运行的轮次编号。保留的是学生实现的函数名，
# 以及动画中在页上解释了的调用。
#
# 回答分两个部分，每个部分以只有标题的衔接页开始，各部分的做法写在它开头的注释中。
#
# 页序：多轮对话 → 一层中的注意力（公式）→ 之前 token 的键与值（动画：不使用与使用 KV Cache）→
# KV Cache 在内存中的结构（2 页：结构、一行的位置）→ 第一轮对话的写入（动画，15 帧）。
# 2026-11-23 本讲义由 tut-quiz 的第三部分拆出，原来的前两个小节成为两个部分。原来第二小节的最后一页
# （下一次请求的查找与继续计算）与第三小节（内存上限与失效）同时删去，代码、图与译文在
# build/removed-tut-quiz-kv-next-limit/。
# 同日实验改为只有一段多轮对话，KV Cache 是一段连续的内存，块 tok 存放第 tok 个 token，学生实现的函数
# 是 5 个。各页随之改写：「会话与多轮对话」（kv-sessions）一页改为「多轮对话」（kv-turns），结构的两页与
# 动画改为连续的内存，动画由 16 帧减为 15 帧。改写前的目录在 build/scratch-kvdeck3/before/，kv-sessions
# 的脚本与图在 build/removed-tut-kvcache-lab/。
# 第二小节原来的两页（框架与 kvcache.c、一轮对话中的调用）于 2026-11-22 删去，代码、图与译文在
# build/removed-tut-quiz-kv-sec2/。
# make run 的演示一页与 make test 的检查一页于 2026-11-20 删去，代码与图在 build/removed-tut-quiz-kv/。
#
# 材料：实验目录 labs/kvcache（README.md、include/kvcache.h、src/kvcache.c 的注释、frame/main.c、
# frame/kvmem.c、tests/），助教目录 labs/kvcache-ta/README.md。页面与讲稿不含参考答案的代码。
#
# 一层中的注意力一页只有公式。
# 图由 diagrams/kv_*.py 生成（共用 kv_kit.py）。配色：已缓存的行与块浅蓝，这一步写入或改变的橙色，读取的
# 绿色，重复的计算红色，预留的块白底蓝色虚线，未使用的块灰色，输入向量灰色，本步定位或要找的格保持原色、
# 加深色框。kvcache.c 中你实现的函数是
# 深色圆角标签，已给出的 compute 是灰色圆角标签，框架的函数是白底灰框。
#
# 第二部分的三页共用 diagrams/kv_example_kit.py。动画每帧各区域位置固定：serve 的步骤、调用与实参及算出
# 和返回的值、对话的 token、左边的 struct kv_cache 与右边的内存、一句说明。token 画在它的块的正上方。块一律
# 写作「块 n」，token 一律写作 t 加编号，地址一律写作 base 之后的字节数。例子的数字与独立的重放
# （build/scratch-kvdeck3/tools/check_numbers.py）逐项相同。


def _kv_slide(p, md, **kw):
    """页面文案。粗体在文案里逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def _kv_figure(p, name, width):
    """配图 assets/<name>.svg，由 diagrams/ 下的同名脚本生成。"""
    return p.image(f"assets/{name}.svg", width_px=width)


_KV_LAB = "/labs/kvcache"


def _kv_file(path):
    """实验目录中一个文件的按钮，显示文件名，path 相对于实验目录。"""
    return f"[{path.rsplit('/', 1)[-1]}](file:{_KV_LAB}/{path})"


# ==============================================================================
# 第一部分：KV Cache 的作用
# ==============================================================================
#
# 一段对话有 5 轮，每轮把消息与回答接在对话的文本之后。回答的每个 token 经过模型的 28 层，第 pos 个
# token 的注意力公式用到第 0 到 pos 个 token 的键与值，每个 token 在每层有一行键与一行值。不使用
# KV Cache 时每一步重新计算之前的行，KV Cache 直接读取它们。
# 这一部分只讲做法，kvcache.c 的函数在第二部分出现。
# 页序：多轮对话 → 一层中的注意力（公式）→ 之前 token 的键与值（动画）。

# 本讲义的第一页，先看程序的输入：一段对话的 5 轮，每轮的消息与回答接在对话的文本之后，所以每轮的
# 文本包含之前各轮的文本。回答逐个 token 生成，由此引出下一页：一个 token 经过模型时要用到哪些数据。
def kv_turns(p):
    p.title("多轮对话")
    _kv_slide(p, """
假如一段对话有 5 轮，用户每轮发来一条消息，消息与回答依次接在对话的文本之后。回答逐 token 生成，每轮最多生成 8 个 token。
""")
    _kv_figure(p, "kv-turns", 979)
    p.highlight("生成一个 token 时，模型的每一层要用到哪些数据？")
    p.notes("""
图的左边是这段对话的 5 轮，圆圈中的数字是轮次，后面是用户这一轮的消息。
一轮分三步。第一步把消息按模型的对话格式接在对话已有的文本之后。第二步计算文本中还没有进入 KV Cache 的 token。第三步逐个生成回答，每一步取分数最高的 token，直到模型给出结束符或回答达到 8 个 token。回答也接在文本之后，下一轮从这里继续。
图的右边每一行是一轮结束时对话的文本，一格是一个 token。浅灰色的条是之前各轮的文本，深灰色是这一轮的消息，包括对话格式的标记，比如标出用户发言开始与结束的 token，白色是这一轮的回答。格的上方是这一轮「消息 + 回答」的 token 数，比如第 1 轮是 13 个 token 的消息与 8 个 token 的回答，第 2 轮 14 个 token 的消息接在这 21 个 token 之后。图中的数是实验所用的千问模型回答这段对话时得到的，5 轮的回答都达到了 8 个 token，5 轮结束时文本共有 111 个 token。
回答的每一个 token 都要经过模型的全部 28 层，下一页看一个 token 在一层中用到哪些数据。
""")


# 上一页问生成一个 token 时每一层要用到哪些数据。本页用一个头的注意力公式回答：权重用到第 0 到 pos 个
# token 的键，输出再用到它们的值，所以需要之前 token 的键与值。每个 token 在每层有一行键与一行值，这一对应
# 写在讲稿中。之前 token 的键与值从哪里来，留给下一页。
# 公式行首的 {} 要保留：aligned 把紧随其后的 [...] 当作可选参数，等号左边会消失。
def kv_step(p):
    p.title("一层中的注意力")
    p.valign("middle")
    _kv_slide(p, r"""
第 pos 个 token 在每一个注意力头中算出自己的查询 $q_{\mathrm{pos}}$、键 $k_{\mathrm{pos}}$ 与值 $v_{\mathrm{pos}}$。一个注意力头的权重 $a$ 与输出 $o_{\mathrm{pos}}$ 如下，$d$ 是这个注意力头的维度。

 $$
 \begin{aligned}
 {}[a_0,\ldots,a_{\mathrm{pos}}] &= \mathrm{softmax}\Big(\tfrac{1}{\sqrt{d}}\,[\,\underbrace{q_{\mathrm{pos}}\cdot k_0,\ \ldots,\ q_{\mathrm{pos}}\cdot k_{\mathrm{pos}-1}}_{\text{之前 token 的键}},\ q_{\mathrm{pos}}\cdot k_{\mathrm{pos}}\,]\Big)\\
 o_{\mathrm{pos}} &= \underbrace{a_0 v_0+\cdots+a_{\mathrm{pos}-1}v_{\mathrm{pos}-1}}_{\text{之前 token 的值}}+a_{\mathrm{pos}}v_{\mathrm{pos}}
 \end{aligned}
 $$
""")
    p.highlight("计算第 pos 个 token 的注意力，需要第 0 到 pos−1 个 token 的键与值")
    p.notes("""
第 pos 个 token 经过模型时，先换成它的向量，这个向量依次经过 28 层，每一层先做注意力，再做前馈计算。由最后一层之后的向量得到分数最高的 token。
公式是一层中一个头的注意力计算，真实模型中 d = 128。一层由它的输入算出第 pos 个 token 的查询，16 个头，每头 128 个 float，再算出它的键与值，各 8 个头、每头 128 个 float，即各 1024 个 float，这是它在这一层的一行键与一行值。第 j 个 token 在这一层的一行键与一行值称为这一层的第 j 行，每个 token 在每一层都有这样两行。
16 个查询头按编号顺序每两个共用一个键值头，比如第 0、1 个查询头都使用第 0 个键值头。公式中的 q_pos 是一个查询头的 128 个数，k_j 与 v_j 是第 j 行键与第 j 行值中属于对应键值头的 128 个数。
第一个式子对 j = 0 到 pos 计算 q_pos · k_j / √d，softmax 在这 pos+1 个数上归一化，得到权重 a_0 到 a_pos。softmax 的分母涉及全部的键，所以每一个权重都依赖之前 token 的键，包括 a_pos。
第二个式子把 v_0 到 v_pos 按权重相加，得到这个头的输出 o_pos，共 128 个 float。16 个头的输出依次排成 2048 个 float，经过一次线性变换之后加到这个 token 的向量上。
第 0 到 pos−1 行属于之前的 token。一个 token 在一层的输入只取决于它与它之前的 token，所以它的键与值在之后的每一步都与第一次算出时相同。
下一页比较两种做法：每一步重新计算这些行，或者把它们保存下来直接读取。
""")

# 上一页用公式指出注意力需要之前 token 的键与值。本页用两帧图比较两种做法：不使用 KV Cache 时每一步
# 重新计算它们，KV Cache 从内存中直接读取。kvcache.c 的函数留给下一部分，图中不标出。
def kv_reuse(p):
    p.title("之前 token 的键与值")
    _kv_slide(p, """
不使用 KV Cache 时，每一步重新计算之前全部 token 的键与值。
KV Cache 把它们保存在内存中，每一步只计算一行。
""")
    p.frames("assets/kv-reuse-1.svg", "assets/kv-reuse-2.svg", width_px=1000)
    p.notes("""
共 2 帧，图中编号为 j 的一列是第 j 个 token，这一列中键与值的两格是它在这一层的第 j 行键与第 j 行值。橙色的一列是正在计算的新 token，即第 pos 个。
第 1 帧不使用 KV Cache。程序保存每个 token 在每一层的输入向量，每一步在每一层由它们重新计算第 0 到 pos−1 个 token 的键与值，然后才做新 token 的注意力。红色是这些重复的计算。第 pos 步每层重新计算 pos 行，对话越长，每一步重新计算得越多。
第 2 帧是 KV Cache。蓝色虚线框是 KV Cache 中保存的、之前 token 在这一层的键与值，框中只表示有哪些行。注意力计算把新 token 的键与值写入第 pos 行，再读取第 0 到 pos 行，绿色是直接读取的之前各行。28 层都写完之后，这个 token 进入缓存，之后的每一步都直接读取它的行。
两种做法中每个 token 经过相同的运算，次序也相同：缓存中的一行就是这个 token 第一次计算时得到的键或值。所以两种做法的回答逐字节相同，这也是 KV Cache 是否正确的判据。
接下来先看这些键与值怎样保存在内存中，再看一轮对话怎样写入与读取它们。
""")


# ==============================================================================
# 第二部分：KV Cache 的结构与访问过程
# ==============================================================================
#
# 课上讲过为什么缓存 K 与 V，怎样存是新内容，所以先给结构，再给函数。顺序是在内存中怎么放
# （struct kv_cache 的字段、一段依次相接的等大的块、一块中的四行）、怎么找到一行（由 token 的编号算出
# 块的起点，再加块内偏移）、第一轮对话怎么写入（动画）。三页用同一个小例子：2 层，kv_dim 为 10，一块
# 160 字节，内存 8 块，max_answer 为 2，第 1 轮的消息是 t0 t1 t2。结构的两页取 t2 算完、t3 未算的时刻，
# 已缓存、预留、未使用三种块都有，动画从空的 KV Cache 走完第 1 轮。
# kv_dim 取 10，使一行的 float 数不与 token 数、块数混淆。块 tok 存放第 tok 个 token，所以 kv_reserve
# 要数出多少个 token 会作为输入经过模型：动画用具体的 token 数出 4，在三帧标出刚输出或只作为输出的 token
# （数块的一帧、t3 刚被输出的一帧、一轮结束的一帧），页面与讲稿不给一般算式，它由学生自己算出。
# 内存在这一部分总是放得下。放不下时 kv_reserve 返回 0，这条规则只在数块一帧的比较与讲稿中交代。
# 页序：KV Cache 在内存中的结构（结构、一行的位置）→ 第一轮对话的写入（动画）。

# 上一页说明 KV Cache 把之前 token 的键与值保存在内存中。本页回答怎样保存：struct kv_cache 的六个字段，
# 一段依次相接的等大的块，块 tok 存放第 tok 个 token，一块中只有 float。取第 1 轮 t2 算完、t3 未算的时刻，
# 已缓存、预留、未使用三种块都有。先只讲结构，图中不出现函数。下一页在这个时刻找一行。
def kv_layout(p):
    p.title("KV Cache 在内存中的结构")
    _kv_slide(p, f"""
实验中千问模型有 28 层，每层注意力 k、v 向量均为 1024 维，KVCache 结构上与下图相同。
""")
    _kv_figure(p, "kv-layout", 1120)
    p.notes("""
KV Cache 要为对话的每个 token、每一层保存一行键与一行值。例子中有 2 层、1 个键值头，这个头的长度 d = 10，所以 kv_dim = 10，一行是 10 个 float。真实模型有 28 层、8 个键值头，每头 128 个数，一行是 1024 个 float，一块是 224 KiB，结构相同。
左边是 struct kv_cache，它描述整个 KV Cache。n_layer、kv_dim、base 与 n_blocks 由框架设置：base 是这段内存第一个字节的地址，n_blocks 是其中的块数。n_cached 是已缓存的 token 数，capacity 是已预留的块数，这两个字段由你实现的函数维护。
右边是 KV Cache 的内存。框架在程序开始时按内存上限一次分配这段内存，把它分成等大的块，上限中整块的数目就是 n_blocks，例子中是 8 块。一块存放一个 token 在全部层中的键与值，各块按 token 的编号依次相接，所以块 tok 存放第 tok 个 token，从 base 之后 tok × 160 字节处开始。块名下面一行是这个起点，比如块 2 从 base + 320 开始。
图中是这样一个时刻：第 1 轮的消息是 t0、t1、t2，这一轮为 4 个 token 预留了块 0 到 3，现在 t0、t1、t2 已算完，t3 还没有算。三种块都在图中。块 0 到 n_cached − 1 已缓存，即块 0、1、2，其中是算好的键与值。块 n_cached 到 capacity − 1 是预留的块，即块 3，内容还没有意义。从块 capacity 起的块还没有使用，即块 4 到 7。
下方放大块 2：4 行依次是第 0 层的键、第 0 层的值、第 1 层的键、第 1 层的值，每行 10 个 float，共 40 个 float、160 字节，各行左边的数是它在块内的字节偏移。块内只有 float，块的编号就是它存放的 token 的编号。
每一行的地址由 token 的编号、层与 K、V 算出，下一页看怎样计算。
""")


# 上一页给出结构。本页在同一时刻由（层、K 或 V、token 编号）分两步算出一行的地址：由 token 的编号算出
# 它的块的起点，即 base 之后 tok × 160 字节，再加块内偏移，偏移由 float 数换成字节数。例子取第 1 层 token 2
# 的值，地址是 base + 440。kv_block_bytes 是一块的长度。只给数值演算。下一页动画中各帧的地址都按本页的方法得到。
def kv_locate(p):
    p.title("KV Cache 在内存中的结构")
    _kv_slide(p, f"""
你需要实现 `kv_key` 与 `kv_value`：先算出块的起点，再加块内偏移。{_kv_file("src/kvcache.c")}
""")
    _kv_figure(p, "kv-locate", 1048)
    p.notes("""
找一行需要三项：哪一层、K 或 V、第几个 token。图中要找第 1 层 token 2 的值，调用是 kv_value(cache, 1, 2)。
第 1 步找到块。各块依次相接，块 2 之前有块 0 与块 1，所以块 2 从 base 之后 2 × 160 = 320 字节处开始。160 是一块的字节数，即 kv_block_bytes(cache) 的值：2 层 × 2 行 × 10 个 float × 4 字节。base 是一个字节的地址，在它上面加的是字节数。
第 2 步加块内偏移。第 1 层之前有第 0 层的一行键与一行值，共 1 × 2 × 10 = 20 个 float。这一层的值又在这一层的键之后，再加 10 个，共 30 个 float。一个 float 是 4 字节，所以块内偏移是 120 字节，v2 是图中编号 30 到 39 的 10 个 float。
两步相加，v2 的地址是 base + 320 + 120 = base + 440。kv_key(cache, 1, 2) 返回同一块中的 base + 400。两个函数只计算地址，内存中的内容与 struct kv_cache 都保持原样。块内偏移只取决于层与 K、V，在所有块中相同。
下一页从空的 KV Cache 开始，看第 1 轮对话怎样调用这些函数并写入内存。
""")


# 前两页给出结构与一行的位置。本页用动画从空的 KV Cache 走完第 1 轮：kv_reserve 数出会作为输入的
# 4 个 token，把 capacity 改为 4，已给出的 compute 在每层用 kv_key、kv_value 取得地址，attention 写第 pos
# 个 token 的行、读第 0 到 pos 个 token 的行，全部层之后 kv_append。第一个 token 分层讲清，第二个 token
# 展示读取之前的行与注意力公式，其余的 compute 各合并成一帧。结束时 t4 只作为输出，它的键与值在下一轮算出。
# 每帧给出调用者与实参、算出与返回的值、读写的格以及 n_cached、capacity 的变化，符号与 kv_step 相同。
def kv_first(p):
    p.title("第一轮对话的写入")
    _kv_slide(p, """
对话第 1 轮的消息有 3 个 token，最多生成 2 个 token，KV Cache 从空开始。
""")
    p.frames(*(f"assets/kv-first-{k}.svg" for k in range(1, 16)), width_px=1120)
    p.notes("""
共 15 帧。最上面一行是框架的 serve 处理一轮对话的步骤，有橙色框的是当前一步。下面的框是这一步的调用与实参、算出和返回的值。再下面是对话的 token，每个 token 在它的块的正上方。左边是 struct kv_cache，右边是 KV Cache 的内存。橙色是写入或改变，绿色是读取，深色框是本步定位的格，比如取得地址的一行。例子中只画一个头，d = 10。token 左边一行给出这一轮的两个数：n_prompt 是文本的 token 数，max_answer 是最多生成的 token 数。地址写作 base 之后的字节数。
第 1 帧：第 1 轮开始。append_message 把消息转换为 t0、t1、t2，n_prompt = 3。KV Cache 为空，n_cached 与 capacity 都是 0，8 块都未使用。
第 2 帧：框架调用 kv_reserve(cache, 3, 2)，两个数是 n_prompt 与 max_answer：文本有 3 个 token，最多生成 2 个。kv_reserve 要先数出这一轮有多少个 token 会作为输入经过模型，每个这样的 token 要一块。在这个例子中它们是消息的 t0、t1、t2 与回答的第一个 token t3，共 4 个。t3 要作为输入，才能得到回答的第二个 token t4。t4 生成之后本轮就结束，它只作为输出，所以需要的块数是 4。capacity 是 0，小于 4，要预留。4 不大于 n_blocks，放得下。需要的块数大于 n_blocks 时，这一轮放不下，kv_reserve 返回 0，各字段保持原样，程序报告并停止。
第 3 帧：kv_reserve 把 capacity 改为 4，返回 1。块 0 到 3 成为预留，块 n 留给第 n 个 token。n_cached 仍是 0，内存中的内容不变：块 0 到 3 本来就在内存中，kv_reserve 只改 capacity 一个字段。
第 4 帧：框架从第 n_cached 个 token 起，逐个调用 compute。compute 已经给出，它是一个写好的例子，你可以读它来理解一个 token 的计算流程：它调用由你实现的 kv_key、kv_value 与 kv_append，以及框架的 embed、attention 与 feed_forward。compute(cache, t0, state) 把调用时的 n_cached = 0 作为 pos，先调用 embed(t0, state)，得到 t0 的向量。
第 5 帧：第 0 层。compute 调用 kv_key(cache, 0, 0) 与 kv_value(cache, 0, 0)：块 0 从 base + 0 开始，再分别加 0 与 40 字节。两个地址收进 keys、values 两个指针数组的第 0 项。这一步只计算地址。
第 6 帧：compute 调用 attention(0, 0, state, keys, values)。attention 算出 k0、v0，写入 keys[0] 与 values[0] 指向的两行，再读这两行计算注意力。只有一个 token，所以 a0 = 1，o0 = a0 v0。o0 经过一次线性变换后加到 state 上，feed_forward 再更新 state。
第 7 帧：第 1 层同样，仍是块 0，偏移换成 80 与 120 字节。一个 token 在全部层中的键与值都在它的一块中。
第 8 帧：两层都完成之后，compute 调用一次 kv_append(cache)，n_cached 从 0 变为 1，块 0 从预留变为已缓存。kv_append 只修改这一个字段。compute 返回时，state 是 t0 经过最后一层之后的向量。
第 9 帧：t1，pos = 1。第 0 层收集两项：keys 是 base + 0 与 base + 160，values 是 base + 40 与 base + 200。attention 先把 k1、v1 写入最后一项指向的块 1。
第 10 帧：attention 读两项。attn_scores 计算 q1·k0 与 q1·k1，除以 √d，softmax 得到 a0、a1。attn_values 计算 o1 = a0 v0 + a1 v1，与注意力公式中 pos = 1 的情况相同，o1 经过一次线性变换后加到 state 上。k0、v0 直接取自块 0。
第 11 帧：第 1 层同样，keys 是 base + 80 与 base + 240，values 是 base + 120 与 base + 280。两层完成后 kv_append 把 n_cached 变为 2。
第 12 帧：t2 重复同样的过程：每层收集 3 项，写块 2，读块 0、1、2，最后 n_cached 变为 3。消息已全部计算。
第 13 帧：next_token 由 t2 最后一层之后的 state 选出回答的第一个 token t3。t3 是模型的输出，块 3 仍是预留。t3 作为输入再经过一次模型，才有 k3、v3。
第 14 帧：框架对 t3 调用 compute，k3、v3 写入块 3，n_cached 变为 4，等于 capacity，预留的块都已缓存。
第 15 帧：next_token 输出 t4，回答达到 2 个 token，本轮结束。文本有 5 个 token，n_cached 是 4，块 4 仍未使用：t4 只作为输出，在下一轮作为输入时，才算出它的键与值。
每次调用 compute 时，pos 都等于 n_cached，并且小于 capacity：kv_reserve 已经为这一轮会作为输入的每个 token 预留了一块。
""")
