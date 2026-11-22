"""Quiz 与实验说明：页面函数。

三个部分：Quiz 的题面（第 1 ~ 5 题），mini-cpu 实验二的组相联缓存，kvcache 实验的 KV Cache。
页面的顺序与 lecture.py 相同，函数名把下划线换成连字符就是页面的 id。各部分的辅助函数与常量
分别以 _quiz_、_cache_、_kv_ 开头。每个部分以一段注释开头，写明它的设计。
"""


# 第一部分：Quiz 题目。
#
# 本部分是 Quiz 第 1 ~ 5 题的题面。题号、分值、文字、代码、表与图照原卷，不加提示，不放答案。
# 例外是第 4 题（2）第 1 问之后的 q4-rob-answer（2026-11-19 按讲师的要求加入）：答案中各指令进入、
# 执行、完成与提交的周期，换成周期 1 ~ 9 结束时 ROB 中的条目与状态。
#
# 设计：一道题可以占几页，每页放一个小问或几个相互引用的小问。小问用到的代码、寄存器与内存的值、
# 表、规则、假设与图尽量放在这一页，后一问引用前一问时带上前一问的题面，讲一个小问时尽量不翻页。
# 重复的材料按小问用到的部分裁剪：表只留小问用到的行，规则与假设只留小问用到的条目，句子整句保留。
# 第 1、2、3 题的各页标题是题目的标题。第 4、5 题各是一个小节，小节名是题目的标题，各页标题是所在
# 部分的标题。同一标题的相邻页在大纲中合为一行。
#
# 尺寸由主题决定：17 行的清单高约 510 px，表的一行约 50 px，正文一行约 37 px，标题之下可用约 580 px。
# 每页的 p.gap(16, min_px=4) 让排满的页把块间距收到 4 px。下列几页放不下小问用到的全部材料，
# 缺的材料在同一题的另一页：
# - q1-push：第 2 问说「表中哪 8 个字节」，栈上内存的表在下一页 q1-ret。改变的字节与 %rsp
#   的新值由本页的 %rsp、%rbx 求出，清单占去左半页，内存表放不下。
# - q1-input：题干首句说输入字符串作为参数 line 传入 phase_quiz，这句在 q1-code。本页有
#   phase_quiz 的声明，加上这句就超出页面。
# - q2-gets 与 q2-frame：第 2 题第 1 问的材料超过一页，Gets 的源代码在前一页，chat_turn 的
#   反汇编在后一页，两页都带第 1 问。
# - q2-rop：第 4 问覆盖返回地址前的填充字节数由第 1 问从 chat_turn 的反汇编求出，chat_turn 的
#   反汇编在 q2-frame 与 q2-nx。
# - q2-canary：题干首句说每轮对话调用一次 chat_turn，这句在 q2-gets。
# - q4-rob-rules 与 q4-rob-q1：第 4 题（2）第 1 问的材料超过一页，规则在前一页，起始条件、
#   假设与指令表在后一页，两页都带第 1 问。
# - q5-cpu-simd：带第 2 问，第 2 问所说的这些向量元素与这些指令指第 1 问求出的量，第 1 问在
#   q5-cpu-count 与 q5-cpu-bound。这些量由本页的题干求出，再放第 1 问就超出页面。
# - q5-gpu-batch：第 4 问用到第 1、2 问求出的 GPU 块数与每块的时间，这两问在 q5-gpu-pipeline。
#   这两个量由本页的题干与图求出，本页带第 3 问，给出周期的定义。


# ---------------------------------------------------------------- 第 1 题

_QUIZ_Q1_TITLE = "第 1 题：拆炸弹（20 分）"

_QUIZ_Q1_INTRO = ("某程序读入一行字符串，将其作为参数 `line` 传入函数 `phase_quiz` 进行检查。"
                  "涉及到两个 C 函数的声明如下：")

_QUIZ_Q1_DECL = """void phase_quiz(const char *line, int key);
void explode_bomb(void);"""

_QUIZ_Q1_GOAL = ("你的目标是找出一个输入字符串，使得 `phase_quiz` 正常返回，不触及 `explode_bomb` 函数。"
                 "该程序的部分反汇编代码如下。")

# objdump keeps a tab after the address; the deck shows it as objdump prints it.
_QUIZ_Q1_ASM = "\n".join([
    "0000000000401901 <phase_quiz>:",
    "  401901:\tpush   %rbx",
    "  401902:\tlea    0x878cd(%rip),%rdx",
    "  401909:\tmov    $0x0,%ebx",
    "  40190e:\tmovzbl (%rdx,%rbx,1),%eax",
    "  401912:\ttest   %al,%al",
    "  401914:\tje     401928 <phase_quiz+0x27>",
    "  401916:\tadd    %esi,%eax",
    "  401918:\tmovsbl (%rdi,%rbx,1),%r8d",
    "  40191d:\tcmp    %eax,%r8d",
    "  401920:\tjne    401930 <phase_quiz+0x2f>",
    "  401922:\tadd    $0x1,%rbx",
    "  401926:\tjmp    40190e <phase_quiz+0xd>",
    "  401928:\tcmpb   $0x0,(%rdi,%rbx,1)",
    "  40192c:\tjne    401930 <phase_quiz+0x2f>",
    "  40192e:\tpop    %rbx",
    "  40192f:\tret",
    "  401930:\tcall   401885 <explode_bomb>",
])

_QUIZ_Q1_REG_CAPTION = "在即将执行 `phase_quiz` 函数的第一行汇编时，部分 CPU 寄存器的值如下。"

# The full tables of the quiz. Each page shows the rows its questions use.
_QUIZ_Q1_REGS = {
    "rip": ["`%rip`", "`0x401901`"],
    "rsp": ["`%rsp`", "`0x7fffffffcaf8`"],
    "rdi": ["`%rdi`", "`0x4b2ac0`"],
    "rsi": ["`%rsi`", "`0x4`"],
    "rax": ["`%rax`", "`0x5`"],
    "rbx": ["`%rbx`", "`0x4b2ac0`"],
    "rdx": ["`%rdx`", "`0x0`"],
    "r8": ["`%r8`", "`0x4ba776`"],
}

_QUIZ_Q1_MEM_CAPTION = "同一时刻内存中的相关数据如下。"

_QUIZ_Q1_MEM = {
    "caf0": ["`0x7fffffffcaf0` ~ `0x7fffffffcaf7`", "`0xb0 0xcb 0xff 0xff 0xff 0x7f 0x00 0x00`"],
    "caf8": ["`0x7fffffffcaf8` ~ `0x7fffffffcaff`", "`0xee 0x18 0x40 0x00 0x00 0x00 0x00 0x00`"],
    "cb00": ["`0x7fffffffcb00` ~ `0x7fffffffcb07`", "`0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00`"],
    "91c0": ["`0x4891c0` ~ `0x4891c7`", "`0x25 0x64 0x20 0x25 0x64 0x00 0x65 0x72`"],
    "91c8": ["`0x4891c8` ~ `0x4891cf`", "`0x72 0x6f 0x72 0x00 0x00 0x00 0x00 0x6f`"],
    "91d0": ["`0x4891d0` ~ `0x4891d7`", "`0x70 0x5d 0x5f 0x67 0x00 0x00 0x62 0x6e`"],
    "91d8": ["`0x4891d8` ~ `0x4891df`", "`0x5d 0x69 0x61 0x00 0x00 0x00 0x00 0x00`"],
}

_QUIZ_Q1_ASCII = "ASCII 中 `'a'` ~ `'z'` 的编码是 `0x61` ~ `0x7a`，依字母顺序连续。"

_QUIZ_Q1_ASKS = {
    1: "1. （4 分）`phase_quiz` 的第 1、2 个参数各在哪个寄存器中？`0x401902` 处的 `lea` 指令"
       "执行完成后 `%rdx` 的值是多少？提示：`lea` 指令 `%rip` 相对寻址的基址是下一条指令的地址。",
    2: "2. （4 分）`0x401901` 处的 `push %rbx` 指令和 `0x40192e` 处的 `pop %rbx` 指令各完成什么操作？"
       "`push %rbx` 执行后，表中哪 8 个字节的内容改变，变为什么，`%rsp` 变为多少？"
       "`phase_quiz` 为什么需要这两条指令？",
    3: "3. （4 分）`phase_quiz` 正常返回时，`ret` 执行结束后，`%rip` 和 `%rsp` 的值各是多少？",
    4: "4. （8 分）给出能够使得 `phase_quiz` 正常返回的输入字符串。",
}


def _quiz_page(p, title):
    """Every quiz page: its title, and seams that close up when the page is full."""
    p.title(title)
    p.gap(16, min_px=4)


def _quiz_text(p, text):
    """Statement text as the quiz writes it: no automatic bold."""
    return p.slide(text, autobold=False)


def _quiz_asks(p, asks, numbers):
    _quiz_text(p, "\n".join(asks[n] for n in numbers))


def _quiz_q1_regs(p, names):
    _quiz_text(p, _QUIZ_Q1_REG_CAPTION)
    p.table([_QUIZ_Q1_REGS[n] for n in names], headers=["寄存器", "值"])


def _quiz_q1_mem(p, names, widths=None):
    _quiz_text(p, _QUIZ_Q1_MEM_CAPTION)
    p.table([_QUIZ_Q1_MEM[n] for n in names], headers=["地址", "内容"], widths=widths)


# 题干与第 1 问。第 1 问用到两个函数的声明、lea 一行与下一条指令的地址。
# 清单在左，题干、声明与问题在右。
def q1_code(p):
    _quiz_page(p, _QUIZ_Q1_TITLE)
    with p.flex("row", widths=[69, 68]):
        p.code("assembly", _QUIZ_Q1_ASM)
        with p.flex("column"):
            _quiz_text(p, _QUIZ_Q1_INTRO)
            p.code("c", _QUIZ_Q1_DECL)
            _quiz_text(p, _QUIZ_Q1_GOAL)
            _quiz_text(p, "回答下列问题。")
            _quiz_asks(p, _QUIZ_Q1_ASKS, [1])


# 第 2 问。push 写栈要看 %rsp 与 %rbx，问 phase_quiz 为什么需要这两条指令要看清单中 %rbx 的用法。
# 清单在左，寄存器与问题在右。栈上内存的表放不下，在下一页。
def q1_push(p):
    _quiz_page(p, _QUIZ_Q1_TITLE)
    with p.flex("row"):
        p.code("assembly", _QUIZ_Q1_ASM)
        with p.flex("column"):
            _quiz_q1_regs(p, ["rsp", "rbx"])
            _quiz_asks(p, _QUIZ_Q1_ASKS, [2])


# 第 3 问。ret 之后的 %rip 与 %rsp 要看 %rsp、清单中成对的 push 与 pop、push 写入的 8 个字节
# 与其上的返回地址。清单在左，%rsp 与这两行内存在右，问题在两栏之下占满一行，与其他页一样
# 位于页面底部。问题放在右栏末尾时要折成两行，贴着页面底边（y 约 688）。寄存器只有一行，
# 表放在说明的右边，与 q1-input 相同。%rsp 的值较宽：表占右栏的 3/5，两列按 9:16 分，
# 表头「寄存器」不折行，值不超出表线。说明折成 5 行，表下的空处放内存的说明，内存表紧接其下。
def q1_ret(p):
    _quiz_page(p, _QUIZ_Q1_TITLE)
    with p.flex("row"):
        p.code("assembly", _QUIZ_Q1_ASM)
        with p.flex("column"):
            with p.flex("row", widths=[2, 3]):
                _quiz_text(p, _QUIZ_Q1_REG_CAPTION)
                with p.flex("column"):
                    p.table([_QUIZ_Q1_REGS["rsp"]], headers=["寄存器", "值"], widths=[9, 16])
                    _quiz_text(p, _QUIZ_Q1_MEM_CAPTION)
            p.table([_QUIZ_Q1_MEM[n] for n in ["caf0", "caf8"]], headers=["地址", "内容"])
    _quiz_asks(p, _QUIZ_Q1_ASKS, [3])


# 第 4 问。比较循环要看整段清单、phase_quiz 的声明、key 所在的 %rsi、lea 算出的地址处的字节
# 与 ASCII 编码。清单与声明在左，寄存器、内存、ASCII 与问题在右，内存只留存放字符串的两行。
def q1_input(p):
    _quiz_page(p, _QUIZ_Q1_TITLE)
    with p.flex("row"):
        with p.flex("column"):
            p.code("assembly", _QUIZ_Q1_ASM)
            p.code("c", _QUIZ_Q1_DECL.splitlines()[0])
        with p.flex("column"):
            with p.flex("row", widths=[3, 2]):
                _quiz_text(p, _QUIZ_Q1_REG_CAPTION)
                p.table([_QUIZ_Q1_REGS["rsi"]], headers=["寄存器", "值"])
            _quiz_q1_mem(p, ["91d0", "91d8"], widths=[1, 1])
            _quiz_text(p, _QUIZ_Q1_ASCII)
            _quiz_asks(p, _QUIZ_Q1_ASKS, [4])


