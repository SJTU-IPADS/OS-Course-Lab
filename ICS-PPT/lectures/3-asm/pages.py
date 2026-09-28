"""ICS 第 3 章：程序的机器级表示与执行机制。

页面文案逐字取自同目录的 PPTContents.md（第 01 ~ 77 页），不改写任何表述。
开头「问题提出」一页的任务与悬念两条由作者另行给出，不在 PPTContents.md 中。
一页排不下时按语义拆成几页，拆出的页沿用同一个页面标题；
每页「配图建议」对应的图由 diagrams/ 下的同名脚本生成，放在正文之后：
与正文放得进一页时放在同一页，否则放在同标题的下一页。
过渡页（第 09、26、39、56、74 页）写成 lecture.bridge，见 lecture.py：只放页面标题。

排版约定：
- 原文的粗体逐处写明，slide() 因此关闭自动加粗。
- 原文的表格写成 p.table。
- 汇编清单取自 gcc 15.2 的真实输出（-fcf-protection=none，不含 endbr64），制表符原样保留。
  演示命令用 examples/asm.sed 去掉汇编伪指令，只留下指令与跳转标号。
- 演示命令在本节目录下执行，所以以 `cd examples` 开头。
"""


def slide(p, md, **kw):
    """PPTContents.md 的页面文案。粗体由原文逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def figure(p, name, width):
    """配图建议对应的图，由 diagrams/ 下的同名脚本生成。"""
    return p.image(f"assets/{name}.svg", width_px=width)


def problem(p):
    p.title('问题提出')
    slide(p, r"""
- 大模型想要预测下一个词，最重要的算子就是 $w$ 和 $x$ 的内积。推理程序 `ollama` 负责完成这些计算。但现代 CPU 不懂什么是 `for` 循环，什么是数组下标，什么是浮点数乘法。硅片上只有数以亿计的晶体管和电路开关，它是怎么把 `ollama` 执行完并算出结果的？

**本节回答以下问题**：
1. 可执行程序 `ollama` 与权重文件 `llama3.gguf` 在运行前存放在哪里，运行时如何进入内存并由 CPU 执行？
2. 可执行程序 `ollama` 是如何构建出来的？自然语言或高级语言编写的程序可以直接由 CPU 执行吗？
3. 同一个 `ollama` 程序可以在不同的 CPU 上执行吗？Windows / Linux / macOS 等操作系统可以使用同一个程序文件吗？
""")
    p.notes('以 ollama 的推理过程提出全节要回答的问题：程序如何在由晶体管构成的 CPU 上执行。')


def system_view(p):
    p.title('系统执行：大模型推理的端到端硬件协同')
    slide(p, r"""
**计算机核心硬件实体**：
- **外部存储（Disk）**：非易失性介质，持久化存放可执行程序 `ollama` 与模型权重文件 `llama3.gguf`。
- **主存储器（DRAM）**：易失性介质，常驻操作系统内核（OS Kernel），受 OS 统一管理分配。
- **中央处理器（CPU）**：运算核心，包含程序计数器（PC）、寄存器堆、控制单元与算术逻辑部件（ALU）。

**底层二进制统一性（程序与数据都以二进制形式存储）**：
- 磁盘上的可执行程序 `ollama` 是机器指令二进制流。
- 磁盘上的模型文件 `llama3.gguf` 是参数权重二进制流。
- **二者都是比特序列，存储介质不区分指令与数据**。
""")
    p.notes('现代计算机整机系统执行模型——以大模型推理引擎（Ollama）的运行过程为例。')


def system_view_2(p):
    p.title('系统执行：大模型推理的端到端硬件协同')
    slide(p, r"""
**执行的四个阶段**：
1. **程序装载**：用户在 Shell 键入命令，OS 将 `ollama` 机器指令由 Disk 载入 DRAM 代码区。
2. **取指运行**：CPU 从 DRAM 取出 `ollama` 指令流执行，产生文件读写与内存映射系统调用。
3. **数据加载**：操作系统响应系统调用，驱动总线将权重数据从 Disk 读入 DRAM 数据缓存区。
4. **运算产出**：结合输入 Token 向量 $x$ 与模型权重 $w$，CPU 循环将数据加载到内部寄存器，ALU 完成计算并输出预测 Token。
""")


def system_view_fig(p):
    p.title('系统执行：大模型推理的端到端硬件协同')
    figure(p, "system-view", 1120).footnote('照片从左到右来自 Wikimedia Commons 的 Evan-Amos（CC BY-SA 3.0）、D-Kuru（CC BY-SA 4.0）、PantheraLeo1359531（CC BY 4.0）、Eric Gaba（CC BY-SA 4.0），经裁剪缩放。')


def compile_mapping(p):
    p.title('编译映射：从 C 源码到二进制机器指令')
    p.gap(4)
    slide(p, r"""
**本节主线算子**：
- 大模型计算的核心数学算子：向量内积 `dot_product`，这段代码如何才能被 CPU 执行？
""")
    p.code('c', """int dot_product(const int *w, const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += w[i] * x[i];  // <-- 全节核心主线
    }
    return sum;
}""")
    slide(p, r"""
**同一段代码的三种表示**：
1. **高级语言（C）**：面向程序员的控制结构与变量符号抽象。
2. **汇编指令（Assembly）**：通过 `objdump -d` 反汇编或 `gcc -S` 直接生成，与机器码对应。
3. **目标机器码（Object Code）**：经 `gcc -c` 生成的纯二进制目标文件（ELF），通过 `hexdump` / `xxd` 观察，呈现为十六进制机器字节流。
""")
    p.notes('全节核心主线程序的提出，以及从高级 C 源码到二进制机器码与反汇编的映射概览。')


def compile_mapping_fig(p):
    p.title('编译映射：从 C 源码到二进制机器指令')
    figure(p, "compile-mapping", 1120)


def toolchain(p):
    p.title('编译流程：预处理、编译、汇编与链接四阶段')
    slide(p, r"""
**GNU 编译系统四阶段**：
1. **预处理（Preprocess）**：
   - 命令：`gcc -E dot.c -o dot.i`
   - 动作：展开以 `#` 开头的预处理指令（`#include`, `#define`），剔除注释，生成纯 C 文本文件 `.i`。
2. **编译（Compile）**：
   - 命令：`gcc -S dot.i -o dot.s`
   - 动作：进行词法分析、语法分析等等，将高级语义翻译为机器级汇编文本 `.s`。
3. **汇编（Assemble）**：
   - 命令：`gcc -c dot.s -o dot.o`
   - 动作：将汇编助记符翻译成二进制机器指令，打包为 ELF 格式可重定位目标文件 `.o`。
""")
    p.notes('C 语言源码到可执行文件的四个标准阶段、各阶段生成文件与对应 GCC 参数。')


def toolchain_2(p):
    p.title('编译流程：预处理、编译、汇编与链接四阶段')
    slide(p, r"""
4. **链接（Link）**：
   - 命令：`gcc dot.o main.o -o dot_product`
   - 动作：解析跨文件符号引用，重定位函数与全局变量地址，合并代码段与数据段，生成最终可执行二进制文件。
""")
    p.demo('四个阶段逐步执行',
           """cd examples
gcc -E dot.c -o dot.i
gcc -S dot.i -o dot.s
gcc -c dot.s -o dot.o
gcc -c main.c -o main.o
gcc dot.o main.o -o dot_product
./dot_product; echo $?""",
           output="""70""",
           files=['examples/dot.c', 'examples/main.c'])
    p.notes('main.c 以 w = {1, 2, 3, 4}、x = {5, 6, 7, 8} 调用 dot_product，返回值 1·5 + 2·6 + 3·7 + 4·8 = 70 作为进程的退出状态，由 echo $? 打印。')


def toolchain_fig(p):
    p.title('编译流程：预处理、编译、汇编与链接四阶段')
    figure(p, "toolchain", 1120)
    p.aside("本课程统一采用 Linux / GNU 工具链默认的 AT&T 语法，对于 Intel 语法，可在课外自行学习")


def isa_contract(p):
    p.title('体系契约：指令集架构与微架构实现的解耦')
    slide(p, r"""
程序编译之后得到的是二进制机器码。但 CPU 执行需要能够认识这段二进制所代表的指令，因此二进制编码与 CPU 指令之间的约定必不可少。

**指令集架构（ISA: Instruction Set Architecture）的定义**：
- ISA 是软件与硬件电路之间达成的接口契约规范，在长期演进中保持严格的向后兼容性。
- **ISA 规范的内容**：
  - **程序员可见状态**：程序计数器（PC / `%rip`）、16 个通用寄存器、条件码寄存器（RFLAGS）、连续虚拟内存空间；
  - **指令编码与格式**：每条机器指令的二进制操作码、操作数编码规则与指令长度；
  - **支持的数据类型与寻址模式**。
""")
    p.notes('指令集架构（ISA）的契约定义、程序员可见状态，以及 ISA 与微架构实现的解耦关系。')


def isa_contract_2(p):
    p.title('体系契约：指令集架构与微架构实现的解耦')
    slide(p, r"""
**ISA 与微架构（Microarchitecture）的解耦**：
- **ISA 是接口（Interface）**：规定处理器能执行什么；
- **微架构是实现（Implementation）**：规定处理器在物理电路层面如何执行。
- **现实案例**：
  - Intel Core i9-11900H（Tiger Lake 架构）与 AMD Ryzen 7000（Zen 4 架构）运行同一套 x86-64 ISA；
  - 两者可无缝执行同一个可执行文件，但在内部的流水线级数、乱序调度窗口、分支预测器和执行端口数量等微架构设计上不同。
""")


def isa_contract_fig(p):
    p.title('体系契约：指令集架构与微架构实现的解耦')
    figure(p, "isa-contract", 1120)


def single_mac(p):
    p.title('单次乘加：w[0] * x[0] 的执行过程')
    slide(p, r"""
**点积代码切片**：
""")
    p.code('c', """sum += w[0] * x[0];""")
    slide(p, r"""
**执行动作分解**：
1. 找到 `w[0]` 在内存中的位置；
2. 找到 `x[0]` 在内存中的位置；
3. 把数据从内存读出来；
4. 执行乘法运算；
5. 把乘积累加到 `sum`。

**核心硬件约束**：
- 运算器（ALU）无法直接在内存中就地执行乘法。
- 数据如何从内存送到 ALU 完成算术运算？
""")
    p.notes("""
第一部分主线引入。
先忽略循环与函数外壳，聚焦最核心的单次乘加操作。
""")


def single_mac_fig(p):
    p.title('单次乘加：w[0] * x[0] 的执行过程')
    figure(p, "single-mac", 1120)


def von_neumann(p):
    p.title('冯·诺依曼架构：运算部件与存储介质的物理分离')
    slide(p, r"""
**冯·诺依曼架构核心特征**：运算器（ALU）与存储器（Memory）在物理实体上分离。

**为什么不能在主存中直接计算**：
- DRAM 内存芯片由单晶体管-电容阵列（1T1C）构成，专为高密度存储电荷设计，不具备算术逻辑门电路。
- 乘法器需要复杂的门电路阵列，无法集成在 DRAM 上。

**结论**：计算只能在 CPU 内部进行，必须将数据从内存读取到 CPU 内部的寄存器。
""")
    p.notes('冯·诺依曼架构计算与存储分离的物理硬件约束。')
    figure(p, "von-neumann", 1120)


def visible_state(p):
    p.title('硬件状态：程序员可见状态与寄存器堆设计约束')
    slide(p, r"""
**程序员可见状态（Programmer-Visible State）**：
- 指令集架构直接暴露给软件程序、能够被指令读取和修改的全部硬件实体：
  - **程序计数器（PC / `%rip`）**：存放下一条取指执行指令的虚拟地址；
  - **通用寄存器堆（Register File）**：16 个 64 位高速存储单元；
  - **条件标志位寄存器（RFLAGS）**：存放最近一次算术逻辑运算的状态结果；
  - **虚拟内存空间**：通过地址总线访问的代码、全局变量与栈内存。
""")
    p.notes('程序员可见状态的定义，以及 CPU 内部通用寄存器数量受到硬件严格限制的原因。')


def visible_state_2(p):
    p.title('硬件状态：程序员可见状态与寄存器堆设计约束')
    slide(p, r"""
**通用寄存器数量为什么只有 16 个**：
- **访问速度要求**：寄存器是 CPU 内部由静态触发器直接搭建的电路，单周期内完成访问；
- **指令编码字段限制**：指令中寄存器字段的位数有限，在定长字段中指定一个寄存器，16 个寄存器只需 4 位二进制（$2^4 = 16$），若扩展到上千个，指令编码将急剧膨胀；
- **硬件布线与延迟瓶颈**：多端口读写逻辑（如超标量 CPU 需要多个读写端口），寄存器堆面积主要随端口数平方增长；寄存器数量过多会导致内部走线与多路选择器延迟增大，降低最高工作频率。
""")


def visible_state_fig(p):
    p.title('硬件状态：程序员可见状态与寄存器堆设计约束')
    figure(p, "visible-state", 1120)


def register_slices(p):
    p.title('寄存器切片：16 个通用寄存器及其命名规则')
    slide(p, r"""
**16 个通用寄存器（64 位宽）及其子寄存器切片**：
- **前 8 个经典寄存器**（源自 8086/IA32 历史兼容演进）：
  - `%rax` $\to$ 32位 `%eax` $\to$ 16位 `%ax` $\to$ 8位高 `%ah` / 低 `%al`；
  - `%rbx` $\to$ `%ebx` $\to$ `%bx` $\to$ `%bh` / `%bl`；
  - `%rcx` $\to$ `%ecx` $\to$ `%cx` $\to$ `%ch` / `%cl`；
  - `%rdx` $\to$ `%edx` $\to$ `%dx` $\to$ `%dh` / `%dl`；
  - `%rsi` $\to$ `%esi` $\to$ `%si` $\to$ 低 8 位 `%sil`；
  - `%rdi` $\to$ `%edi` $\to$ `%di` $\to$ 低 8 位 `%dil`；
  - `%rbp` $\to$ `%ebp` $\to$ `%bp` $\to$ 低 8 位 `%bpl`；
  - `%rsp` $\to$ `%esp` $\to$ `%sp` $\to$ 低 8 位 `%spl`。
