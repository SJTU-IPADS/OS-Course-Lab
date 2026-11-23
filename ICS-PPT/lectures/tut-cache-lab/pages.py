"""MiniCPU Lab2: Cache：页面函数。

mini-cpu 实验二（组相联缓存）的说明，三个部分：访存路径，cache.c 的函数与调用者，缓存的结构与访问过程。
页面的顺序与 lecture.py 相同，函数名把下划线换成连字符就是页面的 id。辅助函数与常量以 _cache_ 开头。
下面的一段注释写明讲义的设计，每个部分另以一段注释开头。
"""


# ==============================================================================
# mini-cpu 实验二的组相联缓存。
#
# 每页一张为本实验画的图，正文一两句或几条短句，讲解写在 p.notes 中。页面不给出答案代码。
#
# 讲稿中数字的来源（2026-11-23 按分成 frame/cpu/ 与 frame/os/ 的框架重录）：把 ../../labs/mini-cpu
# 复制到 build/scratch-cachedeck/mini-cpu，src/lab1/ 换成 labs/mini-cpu-ta/lab1/ 的参考实现，
# src/lab2/cache.c 换成 labs/mini-cpu-ta/lab2/cache.c，make（LAB=2）。计数录自这个副本的 --stats，
# 命令的写法与学生在 labs/mini-cpu 中运行的写法相同（程序的路径在客户机的栈上，影响计数）。
# 2026-11-18 在 build/scratch-tutquiz-cache/ 下按旧框架录得的数字与此相同。
# 本讲义没有演示：有缓存与没有缓存的计数一页、运行与评测一小节（框架调用 cache_init 与 cache_flush 的
# 时机、命中与内存读写、计数的检查）于 2026-11-20 删去，代码与图在 build/removed-tut-quiz-stats/ 与
# build/removed-tut-quiz-run/。2026-11-23 本讲义由 tut-quiz 的第二部分拆出，原来的三个小节成为三个部分。
#
# 插图由 diagrams/cache_*.py 生成，脚本与图同名（下划线对应连字符），共用 diagrams/cache_kit.py
# 的画法：CPU 白底深色框，缓存的行浅蓝，内存绿色，命中绿色，缺失时的内存读写红色。
# 地址的三段 tag 紫、组号蓝、行内偏移橙。你实现的函数是深色胶囊，已写出的 cache_init 是灰色胶囊，
# 框架的函数是白底灰框。
# cache_chain.py 画出框架中访存的调用链：灰色的两块是硬件（frame/cpu/、解释器与缓存）与软件
# （frame/os/），每个文件是一条白色的带，你实现的文件边框深色，cache.c 的边框最粗。
# cache_funcs.py 另外生成 cache-funcs-hit、cache-funcs-miss、cache-funcs-evict 三张图：一次行访问调用的
# 函数与箭头保持原色，箭头加粗，其余部分的不透明度降到 20%。

# 设计。
#
# 起点问题：实验一的机器用 dram_ptr 直接读写映射到进程内存的 dram.img，访存经过宿主 CPU 的缓存。
# 实验二起内存只能经 dram.c 一次读写一行，每读写一行都用 clflush 把这一行移出宿主 CPU 的缓存，读写的
# 是宿主的内存。
# 缓存怎样减少内存读写，你实现的函数在其中各做什么？
# 回答：框架把缓存与内存分在两处：缓存的行是进程中的数组 cache_lines，通常留在宿主 CPU 的缓存中，
# 内存是映射到进程中的 dram.img。cache.c 只经 dram_line_read 与 dram_line_write 读写内存，每读写一行
# 都访问宿主的内存。有缓存时绝大多数行访问是不读写内存的命中。缓存按
# 地址的组号只在一组中查找，组内按 LRU 替换，脏行在离开缓存时写回，这些步骤就是 cache.c 中的十个函数。
# 访存在到达缓存之前已经按行拆开：指令的访存由 CPU 的访存单元（frame/cpu/mem.c）拆开，系统调用复制的
# 缓冲区由软件（frame/os/copy.c）切分，cache_read 与 cache_write 每次只处理一行。
# 页序：实验一的访存路径 → 实验二的访存路径（文件、函数与边界）→ cache.c 中函数的调用关系 →
# 框架中访存的调用链（从实验一的 interp_run 调用的 rm_get 到内存，硬件与软件）→ mem_access 对访存的拆分
# （frame/cpu/mem.c）→ 内存地址的拆分 → 缓存的组与路 → 命中时调用的函数 → 命中（动画）→
# 缺失时调用的函数 → 替换脏行时调用的函数 → 缺失并写回脏行（动画）。
# 部分：访存路径（前两页，问题与机制）→ cache.c 的函数与调用者（调用关系，框架中访存的调用链，
# 调用 cache_read 与 cache_write 的 mem_access）→ 缓存的结构与访问过程（按 cache.c 中函数的顺序：地址、
# 组与路、命中、缺失，命中与缺失各先用调用关系的图标出调用的函数，再用动画逐步走一遍）。
# 每个部分以只有标题的衔接页开始。