# ---------------------------------------------------------------- 第 2 题

_QUIZ_Q2_TITLE = "第 2 题：ROP 攻击（20 分）"

_QUIZ_Q2_INTRO = ("某大模型聊天程序每轮对话调用一次 `chat_turn()` 函数进行处理，其中 `Gets` 函数将终端里的输入"
                  "读入栈上的缓冲区。`chat_turn` 的声明和 `Gets` 的源代码如下：")

_QUIZ_Q2_C = """void chat_turn(void);

char *Gets(char *buf)
{
    int ch;
    char *cursor = buf;

    while ((ch = getchar()) != EOF && ch != '\\n')
        *cursor++ = (char)ch;
    *cursor = '\\0';
    return buf;
}"""

_QUIZ_Q2_GOAL = ("你攻击的目标是将控制流转移到 `report_pass(unsigned long password)` 函数，"
                 "并以正确的参数调用它。本题中，栈不可执行。")

_QUIZ_Q2_ASM_INTRO = "`chat_turn` 和 `report_pass` 的反汇编分别如下。"

_QUIZ_Q2_CHAT_ASM = "\n".join([
    "000000000040186d <chat_turn>:",
    "  40186d:\t55                   \tpush   %rbp",
    "  40186e:\t48 89 e5             \tmov    %rsp,%rbp",
    "  401871:\t48 81 ec 80 00 00 00 \tsub    $0x80,%rsp",
    "  401875:\t48 8d 45 80          \tlea    -0x80(%rbp),%rax",
    "  401879:\t48 89 c7             \tmov    %rax,%rdi",
    "  40187c:\te8 a4 ff ff ff       \tcall   401825 <Gets>",
    "  401881:\t48 8d 45 80          \tlea    -0x80(%rbp),%rax",
    "  401885:\t48 89 c7             \tmov    %rax,%rdi",
    "  401888:\tb8 00 00 00 00       \tmov    $0x0,%eax",
    "  40188d:\te8 6e 37 00 00       \tcall   405000 <printf>",
    "  401892:\t90                   \tnop",
    "  401893:\tc9                   \tleave",
    "  401894:\tc3                   \tret",
])

_QUIZ_Q2_REPORT_ASM = "\n".join([
    "0000000000500000 <report_pass>:",
    "  500000:\t55                   \tpush   %rbp",
    "  500001:\t48 89 e5             \tmov    %rsp,%rbp",
    "  500004:\t48 83 ec 10          \tsub    $0x10,%rsp",
    "  500008:\t48 89 7d f8          \tmov    %rdi,-0x8(%rbp)",
    "  50000c:\t48 b8 5f c3 e8 d1 72 \tmovabs $0x4f3a9c72d1e8c35f,%rax",
    "  500013:\t9c 3a 4f ",
    "  500016:\t48 39 45 f8          \tcmp    %rax,-0x8(%rbp)",
    "  50001a:\t75 0a                \tjne    500026 <report_pass+0x26>",
    "  50001c:\tbf 00 00 00 00       \tmov    $0x0,%edi",
    "  500021:\te8 ea 4c f0 ff       \tcall   404d10 <exit>",
    "  500026:\tbf 01 00 00 00       \tmov    $0x1,%edi",
    "  50002b:\te8 e0 4c f0 ff       \tcall   404d10 <exit>",
])

_QUIZ_Q2_CODE_HEAD = "以下指令的机器码供参考："

_QUIZ_Q2_CODE_HEADERS = ["指令", "`movabs $0x1122334455667788, %rdi`", "`pop %rdi`", "`ret`"]

_QUIZ_Q2_CODE_ROWS = [["机器码", "`48 bf 88 77 66 55 44 33 22 11`", "`5f`", "`c3`"]]

_QUIZ_Q2_MOVABS = "`movabs` 指令的立即数是 8 字节。"

_QUIZ_Q2_ASKS = {
    1: "1. （2 分）`Gets` 函数输入多少字节后，下一个字节开始覆盖 `chat_turn` 的返回地址？",
    2: "2. （2 分）调用 `report_pass` 后如果程序 `exit(0)` 正常退出，寄存器 `%rdi` 中的值应为多少？",
    3: "3. （4 分）如果栈可执行，可以把设置参数并调用 `report_pass` 的机器指令写入到缓冲区，"
       "再溢出把返回地址覆盖为这段指令的地址。本题中栈不可执行，如果继续这样做，会发生什么？",
    4: "4. （6 分）给出能够使得程序控制流正确转移到 `report_pass` 并正常返回的一行输入。"
       "格式：按从低地址到高地址的顺序，写出输入的所有字节，以两位十六进制的字节序列表示，"
       "同时需要简单注明每段的作用。如出现大量重复字节，可用 `...` 表示，并注明字符的重复次数。",
    5: "5. （6 分）如果 `chat_turn` 开启栈保护，栈帧中加入金丝雀值，程序每次重新启动之后金丝雀的值"
       "都发生变化，但程序同一次运行过程中多次调用 `chat_turn` 时金丝雀的值保持不变。攻击者在仅可以读取"
       "程序的输出，不预先知道金丝雀值的情况下，怎样才能让 `chat_turn` 通过金丝雀检查并执行 ROP 链？",
}

_QUIZ_Q2_ASK5_HINT = ("提示：`printf` 支持 `%n$p` 这种按位置打印参数的写法，表示以指针形式打印第 n 个参数，"
                      "如 `%6$p` 打印第 6 个参数。而 x86-64 的 Linux 操作系统上，第 6 个及后面的参数通过栈传递，"
                      "位置在调用对应函数时压入的返回地址上方。")


# 题干与第 1 问之一。Gets 把输入字符依次写入 buf 并在末尾补 '\0'，chat_turn 的反汇编在下一页，
# 下一页同样带第 1 问。代码在左，题干在右，问题在下。
def q2_gets(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[11, 10]):
        p.code("c", _QUIZ_Q2_C)
        with p.flex("column"):
            _quiz_text(p, _QUIZ_Q2_INTRO)
            _quiz_text(p, _QUIZ_Q2_GOAL)
            _quiz_text(p, "请回答下列问题。")
    _quiz_asks(p, _QUIZ_Q2_ASKS, [1])


# 第 1 问之二。buf 在栈帧中的偏移由 chat_turn 的反汇编给出，Gets 的源代码在上一页。
# 清单在左，反汇编说明在右，问题在下。
def q2_frame(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[2, 1]):
        p.code("assembly", _QUIZ_Q2_CHAT_ASM)
        _quiz_text(p, _QUIZ_Q2_ASM_INTRO)
    _quiz_asks(p, _QUIZ_Q2_ASKS, [1])


# 第 2 问。正常退出时 %rdi 的值由 report_pass 的比较指令给出。清单在左，攻击目标在右，问题在下。
def q2_pass(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[3, 1]):
        p.code("assembly", _QUIZ_Q2_REPORT_ASM)
        _quiz_text(p, _QUIZ_Q2_GOAL)
    _quiz_asks(p, _QUIZ_Q2_ASKS, [2])


# 第 3 问。覆盖返回地址后 chat_turn 的 ret 跳转到栈上的 buf。清单在左，栈不可执行的条件与问题在右。
def q2_nx(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[9, 4]):
        p.code("assembly", _QUIZ_Q2_CHAT_ASM)
        with p.flex("column"):
            _quiz_text(p, _QUIZ_Q2_GOAL)
            _quiz_asks(p, _QUIZ_Q2_ASKS, [3])


# 第 4 问。构造 ROP 输入要在 report_pass 的机器码中找 pop %rdi ; ret。覆盖返回地址前的填充字节数
# 由第 1 问求出。清单、机器码表与说明在左，问题在右。
def q2_rop(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[3, 1]):
        with p.flex("column"):
            p.code("assembly", _QUIZ_Q2_REPORT_ASM)
            with p.flex("row", widths=[45, 55]):
                _quiz_text(p, _QUIZ_Q2_CODE_HEAD)
                _quiz_text(p, _QUIZ_Q2_MOVABS)
            p.table(_QUIZ_Q2_CODE_ROWS, headers=_QUIZ_Q2_CODE_HEADERS)
        _quiz_asks(p, _QUIZ_Q2_ASKS, [4])


# 第 5 问。泄漏金丝雀用到每轮对话调用一次 chat_turn，以及 chat_turn 把 buf 作为格式串传给 printf。
# 清单在左，问题在右，提示在下。
def q2_canary(p):
    _quiz_page(p, _QUIZ_Q2_TITLE)
    with p.flex("row", widths=[25, 12]):
        p.code("assembly", _QUIZ_Q2_CHAT_ASM)
        _quiz_asks(p, _QUIZ_Q2_ASKS, [5])
    _quiz_text(p, _QUIZ_Q2_ASK5_HINT)


# ---------------------------------------------------------------- 第 3 题

_QUIZ_Q3_TITLE = "第 3 题：函数调用（10 分）"

_QUIZ_Q3_ASK1 = """1. （4 分）以下 ______ 是软件约定，______ 是硬件约定。（填选项字母）

   A.  函数的返回值放在 `%rax` 中
   B.  `pushq` 指令把 `%rsp` 减 8
   C.  `add` 指令根据运算结果设置 `rflags` 中的标志位
   D.  函数的第一个参数通过 `%rdi` 传递
   E.  `call` 指令把返回地址压入栈
   F.  `%r10`、`%r11` 是 caller-saved 寄存器"""

_QUIZ_Q3_C = """long g(void);

long f(long x)
{
    return g() + x;
}"""

_QUIZ_Q3_ASM = """gcc's output:                         x saved on the stack directly:
f:                                    f:
    pushq   %rbx                          pushq   %rdi      # save x
    movq    %rdi, %rbx                    call    g
    call    g                             popq    %rdi      # restore x
    addq    %rbx, %rax                    addq    %rdi, %rax
    popq    %rbx                          ret
    ret"""


# 第 1 问。六个选项各一行，不需要别的材料。
def q3_convention(p):
    _quiz_page(p, _QUIZ_Q3_TITLE)
    _quiz_text(p, _QUIZ_Q3_ASK1)


# 第 2 问。C 代码在左，两段汇编在右，a、b 两小问在下。
def q3_callee(p):
    _quiz_page(p, _QUIZ_Q3_TITLE)
    with p.flex("row", widths=[24, 76]):
        with p.flex("column"):
            _quiz_text(p, "2. （6 分）函数 `f` 的 C 代码如下。")
            p.code("c", _QUIZ_Q3_C)
        with p.flex("column"):
            _quiz_text(p, "左边是 gcc 编译 `f` 得到的汇编，右边是手写的另一种做法，把 `x` 直接保存在栈上。")
            p.code("assembly", _QUIZ_Q3_ASM)
    _quiz_text(p, """a. （2 分）左边的 `pushq %rbx` 与 `popq %rbx` 两条指令有什么作用？
b. （4 分）右边的指令更少，访存次数相同，都是 1 次写栈、1 次读栈。gcc 为什么不直接把 `x` 存到栈上？举一个左边的做法比右边好的 C 代码例子说明。""")


# ---------------------------------------------------------------- 第 4 题

# The problem's own heading names its section. Each part's heading is the
# title of the part's pages.

_QUIZ_Q4_PIPE_TITLE = "第 4 题（1）：五级流水线模型（11 分）"

_QUIZ_Q4_PIPE_INTRO = ("设想在一个五级流水线 CPU（取指、译码、执行、访存、写回）上执行下面的循环。"
                       "循环体共 8 条指令，某次执行过程中，循环一共执行了 3 次，其中 `jl` 前两次跳转，第三次不跳转。"
                       "假定所有指令在每个阶段都需要且仅需要 1 个周期，流水线中每个阶段的硬件在每个周期里"
                       "最多处理 1 条指令。")

_QUIZ_Q4_LOOP = """.L3:
    movslq  %eax, %r8
    movl    (%rsi,%r8,4), %ecx
    movl    (%rdi,%r8,4), %r10d
    imull   %r10d, %ecx
    addl    %ecx, %r9d
    addl    $1, %eax
    cmpl    %edx, %eax
    jl      .L3"""

_QUIZ_Q4_PIPE_ASKS = {
    1: "1. （5 分）循环体中哪两条指令之间会产生由 data hazard 引起的 bubble？这次执行的 3 次循环中"
       "共有几个这样的 bubble？画出第 1 次循环里刚发生 data hazard 时，五个阶段正在执行的指令，"
       "如果为空则写为 `nop`，并在图上标出此时 CPU 硬件对哪个阶段实施了 stall 或 bubble。",
    2: "2. （3 分）假设分支预测器总是预测 `jl` 不跳转，这次执行的 3 次循环中共有几个由 control hazard "
       "引起的 bubble？",
    3: "3. （3 分）在第 2 问的条件下，这 3 次循环执行的总周期数（从第 1 条指令取指到最后一条 `jl` "
       "完成写回）是多少？",
}