""")
    p.notes('x86-64 体系 16 个通用寄存器的高低位嵌套切片命名体系及核心约定用途。')


def register_slices_2(p):
    p.title('寄存器切片：16 个通用寄存器及其命名规则')
    slide(p, r"""
- **后 8 个新增寄存器**（x86-64 扩展引入）：
  - 64 位完整：`%r8`, `%r9`, `%r10`, `%r11`, `%r12`, `%r13`, `%r14`, `%r15`；
  - 32 位切片（加后缀 `d`）：`%r8d` ~ `%r15d`；
  - 16 位切片（加后缀 `w`）：`%r8w` ~ `%r15w`；
  - 8 位切片（加后缀 `b`）：`%r8b` ~ `%r15b`。

**核心用途（将在后续介绍）**：
- `%rsp`：栈指针（Stack Pointer）；
- `%rax`：函数返回值（Return Value）；
- `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9`：用于传递前 6 个整型与指针参数。
""")


def register_slices_fig(p):
    p.title('寄存器切片：16 个通用寄存器及其命名规则')
    figure(p, "register-table", 1120)


def width_write(p):
    p.title('数据加载：movl 指令的内存读取语义与约束')
    slide(p, r"""
**写入不同子寄存器的硬件行为规则**：
1. **写 32 位子寄存器（如 `%eax`, `%r8d`）**：
   - **硬件行为**：CPU 在写入低 32 位的同时，**自动将高 32 位全部清零**。
   - 例如：若 `%rax` 原为 `0xFFFFFFFFFFFFFFFF`，执行 `movl $1, %eax` 后，`%rax` 的值变为 `0x0000000000000001`。
2. **写 8 位或 16 位子寄存器（如 `%al`, `%ax`）**：
   - **硬件行为**：CPU 只改变目标所在的低 8 位或低 16 位，**高位剩余的所有比特保留原值不变**。
   - 例如：若 `%rax` 原为 `0xFFFFFFFFFFFFFFFF`，执行 `movb $1, %al` 后，`%rax` 的值变为 `0xFFFFFFFFFFFFFF01`。
""")
    p.notes('向子寄存器写入数据时，x86-64 硬件对待高位部分截然不同的处理规则。')


def load_example_state(p):
    """课堂示例三张幻灯片共用的内存与寄存器初值。"""
    p.table([
        ['字节', '`0x78`', '`0x56`', '`0x34`', '`0x12`', '`0xF0`', '`0xFF`', '`0xFF`', '`0xFF`'],
    ], headers=['地址', '`0x100`', '`0x101`', '`0x102`', '`0x103`', '`0x104`', '`0x105`', '`0x106`', '`0x107`'])
    slide(p, r"""
寄存器初值：`%rax = 0x100`，`%rcx = 0x1`，`%rdx = 0xFFFFFFFFFFFFFFFF`。
""")


def load_example(p):
    p.title('练习：内存字节、寻址与数据加载')
    slide(p, r"""
**已知**：x86-64 采用小端序，多字节数据的低位字节存放在低地址。从地址 `0x100` 开始的 8 个字节为：
""")
    load_example_state(p)
    slide(p, r"""
**问题一**：按 32 位读取操作数 `$0x104`，`0x104`，`(%rax)`，`2(%rax)`，`(%rax,%rcx,4)` 的值。

**问题二**：从上述初值出发，分别执行下列指令，写出每条指令执行后 `%rdx` 的值：
- `movl (%rax), %edx`
- `movb (%rax), %dl`
- `movw 2(%rax), %dx`
- `movq (%rax), %rdx`
""")
    p.notes('按小端序由字节序列读出多字节数值，结合寻址模式与位宽规则计算操作数与寄存器的值。')


def load_example_2(p):
    p.title('练习：内存字节、寻址与数据加载')
    load_example_state(p)
    slide(p, r"""
**问题一解答**：
""")
    p.table([
        ['`$0x104`', '立即数寻址', '无', '`0x104`'],
        ['`0x104`', '绝对寻址', '`0x104`', '`0xFFFFFFF0`'],
        ['`(%rax)`', '间接寻址', '`0x100`', '`0x12345678`'],
        ['`2(%rax)`', '基址+偏移寻址', '`0x100 + 2 = 0x102`', '`0xFFF01234`'],
        ['`(%rax,%rcx,4)`', '比例变址寻址', '`0x100 + 1 × 4 = 0x104`', '`0xFFFFFFF0`'],
    ], headers=['操作数', '寻址类型', '有效地址', '值'])


def load_example_3(p):
    p.title('练习：内存字节、寻址与数据加载')
    load_example_state(p)
    slide(p, r"""
**问题二解答**：
""")
    p.table([
        ['`movl (%rax), %edx`', '`78 56 34 12`', '`0x0000000012345678`', '写 32 位，高 32 位清零'],
        ['`movb (%rax), %dl`', '`78`', '`0xFFFFFFFFFFFFFF78`', '写 8 位，其余位保留'],
        ['`movw 2(%rax), %dx`', '`34 12`', '`0xFFFFFFFFFFFF1234`', '写 16 位，其余位保留'],
        ['`movq (%rax), %rdx`', '`78 56 34 12 F0 FF FF FF`', '`0xFFFFFFF012345678`', '写满 64 位'],
    ], headers=['指令', '读取的字节', '执行后 `%rdx`', '依据'])


def effective_address(p):
    p.title('有效地址：数组下标与基址步长的数学映射')
    slide(p, r"""
**高级语言表达**：`w[i]`

**内存连续排布模型**：
- 数组名 `w` 代表首元素起始地址（Base Address）。
- 下标 `i` 为元素逻辑偏移。
- 每个 `int` 元素占用 4 个字节（Scale Factor = 4）。

**有效地址计算公式**：
 $$\text{Address}(w[i]) = \text{Base}(w) + i \times 4$$
""")
    p.notes('高级语言中的数组下标表达式如何映射为内存虚拟地址。')
    figure(p, "array-address", 1120)


def addressing_modes(p):
    p.title('寻址模式：x86-64 的八种操作数寻址形态')
    slide(p, r"""
**操作数的两大来源**：
1. 寄存器操作数：直接使用寄存器中的数值（`r_a`）；
2. 立即数操作数：指令内部硬编码的常数（`$Imm`）；

**常见操作数寻址形式**：
""")
    p.table([
        ['立即数寻址', '`$Imm`', '无（直接取常数值）', '常数赋值与初始化'],
        ['寄存器寻址', '`r_a`', '无（寄存器内部）', '寄存器间传递数据'],
        ['绝对寻址', '`Imm`', '$\\text{Imm}$', '少数特殊绝对寻址场景'],
        ['间接寻址', '`(r_a)`', '$r_a$', '指针解引用 `*ptr`'],
    ], headers=['寻址类型', '汇编语法', '有效地址计算公式', '典型对应场景'])
    p.notes('x86-64 指令集常见的操作数分类、内存寻址模式及 RIP 相对寻址。')


def addressing_modes_2(p):
    p.title('寻址模式：x86-64 的八种操作数寻址形态')
    p.table([
        ['基址+偏移寻址', '`Imm(r_b)`', '$r_b + \\text{Imm}$', '结构体成员、局部栈变量'],
        ['变址寻址', '`(r_b, r_i)`', '$r_b + r_i$', '字节数组 `b[i]`'],
        ['比例变址寻址', '`Imm(r_b, r_i, s)`', '$\\text{Imm} + r_b + r_i \\times s$', '`int` 数组：每个元素占 4 字节，比例因子 $s = 4$'],
        ['RIP 相对寻址', '`Imm(%rip)`', '$\\text{Next\\_RIP} + \\text{Imm}$', '访问全局变量与位置无关代码'],
    ], headers=['寻址类型', '汇编语法', '有效地址计算公式', '典型对应场景'],
        widths=[18, 21, 23, 38])
    slide(p, r"""
**说明与约束**：
- x86-64 默认生成位置无关代码，访问全局变量常使用 `%rip` 相对寻址，绝对寻址在现代代码中极少出现；
- 比例因子只能取 $s \in \{1, 2, 4, 8\}$，在指令编码的 SIB 字节中仅占用 2 位。
""")


def addressing_modes_fig(p):
    p.title('寻址模式：x86-64 的八种操作数寻址形态')
    figure(p, "addressing-modes", 1120)


def lea_agu(p):
    p.title('地址计算：lea 指令与硬件 AGU 的电路支持')
    slide(p, r"""
**硬件地址生成单元（AGU: Address Generation Unit）**：
- CPU 内部包含独立的组合逻辑电路，在单周期内完成有效地址的乘法移位与三数相加。

**`lea`（Load Effective Address）指令**：
- 语法：`leal Disp(Base, Index, Scale), Dest`
- **硬件机制**：利用 AGU 计算出有效地址后，**直接把计算得到的数值写入目的寄存器，不发生任何内存总线访问**。

**课堂快速计算练习**：
- 假设 `%rdi = 0x2000`，`%rcx = 3`，计算指令 `leal 8(%rdi,%rcx,4), %eax` 执行后 `%eax` 的数值。
""")
    p.notes('地址生成单元（AGU）的硬件电路，以及 `lea` 指令作为纯算术优化手段的机制。')


def lea_agu_fig(p):
    p.title('地址计算：lea 指令与硬件 AGU 的电路支持')
    slide(p, r"""
**课堂快速计算练习**：假设 `%rdi = 0x2000`，`%rcx = 3`，计算指令 `leal 8(%rdi,%rcx,4), %eax` 执行后 `%eax` 的数值。
""")
    figure(p, "agu", 1120)
    slide(p, r"""
**解答**：$0x2000 + 3 \times 4 + 8 = 0x2000 + 0xC + 0x8 = 0x2014$（$12 + 8 = 20 = \text{0x14}$）。
""")


def movl_load(p):
    p.title('数据加载：movl 指令的内存读取语义与约束')
    slide(p, r"""
**加载指令**：`movl`（Move Longword，32 位）

**语义解剖**：
- 指令格式：`mov S, D`，带两个操作数，把源操作数 S 的值复制到目的操作数 D（AT&T 语法中源在前、目的在后）。
- 每个操作数属于以下三类之一：立即数（`$Imm`）、寄存器（`r_a`）、内存操作数（如 `(r_a)`，按寻址模式计算有效地址）。
- 本例 `movl (%rsi), %eax`：
  - 源操作数：内存操作数 `(%rsi)`（读取首元素 `x[0]`）
  - 目的操作数：通用寄存器 `%eax`

**执行过程**：
- 地址送到访存单元，经地址转换后访问缓存或内存。
- 读取出的 32 位数据存入 `%eax`，其所在 64 位寄存器的高 32 位自动清零。
""")
    p.notes('`movl` 数据加载指令的语义及操作数约束规则。')


def movl_load_2(p):
    p.title('数据加载：movl 指令的内存读取语义与约束')
    slide(p, r"""
**`mov` 指令的操作数约束规则**：
1. **不能同时访问两处内存**：源与目的不可同时为内存地址（`movl (%rsi), (%rdi)` 是非法指令）；
2. **立即数不能作为目的操作数**；
3. 内存到内存的数据搬运，必须拆分为“内存 $\to$ 寄存器 $\to$ 内存”两条连续指令。
""")
    p.code('assembly', """movl	(%rsi), %eax""")
    figure(p, "load-path", 1120)


def extension(p):
    p.title('扩展传送：零扩展、符号扩展与 cltq 指令')
    slide(p, r"""
**不同位宽传递的两类扩展指令**：
1. **零扩展传送（Zero Extension）**：
   - `movzbl`（字节转双字）, `movzwl`（字转双字）；
   - 动作：将高位全部填充为 0，适用于无符号类型（`unsigned`）。
2. **符号扩展传送（Sign Extension）**：
   - `movsbl`（字节转双字）, `movswl`（字转双字）, `movslq`（双字转四字）；
   - 动作：将源操作数的最高位（符号位）复制填充到高位所有比特，从而保持有符号补码数值不变。

**专用符号扩展指令 `cltq`（Convert Long to Quad）**：
- 无显式操作数，专门将 `%eax` 符号扩展填充满整个 64 位的 `%rax`；
- 指令长度更短，实际执行效果与 `movslq %eax, %rax` 相同。
""")
    p.notes('不同位宽数据传送时的补零与符号位扩展规则，以及 `cltq` 指令的作用。')


def extension_fig(p):
    p.title('扩展传送：零扩展、符号扩展与 cltq 指令')
    figure(p, "extension", 1120)


def int_arith(p):
    p.title('整数算术：imull 与 addl 指令的执行')
    slide(p, r"""
**乘法指令：`imull`**：
- 格式：`imull (%rdi), %eax` 或 `imull %ecx, %eax`
- 语义：`%eax` $\leftarrow$ `%eax` $\times$ 源操作数
- 硬件执行：乘法器流水线计算两数乘积，乘积只保留低 32 位写入 `%eax`；根据前面介绍的位宽规则，写入 `%eax` 会将 `%rax` 的高 32 位自动清零。

**加法指令：`addl`**：
- 格式：`addl %ecx, %eax`
- 语义：`%eax` $\leftarrow$ `%eax` $+$ `%ecx`
- 硬件执行：ALU 内部全加器执行 32 位二进制加法，结果写回寄存器。
""")
    figure(p, "int-alu", 1120)
    p.notes('整数乘法与加法指令在 ALU 中的执行。')


def shift_ops(p):
    p.title('算术逻辑：一元、二元与移位运算指令体系')
    slide(p, r"""
**一元与二元运算指令体系**：
- **一元运算**：`incl D`（自增 1），`decl D`（自减 1），`negl D`（取相反数 $D \leftarrow -D$），`notl D`（按位取反）；
- **二元运算**：`addl S, D`，`subl S, D`（减法 $D \leftarrow D - S$），`andl S, D`，`orl S, D`，`xorl S, D`。