# The loop of mem_access in frame/cpu/mem.c (cache-walk shows it), one level of indentation
# removed. The whole function does not fit the slide: its head, the check of nbytes before the
# loop and the return after it are left out, and the two lines of a failed translation are one
# line "...". The notes say what was left out.
_CACHE_MEM_ACCESS = """while (nbytes) {
    unsigned in_page = PAGE_SIZE - (unsigned)(va % PAGE_SIZE);
    unsigned chunk = in_page < nbytes ? in_page : nbytes;
    uint64_t pa;
    if (mmu_translate(va, acc, &pa) < 0) {
        ...
    }
    for (unsigned done = 0; done < chunk;) {
        unsigned in_line = LINE_SIZE - (unsigned)((pa + done) % LINE_SIZE);
        unsigned count = in_line < chunk - done ? in_line : chunk - done;
        if (acc == MEM_WRITE)
            cache_write(pa + done, buf + done, count);
        else
            cache_read(pa + done, buf + done, count);
        done += count;
    }
    va += chunk; buf += chunk; nbytes -= chunk;
}"""


def _cache_slide(p, md, **kw):
    """页面文案。粗体逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def _cache_figure(p, name, width):
    return p.image(f"assets/{name}.svg", width_px=width)


# ==============================================================================
# 第一部分：访存路径
# ==============================================================================
#
# 问题与机制：实验一的机器直接读写映射，访存经过宿主 CPU 的缓存。实验二的机器经 cache.c 访存，
# cache.c 只经 frame/cpu/dram.c 读写内存。页序：实验一的访存路径 → 实验二的访存路径。

# 本讲义的第一页：从实验一已经写好的机器出发，看访存经过哪些文件：frame/cpu/nocache.c 直接读写映射。
# 本页与下一页的图画布相同、各部分的位置相同，都放在标题下方。两页中间都有虚线框「宿主 CPU 的缓存」：
# 本页 nocache.c 经 dram_ptr 的读写穿过它，读写过的行由 CPU 硬件自动缓存；换页时中间一块换成缓存，
# 虚线框中换成组 0 到组 63，dram_ptr 的箭头打叉，出现经 dram.c 访问物理内存的路径。两页不提系统调用，
# 课上还没有讲。第二部分的调用链按实验二 README 的访存路径画出系统调用读写缓冲区的一支（sys_dispatch、
# guest_read），讲稿不讲系统调用怎样进入 sys_dispatch，这一支是否保留待定（PENDING）。
def cache_lab1(p):
    p.title("实验一的访存路径")
    _cache_figure(p, "cache-path-lab1", 970)
    _cache_slide(p, """
实验一的 `nocache.c` 用 `dram_ptr` 直接读写映射，读写过的行由 CPU 硬件自动缓存。
""")
    p.notes("""