_QUIZ_Q4_ROB_TITLE = "第 4 题（2）：乱序执行与重排序缓冲区（9 分）"

_QUIZ_Q4_ROB_WHY = ("乱序执行的处理器让操作数就绪的指令先执行，但指令的结果必须按程序原顺序依次写入寄存器。"
                    "为此，处理器在内部设置了一个**重排序缓冲区**（reorder buffer，ROB）。")

_QUIZ_Q4_ROB_WHAT = ("ROB 内部有固定数量的条目，每个条目保存一条已译码的指令，并记录这条指令、它的目的寄存器、"
                     "执行结果和状态。执行状态一共有三种：未执行、正在执行、已完成。")

_QUIZ_Q4_ROB_FIGURE = "下图是一个有 4 个条目的 ROB 在某一时刻的内容。"

_QUIZ_Q4_ROB_RULES_HEAD = "ROB 按以下规则工作："

_QUIZ_Q4_ROB_RULES = {
    "进入": "- **进入**：指令译码后按程序原顺序依次进入 ROB，位置排在所有已有条目之后，状态标记为未执行。"
          "每个周期最多进入 1 条指令，当 ROB 没有空闲条目时，指令需要等到有空闲条目后才能够进入。",
    "执行": "- **执行**：进入 ROB 的指令从下一个周期起，只要操作数就绪就能够开始执行。一条指令在周期 t "
          "执行完成，用到它的结果的指令最早在周期 t + 1 开始执行，这个结果从 ROB 中取得。",
    "完成": "- **完成**：指令执行完成后，把结果写入自己的条目，状态改为已完成，暂不修改寄存器。",
    "提交": "- **提交**：从 ROB 中顺序最靠前的条目开始，按程序顺序提交。最靠前的条目为已完成时，"
          "把结果写入寄存器，然后离开 ROB。在周期 t 完成的指令最早在周期 t + 1 提交，同时，在周期 t "
          "因提交而空出的条目，从周期 t + 1 起才能被新的指令使用。",
}

_QUIZ_Q4_ROB_ASSUME_HEAD = "为了方便分析计算，我们有如下假设："

_QUIZ_Q4_ROB_ASSUME = {
    1: "1. 执行部件的数量足够多，不存在操作数就绪的指令无法执行的情况。",
    2: "2. 每个周期提交的指令条数不受限制，在周期 t，从最靠前的条目起，连续的、在周期 t − 1 "
       "或更早执行完成的条目全部提交。",
    3: "3. 写内存的指令也在提交时才修改内存。",
    4: "4. 读写内存的指令执行需要 4 个周期，`imull` 需要 3 个周期，其余指令仅需要 1 个周期。"
       "这里所需的周期数指指令在 ROB 中从开始执行到执行完成的执行延迟，本题中不考虑流水线的各个阶段。",
    5: "5. 不考虑指令引发 CPU 异常的情况。",
}

_QUIZ_Q4_ROB_START = "该处理器依次执行下面的 8 条指令，初始时 ROB 为空，且周期 1 之前所有的寄存器都已就绪。"

_QUIZ_Q4_ROB_INSTS = [
    ["I1", "`movslq %eax, %r8`"],
    ["I2", "`movl (%rsi,%r8,4), %ecx`"],
    ["I3", "`movl (%rdi,%r8,4), %r10d`"],
    ["I4", "`imull %r10d, %ecx`"],
    ["I5", "`addl %ecx, %r9d`"],
    ["I6", "`addl $1, %eax`"],
    ["I7", "`cmpl %edx, %eax`"],
    ["I8", "`jl .L3`"],
]

_QUIZ_Q4_ROB_ASKS = {
    1: "1. （5 分）假定某 CPU 的 ROB 有 5 个条目。周期 9 结束时，ROB 中有哪些条目？"
       "按程序顺序写出每个条目中指令的编号和状态。",
    2: "2. （4 分）处理器遇到分支跳转指令 `jl` 时，会按照预测的方向取进后面的指令，让它们进入 ROB "
       "并执行。如果 `jl` 执行后发现预测错误，处理器需要执行哪些操作？",
}


def _quiz_q4_pipe_context(p):
    with p.flex("row", widths=[2, 3]):
        p.code("assembly", _QUIZ_Q4_LOOP)
        _quiz_text(p, _QUIZ_Q4_PIPE_INTRO)


def _quiz_q4_rules(names):
    return "\n".join([_QUIZ_Q4_ROB_RULES_HEAD] + [_QUIZ_Q4_ROB_RULES[n] for n in names])


def _quiz_q4_assume(numbers, head=True):
    # A list that starts past 1 cannot interrupt a paragraph: hence the blank line.
    lines = [_QUIZ_Q4_ROB_ASSUME_HEAD, ""] if head else []
    return "\n".join(lines + [_QUIZ_Q4_ROB_ASSUME[n] for n in numbers])


# （1）第 1 问。data hazard 用到循环与题干中流水线的假定，二者并排在上，问题在下。
def q4_pipe_hazard(p):
    _quiz_page(p, _QUIZ_Q4_PIPE_TITLE)
    _quiz_q4_pipe_context(p)
    _quiz_asks(p, _QUIZ_Q4_PIPE_ASKS, [1])


# （1）第 2、3 问。第 3 问引用第 2 问的条件，两问同页。重复循环与题干。
def q4_pipe_cycles(p):
    _quiz_page(p, _QUIZ_Q4_PIPE_TITLE)
    _quiz_q4_pipe_context(p)
    _quiz_asks(p, _QUIZ_Q4_PIPE_ASKS, [2, 3])


# （2）的题干与示意图。题干说明 ROB 的作用、条目记录的内容与三种状态，图是一个 4 个条目的 ROB。
def q4_rob(p):
    _quiz_page(p, _QUIZ_Q4_ROB_TITLE)
    _quiz_text(p, _QUIZ_Q4_ROB_WHY + _QUIZ_Q4_ROB_WHAT + _QUIZ_Q4_ROB_FIGURE)
    p.image("assets/q-rob.svg", width_px=1120)


# （2）第 1 问之一。推演周期 9 的 ROB 要用条目与状态的定义和四条规则。
# 重复题干中条目与状态的两句，假设与指令表在下一页，下一页同样带第 1 问。
def q4_rob_rules(p):
    _quiz_page(p, _QUIZ_Q4_ROB_TITLE)
    _quiz_text(p, _QUIZ_Q4_ROB_WHAT)
    _quiz_text(p, _quiz_q4_rules(["进入", "执行", "完成", "提交"]))
    _quiz_asks(p, _QUIZ_Q4_ROB_ASKS, [1])


# （2）第 1 问之二。假设 1、2、4 决定开始执行与提交的周期，指令表给出依赖。
# 假设 3、5 与这 8 条指令无关，不放。规则在上一页。
# 起始条件一句在最上面占一行，假设与问题在左，指令表在右。
def q4_rob_q1(p):
    _quiz_page(p, _QUIZ_Q4_ROB_TITLE)
    _quiz_text(p, _QUIZ_Q4_ROB_START)
    with p.flex("row", widths=[3, 2]):
        with p.flex("column"):
            # Item 4 is a block of its own, so its list keeps the quiz's number.
            _quiz_text(p, _quiz_q4_assume([1, 2]))
            _quiz_text(p, _quiz_q4_assume([4], head=False))
            _quiz_asks(p, _QUIZ_Q4_ROB_ASKS, [1])
        p.table(_QUIZ_Q4_ROB_INSTS, headers=["编号", "指令"])


# The answer of question 1, from the answer sheet (exames/quiz-answer.md): the
# entries of the ROB at the end of cycles 1 to 9, oldest first, each with its
# state. Cycle 9 is the answer. Simulated by the rules of the quiz: I7 enters
# in cycle 8 and I8 in cycle 9, after I2 and I3 commit in cycles 7 and 8.
_QUIZ_Q4_ROB_CYCLES = [
    ["1", "I1 未执行", "", "", "", ""],
    ["2", "I1 已完成", "I2 未执行", "", "", ""],
    ["3", "I2 正在执行", "I3 未执行", "", "", ""],
    ["4", "I2 正在执行", "I3 正在执行", "I4 未执行", "", ""],
    ["5", "I2 正在执行", "I3 正在执行", "I4 未执行", "I5 未执行", ""],
    ["6", "I2 已完成", "I3 正在执行", "I4 未执行", "I5 未执行", "I6 未执行"],
    ["7", "I3 已完成", "I4 未执行", "I5 未执行", "I6 已完成", ""],
    ["8", "I4 正在执行", "I5 未执行", "I6 已完成", "I7 未执行", ""],
    ["**9**", "**I4 正在执行**", "**I5 未执行**", "**I6 已完成**", "**I7 已完成**", "**I8 未执行**"],
]


# （2）第 1 问的答案。上一页是第 1 问，本页把答案中各指令进入、执行、完成与提交的周期换成
# ROB 的条目：从上到下是周期 1 ~ 9 结束时，从左到右是 ROB 中按程序顺序排列的条目，每格是
# 指令的编号与状态，周期 9 一行是第 1 问的答案。九行的表占满页面，周期 7、8 只有 4 个条目的
# 原因写在讲稿中。
def q4_rob_answer(p):
    _quiz_page(p, "第 1 问：各周期结束时 ROB 中的条目")
    _quiz_text(p, "每一行是一个周期结束时 ROB 中的条目，从左到右按程序顺序排列。")
    p.table(_QUIZ_Q4_ROB_CYCLES, headers=["周期", "第 1 个", "第 2 个", "第 3 个", "第 4 个", "第 5 个"],
            widths=[1, 2, 2, 2, 2, 2])
    p.notes("每一行是一个周期结束时的 ROB，从左到右是按程序顺序排列的条目，最靠前的条目在最左边，"
            "每格是条目中指令的编号与状态。条目在进入的周期为未执行，开始执行的周期起为正在执行，"
            "执行完成的周期起为已完成，提交的周期离开 ROB。\n\n"
            "I1 在周期 3 提交，I2、I3 读内存各要 4 个周期，在周期 6、7 完成，周期 7、8 提交。"
            "I4 要用 I2、I3 的结果，周期 8 才开始执行，要 3 个周期，周期 10 完成。"
            "I6 与前面的指令无关，周期 7 完成，但排在未完成的 I4 之后，不能提交。\n\n"
            "周期 7、8 结束时只有 4 个条目：在周期 t 提交而空出的条目从周期 t + 1 起才能使用，"
            "I2 在周期 7 提交，I7 在周期 8 进入；I3 在周期 8 提交，I8 在周期 9 进入。\n\n"
            "周期 9 一行是第 1 问的答案：I4 正在执行，I5 未执行，I6、I7 已完成，I8 未执行。")


# （2）第 2 问。预测错误时要做的事取决于结果何时写入寄存器与内存。
# 重复完成与提交两条规则和假设 3。题干对 ROB 的说明在前几页已出现，本页不再重复（讲师
# 2026-11-19 的意见）。
def q4_rob_branch(p):
    _quiz_page(p, _QUIZ_Q4_ROB_TITLE)
    # Assumption 3 shares the rules' block: a list block of its own right above
    # the question would run into the question's list and renumber it.
    _quiz_text(p, _quiz_q4_rules(["完成", "提交"]) + "\n\n" + _quiz_q4_assume([3]))
    _quiz_asks(p, _QUIZ_Q4_ROB_ASKS, [2])


# ---------------------------------------------------------------- 第 5 题


_QUIZ_Q5_CPU_TITLE = "第 5 题（1）：CPU 上的标量执行与向量执行（10 分）"

_QUIZ_Q5_CPU_INTRO = ("函数 `dot_product` 计算两个 `int` 向量 `w` 与 `x` 的点积，即把两个向量对应位置的元素"
                      "相乘并累加，每个元素占用 32 位。")

_QUIZ_Q5_DOT = """int dot_product(const int *w, const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += w[i] * x[i];
    }
    return sum;
}"""

_QUIZ_Q5_CPU_FIGURE = r"机器的部分参数如下图，其中 1 GB 按 $10^9$ 字节计算。"

_QUIZ_Q5_CPU_CALL = ("程序某次调用 `dot_product`，传入的参数 `n` 为 1200，即两个向量各有 1200 个元素。"
                     "通过 gcc 编译得到的标量程序每执行一次循环体完成 1 组乘加，循环体有 6 条指令，"
                     "循环之外的指令忽略不计。")

_QUIZ_Q5_CPU_ASKS = {
    1: "1. （3 分）这次调用一共完成多少组乘加运算？需要从内存读取多少字节的向量元素？"
       "标量程序一共执行多少条指令？",
    2: "2. （3 分）按图中的带宽，从内存读取这些向量元素到 CPU 至少需要多少时间？按每个周期最多开始"
       "执行 5 条指令计算，执行这些指令至少需要多少时间？从内存读取向量元素与执行指令可以同时进行，"
       "因此这次调用的执行时间不短于两者中较长的一个。在只考虑带宽和指令吞吐的情况下，"
       "求这次调用的理论执行时间下限，并指出它由内存带宽还是指令吞吐决定。",
    3: "3. （4 分）程序改用 SIMD 指令后，每执行一次循环体完成 8 组乘加，而循环体仍只有 6 条指令。"
       "求这次调用的理论执行时间下限，以及按这个下限计算的相对于标量程序的加速比。"
       "如果继续减少每组乘加需要的指令数，这次调用的理论执行时间下限还能降低吗？说明理由。",
}

