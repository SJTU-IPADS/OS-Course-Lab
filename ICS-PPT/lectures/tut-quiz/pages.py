"""Quiz 题目：页面函数。

Quiz 第 1 ~ 5 题的题面。页面的顺序与 lecture.py 相同，函数名把下划线换成连字符就是页面的 id。
辅助函数与常量以 _quiz_ 开头。下面的一段注释写明讲义的设计。
"""


# Quiz 题目。
#
# 本讲义是 Quiz 第 1 ~ 5 题的题面。题号、分值、文字、代码、表与图照原卷，不加提示，不放答案。
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