图中是一次访存经过的文件，从左上角的解释器开始。实验一的解释器 interp.c 取指、读数据、写数据，经 frame/cpu/x86.c 分别调用 frame/cpu/mem.c 的 fetch_bytes、mem_read 与 mem_write。mem.c 是 CPU 的访存单元，它求出物理地址 pa，在行的边界处拆开，每一行调用一次 cache_read 或 cache_write：取指与读数据调用 cache_read，写数据调用 cache_write。
实验一的机器中，cache_read 与 cache_write 来自 frame/cpu/nocache.c：它们用 dram_ptr(pa) 得到这个地址在映射中的指针，再用 memcpy 复制要读写的字节。这些读写与进程中的任何访存一样经过宿主 CPU 的缓存：读写过的行由 CPU 硬件自动保存在缓存中，再次访问时命中，程序与操作系统都不参与。图中的虚线框是宿主 CPU 的缓存，框中绿色的格子是映射中被缓存的行。
灰色的底是 mini-cpu 进程。图的右边是客户机的物理内存，即 64 MiB 的文件 dram.img，mini-cpu 启动时用 mmap 把它映射到进程的内存中，映射中的字节就是文件中的字节。
实验二把中间一块换成缓存，虚线框中换成缓存的行，缓存读写内存时绕过宿主 CPU 的缓存。下一页是这时访存的路径。
""")


# 上一页的机器访存经过宿主 CPU 的缓存。本页给出实验二的机器：cache.c 取代 nocache.c，dram_ptr 的箭头
# 打叉，缓存经 dram_line_read 与 dram_line_write 读写映射，并用 clflush 绕过宿主 CPU 的缓存。本页说明哪些是
# 框架的文件、哪个文件由你实现、两级的数据在哪里。
def cache_path(p):
    p.title("实验二的访存路径")
    _cache_figure(p, "cache-path", 970)
    _cache_slide(p, """
`cache.c` 读写内存只能调用 `dram.c` 的 `dram_line_read` 与 `dram_line_write`。
""")
    p.notes("""
实验二中，cache_read 与 cache_write 换成 src/lab2/cache.c 中由你实现的函数。左边两个文件不变：解释器照样调用 mem.c，mem.c 照样调用 cache_read 与 cache_write，每次调用的字节都在一行之内。
frame/main.c 调用另外两个函数：cache_init 建立缓存，已经写出，cache_flush 把脏行写回内存。
缓存的 64 × 4 行在 cache.c 的数组 cache_lines 中，这个数组只有约 22 KiB，读写它通常在宿主 CPU 的缓存中命中，图中的虚线框表示这一点。
右边的映射仍然存在，框架装载程序时用 dram_ptr 读写它。cache.c 不能使用 dram_ptr，图中这两个箭头打了叉。
cache.c 读写内存只能调用 frame/cpu/dram.c 的 dram_line_read 与 dram_line_write，每次读写整整一行 64 字节。两个函数都在映射中复制这一行，并用 clflush 指令把这一行移出宿主 CPU 的缓存。dram_line_read 先移出再读，读到的是宿主内存中的字节。dram_line_write 先写再移出，写入的字节到达宿主的内存。图中红色的两条路径就是这两种内存读写，每一次都访问物理内存。
关闭缓存时，框架把每次行访问转给 dram.c，每一行都读写一次内存。接下来看 cache.c 中有哪些函数，框架从哪里调用它们。
""")


# ==============================================================================
# 第二部分：cache.c 的函数与调用者
# ==============================================================================
#
# cache.c 中函数的调用关系，框架中一次访存到达缓存的调用链，以及调用 cache_read 与 cache_write 的
# mem_access。调用链把实验一的解释器、CPU 的访存单元、缓存与内存连起来，并说明访存在到达缓存之前
# 已经按行拆开。页序：cache.c 中函数的调用关系 → 框架中访存的调用链 → mem_access 对访存的拆分。

# 上一页给出 cache.c 在访存路径中的位置。本页给出 cache.c 中十一个函数的调用关系。
# 下一页把框架对 cache_read 与 cache_write 的调用放回从解释器到内存的调用链。之后各页按文件中的顺序说明
# 这些函数，命中与缺失的动画之前各用这张图标出实际调用的函数。
def cache_funcs(p):
    p.title("cache.c 中函数的调用关系")
    _cache_slide(p, """