**移位操作指令（算术 vs 逻辑）**：
- **左移指令**：`sall` / `shll`（左移，低位补 0，二者等价）；
- **算术右移**：`sarl`（右移时复制最高符号位，保持有符号负数语义）；
- **逻辑右移**：`shrl`（右移时高位严格补 0，用于无符号数除以 $2^k$）。
""")
    p.notes('x86-64 体系中除加减法外的通用整数算术、按位逻辑与移位指令集全览。')


def shift_ops_3(p):
    p.title('算术逻辑：一元、二元与移位运算指令体系')
    slide(p, r"""
**实例：用移位代替乘除**（`gcc -O2 -mavx2` 编译 `dot.c`，进入向量循环前的四条指令）：
""")
    p.code('assembly', """shrl    $3, %edx               # unsigned iters = (unsigned)n / 8;
xorl    %eax, %eax             # long off = 0;
vpxor   %xmm1, %xmm1, %xmm1    # int vsum[8] = {0};
salq    $5, %rdx               # long bytes = (long)iters * 32;""")
    slide(p, r"""
- 执行前 `%edx` 保存元素总数 $n$；向量循环每轮处理 8 个 int32，共 32 字节；注释中的 C 代码与各条指令等价，变量名为讲解所取；
- `shrl $3, %edx`：将总元素数逻辑右移 3 位，相当于除以 $2^3 = 8$，得到向量循环的迭代数 `iters`；入口处已确认 $n > 0$，因此按无符号数右移结果正确；
- `salq $5, %rdx`：将 `iters` 左移 5 位，相当于乘以 $2^5 = 32$，得到向量循环处理的总字节数 `bytes`。写 `%edx` 时高 32 位已被清零，因此这里可以直接使用 64 位的 `%rdx`；
- 中间两条指令将字节偏移 `off`（`%rax`）与部分和 `vsum`（`%ymm1`）清零，将在后续介绍。
""")


def shift_ops_fig(p):
    p.title('算术逻辑：一元、二元与移位运算指令体系')
    slide(p, r"""
**同一位模式 `0xFFFFFFF0` 右移 2 位**：`sarl` 将源操作数解释为有符号数 -16，`shrl` 将源操作数解释为无符号数 4294967280。
""")
    figure(p, "shift", 1120)


def xor_strength(p):
    p.title('指令优化：xorl 寄存器清零与乘常数强度削减')
    slide(p, r"""
**惯用写法：`xorl %eax, %eax` 寄存器清零**：
- 任何数异或自身恒为 0（$A \oplus A = 0$）；
- **为什么不用 `movl $0, %eax`**：
  - 编码长度：`xorl %eax, %eax` 仅需 2 字节（`31 c0`），而 `movl $0, %eax` 占用 5 字节（`b8 00 00 00 00`）；
  - 硬件重命名优化：现代 CPU 译码器识别到自异或时，直接在寄存器别名表（RAT）中将其映射至物理零，**执行延迟为 0 周期，不占用 ALU 资源**。
""")
    p.notes('编译器在基础算术中常用的机器级优化惯用法：异或清零与乘常数强度削减。')


def xor_strength_2(p):
    p.title('指令优化：xorl 寄存器清零与乘常数强度削减')
    slide(p, r"""
**编译器优化：乘常数强度削减（Strength Reduction）**：
- 整数乘法 `imull` 耗时通常需 3 个时钟周期；
- 编译器会自动将常数乘法拆解为组合开销为 1 周期的 `lea` 与移位：
  - `x * 5` $\to$ `leal (%rax,%rax,4), %eax`（$x + x \times 4$）；
  - `x * 9` $\to$ `leal (%rax,%rax,8), %eax`（$x + x \times 8$）；
  - `x * 12` $\to$ `leal (%rax,%rax,2), %eax` 随后 `sall $2, %eax`。
""")
    figure(p, "xor-lea", 1120)


def mac_exercise(p):
    p.title('练习：从汇编推导 C 代码')
    slide(p, r"""
**从汇编推导 C 代码**：给定函数 `mac` 的汇编代码，推导其 C 函数体：
""")
    p.code('assembly', """mac:
	movl	(%rsi), %eax
	imull	(%rdi), %eax
	addl	%edx, %eax
	ret""")
    slide(p, r"""
**已知**：
- 函数原型：`int mac(const int *w, const int *x, int sum)`；
- 执行前，`%rdi` 保存指针 `w`，即 `w[0]` 的地址；`%rsi` 保存指针 `x`，即 `x[0]` 的地址；`%edx` 保存 `sum` 的值；
- 执行 `ret` 时，`%eax` 中的值即函数的返回值。
""")
    p.notes('课堂练习：已知寄存器内容，由单次乘加的汇编推导 C 函数体。')


def mac_exercise_2(p):
    p.title('练习：从汇编推导 C 代码')
    slide(p, r"""
**分析推导**：逐条对应的 C 代码如下：
""")
    p.code('assembly', """movl    (%rsi), %eax    # int t = x[0];
imull   (%rdi), %eax    # t = t * w[0];
addl    %edx, %eax      # t = t + sum;
ret                     # return t;""")
    slide(p, r"""
- `movl (%rsi), %eax`：以 `%rsi` 中的地址读取 4 字节，得到 `x[0]`；
- `imull (%rdi), %eax`：以 `%rdi` 中的地址读取 `w[0]`，与 `%eax` 相乘，乘积的低 32 位写回 `%eax`；
- `addl %edx, %eax`：加上 `sum`，结果即返回值；
- 对应的 C 函数体：`return sum + w[0] * x[0];`。
""")


def memory_operand(p):
    p.title('拓展：x86 算术指令的单内存操作数约束')
    slide(p, r"""
**观察**：
- 上一页的清单中，`imull (%rdi), %eax` 的源操作数是一个内存地址。

**硬件执行机制**：
- x86-64 属于复杂指令集（CISC），允许双操作数算术指令的源操作数为内存地址。
- 硬件执行时，访存单元先将内存数据读取出来，再直接送往 ALU 与 `%eax` 相乘。

**单内存操作数约束**：
- x86 单条指令中**最多只能包含一个内存操作数**。
""")
    p.notes('x86 允许算术指令包含内存操作数的机制、单内存限制与 CISC/RISC 特征。')


def memory_operand_2(p):
    p.title('拓展：x86 算术指令的单内存操作数约束')
    slide(p, r"""
**架构对比（CISC vs RISC）**：`return w[0] + x[0];` 分别编译为 x86-64 与 ARM64 汇编：
""")
    p.code('text', """# x86-64 (gcc -Og)              // ARM64 (aarch64-linux-gnu-gcc -Og)
movl    (%rsi), %eax            ldr     w2, [x0]
addl    (%rdi), %eax            ldr     w0, [x1]
ret                             add     w0, w2, w0
                                ret""")
    slide(p, r"""
- 指针 `w` 与 `x` 在 x86-64 中位于 `%rdi` 与 `%rsi`，在 ARM64 中位于 `x0` 与 `x1`；返回值分别写入 `%eax` 与 `w0`（`x0` 的低 32 位）；
- x86-64：`addl (%rdi), %eax` 的源操作数是内存操作数，一条指令完成读取 `w[0]` 与相加；
- ARM64：两条 `ldr` 先将 `w[0]` 与 `x[0]` 读入 `w2` 与 `w0`，再由 `add` 相加（ARM64 汇编的目的操作数写在最前面）；
- ARM64 只有加载与存储指令（`ldr` / `str`）访问内存，算术指令只能使用寄存器，同一计算因此多用一条指令。
""")


def loop_need(p):
    p.title('循环需求：4096 维内积与控制流的挑战')
    slide(p, r"""
**回顾核心代码**：
""")
    p.code('c', """for (int i = 0; i < n; i++) {
    sum += w[i] * x[i];
}""")
    slide(p, r"""
**现实情况**：
- 第一部分实现了单次乘加。
- 某大模型向量内积的实际维度为 $n = 4096$。
- 机器指令中没有 `for` 结构，CPU 如何通过离散指令实现 4096 次循环迭代？
""")
    figure(p, "loop-unroll", 1120)
    p.notes("""
第二部分主线引入。
由单次计算推向完整循环，交代硬件面临的控制流问题。
""")


def pc_update(p):
    p.title('程序计数器：顺序执行时 PC 的更新规则')
    slide(p, r"""
**程序计数器（PC / `%rip`）**：
- 存放当前即将从内存代码段取出的指令的虚拟地址。

**顺序更新规则**：
- 不执行跳转指令时，`%rip` 自动按当前指令长度递增，**该操作由硬件自动完成**：
 $$\%rip \leftarrow \%rip + \text{Instruction\_Length}$$

**局限**：按地址递增的顺序依次取指，执行流不会回到地址更小的指令。
""")
    p.notes('程序计数器（PC / `%rip`）在顺序执行时的推移机制。')
    figure(p, "pc-increment", 1120)


def jump_encoding(p):
    p.title('跳转指令：直接跳转、间接跳转与 PC 相对编码')
    slide(p, r"""
**改变执行方向**：通过改写 `%rip` 寄存器的数值实现程序流跳转。

**无条件跳转的两类形式**：
1. **直接跳转（Direct Jump）**：
   - 语法：`jmp Label`；
   - 目标地址在编译汇编时已确定，作为指令的一部分直接编码；
2. **间接跳转（Indirect Jump）**：
   - 语法：`jmp *%rax`（从寄存器读取目标地址）或 `jmp *(%rax)`（从内存读取目标地址）；
   - 作用：为 C 语言函数指针、多态虚函数分发、switch 跳转表及 `ret` 返回指令提供硬件基础。
""")
    p.notes('无条件跳转指令的硬件机制、直接与间接跳转分类，以及 PC 相对编码的原理。')


def jump_encoding_2(p):
    p.title('跳转指令：直接跳转、间接跳转与 PC 相对编码')
    slide(p, r"""
**跳转目标的 PC 相对编码（PC-Relative Encoding）**：
- 机器指令中存储的是目标地址相对于下一条指令地址的偏移：
 $$\text{Offset} = \text{Target\_Address} - \text{Next\_RIP}$$
- **优势**：指令长度短（常用 1 字节或 4 字节偏移），且代码在内存中整体搬移重定位后依然正确执行。
""")
    figure(p, "pc-relative", 1120)


def rflags(p):
    p.title('状态标志：算术运算的副作用与 RFLAGS 条件码')
    slide(p, r"""
**条件状态标志位（RFLAGS 寄存器）**：
- CPU 维护单比特标志位，记录最近一次算术或逻辑运算的特征：
  - **ZF（Zero Flag）**：最近一次运算结果为 0 则置 1；
  - **SF（Sign Flag）**：最近一次运算结果为负数（最高符号位为 1）则置 1；
  - **CF（Carry Flag）**：最高有效位发生无符号溢出（产生进位或借位）则置 1；
  - **OF（Overflow Flag）**：补码有符号整数发生正负号反转溢出则置 1。
""")
    p.notes('运算副作用产生的条件状态位（ZF, SF, CF, OF）及各指令对其影响规范。')


def rflags_2(p):
    p.title('状态标志：算术运算的副作用与 RFLAGS 条件码')
    slide(p, r"""
**各类指令对标志位的影响规范**：
- 算术与逻辑指令（`add`, `sub`, `imul`, `and`, `or`, `xor`）会更新标志位；
- `lea` 与 `mov` 纯地址与数据传送，**不改变任何标志位**；
- `inc` / `dec` 指令更新 ZF、SF、OF，但**保留 CF 不变**；
- 逻辑指令（`and/or/xor/test`）执行后将 CF 与 OF 清零。

**CF 与 OF 的区别，以 8 位二进制加法为例**：
- `0xFF + 0x01 = 0x00`：无符号 $255+1=256$ 溢出（CF=1）；有符号 $-1+1=0$ 无溢出（OF=0）；
- `0x7F + 0x01 = 0x80`：无符号 $127+1=128$ 无溢出（CF=0）；有符号 $127+1=-128$ 符号反转溢出（OF=1）。
""")


def cmp_test(p):
    p.title('状态比较：cmp 与 test 指令的状态设置机制')
    slide(p, r"""
**比较指令：`cmp`**：
- 语法：`cmpl S2, S1`
- **语义**：计算 $S1 - S2$ 的差值，仅按差值结果更新 ZF, SF, CF, OF 标志位，**丢弃减法结果，两个操作数寄存器保持原值**。

**检测指令：`test`**：
- 语法：`testl S2, S1`
- **语义**：计算 $S1 \ \& \ S2$ 的按位与，仅按与运算结果设置 ZF 与 SF（并将 CF 和 OF 清零），**丢弃运算结果**。
""")
    figure(p, "cmp-test", 1120)
    p.notes('`cmp` 与 `test` 指令的执行语义，以及 dot.c 编译输出入口处的卫语句。')


def cmp_test_2(p):
    p.title('状态比较：cmp 与 test 指令的状态设置机制')
    slide(p, r"""
**实例：函数入口的维度检查**（`gcc -O2` 编译 `dot.c`，`%edx` 保存维度 $n$）：
""")
    p.code('assembly', """dot_product:
	movq	%rsi, %r8
	testl	%edx, %edx
	jle	.L4""")
    slide(p, r"""
跳转目标 `.L4`（函数的另一个出口）：
""")
    p.code('assembly', """.L4:
	xorl	%esi, %esi
	movl	%esi, %eax
	ret""")
    slide(p, r"""
- **机制分析**：`testl %edx, %edx` 将维度 $n$ 与自身做按位与。若 $n = 0$，ZF 置位；若 $n < 0$，SF 置位。配合 `jle`，只需一条指令即可检测并拦截 $n \le 0$ 的非正维度边界，直接跳到退出点 `.L4`，以返回值 0 返回。
""")


def cond_jump(p):
    p.title('条件分支：完整的条件跳转指令体系')
    slide(p, r"""
**完整的条件跳转指令分类**：
1. **相等与零值检测**：`je` / `jz` (ZF=1), `jne` / `jnz` (ZF=0)；
2. **有符号数值比较分支**：
   - `jl`：小于，条件为 $\text{SF} \ne \text{OF}$；
   - `jle`：小于等于，条件为 $(\text{SF} \ne \text{OF}) \lor (\text{ZF}=1)$；
   - `jg`：大于，条件为 $(\text{SF} = \text{OF}) \land (\text{ZF}=0)$；
   - `jge`：大于等于，条件为 $\text{SF} = \text{OF}$；
3. **无符号数值比较分支**：
   - `jb`：Below，无符号小于，条件为 $\text{CF}=1$；
   - `jbe`：Below or Equal，无符号小于等于，条件为 $\text{CF}=1 \lor \text{ZF}=1$；
   - `ja`：Above，无符号大于，条件为 $\text{CF}=0 \land \text{ZF}=0$；
   - `jae`：Above or Equal，无符号大于等于，条件为 $\text{CF}=0$。
""")
    p.notes('x86-64 完整的条件跳转指令分类、条件码判定组合，以及有符号与无符号跳转的本质差异。')