_QUIZ_Q5_GPU_TITLE = "第 5 题（2）：GPU 推理流水线（12 分）"

_QUIZ_Q5_GPU_MODEL = (r"假设某 fp8 量化的大模型一共有 $405 \times 10^9$ 个权重，分为 126 层，每层包含的权重数相同。"
                      "fp8 量化格式下，平均每个权重占用 8 位空间。")

_QUIZ_Q5_GPU_FIGURE = ("推理机器的部分参数如下图。忽略 GPU 之间传递数据的时间与 GPU 执行运算的时间，"
                       "仅考虑权重从显存传输到 GPU 的时间。")

_QUIZ_Q5_GPU_TOKEN = ("生成每个 Token 时，由 126 层按顺序依次计算。每次 Token 生成时，"
                      "都需要把每一层的权重从显存读到 GPU 一次。")

_QUIZ_Q5_GPU_ASKS = {
    1: "1. （2 分）至少需要几块 GPU 才能放下全部权重？用这个块数按层平均分配，每块 GPU 存放多少 GB 权重？",
    2: "2. （2 分）按第 1 问的结论把模型划分到多块 GPU 上，一个 Token 依次经过各块 GPU。"
       "在只有一个用户请求时，生成一个 Token 需要多长时间？",
    3: "3. （4 分）以一块 GPU 处理一个 Token 的时间为一个周期。画出这个用户单次请求生成前 2 个 Token 时，"
       "各块 GPU 在周期 1 ~ 12 的时空图，每格写出正在处理的 Token 编号，空闲的格留空代表 bubble。"
       "第 2 个 Token 为什么不能在周期 2 进入第 1 块 GPU？按这张时空图，各块 GPU 的平均利用率是多少？",
    4: "4. （4 分）现在有 N 个相互独立的用户请求，每个请求都需要生成大量 Token，"
       "不同请求可以在不同的 GPU 上重叠执行。N 至少为多少时，所有 GPU 几乎在每个周期都能满载？"
       "此时系统每秒共生成多少个 Token？N = 3 时，各块 GPU 的平均利用率是多少？",
}

_QUIZ_Q5_DISK_TITLE = "第 5 题（3）：单 GPU 推理（8 分）"

_QUIZ_Q5_DISK_INTRO = ("假设现在只能使用一块 GPU 运行第（2）问的模型，显存无法放下全部权重。生成每个 Token 时，"
                       "每一层在参与计算之前都需要从磁盘读入显存，计算完成之后它占用的显存就可以留给后面的层使用。"
                       "与（2）中相同，当某层权重全部在显存中时，近似把这层权重从显存传输到 GPU 的时间"
                       "视为这层的计算时间。推理机器的部分参数如下图。")

_QUIZ_Q5_DISK_ASKS = {
    1: "1. （5 分）边加载边计算是指 GPU 计算第 i 层的同时，磁盘把第 i+1 层读入显存。"
       "与逐层串行（一层读入显存之后 GPU 才开始计算这一层，计算完再读入下一层）相比，"
       "边加载边计算能否明显缩短生成一个 Token 的时间？结合图中的两个带宽说明理由。",
    2: """2. （3 分）在边加载边计算的基础上，下面三项改动分别单独进行。每项改动能否缩短生成一个 Token 的时间？能缩短的，时间缩短为原来的几分之几？说明理由。

   A.  GPU 换成显存带宽提高一倍的型号
   B.  换用 3 块相同的磁盘同时读入，每块读入每层权重的 1/3
   C.  权重改用平均每个权重占用 4 位的量化格式""",
}


def _quiz_q5_cpu_context(p, listing):
    """The machine and the call; the listing only where the page has room."""
    if listing:
        with p.flex("row", widths=[2, 3]):
            _quiz_text(p, _QUIZ_Q5_CPU_INTRO)
            p.code("c", _QUIZ_Q5_DOT)
    else:
        _quiz_text(p, _QUIZ_Q5_CPU_INTRO)
    with p.flex("row", widths=[1, 2]):
        _quiz_text(p, _QUIZ_Q5_CPU_FIGURE)
        p.image("assets/q-cpu-memory.svg")
    _quiz_text(p, _QUIZ_Q5_CPU_CALL)


def _quiz_q5_gpu_context(p):
    _quiz_text(p, _QUIZ_Q5_GPU_MODEL + _QUIZ_Q5_GPU_FIGURE)
    # Pinned at 600 px, the figure leaves the sentence beside it room for
    # three lines and the page with three questions still fits.
    with p.flex("row"):
        p.image("assets/q-gpu-memory.svg", width_px=600)
        _quiz_text(p, _QUIZ_Q5_GPU_TOKEN)


# （1）第 1 问。乘加次数、字节数与指令数用到元素宽度、n 与循环体的指令数。
# dot_product 的代码只在这一页，后两问用不到它。
def q5_cpu_count(p):
    _quiz_page(p, _QUIZ_Q5_CPU_TITLE)
    _quiz_q5_cpu_context(p, listing=True)
    _quiz_asks(p, _QUIZ_Q5_CPU_ASKS, [1])


# （1）第 2 问。时间下限还要用图中的带宽、主频与每周期的指令数，第 2 问所说的这些向量元素
# 与这些指令是第 1 问求出的量。重复题干与图，带第 1 问。
def q5_cpu_bound(p):
    _quiz_page(p, _QUIZ_Q5_CPU_TITLE)
    _quiz_q5_cpu_context(p, listing=False)
    _quiz_asks(p, _QUIZ_Q5_CPU_ASKS, [1, 2])


# （1）第 3 问。SIMD 版本的下限沿用第 2 问对下限的说明，重复题干、图与第 2 问。
def q5_cpu_simd(p):
    _quiz_page(p, _QUIZ_Q5_CPU_TITLE)
    _quiz_q5_cpu_context(p, listing=False)
    _quiz_asks(p, _QUIZ_Q5_CPU_ASKS, [2, 3])


# （2）第 1 ~ 3 问。第 2 问按第 1 问的结论划分模型，第 3 问的时空图画的是第 2 问那个用户的请求，
# 三问同页。
def q5_gpu_pipeline(p):
    _quiz_page(p, _QUIZ_Q5_GPU_TITLE)
    _quiz_q5_gpu_context(p)
    _quiz_asks(p, _QUIZ_Q5_GPU_ASKS, [1, 2, 3])


# （2）第 4 问。周期沿用第 3 问的定义，重复题干、图与第 3 问。
def q5_gpu_batch(p):
    _quiz_page(p, _QUIZ_Q5_GPU_TITLE)
    _quiz_q5_gpu_context(p)
    _quiz_asks(p, _QUIZ_Q5_GPU_ASKS, [3, 4])


# （3）第 1、2 问。第 2 问的三项改动以第 1 问定义的边加载边计算为基础，两问同页。
# 题干在上，图与第 1 问并排，第 2 问在下。
def q5_disk(p):
    _quiz_page(p, _QUIZ_Q5_DISK_TITLE)
    _quiz_text(p, _QUIZ_Q5_DISK_INTRO)
    # The figure is pinned so that the question beside it takes five lines.
    with p.flex("row"):
        p.image("assets/q-gpu-disk.svg", width_px=450)
        _quiz_asks(p, _QUIZ_Q5_DISK_ASKS, [1])
    _quiz_asks(p, _QUIZ_Q5_DISK_ASKS, [2])


# ==============================================================================
# 第二部分：mini-cpu 实验二的组相联缓存。
#
# 每页一张为本实验画的图，正文一两句或几条短句，讲解写在 p.notes 中。页面不给出答案代码。
#
# 图与讲稿中数字的来源（2026-11-18 按新的内存重录）：把 ../../labs/mini-cpu 复制到
# build/scratch-tutquiz-cache/ 下的两个副本，src/lab1/ 换成 labs/mini-cpu-ta/lab1/ 的参考实现：
# - mini-cpu：src/lab2/cache.c 换成 labs/mini-cpu-ta/lab2/cache.c，make（LAB=2）；
# - mini-cpu-lab1：只有 src/lab1，make LAB=1，即实验一的机器。
# 计数录自这两个副本的 --stats，命令的写法与学生在 labs/mini-cpu 中运行的写法相同（程序的路径
# 在客户机的栈上，影响计数）。
# 本部分没有演示：有缓存与没有缓存的计数一页、运行与评测一小节（框架调用 cache_init 与 cache_flush 的
# 时机、命中与内存读写、计数的检查）于 2026-11-20 删去，代码与图在 build/removed-tut-quiz-stats/ 与
# build/removed-tut-quiz-run/。
#
# 插图由 diagrams/cache_*.py 生成，脚本与图同名（下划线对应连字符），共用 diagrams/cache_kit.py
# 的画法：CPU 白底深色框，缓存的行浅蓝，内存绿色，命中绿色，缺失时的内存读写红色。
# 地址的三段 tag 紫、组号蓝、行内偏移橙。你实现的函数是深色胶囊，已写出的 cache_init 是灰色胶囊，
# 框架的函数是白底灰框。
# cache_funcs.py 另外生成 cache-funcs-hit、cache-funcs-miss、cache-funcs-evict 三张图：一次行访问调用的
# 函数与箭头保持原色，箭头加粗，其余部分的不透明度降到 20%。

# 第二部分：mini-cpu 的组相联缓存。
#
# 起点问题：实验一的机器用 dram_ptr 直接读写映射到进程内存的 dram.img，访存经过宿主 CPU 的缓存。
# 实验二起内存只能经 dram.c 一次读写一行，每读写一行都用 clflush 把这一行移出宿主 CPU 的缓存，读写的
# 是宿主的内存。
# 缓存怎样减少内存读写，你实现的函数在其中各做什么？
# 回答：框架把缓存与内存分在两处：缓存的行是进程中的数组 cache_lines，通常留在宿主 CPU 的缓存中，
# 内存是映射到进程中的 dram.img。cache.c 只经 dram_line_read 与 dram_line_write 读写内存，每读写一行
# 都访问宿主的内存。有缓存时绝大多数行访问是不读写内存的命中。缓存按
# 地址的组号只在一组中查找，组内按 LRU 替换，脏行在离开缓存时写回，这些步骤就是 cache.c 中的十个函数。
# 页序：实验一的访存路径 → 实验二的访存路径（文件、函数与边界）→ cache.c 中函数的调用关系 →
# cache_read 与 cache_write 的调用者（frame/mem.c 的 walk）→ 内存地址的拆分 → 缓存的组与路 →
# 命中时调用的函数 → 命中（动画）→ 缺失时调用的函数 → 替换脏行时调用的函数 → 缺失并写回脏行（动画）→
# 跨两行的访问。
# 小节：访存路径（前两页，问题与机制）→ cache.c 的函数与调用者（调用关系，框架中调用
# cache_read 与 cache_write 的 walk）→ 缓存的结构与访问过程（按 cache.c 中函数的顺序：地址、组与路、
# 命中、缺失、跨两行，命中与缺失各先用调用关系的图标出调用的函数，再用动画逐步走一遍）。
# 每个小节以只有标题的衔接页开始。


# walk of frame/mem.c (cache-walk shows it), its two long lines wrapped to fit the slide.
_CACHE_WALK = """static int walk(mcpu *cpu, uint64_t va, uint8_t *buf,
                unsigned nbytes, int acc)
{
    while (nbytes) {
        unsigned off = (unsigned)(va % PAGE_SIZE);
        unsigned chunk = PAGE_SIZE - off < nbytes
                       ? PAGE_SIZE - off : nbytes;
        uint64_t pa;
        if (mmu_translate(va, acc, &pa) < 0) {
            cpu->fault_va = va;
            return -1;
        }
        if (acc == MEM_WRITE)
            cache_write(pa, buf, chunk);
        else
            cache_read(pa, buf, chunk);
        va += chunk; buf += chunk; nbytes -= chunk;
    }
    return 0;
}"""


def _cache_slide(p, md, **kw):
    """页面文案。粗体逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def _cache_figure(p, name, width):
    return p.image(f"assets/{name}.svg", width_px=width)


# ==============================================================================
# 小节：访存路径
# ==============================================================================

# 本部分的第一页：从实验一已经写好的机器出发，看访存经过哪些文件：nocache.c 直接读写映射。
# 本页与下一页的图画布相同、各部分的位置相同，都放在标题下方。两页中间都有虚线框「宿主 CPU 的缓存」：
# 本页 nocache.c 经 dram_ptr 的读写穿过它，读写过的行由 CPU 硬件自动缓存；换页时中间一块换成缓存，
# 虚线框中换成组 0 到组 63，dram_ptr 的箭头打叉，出现经 dram.c 访问物理内存的路径。两页不提系统调用，
# 课上还没有讲。
def cache_lab1(p):
    p.title("实验一的访存路径")
    _cache_figure(p, "cache-path-lab1", 970)
    _cache_slide(p, """