框架仅调用 `cache_init` `cache_flush` `cache_read` `cache_write` 四个函数。
""")
    _cache_figure(p, "cache-funcs", 1080)
    p.notes("""
深色的胶囊是你实现的十个函数，灰色的 cache_init 已经写出，作为使用 cache_lines、cache_set_count、cache_way_count 的例子。白框是框架的函数。
框架只调用四个函数：frame/main.c 调用 cache_init 与 cache_flush，frame/cpu/mem.c 调用 cache_read 与 cache_write。
cache_read 与 cache_write 每次收到一行之内的字节，各调用一次 cache_access 找到这一行。cache_access 是读与写共用的一次行访问：用 cache_set_index 与 cache_tag 求出组号与 tag，用 cache_lookup 在这一组中查找。缺失时用 cache_victim 选出被替换的路，再用 cache_fill 写回脏行、读入新行。
cache_fill 与 cache_flush 都要把一行写回它在内存中的位置，行中只保存了 tag，位置由 cache_line_address 从 tag 与组号求出。只有这两个函数读写内存。红色的箭头通向 dram_line_read 与 dram_line_write，每一次调用读写一行，访问宿主的内存。
下一页看一次访存从解释器出发，经过框架中的哪些函数到达 cache_read 与 cache_write。
""")


# 上一页的图中 frame/cpu/mem.c 调用 cache_read 与 cache_write。本页把这次调用放回从解释器到内存的整条调用链：
# 实验一的 interp_run 读内存操作数时调用 rm_get，rm_get 经 load、mem_read 到达 CPU 的访存单元 mem_access，
# mem_access 每页调用一次 mmu_translate，每行调用一次 cache_read 或 cache_write，缓存缺失时经 frame/cpu/dram.c
# 读写内存。图分硬件（frame/cpu/、解释器、缓存）与软件（frame/os/）两块：系统调用 write 的 guest_read 按行切分
# 缓冲区，同样经 mem_read 进入访存单元。于是 cache_read 与 cache_write 每次只收到一行之内的字节。
# 下一页给出 mem_access 的代码：它怎样按页、按行拆开一次访存。
def cache_chain(p):
    p.title("框架中访存的调用链")
    _cache_slide(p, """
实验一的 `interp_run` 调用的 `rm_get` 经 `load`、`mem_read` 到达 `frame/cpu/mem.c` 的 `mem_access`，再由它每行调用一次 `cache_read`。
""")
    _cache_figure(p, "cache-chain", 1120)
    p.notes("""