def loop_forms_for(p):
    p.title('循环翻译：从 jump-to-middle 到 guarded-do 结构')
    slide(p, r"""
**for 循环改写为 while 循环**：
""")
    p.code('c', """/* for loop in dot.c */            /* equivalent while loop */
int sum = 0;                       int sum = 0;
for (int i = 0; i < n; i++) {      int i = 0;
    sum += w[i] * x[i];            while (i < n) {
}                                      sum += w[i] * x[i];
                                       i++;
                                   }""")
    slide(p, r"""
**改写规则**：`for (init; test; update) body` 等价于 `init; while (test) { body; update; }`（循环体不含 `continue` 时成立）：
- `init`（`int i = 0`）在进入循环前执行一次；
- `test`（`i < n`）在每轮循环体之前判断，第一轮之前也判断；
- `update`（`i++`）在每轮循环体之后执行。

机器级翻译因此只需处理 do-while 与 while 两种循环；dot.c 中的 for 循环按 while 循环翻译。
""")
    p.notes('C 语言的 for 循环可改写为等价的 while 循环，机器级翻译只需处理 do-while 与 while 两种循环。')


def loop_forms(p):
    p.title('循环翻译：从 jump-to-middle 到 guarded-do 结构')
    slide(p, r"""
**C 循环与三种机器级结构**：do-while 循环在底部判断条件，循环体至少执行一次；while 循环在第一轮之前也判断条件，循环体可能一次也不执行。
1. **`do-while` 结构（翻译 do-while 循环）**：
   - 先执行循环体，底部比较并条件跳转，满足条件时跳回循环体开头。
2. **`jump-to-middle` 结构（翻译普通 while 循环，`-Og` 采用）**：
   - 入口用无条件跳转 `jmp` 跳到底部的条件判断，满足条件时跳回循环体开头；
   - 第一轮之前即判断条件，$n \le 0$ 时循环体一次也不执行。
3. **`guarded-do` 结构（翻译普通 while 循环，`-O2` 采用）**：
   - 入口先判断一次条件，作为卫语句（Guard），初始条件不成立时直接跳出；
   - 卫语句通过后，循环按 do-while 结构执行，在底部判断条件；
   - 与 jump-to-middle 相比，每次调用少执行一条入口 `jmp`。
""")
    p.notes('编译器翻译循环的三种结构：do-while 结构翻译 do-while 循环，jump-to-middle（`-Og`）与 guarded-do（`-O2`）翻译普通 while 循环。')


def loop_forms_fig(p):
    p.title('循环翻译：从 jump-to-middle 到 guarded-do 结构')
    figure(p, "loop-forms", 1120)


def cmov(p):
    p.title('无分支选择：setX 指令与 cmov 条件传送')
    slide(p, r"""
**条件设置指令：`setX`**：
- 根据标志位状态，将单个字节目标设为 0 或 1（如 `setl %al` 用于布尔求值 `a < b`）。

**条件传送指令：`cmovX`（Conditional Move）**：
- 语法：`cmovl %edx, %eax`（若小于则将 `%edx` 拷贝到 `%eax`，否则 `%eax` 保持原值）；
- 两个候选值先分别放入寄存器，`cmovX` 依据标志位选出其中一个；
- 选择过程不需要条件跳转，指令序列中少了分支跳转指令与对应的标号。

**示例**：`int max(int a, int b) { int v = a; if (a < b) v = b; return v; }`
- `gcc -Og` 用条件跳转 `jl` 实现选择；
- `gcc -O2` 改用 `cmovge`，少了一条分支跳转指令。
""")
    p.notes('条件设置指令 `setX` 与条件传送指令 `cmov`：同一个选择用 `cmov` 实现时，少了分支跳转指令。')


def cmov_fig(p):
    p.title('无分支选择：setX 指令与 cmov 条件传送')
    slide(p, r"""
`%edi` 保存参数 `a`，`%esi` 保存参数 `b`，`%eax` 保存返回值（对应规则将在后续介绍）。
""")
    figure(p, "cmov-mux", 1120)


def type_neutral(p):
    p.title('类型中立：机器级指令不携带类型的特征')
    slide(p, r"""
**ALU 硬件的类型中立性**：
- CPU 内部只有一套纯粹的二进制全加器与乘法器；
- 无论高级语言中声明的是 `int`（有符号补码）还是 `unsigned int`（无符号二进制编码），底层的二进制位运算规则相同；
- 编译器生成的指令都是 `addl` 与 `subl`，指令本身不区分有符号或无符号。

**类型的延后体现**：
- 变量究竟是有符号还是无符号，硬件并不在运算这一刻决定；
- 算术运算在生成结果的同时，同步设置无符号进位标志（CF）与有符号溢出标志（OF）；
- **类型的真正差异体现在后续选用哪一条条件跳转指令**（例如根据有符号标志跳转 `jl` 还是根据无符号标志跳转 `jb`）。
""")
    p.notes('汇编语言与硬件在算术层面的无类型特征，类型差异向后续分支跳转的延后。')


def type_neutral_fig(p):
    p.title('类型中立：机器级指令不携带类型的特征')
    figure(p, "adder-flags", 1120)


def procedure_need(p):
    p.title('过程抽象：函数调用的控制转移需求')
    slide(p, r"""
**主线函数的模块化封装**：
""")
    p.code('c', """int dot_product(const int *w, const int *x, int n);""")
    slide(p, r"""
**过程调用的硬件动作需求**：
1. **控制转移**：执行流跳转到 `dot_product` 的第一条指令；
2. **数据传递**：调用者把参数（`w`, `x`, `n`）交给被调用者；
3. **状态隔离**：函数执行期间不能破坏调用者的关键局部数据；
4. **局部状态**：被调用者执行期间需要存放自己的局部变量，调用返回后这些空间随之释放；
5. **结果返回与恢复**：被调用者将结果交给调用者，执行流返回到调用点之后继续执行。
""")
    p.notes("""
第三部分主线引入。
过程抽象在工程中的必然性，引出调用与返回的硬件机制。
""")


def local_state(p):
    p.title('局部状态：局部变量与栈帧')
    slide(p, r"""
**`main.c` 中 `main` 的局部变量**：
""")
    p.code('c', """int dot_product(const int *w, const int *x, int n);
int main(void) {
    int w[4] = {1, 2, 3, 4};
    int x[4] = {5, 6, 7, 8};
    return dot_product(w, x, 4);
}""")
    slide(p, r"""
**局部变量的存储需求**：
- 数组元素按地址访问（`w[i]` 即 `*(w + i)`），寄存器没有地址，数组只能放在内存中；
- 局部变量只在一次调用期间存在：调用开始时分配，调用返回时释放；
- 同一函数的多次调用可以同时处于活动状态（例如递归），每次调用需要各自的一份局部变量，因此局部变量不能分配在固定地址。
""")
    p.notes("""
从局部变量的存放位置引入运行时栈与栈帧。
全局变量分配在数据段的固定地址，整个程序运行期间只有一份；局部变量每次调用各有一份。
""")


def local_state_2(p):
    p.title('局部状态：局部变量与栈帧')
    slide(p, r"""
**用栈管理局部状态**：
- 调用按后进先出（LIFO）的顺序返回：最后开始的调用最先返回；
- 局部状态随调用开始而分配、随调用返回而释放，分配与释放的顺序同样是后进先出，因此用栈管理；
- 每次调用在栈上占用一段连续区域，存放本次调用的局部状态，这段区域称为该调用的**栈帧（Stack Frame）**；
- 调用开始时在栈顶分配栈帧，调用返回时释放栈帧；栈顶的栈帧属于正在执行的调用。
""")
    figure(p, "frame-lifo", 1120)


def runtime_stack(p):
    p.title('运行时栈：进程内存结构与向低地址生长')
    p.side_image("assets/address-space.svg", width="30%", alt="contain")
    slide(p, r"""
**进程典型虚拟内存布局（从低地址到高地址）**：
1. **代码段（.text）**：存放机器指令，只读且可执行；
2. **数据段（.data / .bss）**：存放已初始化/未初始化全局变量；
3. **堆（Heap）**：由 `malloc` / `new` 动态分配，向高地址生长；
4. **运行时栈（Stack）**：存放各次调用的栈帧，**向低地址生长**。
""")
    p.notes('进程虚拟内存全景、运行时栈（Stack）的生长方向与栈顶指针 %rsp。')


def runtime_stack_2(p):
    p.title('运行时栈：进程内存结构与向低地址生长')
    p.side_image("assets/address-space.svg", width="30%", alt="contain")
    slide(p, r"""
**栈顶指针 `%rsp`**：
- `%rsp` 保存栈顶地址，即栈中已使用部分的最低地址；
- 分配栈帧时减小 `%rsp`，释放栈帧时增大 `%rsp`。

**为什么栈要向低地址生长**：
- 历史设计：堆和栈分别置于虚拟地址空间的两端，彼此相对生长，最大化利用有限的地址空间而不必预先划分死边界。
""")


def stack_frame(p):
    p.title('栈帧实例：main 的局部数组')
    p.side_image("assets/main-stack.svg", width="42%", alt="contain")
    slide(p, r"""
**`main.c` 中 `main` 的汇编清单**：
""")
    p.code('assembly', """main:
    subq  $40, %rsp        # allocate the frame
    movl  $1, 16(%rsp)     # w[0] = 1;
    movl  $2, 20(%rsp)     # w[1] = 2;
    movl  $3, 24(%rsp)     # w[2] = 3;
    movl  $4, 28(%rsp)     # w[3] = 4;
    movl  $5, (%rsp)       # x[0] = 5;
    movl  $6, 4(%rsp)      # x[1] = 6;
    movl  $7, 8(%rsp)      # x[2] = 7;
    movl  $8, 12(%rsp)     # x[3] = 8;
    movq  %rsp, %rsi       # these four lines call
    leaq  16(%rsp), %rdi   # dot_product(w, x, 4)
    movl  $4, %edx
    call  dot_product@PLT
    addq  $40, %rsp        # free the frame
    ret""")
    p.notes("""
清单由 cd examples; gcc -Og -fcf-protection=none -fno-stack-protector -S main.c -o - | sed -f asm.sed 得到，注释为讲解所加。
-fno-stack-protector 关闭栈保护，栈帧中只剩局部数组；默认编译时的栈保护在本部分最后介绍。
""")


def stack_frame_2(p):
    p.title('栈帧实例：main 的局部数组')
    slide(p, r"""
- 清单由 `gcc -Og -fcf-protection=none -fno-stack-protector -S main.c` 生成，注释中的 C 代码与各条指令等价；`-fno-stack-protector` 关闭栈保护，栈保护与调用 `dot_product` 的 4 条指令将在后续介绍。

**栈帧的分配、使用与释放**：
1. **分配**：`subq $40, %rsp` 把 `%rsp` 减 40，从 `%rsp` 开始的 40 字节成为 `main` 的栈帧；
2. **使用**：局部变量按相对 `%rsp` 的偏移访问，`x[i]` 位于偏移 $4i$，`w[i]` 位于偏移 $16 + 4i$。偏移在编译时确定，每次调用的 `%rsp` 不同，同一段指令因此访问本次调用的局部变量；
3. **释放**：返回前 `addq $40, %rsp` 把 `%rsp` 加回 40，栈帧随之释放，其中的数据不需要清除。
""")
    p.aside("栈帧中偏移 32 ~ 39 的 8 字节未被使用，原因将在后续介绍。")


def return_address(p):
    p.title('函数调用：jmp 指令无法支持动态返回')
    slide(p, r"""
**为什么不能只用 `jmp`**：
- `jmp` 的目标地址在指令中是固定的。
- 函数会被程序中不同位置的调用者调用（例如调用点 A `0x4010`、调用点 B `0x4080`）。
- 函数执行完毕时，硬件无法仅凭固定操作数的 `jmp` 确定应跳转的目标返回地址。

**硬件机制**：
- 调用发生时必须动态保存调用点下一条指令的地址（返回地址 Return Address）。
""")
    p.notes('过程调用的硬件困境与简单 `jmp` 的局限。')
    figure(p, "return-address", 1120)


def call_ret(p):
    p.title('复合指令：x86-64 的 call 与 ret 指令')
    p.side_image("assets/call-ret.svg", width="32%", alt="contain")
    slide(p, r"""
**x86-64 的专用指令设计**：
- 为过程调用提供了专用指令。

**`call Label` 的硬件微操作**：
1. 把 `call` 指令的下一条指令的地址（返回地址）压栈；
2. 跳转到 `Label`。

**`ret` 的硬件微操作**：
1. 从栈顶弹出地址写入 `%rip`；
2. 从调用点的下一条指令继续执行。
""")
    p.notes('x86-64 为过程调用提供的专用复合指令。')


def push_pop(p):
    p.title('指令分解：push 与 pop 的等价微操作')
    slide(p, r"""
**`pushq Src` 的等价指令分解**：
1. 步进指针：`subq $8, %rsp`（栈顶指针向低地址延伸 8 个字节，开辟槽位）；
2. 数据存入：`movq Src, (%rsp)`（将 64 位源操作数写入新栈顶）。

**`popq Dest` 的等价指令分解**：
1. 数据读出：`movq (%rsp), Dest`（从当前栈顶读取 64 位数据到目标寄存器）；
2. 回退指针：`addq $8, %rsp`（栈指针自增 8 字节，收缩释放栈顶槽位）。
""")
    p.notes('栈操作基础指令 `push` 与 `pop` 向底层指针算术与内存访存的等价指令拆解。')


def push_pop_fig(p):
    p.title('指令分解：push 与 pop 的等价微操作')
    figure(p, "push-pop", 1120)