实验一的 [`nocache.c`](file:/labs/mini-cpu/frame/nocache.c) 用 `dram_ptr` 直接读写映射，读写过的行由 CPU 硬件自动缓存。
""")
    p.notes("""
图中是一次访存经过的文件，从左上角的解释器开始。实验一的解释器 interp.c 取指、读数据、写数据，分别调用 frame/mem.c 的 fetch_bytes、mem_read 与 mem_write。mem.c 求出物理地址 pa，取指与读数据调用 cache_read，写数据调用 cache_write。
实验一的机器中，cache_read 与 cache_write 来自 frame/nocache.c：它们用 dram_ptr(pa) 得到这个地址在映射中的指针，再用 memcpy 复制要读写的字节。这些读写与进程中的任何访存一样经过宿主 CPU 的缓存：读写过的行由 CPU 硬件自动保存在缓存中，再次访问时命中，程序与操作系统都不参与。图中的虚线框是宿主 CPU 的缓存，框中绿色的格子是映射中被缓存的行。
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
[`cache.c`](file:/labs/mini-cpu/src/lab2/cache.c) 读写内存只能调用 [`dram.c`](file:/labs/mini-cpu/frame/dram.c) 的 `dram_line_read` 与 `dram_line_write`。
""")
    p.notes("""
实验二中，cache_read 与 cache_write 换成 src/lab2/cache.c 中由你实现的函数。左边两个文件不变：解释器照样调用 mem.c，mem.c 照样调用 cache_read 与 cache_write，收到的是一段连续的物理地址，长度为 1 到 4096 字节，可能跨过行的边界。
frame/main.c 调用另外两个函数：cache_init 建立缓存，已经写出，cache_flush 把脏行写回内存。
缓存的 64 × 4 行在 cache.c 的数组 cache_lines 中，这个数组只有约 22 KiB，读写它通常在宿主 CPU 的缓存中命中，图中的虚线框表示这一点。
右边的映射仍然存在，框架装载程序时用 dram_ptr 读写它。cache.c 不能使用 dram_ptr，图中这两个箭头打了叉。
cache.c 读写内存只能调用 frame/dram.c 的 dram_line_read 与 dram_line_write，每次读写整整一行 64 字节。两个函数都在映射中复制这一行，并用 clflush 指令把这一行移出宿主 CPU 的缓存。dram_line_read 先移出再读，读到的是宿主内存中的字节。dram_line_write 先写再移出，写入的字节到达宿主的内存。图中红色的两条路径就是这两种内存读写，每一次都访问物理内存。
关闭缓存时，访存涉及的每一行都读写一次内存，不调用你实现的任何函数。接下来看 cache.c 中有哪些函数，框架从哪里调用它们。
""")


# ==============================================================================
# 小节：cache.c 的函数与调用者
# ==============================================================================

# 上一页给出 cache.c 在访存路径中的位置。本页给出 cache.c 中十一个函数的调用关系。
# 下一页给出框架中调用 cache_read 与 cache_write 的地方。之后各页按文件中的顺序说明这些函数，命中与缺失
# 的动画之前各用这张图标出实际调用的函数。
def cache_funcs(p):
    p.title("cache.c 中函数的调用关系")
    _cache_slide(p, """
框架只调用四个函数。其余函数按文件中的顺序写，后面的函数调用前面的函数。
""")
    _cache_figure(p, "cache-funcs", 1080)
    p.notes("""
深色的胶囊是你实现的十个函数，灰色的 cache_init 已经写出，作为使用 cache_lines、cache_set_count、cache_way_count 的例子。白框是框架的函数。
框架只调用四个函数：frame/main.c 调用 cache_init 与 cache_flush，frame/mem.c 调用 cache_read 与 cache_write。
cache_read 与 cache_write 把一次访存按行拆开，每一行调用一次 cache_access。cache_access 是读与写共用的一次行访问：用 cache_set_index 与 cache_tag 求出组号与 tag，用 cache_lookup 在这一组中查找。缺失时用 cache_victim 选出被替换的路，再用 cache_fill 写回脏行、读入新行。
cache_fill 与 cache_flush 都要把一行写回它在内存中的位置，行中只保存了 tag，位置由 cache_line_address 从 tag 与组号求出。只有这两个函数读写内存。红色的箭头通向 dram_line_read 与 dram_line_write，每一次调用读写一行，访问宿主的内存。
下一页看框架中哪里调用 cache_read 与 cache_write。
""")


# 上一页的图中 frame/mem.c 调用 cache_read 与 cache_write。本页给出这次调用所在的 walk：它把一次访存
# 按页拆开，每页求出物理地址后调用 cache_read 或 cache_write，框架中的取指、数据读写与系统调用都经过它。
# 代码在左，右栏是 walk 的地位与调用它的三种访存。代码中两个长行折成两行，以免超出左栏。
def cache_walk(p):
    p.title("cache_read 与 cache_write 的调用者")
    with p.flex("row", widths=[3, 2], valign="middle"):
        p.code("c", _CACHE_WALK)
        _cache_slide(p, """
框架中只有 [`frame/mem.c`](file:/labs/mini-cpu/frame/mem.c) 的 `walk` 调用 `cache_read` 与 `cache_write`。

调用 `walk` 的三种访存：

- 取指：`fetch_bytes`；
- 读写数据：`mem_read`、`mem_write`；
- 系统调用读写程序的缓冲区：`mem_read`、`mem_write`。
""")
    p.notes("""
walk 是 frame/mem.c 中的静态函数，框架的其他文件都经由 mem.c 的三个函数到达它。
取指：frame/x86.c 译码一条指令之前调用 fetch_bytes，读出 pc 起最多 15 个字节，walk 的 acc 是 MEM_FETCH。
读写数据：frame/x86.c 的 load 与 store 调用 mem_read 与 mem_write。解释器读写内存操作数、push、pop、call、ret 都经过 load 与 store。
系统调用：frame/sysbase.c 处理 write 时用 mem_read 读出程序的缓冲区，处理 fstat 时用 mem_write 把结果写入程序的缓冲区。
va 是程序中使用的地址，即虚拟地址。walk 每次处理一页之内的部分，先用 mmu_translate 把 va 换成物理地址 pa，实验二中这一步由框架的 frame/pagemap.c 完成。两个相邻的虚拟页在物理内存中可以相距很远，所以每页单独换算。cache_read 与 cache_write 因此收到一段连续的物理地址，长度为 1 到 4096 字节，可能跨过行的边界，cache.c 要再按行拆开。
关闭缓存时，walk 的读写由框架逐行访问内存完成，不调用你实现的函数。
装载程序时框架用 dram_ptr 直接写入 dram.img，不经过 walk，也不经过缓存。
接下来从地址的划分开始，按文件中的顺序说明 cache.c 中的函数。
""")


# ==============================================================================
# 小节：缓存的结构与访问过程
# ==============================================================================

# 上一页说明 cache_read 与 cache_write 收到的是物理地址，cache_access 先由它求组号与 tag。
# 本页说明物理地址怎样分成三段，
# 以及 cache_set_index、cache_tag、cache_line_address 三个函数分别求出什么。
def cache_split(p):
    p.title("内存地址的拆分")
    p.valign("middle")
    _cache_slide(p, """
64 组时，组号是第 6 到 11 位，tag 是第 12 位及以上。用 `%` 与 `/` 计算，对任意组数都成立，比如 3 组的缓存。
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
一个地址只比较一组中 4 路的 `valid` 与 `tag`，`cache_lookup` 只查找。
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
    p.title("命中时调用的函数")
    _cache_slide(p, """
命中时 `cache_lookup` 找到这一行，`cache_read` 从行中复制，不读写内存。
""")
    _cache_figure(p, "cache-funcs-hit", 1080)
    p.notes("""
图是 cache.c 中函数的调用关系，只突出 cache_read 的一次行访问命中时调用的函数与箭头，其余部分是灰色。
frame/mem.c 调用 cache_read，cache_read 为这一行调用 cache_access。cache_access 用 cache_set_index 与 cache_tag 求出组号与 tag，再用 cache_lookup 在这一组中查找，找到 valid 为 1 且 tag 相等的一路。cache_access 把 cache_hits 加 1，把这一行的 last_use 设为 cache_use_clock 加 1 之后的值，返回这一行。cache_read 从行中复制要读的字节。
整条路径只读写数组 cache_lines，不调用 dram.c，不读写内存。cache_write 命中时的路径相同，复制的方向相反，并把这一行的 dirty 设为 1。绝大多数行访问走这条路径，比如一个程序给 128 × 128 的 int 矩阵（64 KiB）赋值，再按行求和 8 遍，在 64 组、每组 4 路的 16 KiB 缓存中，它的行访问（含取指）有 99.43% 命中。
下一页用一个地址逐步走一遍命中。
""")


# 上一页给出命中时调用的函数。本页用动画在组与路中走一遍命中：cache_access 求组号与 tag，cache_lookup 找到 way 1，
# 计数器与 last_use 的变化，cache_read 从行中复制。
def cache_hit(p):
    p.title("命中时的 cache_access")
    p.valign("middle")
    _cache_slide(p, """
`cache_read(0x12345, dst, 4)` 只涉及一行，调用一次 `cache_access(0x12345)`。
""")
    p.frames(*(f"assets/cache-hit-{k}.svg" for k in (1, 2, 3)), width_px=1040)
    p.notes("""
组 13 的四路都有效，tag 分别是 0x05、0x12、0x07、0x2a，way 3 是脏行。这是为说明过程设的状态。
第 1 步：cache_set_index 与 cache_tag 求出组号 13 与 tag 0x12，只在组 13 中查找。
第 2 步：cache_lookup(13, 0x12) 依次比较四路，way 1 有效且 tag 相等，返回 way 1。
第 3 步：命中。cache_access 给 cache_hits 加 1，cache_use_clock 加 1 变为 60，把 60 写入 way 1 的 last_use，返回 way 1。cache_read 从 data[5] 起复制 4 个字节到 dst。这次行访问不读写内存。
last_use 记录的是最近一次访问的先后：way 1 现在最大，它是这一组中最近使用的行。下一页看缺失时调用哪些函数。
""")


# 上一页的命中不经过 dram.c。本页是缺失、被替换的路无效或不脏：路径上多出 cache_victim 与
# cache_fill，cache_fill 用 dram_line_read 读入一行。
def cache_funcs_miss(p):
    p.title("缺失时调用的函数")
    _cache_slide(p, """
缺失时 `cache_victim` 选出被替换的路，`cache_fill` 用 `dram_line_read` 读入一行。
""")
    _cache_figure(p, "cache-funcs-miss", 1080)
    p.notes("""
缺失时 cache_lookup 返回 NULL。cache_access 把 cache_misses 加 1，调用 cache_victim 在这一组中选出被替换的路：有 valid 为 0 的路时选它，否则选 last_use 最小的一路，即最久没有访问的一路。
cache_fill 用 cache_line_address 由新的 tag 与组号求出这一行在内存中的地址，调用 dram_line_read 读入 64 字节，再把 valid 设为 1、dirty 设为 0、tag 设为新的 tag。之后的步骤与命中相同：cache_access 更新 last_use 并返回这一行，cache_read 从中复制。
图中被替换的路无效或没有被写过，cache_fill 直接覆盖它，dram_line_write 是灰色的。这一次行访问读写一次内存。
下一页是被替换的行是脏行时的路径。
""")


# 上一页被替换的路不脏，cache_fill 直接覆盖它。本页是被替换的行是脏行：cache_fill 先用
# dram_line_write 写回它，再读入新行，一次行访问读写两次内存。
def cache_funcs_evict(p):
    p.title("替换脏行时调用的函数")
    _cache_slide(p, """
被替换的行是脏行时，`cache_fill` 先用 `dram_line_write` 写回它，再读入新行。
""")
    _cache_figure(p, "cache-funcs-evict", 1080)
    p.notes("""
一行被 cache_write 写过之后 dirty 为 1，内存中这一行的内容较旧。这样的行被 cache_victim 选中时，它的内容不能直接丢弃。
cache_fill 先用 cache_line_address 由这一行自己的 tag 与组号求出它在内存中的地址，调用 dram_line_write 写回 64 字节，把 cache_writebacks 加 1，再按缺失的步骤求出新行的地址、调用 dram_line_read 读入。两个地址的组号相同，tag 不同。
图中 cache_fill 调用了全部三个函数。这一次行访问读写两次内存，是三种情况中最慢的一种。
图左的 cache_flush 也调用 dram_line_write，它把仍在缓存中的脏行全部写回，不在一次行访问的路径上。
下一页接着命中的例子读另一个地址，逐步走一遍缺失并写回脏行。
""")