图中是一次访存从解释器到内存经过的函数，从上到下每一条带是一个文件。左边灰色的一块是硬件：frame/cpu/ 中的文件模拟机器的硬件，实验一的解释器与实验二的缓存也属于硬件。右边灰色的一块是软件：frame/os/ 中的代码是在这台机器上运行的软件。深色的胶囊是你实现的函数，白框是框架的函数。
第一条带：实验一的 interp_run 每一轮执行一条指令。它调用 frame/cpu/x86.c 的 x86_begin 取指，用 rm_get 读操作数，用 store 写内存，比如 push 把寄存器写到栈上。
第二条带：rm_get 的操作数在内存中时调用 load。x86_begin、store 与 load 分别调用 frame/cpu/mem.c 的 fetch_bytes、mem_write 与 mem_read，参数是程序中使用的地址 va，即虚拟地址。解释器读写内存操作数、push、pop、call、ret 都经过 load 与 store。
第三条带：fetch_bytes、mem_write 与 mem_read 都调用 mem_access，它是 CPU 的访存单元。mem_access 每页调用一次 frame/cpu/pagemap.c 的 mmu_translate，把 va 换成物理地址 pa，再在这一页之内按行的边界拆开，每一行调用一次 cache_read 或 cache_write。
第四条带：实验二的 cache_read 与 cache_write 在这一级起作用。每次调用收到的字节都在一行之内，比如从行内偏移 60 处读 8 个字节是两次调用。实验一的机器中这一级是 frame/cpu/nocache.c，它直接读写映射。
第五条带：缓存缺失时调用 frame/cpu/dram.c 的 dram_line_read 读入一行，替换脏行时调用 dram_line_write 写回一行，每一次都访问宿主的内存。
右边是系统调用读写程序的缓冲区，以 write 为例：frame/os/sysbase.c 的 sys_dispatch 用 frame/os/copy.c 的 guest_read 读出程序的缓冲区。缓冲区可以长达 4096 字节，guest_read 像 memcpy 的循环一样在行的边界处切分，每一段调用一次 mem_read，之后的路径与指令的访存相同。系统调用把结果写进程序的缓冲区时用 guest_write，每一段调用一次 mem_write。
所以 cache_read 与 cache_write 每次只处理一行之内的字节。下一页看 mem_access 怎样按页、按行拆开一次访存。
""")


# 上一页的调用链中，取指、读写数据与系统调用的复制都经 frame/cpu/mem.c 的 mem_access 到达缓存。本页给出
# mem_access 的循环：外层每页用 mmu_translate 求出物理地址，内层在页内按行调用 cache_read 或
# cache_write，正文说明内层的 count 为什么让每次调用都在一行之内。整个函数在页面的高度中放不下，只列出
# 循环（18 行，最长 75 列），讲稿写明略去的函数头、循环前后的代码与换算失败时的两行。之后进入缓存的结构，
# 从地址的拆分开始。
def cache_walk(p):
    p.title("`mem_access` 对访存的拆分")
    _cache_slide(p, """
`frame/cpu/mem.c` 中 `count` 不超过到行末的字节数 `in_line`，每次调用都在一行之内。
""")
    p.code("c", _CACHE_MEM_ACCESS)
    p.notes("""
代码是 mem_access 的循环：外层每页翻译一次地址，内层每行调用一次 cache_read 或 cache_write。
mem_access 是 mem.c 中的静态函数，fetch_bytes、mem_read 与 mem_write 都调用它，acc 分别是 MEM_FETCH、MEM_READ 与 MEM_WRITE。程序运行时，只有 mem_access 调用 cache_read 与 cache_write。页面上略去了函数头、循环之后的 return 0 与循环之前的长度检查：一次访存最多 LINE_SIZE 即 64 个字节，更长的一次访存是框架的错误，程序以状态 4 退出，所以一次访存最多跨两行。
外层的 while 每一轮处理一页之内的部分。chunk 是 va 所在的页中剩下的字节数，最多为 nbytes。mmu_translate 把 va 换成物理地址 pa，实验二中这一步由框架的 frame/cpu/pagemap.c 完成。两个相邻的虚拟页在物理内存中可以相距很远，所以每页单独换算。写作 ... 的两行在换算失败时记下出错的地址 fault_va 并返回 -1。
内层的 for 每一轮处理一行之内的部分。in_line 是 pa + done 所在的行中剩下的字节数，count 取 in_line 与 chunk - done 中较小的一个，chunk - done 是这次访存在这一页中还剩的字节数。每一轮调用一次 cache_read 或 cache_write，所以每次调用的字节都在一行之内，长度为 1 到 64 字节，一次调用就是一次行访问。跨两行的一次访存因此是两次行访问，计两次命中或缺失，比如从行内偏移 60 处读 8 个字节是两次 cache_read，各读 4 个字节。取指一次读最多 15 个字节，跨两行的情况经常出现，这也是行访问次数多于指令数的原因之一。
框架用 frame/memtrace.c 检查每一次调用，收到跨行的调用时打印 framework error 并以状态 4 退出。关闭缓存时，它把这两个调用转给 frame/cpu/dram.c 的 uncached_read 与 uncached_write，每次调用读写一次内存。
装载程序时框架用 dram_ptr 直接写入 dram.img，这些写入绕过 mem_access 与缓存。
接下来从地址的划分开始，按文件中的顺序说明 cache.c 中的函数。
""")


# ==============================================================================
# 第三部分：缓存的结构与访问过程
# ==============================================================================
#
# 按 cache.c 中函数的顺序：地址、组与路、命中、缺失。命中与缺失各先用调用关系的图标出调用的
# 函数，再用动画逐步走一遍。
# 页序：内存地址的拆分 → 缓存的组与路 → 命中时调用的函数 → 命中（动画）→ 缺失时调用的函数 →
# 替换脏行时调用的函数 → 缺失并写回脏行（动画）。

# 上一页说明 cache_read 与 cache_write 收到的是物理地址，cache_access 先由它求组号与 tag。
# 本页说明物理地址怎样分成三段，
# 以及 cache_set_index、cache_tag、cache_line_address 三个函数分别求出什么。
def cache_split(p):
    p.title("内存地址的拆分")
    p.valign("middle")
    _cache_slide(p, """