def sysv_abi(p):
    p.title('调用规约：System V AMD64 ABI 约定')
    slide(p, r"""
**应用二进制接口（ABI: Application Binary Interface）**：
- 规定不同编译器生成的二进制模块之间如何协同工作的全套底层协议。

**整型与指针参数传递（前 6 个按序分配寄存器）**：
- 顺序固定为：`%rdi, %rsi, %rdx, %rcx, %r8, %r9`。

**返回值传递规范**：
- 存放在 `%rax` 体系中，按宽度取 `%al` (char), `%ax` (short), `%eax` (int), `%rax` (long / 指针)。

**超过 6 个参数的栈传递规范**：
- 若参数多于 6 个，参数 7 及以后由调用者在发起 `call` 之前按从右向左顺序压入栈中；
- 被调用函数入口处，参数 7 位于栈顶偏上偏移 `8(%rsp)`，参数 8 位于 `16(%rsp)`（`0(%rsp)` 存放返回地址）。
""")
    p.notes('System V AMD64 ABI 接口契约中关于参数、返回值及栈传递的规定。')


def sysv_abi_fig(p):
    p.title('调用规约：System V AMD64 ABI 约定')
    figure(p, "abi-slots", 1120)


def win_abi(p):
    p.title('跨平台 ABI：Linux 与 Windows x64 调用约定对比')
    slide(p, r"""
**跨平台 ABI 的三处核心差异**：
1. **整型参数寄存器集合不同**：
   - Linux (System V)：前 6 个使用 `%rdi, %rsi, %rdx, %rcx, %r8, %r9`；
   - Windows (MS x64)：前 4 个使用 `%rcx, %rdx, %r8, %r9`。
2. **影子空间（Shadow Store / Home Space）**：
   - Windows 规范要求：调用者在发起 `call` 前，**必须在栈顶预留 32 字节（4 个 8 字节槽）的空闲空间**，供被调用者在必要时将前 4 个寄存器参数溢写存入栈中。
3. **寄存器保护职责差异**：
   - 在 Linux 下，`%rsi` 与 `%rdi` 属于调用者保存（Caller-saved）；
   - 在 Windows 下，`%rsi` 与 `%rdi` 属于**被调用者保存（Callee-saved）**。
""")
    p.notes('Linux (System V) 与 Windows (Microsoft x64) 在寄存器分配、影子空间与保护责任上的核心差异。')


def win_abi_2(p):
    p.title('跨平台 ABI：Linux 与 Windows x64 调用约定对比')
    slide(p, r"""
**实验意义**：
- 学生在原生 Windows（MinGW）下反汇编 `dot_product` 时，参数 `w, x, n` 分别在 `%rcx, %rdx, %r8d` 中，此现象源于调用约定的跨平台分歧。
""")
    figure(p, "abi-frames", 1120)


def dot_params(p):
    p.title('参数映射：dot_product 的形参和返回值绑定')
    slide(p, r"""
**函数原型**：
""")
    p.code('c', """int dot_product(const int *w, const int *x, int n);""")
    slide(p, r"""
**ABI 架构寄存器分配表**：
- 参数 1 `w`（指针，64 位）：进入 `%rdi`
- 参数 2 `x`（指针，64 位）：进入 `%rsi`
- 参数 3 `n`（整数，32 位）：进入 `%edx`
- 返回值 `sum`（整数，32 位）：放入 `%eax`

**硬件一致性**：
- 任何符合 System V ABI 规范的编译器调用该函数时，必须在 `call` 之前将参数就绪于这三个寄存器中。
""")
    p.notes('主线函数 `dot_product` 在 ABI 契约下的架构寄存器分配。')


def dot_params_fig(p):
    p.title('参数映射：dot_product 的形参和返回值绑定')
    figure(p, "param-binding", 1120)


def saved_regs(p):
    p.title('寄存器保护：调用者保存与被调用者保存')
    slide(p, r"""
**核心矛盾**：通用寄存器仅有 16 个，若所有函数都自由使用，调用者的数据会被被调用者改写。

**职责划分契约**：
1. **调用者保存（Caller-saved / 临时寄存器）**：
   - 包括：`%rax`, `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8` ~ `%r11`
   - 规则：被调用者可以随意修改。如果调用者在调用后还需要其原有数值，必须在 `call` 之前**自己负责存入栈中**。
2. **被调用者保存（Callee-saved / 保持寄存器）**：
   - 包括：`%rbx`, `%rbp`, `%r12` ~ `%r15`, `%rsp`
   - 规则：被调用者必须保证函数返回时这些寄存器的值原封不动。若被调用者需要使用，**必须在函数入口 `pushq` 保存，退出前 `popq` 恢复**。
""")
    p.notes('调用链条中的寄存器覆盖矛盾与两种保存机制的职责划分。')


def saved_regs_fig(p):
    p.title('寄存器保护：调用者保存与被调用者保存')
    figure(p, "saved-regs", 1120)


def stack_align(p):
    p.title('栈对齐：ABI 的 16 字节对齐要求')
    slide(p, r"""
**ABI 16 字节栈对齐规范**：
- **规范要求**：在调用者发起 `call` 指令前，栈指针 `%rsp` 的数值必须是 16 的倍数；
- **原因分析**：ABI 规定对齐是为了被调用函数能够安全使用要求 16 字节对齐的 SSE 向量指令（例如 `movaps`）；若地址未对齐，执行这些指令会触发通用保护异常（#GP）；
- **为什么减 40**：`main` 的两个数组共 32 字节。函数进入时 `call` 已将 8 字节返回地址压栈（此时 `%rsp` 除以 16 余 8），`main` 开辟 40 字节后，总偏移量为 40 + 8 = 48，是 16 的倍数，恰好保证下一次 `call` 时满足对齐要求；多出的 8 字节即栈帧中未使用的偏移 32 ~ 39。
""")
    p.notes('ABI 16 字节栈对齐的规定与原因，以及它如何决定 main 栈帧的大小。')


def stack_align_fig(p):
    p.title('栈对齐：ABI 的 16 字节对齐要求')
    figure(p, "main-align", 1120)


def red_zone(p):
    p.title('红区机制：System V ABI 的 128 字节栈顶安全区')
    slide(p, r"""
**叶子函数（Leaf Function）**：
- 不调用任何其他子函数的末端函数。

**System V ABI 红区（Red Zone）机制**：
- 规定栈指针 `%rsp` 下方的 **128 字节空间**（`[-128, -1](%rsp)`）留给当前叶子函数自由使用；
- 操作系统内核的中断与异步信号处理程序不会覆写这 128 字节；
- 当叶子函数需要使用少量局部变量时，可直接通过相对于 `%rsp` 的负偏移寻址这 128 字节，不必生成 `subq` 调整栈指针，退出前也不必生成 `addq` 回收栈。

**红区使用示例**：
""")
    p.code('c', """int leaf_example(int a, int b) {
    volatile int x = a, y = b; // volatile 局部变量分配在栈内存中
    return x + y;
}""")
    p.notes('System V ABI 红区（Red Zone）规范及其在叶子函数中的栈操作消除。')


def red_zone_2(p):
    p.title('红区机制：System V ABI 的 128 字节栈顶安全区')
    slide(p, r"""
- 编译器实测汇编（`-Og` 与 `-O2`）：
""")
    p.demo('编译 leaf.c',
           """cd examples
gcc -Og -fcf-protection=none -S leaf.c -o - | sed -f asm.sed""",
           output="""leaf_example:
	movl	%edi, -8(%rsp)
	movl	%esi, -4(%rsp)
	movl	-8(%rsp), %eax
	movl	-4(%rsp), %edx
	addl	%edx, %eax
	ret""",
           files=['examples/leaf.c', 'examples/asm.sed'])
    slide(p, r"""
- 汇编指令直接利用 `-8(%rsp)` 与 `-4(%rsp)` 存放数据，无需调整 `%rsp` 指针，消除了栈帧分配与回收开销。
""")
    p.notes("""
examples/asm.sed 删去 gcc -S 输出中的汇编伪指令（.file、.cfi_* 等）与 .LFB/.LFE 标号，只留下指令与跳转标号，与页面上的清单一致。
把 -Og 换成 -O2，清单相同。
""")


def red_zone_fig(p):
    p.title('红区机制：System V ABI 的 128 字节栈顶安全区')
    figure(p, "red-zone", 1120)


def dot_function(p):
    p.title('汇编函数：标准的 dot_product 汇编清单')
    slide(p, r"""
**主线算子的极简栈特征**：
- `dot_product` 是不调用其他函数的叶子函数，且所需变量全部保存在寄存器中；
- 既不需要在栈上分配局部变量，也不需要保存被调用者保存寄存器，因此**不建立栈帧，也没有栈操作开销**。

**代码全景解读**：
- 入口直接使用 `%rdi`, `%rsi`, `%edx`（参数由调用方放好）；
- 退出前通过 `movl %r9d, %eax` 将结果放入规定的返回值寄存器，执行 `ret` 返回。
""")
    p.notes("""
第三部分结果，展示完整的汇编函数。
`gcc -Og -fcf-protection=none -S dot.c` 的完整函数清单解读。
""")


def dot_function_2(p):
    p.title('汇编函数：标准的 dot_product 汇编清单')
    slide(p, r"""
**函数入口与初始化（4 行）**：
""")
    p.code('assembly', """dot_product:
	movl	$0, %eax
	movl	$0, %r9d
	jmp	.L2""")
    slide(p, r"""
**函数出口与返回（2 行）**：
""")
    p.code('assembly', """	movl	%r9d, %eax
	ret""")


def dot_function_fig(p):
    p.title('汇编函数：标准的 dot_product 汇编清单')
    figure(p, "dot-function", 1120)


def callee_example(p):
    p.title('被调用者保存：dot_bias 中的 %rbx')
    slide(p, r"""
**被调用者保存寄存器（Callee-saved）真实例子：`dot_bias`**：
- 代码：
""")
    p.code('c', """int dot_bias(const int *w, const int *x, int n, int b) {
    return dot_product(w, x, n) + b;
}""")


def callee_example_2(p):
    p.title('被调用者保存：dot_bias 中的 %rbx')
    slide(p, r"""
- `gcc -Og -fcf-protection=none -S` 汇编（6 行）：
""")
    p.demo('编译 dot_bias.c',
           """cd examples
gcc -Og -fcf-protection=none -S dot_bias.c -o - | sed -f asm.sed""",
           output="""dot_bias:
	pushq	%rbx
	movl	%ecx, %ebx
	call	dot_product@PLT
	addl	%ebx, %eax
	popq	%rbx
	ret""",
           files=['examples/dot_bias.c', 'examples/asm.sed'])
    slide(p, r"""
- **机制分析**：参数 `b` 位于 `%ecx`（Caller-saved 寄存器），调用 `dot_product` 后可能被改写。编译器因此把 `b` 移到 Callee-saved 寄存器 `%rbx`；`dot_bias` 自己必须在入口 `pushq %rbx` 保存，在出口 `popq %rbx` 恢复。
""")
    p.notes('examples/asm.sed 删去 gcc -S 输出中的汇编伪指令（.file、.cfi_* 等）与 .LFB/.LFE 标号，只留下指令与跳转标号，与页面上的清单一致。')


def recursion(p):
    p.title('递归调用：多层栈帧的硬件推演与回溯')
    slide(p, r"""
**主线递归形式：`dot_product_rec`**：
""")
    p.code('c', """int dot_product_rec(const int *w, const int *x, int n) {
    if (n <= 0) return 0;
    return dot_product_rec(w, x, n - 1) + w[n - 1] * x[n - 1];
}""")
    slide(p, r"""
**递归执行的栈帧状态推演**：
- 每次执行自身调用时，`call` 压入当前层的返回地址；
- 编译器通过 `pushq` 连续保存 `%r15, %r14, %rbx`，是为了在递归调用返回后，继续使用它们所保存的参数 `w, x, n` 计算 `w[n-1] * x[n-1]`；递归的累加中间结果则由 `%eax` 返回；
- 栈空间随着递归深度 $n$ 线性向下延伸，构建出链式连续的多个栈帧。
""")
    p.notes('以递归向量内积为例，展示多层嵌套调用时栈帧的线性堆叠与逐层返回。')


def recursion_3(p):
    p.title('递归调用：多层栈帧的硬件推演与回溯')
    slide(p, r"""
- 以 `dot_product_rec(w, x, 4)` 为例：$n > 0$ 的每一层在栈上占用 32 字节，即 `call` 压入的 8 字节返回地址与 `pushq` 保存的 3 个寄存器；$n = 0$ 的一层直接返回，不保存寄存器；
- 返回时逐层 `popq` 恢复寄存器，`ret` 回到上一层，部分和经 `%eax` 带回。
""")
    figure(p, "rec-stack", 1120)


def canary_plt(p):
    p.title('栈保护机制：金丝雀值与过程链接表 PLT')
    slide(p, r"""
**栈保护机制（Stack Canary）**：
- **安全威胁**：局部数组若发生越界写入，会覆盖栈顶保存的返回地址，使攻击者劫持控制流。
- **金丝雀防护机理（不加 `-fno-stack-protector` 时，`main` 的栈帧为 56 字节）**：
  1. 写入金丝雀值：`movq %fs:40, %rax` 从操作系统线程局部存储（TLS）读取随机生成的金丝雀值，通过 `movq %rax, 40(%rsp)` 存入局部数组与返回地址（`56(%rsp)`）之间；
  2. 出口检验：返回前执行 `subq %fs:40, %rdx`；若金丝雀值被篡改，`jne` 立即成立；
  3. 检查失败：跳往 `.L4` 执行 `call __stack_chk_fail@PLT`，终止进程，防范攻击。
""")
    p.notes('现代二进制安全中的栈溢出保护金丝雀（Canary）机制，以及跨文件调用的过程链接表（PLT）。')


def canary_plt_2(p):
    p.title('栈保护机制：金丝雀值与过程链接表 PLT')
    slide(p, r"""
**过程链接表（PLT: Procedure Linkage Table）**：
- 汇编中形如 `call dot_product@PLT` 中的 `@PLT` 标记；
- 跨文件或调用动态库函数时，函数地址在编译期尚未确定；
- 代码首先跳转到 PLT 存根（Stub），在运行时由动态链接器进行地址解析与延迟绑定（细节详见链接专题）。
""")
    figure(p, "canary", 1120)