# 上一页给出替换脏行时调用的函数。本页接着命中的例子读另一个地址，用动画走一遍缺失：查找失败，cache_victim 按 last_use 选出 way 3，
# cache_fill 先写回脏行、再读入新行，两次内存读写都由 cache_line_address 求出地址。
def cache_miss(p):
    p.title("缺失时的 cache_access")
    p.valign("middle")
    _cache_slide(p, """
接着读 `0x13345`：组号同为 13，tag 为 0x13，组 13 中没有这一行。
""")
    p.frames(*(f"assets/cache-miss-{k}.svg" for k in range(1, 6)), width_px=1040)
    p.notes("""
第 1 步：cache_lookup(13, 0x13) 比较四路，没有一路的 tag 是 0x13，返回 NULL。cache_access 给 cache_misses 加 1。
第 2 步：cache_victim(13) 选出被替换的路。组中有无效的路时选编号最小的无效路。四路都有效时选 last_use 最小的一路，即最近最少使用的行。这里 last_use 为 41、60、50、33，选 way 3。cache_victim 只负责选择，不改计数器。
第 3 步：cache_fill 处理缺失。way 3 是脏行，它的内容比内存新，先用 dram_line_write 写回。行中只有 tag 0x2a，地址由 cache_line_address(0x2a, 13) 求出：(0x2a × 64 + 13) × 64 = 0x2a340。cache_writebacks 加 1，这是一次内存读写。被替换的行不脏时直接丢弃，不写回。
第 4 步：cache_fill 用 dram_line_read 从 cache_line_address(0x13, 13) = 0x13340 读入整行，再置 valid = 1、dirty = 0、tag = 0x13。这是第二次内存读写。
第 5 步：回到 cache_access，cache_use_clock 加 1 变为 61，写入 way 3 的 last_use，返回 way 3。cache_read 从 data[5] 起复制。这次行访问共读写两次内存。
两次都是 4 个字节，都在一行之内。下一页看一次访存跨过行边界的情况。
""")


# 上一页的访存都在一行之内。本页说明一次访存跨过两行时 cache_read 与 cache_write 怎样逐行处理，
# 以及写时的 dirty 与写分配。
def cache_span(p):
    p.title("跨两行的访问")
    p.valign("middle")
    _cache_slide(p, """
- 每一轮处理一行：`offset = pa % 64`，复制 64 − `offset` 与剩余字节数中较小的一个；
- `cache_write` 缺失时，`cache_access` 先读入整行（写分配），再改其中的字节、置 `dirty`，不写内存（写回）。
""")
    _cache_figure(p, "cache-span", 1040)
    p.notes("""
mem.c 交给 cache_read 与 cache_write 的是一段连续的物理地址，长度 1 到 4096 字节，可能跨过行的边界。取指一次读 15 个字节，跨两行的情况经常出现。
图中的例子从 0x1237c 读 8 个字节。行内偏移是 60，这一行只剩 4 个字节，第 1 轮调用 cache_access(0x1237c)，在组 13 中找到行 0x12340，复制最后 4 个字节。第 2 轮 pa 前进到 0x12380，行内偏移 0，调用 cache_access(0x12380)，在组 14 中找到下一行，复制前 4 个字节。
每一轮是一次行访问，所以这次读取计两次命中或缺失。这也是行访问次数多于指令数的原因之一。
cache_write 是同一个循环，复制的方向相反，并把两行的 dirty 都置为 1。写只改行中的几个字节，其余字节必须保持内存中的值，所以写缺失时 cache_access 先读入整行，这就是写分配。写入的内容留在缓存中，行离开缓存或 cache_flush 时才写回内存。
程序结束时缓存中还有脏行，框架在程序结束后调用 cache_flush 把它们写回。
""")


# ==============================================================================
# 第三部分 · mini-ollama 的 KV Cache
# ==============================================================================
#
# 本部分回答一个问题：mini-ollama 怎样在有上限的内存中为多个会话保存键与值，kvcache.c 中的函数各在何时
# 被调用、算什么、改什么。kv_open 与 compute 已给出，compute 是写好的例子，供理解流程，它调用由你实现的函数。
# 其余 7 个函数由你实现：kv_block_bytes、kv_key、kv_value、kv_append、kv_invalidate、kv_victim、kv_reserve。
#
# 各页脱离代码也能读懂：学生手里没有仓库，也没有运行过程序。一个例子在页上给全它的场景（之前的状态、
# 这次请求、其他数字所依据的数），选项按含义写（「每次最多生成 8 个 token」），实测在页上写明测了什么。
# 页面与讲稿不引用实验的文件、make 目标、命令行选项与某次运行的轮次编号。保留的是学生实现的函数名，
# 以及两段动画中在页上解释了的调用。
#
# 回答分三个小节，每个小节以只有标题的衔接页开始。
# KV Cache 的作用：3 个会话依次发来 7 次请求，每次把消息与回答接在会话的文本之后。回答的
# 每个 token 经过模型的 28 层，第 pos 个 token 的注意力公式用到第 0 到 pos 个 token 的键与值，每个
# token 在每层有一行键与一行值。不使用 KV Cache 时每一步重新计算之前的行，KV Cache 直接读取它们。
# 这一小节只讲做法，不出现 kvcache.c 的函数。
# KV Cache 的结构与访问过程：课上只讲过为什么缓存 K 与 V，没有讲过怎样存，所以先给结构，再给函数。顺序是
# 在内存中怎么放（会话表、每个会话的块表、分成等大块的内存池与位图、一块中的四段）、怎么找到一行（查
# block[tok]，再加块内偏移）、第一次请求怎么写入、下一次请求怎么查到并继续（两段动画）。四页用同一个小例子：
# 2 层，kv_dim 为 10，一块 160 字节，池 12 块，max_answer 为 2。A 第 1 轮的消息是 t0 t1 t2，分到块 0–3，
# B 的一轮分到块 4、5，A 第 2 轮的新消息是 t5，只再分到块 6–8，所以 A 的块不连续，从 token 4 起 token 编号
# 与块编号不同。kv_dim 取 10，使一行的 float 数不与 token 数、块数混淆。每个 token 一块，所以 kv_reserve
# 要数出多少个 token 会作为输入经过模型：动画用具体的 token 数出 4 与 7，在三处说明只输出的 token 不占块
# （数块的一帧、t3 刚被输出的一帧、一轮结束的一帧），页面与讲稿不给一般算式，它由学生自己算出。
# 内存在这一小节总是放得下，kv_victim 与 kv_invalidate 留给下一小节。
# 内存上限与失效：前三页共用一个例子。96 块的内存池中 A、B、C 已缓存 42、19、21 个 token，B 又收到一条
# 消息。B 只缺 25 块，三个会话最终需要 107 块，真正超出上限。kv_reserve 先收回预留（此例为 0 块），仍不够才使会话失效。两种策略选择不同的会话，重新计算的 token 数
# 随之不同，运行时间由经过模型的 token 数决定。
#
# 页序：会话与多轮对话 → 一层中的注意力（公式）→ 之前 token 的键与值（动画：不使用与使用 KV Cache）→
# KV Cache 在内存中的结构（2 页：结构、一行的位置）→ 第一次请求的写入（动画，16 帧）→
# 下一次请求的查找与继续计算（动画，13 帧）→ KV Cache 的内存与上限 →
# 内存不足时的 kv_reserve（动画，B 的请求，6 帧）→ 两种策略的失效（同一起点之后的三次请求）→ 运行时间与内存上限。
# 第二小节原来的两页（框架与 kvcache.c、一轮对话中的调用）于 2026-11-22 删去，代码、图与译文在
# build/removed-tut-quiz-kv-sec2/。
# make run 的演示一页与 make test 的检查一页于 2026-11-20 删去，代码与图在 build/removed-tut-quiz-kv/。
#
# 材料：实验目录 labs/kvcache（README.md、include/kvcache.h、src/kvcache.c 的注释、frame/main.c、
# frame/kvmem.c、tests/），助教目录 labs/kvcache-ta/README.md。页面与讲稿不含参考答案的代码。
#
# 一层中的注意力一页只有公式，没有图。
# 图由 diagrams/kv_*.py 生成（共用 kv_kit.py）。配色：有效的行与块浅蓝，这一步写入或改变的橙色，读取的
# 绿色，重复的计算与失效释放的块红色，预留的块白底蓝色虚线，空闲的块灰色，输入向量灰色。块下方标出它
# 属于谁：A 蓝、B 紫、C 棕。kvcache.c 中你实现的函数是深色圆角标签，已给出的 kv_open 与 compute 是灰色
# 圆角标签，框架的函数是白底灰框。
#
# 第二小节的四页共用 diagrams/kv_example_kit.py。动画每帧各区域位置固定：serve 的步骤、调用与实参及算出
# 和返回的值、会话的 token、会话表与块表、内存池与位图、一句说明。token 画在它的块表项的正上方。块一律
# 写作「块 n」，token 一律写作 t 或 b 加编号。第三小节例子的数字在 diagrams/kv_run_kit.py 与各脚本
# 中，与按块存储规则的独立重放（build/scratch-kvblocks/contract/ 的 simulate.py 与 expected.txt）逐项相同。
# 运行时间一页的时间与计数取自 build/scratch-kvblocks/handoff/runs.md（真实模型上的四次运行，各一次），
# 时间随硬件与负载而异。


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
# 小节：KV Cache 的作用
# ==============================================================================

# 本部分的第一页，先看程序的输入：依次到来的请求，各属于一个会话，
# 每次请求的消息与回答接在这个会话的文本之后。回答逐个 token 生成，由此引出下一页：
# 一个 token 经过模型时要用到哪些数据。
def kv_sessions(p):
    p.title("会话与多轮对话")
    _kv_slide(p, """
三个会话依次发来 7 次请求，每次请求的消息接在所属会话的文本之后。
回答逐个 token 生成，每次最多 8 个。
""")
    _kv_figure(p, "kv-sessions", 1040)
    p.highlight("生成一个 token 时，模型的每一层要用到哪些数据？")
    p.notes("""
图的左边是依次到来的 7 次请求，圆圈中的数字是到来的先后，后面是会话与消息。A、B、C 是三个会话，同一个会话的几次请求就是一段多轮对话。
一次请求分三步。第一步把消息按模型的对话格式接在这个会话已有的文本之后。第二步计算文本中还没有进入 KV Cache 的 token。第三步逐个生成回答，每一步取分数最高的 token，直到模型给出结束符或回答达到 8 个 token。
图的右边是全部请求回答完之后每个会话的文本，一格是一个 token，格的上方是这次请求的先后与「消息 + 回答」的 token 数。深灰色是消息，包括对话格式的标记，比如标出用户发言开始与结束的 token。白色是回答，这 7 次回答都达到了 8 个 token。比如 A 的第一次请求是 13 个 token 的消息与 8 个 token 的回答，A 之后又有两次请求接在它的后面，最后共有 68 个 token。
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
第 pos 个 token 在一层中算出自己的查询 $q_{\mathrm{pos}}$、键 $k_{\mathrm{pos}}$ 与值 $v_{\mathrm{pos}}$。
一个头的权重 $a$ 与输出 $o_{\mathrm{pos}}$ 如下，$d$ 是一个头的向量长度。

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
第 0 到 pos−1 行属于之前的 token。一个 token 在一层的输入只取决于它与它之前的 token，所以它的键与值不随新 token 改变，在之后的每一步都与第一次算出时相同。
下一页比较两种做法：每一步重新计算这些行，或者把它们保存下来直接读取。
""")


# 上一页用公式指出注意力需要之前 token 的键与值。本页用两帧图比较两种做法：不使用 KV Cache 时每一步
# 重新计算它们，KV Cache 从内存中直接读取。kvcache.c 的函数留给下一小节，图中不标出。
def kv_reuse(p):
    p.title("之前 token 的键与值")
    _kv_slide(p, """
不使用 KV Cache 时，每一步重新计算之前全部 token 的键与值。
KV Cache 把它们保存在内存中，每一步只计算一行。
""")
    p.frames("assets/kv-reuse-1.svg", "assets/kv-reuse-2.svg", width_px=1000)
    p.notes("""
共 2 帧，图中编号为 j 的一列是第 j 个 token，这一列中键与值的两格是它在这一层的第 j 行键与第 j 行值。橙色的一列是正在计算的新 token，即第 pos 个。
第 1 帧不使用 KV Cache。程序保存每个 token 在每一层的输入向量，每一步在每一层由它们重新计算第 0 到 pos−1 个 token 的键与值，然后才做新 token 的注意力。红色是这些重复的计算。第 pos 步每层重新计算 pos 行，会话越长，每一步重新计算得越多。
第 2 帧是 KV Cache。蓝色虚线框是这个会话在 KV Cache 中保存的、之前 token 在这一层的键与值，框中只表示有哪些行。注意力计算把新 token 的键与值写入第 pos 行，再读取第 0 到 pos 行，绿色是直接读取的之前各行。28 层都写完之后，这个 token 进入缓存，之后的每一步都直接读取它的行。
两种做法中每个 token 经过相同的运算，次序也相同：缓存中的一行就是这个 token 第一次计算时得到的键或值。所以两种做法的回答逐字节相同，这也是 KV Cache 是否正确的判据。
接下来先看这些键与值怎样保存在内存中，再看一次请求怎样写入与读取它们。
""")