实验中规定每个缓存行的容量是 64 字节，因此行内偏移占用低 6 位。组数由输入参数决定，当一共 64 组时，组号占用 6 位，即第 6 到 11 位，而 tag 则是第 12 位及以上。
""")
    _cache_figure(p, "cache-split", 1040)
    p.notes("""
一行 64 字节，地址除以 64 的余数是行内偏移，商是行号，即内存中的第几行。行号除以组数的余数是组号，这一行只能放在这一组中。商是 tag，区分放在同一组中的不同行。
图中的例子是 pa = 0x12345，行内偏移 5，行号 0x48d，组号 13，tag 0x12。组数是 2 的幂时，余数与商就是地址中的位段：64 组时组号是第 6 到 11 位，tag 是第 12 位及以上。% 与 / 的写法对任意组数成立，比如 3 组的缓存。
cache_set_index 与 cache_tag 求出这两个字段。行内偏移不需要单独的函数，cache_read 与 cache_write 用 pa % 64 求出它，决定从行中的第几个字节开始复制。
反过来，由 tag 与组号可以求出行的第一个字节的地址：(0x12 × 64 + 13) × 64 = 0x12340，即 pa 去掉行内偏移。一行中只保存 tag，写回脏行时要知道它在内存中的位置，cache_line_address 求出这个地址，cache_fill 与 cache_flush 都用它。
下一页看组号选中的那一组在缓存中的位置。
""")


# 上一页求出组号与 tag。本页给出组号选中的位置：64 组 × 4 路的行、一行的字段、
# 数组 cache_lines 中的下标，cache_init 建立它，cache_lookup 只在一组中比较。
def cache_grid(p):
    p.title("缓存的组与路")
    p.valign("middle")
    _cache_slide(p, """
`cache_lookup` 只比较一组中所有缓存行的 `valid` 与 `tag`，返回其中匹配的一行。
""")
    _cache_figure(p, "cache-grid", 1040)
    p.notes("""
默认的缓存有 64 组，每组 4 路，共 256 行，数据 16 KiB。图左每一格是一个 cache_line，右上是它的五个字段。valid 为 1 表示这一行保存了内存中某一行的副本。dirty 为 1 表示副本读入之后被写过，内存中的内容较旧。tag 是副本所在地址的 tag。last_use 是最近一次访问时 cache_use_clock 的值。data 是 64 字节的内容。
全部的行放在一个数组 cache_lines 中，一组的各路相邻：组 13 的 way 1 是 cache_lines[13 * 4 + 1]，即 cache_lines[53]。cache_init 已经写出，它用 calloc 分配 cache_set_count × cache_way_count 行，全部清零，所以每一行开始时 valid 为 0。
cache_lookup(set_index, tag) 只看这一组的 4 路：valid 为 1 且 tag 相等的一路就是要找的行，找不到返回 NULL。valid 为 0 的路不能匹配，因为 cache_init 之后每一路的 tag 都是 0，而 tag 0 是真实存在的 tag。
cache_lookup 只负责查找，命中或缺失之后做什么由 cache_access 决定。下一页看命中时调用哪些函数。
""")


# 上一页说明命中或缺失之后做什么由 cache_access 决定。本页用函数调用关系的图只突出命中时 cache_read 的
# 一次行访问调用的函数，其余变灰：cache_lookup 找到这一行，路径不经过 dram.c。下一页用动画走一遍这条路径。
def cache_funcs_hit(p):
    p.title("缓存命中时的 `cache_read`")
    _cache_slide(p, """