def flops_estimate(p):
    p.title('算力需求：大模型推理的乘加规模与耗时推算')
    slide(p, r"""
**大模型推理的算力挑战**：
- 以参数量为 70 亿（7B）的语言模型（如 LLaMA-7B）为例；
- 自回归生成单个 Token 时，需要对全网权重矩阵进行一次前向传播计算；
- 每个参数对应一次乘法和一次累加，**生成单个 Token 需要约 70 亿次（$7 \times 10^9$）乘加运算（MAC）**。
""")
    p.notes("""
第四部分主线引入。
大模型 Token 预测的乘加计算规模，以及用标量单核 CPU 执行时的理论耗时推算，引出向量化与并行加速的必要性。
""")


def flops_estimate_2(p):
    p.title('算力需求：大模型推理的乘加规模与耗时推算')
    slide(p, r"""
**单核标量 CPU 执行耗时推算**：
- 假设 CPU 核心主频为 $4.0\text{ GHz}$（每秒提供 $4 \times 10^9$ 个时钟周期）；
- `gcc -O2` 编译的标量循环每完成一次乘加执行 6 条机器指令（本部分后续统计）；
- 参考实际运行时的平均指令执行效率 IPC $\approx 3.82$（本部分后续通过工具实测）：
  - 所需机器指令总数：$7 \times 10^9 \times 6 = 4.2 \times 10^{10}$ 条指令；
  - 所需时钟周期总数：$4.2 \times 10^{10} / 3.82 \approx 1.1 \times 10^{10}$ 个周期；
  - 单核纯计算理论耗时：$1.1 \times 10^{10} / (4.0 \times 10^9) \approx 2.75\text{ 秒}$；
- **工程结论**：
  - 单核标量执行下，每秒仅能生成不到 0.4 个 Token；
  - 生成一段 100 个 Token 的回答需要数分钟。
""")


def flops_estimate_fig(p):
    p.title('算力需求：大模型推理的乘加规模与耗时推算')
    figure(p, "token-time", 1120)


def memory_wall(p):
    p.title('访存墙：大模型推理的内存带宽瓶颈推算')
    slide(p, r"""
**自回归推理的数据搬运量推算**：
- 7B 模型即使采用 8 位整型（int8）紧凑量化，70 亿个参数占用的内存也达到 **$7\text{ GB}$**；
- 自回归解码机制决定了：**每生成一个 Token，都必须将这 7 GB 权重从主内存（DRAM）中完整读取一遍**。
""")
    p.notes('大模型自回归解码的访存密集型特征，推导权重数据纯搬运时间下限与 Roofline 性能边界。')


def memory_wall_2(p):
    p.title('访存墙：大模型推理的内存带宽瓶颈推算')
    slide(p, r"""
**物理内存总线带宽瓶颈**：
- 主流 PC 双通道 DDR4/DDR5 内存的物理峰值带宽约为 $40 \sim 60\text{ GB/s}$（取典型均值 $50\text{ GB/s}$）；
- 纯粹把权重从 DRAM 搬运至 CPU 缓存所需的物理时间下限：
 $$T_{\text{transfer}} = \frac{7\text{ GB}}{50\text{ GB/s}} = 0.14\text{ 秒/Token}$$
- **物理带宽决定的吞吐上限**：$\frac{1}{0.14\text{ s}} \approx 7.1\text{ Tokens/s}$。

**性能模型分析（Roofline 模型）**：
- 即使 CPU 算力无限充裕（运算耗时为 0），受限于内存总线物理带宽，单并发解码速度也无法超越 7.1 Token/s；
- 现代高性能系统设计必须兼顾计算吞吐优化与内存层次缓存利用。
""")


def memory_wall_fig(p):
    p.title('访存墙：大模型推理的内存带宽瓶颈推算')
    figure(p, "roofline", 1120)


def insn_mix(p):
    p.title('指令统计：标量循环的指令构成分析')
    slide(p, r"""
**标量内积循环的指令清单**：
- `-Og` 汇编循环体：包含 `movslq`, `movl`, `imull`, `addl`, `addl`, `cmpl`, `jl` 共 **7 条指令**；
- `-O2` 优化循环体：包含 `movl`, `imull`, `addq`, `addl`, `cmpq`, `jne` 共 **6 条指令**。

**指令类型与开销解构**：
1. **有效计算指令仅占 1/3**：
   - 6 条指令中，真正执行乘加数学运算的仅有 `imull`（乘法）与 `addl`（累加）2 条；
2. **控制与访存指令占 2/3**：
   - 6 条指令中 4 条不做乘加，全部消耗在内存读取（`movl`）、指针步进（`addq $4`）与循环控制判断（`cmpq` + `jne`）上，占比达 67%；
3. **处理器执行开销**：
   - 处理器的译码单元与调度部件需要为大量辅助开销指令服务，限制了算术逻辑单元的吞吐。
""")
    p.notes('拆解标量循环的指令构成，揭示有效计算与控制开销失衡的问题。')


def insn_mix_fig(p):
    p.title('指令统计：标量循环的指令构成分析')
    figure(p, "insn-mix", 1120)


def datapath_width(p):
    p.title('数据通路：标量指令对宽通路的利用率分析')
    slide(p, r"""
**现代 CPU 物理通路的硬件现实**：
- 现代主流 CPU 在微架构层面普遍具备 256 位宽度的物理向量执行单元与内部数据总线；

**标量指令的前端开销与产出**：
- 标量 `imull` 在标量整数乘法器上执行，不占用 256 位向量单元；
- 处理器为单条标量指令付出完整的取指、译码、调度与退休开销，却仅产出 1 个 32 位运算结果；
- 相比之下，若使用一条 256 位向量指令，付出相近的前端指令管理开销，即可同时驱动向量算术单元产出 8 个运算结果；

**演进诉求**：
- 硬件需提供指令支持：执行一条机器指令，即可驱动 256 位向量算术单元同时对 8 对 32 位整数执行并行乘加。
""")
    p.notes('现代超标量 CPU 内部数据通路物理宽度与 32 位标量指令的错配。')


def datapath_width_fig(p):
    p.title('数据通路：标量指令对宽通路的利用率分析')
    figure(p, "insn-results", 1120)


def simd_history(p):
    p.title('向量演进：从 MMX、SSE 到 AVX 与 AVX-512')
    slide(p, r"""
**x86 SIMD 向量扩展指令集演进脉络**：
1. **MMX（1996 年）**：引入 64 位宽向量运算（借用 x87 浮点寄存器），支持 8/16 位整型；
2. **SSE 系列（1999–2006 年）**：引入 16 个独立的 128 位寄存器 `%xmm0` ~ `%xmm15`，全面支持单双精度浮点与整型；
3. **AVX / AVX2（2011–2013 年）**：将寄存器拓宽至 **256 位**（`%ymm0` ~ `%ymm15`），引入 AVX2 整型向量算术与 FMA 乘加融合；
4. **AVX-512（2016 年至今）**：进一步拓宽至 **512 位**（`%zmm0` ~ `%zmm31`），寄存器增至 32 个，新增 8 个专用的掩码寄存器 `%k0` ~ `%k7`。

**跨架构对比：ARM 体系的向量演进**：
- **ARM NEON**：固定 128 位宽度的 SIMD 扩展，广泛应用于移动端处理器与 Apple Silicon；
- **ARM SVE / SVE2**：可伸缩可变长向量架构（Scalable Vector Extension，向量长度由硬件实现决定，程序中不写死）。
""")
    p.notes('x86 架构 SIMD 向量扩展指令集的代际演进历程，以及与 ARM 架构向量技术的对比。')


def simd_history_fig(p):
    p.title('向量演进：从 MMX、SSE 到 AVX 与 AVX-512')
    figure(p, "simd-widths", 1120)


def ymm(p):
    p.title('向量体系：SIMD 思想与 256 位 YMM 寄存器')
    slide(p, r"""
**SIMD（Single Instruction, Multiple Data）执行模型**：
- 处理器控制单元执行一条向量指令，多个并行算术通道同时对向量中的各个独立数据分量执行相同的操作。

**x86-64 YMM 寄存器体系**：
- 硬件提供 16 个 256 位宽度的向量寄存器：`%ymm0` ~ `%ymm15`；
- **向下兼容映射**：每个 `%ymm` 寄存器的低 128 位映射为同名的 `%xmm` 寄存器；
- 单个 256 位 `%ymm` 寄存器可同时封装容纳：
  - **8 个 32 位整数（int32）** $\leftarrow$ 本节主线算子；
  - 或 8 个单精度浮点数（float32）；
  - 或 4 个 64 位整数/双精度浮点数；
  - 或 32 个 8 位整数（int8 / 字符）。
""")
    p.notes('单指令多数据流（SIMD）思想、256 位 YMM 寄存器结构、向下兼容映射与 `vzeroupper` 指令。')


def ymm_2(p):
    p.title('向量体系：SIMD 思想与 256 位 YMM 寄存器')
    slide(p, r"""
**微架构状态清理：`vzeroupper` 指令**：
- 在执行完 AVX 指令后、返回调用者前，编译器会插入 `vzeroupper`；
- **微架构机理**：将所有 YMM 寄存器的高 128 位清零，消除 AVX 状态与传统 128 位 SSE 代码混用时的状态保存与恢复开销（规避数十个时钟周期的流水线停顿）。
""")
    figure(p, "ymm-lanes", 1120)


def vex_naming(p):
    p.title('向量编码：VEX 三操作数格式与向量指令命名')
    slide(p, r"""
**VEX 编码的三操作数格式**：
- 传统标量汇编为两操作数格式（破坏性写入，如 `addl %ecx, %r9d` 会覆盖 `%r9d` 原值）；
- AVX 采用 VEX 编码前缀，支持非破坏性三操作数格式：
""")
    p.code('assembly', """vpaddd    %ymm0, %ymm1, %ymm1    # 语义：%ymm1 = %ymm1 + %ymm0""")
    slide(p, r"""
- **优势**：源操作数内容不被覆盖，编译器无需插入额外的寄存器暂存与拷贝指令。

**向量寄存器异或清零惯用法**：
""")
    p.code('assembly', """vpxor    %xmm1, %xmm1, %xmm1     # 向量累加器置零""")
    p.notes('AVX 指令集的 VEX 前缀编码规范、非破坏性三操作数格式、异或清零惯用法与向量指令助记符命名规律。')


def vex_naming_2(p):
    p.title('向量编码：VEX 三操作数格式与向量指令命名')
    slide(p, r"""
- 硬件直接在寄存器重命名阶段完成清零，不占用实际执行单元与算术流水线周期。

**向量指令助记符命名规律**：
- `v` 前缀：代表采用 VEX 编码的向量扩展指令；
- `p` 标记：代表 Packed（打包的向量整型数据）；
- 运算操作名称：如 `add`（加法）、`mul`（乘法）、`xor`（异或）；
- 元素位宽类型后缀：
  - `b`（byte，8 位整型）；`w`（word，16 位整型）；
  - `d`（doubleword，32 位整型）；`q`（quadword，64 位整型）。
- 实例：`vpmulld` = Vector Packed Multiply Low Doubleword（保留乘积低 32 位）。
""")


def vex_naming_fig(p):
    p.title('向量编码：VEX 三操作数格式与向量指令命名')
    figure(p, "mnemonic", 1120)


def vector_arith(p):
    p.title('向量算术：vmovdqu、vpmulld 与 vpaddd 指令')
    slide(p, r"""
**向量加载指令：`vmovdqu`**：
- 语法：`vmovdqu (%rdi,%rax), %ymm2`
- 含义：从内存地址连续读取 256 位（32 字节，即 8 个连续 `int32`）载入寄存器 `%ymm2`；
- **`vmovdqu` 与 `vmovdqa` 的对齐约束区别**：
  - `vmovdqa`（Aligned）：要求内存地址必须以 32 字节严格对齐；若地址未对齐，CPU 触发通用保护异常（#GP），Linux 下进程收到 `SIGSEGV` 信号；
  - `vmovdqu`（Unaligned）：允许内存地址不对齐，硬件总线自动完成跨缓存行拆分加载，通用性更强。
""")
    p.notes('向量内积循环体内部的三条核心 AVX2 机器指令微架构细节及对齐约束。')


def vector_arith_2(p):
    p.title('向量算术：vmovdqu、vpmulld 与 vpaddd 指令')
    slide(p, r"""
**向量并行乘法：`vpmulld`**：
- 语法：`vpmulld (%rsi,%rax), %ymm2, %ymm0`
- 含义：同时完成 8 对 32 位整数的乘法运算，将 8 个低 32 位乘积写入 `%ymm0`。

**向量并行加法：`vpaddd`**：
- 语法：`vpaddd %ymm0, %ymm1, %ymm1`
- 含义：将 `%ymm0` 中的 8 个 32 位乘积分量分别累加到 `%ymm1` 的对应通道槽位中。
""")


def vector_arith_fig(p):
    p.title('向量算术：vmovdqu、vpmulld 与 vpaddd 指令')
    figure(p, "lanes-mul-add", 1120)


def vector_entry(p):
    p.title('向量入口：卫语句检查与向量步长计算')
    slide(p, r"""
**真实入口汇编清单（`dot_avx2.s` 入口）**：
""")
    p.demo('编译 AVX2 版本，查看入口',
           """cd examples
gcc -O2 -mavx2 -fcf-protection=none -S dot.c -o dot_avx2.s && sed -f asm.sed dot_avx2.s | sed -n '/^dot_product:/,/salq/p'""",
           output="""dot_product:
	movl	%edx, %r8d
	testl	%edx, %edx
	jle	.L8
	leal	-1(%rdx), %eax
	cmpl	$6, %eax
	jbe	.L9
	shrl	$3, %edx
	xorl	%eax, %eax
	vpxor	%xmm1, %xmm1, %xmm1
	salq	$5, %rdx""",
           files=['examples/dot.c', 'examples/asm.sed'])
    p.notes("""
`gcc -O2 -mavx2` 编译生成的真实函数入口代码逐行深入剖析。
examples/asm.sed 删去 gcc -S 输出中的汇编伪指令（.file、.cfi_* 等）与 .LFB/.LFE 标号，只留下指令与跳转标号，与页面上的清单一致。
dot_avx2.s 留在 examples/ 下，第 66、68 页的循环与规约都出自这一份清单。
""")