# ==============================================================================
# 小节：KV Cache 的结构与访问过程
# ==============================================================================

# 上一页说明 KV Cache 把之前 token 的键与值保存在内存中。本页回答怎样保存：会话表每项中有一张块表，
# 块表的每一项指向内存池中的一块，池有一张每块一位的位图，一块中只有 float。取第二轮 t4 算完、t5 未算的
# 时刻，有效、预留、空闲三种块都有，A 的块也不连续。先只讲结构，图中不出现函数。下一页在这个时刻找一行。
def kv_layout(p):
    p.title("KV Cache 在内存中的结构")
    _kv_slide(p, f"""
真实模型有 28 层、一行 1024 个 float，结构与这个例子相同。{_kv_file("include/kvcache.h")}
""")
    _kv_figure(p, "kv-layout", 1120)
    p.notes("""
KV Cache 要为每个会话、每个 token、每一层保存一行键与一行值。例子中有 2 层、1 个键值头，这个头的长度 d = 10，所以 kv_dim = 10，一行是 10 个 float。真实模型有 28 层、8 个键值头，每头 128 个数，一行是 1024 个 float，一块是 224 KiB，结构相同。
最上面是会话表，即 kv_cache 中的数组 session[]，每一项对应一个会话。id 是会话的编号。n_cached 是已经算完的 token 数。capacity 是已经分到的块数。last_use 记录会话最近一次被使用的先后，越大越近。
每一项中有一张块表 block，它是有 512 项的指针数组：block[tok] 是第 tok 个 token 那一块的地址。图中各项写出它指向的块的编号，箭头指到这一块。没有分到块的项是 NULL。
下面是内存池。程序一次分配好内存池，把它分成等大的 12 块，每块放一个 token 在全部层中的键与值。位图每块一位，1 表示这一块在用，0 表示空闲。
图中是这样一个时刻：A 先分到块 0、1、2、3，随后 B 分到块 4、5，之后 A 又分到块 6、7、8，现在 A 算完了 t0 到 t4，t5 还没有算。所以 A 的块不连续：block[4] 指向块 6。一个会话的块可以在池中的任何位置，顺序只由块表决定。
三种状态都在图中。A 的前 5 项有效，即块 0、1、2、3、6，其中是算好的键与值。接下来的 2 项是预留，即块 7、8，位已经是 1，内容还没有意义。块 9、10、11 空闲。
右下放大块 6：4 行依次是第 0 层的键、第 0 层的值、第 1 层的键、第 1 层的值，每行 10 个 float，共 40 个 float、160 字节。块内只有 float。一块属于哪个会话的哪个 token 只记在块表中，池下方「属于」一行是按块表标出的。块表与位图不计入内存上限。
结构中没有记录每一行的位置，下一页看怎样算出一行的地址。
""")


# 上一页给出会话表、块表与池。本页在同一时刻由（会话、层、K 或 V、token 编号）分两步算出一行的地址：
# 查 block[tok]，再加块内偏移。例子取 token 4，它在块 6，两个编号不同。kv_block_bytes 是一块的长度。
# 只给数值演算。之后两段动画中各帧的地址都按本页的方法得到。
def kv_locate(p):
    p.title("KV Cache 在内存中的结构")
    _kv_slide(p, f"""
你需要实现 `kv_key` 与 `kv_value`：先查块表，再加块内偏移。{_kv_file("src/kvcache.c")}
""")
    _kv_figure(p, "kv-locate", 1120)
    p.notes("""
找一行需要四项：哪个会话、哪一层、K 或 V、第几个 token。图中要找会话 A 第 1 层 token 4 的值，调用是 kv_value(cache, A, 1, 4)。
第 1 步查块表。第 4 个 token 的块由 A 的 block[4] 给出，它是池中的块 6。token 编号是 4，块编号是 6，块的位置只能通过块表得到，比如 token 2 也要查 block[2]，它在这个例子中是块 2。
第 2 步加块内偏移，单位是 float，与 C 中 float 指针的加法一致。第 1 层之前有第 0 层的一行键与一行值，共 1 × 2 × 10 = 20 个 float。这一层的值又在这一层的键之后，再加 10 个。所以 v4 从 block[4] + 30 开始，是图中编号 30 到 39 的 10 个 float。
kv_key(cache, A, 1, 4) 返回同一块中的 block[4] + 20。两个函数只计算地址，块中的内容与会话表都保持原样。偏移只取决于层与 K、V，在所有块中相同。
kv_block_bytes(cache) 是一块的长度：2 层 × 2 行 × 10 个 float × 4 字节 = 160 字节。内存池按这个长度分成块。
下一页从一张空的会话表开始，看一次请求怎样调用这些函数并写入内存池。
""")


# 前两页给出结构与一行的位置。本页用动画从空的会话表走完第一次请求：kv_open 建立一项，kv_reserve 数出
# 会作为输入的 4 个 token，经 kv_alloc 置 4 位并填块表，已给出的 compute 在每层用 kv_key、kv_value 取得地址，
# attention 写第 pos 项、读第 0 到 pos 项，全部层之后 kv_append。第一个 token 分层讲清，第二个 token
# 展示读取之前的行与注意力公式，其余的 compute 各合并成一帧。结束时 t4 只输出，下一页由它继续。
# 每帧给出调用者与实参、算出与返回的值、读写的格、会话表与位图的变化，符号与 kv_step 相同。
def kv_first(p):
    p.title("第一次请求的写入")
    _kv_slide(p, """
会话 A 的第一次请求有 3 个 token，每次最多生成 2 个 token，缓存从空开始。
""")
    p.frames(*(f"assets/kv-first-{k}.svg" for k in range(1, 17)), width_px=1120)
    p.notes("""
共 16 帧。最上面一行是框架的 serve 处理一轮对话的步骤，有橙色框的是当前一步。下面的框是这一步的调用与实参、算出和返回的值。再下面是会话 A 的 token，每个 token 在它的块表项正上方，然后是会话表与内存池。橙色是写入或改变，绿色是读取，深色框是本步定位的项，比如取得地址的一行。例子中只画一个头，d = 10。会话表上方一行给出这一轮的两个数：n_prompt 是文本的 token 数，max_answer 是最多生成的 token 数。
第 1 帧：请求到来。append_message 把消息转换为 t0、t1、t2，n_prompt = 3。会话表为空，12 块都空闲，位图全为 0。
第 2 帧：serve 调用 kv_open(cache, 0)，这个函数已经给出。表中没有 id 为 0 的项，所以新建 session[0]：n_cached 与 capacity 为 0，块表全为 NULL，n_sessions 变为 1。clock 变为 1，记入 last_use。函数返回这一项的地址，以下记为 A。
第 3 帧：框架调用 kv_reserve(cache, A, 3, 2)，两个数是 n_prompt 与 max_answer：文本有 3 个 token，最多生成 2 个。kv_reserve 要先数出这一轮有多少个 token 会作为输入经过模型，每个这样的 token 要一块。在这个例子中它们是消息的 t0、t1、t2 与回答的第一个 token t3，共 4 个。t3 要作为输入，才能得到回答的第二个 token t4。t4 生成之后本轮就结束，它是最后一个输出，在本轮中没有作为输入，所以不需要块。capacity 是 0，缺 4 块，kv_num_free() 返回 12，足够。
第 4 帧：kv_reserve 调用 4 次 kv_alloc。kv_alloc 是框架的函数，每次取编号最小的空闲块，把它的位从 0 改为 1，返回块的地址。kv_reserve 把地址依次填入 block[0] 到 block[3]，capacity 变为 4，返回 1。n_cached 仍是 0，这 4 块都是预留。
第 5 帧：框架从第 n_cached 个 token 起，逐个调用 compute。compute 与 kv_open 一样已经给出，它是一个写好的例子，你可以读它来理解一个 token 的计算流程：它调用由你实现的 kv_key、kv_value 与 kv_append，以及框架的 embed、attention 与 feed_forward。compute(cache, A, t0, state) 把调用时的 n_cached = 0 作为 pos，先调用 embed(t0, state)，得到 t0 的向量。
第 6 帧：第 0 层。compute 调用 kv_key(cache, A, 0, 0) 与 kv_value(cache, A, 0, 0)：查 block[0] 得到块 0，分别加 0 与 10。两个地址收进 keys、values 两个指针数组的第 0 项。这一步只计算地址。
第 7 帧：compute 调用 attention(0, 0, state, keys, values)。attention 算出 k0、v0，写入 keys[0] 与 values[0] 指向的两行，再读这两行计算注意力。只有一个 token，所以 a0 = 1，o0 = a0 v0。o0 加到 state 上，feed_forward 再更新 state。
第 8 帧：第 1 层同样，查到的仍是块 0，偏移换成 20 与 30。一个 token 在全部层中的键与值都在它的一块中。
第 9 帧：两层都完成之后，compute 调用一次 kv_append(A)，n_cached 从 0 变为 1，块 0 从预留变为有效。kv_append 只修改这一个字段。compute 返回时，state 是 t0 经过最后一层之后的向量。
第 10 帧：t1，pos = 1。第 0 层收集两项：keys 是块 0 + 0 与块 1 + 0，values 是块 0 + 10 与块 1 + 10。attention 先把 k1、v1 写入最后一项指向的块 1。
第 11 帧：attention 读两项。attn_scores 计算 q1·k0 与 q1·k1，除以 √d，softmax 得到 a0、a1。attn_values 计算 o1 = a0 v0 + a1 v1，与注意力公式中 pos = 1 的情况相同。k0、v0 直接取自块 0。
第 12 帧：第 1 层同样，偏移是 20 与 30。两层完成后 kv_append 把 n_cached 变为 2。
第 13 帧：t2 重复同样的过程：每层收集 3 项，写块 2，读块 0、1、2，最后 n_cached 变为 3。消息已全部计算。
第 14 帧：next_token 由 t2 最后一层之后的 state 选出回答的第一个 token t3。t3 是模型的输出，这时还没有 k3、v3：block[3] 指向的块 3 仍是预留。t3 作为输入再经过一次模型，才有 k3、v3。
第 15 帧：框架对 t3 调用 compute，k3、v3 写入块 3，n_cached 变为 4，预留的块都已有效。
第 16 帧：next_token 输出 t4，回答达到 2 个 token，本轮结束。文本有 5 个 token，n_cached 是 4：t4 是最后一个输出，还没有作为输入，所以池中没有 k4、v4，block[4] 是 NULL。t4 在下一轮作为输入时，才算出它的键与值。
每次调用 compute 时，pos 都等于 n_cached，并且小于 capacity：kv_reserve 已经为这一轮会作为输入的每个 token 分到了一块。
下一页看同一个会话的下一次请求怎样找到这 4 块。
""")


# 上一页结束时 A 有 4 块、t4 只输出。本页用另一段动画走完下一次请求：先交代 B 的一轮（块 4、5），再按 id
# 找到原来的项，kv_reserve 只补 3 块、原有 4 项不动，compute 按块表收集第 0、1、2、3、6 块中的各行。
# 会话 B 用来显示按 id 查找、last_use 与不连续的块。内存不足留给下一小节。
def kv_next(p):
    p.title("下一次请求的查找与继续计算")
    _kv_slide(p, """
会话 A 又收到 1 个 token：按块表找到上一轮的块，从第 `n_cached` 个 token 继续计算。
""")
    p.frames(*(f"assets/kv-next-{k}.svg" for k in range(1, 14)), width_px=1120)
    p.notes("""
共 13 帧，各区域与第一次请求的图相同。
第 1 帧先交代两轮之间会话 B 的一轮。B 的消息只有 b0。kv_open(cache, 1) 新建 session[1]，clock 变为 2。kv_reserve(cache, B, 1, 2) 数出会作为输入的 b0、b1，分到块 4、5。两次 compute 把它们的键与值写入这两块，b2 只输出。A 的块与块表都不变。
第 2 帧：A 的下一次请求到来。append_message 把新消息 t5 接在 t0 到 t4 之后，n_prompt = 6：它包含之前各轮的文本。真实的对话格式在消息之前还有结束上一个回答的标记，例子中省略。t4 在上一轮只输出，还没有 k4、v4。
第 3 帧：kv_open(cache, 0) 按 id 找到原来的 session[0] 并返回它，clock 变为 3，A 的 last_use 从 1 变为 3，其他字段不变。上一轮的键与值经三步找到：由 id 找到会话表中的一项，由 n_cached = 4 知道前 4 项有效，由 block[0] 到 block[3] 找到块 0、1、2、3。
第 4 帧：kv_reserve(cache, A, 6, 2) 数 token。会作为输入的是 t0 到 t5 与回答的第一个 token t6，这一轮结束时最多缓存这 7 个。t7 只输出，不占块。A 已有的 4 块继续使用，只缺 3 块。kv_num_free() 返回 6，足够。
第 5 帧：kv_alloc 调用 3 次。块 4、5 属于 B，编号最小的空闲块是 6、7、8。位图只有这 3 位改变，块表只填 block[4]、block[5]、block[6]。原有 4 项与块中的内容都不变。capacity 从 4 变为 7。从这里起 A 的块不连续，token 4 在块 6。
第 6 帧：pos = n_cached = 4，计算从 t4 继续。上一轮只输出的 t4 现在作为输入，compute 先调用 embed(t4, state)。
第 7 帧：第 0 层。compute 对 tok 取 0 到 4 各调用一次 kv_key 与 kv_value，按块表得到块 0、1、2、3、6，各加 0 与 10，收进 keys 与 values。
第 8 帧：attention 只写最后一项指向的 k4、v4，再读全部 5 项：softmax 得到 a0 到 a4，o4 = a0 v0 + … + a4 v4。前 4 项是上一轮留下的。5 行在池中不相邻，attention 只按两个指针数组读写，所以计算不受影响。
第 9 帧：第 1 层仍是这 5 块，各加 20 与 30：keys 是块 0、1、2、3、6 各 + 20，values 是各 + 30。同样只写最后一项，读全部 5 项。
第 10 帧：kv_append 把 n_cached 从 4 变为 5。此刻块 0、1、2、3、6 有效，块 7、8 预留，块 9、10、11 空闲。
第 11 帧：t5 重复同样的过程，写块 7，n_cached 变为 6。
第 12 帧：next_token 输出回答的第一个 token t6。它作为输入再经过 compute，k6、v6 写入块 8，n_cached 变为 7。
第 13 帧：next_token 输出 t7，本轮结束。t7 是最后一个输出，还没有作为输入，所以池中没有 k7、v7，它在下一轮作为输入时才算出。A 的下一次请求仍按 id、n_cached 与块表找到这 7 块，从 t7 继续。
接下来看所有会话共用的内存池与它的上限。
""")