命中时 `cache_lookup` 找到这一行，`cache_read` 只从缓存的行中复制。
""")
    _cache_figure(p, "cache-funcs-hit", 1080)
    p.notes("""
图是 cache.c 中函数的调用关系，只突出 cache_read 的一次行访问命中时调用的函数与箭头，其余部分是灰色。
frame/cpu/mem.c 调用 cache_read，cache_read 调用一次 cache_access。cache_access 用 cache_set_index 与 cache_tag 求出组号与 tag，再用 cache_lookup 在这一组中查找，找到 valid 为 1 且 tag 相等的一路。cache_access 把 cache_hits 加 1，把这一行的 last_use 设为 cache_use_clock 加 1 之后的值，返回这一行。cache_read 从行中复制要读的字节。
整条路径只读写数组 cache_lines。cache_write 命中时的路径相同，复制的方向相反，并把这一行的 dirty 设为 1。绝大多数行访问走这条路径，比如一个程序给 128 × 128 的 int 矩阵（64 KiB）赋值，再按行求和 8 遍，在 64 组、每组 4 路的 16 KiB 缓存中，它的行访问（含取指）有 99.43% 命中。
下一页用一个地址逐步走一遍命中。
""")


# 上一页给出命中时调用的函数。本页用动画在组与路中走一遍命中：cache_access 求组号与 tag，cache_lookup 找到 way 1，
# 计数器与 last_use 的变化，cache_read 从行中复制。
def cache_hit(p):
    p.title("缓存命中时的 `cache_read`")
    p.valign("middle")
    _cache_slide(p, """
`cache_read(0x12345, dst, 4)` 读一行中的 4 个字节，调用一次 `cache_access(0x12345)`。
""")
    p.frames(*(f"assets/cache-hit-{k}.svg" for k in (1, 2, 3)), width_px=1040)
    p.notes("""
组 13 的四路都有效，tag 分别是 0x05、0x12、0x07、0x2a，way 3 是脏行。这是为说明过程设的状态。
第 1 步：cache_set_index 与 cache_tag 求出组号 13 与 tag 0x12，只在组 13 中查找。
第 2 步：cache_lookup(13, 0x12) 依次比较四路，way 1 有效且 tag 相等，返回 way 1。
第 3 步：命中。cache_access 给 cache_hits 加 1，cache_use_clock 加 1 变为 60，把 60 写入 way 1 的 last_use，返回 way 1。cache_read 从 data[5] 起复制 4 个字节到 dst。这次行访问只读写缓存的行。
last_use 记录的是最近一次访问的先后：way 1 现在最大，它是这一组中最近使用的行。下一页看缺失时调用哪些函数。
""")


# 上一页的命中不经过 dram.c。本页是缺失、被替换的路无效或不脏：路径上多出 cache_victim 与
# cache_fill，cache_fill 用 dram_line_read 读入一行。
def cache_funcs_miss(p):
    p.title("缓存缺失时的 `cache_read`")
    _cache_slide(p, """
缺失时 `cache_victim` 选出被替换的路，`cache_fill` 用 `dram_line_read` 读入一行。
""")
    _cache_figure(p, "cache-funcs-miss", 1080)
    p.notes("""