def vector_entry_2(p):
    p.title('向量入口：卫语句检查与向量步长计算')
    slide(p, r"""
**入口代码逐行执行逻辑解析**：
1. `movl %edx, %r8d`：将原始维度 $n$ 备份到 `%r8d`（后续尾部循环使用）；
2. `testl %edx, %edx` + `jle .L8`：卫语句拦截，若 $n \le 0$ 直接跳到 `.L8` 清零退出；
3. `leal -1(%rdx), %eax` + `cmpl $6, %eax` + `jbe .L9`：利用 AGU 计算 $n - 1$，若 $n - 1 \le 6$（即 $n < 8$），说明数据不足 8 个无法填满向量通道，直接跳往标量分支 `.L9`；
4. `shrl $3, %edx`：逻辑右移 3 位（等价于除以 8），计算向量循环的完整迭代轮次存入 `%edx`；
5. `xorl %eax, %eax` 与 `vpxor %xmm1, %xmm1, %xmm1`：将字节索引 `%rax` 与向量累加器 `%ymm1` 清零；
6. `salq $5, %rdx`：将迭代轮次左移 5 位（乘以 32），计算出向量循环结束的字节上限，供后续循环终止判定使用。
""")


def vector_entry_fig(p):
    p.title('向量入口：卫语句检查与向量步长计算')
    figure(p, "vector-entry", 1120)


def vector_loop(p):
    p.title('向量循环：gcc 生成的 AVX2 向量主循环')
    slide(p, r"""
**真实的 AVX2 向量循环指令（仅 6 条指令）**：
""")
    p.code('assembly', """.L4:
	vmovdqu	(%rdi,%rax), %ymm2
	vpmulld	(%rsi,%rax), %ymm2, %ymm0
	addq	$32, %rax
	vpaddd	%ymm0, %ymm1, %ymm1
	cmpq	%rdx, %rax
	jne	.L4""")
    slide(p, r"""
**单循环吞吐量变化**：
- 循环体依然由 6 条指令构成，但每轮迭代处理 **32 字节（8 个 int32 元素）**；
- 每完成 1 个内积元素的计算，平均指令消耗从标量的 6 条降为 **$6 / 8 = 0.75\text{ 条指令}$**。
""")
    p.notes("""
主线真实的 6 条向量主循环指令、自动向量化触发条件与工程阻碍。
寄存器编号与两个内存操作数的先后取决于编译器版本：本机 gcc 15.2 生成的循环是 vmovdqu (%rsi,%rax), %ymm0 与 vpmulld (%rdi,%rax), %ymm0, %ymm0，指令条数与结构与页面上相同。
""")


def vector_loop_2(p):
    p.title('向量循环：gcc 生成的 AVX2 向量主循环')
    slide(p, r"""
**编译器选项与自动向量化**：
- `-mavx2`：允许使用 AVX2 指令集扩展；
- `-march=native`：允许使用当前宿主机 CPU 支持的全部指令集；
- GCC 12 起，`-O2` 也会自动执行代价较低的向量化，`-O3` 开启更宽范围的向量化；
- 使用 `-fopt-info-vec` 参数可查看编译器向量化诊断报告。
""")
    p.demo('查看向量化报告',
           """cd examples
gcc -O2 -mavx2 -fcf-protection=none -fopt-info-vec -c dot.c""",
           output="""dot.c:3:23: optimized: loop vectorized using 32 byte vectors""",
           files=['examples/dot.c'])


def vector_loop_3(p):
    p.title('向量循环：gcc 生成的 AVX2 向量主循环')
    slide(p, r"""
**阻止自动向量化的常见工程情形**：
1. **指针别名（Pointer Aliasing）**：编译器无法排除指针重叠风险，需使用 C99 `restrict` 关键字显式声明；
2. **循环携带数据依赖（Loop-carried Dependency）**：前后迭代存在强因果依赖（如 `a[i] = a[i-1] + ...`，而本节的求和规约 `sum += ...` 能被编译器识别并自动处理）；
3. **循环体内部存在不可内联的函数调用**。
""")
    figure(p, "scalar-vs-vector", 1120)


def intrinsics(p):
    p.title('内建函数：AVX2 Intrinsics 手写向量点积')
    slide(p, r"""
**什么是 SIMD Intrinsics（`<immintrin.h>`）**：
- 编译器提供的具有 C 语言函数外观的底层内建接口，在编译时通常对应特定的 CPU 向量机器指令。

**手写 AVX2 内积核心源码**：
""")
    p.code('c', """#include <immintrin.h>
__m256i vsum = _mm256_setzero_si256();   // vpxor %xmm1, %xmm1, %xmm1
for (int i = 0; i <= n - 8; i += 8) {
    __m256i va = _mm256_loadu_si256((__m256i*)&w[i]);  // vmovdqu
    __m256i vb = _mm256_loadu_si256((__m256i*)&x[i]);  // 并入 vpmulld 的内存操作数
    __m256i vprod = _mm256_mullo_epi32(va, vb);       // vpmulld
    vsum = _mm256_add_epi32(vsum, vprod);             // vpaddd
}""")
    p.notes('C 语言 SIMD 内建函数（Intrinsics）的编程规范及其与机器级汇编的精准映射。')


def intrinsics_2(p):
    p.title('内建函数：AVX2 Intrinsics 手写向量点积')
    slide(p, r"""
**Intrinsics 与汇编指令的对应关系**：
- `__m256i` 数据类型 $\longleftrightarrow$ 硬件 256 位 YMM 向量寄存器；
- `_mm256_loadu_si256` $\longleftrightarrow$ `vmovdqu` 向量非对齐加载指令；
- `_mm256_mullo_epi32` $\longleftrightarrow$ `vpmulld` 并行低位双字乘法指令；
- `_mm256_add_epi32` $\longleftrightarrow$ `vpaddd` 并行双字累加指令。
""")
    p.demo('编译运行 Intrinsics 版本',
           """cd examples
gcc -O2 dot_intrin.c -o dot_intrin
./dot_intrin""",
           output="""avx2   3182690
check  3182690""",
           files=['examples/dot_intrin.c'])
    p.notes("""
dot_intrin.c 用 __attribute__((target("avx2"))) 只让这一个函数使用 AVX2，运行时用 __builtin_cpu_supports("avx2") 选择版本，不支持 AVX2 的机器走标量版本；check 一行是标量循环的结果。
gcc -O2 编出的循环比第 66 页多一条 vmovdqa 寄存器拷贝。
""")


def intrinsics_fig(p):
    p.title('内建函数：AVX2 Intrinsics 手写向量点积')
    figure(p, "intrinsics-map", 1120)


def reduction(p):
    p.title('水平规约：向量累加和向标量返回值的转换')
    slide(p, r"""
**真实的水平规约与折半累加（`dot_avx2.s` 本机实测清单）**：
""")
    p.code('assembly', """vextracti128	$0x1, %ymm1, %xmm0
movl	%r8d, %eax
vpaddd	%xmm1, %xmm0, %xmm0
andl	$-8, %eax
vpsrldq	$8, %xmm0, %xmm1
vpaddd	%xmm1, %xmm0, %xmm0
vpsrldq	$4, %xmm0, %xmm1
vpaddd	%xmm1, %xmm0, %xmm0
vmovd	%xmm0, %ecx""")
    slide(p, r"""
- **折半树状规约机理**：高 128 位提取与低 128 位相加（8 通道压缩为 4 通道） $\to$ 逻辑右移 8 字节相加（4 通道压缩为 2 通道） $\to$ 逻辑右移 4 字节相加（2 通道压缩为 1 通道） $\to$ `vmovd` 将最终标量累加和提取装填至 `%ecx`（中间穿插两条指令 `movl %r8d, %eax` 与 `andl $-8, %eax` 计算尾部循环起点）。
""")
    p.notes('真实汇编中水平规约指令的微操作、尾部余数循环处理及代数重排合法性剖析。')


def reduction_2(p):
    p.title('水平规约：向量累加和向标量返回值的转换')
    slide(p, r"""
**尾部余数检查与标量收尾**：
- `testb $7, %r8b` + `je .L14`：检测 $n \pmod 8$ 是否存在未填满向量的剩余元素；
- 若有余数，执行 `cltq` 扩展后进入 `.L7` 标量循环累加剩余元素；
- 函数返回前生成 `vzeroupper` 恢复微架构状态。

**代数重排合法性剖析**：
- **整数加法**严格满足结合律 $(a+b)+c = a+(b+c)$，折半树状规约与标量串行求和在数学结果上等价；
- **浮点加法不满足结合律**（受舍入误差影响），没有 `-ffast-math`（或 `-fassociative-math`）时，GCC 不会改变浮点加法的计算顺序，因此不会对浮点点积自动进行这种树状规约。这是本节以整数内积为主线的重要原因。
""")


def reduction_fig(p):
    p.title('水平规约：向量累加和向标量返回值的转换')
    figure(p, "reduction-tree", 1120)