# ==============================================================================
# 小节：内存上限与失效
# ==============================================================================

# 上一小节用 12 块的小例子走完分配与写入，每次都放得下。本页换一个 96 块、三个会话共用的例子，并在
# 页上给全依据：各会话已缓存的 token、last_use 与所占的块，B 这次请求的文本与回答，逐格数出的 44 个输入。
# 由此提出问题：B 缺 25 块，空闲 14 块。本页不出现答案。
def kv_memory(p):
    p.title("KV Cache 的内存与上限")
    _kv_slide(p, """
三个会话共用 96 块的内存池，每块保存一个 token 的键与值。
A、B、C 已分别缓存 42、19、21 个 token。现在 B 又收到一条消息。
""")
    _kv_figure(p, "kv-memory", 1120)
    p.highlight("B 还缺 25 块，空闲只有 14 块，怎么办？")
    p.notes("""
左上是现在的状态。n_cached 是已缓存的 token 数，每个 token 占一块。last_use 越大表示越近使用：A 是 2，B 是 3，C 是 4，所以 A 最久没有使用。A 在块 0–41，B 在块 42–60，C 在块 61–81。
下面是内存池，按比例画出，一格是一块，位图中每块一位。一块保存一个 token 在全部层中的键与值，比如真实模型有 28 层，一行是 1024 个 float，一块就是 2 × 28 × 1024 × 4 字节，即 224 KiB。三个会话共占用 82 块，空闲 14 块。
右上是 B 的这次请求，一格是一个 token。B 已有 20 个 token 的文本，前 19 个已缓存，第 20 个是上一次回答的最后一个 token，它只被输出，还没有键与值。新消息是 17 个 token，文本增长到 37 个。回答最多 8 个 token，前 7 个生成之后还要作为输入，第 8 个只输出。
所以会作为输入的是文本的 37 个与回答的前 7 个，共 44 个，各需要一块。B 已有 19 块，还缺 25 块，空闲的只有 14 块。下一页逐步看 kv_reserve 怎样处理 B 的这次请求。
""")


# 上一页的问题：B 缺 25 块，空闲 14 块。本页承接同一个例子，正文是规则，图中写明两个实参与 last_use 的含义，逐帧走完
# B 的这次请求在 lru 下的 kv_reserve：数出只缺 25 块 →
# 最终需要 107 块，真正超出上限 → 先检查可收回的预留（0 块）→ kv_victim 选出 A → kv_invalidate 释放 A →
# 只分配 25 块，B 原有的块不动。两种提前返回写在讲稿中。
def kv_reserve(p):
    p.title("内存不足时的 kv_reserve")
    _kv_slide(p, """
`kv_reserve` 只分配缺的块。空闲块不够时先收回预留，仍不够时才使其他会话失效。
""")
    p.frames(*(f"assets/kv-reserve-{k}.svg" for k in range(1, 7)), width_px=1120)
    p.highlight("被失效的会话继续时从第 0 个 token 起重新计算，选哪个会话失效？")
    p.notes("""
共 6 帧，仍是三个会话与 B 的这次请求，内存池的比例不变。最上面是 kv_reserve 的四个步骤，顺序不能交换：只分配缺的块，空闲块不够时先收回预留，仍不够时才使其他会话失效。内存池下面的长条是这次请求结束时需要的块数，用同一比例画出。
调用是 kv_reserve(cache, B, 37, 8)。37 是 B 的文本 token 数，即原有的 20 个与新消息的 17 个，8 是最多回答的 token 数。框架在调用它之前已用 kv_open 把 B 记为最近使用，所以 B 的 last_use 是原来最大的 4 加 1，即 5，比 A 的 2 与 C 的 4 都大。
第 1 帧：数出会作为输入的 token：文本的 37 个与回答的前 7 个，共 44 个，回答的第 8 个只输出。B 已有的 19 块继续使用，内容也继续有效，所以只需再分配 25 块。
第 2 帧：44 不超过内存池的 96 块，B 单独放得下。空闲 14 块，少于 25 块。这次请求结束时三个会话共需要 A 42 + B 44 + C 21 = 107 块，比池中的 96 块多 11 块，是真正超出了上限，至少一个会话要失去它的缓存。
第 3 帧：先收回所有其他会话预留而未用的块。这样的块是块表中第 n_cached 项起、capacity 之内的项，收回它们不丢失任何算好的键与值。回答因结束符提前结束时，会留下这样的块，比如按最多回答 8 个 token 预留了块，模型只回答 3 个 token 就给出了结束符。这里 A、C 的 capacity 都等于 n_cached，可收回 0 块。空闲块足够时，这一步不进行。
第 4 帧：仍不够，才调用 kv_victim(cache, B)。候选是有块并且不是 B 的会话，即 A 与 C。最近最少使用（lru）选 last_use 最小的 A（2，C 是 4）。
第 5 帧：kv_invalidate(A) 逐块调用 kv_free 释放块 0–41，块表项置为 NULL，capacity 与 n_cached 归零，id 与 last_use 保留。空闲从 14 块变为 56 块，不少于 25 块，失效到此停止。
第 6 帧：调用 25 次 kv_alloc，得到块 0–24，按 token 的先后填入 B 的 block[19] 到 block[43]，capacity 变为 44，返回 1。B 原有的块 42–60 的地址与内容都不变。n_cached 仍是 19，这 25 个 token 随后由 compute 逐个写入。在用 65 块。
kv_reserve 还有两种提前返回的情形，都不修改任何数据。第一种：capacity 已经容得下这次请求会缓存的全部 token，直接返回 1。第二种：这次请求要缓存的 token 数超过池的总块数或块表的 512 项，其他会话全部失效也放不下，返回 0，表示这次请求无法处理。
被失效的 A 再收到请求时 n_cached 为 0，它已缓存过的 42 个 token 全部要重新计算。选哪个会话失效决定了之后重新计算多少，下一页比较两种策略。
""")


# 上一页的问题：选哪个会话失效。本页先画出相同的起点与随后依次到来的三次请求（B、A、C），再比较两种策略：
# lru 失效 4 次、重新计算 63 个 token（A 的请求先 C 后 B），smallest 失效 3 次、重新计算 21 个。
def kv_policies(p):
    p.title("lru 与 smallest 的失效")
    _kv_slide(p, """
`lru` 使最久没有使用的会话失效，`smallest` 使缓存 token 最少的会话失效。
""")
    _kv_figure(p, "kv-policies", 1120)
    p.highlight("重新计算的 token 使运行时间增加多少？")
    p.notes("""
最上面是两种策略共同的条件。起点是 96 块中 A、B、C 分别缓存 42、19、21 个 token，空闲 14 块，按最近使用的先后，A 最久，其次是 B，最近的是 C。随后依次到来 B、A、C 的三次请求，每次最多回答 8 个 token，实际都是 8 个，会作为输入的是文本与回答的前 7 个：B 是 37 + 7 = 44，A 是 60 + 7 = 67，C 是 36 + 7 = 43，即各自最多需要的块数。发来请求的会话先成为最近使用的会话，被失效的只能是其他会话。
下面每一条是一次请求结束时的内存池，各段画在块的位置上，竖线是上限 96 块，段下方的色条表示属于哪个会话。每一条上方左边是这次请求缺几块、开始时空闲几块，右边红字是被失效的会话。这三次请求都没有可收回的预留。
lru（最近最少使用）：B 的请求使 A 失效（last_use 2，C 是 4）。A 的请求到来时 A 已没有块，要 67 块，空闲 31 块。候选是 C 与 B，C 更久没有使用，先使 C 失效，空闲 52 块，仍不够，再使 B 失效，空闲 96 块。A 重新计算 42 个 token，再计算新的 25 个。C 的请求要 43 块，空闲 29 块，唯一的候选是 A，C 重新计算 21 个。共失效 4 次，重新计算 63 个 token。
smallest（最小缓存优先）：B 的请求在 A（42）与 C（21）中选缓存最少的 C，空闲 35 块，不少于缺的 25 块。A 的请求到来时 A 仍在缓存中，只缺 25 块，空闲 10 块，唯一的候选是 B。C 的请求要 43 块，空闲 29 块，使 A 失效，C 重新计算 21 个。共失效 3 次，重新计算 21 个 token。n_cached 相同时，smallest 选 last_use 较小的会话。
对这三次请求，smallest 重新计算的 token 是 lru 的三分之一。结果取决于之后的请求：最后被失效的会话如果再继续一轮，还要重新计算它的全部 token，比如两种策略最后都使 A 失效，A 再继续时要重新计算 67 个。
两种策略你都需要实现。lru 是推理系统常用的默认策略，smallest 每次选择当前缓存最少的会话。下一页看重新计算对运行时间的影响。
""")


# 上一页比较了两种策略重新计算的 token 数。本页给出它们对运行时间的影响，并在页上写明测的是什么：真实模型，
# 三个会话从空缓存开始依次发来的 7 次请求（A A B C B A C），各行按含义命名。时间与计数取自
# build/scratch-kvblocks/handoff/runs.md。上限放得下全部会话时没有失效，时间最短。
def kv_run_time(p):
    p.title("运行时间与内存上限")
    _kv_slide(p, """
用真实模型测量：A、B、C 三个会话从空缓存开始，按图中的先后发来 7 次请求，每次回答 8 个 token。
""")
    _kv_figure(p, "kv-run-time", 1120)
    p.notes("""
图的最上面是测量的内容：A、B、C 三个会话按 A A B C B A C 的先后发来 7 次请求，模型是 28 层的真实模型，缓存从空开始，每次最多回答 8 个 token，实际都是 8 个。下面是四次测量，各运行一次，条的长度按时间画出，右边是经过模型的 token 数。四次的回答逐字节相同。时间随硬件与负载而异。
每次请求下面的黑字是这次请求之后该会话缓存的 token 数，即文本与回答的前 7 个，每个 token 占一块。三个会话最后共缓存 67 + 44 + 43 = 154 个 token。内存池有 182 块时全部会话都放得下，没有失效，154 个 token 各经过模型一次，用时 181.9 秒，两种策略在这时相同。
内存池只有 96 块时，kv_reserve 要使会话失效。最近最少使用多经过模型 63 个 token，共 217 个，用时 272.7 秒。最小缓存优先多 21 个，共 175 个，用时 220.2 秒。每个 token 经过模型时要与 28 层的全部权重相乘，所以使用 KV Cache 时，时间主要由经过模型的 token 数决定。
不使用 KV Cache 时也只有 154 个 token 经过模型，但每一步在每一层重新计算之前全部 token 的键与值。红字是每次请求重新计算的次数，一次指一个之前的 token 在 28 层中各算一行键与一行值，比如第一次请求的 20 个 token 依次重新计算 0、1、……、19 个之前的 token，共 190 次。会话越长，每一步重新计算得越多，7 次请求共 4060 次，用时 616.8 秒。
这种做法为重新计算保存各层的输入。一个 token 在一层的输入有 2048 个数，与它在这一层的一行键加一行值一样多，所以保存一个 token 的输入与缓存它的键和值占用同样多的内存。
""")