缺失时 cache_lookup 返回 NULL。cache_access 把 cache_misses 加 1，调用 cache_victim 在这一组中选出被替换的路：有 valid 为 0 的路时选它，否则选 last_use 最小的一路，即最久没有访问的一路。
cache_fill 用 cache_line_address 由新的 tag 与组号求出这一行在内存中的地址，调用 dram_line_read 读入 64 字节，再把 valid 设为 1、dirty 设为 0、tag 设为新的 tag。之后的步骤与命中相同：cache_access 更新 last_use 并返回这一行，cache_read 从中复制。
图中被替换的路无效或没有被写过，cache_fill 直接覆盖它，dram_line_write 是灰色的。这一次行访问读写一次内存。
cache_write 缺失时经过同样的步骤。写只改行中的几个字节，其余字节必须保持内存中的值，所以写缺失时 cache_access 先读入整行，这就是写分配。之后 cache_write 把字节写入这一行并置 dirty 为 1，写入的内容留在缓存中，行离开缓存或 cache_flush 时才写回内存。
下一页是被替换的行是脏行时的路径。
""")


# 上一页被替换的路不脏，cache_fill 直接覆盖它。本页是被替换的行是脏行：cache_fill 先用
# dram_line_write 写回它，再读入新行，一次行访问读写两次内存。
def cache_funcs_evict(p):
    p.title("缓存替换脏行时的 `cache_read`")
    _cache_slide(p, """
被替换的行是脏行时，`cache_fill` 先用 `dram_line_write` 写回它，再读入新行。
""")
    _cache_figure(p, "cache-funcs-evict", 1080)
    p.notes("""
一行被 cache_write 写过之后 dirty 为 1，内存中这一行的内容较旧。这样的行被 cache_victim 选中时，它的内容不能直接丢弃。
cache_fill 先用 cache_line_address 由这一行自己的 tag 与组号求出它在内存中的地址，调用 dram_line_write 写回 64 字节，把 cache_writebacks 加 1，再按缺失的步骤求出新行的地址、调用 dram_line_read 读入。两个地址的组号相同，tag 不同。
图中 cache_fill 调用了全部三个函数。这一次行访问读写两次内存，是三种情况中最慢的一种。
图左的 cache_flush 也调用 dram_line_write，它把仍在缓存中的脏行全部写回，框架在程序结束后调用它。
下一页接着命中的例子读另一个地址，逐步走一遍缺失并写回脏行。
""")


# 上一页给出替换脏行时调用的函数。本页接着命中的例子读另一个地址，用动画走一遍缺失：查找失败，cache_victim 按 last_use 选出 way 3，
# cache_fill 先写回脏行、再读入新行，两次内存读写都由 cache_line_address 求出地址。
def cache_miss(p):
    p.title("缓存替换脏行时的 `cache_read`")
    p.valign("middle")
    _cache_slide(p, """
接着读 `0x13345`：组号同为 13，tag 为 0x13，组 13 中没有这一行。
""")
    p.frames(*(f"assets/cache-miss-{k}.svg" for k in range(1, 6)), width_px=1040)
    p.notes("""
第 1 步：cache_lookup(13, 0x13) 比较四路，没有一路的 tag 是 0x13，返回 NULL。cache_access 给 cache_misses 加 1。
第 2 步：cache_victim(13) 选出被替换的路。组中有无效的路时选编号最小的无效路。四路都有效时选 last_use 最小的一路，即最近最少使用的行。这里 last_use 为 41、60、50、33，选 way 3。cache_victim 只负责选择，计数器由 cache_access 与 cache_fill 修改。
第 3 步：cache_fill 处理缺失。way 3 是脏行，它的内容比内存新，先用 dram_line_write 写回。行中只有 tag 0x2a，地址由 cache_line_address(0x2a, 13) 求出：(0x2a × 64 + 13) × 64 = 0x2a340。cache_writebacks 加 1，这是一次内存读写。被替换的行不脏时直接丢弃。
第 4 步：cache_fill 用 dram_line_read 从 cache_line_address(0x13, 13) = 0x13340 读入整行，再置 valid = 1、dirty = 0、tag = 0x13。这是第二次内存读写。
第 5 步：回到 cache_access，cache_use_clock 加 1 变为 61，写入 way 3 的 last_use，返回 way 3。cache_read 从 data[5] 起复制。这次行访问共读写两次内存。
""")