def perf(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    slide(p, r"""
**实验设置（源自 `examples/bench.c` 实测）**：
- 数组维度 $n = 4096$，循环调用 100,000 次，累计完成约 4.1 亿次乘加（$4.1 \times 10^8$ MAC）。
""")
    p.demo('构建两个版本并用 perf 计数',
           """cd examples
make
perf stat -e instructions,cycles ./dot_scalar
perf stat -e instructions,cycles ./dot_avx2""",
           files=['examples/Makefile', 'examples/bench.c', 'examples/dot.c'])
    p.notes("""
第四部分实验实测结果展示。
在真实现代硬件上使用 `perf stat` 进行实测对比、数据分析与跨平台策略。
perf 需要内核允许普通用户读取硬件计数器：/proc/sys/kernel/perf_event_paranoid 不大于 1（Ubuntu 默认是 4，课前由授课人执行一次 sudo sysctl kernel.perf_event_paranoid=1）。
计数与耗时随 CPU 型号和频率变化，页面上的数值来自 i9-11900H。
""")


def perf_2(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    slide(p, r"""
**真实性能测量报告对比**：
- **标量版本（`-O2`）实测**：
""")
    p.code('text', """ 2,459,562,893      instructions                     #    3.82  insn per cycle
   643,892,105      cycles                           #    4.286 GHz

   0.150218491 seconds time elapsed
   0.149999000 seconds user
   0.000219000 seconds sys""")
    slide(p, r"""
- **AVX2 向量版本（`-O2 -mavx2`）实测**：
""")
    p.code('text', """   310,762,907      instructions                     #    3.02  insn per cycle
   103,024,819      cycles                           #    4.286 GHz

   0.024035128 seconds time elapsed
   0.023999000 seconds user
   0.000036000 seconds sys""")


def perf_3(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    slide(p, r"""
**实测指标归纳**：
- **指令削减**：指令数从 24.6 亿减少到 3.1 亿，指令数比值为 7.91x（标量版 24.6 亿条指令除以每次乘加 6 条，正好对应 4.1 亿次乘加）；
- **周期与耗时加速**：耗时从 0.150s 降至 0.024s，端到端加速比达到 6.25x。

**跨平台测量替代方案**：
- Windows 原生无 Linux `perf` 工具，可采用两种替代方案：
  1. 程序内部调用 `clock_gettime(CLOCK_MONOTONIC)` 或通过 `__rdtsc()` 读取时间戳计数器；
  2. 安装 WSL2，在 Ubuntu 子系统内运行原生 Linux `perf`。

**思考题**：若将数组规模 $n$ 扩大至超出 CPU L3 缓存容量（如数百兆字节），加速比将显著下降，直观印证前面讨论的“访存墙”与 Roofline 瓶颈。
""")


def perf_fig(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    figure(p, "perf-bars", 1120)


def openmp(p):
    p.title('多核并行：从单核 SIMD 到多核 OpenMP 并发')
    slide(p, r"""
**多核与向量化结合**：
- 向量化（SIMD）是单个 CPU 核心内部的数据级并行；
- 现代服务器与 PC 普遍具备多个物理 CPU 核心。

**OpenMP 多核并发实现**：
""")
    p.code('c', """#pragma omp parallel for reduction(+:sum)
for (int i = 0; i < n; i++) {
    sum += w[i] * x[i];
}""")
    slide(p, r"""
**“8 核 $\times$ 8 通道”硬件执行模型**：
- OpenMP 运行时将循环的迭代范围切分给各个线程，由操作系统调度到 8 个 CPU 核心上；
- 每个核心各自运行 256 位 AVX2 向量指令（每周期处理 8 个数据通道）；
- **硬件并发规模**：8 核心 $\times$ 8 通道/核心 = 64 通道并发处理数据。
""")
    p.notes('CPU 并发体系——单核向量 SIMD（数据级并行）与多核心 OpenMP（线程并行）的结合。')


def openmp_2(p):
    p.title('多核并行：从单核 SIMD 到多核 OpenMP 并发')
    slide(p, r"""
**编译选项说明**：
- 编译时必须显式添加 `-fopenmp`，否则 `#pragma` 将被编译器忽略；
- 必须同时启用 `-mavx2` 或 `-march=native`，否则各核心执行标量指令。线程数量可通过 `OMP_NUM_THREADS=8` 环境变量设定（对应大模型框架中的 `n_threads = 8`）。
""")
    p.demo('编译运行 OpenMP 版本',
           """cd examples
gcc -O2 -mavx2 -fopenmp dot_omp.c -o dot_omp
OMP_NUM_THREADS=8 ./dot_omp""",
           output="""threads 8
sum     41943040
check   41943040""",
           files=['examples/dot_omp.c'])
    p.notes("""
dot_omp.c 的循环与页面上的相同，n = 2^24；check 一行是单线程循环的结果。
去掉 -fopenmp 时 pragma 被忽略，程序仍然正确，只是单线程运行。
""")


def openmp_fig(p):
    p.title('多核并行：从单核 SIMD 到多核 OpenMP 并发')
    figure(p, "core-lanes", 1120)


def simt(p):
    p.title('异构并行：从 CPU 向量化到 GPU SIMT 执行模型')
    slide(p, r"""
**CPU 与 GPU 的微架构设计哲学分歧**：
- **CPU 核心**：芯片面积多分配给复杂乱序控制逻辑、分支预测器和多级缓存（SRAM），以降低单线程执行延迟；
- **GPU 核心**：芯片面积多直接分配给大量并行的算术逻辑单元（ALU），以追求大规模数据并行的高吞吐量。
""")
    p.notes('从 CPU SIMD 走向 GPU SIMT（单指令多线程）体系结构与异构计算执行模型。')


def simt_2(p):
    p.title('异构并行：从 CPU 向量化到 GPU SIMT 执行模型')
    slide(p, r"""
**GPU SIMT（Single Instruction, Multiple Threads）执行模型**：
- **Thread（线程）**：最基本执行单元；
- **Thread Block（线程块）**：共享片上共享内存的一组线程；
- **Grid（网格）**：整个 Kernel 启动时的全部线程网格集合；
- **Warp（线程束，32 个线程）**：GPU 硬件调度的最小单元，32 个线程锁步执行相同指令。

**分支分化（Branch Divergence）**：
- 若同一个 Warp 内的 32 个线程在分支结构中走向不同路径，硬件只能串行化执行所有分支路径，导致性能下降。
""")


def simt_fig(p):
    p.title('异构并行：从 CPU 向量化到 GPU SIMT 执行模型')
    figure(p, "cpu-gpu-area", 1120)


def cuda(p):
    p.title('GPU 编程：CUDA 向量内积算子与共享内存规约')
    slide(p, r"""
**CUDA 点积核函数实现**：
""")
    p.code('cuda', """__global__ void dot_kernel(const int *w, const int *x, int *block_sum, int n) {
    __shared__ int cache[256];
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    cache[tid] = (idx < n) ? w[idx] * x[idx] : 0;
    __syncthreads();
    // 块内共享内存树状折半规约（对应 CPU 向量水平规约）
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) cache[tid] += cache[tid + s];
        __syncthreads();
    }
    if (tid == 0) block_sum[blockIdx.x] = cache[0];
}""")
    p.notes('向量内积的 CUDA Kernel 实现、片上共享内存树状折半规约及 PCIe 总线瓶颈。')


def cuda_2(p):
    p.title('GPU 编程：CUDA 向量内积算子与共享内存规约')
    slide(p, r"""
**概念映射与思想统一**：
- 每个 CUDA 线程只算一次乘法，没有标量循环；
- 共享内存折半累加过程与 CPU 向量水平折半规约思想一致。

**异构系统数据传输开销**：
- GPU 算力高，但数据必须经由 PCIe 总线从主机（CPU 内存）拷贝到设备（GPU 显存）；
- 若每次内积都进行主机与设备间的数据拷贝，拷贝时间将超过计算时间。
""")
    p.demo('编译运行 CUDA 版本',
           """cd examples
nvcc -O2 -arch=native dot_cuda.cu -o dot_cuda
./dot_cuda""",
           output="""blocks  65536 x 256 threads
sum     41943040
check   41943040""",
           files=['examples/dot_cuda.cu'])
    p.notes("""
需要 NVIDIA GPU 与 CUDA 工具包。
dot_cuda.cu 的 kernel 与页面上相同（注释改为英文），主机端把 65536 个块的部分和拷回 CPU 求和；check 一行是 CPU 循环的结果。
""")


def cuda_fig(p):
    p.title('GPU 编程：CUDA 向量内积算子与共享内存规约')
    figure(p, "cuda-tree", 1120)


def cpu_dispatch(p):
    p.title('硬件探测：大模型框架的指令集探测与多库分发')
    slide(p, r"""
**硬件指令集适配问题**：
- 若编译时启用 `-mavx2` 或 `-mavx512f`，在不支持该指令集的 CPU 上运行，会直接触发**非法指令异常（SIGILL / Illegal Instruction）导致程序崩溃**。

**硬件探测机制**：
1. **汇编级探测指令：`cpuid`**：
   - CPU 提供的专用硬件信息查询指令；向 `%eax` 写入功能号，调用 `cpuid`，返回寄存器（`%ebx, %ecx, %edx`）的特定比特位即代表是否支持 AVX、AVX2、AVX512F 等特性；
2. **C/C++ 语言层内置接口**：
""")
    p.code('c', """if (__builtin_cpu_supports("avx2")) {
    dot_product_avx2(w, x, n);
} else {
    dot_product_scalar(w, x, n);
}""")
    p.notes('大模型工程框架如何在不同硬件环境间动态探测指令集特性，并实现多版本二进制安全分发。')


def cpu_dispatch_2(p):
    p.title('硬件探测：大模型框架的指令集探测与多库分发')
    slide(p, r"""
**真实大模型推理引擎（Ollama）的日志与多库分发**：
- 启动时的系统特性探测实测日志：
""")
    p.demo('Ollama 日志中的特性探测与后端加载',
           "journalctl -u ollama | grep -o 'system_info.*' | tail -1 | sed 's/ CUDA :.*//' | fold -s -w 88\n"
           "journalctl -u ollama | grep -o 'load_backend: loaded CPU.*' | tail -1",
           output="""system_info: n_threads = 8 (n_threads_batch = 8) / 16 | CPU : SSE3 = 1 | SSSE3 = 1 |
AVX = 1 | AVX2 = 1 | F16C = 1 | FMA = 1 | BMI2 = 1 | AVX512 = 1 | AVX512_VBMI = 1 |
AVX512_VNNI = 1 | LLAMAFILE = 1 | REPACK = 1 |
load_backend: loaded CPU backend from /usr/local/lib/ollama/libggml-cpu-icelake.so""")
    p.notes("""
第一条命令末尾的 sed 去掉行尾的 CUDA 字段，没有 NVIDIA GPU 的机器上这一段本来就不存在；fold -s -w 88 让这一行在「| 」处折行。
日志需要 Ollama 以 systemd 服务运行过至少一次；读 journal 需要用户在 systemd-journal 或 adm 组。
加载哪一个库取决于 CPU：i9-11900H（Tiger Lake）加载 icelake 版本，只有 AVX2 的机器加载 haswell 版本。
""")


def cpu_dispatch_3(p):
    p.title('硬件探测：大模型框架的指令集探测与多库分发')
    slide(p, r"""
- Ollama 安装目录 `/usr/local/lib/ollama/` 下针对不同微架构编译的动态链接库：
  - `libggml-cpu-x64.so`（x86-64 基线版本）；
  - `libggml-cpu-sse42.so`（SSE4.2 版本）；
  - `libggml-cpu-haswell.so`（AVX2 + FMA）；
  - `libggml-cpu-alderlake.so`（AVX2 + AVX_VNNI）；
  - `libggml-cpu-icelake.so`（AVX-512 + VBMI + VNNI）；
  - `libggml-cpu-zen4.so`（AVX-512）。
""")


def cpu_dispatch_4(p):
    p.title('硬件探测：大模型框架的指令集探测与多库分发')
    slide(p, r"""
**学生自主探测实操**：
- Linux 终端：`lscpu` 或 `cat /proc/cpuinfo | grep avx2`；
- Windows 平台：编译运行一个包含 `__builtin_cpu_supports` 的 C 语言小程序。
""")
    p.demo('本机支持的向量扩展',
           "lscpu | grep -o -w 'avx2\\|avx512f\\|avx512_vnni'",
           output="""avx2
avx512f
avx512_vnni""")
    p.notes('grep -o -w 只打印这三个标志；本机 i9-11900H 三个都支持，只有 AVX2 的机器只打印 avx2。')
    figure(p, "dispatch", 1120)


def insn_summary(p):
    p.title('指令总览：本节核心机器级指令分类速查')
    slide(p, r"""
**机器指令全景速查矩阵**：
1. **数据传送与扩展**：
   - `movb`, `movw`, `movl`, `movq`（通用数据传送，不可双内存操作数）；
   - `movzbl`, `movzwl`（零扩展传送）；
   - `movsbl`, `movswl`, `movslq`, `cltq`（符号扩展传送）；
   - `pushq`, `popq`（栈操作复合指令）；`leal`, `leaq`（加载有效地址/算术运算）。
2. **算术与逻辑运算**：
   - `addl`, `addq`, `subl`, `subq`, `imull`, `imulq`（基础整数算术）；
   - `incl`, `decl`, `negl`, `notl`（单操作数运算）；
   - `andl`, `orl`, `xorl`（位逻辑运算，`xorl` 用于高效寄存器清零）；
   - `sall`/`shll`, `sarl`, `shrl`, `salq`（算术与逻辑移位）。
""")
    p.notes('将全节出现的六类核心机器级指令汇总为结构化速查表。')


def insn_summary_2(p):
    p.title('指令总览：本节核心机器级指令分类速查')
    slide(p, r"""
3. **状态比较与分支跳转**：
   - `cmpl`, `cmpq`（减法设置标志位）；`testl`, `testq`（与逻辑设置标志位）；
   - `jmp`（无条件跳转，直接/间接）；
   - `jX`（条件跳转：有符号 `jl/jle/jg/jge`，无符号 `jb/jbe/ja/jae`，零值 `je/jne`）；
   - `setX`（条件设置字节）；`cmovX`（条件传送，消除分支）。
4. **过程调用与运行时安全**：
   - `call`, `ret`（返回地址压栈跳转与出栈恢复）；
   - `%fs:40`, `__stack_chk_fail@PLT`（金丝雀栈溢出保护）。
""")


def insn_summary_3(p):
    p.title('指令总览：本节核心机器级指令分类速查')
    slide(p, r"""
5. **向量计算（AVX2 扩展）**：
   - `vmovdqu`, `vmovdqa`（非对齐/对齐 256 位向量加载与存储）；
   - `vpmulld`, `vpaddd`（8 通道 32 位整数并行乘加）；
   - `vpxor`（向量寄存器清零）；
   - `vextracti128`, `vpsrldq`, `vmovd`（跨通道折半水平规约）；
   - `vzeroupper`（清除 YMM 高位，消除与 SSE 切换开销）。
6. **硬件特性查询**：
   - `cpuid`（CPU 硬件特性查询指令）。
""")


def insn_summary_fig(p):
    p.title('指令总览：本节核心机器级指令分类速查')
    figure(p, "insn-cards", 1120)


def summary(p):
    p.title('全节总结：计算机系统的抽象层级与指令集契约')
    slide(p, r"""
**计算机系统的抽象层级划分**：
1. **高级语言层（C/C++）**：提供算法表达的抽象与结构化思维（循环、函数、数组），屏蔽底层物理硬件细节；
2. **应用二进制接口（ABI）**：建立在 ISA 之上的软件契约规范（函数调用规约、寄存器保护职责、运行时栈对齐、数据类型大小），同一 ISA 在 Linux 与 Windows 下有不同 ABI；
3. **指令集体系结构（ISA）**：计算机软硬件之间的接口规范（指令格式与编码、架构寄存器、程序员可见状态、内存寻址模式），在代际演进中保持严格的向后兼容；
4. **微架构与硬件电路层（Microarchitecture）**：芯片内部的物理实现（乱序执行流水线、ALU/AGU 算术单元、执行端口、物理总线与晶体管）。
""")
    p.notes('统揽全局，总结从高阶代码到硅片硬件的完整链条与 ISA 接口契约规范。')


def summary_2(p):
    p.title('全节总结：计算机系统的抽象层级与指令集契约')
    slide(p, r"""
**从标量到向量与并发的系统思维**：
- 性能的提升不能单靠提升时钟频率（受功耗墙与散热限制）；
- 现代系统软件与高性能算子开发的核心，在于理解数据通路宽度、内存层次带宽以及指令吞吐并行性；
- 向量内积计算 `sum += w[i] * x[i]`，在底层通过指令编码、寄存器分配、流水线调度与向量化扩展实现高效执行。
""")
    figure(p, "system-layers", 1120)


def lab(p):
    p.title('课后实验：反汇编与性能测量指南')
    slide(p, r"""
**实验一：反汇编对照与寻址模式验证**：
- **操作命令**：
""")
    p.code('bash', """gcc -Og -fcf-protection=none -S dot.c -o dot.s
gcc -c dot.s -o dot.o
objdump -d dot.o""")
    slide(p, r"""
- **观察与记录**：在汇编清单中定位 `dot_product` 循环体，标出基址比例变址寻址 `(%rsi,%r8,4)`；换用 `gcc -O2` 编译，观察循环如何演变为 `addq $4` 步进指针模式。
""")
    p.notes('为学生发布完整的课后实践指南，明确实验文件、运行命令、观察重点、跨平台注意事项与提交要求。')


def lab_2(p):
    p.title('课后实验：反汇编与性能测量指南')
    slide(p, r"""
**实验二：AVX2 向量加速比与硬件性能事件测量**：
- **操作命令（Linux 环境，使用 `examples/`）**：
""")
    p.code('bash', """make -C examples
perf stat -e instructions,cycles ./examples/dot_scalar
perf stat -e instructions,cycles ./examples/dot_avx2""")


def lab_3(p):
    p.title('课后实验：反汇编与性能测量指南')
    slide(p, r"""
- **跨平台与环境提示**：
  - Linux 若提示权限不足，需临时执行：`sudo sysctl kernel.perf_event_paranoid=1`；
  - 原生 Windows（WinLibs 环境）无 Linux `perf`：可使用 WSL2 运行 `perf`，或在 `bench.c` 中调用 `clock_gettime(CLOCK_MONOTONIC)` 进行计时；反汇编时注意 Windows x64 的参数寄存器为 `%rcx, %rdx, %r8d`；
  - 思考与进阶：修改 `examples/bench.c` 中的数组维度 $n$，当数组规模显著超出 CPU L3 缓存时观察向量加速比的变化，分析访存带宽对性能的影响。

**实验提交要求**：
- 提交实测终端日志截图、两组实验数据记录表以及对“超出 L3 Cache 规模后向量加速比衰减原因”的简要分析。
""")


def lab_fig(p):
    p.title('课后实验：反汇编与性能测量指南')
    figure(p, "lab-cards", 1120)
