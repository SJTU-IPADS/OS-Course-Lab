"""ICS 第 3 章：程序的机器级表示与执行。

一页排不下时按语义拆成几页，拆出的页沿用同一个页面标题；
每页的配图由 diagrams/ 下的同名脚本生成，放在正文之后：
与正文放得进一页时放在同一页，否则放在同标题的下一页。
过渡页写成 lecture.bridge，见 lecture.py：只放页面标题。

排版约定：
- 粗体在文案里逐处写明，slide() 因此关闭自动加粗。
- 表格写成 p.table。
- 汇编清单取自 gcc 15.2 的真实输出（-fcf-protection=none，不含 endbr64），制表符原样保留。
  演示命令用 examples/asm.sed 去掉汇编伪指令，只留下指令与跳转标号。
- 演示命令在本节目录下执行，所以以 `cd examples` 开头。
"""


def slide(p, md, **kw):
    """页面文案。粗体在文案里逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def figure(p, name, width):
    """页面的配图，由 diagrams/ 下的同名脚本生成。"""
    return p.image(f"assets/{name}.svg", width_px=width)


def problem(p):
    p.title('问题提出')
    slide(p, r"""
- 模型推理过程中有大量计算是矩阵乘法，例如需要计算 $w$ 和 $x$ 的内积，由推理程序 `ollama` 负责组织完成这些计算。如果在 CPU 上进行推理，内积可以由一个简单的 `for` 循环累积来完成。CPU不认识for、不认识内积，是怎么完成执行的？
""")
    p.code('c', """int dot_product(const int *w, const int *x, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += w[i] * x[i]; 
    }
    return sum;
}""")
    p.notes('以 ollama 的推理过程提出全节要回答的问题：程序如何在由晶体管构成的 CPU 上执行；并给出全节围绕的核心函数 dot_product。')


def system_view_fig(p):
    p.title('CPU从内存中获取指令与数据进行计算')
    figure(p, "system-view", 1120).footnote('照片从左到右来自 Wikimedia Commons 的 Evan-Amos（CC BY-SA 3.0）、D-Kuru（CC BY-SA 4.0）、PantheraLeo1359531（CC BY 4.0）、Eric Gaba（CC BY-SA 4.0），经裁剪缩放。')
    p.notes('左下的放大框取自 ollama 可执行文件偏移 0xcb4cf0 处的 22 字节，是其中 gonum 库 float32 内积函数 f32.DotUnitary 逐个元素处理的循环：前 3 条完成 sum += w[i] * x[i]，后 3 条更新下标与剩余次数并跳回循环开头。')


def compile_mapping(p):
    p.title('编译：从 C 源码到二进制机器指令')
    p.gap(4)
    slide(p, r"""
**本节的核心函数**：
- 大模型计算的数学算子：向量内积 `dot_product`，这段代码如何才能被 CPU 执行？

**同一段代码的三种表示**：
1. **高级语言（C）**：面向程序员的控制结构与变量符号抽象。
2. **汇编指令（Assembly）**：通过 `objdump -d` 反汇编或 `gcc -S` 直接生成，与机器码对应。
3. **目标机器码（Object Code）**：经 `gcc -c` 生成的纯二进制目标文件（ELF），通过 `hexdump` / `xxd` 观察，呈现为十六进制机器字节流。
""")
    figure(p, "compile-mapping", 1120)
    p.notes('从高级 C 源码到二进制机器码与反汇编的映射概览。')


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


def isa_contract(p):
    p.title('指令集架构')
    slide(p, r"""
程序编译之后得到的是二进制机器码。但 CPU 执行需要能够认识这段二进制所代表的指令，因此二进制编码与 CPU 指令之间的约定必不可少。

**指令集架构（ISA: Instruction Set Architecture）的定义**：
- ISA 是软件与硬件电路之间达成的接口契约规范，在长期演进中保持严格的向后兼容性。
- **ISA包括如下**：
  - **程序员可见状态**：程序计数器（PC / `%rip`）、16 个通用寄存器、条件码寄存器（RFLAGS）、连续虚拟内存空间；
  - **指令编码与格式**：每条机器指令的二进制操作码、操作数编码规则与指令长度；
  - **支持的数据类型与寻址模式**。
  - ...
""")
    p.notes('指令集架构（ISA）的契约定义、程序员可见状态，以及 ISA 与微架构实现的解耦关系。')


def isa_contract_fig(p):
    p.title('指令集架构与微架构实现的解耦')
    slide(p, r"""
- **ISA 是接口（Interface）**：规定处理器能执行什么；
- **微架构是实现（Implementation）**：规定处理器在物理电路层面如何执行。
""")
    figure(p, "isa-contract", 1040)


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
第一部分的引入。
先忽略循环与函数外壳，聚焦最核心的单次乘加操作。
""")


def single_mac_fig(p):
    p.title('单次乘加：w[0] * x[0] 的执行过程')
    figure(p, "single-mac", 1120)


def visible_state(p):
    p.title('程序员可见状态：寄存器')
    slide(p, r"""
**程序员可见状态（Programmer-Visible State）**：
- 指令集架构直接暴露给软件程序、能够被指令读取和修改的全部硬件实体：
  - **程序计数器（PC / `%rip`）**：存放下一条取指执行指令的虚拟地址；
  - **通用寄存器堆（Register File）**：16 个 64 位高速存储单元；
  - **条件标志位寄存器（RFLAGS）**：存放最近一次算术逻辑运算的状态结果；
  - **虚拟内存空间**：通过地址总线访问的代码、全局变量与栈内存。
""")
    figure(p, "visible-state", 1120)
    p.notes('程序员可见状态的定义，以及 CPU 内部通用寄存器数量受到硬件严格限制的原因。')


def visible_state_2(p):
    p.title('sidebar：寄存器堆设计约束')
    slide(p, r"""
**通用寄存器数量为什么只有 16 个**：
- **指令编码字段限制**：指令中寄存器字段的位数有限，在定长字段中指定一个寄存器，16 个寄存器只需 4 位二进制（$2^4 = 16$），若扩展到上千个，指令编码将急剧膨胀；
- **硬件布线与延迟瓶颈**：寄存器数量过多会导致内部走线与多路选择器延迟增大，降低最高工作频率。
""")


def register_slices_fig(p):
    p.title('寄存器切片：16 个通用寄存器及其命名规则')
    figure(p, "register-table", 1120)


def effective_address(p):
    p.title('数组下标与基址步长的数学映射')
    slide(p, r"""
**高级语言表达**：`w[i]`

**内存连续排布模型**：
- 数组名 `w` 代表首元素起始地址（Base Address）。
- 下标 `i` 为元素逻辑偏移。
- 每个 `int` 元素占用 4 个字节（Scale Factor = 4）。

**计算公式**：
 $$\text{Address}(w[i]) = \text{Base}(w) + i \times 4$$
""")
    p.notes('高级语言中的数组下标表达式如何映射为内存虚拟地址。')
    figure(p, "array-address", 1120)


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
- 例子 `movl (%rsi), %eax`：
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


def extension_example(p):
    p.title('扩展传送实例：比较 signed char 与 unsigned char')
    p.code('c', """int equal(signed char a, unsigned char b) { return a == b; }""")
    slide(p, r"""
**第二讲的规则**：窄于 `int` 的操作数先提升为 `int`，再做比较。
""")
    slide(p, r"""
**`gcc -Og` 编译得到的汇编**：
""")
    p.code('assembly', """equal:
	movsbl	%dil, %edi      # a: signed char -> int, sign extension
	movzbl	%sil, %esi      # b: unsigned char -> int, zero extension
	cmpl	%esi, %edi
	sete	%al
	movzbl	%al, %eax
	ret""")
    slide(p, r"""
**编译器选择的指令**：两次提升各由一条扩展传送指令完成，补符号位还是补 0 由原类型决定。
- `a` 的类型是 `signed char`，编译器选用符号扩展指令 `movsbl`；
- `b` 的类型是 `unsigned char`，编译器选用零扩展指令 `movzbl`；
- 两个参数的字节都是 `0xF0` 时，`%edi` 为 $-16$，`%esi` 为 $240$，比较结果为不相等。
""")
    p.notes("""
第二讲「混合比较：宽度不同的操作数进行比较」一页给出这条规则：先把窄于 `int` 的类型提升为 `int`，补 0 还是补符号位由原类型决定。两个操作数提升后都是 `int`，比较按 `int` 进行。
清单由 cd examples; gcc -Og -fcf-protection=none -S equal.c -o - | sed -f asm.sed 得到，注释为讲解所加。
`%dil` 保存参数 `a`，`%sil` 保存参数 `b`，`%eax` 保存返回值，对应规则在第三部分详细讲。
后三条指令比较两个 32 位值并产生返回值，在第二部分详细讲。
""")


def int_arith(p):
    p.title('整数乘法指令：imull')
    slide(p, r"""
**二操作数格式 `imull S, D`**（单次乘加中的 `imull (%rdi), %eax`）：
- 语义：$D \leftarrow D \times S$；$S$ 可以是寄存器、内存操作数或立即数，$D$ 只能是寄存器；
- 乘积只保留低 32 位写入 $D$。有符号与无符号乘法的低 32 位相同，`int` 与 `unsigned` 的乘法因此都编译为 `imull`；

**三操作数格式 `imull $k, S, D`**：$D \leftarrow S \times k$，$k$ 是立即数，源与目的可以是不同的寄存器。

**单操作数格式 `imull S`**：$\%edx\!:\!\%eax \leftarrow \%eax \times S$，保留完整的 64 位乘积，高 32 位在 `%edx`，低 32 位在 `%eax`；无符号版本是 `mull S`。

**后缀与位宽**：`imulw` / `imull` / `imulq` 分别做 16、32、64 位乘法；写入 32 位寄存器时 `%rax` 等的高 32 位清零。
""")
    figure(p, "int-alu", 1120)
    p.notes("""
图中是 imull 在 ALU 中的执行：D 与 S 进入乘法器，得到 64 位乘积，两半的去向由格式决定。二操作数与三操作数格式只把低 32 位写回 D，高 32 位丢弃，与 C 语言 int 乘法的截断一致；单操作数格式把高 32 位写入 %edx、低 32 位写入 %eax，C 代码中 (long)a * b 这类表达式会用到。
三操作数格式 imull $k, S, D 是 x86-64 少有的目的操作数可以与源不同的算术指令。
""")


def int_arith_2(p):
    p.title('整数加法指令：addl')
    slide(p, r"""
**格式 `addl S, D`**：
- 语义：$D \leftarrow D + S$；$S$ 可以是立即数、寄存器或内存操作数，$D$ 可以是寄存器或内存操作数，二者不能同时是内存操作数；
- 32 位二进制加法，结果写回 $D$，超出 32 位的进位丢弃；
- 有符号与无符号加法的位运算相同。

**三种写法示例**：
- `addl %edx, %eax`：寄存器加寄存器，单次乘加中累加 `sum`；
- `addl $1, %eax`：加立即数，`i++` 编译为这一条指令；
- `addl %eax, (%rdi)`：目的是内存操作数，读出 `*w`、相加、写回，一条指令完成 `*w += t`。
""")
    p.notes('加法指令的格式、位宽与标志位。上一页的 ALU 示意图同样适用于 addl：操作数进入加法器，和写回寄存器，标志位写入 RFLAGS。')


def insn_table(p):
    p.title('其他算术与逻辑指令')
    slide(p, r"""
**指令一览**（`w` / `l` / `q` 为 16 / 32 / 64 位版本；操作数规则与 `imull`、`addl` 相同）：
""")
    p.table([
        ['`leaq S, D`', '$D \\leftarrow \\&S$', '计算地址，不访问内存'],
        ['`incl`、`decl`、`negl`、`notl D`', '$D \\leftarrow D \\pm 1$、$-D$、$\\sim D$', '一元运算'],
        ['`addl`、`subl S, D`', '$D \\leftarrow D + S$、$D - S$', '加、减'],
        ['`imull S, D`', '$D \\leftarrow D \\times S$', '乘，保留低 32 位'],
        ['`andl`、`orl`、`xorl S, D`', '$D \\leftarrow D \\,\\&\\, S$、$D \\mid S$、$D \\oplus S$', '按位与、或、异或'],
        ['`sall`、`sarl`、`shrl k, D`', '$D \\leftarrow D \\ll k$、$D \\gg k$', '左移补 0；算术右移补符号位，逻辑右移补 0'],
        ['`imull`、`mull S`；`cltd`；`idivl`、`divl S`', '$\\%edx\\!:\\!\\%eax \\leftarrow \\%eax \\times S$；$\\%eax \\leftarrow$ 商，$\\%edx \\leftarrow$ 余数', '完整 64 位乘积；除法前 `cltd` 把 `%eax` 符号扩展到 `%edx`'],
    ], headers=['指令', '效果', '说明'], widths=[3.6, 3.2, 3.2])
    p.notes("""
一元指令只有一个操作数，二元指令的目的操作数同时是一个源。移位量 k 是立即数或 %cl 寄存器。
sarl 与 shrl 的区别对应 C 语言中 int 与 unsigned 的右移：int 右移补符号位（除以 2^k 向下取整），unsigned 右移补 0。
除法只有单操作数格式，被除数固定在 %edx:%eax 中；cltd 把 %eax 符号扩展到 %edx，64 位版本是 cqto 与 idivq。
""")


def xor_strength(p):
    p.title('指令优化：xorl 寄存器清零')
    slide(p, r"""
**编译器优化：`xorl %eax, %eax` 寄存器清零**：
- 任何数异或自身恒为 0（$A \oplus A = 0$）；
- **为什么不用 `movl $0, %eax`**：
  - 编码长度：`xorl %eax, %eax` 仅需 2 字节（`31 c0`），而 `movl $0, %eax` 占用 5 字节（`b8 00 00 00 00`）；
  - 硬件重命名优化：现代 CPU 译码器识别到自异或时，直接在寄存器别名表（RAT）中将其映射至物理零，**执行延迟为 0 周期，不占用 ALU 资源**。
""")
    p.notes('编译器在基础算术中常用的机器级优化惯用法：异或清零与乘常数强度削减。')


def xor_strength_2(p):
    p.title('指令优化：整数乘法优化')
    slide(p, r"""
**编译器优化**：
- 整数乘法 `imull` 耗时通常需 3 个时钟周期；
- 编译器会自动将常数乘法拆解为组合开销为 1 周期的 `lea` 与移位：
  - `x * 5` $\to$ `leal (%rax,%rax,4), %eax`（$x + x \times 4$）；
  - `x * 9` $\to$ `leal (%rax,%rax,8), %eax`（$x + x \times 8$）；
  - `x * 12` $\to$ `leal (%rax,%rax,2), %eax` 随后 `sall $2, %eax`。
""")
    figure(p, "xor-lea", 1120)


def insn_bytes(p):
    p.title('指令编码：指令的字节由"操作码"与"操作数"组成')
    slide(p, r"""
**指令的字节表示**：一条机器指令是内存中一段连续的字节，由操作码与操作数字段组成。
""")
    figure(p, "insn-bytes", 1120)
    slide(p, r"""
x86-64 指令的开头一般是操作码。CPU 由操作码识别这是哪一条指令，并据此读取其后长度可变的操作数字段。
""")
    p.notes("""
x86-64 的三条指令，字段的边界与字节的边界重合：`ret` 只有操作码；`movl $0, %eax` 的操作码 `b8` 含寄存器编号，其后是 4 字节立即数；`xorl %eax, %eax` 的第二个字节描述两个寄存器操作数。
`c0` 的二进制是 `11 000 000`：前 2 位表示两个操作数都是寄存器，后两段各 3 位，是两个 `%eax` 的编号。带内存操作数的指令把寄存器编号与寻址方式合并编码在操作数字节中，规则见 Intel 手册第 2 卷第 2 章，本讲不展开。
Y86-64 是教材 CS:APP 第 4 章为教学设计的指令集，指令的功能与 x86-64 相近，编码规则更简单：第 1 字节是操作码，第 2 字节的两个十六进制位是两个寄存器编号（F 表示没有寄存器），其后是 8 字节常数。`irmovq $0, %rax` 与 `movl $0, %eax` 是同一操作：把常数 0 写入寄存器 `%rax`。Y86-64 只有 64 位运算，常数占 8 字节，所以整条指令 10 字节，x86-64 的 5 字节编码更紧凑。寄存器编号与 x86-64 相同：`%rax` 为 0。
x86-64 的字节可用 `echo 'ret; movl $0, %eax; xorl %eax, %eax' | gcc -c -x assembler - -o t.o && objdump -d t.o` 得到。
""")


def recap_part1(p):
    p.title('前情回顾：一行 C 代码如何运行在 CPU 上')
    p.code('c', """int a = 1, b = 6, y;   // 全局变量

void add(void) {
    y = a + b;
}""")
    p.demo('编译并反汇编',
           """cd examples
gcc -O0 -fomit-frame-pointer -fcf-protection=none -no-pie add.c -o add
objdump -d add | sed -n '/<add>:/,/ret/p'""",
           output="""0000000000401106 <add>:
  401106:	8b 15 1c 2f 00 00    	mov    0x2f1c(%rip),%edx        # 404028 <a>
  40110c:	8b 05 1a 2f 00 00    	mov    0x2f1a(%rip),%eax        # 40402c <b>
  401112:	01 d0                	add    %edx,%eax
  401114:	89 05 1a 2f 00 00    	mov    %eax,0x2f1a(%rip)        # 404034 <y>
  40111a:	90                   	nop
  40111b:	c3                   	ret""",
           bold=[2, 3, 4, 5],
           files=['examples/add.c'])
    p.notes("""
第一部分的小结。先只看代码与反汇编：y = a + b 变成两条 movl 读 a、b，一条 addl 相加，一条 movl 写回 y；nop 是 -O0 留下的空操作，ret 返回 main。
objdump 每行三栏：指令的内存地址、机器码字节、汇编助记符；行尾的 # 404028 <a> 是 objdump 算出的 a(%rip) 实际地址。
编译选项：-O0 让两次读取与加法分开成三条指令；-fomit-frame-pointer 去掉 push/pop %rbp（第三部分再讲栈帧）；-no-pie 让文件中的地址就是装入内存后的地址；-fcf-protection=none 去掉 endbr64。
下一页用示意图把可执行文件、内存与 CPU 串起来。
""")


def recap_part1_fig(p):
    p.title('前情回顾：一行 C 代码如何运行在 CPU 上')
    figure(p, "recap-part1", 1120)
    p.notes("""
串起已经讲过的三个重点：编译得到可执行文件（代码段与数据段）；装入内存后指令与数据都是带地址的字节（整数的小端补码编码、指令的机器码编码）；CPU 以 PC 取指、把 a 与 b 读入寄存器、在 ALU 中相加、再写回 y。
图中 CPU 的快照取在 addl 执行时：a、b 已读入 %edx、%eax，y 尚未写回。地址、机器码与上一页 objdump 的输出一致；a(%rip) 的位移量 0x2f1c = 0x404028 − 0x40110c，即目标地址减去下一条指令的地址。
y 未初始化，放在 .bss；0x404030 处是 libc 的一个 1 字节标志，所以 y 在 0x404034 而不是紧跟 b。
""")


def disasm_exercise(p):
    p.title('练习：反汇编和反编译')
    figure(p, "x86-rules", 1120)
    slide(p, r"""
**第一步，反汇编**：按上面的规则切分函数 `mac` 的机器码，写出每条指令的汇编。
""")
    p.demo('编译 mac.c，查看 mac 的机器码',
           """cd examples
gcc -Og -fcf-protection=none -c mac.c
objcopy -O binary -j .text mac.o /dev/stdout | od -An -tx1""",
           output=""" 8b 06 0f af 07 01 d0 c3""",
           files=['examples/mac.c'])
    p.notes("""
课堂练习。`mac.c` 中只有函数 `mac`。`objcopy -O binary -j .text` 从 `mac.o` 中取出代码段的字节，`od -An -tx1` 按十六进制逐字节打印，共 8 个字节、4 条指令。
规则只列出本题用到的 4 条指令，以及操作数字节的两种模式：11（寄存器）与 00（内存，地址在寄存器中）。
做法是重复三步：读取操作码，按规则判断其后有没有操作数字节，把操作数字节写成二进制并分成三段。
操作数字节在 Intel 手册中称为 ModR/M。mm 为 01、10 时地址中还有偏移；mm 为 00 且 bbb 为 100 或 101 时另有规则，本题不涉及。
""")


def disasm_exercise_2(p):
    p.title('练习：反汇编和反编译')
    slide(p, r"""
**反汇编过程**：读取操作码，再把操作数字节写成二进制并分成三段：
""")
    p.code('text', """8b 06       06 = 00 000 110    movl  (%rsi), %eax
0f af 07    07 = 00 000 111    imull (%rdi), %eax
01 d0       d0 = 11 010 000    addl  %edx, %eax
c3                             ret""")
    slide(p, r"""
- `8b 06`：`movl B, R`。mm 是 00，B 是内存操作数，地址在 110 号寄存器 `%rsi` 中；R 是 000 号寄存器 `%eax`；
- `0f af 07`：`imull B, R`，操作码占 2 字节。B 是 `(%rdi)`，R 是 `%eax`；
- `01 d0`：`addl R, B`。mm 是 11，B 是 000 号寄存器 `%eax`；R 是 010 号寄存器 `%edx`；
- `c3`：`ret`，没有操作数字节。
""")
    p.notes('8 个字节切分为 2、3、2、1 字节的 4 条指令。`objdump -d` 完成的就是这一过程。')


def mac_exercise(p):
    p.title('练习：反汇编和反编译')
    slide(p, r"""
**第二步，反编译**：由第一步得到的函数 `mac` 的汇编代码，推导其 C 函数体：
""")
    p.demo('编译 mac.c，查看 mac 的汇编代码',
           """cd examples
gcc -Og -fcf-protection=none -S mac.c -o - | sed -f asm.sed""",
           output="""mac:
	movl	(%rsi), %eax
	imull	(%rdi), %eax
	addl	%edx, %eax
	ret""",
           files=['examples/mac.c', 'examples/asm.sed'])
    slide(p, r"""
**已知**：
- 函数原型：`int mac(const int *w, const int *x, int sum)`；
- 执行前，`%rdi` 保存指针 `w`，即 `w[0]` 的地址；`%rsi` 保存指针 `x`，即 `x[0]` 的地址；`%edx` 保存 `sum` 的值；
- 执行 `ret` 时，`%eax` 中的值即函数的返回值。
""")
    p.notes('课堂练习：已知寄存器内容，由单次乘加的汇编推导 C 函数体。编译器输出的 4 条指令与第一步反汇编的结果相同。')


def mac_exercise_2(p):
    p.title('练习：反汇编和反编译')
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


def loop_need(p):
    p.title('向量内积和控制流')
    slide(p, r"""
**回顾核心代码**：
""")
    p.code('c', """for (int i = 0; i < n; i++) {
    sum += w[i] * x[i];
}""")
    slide(p, r"""
**顺序执行**：程序计数器（PC，即 `%rip`）存放下一条要取出的指令的地址。CPU 从该地址取出指令并执行，同时把 `%rip` 加上这条指令的长度，下一次取指因此从紧接着的那条指令开始：
$$\%rip \leftarrow \%rip + \text{指令长度}$$
""")
    figure(p, "pc-increment", 1120)
    p.notes("""
第二部分从循环引入控制流。先说明顺序执行：%rip 的递增由 CPU 在执行每条指令时完成，程序中没有对应的指令。
图中是 dot_product（gcc -Og）循环体的 7 条指令在内存中的位置，地址与长度来自 objdump -d dot.o。按地址递增依次取指，执行流只会前进到更高的地址。
""")


def loop_need_2(p):
    p.title('向量内积和控制流')
    p.code('text', """sum += w[0] * x[0];        movl    (%rsi), %eax
                           imull   (%rdi), %eax
                           addl    %edx, %eax""")
    slide(p, r"""
**只有顺序执行时**：三条指令完成一次乘加，执行流前进到下一条指令，不会回到这三条指令。
- 完成 4096 次乘加，就要把同样的三条指令写 4096 份，一份接一份地放在内存中；
- 代码长度随 $n$ 增长；$n$ 在运行时才确定的程序无法这样写出。
""")
    figure(p, "loop-unroll", 1120)
    slide(p, r"""
**需要的能力**：一次乘加执行完后，让 `%rip` 回到这段指令的开头再执行一次，并在执行了 $n$ 次之后停下来。改写 `%rip` 的指令称为跳转指令。
""")
    p.notes("""
页面上方的代码块是第一部分的单次乘加：左边是 C 代码，右边是它对应的三条指令，%rdi 保存 w，%rsi 保存 x，%edx 保存 sum。
某大模型（Qwen3-8B）的隐藏层维度是 4096，一次向量内积要做 4096 次乘加。
展开 4096 份的代码可以写出，但 n 是 dot_product 的参数，调用时才知道，展开的份数无法在编译时确定。
""")


def loop_asm(p):
    p.title('循环的汇编：两条跳转指令')
    p.side_image("assets/loop-jumps.svg", width="40%", alt="contain", side="left")
    p.demo('用 gcc -Og 编译 dot.c',
           """cd examples
gcc -Og -fcf-protection=none -S dot.c -o - \\
    | sed -f asm.sed""",
           files=['examples/dot.c', 'examples/asm.sed'])
    slide(p, r"""
- `%eax` 保存 `i`，`%r9d` 保存 `sum`，`%edx` 保存 `n`，`%rdi` 与 `%rsi` 保存 `w` 与 `x`（参数与寄存器的对应规则在第三部分讲解）；
- `.L3:`、`.L2:` 是标号：紧随其后那条指令的地址的名字，汇编器把它换算成地址；
- 循环体是 `.L3` 之后的 5 条指令：`movslq` 把 `i` 扩展为 64 位，`movl` 与 `imull` 读出 `x[i]` 并乘以 `w[i]`，两条 `addl` 累加 `sum` 并完成 `i++`；
- 循环体之后，`cmpl` 比较 `i` 与 `n`，`jl` 决定是否再执行一次循环体。
""")
    p.notes("""
左侧的清单是这条命令的输出，加上两条跳转指令的箭头。命令上方的 dot.c 按钮在右侧打开源码，与左侧的汇编逐行对照。
循环体的 5 条指令：movslq 把 i 扩展为 64 位下标，movl 读出 x[i]，imull 乘以 w[i]，addl 累加到 sum，addl $1 完成 i++。
与第一部分的单次乘加相比，数组下标由 i 决定，因此 movl 与 imull 使用 (%rsi,%r8,4) 这种带变址的内存操作数。
""")


def loop_asm_2(p):
    p.title('循环的汇编：两条跳转指令')
    p.side_image("assets/loop-jumps.svg", width="40%", alt="contain", side="left")
    p.demo('用 gcc -Og 编译 dot.c',
           """cd examples
gcc -Og -fcf-protection=none -S dot.c -o - \\
    | sed -f asm.sed""",
           files=['examples/dot.c', 'examples/asm.sed'])
    slide(p, r"""
- **`jl .L3`，条件跳转**：`i < n` 时把 `.L3` 的地址写入 `%rip`，代替 `%rip + 指令长度`，执行流回到循环体开头；否则 `%rip` 照常递增，执行 `jl` 之后的两条指令，返回 `sum`；
- **`jmp .L2`，无条件跳转**：进入循环前跳到 `.L2`，先比较，再决定是否执行循环体；`n` 为 0 时循环体一次也不执行。

**问题**：`jl` 的操作数只有一个标号，`i < n` 的比较结果从哪里来？
""")
    p.notes("""
跳转指令把目标标号的地址写入 %rip。其余指令执行后 %rip 仍按长度递增，只有跳转指令改写它。
对照 dot.c：jl .L3 对应 for 的条件 i < n，jmp .L2 对应第一次进入循环前的条件检查。
jl 读取的是 cmpl 留下的标志位，下面几页说明标志位、cmp 与 test 指令，再回到 jl 这一类条件跳转指令。
""")


def jump_insn(p):
    p.title('跳转指令：jmp 与 jl 如何改写 %rip')
    slide(p, r"""
**无条件跳转 `jmp Label`**：
- 执行时把 `Label` 的地址写入 `%rip`，下一条执行的指令即 `Label` 处的指令；
- 没有条件，总是跳转；指令本身仅修改 PC `%rip` 寄存器。

**条件跳转 `jl Label`**（jump if less）：
- 条件成立时与 `jmp Label` 相同；条件不成立时 `%rip` 按指令长度递增，执行下一条指令；
- 条件来自最近的一条算术或比较指令：`cmpl %edx, %eax` 计算 `%eax - %edx`，把结果的特征记录在 CPU 的标志位中，`jl` 读取标志位判断 `%eax < %edx` 是否成立。

**问题**：标志位记录结果的哪些特征？`cmpl` 如何设置它们？
""")
    p.notes("""
跳转指令与数据传送、算术指令一样是一条普通的指令：取指、执行、更新 %rip。区别只在更新 %rip 的方式。
""")


def rflags(p):
    p.title('标志位：运算结果的特征记录在 RFLAGS 寄存器中')
    slide(p, r"""
**标志位（条件码，RFLAGS 寄存器中的单个比特）**：记录最近一次算术或逻辑运算结果的特征：
- **ZF（Zero Flag）**：结果为 0 时置 1；
- **SF（Sign Flag）**：结果为负数（最高位为 1）时置 1；
- **CF（Carry Flag）**：最高位产生进位或借位时置 1，即按无符号数解释时溢出；
- **OF（Overflow Flag）**：按补码有符号数解释时溢出置 1。以加法为例：两个加数同号（同正或同负），和的符号与它们相反。

**标志位由运算指令顺带设置**：`addl %ecx, %r9d` 在写回结果的同时设置四个标志位，程序中没有单独设置标志位的指令。
""")
    p.notes('标志位 ZF、SF、CF、OF 的定义。每条算术指令在写回结果的同时设置它们，条件跳转指令读取它们。')


def rflags_2(p):
    p.title('标志位：运算结果的特征记录在 RFLAGS 寄存器中')
    slide(p, r"""
**各类指令对标志位的影响**：
- 算术与逻辑指令（`add`、`sub`、`imul`、`and`、`or`、`xor`）按结果更新标志位；
- `lea` 与 `mov` 只传送地址与数据，**不改变任何标志位**；
- `inc` / `dec` 更新 ZF、SF、OF，**保留 CF 不变**；
- 逻辑指令（`and` / `or` / `xor` / `test`）执行后把 CF 与 OF 清零。

**CF 与 OF 的区别，以 8 位二进制加法为例**：
- `0xFF + 0x01 = 0x00`：无符号 $255+1=256$ 溢出（CF=1）；有符号 $-1+1=0$ 无溢出（OF=0）；
- `0x7F + 0x01 = 0x80`：无符号 $127+1=128$ 无溢出（CF=0）；有符号 $127+1=-128$，两个正数相加得到负数，溢出（OF=1）。
""")
    p.notes('同一次加法同时设置 CF 与 OF：CF 按无符号解释判断溢出，OF 按有符号解释判断溢出。类型的区别由随后的条件跳转指令读取哪一个标志位体现。')


def cmp_test(p):
    p.title('比较指令：cmp 与 test 只设置标志位')
    slide(p, r"""
**比较指令 `cmpl S2, S1`**：
- 计算 $S1 - S2$，按差设置 ZF、SF、CF、OF；**差本身丢弃，两个操作数保持原值**；
- `cmpl %edx, %eax` 计算 `i - n`：差为负则 SF=1，差为 0 则 ZF=1。

**测试指令 `testl S2, S1`**：
- 计算 $S1 \ \& \ S2$ 的按位与，按结果设置 ZF 与 SF，CF 与 OF 清零；**结果丢弃**；
- `testl %edx, %edx`：`n` 与自身按位与仍是 `n`，于是 ZF 表示 `n` 是否为 0，SF 表示 `n` 是否为负。
""")
    figure(p, "cmp-test", 1120)
    p.notes('cmp 与 test 是只设置标志位的减法与按位与。比较两个数就是做减法看差的符号与是否为零。')


def cmp_test_2(p):
    p.title('比较指令：cmp 与 test 只设置标志位')
    slide(p, r"""
**实例：`gcc -O2` 编译的 `dot_product` 在函数入口检查 $n$**（`%edx` 保存 $n$）：
""")
    p.demo('编译 dot.c，查看函数入口与跳转目标 .L4（中间的循环略去）',
           """cd examples
gcc -O2 -fcf-protection=none -S dot.c -o - | sed -f asm.sed | sed -n '1,4p;/^\\.L4:/,$p'""",
           output="""dot_product:
	movq	%rsi, %r8
	testl	%edx, %edx
	jle	.L4
.L4:
	xorl	%esi, %esi
	movl	%esi, %eax
	ret""",
           bold=[3, 4],
           files=['examples/dot.c', 'examples/asm.sed'])
    slide(p, r"""
- `testl %edx, %edx` 按 $n$ 设置标志位：$n = 0$ 时 ZF=1，$n < 0$ 时 SF=1；`jle`（小于等于时跳转）读取这两个标志位，$n \le 0$ 时跳到 `.L4`，以返回值 0 返回；
- `dot.c` 的 `for` 循环在第一次执行循环体之前检查 `i < n`；`-O2` 把这次检查放在函数入口，`testl` 与 `jle` 两条指令的效果等同于 `if (n <= 0) return 0;`。
""")
    p.notes("""
-O2 把循环改为先检查一次 n，再进入底部判断的循环；这里只看入口的两条指令。
输出的前 4 行是函数入口，后 4 行是跳转目标 .L4，两段之间是循环。sed -n '1,4p;/^\\.L4:/,$p' 打印第 1 到 4 行，以及从 .L4: 到末尾的各行。
dot.c 按钮在右侧打开源码：源码中没有 if (n <= 0) 这一行，它来自 for 循环第一次的条件检查。
""")


def cond_jump(p):
    p.title('条件跳转指令：读取标志位的组合')
    slide(p, r"""
**条件跳转指令 `jX Label`，按 X 读取不同的标志位**：
1. **相等与零值**：`je` / `jz`（ZF=1），`jne` / `jnz`（ZF=0）；
2. **有符号比较**（`cmpl S2, S1` 之后，判断 $S1$ 与 $S2$ 的大小）：
   - `jl`：小于，条件为 $\text{SF} \ne \text{OF}$；
   - `jle`：小于等于，条件为 $(\text{SF} \ne \text{OF}) \lor (\text{ZF}=1)$；
   - `jg`：大于，条件为 $(\text{SF} = \text{OF}) \land (\text{ZF}=0)$；
   - `jge`：大于等于，条件为 $\text{SF} = \text{OF}$；
3. **无符号比较**：
   - `jb`：below，无符号小于，条件为 $\text{CF}=1$；
   - `jbe`：无符号小于等于，条件为 $\text{CF}=1 \lor \text{ZF}=1$；
   - `ja`：above，无符号大于，条件为 $\text{CF}=0 \land \text{ZF}=0$；
   - `jae`：无符号大于等于，条件为 $\text{CF}=0$。
""")
    p.notes("""
有符号小于的条件是 SF ≠ OF：差不溢出时 OF=0，SF 即差的符号，SF=1 表示小于；差溢出时 OF=1，SF 与差的真实符号相反，SF=0 表示小于。两种情况合起来就是 SF ≠ OF。
同一条 cmpl 之后，编译器按操作数的类型选用 jl 或 jb：int 用 jl，unsigned 用 jb。
""")


def jump_direct(p):
    p.title('直接跳转与间接跳转')
    slide(p, r"""
**直接跳转（Direct Jump）**：`jmp .L2`、`jl .L3` 的目标是一个标号，目标地址在汇编与链接时确定，作为指令的一部分编码在指令中。`dot_product` 中的跳转都是直接跳转。

**目标在运行时才确定的情况**：
- 执行哪一段代码取决于运行时的一个值，例如 `switch (op)` 按 `op` 的值选择分支，`op` 是函数的参数；
- 一条直接跳转只能写死一个目标，表示不了"按 `op` 的值选择目标"。

**间接跳转（Indirect Jump）**：目标地址从寄存器或内存中读出，写入 `%rip`：
- `jmp *%rax`：`%rax` 中的值即目标地址；
- `jmp *(%rax)`：从地址 `%rax` 处读出 8 字节作为目标地址；
- `jmp *.L4(,%rdi,8)`：从地址 $\text{.L4} + 8 \times \text{\%rdi}$ 处读出 8 字节作为目标地址，寻址方式与第一部分的内存操作数相同。
""")
    p.notes("""
间接跳转的目标是数据：寄存器中的值或内存中的 8 字节。C 语言的函数指针调用、C++ 的虚函数调用也编译为间接跳转或间接调用。
""")


def switch_table(p):
    p.title('switch 语句：跳转表与间接跳转')
    p.code('c', """int calc(int op, int a, int b) {
    switch (op) {
    case 0: return a + b;   case 1: return a - b;   case 2: return a * b;
    case 3: return a & b;   case 4: return a | b;   case 5: return a ^ b;
    }
    return 0;
}""")
    slide(p, r"""
**6 个 `case`，6 段代码**：执行哪一段由参数 `op` 决定，编译时不知道 `op` 的值。
- 逐一比较：`op` 与 0、1、…、5 依次 `cmpl` 并条件跳转，最多比较 6 次；
- 跳转表：把 6 段代码的地址排成一张表，按 `op` 取出第 `op` 个地址，跳过去。
""")
    p.notes("""
switch 语句是间接跳转最常见的来源。case 的值连续且个数足够时（gcc 约 5 个起），编译器生成跳转表，否则逐一比较。
""")


def switch_table_2(p):
    p.title('switch 语句：跳转表与间接跳转')
    p.demo('编译 calc.c，查看函数入口与跳转表',
           """cd examples
gcc -O1 -fno-pie -fcf-protection=none -S calc.c -o - | sed -e '/\\.quad/b' -f asm.sed | sed -n '1,12p'""",
           output="""calc:
	cmpl	$5, %edi
	ja	.L10
	movl	%edi, %edi
	jmp	*.L4(,%rdi,8)
.L4:
	.quad	.L9
	.quad	.L8
	.quad	.L7
	.quad	.L6
	.quad	.L5
	.quad	.L3""",
           bold=[5],
           files=['examples/calc.c', 'examples/asm.sed'])
    slide(p, r"""
`%edi` 保存 `op`；`.L9` 到 `.L3` 是 6 个 `case` 的代码，`.L10` 是 `return 0`。`.quad` 一行在内存中放一个 8 字节的地址。
""")
    p.notes("""
sed 的第一个表达式保留 .quad 行，asm.sed 删去其余伪指令；sed -n '1,12p' 只取入口与跳转表，其后是六段 case 代码，各自以 ret 结束。
-fno-pie 使跳转表中直接存放绝对地址；默认的 PIE 编译存放的是相对偏移，入口多两条指令，间接跳转写为 jmp *%rax。
""")


def switch_table_3(p):
    p.title('switch 语句：跳转表与间接跳转')
    figure(p, "jump-table", 1000)
    slide(p, r"""
- **范围检查**：`cmpl $5, %edi` 与 `ja .L10`，`op` 不在 0 到 5 之间时跳到 `return 0`；按无符号比较，负数被视为很大的数，一次比较同时排除负数与过大的值；
- **取表项**：`movl %edi, %edi` 把 `op` 零扩展为 64 位下标，`jmp *.L4(,%rdi,8)` 从第 `op` 项读出地址，写入 `%rip`；
- **为什么是间接跳转**：目标随 `op` 变化，直接跳转表示不了。与逐一比较相比（最多 6 次 `cmpl` 与 `je`），跳转表用一次比较、一次访存与一次跳转完成，执行时间与 case 的个数无关。
""")
    p.notes("""
跳转表 .L4 的 6 个项由汇编器写入只读数据段，第 op 项存放 case op 的代码的地址；movl %edi, %edi 零扩展的依据是第一部分的位宽规则。
case 的值稀疏或个数少时，编译器仍用逐一比较；gcc 对连续的 case 在 case 数达到 5 个左右时生成跳转表。
""")


def cmov(p):
    p.title('sidebar：条件传送 cmov 与条件设置 setX')
    slide(p, r"""
**读取标志位的指令除条件跳转外还有两类**：

**条件设置指令 `setX D`**：按标志位把单字节目标 `D` 置为 0 或 1。第一部分 `equal` 的汇编中，`cmpl %esi, %edi` 之后的 `sete %al` 把"相等"写入 `%al`，`movzbl %al, %eax` 再扩展为返回值。

**条件传送指令 `cmovX S, D`**（Conditional Move）：
- 条件成立时把 `S` 复制到 `D`，否则 `D` 保持原值；
- 两个候选值先分别放入寄存器，`cmovX` 依据标志位选出其中一个，指令序列中没有跳转指令与标号。

**示例**：`int max(int a, int b) { int v = a; if (a < b) v = b; return v; }`
- `gcc -Og` 用条件跳转 `jl` 实现选择；
- `gcc -O2` 改用 `cmovge`，少了一条跳转指令。
""")
    p.notes("""
拓展内容。setX 与 cmovX 的 X 与条件跳转的 X 相同：e、ne、l、le、g、ge、b、a 等。
cmov 在候选值都已算出、选择本身很简单时有优势；第四讲说明跳转指令对流水线的影响。
""")


def cmov_fig(p):
    p.title('sidebar：条件传送 cmov 与条件设置 setX')
    slide(p, r"""
`%edi` 保存参数 `a`，`%esi` 保存参数 `b`，`%eax` 保存返回值（对应规则在第三部分讲解）。
""")
    figure(p, "cmov-mux", 1120)


def procedure_need(p):
    p.title('函数调用：main 如何调用 dot_product')
    p.code('c', """int dot_product(const int *w, const int *x, int n);
int main(void) {
    int w[4] = {1, 2, 3, 4};
    int x[4] = {5, 6, 7, 8};
    return dot_product(w, x, 4);
}""")
    slide(p, r"""
**已经学过的**：一个函数内部的执行。数据传送与算术指令完成一次乘加，标志位与跳转指令实现循环，`dot_product` 的循环由这些指令组成。

**新的问题**：执行如何跨越两个函数。`main` 执行到 `dot_product(w, x, 4)` 时，CPU 转去执行 `dot_product` 的指令；`dot_product` 执行完毕后，CPU 回到 `main` 继续执行。

**两个术语**：发起调用的函数称为**调用者（caller）**，被调用的函数称为**被调用者（callee）**。这次调用中 `main` 是调用者，`dot_product` 是被调用者。
""")
    p.notes("""
第三部分的例子是 main 调用 dot_product。前两部分讲的是一个函数内部的指令，这一部分讲两个函数之间的配合。
CS:APP 把函数称为过程（procedure），调用者与被调用者的说法相同。
""")


def call_vs_jump(p):
    p.title('函数调用与跳转的相同点与不同点')
    slide(p, r"""
**相同点**：调用把执行流从 `main` 的指令转移到 `dot_product` 的第一条指令。这是一次无条件跳转，`jmp dot_product` 就能做到。

**不同点**：一次调用还要完成四件事，跳转指令都不做：
1. **返回**：`dot_product` 执行完毕后，执行流回到 `main` 中调用点的下一条指令；
2. **传递数据**：`main` 把参数 `w`、`x`、`4` 交给 `dot_product`，`dot_product` 把返回值交给 `main`；
3. **寄存器**：两个函数使用同一组 16 个寄存器，`main` 放在寄存器中的值可能被 `dot_product` 改写；
4. **局部变量**：`main` 的数组 `w` 与 `x` 在调用期间保持原值，`dot_product` 的 `sum` 与 `i` 只在这次调用期间存在。
""")
    p.notes("""
函数调用首先是一次跳转，在跳转之外多出返回、传递数据、寄存器、局部变量四件事。第三部分按这五件事的顺序展开。
""")


# The five problems of a call, in the order the part solves them: name, what
# has to hold, how it is solved. The checklist page comes back at the head of
# each subsection with one more row filled in.
CALL_PROBLEMS = [
    ('调用被调用者', '执行流进入被调用者的第一条指令', '`call` 指令'),
    ('返回调用者', '执行流回到调用点的下一条指令', '`ret` 指令'),
    ('传递数据', '参数交给被调用者，返回值交给调用者', '约定：寄存器与栈'),
    ('寄存器', '调用者放在寄存器中的值，调用之后仍可使用', '约定：调用者保存与被调用者保存'),
    ('局部变量', '每次调用有自己的一份局部变量', '在栈帧中分配与释放'),
]


def call_checklist_table(p, done, current=1):
    p.title('函数调用的实现：五个问题')
    rows = []
    for k, (name, goal, answer) in enumerate(CALL_PROBLEMS):
        if k < done:
            rows.append([name, goal, '✓ ' + answer])
        elif k < done + current:
            rows.append([f'**{name}**', f'**{goal}**', '**?**'])
        else:
            rows.append([name, goal, '?'])
    p.table(rows, headers=['问题', '要做到的事', '解决的方式'], widths=[2, 5, 4])


def call_checklist(p):
    call_checklist_table(p, 0, current=2)
    slide(p, r"""
**五个问题的来源**：第一个是跳转本身，后四个是调用比跳转多出的四件事。每解决一个问题，表中相应的一行填上解决的方式。

**先解决前两个问题**：执行流如何进入 `dot_product`，执行完毕后又如何回到 `main`？
""")
    p.notes("""
这张表在后面每一小节的开头出现一次，已经解决的行标上对勾，加粗的一行是这一小节要解决的问题。
""")


def return_address(p):
    p.title('返回地址：调用点下一条指令的地址')
    slide(p, r"""
**回到调用者**：`dot_product` 执行完毕后跳到调用点的下一条指令。
- `dot_product` 可以在程序的多处被调用，各个调用点的下一条指令地址不同；
- `jmp` 的目标写在指令中，只能是其中一个；
- 调用发生时因此要把调用点下一条指令的地址保存起来，返回时跳到保存的地址。这个地址称为**返回地址（Return Address）**。
""")
    figure(p, "return-address", 1000)
    slide(p, r"""
**问题**：返回地址保存在哪里？
""")
    p.notes("""
进入 dot_product 用已有的指令就能做到：jmp dot_product 把它第一条指令的地址写入 %rip。回到调用者是新的问题。
调用点 A 在 main 中，调用点 B 在另一个函数中，两处都调用 dot_product。dot_product 结束时回到哪里由这一次是谁调用决定，编译 dot_product 时无法确定。
返回时跳转的目标是运行时保存下来的一个地址，这是第二部分讲过的间接跳转。
""")


def runtime_stack(p):
    p.title('运行时栈：按后进先出的顺序保存返回地址')
    p.side_image("assets/address-space.svg", width="30%", alt="contain")
    slide(p, r"""
**调用可以嵌套**：`main` 调用函数 `f`，`f` 又调用函数 `g`。
- 两个返回地址同时需要保存：回到 `main` 的地址先保存，回到 `f` 的地址后保存；
- `g` 最先返回，用到的是后保存的那个地址。保存与取出的顺序是后进先出（LIFO）。

**运行时栈（Stack）**：按后进先出的顺序存取数据的一段内存。
- **位置**：在进程地址空间的高地址一端；
- **栈顶指针 `%rsp`**：保存栈顶的地址，即栈中最后存入的数据的地址；
- **向低地址生长**：存入 8 字节时 `%rsp` 减 8，取出 8 字节时 `%rsp` 加 8；
- 地址不低于 `%rsp` 的部分正在使用，地址低于 `%rsp` 的部分尚未使用。
""")
    p.notes("""
返回地址的保存与取出是后进先出的，因此放在栈中。栈是内存中的一段区域，%rsp 是 16 个通用寄存器之一，用来保存栈顶的地址。
右图是进程的地址空间：代码段存放指令，数据段存放全局变量，堆由 malloc 分配，栈在高地址一端。堆向高地址生长，栈向低地址生长，两者之间的空间由先用到的一方使用。
本讲把图中的地址当作内存地址使用，地址的翻译在后面的讲次说明。
""")


def push_pop(p):
    p.title('栈操作指令：pushq 与 popq')
    slide(p, r"""
**`pushq S`**：把 8 字节的 `S` 存入栈顶。
1. `%rsp` 减 8；
2. 把 `S` 写入 `%rsp` 所指的 8 个字节。

**`popq D`**：从栈顶取出 8 字节，写入 `D`。
1. 读出 `%rsp` 所指的 8 个字节，写入 `D`；
2. `%rsp` 加 8。

**操作数、标志位与等效的指令**：
- `S` 可以是寄存器、内存操作数或立即数（32 位，符号扩展为 64 位），`D` 可以是寄存器或内存操作数；
- `pushq` 与 `popq` 不改变标志位；
- 以 `%rbx` 为例：对 `%rsp` 与栈的效果，`pushq %rbx` 与 `subq $8, %rsp`、`movq %rbx, (%rsp)` 两条指令相同，`popq %rbx` 与 `movq (%rsp), %rbx`、`addq $8, %rsp` 两条指令相同；`pushq %rbx` 占 1 字节，等效的两条指令共 8 字节。
""")
    p.notes("""
pushq 与 popq 各用一条指令完成两步：改写 %rsp，读写栈顶。x86-64 的栈以 8 字节为单位，两条指令的操作数都是 64 位。
等效的两条指令中 subq 与 addq 会设置标志位，pushq 与 popq 不会。操作数是内存操作数时没有这样的两条指令：movq 不能有两个内存操作数。
操作数是 %rsp 时，pushq %rsp 压入的是减 8 之前的值。
字节数来自汇编器的输出：pushq %rbx 是 53，popq %rbx 是 5b；subq $8, %rsp 是 48 83 ec 08，movq %rbx, (%rsp) 是 48 89 1c 24。
""")


def push_pop_fig(p):
    p.title('栈操作指令：pushq 与 popq')
    figure(p, "push-pop", 1120)
    p.notes("""
左半是 pushq 的两步：%rsp 先减 8，再把 Src 写入新的栈顶。右半是 popq 的两步：先读出栈顶的 8 个字节，%rsp 再加 8。
popq 之后原来的 8 个字节仍在内存中，%rsp 已经越过它，这 8 个字节不再属于栈中正在使用的部分。
""")


def call_emulate(p):
    p.title('用已有的指令实现调用与返回')
    slide(p, r"""
**调用**：把返回地址压入栈，再跳到 `dot_product`：
""")
    p.code('assembly', """    leaq   .Lnext(%rip), %rax    # the address of the next instruction
    pushq  %rax                  # save it as the return address
    jmp    dot_product
.Lnext:                          # execution resumes here after the return""")
    slide(p, r"""
**返回**：从栈顶弹出返回地址，用间接跳转跳到这个地址：
""")
    p.code('assembly', """    popq   %rcx                  # the return address
    jmp    *%rcx""")
    slide(p, r"""
- 调用用 3 条指令共 13 字节，返回用 2 条指令共 3 字节，各占用一个寄存器；
- 返回地址是下一条指令的地址，CPU 顺序执行时已经在计算它（`%rip` 加指令长度），程序用 `leaq` 又计算了一遍。

**x86-64 为这两件事各提供一条指令**：`call dot_product` 占 5 字节，`ret` 占 1 字节，都不占用通用寄存器。
""")
    p.notes("""
.Lnext(%rip) 是第一部分的 PC 相对寻址，leaq 算出标号 .Lnext 的地址，即 jmp 之后那条指令的地址。
字节数来自汇编器的输出：leaq 7 字节，pushq %rax 1 字节，jmp dot_product 5 字节；popq %rcx 1 字节，jmp *%rcx 2 字节。
call 与 ret 由 CPU 直接完成这两组动作。CPU 还按 call 与 ret 的配对预测返回地址，第四讲说明。
""")


def call_ret(p):
    p.title('call 与 ret：调用与返回的两条指令')
    p.side_image("assets/call-ret.svg", width="42%", alt="contain", side="left")
    slide(p, r"""
**`call Label`** 相当于 `pushq` 返回地址，再 `jmp Label`：
1. `%rsp` 减 8，把 `call` 的下一条指令的地址写入 `%rsp` 所指处；
2. 把 `Label` 的地址写入 `%rip`。

**`ret`** 相当于把栈顶 `popq` 到 `%rip`：
1. 读出 `%rsp` 所指的 8 个字节，`%rsp` 加 8；
2. 把读出的地址写入 `%rip`。

**`call *Operand`** 是间接调用：目标地址从寄存器或内存中读出，例如 `call *%rax`。
""")
    p.notes("""
左图的地址取自 main.c 与 dot.c 链接成的可执行文件：main 中的 call 位于 0x401156，下一条指令 addq $40, %rsp 位于 0x40115b，dot_product 从 0x401160 开始，它的 ret 位于 0x401199。
三个时刻：① 执行 call 之前；② call 之后，返回地址 0x40115b 占栈顶的 8 字节，%rip 指向 dot_product 的入口；③ ret 之后，返回地址已弹出，%rip 回到 0x40115b。
间接调用的写法与间接跳转相同。C 语言通过函数指针的调用编译为间接调用。
""")


def call_ret_2(p):
    p.title('call 与 ret：调用与返回的两条指令')
    p.side_image("assets/call-ret.svg", width="42%", alt="contain", side="left")
    p.demo('查看 call 与栈顶的返回地址',
           """cd examples
gcc -Og -fcf-protection=none -no-pie \\
    -fno-stack-protector -o main main.c dot.c
objdump -d --no-show-raw-insn main \\
    | grep -A1 'call.*<dot_product>'
gdb -q -batch -ex 'break *dot_product' \\
    -ex run -ex 'x/gx $rsp' main 2>&1 | tail -1""",
           output="""  401156:	call   401160 <dot_product>
  40115b:	add    $0x28,%rsp
0x7fffffffd7a8:	0x000000000040115b""",
           files=['examples/main.c', 'examples/dot.c'])
    slide(p, r"""
- **`objdump` 的两行**：`call` 位于 `0x401156`，占 5 字节，下一条指令位于 `0x40115b`；
- **`gdb` 的一行**：程序停在 `dot_product` 的入口，`%rsp` 所指的 8 个字节是 `0x40115b`，即返回地址。
""")
    p.notes("""
-no-pie 使可执行文件中的地址就是装入内存后的地址。objdump -d 反汇编，--no-show-raw-insn 不打印机器码字节，grep -A1 取出 call 一行与它的下一行。
gdb 的三个 -ex 依次执行三条命令：在 dot_product 的第一条指令处设置断点（*dot_product 是这条指令的地址），运行程序，以十六进制打印 %rsp 所指的 8 个字节（x/gx）；tail -1 只保留最后一行。
最后一行冒号之前是栈顶的地址，随运行环境变化；冒号之后的返回地址不变。换用其他版本的编译器，三个地址会变化，call 的下一条指令的地址与栈顶的 8 个字节仍然相同。
""")


def stack_frames(p):
    p.title('栈帧：一次调用在栈上占用的区域')
    slide(p, r"""
**栈帧（Stack Frame）**：一次调用在栈上占用的一段连续区域，存放这次调用自己的数据。
- `call` 压入的返回地址是调用者栈帧的最后一项，被调用者的栈帧从它之下开始；
- 调用开始时在栈顶建立栈帧，调用返回时释放；`%rsp` 指向最后建立的栈帧的末端，这个栈帧属于正在执行的函数；
- 调用按后进先出的顺序返回，栈帧的建立与释放也按后进先出的顺序进行。
""")
    figure(p, "frame-lifo", 1120)
    slide(p, r"""
**栈帧中的其他数据**：除返回地址外，栈帧还存放经栈传递的参数、保存的寄存器值与局部变量，由后面三个问题的解决方式决定。
""")
    p.notes("""
图中 main 调用 f，f 先后调用 g 与 h。每一列是一个时刻的栈，栈底在上，最下面的栈帧属于正在执行的函数。
g 返回后它的栈帧释放，f 再调用 h 时，h 的栈帧使用同一段内存。
""")


def call_checklist_2(p):
    call_checklist_table(p, 2)
    slide(p, r"""
**问题**：`main` 把 `w`、`x`、`4` 放在哪里，`dot_product` 才能读到？`dot_product` 的结果又放在哪里交给 `main`？
""")
    p.notes("""
前两个问题由 call 与 ret 两条指令解决，返回地址保存在运行时栈中。
""")


def param_regs(p):
    p.title('传递数据：前 6 个参数与返回值使用寄存器')
    slide(p, r"""
**调用双方的约定**：调用者在 `call` 之前把数据写入约定的位置，被调用者从同一位置读出。

**参数**：整数与指针参数的前 6 个依次使用下表的寄存器，按参数的宽度使用相应的部分：
""")
    p.table([
        ['64 位', '`%rdi`', '`%rsi`', '`%rdx`', '`%rcx`', '`%r8`', '`%r9`'],
        ['32 位', '`%edi`', '`%esi`', '`%edx`', '`%ecx`', '`%r8d`', '`%r9d`'],
        ['16 位', '`%di`', '`%si`', '`%dx`', '`%cx`', '`%r8w`', '`%r9w`'],
        ['8 位', '`%dil`', '`%sil`', '`%dl`', '`%cl`', '`%r8b`', '`%r9b`'],
    ], headers=['参数宽度', '参数 1', '参数 2', '参数 3', '参数 4', '参数 5', '参数 6'])
    slide(p, r"""
**返回值**：放入 `%rax`，按返回值的宽度使用 `%rax`、`%eax`、`%ax` 或 `%al`。

**约定的出处**：System V AMD64 ABI。应用二进制接口（ABI）规定分别编译的模块之间如何配合，其中关于函数调用的规定称为调用规约。
""")
    p.notes("""
参数与返回值的位置是调用双方共同遵守的约定，CPU 不检查。约定优先使用寄存器：读写寄存器不需要访问内存。
System V AMD64 ABI 是 x86-64 上的 Linux 使用的 ABI，ABI 是 Application Binary Interface 的缩写。
浮点参数使用另一组寄存器 %xmm0 到 %xmm7，浮点返回值使用 %xmm0；本讲的例子都是整数与指针。
32 位 x86 的约定把全部参数放在栈中。x86-64 有 16 个通用寄存器，改为用寄存器传递前 6 个参数，减少每次调用的访存。
""")


def dot_params(p):
    p.title('实例：dot_product 的参数与返回值')
    figure(p, "param-binding", 760)
    p.demo('编译 main.c，查看 call 之前的指令',
           """cd examples && gcc -Og -fcf-protection=none -fno-stack-protector -S main.c -o - \\
    | sed -f asm.sed | sed -n '/movq/,/call/p'""",
           output="""	movq	%rsp, %rsi
	leaq	16(%rsp), %rdi
	movl	$4, %edx
	call	dot_product@PLT""",
           files=['examples/main.c', 'examples/dot.c', 'examples/asm.sed'])
    slide(p, r"""
- **调用者 `main`**：`call` 之前把 `w` 的地址写入 `%rdi`，`x` 的地址写入 `%rsi`，`4` 写入 `%edx`；
- **被调用者 `dot_product`**：从这三个寄存器读取参数，返回前把 `sum` 放入 `%eax`；
- 数组 `w` 的地址是 `%rsp` 加 16，`x` 的地址是 `%rsp`，在「局部变量」一节说明。
""")
    p.notes("""
图是 dot_product 的原型与约定的对应：两个指针参数用 64 位的 %rdi 与 %rsi，int 参数 n 用 32 位的 %edx，int 返回值用 %eax。
sed -n '/movq/,/call/p' 只打印 main 的清单中从 movq 到 call 的四行。@PLT 是链接用的记号，第五讲说明；call dot_product@PLT 调用的就是 dot_product。
被调用者一侧见第二部分 dot_product 的清单，返回前的 movl %r9d, %eax 把 sum 放入 %eax；dot.c 按钮打开它的源码。main 把 dot_product 的返回值作为自己的返回值，call 之后没有改写 %eax。
""")


def stack_args(p):
    p.title('超过 6 个参数：第 7 个起经栈传递')
    p.side_image("assets/stack-args.svg", width="40%", alt="contain", side="left")
    slide(p, r"""
**第 7 个及以后的参数由调用者压入栈**：
- 压栈在 `call` 之前进行，顺序从右向左：最后一个参数最先压入，第 7 个参数最后压入；
- 每个参数占 8 字节，宽度不足 8 字节的参数也占 8 字节；
- `call` 再压入返回地址。进入被调用者时，`(%rsp)` 是返回地址，`8(%rsp)` 是参数 7，`16(%rsp)` 是参数 8，依此类推；
- 调用返回后，调用者增大 `%rsp`，释放这些参数。

**例子**：`use8` 调用 `last2(1, 2, 3, 4, 5, 6, 7, 8)`，`last2` 返回第 7 个参数减第 8 个参数的差。左图是进入 `last2` 时的栈。
""")
    p.notes("""
从右向左压栈使第 7 个参数离栈顶最近，被调用者按固定的偏移读取：参数 7 总在 8(%rsp)，与参数的总数无关。
左图各行左侧是 last2 访问该位置的写法，右侧是 use8 中写入该位置的指令。三个位置都属于 use8 的栈帧。
""")


def stack_args_2(p):
    p.title('超过 6 个参数：第 7 个起经栈传递')
    p.side_image("assets/stack-args.svg", width="40%", alt="contain", side="left")
    p.demo('编译 args8.c',
           """cd examples && gcc -Og -fcf-protection=none \\
    -S args8.c -o - | sed -f asm.sed""",
           output="""last2:
	movq	8(%rsp), %rax
	subq	16(%rsp), %rax
	ret
use8:
	pushq	$8
	pushq	$7
	movl	$6, %r9d
	movl	$5, %r8d
	movl	$4, %ecx
	movl	$3, %edx
	movl	$2, %esi
	movl	$1, %edi
	call	last2
	addq	$16, %rsp
	ret""",
           bold=[2, 3, 6, 7, 15],
           files=['examples/args8.c', 'examples/asm.sed'])
    p.notes("""
加粗的五行是经栈传递的部分：use8 先压入 8，再压入 7；last2 从 8(%rsp) 读出参数 7，从 16(%rsp) 读出参数 8；返回后 addq $16, %rsp 释放两个参数。
其余六条 movl 把前 6 个参数放入寄存器。last2 没有改写 %rsp，它的两个参数始终在 8(%rsp) 与 16(%rsp)。
""")


def call_checklist_3(p):
    call_checklist_table(p, 3)
    slide(p, r"""
**问题**：调用者放在寄存器中的值，在被调用者执行之后是否还在？
""")
    p.notes("""
数据的传递由约定解决：前 6 个参数与返回值使用寄存器，其余参数经栈传递。
""")


def reg_conflict(p):
    p.title('寄存器：调用者的值可能被被调用者改写')
    p.code('c', """int dot_bias(const int *w, const int *x, int n, int b) {
    return dot_product(w, x, n) + b;
}""")
    slide(p, r"""
**进入 `dot_bias` 时**，`w`、`x`、`n`、`b` 依次在 `%rdi`、`%rsi`、`%edx`、`%ecx` 中。前三个正是 `dot_product` 的参数，可以直接 `call`；返回后把 `%ecx` 中的 `b` 加到 `%eax` 上：
""")
    p.code('text', """dot_bias:                              dot_product:    # two instructions of the loop body
    call   dot_product                     movl   (%rsi,%r8,4), %ecx    # x[i]
    addl   %ecx, %eax     # b ?            imull  (%rdi,%r8,4), %ecx
    ret""")
    slide(p, r"""
- **结果错误**：`dot_product` 的循环体把 `x[i]` 与乘积写入 `%ecx`，`b` 被覆盖；
- **原因**：寄存器只有一组，所有函数共用；两个函数分别编译，编译一方时看不到另一方使用哪些寄存器。

**需要一项约定**：规定每个寄存器的值在一次调用前后是否保持不变。
""")
    p.notes("""
左边是不保存 b 的写法，右边是第二部分 dot_product 清单中循环体的两条指令。
这一页的 dot_bias 只有三条指令，是为说明问题写的；编译器的输出在本节最后一页。
""")


def saved_regs(p):
    p.title('寄存器使用惯例：调用者保存与被调用者保存')
    slide(p, r"""
**被调用者保存寄存器（Callee-saved）**：`%rbx`、`%rbp`、`%r12`、`%r13`、`%r14`、`%r15`
- 约定：函数返回时，这些寄存器的值与函数被调用时相同；
- 被调用者要使用其中一个时，先把原值保存到栈上，返回前恢复。

**调用者保存寄存器（Caller-saved）**：`%rax`、6 个参数寄存器、`%r10`、`%r11`
- 约定：被调用者可以直接改写这些寄存器；
- 调用者在调用之后还要使用其中的值时，在 `call` 之前把它保存到栈上，或者移到一个被调用者保存寄存器中。

**栈顶指针 `%rsp`**：函数返回后，`%rsp` 的值与执行 `call` 之前相同。
""")
    p.notes("""
这项约定与参数的约定同属 System V AMD64 ABI。两类寄存器的名称说明由谁负责保存其中的值。
6 个参数寄存器是 %rdi、%rsi、%rdx、%rcx、%r8、%r9，调用者保存寄存器共 9 个。
被调用者保存寄存器只在被调用者要使用时才保存；一个函数没有用到它们，就没有保存与恢复的指令。
""")


def saved_regs_fig(p):
    p.title('寄存器使用惯例：调用者保存与被调用者保存')
    figure(p, "saved-regs", 1120)
    slide(p, r"""
**`dot_bias` 的 `b` 在调用者保存的 `%ecx` 中，调用之后还要使用，有两种做法**：
- 调用之前把 `b` 压入栈，调用之后弹出；
- 调用之前把 `b` 移到一个被调用者保存寄存器中，按约定 `dot_product` 返回时它的值不变。`dot_bias` 自己也是被调用者，使用这个寄存器之前要先保存它的原值。
""")
    p.notes("""
16 个通用寄存器分为三组：9 个调用者保存，6 个被调用者保存，加上栈顶指针 %rsp。
两种做法都要访问一次栈：第一种保存的是 b，第二种保存的是被调用者保存寄存器的原值。
""")


def callee_example(p):
    p.title('实例：dot_bias 中的 %rbx')
    p.side_image("assets/bias-stack.svg", width="42%", alt="contain", side="left")
    p.demo('编译 dot_bias.c',
           """cd examples && gcc -Og -fcf-protection=none \\
    -S dot_bias.c -o - | sed -f asm.sed""",
           output="""dot_bias:
	pushq	%rbx
	movl	%ecx, %ebx
	call	dot_product@PLT
	addl	%ebx, %eax
	popq	%rbx
	ret""",
           files=['examples/dot_bias.c', 'examples/dot.c', 'examples/asm.sed'])
    slide(p, r"""
- **`movl %ecx, %ebx`**：`b` 移到被调用者保存的 `%ebx`，`dot_product` 返回后它的值不变；
- **`pushq %rbx` 与 `popq %rbx`**：`dot_bias` 改写 `%rbx` 之前保存原值，返回前恢复；
- **`dot_product`** 只改写调用者保存寄存器，没有保存与恢复的指令。
""")
    p.notes("""
左图是 dot_product 执行期间的栈：dot_bias 的栈帧有两项，保存的 %rbx 与 call 压入的返回地址，共 16 字节。
gcc 选用上一页的第二种做法。b 放在寄存器中，调用前后都不需要访存来读写 b；入口与出口各多一条 pushq 与 popq。
dot_product 改写的寄存器可以在第二部分的清单中核对：%eax、%ecx、%r8、%r9d，都是调用者保存寄存器。
examples/asm.sed 删去 gcc -S 输出中的汇编伪指令（.file、.cfi_* 等）与 .LFB/.LFE 标号，只留下指令与跳转标号。
""")


def call_checklist_4(p):
    call_checklist_table(p, 4)
    slide(p, r"""
**问题**：`main` 的数组 `w` 与 `x` 存放在哪里？
""")
    p.notes("""
寄存器的问题由使用惯例解决：6 个被调用者保存寄存器的值在调用前后相同，其余的由调用者在需要时保存。
""")


def local_vars(p):
    p.title('局部变量：放在寄存器中，或在栈帧中分配')
    slide(p, r"""
**能放在寄存器中的局部变量不占用内存**：`dot_product` 的 `sum` 在 `%r9d` 中，`i` 在 `%eax` 中，函数没有在栈上分配空间。

**局部变量必须放在内存中的三种情况**：
1. 寄存器不够用：同时使用的局部变量多于可用的寄存器；
2. 局部变量是数组或结构体：元素按地址访问，`main` 的 `w[4]` 与 `x[4]` 属于这种情况；
3. 程序对局部变量取地址（`&` 运算符）：寄存器没有地址。

**在栈帧中分配**：
- 函数开头把 `%rsp` 减去所需的字节数，这段空间成为函数栈帧的一部分；
- 局部变量按相对 `%rsp` 的偏移访问；
- 返回前把 `%rsp` 加上同样的字节数，这段空间随之释放；
- 每次调用各自分配，同一个函数的多次调用（例如递归）各有一份局部变量。
""")
    p.notes("""
全局变量分配在数据段的固定地址，整个程序运行期间只有一份；局部变量随调用分配，随返回释放，因此放在栈中。
main 把 w 与 x 的地址作为参数传给 dot_product，两个数组必须有地址。
""")


def stack_frame(p):
    p.title('实例：main 的栈帧')
    p.side_image("assets/main-stack.svg", width="42%", alt="contain", side="left")
    p.demo('用 gcc -Og 编译 main.c',
           """cd examples && gcc -Og -fcf-protection=none \\
    -fno-stack-protector -S main.c -o - \\
    | sed -f asm.sed""",
           output="""main:
	subq	$40, %rsp
	movl	$1, 16(%rsp)
	movl	$2, 20(%rsp)
	movl	$3, 24(%rsp)
	movl	$4, 28(%rsp)
	movl	$5, (%rsp)
	movl	$6, 4(%rsp)
	movl	$7, 8(%rsp)
	movl	$8, 12(%rsp)
	movq	%rsp, %rsi
	leaq	16(%rsp), %rdi
	movl	$4, %edx
	call	dot_product@PLT
	addq	$40, %rsp
	ret""",
           bold=[2, 15],
           files=['examples/main.c', 'examples/asm.sed'])
    p.notes("""
清单分四段：第 2 行分配栈帧，第 3 到 10 行写入两个数组的 8 个元素，第 11 到 14 行准备参数并调用 dot_product，第 15 行释放栈帧。
-fno-stack-protector 关闭栈保护，栈帧中只有局部数组；默认编译时的栈保护在本部分「缓冲区溢出与栈保护」一节说明。
main.c 按钮在右侧打开源码，与左侧的栈帧对照。
""")


def stack_frame_2(p):
    p.title('实例：main 的栈帧')
    p.side_image("assets/main-stack.svg", width="42%", alt="contain", side="left")
    slide(p, r"""
**局部数组的分配、使用与释放**：
1. **分配**：`subq $40, %rsp` 把 `%rsp` 减 40，从 `%rsp` 开始的 40 字节成为 `main` 的栈帧；
2. **使用**：8 条 `movl` 按相对 `%rsp` 的偏移写入数组元素，`x[i]` 位于偏移 $4i$，`w[i]` 位于偏移 $16 + 4i$；数组的地址 `(%rsp)` 与 `16(%rsp)` 作为参数传给 `dot_product`；
3. **释放**：`addq $40, %rsp` 把 `%rsp` 加回 40，栈帧随之释放，其中的数据不需要清除。
""")
    p.aside("偏移 32 ~ 39 未被使用：调用规约要求执行 `call` 时 `%rsp` 是 16 的倍数。进入 `main` 时栈顶是 8 字节的返回地址，40 + 8 = 48 是 16 的倍数。")
    p.notes("""
偏移在编译时确定，每次调用的 %rsp 不同，同一段指令因此访问本次调用的局部变量。
main 的栈帧中没有保存的寄存器：它没有使用被调用者保存寄存器，调用 dot_product 之后也不再使用调用者保存寄存器中的值。
「超过 6 个参数」一页的 use8 执行 call 时 %rsp 是 16 的倍数加 8：last2 与 use8 在同一个文件中，gcc 看到 last2 不依赖栈的对齐，省去了调整 %rsp 的指令（选项 -fipa-stack-alignment，默认开启）。被调用者在另一个文件中时，gcc 按规约对齐。
""")


def call_checklist_5(p):
    call_checklist_table(p, 5)
    slide(p, r"""
**五个问题的解决方式分为三类**：
- **两条指令**：`call` 与 `ret`，行为由 ISA 规定；
- **两项约定**：数据的传递与寄存器的使用，由编译器在生成指令时遵守；
- **运行时栈**：返回地址、经栈传递的参数、保存的寄存器值与局部变量都存放在栈帧中。
""")
    p.notes("""
五个问题都已解决。接下来把它们按一次调用的时间顺序排列。
""")


def call_sequence(p):
    p.title('综合起来：一次调用的完整步骤')
    p.side_image("assets/frame-layout.svg", width="34%", alt="contain")
    slide(p, r"""
**调用者，在调用之前**：
1. 保存调用后还要用的调用者保存寄存器；
2. 第 7 个起的参数从右向左压入栈；
3. 前 6 个参数放入寄存器；
4. `call`：压入返回地址，跳到被调用者。

**被调用者**：

5. `pushq`：保存要用的被调用者保存寄存器；
6. `subq`：减小 `%rsp`，分配局部变量；
7. 执行函数体，把返回值放入 `%rax`；
8. `addq`：增大 `%rsp`，释放局部变量；
9. `popq`：恢复被调用者保存寄存器；
10. `ret`：弹出返回地址，回到调用者。
""")
    p.notes("""
右图是这次调用在栈上存放的数据，圆圈中的数字是写入该区域的步骤：第 2 步压入参数，第 4 步压入返回地址，第 5 步保存寄存器，第 6 步分配局部变量。第 8、9、10 步按相反的顺序释放它们。
返回之后，调用者增大 %rsp 释放第 2 步压入的参数，并恢复第 1 步保存的值。
不需要的步骤可以省去。dot_product 只有第 7 步与第 10 步。dot_bias 没有第 6 步与第 8 步；它的第 1 步是把 b 移到 %ebx，三个参数已经在 dot_product 需要的寄存器中，第 3 步没有指令。main 没有第 5 步与第 9 步。
""")


def abi_isa(p):
    p.title('指令与约定：ISA 规定的行为，ABI 规定的用法')
    slide(p, r"""
**ISA 规定指令的行为**：`call` 压入返回地址并改写 `%rip`，`ret` 弹出返回地址写入 `%rip`。这些动作由 CPU 执行指令时完成，编译器不能改变。

**ABI 规定寄存器与栈的用法**：参数与返回值的位置，寄存器由哪一方保存，执行 `call` 时 `%rsp` 是 16 的倍数。这些规定由编译器生成指令时遵守，CPU 执行时不检查。

**同一种 CPU 上的两套约定**：Linux 与 Windows 使用相同的指令，调用规约不同：
""")
    p.table([
        ['整数与指针参数', '`%rdi` `%rsi` `%rdx` `%rcx` `%r8` `%r9`', '`%rcx` `%rdx` `%r8` `%r9`'],
        ['`%rsi` 与 `%rdi`', '调用者保存', '被调用者保存'],
        ['调用者预留的栈空间', '无', '32 字节'],
    ], headers=['规定', 'Linux（System V AMD64 ABI）', 'Windows（Microsoft x64）'],
        widths=[3, 5, 4])
    slide(p, r"""
按一套约定编译的函数，不能直接调用按另一套约定编译的函数。
""")
    p.notes("""
指令的行为写在 ISA 手册中，由 CPU 执行；寄存器与栈的用法写在 ABI 文档中，由编译器遵守。pushq 与 popq 以 %rsp 为栈顶指针，也是 ISA 规定的行为。
GCC 在 x86-64 上提供 __attribute__((ms_abi))，同一个 Linux 程序中的函数可以按 Windows 的约定编译与调用，说明约定由编译器选择。
Windows 的 32 字节称为影子空间（shadow space），调用者在 call 之前预留，被调用者可以把 4 个寄存器参数存入其中。
""")


def buffer_overflow(p):
    p.title('缓冲区溢出：越界写入覆盖栈帧中的相邻数据')
    p.code('c', """static void fill(int *x, int argc, char **argv) {
    for (int i = 1; i < argc; i++) x[i - 1] = atoi(argv[i]);  /* no bound */
}
int main(int argc, char **argv) {
    int w[4] = {1, 2, 3, 4}, x[4];
    fill(x, argc, argv);
    return dot_product(w, x, 4);    /* result = exit status */
}""")
    p.demo('给出不同个数的实参',
           """cd examples
gcc -Og -fcf-protection=none -fno-stack-protector -o overflow overflow.c dot.c
run() { ./overflow "$@" 2>&1 | cat; echo "  $# values -> exit ${PIPESTATUS[0]}"; }
run 5 6 7 8
run 5 6 7 8 1 1 1 1
run 5 6 7 8 1 1 1 1 1 1 1 1""",
           output="""  4 values -> exit 70
  8 values -> exit 26
  12 values -> exit 139""",
           files=['examples/overflow.c', 'examples/dot.c'])
    p.notes("""
fill 把命令行上的每个数写入 x 的一个元素，写入的个数由 argc 决定，没有与 x 的 4 个元素比较。
程序把点积作为退出状态返回，因此 $? 直接显示结果是否被改变；4 个实参时 1*5 + 2*6 + 3*7 + 4*8 = 70。
2>&1 | cat 使崩溃时 shell 的作业提示不进入输出，退出码由 PIPESTATUS[0] 取得。
""")


def buffer_overflow_fig(p):
    p.title('缓冲区溢出：越界写入覆盖栈帧中的相邻数据')
    figure(p, "overflow", 1120)
    slide(p, r"""
- **8 个实参**：多出的 4 个覆盖相邻的局部数组 `w`，结果由 70 变为 26，程序照常返回，不报任何错误；
- **12 个实参**：写入到达返回地址，`ret` 跳向被改写的地址，进程收到 `SIGSEGV`（退出码 139）。
""")
    p.notes("""
清单由 cd examples; gcc -Og -fcf-protection=none -fno-stack-protector -S overflow.c -o - | sed -f asm.sed 得到：
main 的 pushq %rbx 与 subq $32, %rsp 给出图中的偏移，x 由 movq %rsp, %rdi 传给 fill，w 写在 16(%rsp) 起的 16 字节。
8 个实参时 w 变为 {1, 1, 1, 1}，点积即 5 + 6 + 7 + 8 = 26。
""")


def canary(p):
    p.title('栈保护机制：金丝雀值')
    slide(p, r"""
**默认编译即开启**：前面的溢出实验使用了 `-fno-stack-protector`；去掉这个选项重新编译，同一份源程序的溢出在返回之前就被发现：
""")
    p.demo('开启栈保护后重新编译',
           """cd examples
gcc -Og -fcf-protection=none -o overflow_sp overflow.c dot.c
./overflow_sp 5 6 7 8 1 1 1 1 1 1 1 1 2>&1 | cat""",
           output="""*** stack smashing detected ***: terminated""",
           files=['examples/overflow.c'])
    slide(p, r"""
**金丝雀值（Stack Canary）的防护机理**：
1. **入口写入**：`movq %fs:40, %rax` 从线程局部存储读取每次运行都不同的随机值，`movq %rax, 40(%rsp)` 把它存入局部数组与返回地址之间；
2. **返回前检验**：`movq 40(%rsp), %rdx` 与 `subq %fs:40, %rdx` 比较该值是否仍然相同；
3. **不相等则终止**：`jne` 成立时执行 `call __stack_chk_fail@PLT`，进程被终止，退出码 134。
""")
    p.notes("""
开启栈保护后编译器把数组放在栈帧的低地址一侧，金丝雀值位于 40(%rsp)，返回地址位于 56(%rsp)，
因此越界写入先改写金丝雀值，才能到达返回地址。金丝雀值每次运行都不同，越过它而不被发现是困难的。
""")


def canary_fig(p):
    p.title('栈保护机制：金丝雀值')
    figure(p, "canary", 1120)


def recap_part3(p):
    p.title('小结：函数调用用到的指令与约定')
    p.table([
        ['调用与返回',
         '`call` 压入返回地址并跳转，`ret` 弹出返回地址并跳转',
         '`main` 中的 `call dot_product`'],
        ['传递数据',
         '前 6 个参数用寄存器，其余经栈传递；返回值用 `%rax`',
         '`w`、`x`、`n` 在 `%rdi`、`%rsi`、`%edx` 中'],
        ['寄存器',
         '6 个由被调用者保存，9 个由调用者保存',
         '`dot_bias` 的 `pushq %rbx` 与 `popq %rbx`'],
        ['局部变量',
         '减小 `%rsp` 分配，按偏移访问，增大 `%rsp` 释放',
         '`main` 的 `subq $40, %rsp`'],
        ['缓冲区溢出',
         '越界写入改写栈帧中相邻的数据；金丝雀值在返回前检验',
         '`overflow.c` 的 8 个与 12 个实参'],
    ], headers=['问题', '指令与约定', '本部分的例子'], widths=[2, 6, 5])
    slide(p, r"""
**运行时栈**：返回地址、经栈传递的参数、保存的寄存器值与局部变量都在栈帧中，`%rsp` 指向栈顶；栈帧在调用开始时建立，在调用返回时释放。
""")
    p.notes("""
第三部分的小结。前四行是五个问题的解决方式，第五行是栈帧布局带来的安全问题。
call 与 ret 的行为由 ISA 规定，参数、返回值与寄存器的用法由 ABI 约定。被调用者保存的 6 个寄存器是 %rbx、%rbp、%r12 到 %r15。
""")


def reg_exercise(p):
    p.title('练习：dot_sum 中的寄存器')
    p.code('c', """int dot_sum(const int *w, const int *x, const int *y, int n) {
    return dot_product(w, x, n) + dot_product(w, y, n);
}""")
    slide(p, r"""
`dot_sum` 先后两次调用 `dot_product`，把两个结果相加。

**按参数的约定与寄存器使用惯例回答**：
1. 进入 `dot_sum` 时，`w`、`x`、`y`、`n` 分别在哪个寄存器中？
2. 第一次调用之前，哪些寄存器要写入新的值？哪个参数因此被覆盖？
3. 哪些值在一次调用返回之后还要使用？调用期间它们应当放在哪一类寄存器中？
4. `dot_sum` 的开头要用 `pushq` 保存几个寄存器？
""")
    p.notes("""
课堂练习。先按约定分析，再与编译器的输出核对。
dot_sum 与 dot_bias 的区别：调用之后还要使用的值有 4 个，其中一个是第一次调用的返回值。
""")


def reg_exercise_2(p):
    p.title('练习：dot_sum 中的寄存器')
    p.demo('编译 dot_sum.c，查看第一次调用之前的指令',
           """cd examples
gcc -Og -fcf-protection=none -S dot_sum.c -o - | sed -f asm.sed | sed -n '2,10p'""",
           output="""	pushq	%r15
	pushq	%r14
	pushq	%rbp
	pushq	%rbx
	subq	$8, %rsp
	movq	%rdi, %rbx
	movq	%rdx, %r15
	movl	%ecx, %ebp
	movl	%ecx, %edx""",
           files=['examples/dot_sum.c', 'examples/asm.sed'])
    slide(p, r"""
1. `w`、`x`、`y`、`n` 依次在 `%rdi`、`%rsi`、`%rdx`、`%ecx` 中；
2. 只有 `%edx` 要写入 `n`，原来在 `%rdx` 中的 `y` 被覆盖；
3. `w`、`y`、`n` 与第一次调用的结果，放在被调用者保存的 `%rbx`、`%r15`、`%ebp`、`%r14d` 中；
4. 4 个。`subq $8, %rsp` 使执行 `call` 时 `%rsp` 是 16 的倍数。
""")
    p.notes("""
sed -n '2,10p' 打印清单的第 2 到 10 行，即入口到第一次 call 之前；之后是 call、movl %eax, %r14d、第二次调用的三条参数指令与 call、addl、addq $8, %rsp、4 条 popq 与 ret。
x 只用于第一次调用，调用之后不再使用，不需要保存。第一次调用的结果由 movl %eax, %r14d 保存，它要在第二次调用之后使用。
对齐的计算：进入 dot_sum 时栈顶是 8 字节的返回地址，4 条 pushq 共 32 字节，再减 8，合计 48 字节，是 16 的倍数。
""")


def rec_exercise(p):
    p.title('练习：递归调用的栈帧')
    p.code('c', """int dot_product_rec(const int *w, const int *x, int n) {
    if (n <= 0) return 0;
    return dot_product_rec(w, x, n - 1) + w[n - 1] * x[n - 1];
}""")
    slide(p, r"""
**`dot_product_rec` 用递归计算内积**，`main` 调用 `dot_product_rec(w, x, 4)`：
1. 从 `main` 的这次调用算起，`dot_product_rec` 一共被调用几次？
2. `n` 为 0 的那次调用执行时，这几次调用的返回地址有几个在栈上？它们各自回到哪个函数？
3. 递归调用返回之后还要计算 `w[n - 1] * x[n - 1]`。递归调用期间，`w`、`x`、`n` 应当放在哪一类寄存器中？每次调用因此要在栈上保存什么？
4. 实现递归是否需要 `call`、`ret`、寄存器使用惯例与栈帧之外的机制？
""")
    p.demo('编译 dot_rec.c',
           """cd examples && gcc -Og -fcf-protection=none -fno-stack-protector -S dot_rec.c -o - | sed -f asm.sed""",
           files=['examples/dot_rec.c', 'examples/asm.sed'])
    p.notes("""
课堂练习。递归是同一个函数的多次调用同时处于活动状态，每次调用的返回地址与保存的寄存器值各有一份。
演示的输出是 dot_product_rec 与 main 的清单，用来核对第 3 题：入口的 testl 与 jle 处理 n <= 0，之后是 3 条 pushq、3 条保存参数的 mov、leal -1(%rdx), %edx 与递归的 call。
""")


def rec_exercise_2(p):
    p.title('练习：递归调用的栈帧')
    figure(p, "rec-stack", 880)
    slide(p, r"""
1. 5 次，`n` 依次为 4、3、2、1、0；
2. 5 个：最早压入的回到 `main`，其余 4 个回到 `dot_product_rec`；
3. 被调用者保存寄存器；每次调用用 3 条 `pushq` 保存 `%r15`、`%r14`、`%rbx` 的原值；
4. 不需要：每次调用有自己的返回地址与保存的寄存器值。
""")
    p.notes("""
图的每一行是一次调用压入栈的内容：调用它的 call 压入的返回地址，以及它自己用 pushq 保存的三个寄存器，每一层递归在栈上增加 32 字节。n 为 0 的调用在 pushq 之前就返回，只有返回地址。
返回时每一层先用 3 条 popq 恢复寄存器，再用 ret 回到上一层，%eax 中是已经算出的部分和。
""")


def flops_estimate(p):
    p.title('回顾：生成一个 Token 的乘加次数与读取字节数')
    slide(p, r"""
**以第二讲实验 `nano-quant` 量化的模型为例**：
- Qwen3-VL-2B-Instruct 的语言模型有 28 层，17.2 亿个权重；
- 权重采用第二讲的 Q4_0 格式，每 32 个权重占 18 字节，平均每个权重 4.5 位；
- 生成一个 Token 时，输入向量与每个权重矩阵各做一次矩阵向量乘 $y = Wx$，$y$ 的每个元素是 $W$ 的一行与 $x$ 的内积；
- 每个权重参与 **1 次乘加**，并从内存读取 **1 次**。
""")
    p.table([
        ['乘加次数', '$1.72 \\times 10^9$ 次'],
        ['从内存读取的权重', '$1.72 \\times 10^9 \\times 4.5\\text{ bit} \\approx 0.968\\text{ GB}$'],
    ], headers=['每生成 1 个 Token', '数量'])
    slide(p, r"""
**第二讲的结论**：单请求自回归推理处于访存受限区，生成速度的上限等于内存带宽除以每个 Token 读取的权重字节数。
""")
    p.notes("""
两个数量与第二讲的带宽估算口径相同：只计权重，KV cache 与激活的读取未计入。
权重数 1 720 574 976 与 Q4_0 的字节数 968 249 344 取自 nano-quant 实验：前者是 nano-quant plan 的汇总，后者是 tests/expected-qwen3vl.txt 中的 q4_0_nq_bytes。
这个模型的词表矩阵 embed_tokens 同时充当输出层，生成一个 Token 时参与一次矩阵向量乘。一维的归一化系数保持 F32，共 0.5 MB。
""")


def hw_peak(p):
    p.title('硬件上限：i9-11900H 的峰值算力、内存带宽与平衡点')
    p.table([
        ['峰值算力', '$P$', '320 GFLOPS', 'Intel 出口合规指标文档（APP Metrics）'],
        ['内存带宽', '$B$', '51.2 GB/s', 'Intel 产品规格页，双通道 DDR4-3200'],
    ], headers=['硬件上限', '符号', '公开数值', '出处'], widths=[16, 10, 20, 54])
    slide(p, r"""
- **平衡点**：$P \div B = 6.25$ FLOP/Byte，算术强度 $I$ 低于它时速率上限是 $B \times I$；
- Qwen3-VL-2B（Q4_0）：$I \approx 3.6$ FLOP/Byte，位于访存受限区，上限 53 Token/s。
""")
    figure(p, "roofline", 1120)
    p.notes("""
320 GFLOPS 取自 Intel 的 APP Metrics for Intel Microprocessors（Intel Core Processors，Revision 8，2026 年 1 月 6 日），这份文档为出口合规列出每个型号的 GFLOPS。
51.2 GB/s 取自 Intel 产品规格页中 i9-11900H 的 Max Memory Bandwidth 一项，内存规格为双通道、最高 3200 MT/s。
算术强度 I 是计算量与访存量之比（第二讲），1 次乘加计 2 次运算。I 低于平衡点的程序处于访存受限区，速率上限是 B × I；高于平衡点的程序处于算力受限区，上限是 P。
这份文档的 GFLOPS 按 64 位浮点运算计数。本讲沿用第二讲的约定，各种数据类型的乘加都按 1 次乘加 2 次运算计数。
平衡点是第二讲的名称，第二讲中 RTX 5090 的平衡点约为 117 FLOP/Byte。
图中的空心圆是 Qwen3-VL-2B 语言模型的带宽上限：算术强度 3.44 ÷ 0.968 ≈ 3.55 FLOP/Byte，51.2 GB/s × 3.55 FLOP/Byte ≈ 182 GFLOPS，即 51.2 ÷ 0.968 ≈ 53 Token/s。
""")


def measured_speed(p):
    p.title('实测：ollama 与 mini-ollama 各用 1 个线程的生成速度')
    p.demo('mini-ollama，1 个线程', 'cd examples/mini-ollama && make run',
           files=['examples/mini-ollama/mini_ollama.c'])
    p.demo('ollama，1 个线程，只使用 CPU', 'cd examples/mini-ollama && make run-ollama')
    slide(p, r"""
- **mini-ollama**：读取同一个模型文件的 C 程序，每个权重单独还原、相乘、累加；
- 各用 1 个线程：ollama 的运算速率接近单核算力上限，mini-ollama 是这个上限的 1/15。
""")
    figure(p, "roofline-measured", 1120)
    p.notes("""
mini-ollama 的源码在 examples/mini-ollama/ 下，共约 850 行 C：mini_ollama.c 是主循环与一层之内的运算，model.c 把 GGUF 文件载入内存（Linux 上用 mmap 映射，Windows 上用 fread 读入）并找到其中的张量，tokenizer.c 是分词器。它读取第二讲实验 nano-quant 按 q4_0 配方得到的 q4_0.gguf（974 196 320 字节），逐个 Token 生成回答。
主循环每一轮让一个 Token 经过 28 层，每层 attention 做 4 次矩阵向量乘，feed_forward 做 3 次，输出层 1 次，生成一个 Token 共 197 次矩阵向量乘、1 720 451 072 次乘加，与 1.72 × 10⁹ 一致。这些乘加都在函数 matvec 中完成，它占运行时间的 99.7%。
make run 编译并运行 mini-ollama，一次约 40 秒。输出的 eval rate 是生成回答的速度，名字与 ollama run --verbose 的输出相同；matvec rate 是全部矩阵向量乘的运算速率。Makefile 使用 -O2 -fno-tree-vectorize：GCC 12 起在 -O2 下会把这个循环的一部分自动向量化，这个选项使它保持每次处理一个权重。
make run-ollama 先执行 ollama create nq-q4-0 -f Modelfile，再执行 ollama run nq-q4-0 --verbose，需要 ollama serve 已在运行。Modelfile 让 ollama 加载同一个 q4_0.gguf，num_gpu 0 使它只使用 CPU，num_thread 1 使它只用 1 个线程，temperature 0 使每一步取分数最高的 Token，与 mini-ollama 相同。
ollama 的回答是「我是一个虚拟助手，没有实体，但我可以用文字与你交流，为你提供帮助和解答。」前 9 个 Token 与 mini-ollama 相同；第 10 个 Token 上分数最高的两个候选相差 0.008，ollama 把激活量化为 8 位整数再计算，数值误差改变了这一步的选择。
图中的运算速率由生成速度乘以每个 Token 的 3.44 × 10⁹ FLOP 得到：10.7 Token/s 约 37 GFLOPS，0.77 Token/s 约 2.6 GFLOPS，两者相差 14 倍。
1 个线程在 1 个核心上运行。峰值算力 320 GFLOPS 是 8 个核心之和，1 个核心是 320 ÷ 8 = 40 GFLOPS，对应 11.6 Token/s，即图中的虚线。ollama 的 37 GFLOPS 是它的 92%，mini-ollama 的 2.6 GFLOPS 是它的 1/15。
带宽上限 53 Token/s 是整台机器的上限。单线程的 ollama 每秒读取 10.7 × 0.968 ≈ 10.4 GB 权重，是内存带宽 51.2 GB/s 的 1/5，它的生成速度由 1 个核心的运算速率决定。
40 GFLOPS 由按 64 位浮点运算计数的峰值算力除以核心数得到。8 位整数的向量指令每条处理的元素多于 64 位浮点指令，单个核心的实际运算速率可以高于 40 GFLOPS；92% 表示 ollama 与单核算力上限处于同一量级。
全部数值于 2026-10-05 在接通电源的 i9-11900H 上测得，gcc 15.2.0，ollama 0.33.2。多次运行中 ollama 在 10.4 ~ 10.9 Token/s 之间，mini-ollama 在 0.75 ~ 0.79 Token/s 之间。
""")


def system_limits(p):
    p.title('瓶颈定位：内存带宽、峰值算力与指令条数各自限制的位置')
    figure(p, "system-limits", 1120).footnote('照片从左到右来自 Wikimedia Commons 的 D-Kuru（CC BY-SA 4.0）、PantheraLeo1359531（CC BY 4.0）、Eric Gaba（CC BY-SA 4.0），经裁剪缩放。')
    p.notes("""
图的骨架是本讲开头「CPU从内存中获取指令与数据进行计算」一页的图，CPU 展开为寄存器、运算单元与指令执行三部分。
红色：内存带宽 51.2 GB/s 限制权重从内存到 CPU 的通路。每生成一个 Token，0.968 GB 权重经过这条通路一次，生成速度至多 53 Token/s。
蓝色：峰值算力 320 GFLOPS 是 8 个核心的运算单元全部同时运算、每条指令都使用寄存器全部 256 位时的速率。
绿色：单线程的 ollama 使用 1 个核心，它的一条指令使用寄存器的全部 256 位。1 个核心的算力上限是 320 ÷ 8 = 40 GFLOPS，实测约 37 GFLOPS。它计算 Q4_0 内积的函数 ggml_vec_dot_q4_0_q8_0 在本机使用的 libggml-cpu-icelake.so 中，主循环每轮 22 条指令处理 32 个权重，其中一条 vpdpbusd 完成 32 对 8 位整数的乘法并把乘积累加。
橙色：mini-ollama 使用 1 个核心，它的一条指令处理 1 个权重，只使用寄存器的低 32 位。matvec 的内层循环每轮 20 条指令处理 2 个权重。
指令执行：CPU 每个时钟周期能开始执行的指令条数有上限，完成一次乘加需要的指令越多，每秒完成的乘加越少。核心内部有多套运算部件，可以同时处理多条互不依赖的指令，由第四讲第六部分的「超标量」一页讲解。每次乘加的指令条数：mini-ollama 是 20 ÷ 2 = 10 条，ollama 是 22 ÷ 32 ≈ 0.69 条，相差 14.5 倍；实测的生成速度相差 14 倍。
256 位的寄存器与一条指令处理多个数据的向量指令，由后面「向量体系：SIMD 思想与 256 位 YMM 寄存器」起的几页讲解。
""")


def insn_mix(p):
    p.title('瓶颈分析：每次乘加执行的指令条数')
    slide(p, r"""
**按指令条数估算 mini-ollama 的速度上限**（`matvec` 的内层循环，取自 `objdump -d`）：
""")
    p.table([
        ['每次乘加的指令条数', '每轮循环 20 条指令，完成 2 次乘加', '$20 \\div 2 = 10$ 条'],
        ['每个周期至多完成的乘加', '每个周期最多有 5 条指令开始执行', '$5 \\div 10 = 0.5$ 次'],
        ['每秒至多完成的乘加', '运行时主频约 3.8 GHz', '$0.5 \\times 3.8 \\times 10^9 = 1.9 \\times 10^9$ 次'],
        ['生成速度的上限', '每个 Token $1.72 \\times 10^9$ 次乘加', '$1.9 \\div 1.72 \\approx 1.1$ Token/s'],
    ], headers=['计算步骤', '依据', '算式与结果'])
    slide(p, r"""
- **实测** 0.77 Token/s，是这个上限的 70%；
- **ollama** 每次乘加约 0.69 条指令，条数相差 14.5 倍，实测速度相差 14 倍。

**结论**：这个上限来自 CPU 的时钟：主频是每秒的周期数，每个周期最多有 5 条指令开始执行，每秒能执行的指令条数因此有上限，与指令的功能无关。两个程序的速度差距主要来自每次乘加的指令条数，减少条数可以提高运算速率。
""")
    p.notes("""
表中的循环取自 objdump -d mini-ollama 中 matvec 的内层循环，由 gcc -O2 -fno-tree-vectorize 生成，共 20 条指令、79 字节，每轮处理 1 字节权重，完成 2 次乘加，平均每次乘加 10 条。
完成乘法与累加的是 4 条：2 条 mulss 各从内存读取 1 个激活值并相乘，2 条 addss 累加。其余 16 条读取权重（1 条）、拆出两个 4 位存储值（6 条）、把它们转换为 float 并乘以缩放因子 d（6 条）、控制循环（3 条）。
核心每个周期最多有 5 条指令开始执行。一条指令从开始执行到得出结果需要 1 个或几个周期（mulss 需要 4 个周期），多条指令的执行过程相互重叠。核心内部有多套运算部件，可以同时处理多条互不依赖的指令，由第四讲第六部分的「超标量」一页讲解。
每个周期 5 条、每次乘加 10 条，每个周期至多完成 0.5 次乘加，即每轮循环至少 4 个周期。cmp 与 jne 在核心内部合并为 1 条，按 19 条计算每轮至少 3.8 个周期，页面按 20 条估算。
主频约 3.8 GHz 是运行 mini-ollama 时读取 /sys 下的 scaling_cur_freq 得到的。每秒 1.9 × 10⁹ 次乘加即 3.8 GFLOPS，除以每个 Token 的 1.72 × 10⁹ 次乘加得到 1.1 Token/s。
时钟周期是 CPU 内部电路同步工作的时间单位，主频 3.8 GHz 时一个周期约 0.26 纳秒。一个核心每秒至多执行 5 × 3.8 × 10⁹ = 1.9 × 10¹⁰ 条指令，这个数与指令的功能无关：拆出 4 位权重的 shr、and 与完成乘法的 mulss 各计 1 条。mini-ollama 每次乘加的 10 条指令中，8 条用于读取权重、还原为 float 权重与控制循环。
主频的提高受电路延迟与功耗限制，提高运算速率因此依靠减少每次乘加的指令条数与增加核心数。
这个估算只计指令条数，给出的是上限。指令之间的先后依赖（累加 sum 的 addss 需要 4 个周期，下一轮的累加在它完成后才能开始）、运算单元的数量、读取内存的延迟都会使实际速度低于它；cvtsi2ss 在这个微架构上分为 2 个微操作，实际计入的条数也多于 20。实测 0.77 Token/s（matvec rate 2.64 GFLOPS）是上限的 70%，各项原因所占的比例没有逐项测量。
ollama 的 ggml_vec_dot_q4_0_q8_0（icelake 版本）主循环每轮 22 条指令完成 32 次乘加，平均每次乘加 22 ÷ 32 ≈ 0.69 条。10 ÷ 0.69 ≈ 14.5，实测生成速度 10.7 ÷ 0.77 ≈ 14。
""")


def speedup_plan(p):
    p.title('解决方案：生成速度的四个因素与两个提速方案')
    slide(p, r"""
**估算速度上限的四个因素中有两个可以改变，对应接下来讲解的两个方案**：
""")
    p.table([
        ['每次乘加的指令条数', '每轮循环 20 条指令，完成 2 次乘加', '**↓ 可以降低**（方案 1）'],
        ['每个周期至多完成的乘加', '每个周期最多有 5 条指令开始执行', '**↑ 可以提高**，需要改变硬件（方案 2）'],
        ['每秒至多完成的乘加', '运行时主频约 3.8 GHz', '— 难以提高，受电路延迟与功耗限制'],
        ['生成速度的上限', '每个 Token $1.72 \\times 10^9$ 次乘加', '— 保持不变，由模型决定'],
    ], headers=['计算步骤', '依据', '提高速度的方向'])
    slide(p, r"""
1. **一条指令完成多次乘加**：每次乘加的指令条数随之降低，即 SIMD（单指令多数据）；
2. **提高每个周期完成的乘加次数**：需要更多的运算单元，即 GPU（CUDA）。
""")
    p.notes("""
表的前两列与「瓶颈分析：每次乘加执行的指令条数」一页相同，第三列说明每个因素能否改变。
每次乘加的指令条数由程序使用的指令决定。256 位向量寄存器可以存放 8 个 32 位整数或 32 个 8 位整数，一条向量指令对其中每个元素执行同一运算，这个条数降到 1 以下，ollama 的 0.69 条即由此得到。本讲接下来从「向量演进：从 MMX、SSE 到 AVX 与 AVX-512」起讲解这类指令。
每个周期开始执行的指令条数由硬件决定，一个核心是 5 条。提高它需要更多同时工作的运算单元。i9-11900H 有 8 个核心，ollama 用 8 个线程时实测 31.4 Token/s，是 1 个线程的 2.9 倍；GPU 的运算核心数量比 CPU 多三个数量级，由「线程并行：CPU 多核与 GPU」起的各页讲解。
主频的提高受电路延迟与功耗限制。每个 Token 的乘加次数由模型的权重数决定，推理程序不改变它。
两个方案提高的都是运算速率，在 CPU 上共同的上限是「硬件上限」一页的带宽上限 53 Token/s。GPU 使用自己的显存，它的带宽上限由显存带宽决定（第二讲）。
向量指令各页沿用第二部分的整数内积 sum += w[i] * x[i]；dot.c 的 -O2 标量循环每次乘加执行 6 条指令。
""")


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
  - **8 个 32 位整数（int32）** $\leftarrow$ 本节的 `dot_product`；
  - 或 8 个单精度浮点数（float32）；
  - 或 4 个 64 位整数/双精度浮点数；
  - 或 32 个 8 位整数（int8 / 字符）。
""")
    p.notes('单指令多数据流（SIMD）思想、256 位 YMM 寄存器结构与向下兼容映射。')


def vector_naming(p):
    p.title('向量命名：向量指令助记符的组成')
    slide(p, r"""
**向量指令助记符命名规律**：
- `v` 前缀：代表采用 VEX 编码的向量扩展指令，可以写三个操作数，最后一个是目的操作数；
- `p` 标记：代表 Packed（打包的向量整型数据）；
- 运算操作名称：如 `add`（加法）、`mul`（乘法）、`xor`（异或）；
- 元素位宽类型后缀：
  - `b`（byte，8 位整型）；`w`（word，16 位整型）；
  - `d`（doubleword，32 位整型）；`q`（quadword，64 位整型）。
- 实例：`vpmulld` = Vector Packed Multiply Low Doubleword（保留乘积低 32 位）。
""")
    figure(p, "mnemonic", 1120)
    p.notes("""
向量指令助记符由前缀、数据类别、运算名称与元素位宽后缀组成。
三操作数的例子：vpmulld (%rsi,%rax), %ymm2, %ymm0 把内存中的 8 个整数与 %ymm2 的对应元素相乘，结果写入最后一个操作数 %ymm0，%ymm2 的内容保持不变。
""")


def vector_arith(p):
    p.title('向量算术：vmovdqu、vpmulld 与 vpaddd 指令')
    slide(p, r"""
**向量加载：`vmovdqu (%rdi,%rax), %ymm2`**
- 从内存连续读取 256 位（32 字节，即 8 个 `int32`）到寄存器 `%ymm2`；
- `vmovdqu`（Unaligned）对内存地址没有对齐要求；`vmovdqa`（Aligned）要求地址是 32 的倍数，否则 CPU 触发异常，Linux 下进程收到 `SIGSEGV` 信号。

**向量乘法：`vpmulld (%rsi,%rax), %ymm2, %ymm0`**
- 同时完成 8 对 32 位整数的乘法，8 个乘积的低 32 位写入 `%ymm0`。

**向量加法：`vpaddd %ymm0, %ymm1, %ymm1`**
- `%ymm0` 中的 8 个乘积分别加到 `%ymm1` 中的 8 个累加和上。
""")
    p.notes("""
向量内积循环体中的三条 AVX2 指令：加载、乘法、加法。
vmovdqa 在地址未对齐时触发通用保护异常（#GP）。vmovdqu 读取的 32 字节跨越两个缓存行时，由硬件分两次读取。
""")


def vector_arith_fig(p):
    p.title('向量算术：vmovdqu、vpmulld 与 vpaddd 指令')
    slide(p, r"""
**每轮循环的向量运算**：$(s_7, \dots, s_1, s_0) \leftarrow (s_7, \dots, s_1, s_0) + (w_{i+7}\,x_{i+7}, \dots, w_{i+1}\,x_{i+1}, w_i\,x_i)$
""")
    figure(p, "lanes-mul-add", 1120)
    p.notes("""
公式中的 8 个分量同时计算：vpmulld 得到 8 个乘积，vpaddd 把它们加到 8 个累加和上，i 每轮增加 8。
全部循环结束后，8 个累加和相加得到内积：s₀ + s₁ + … + s₇。
""")


def intrinsics(p):
    p.title('内建函数：AVX2 Intrinsics 向量点积')
    slide(p, r"""
**AVX2 内积核心源码**：
""")
    p.code('c', """#include <immintrin.h>
__m256i vsum = _mm256_setzero_si256();   // vpxor %xmm1, %xmm1, %xmm1
for (int i = 0; i <= n - 8; i += 8) {
    __m256i va = _mm256_loadu_si256((__m256i*)&w[i]);  // vmovdqu
    __m256i vb = _mm256_loadu_si256((__m256i*)&x[i]);  // folded into vpmulld as its memory operand
    __m256i vprod = _mm256_mullo_epi32(va, vb);       // vpmulld
    vsum = _mm256_add_epi32(vsum, vprod);             // vpaddd
}
int sum = sum_lanes(vsum) + scalar_tail(w, x, n);  // last n % 8 elements: no vector instructions""")
    slide(p, r"""
**Intrinsics 与汇编指令的对应关系**：
- `__m256i` 数据类型 $\longleftrightarrow$ 硬件 256 位 YMM 向量寄存器；
- `_mm256_loadu_si256` $\longleftrightarrow$ `vmovdqu` 向量非对齐加载指令；
- `_mm256_mullo_epi32` $\longleftrightarrow$ `vpmulld` 并行低位双字乘法指令；
- `_mm256_add_epi32` $\longleftrightarrow$ `vpaddd` 并行双字累加指令。
""")
    p.notes("""
Intrinsics 是编译器在 <immintrin.h> 中提供的内建函数，写法与 C 函数调用相同，编译时通常对应特定的向量指令。
源码取自 examples/dot_intrin.c 的 dot_product_avx2。这个函数带有 __attribute__((target("avx2")))，只有它使用 AVX2 指令，编译时不需要 -mavx2；main 用 __builtin_cpu_supports("avx2") 选择版本，不支持 AVX2 的机器执行标量版本。编译运行：cd examples; gcc -O2 dot_intrin.c -o dot_intrin; ./dot_intrin，输出 avx2 3182690 与 check 3182690 两行，check 一行是标量循环的结果。
_mm256_setzero_si256 对应的 vpxor 写的是 %xmm1：带 VEX 前缀的指令写 128 位寄存器时把同名 YMM 寄存器的高 128 位清零，%ymm1 的 256 位因此全部为 0。
循环条件 i <= n - 8 使每一轮都有完整的 8 个元素。循环结束后，sum_lanes 把 vsum 中的 8 个累加和相加；n 不是 8 的倍数时，剩余的 n % 8 个元素由 scalar_tail 处理。
scalar_tail 是一个普通的 C 循环，从下标 n / 8 * 8 起每次处理一个元素，编译后只有标量指令（movl、imull、addl）。它带有 noinline 属性，保持为单独的函数；被内联进 dot_product_avx2 之后，gcc 会把这个循环也向量化。两个函数的定义都在 examples/dot_intrin.c 中，n 是 8 的倍数时 scalar_tail 返回 0。
""")


def vector_loop(p):
    p.title('向量循环：AVX2 向量主循环的汇编指令')
    p.demo('编译 dot_intrin.c，查看向量循环',
           """cd examples
gcc -O2 -S dot_intrin.c -o - | sed -f asm.sed | sed -n '/vpxor/,/jg/p;/jg/q'""",
           output="""	vpxor	%xmm1, %xmm1, %xmm1
.L9:
	vmovdqu	(%rdi,%rax,4), %ymm0
	vpmulld	(%rsi,%rax,4), %ymm0, %ymm0
	addq	$8, %rax
	vpaddd	%ymm1, %ymm0, %ymm0
	vmovdqa	%ymm0, %ymm1
	cmpl	%eax, %edx
	jg	.L9""",
           files=['examples/dot_intrin.c', 'examples/asm.sed'])
    slide(p, r"""
**单循环吞吐量变化**：
- 循环体从 `.L9` 到 `jg`，由 7 条指令构成，每轮迭代处理 **32 字节（8 个 int32 元素）**；
- 每完成 1 个内积元素的计算，平均指令消耗从标量的 6 条降为 **$7 / 8 = 0.875\text{ 条指令}$**。
""")
    p.notes("""
演示的输出是 dot_product_avx2 中从 vpxor 到 jg 的一段（gcc 15.2.0）：asm.sed 删去汇编伪指令，sed -n '/vpxor/,/jg/p;/jg/q' 打印从第一个含 vpxor 的行到第一个含 jg 的行，随后退出。这一段不含函数入口，输出与是否使用 -fcf-protection=none 无关，命令中省去了这个选项。
vpxor 在进入循环之前执行一次，把 %ymm1 中的 8 个累加和清零。它之前还有 leal -7(%rdx), %edx 与 xorl %eax, %eax，使 %edx 等于 n - 7，%eax 中的 i 等于 0；再往前是 cmpl $7, %edx 与 jle，n 小于 8 时不进入循环。
循环体的 7 条指令中，vmovdqu、vpmulld、vpaddd 对应源码的 4 个 Intrinsics（x 的加载并入 vpmulld 的内存操作数），addq、cmpl、jg 控制循环：addq 使 i 增加 8，cmpl 计算 (n - 7) - i，jg 在结果大于 0 时跳转，条件与源码的 i <= n - 8 相同。操作数 (%rdi,%rax,4) 是基址比例变址寻址，i 每轮增加 8，地址增加 32 字节。
vmovdqa %ymm0, %ymm1 在两个寄存器之间拷贝 256 位，源码中没有对应的语句：vpaddd 把新的累加和写入 %ymm0，这一条把它拷贝到下一轮使用的 %ymm1。寄存器之间的拷贝没有地址对齐的要求。
寄存器编号、标号与指令条数取决于编译器版本与优化选项。
""")


def perf(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    slide(p, r"""
**实验设置**（`bench.c`）：数组维度 $n = 4096$，循环调用 100,000 次，共约 4.1 亿次乘加。
""")
    p.demo('构建两个版本并用 perf 计数',
           """cd examples
make
perf stat -e instructions,cycles ./dot_scalar 2>&1 | grep -v -e '^$' -e 'seconds [us]'
perf stat -e instructions,cycles ./dot_avx2 2>&1 | grep -v -e '^$' -e 'seconds [us]'""",
           output=""" Performance counter stats for './dot_scalar':
     2,459,562,893      instructions                     #    3.82  insn per cycle
       643,892,105      cycles                           #    4.286 GHz
       0.150218491 seconds time elapsed
 Performance counter stats for './dot_avx2':
       310,762,907      instructions                     #    3.02  insn per cycle
       103,024,819      cycles                           #    4.286 GHz
       0.024035128 seconds time elapsed""",
           files=['examples/Makefile', 'examples/bench.c', 'examples/dot.c'])
    slide(p, r"""
- `dot_scalar` 是 `-O2` 编译的标量版本，`dot_avx2` 是 `-O2 -mavx2` 编译的 AVX2 版本。
""")
    p.notes("""
perf stat 把计数结果写到标准错误，2>&1 把它并入标准输出，grep -v 删去其中的空行与 seconds user、seconds sys 两行。
perf 需要内核允许普通用户读取硬件计数器：/proc/sys/kernel/perf_event_paranoid 不大于 1。Ubuntu 默认是 4，此时 perf 报告 No supported events found；课前由授课人执行一次 sudo sysctl kernel.perf_event_paranoid=1。
计数与耗时随 CPU 型号和频率变化，页面上的数值来自 i9-11900H。
dot_avx2 由 dot.c 加上 -mavx2 编译得到，向量循环由编译器生成，共 6 条指令，比「向量循环」一页的循环少一条 vmovdqa。
""")


def perf_2(p):
    p.title('性能测量：使用 perf 测量指令数与周期数')
    slide(p, r"""
**实测指标归纳**：
- **指令削减**：指令数从 24.6 亿减少到 3.1 亿，指令数比值为 7.91x（标量版 24.6 亿条指令除以每次乘加 6 条，正好对应 4.1 亿次乘加）；
- **周期与耗时加速**：耗时从 0.150s 降至 0.024s，端到端加速比达到 6.25x。
""")
    figure(p, "perf-bars", 1120)


def gpu_why(p):
    p.title('线程并行：CPU 多核与 GPU')
    slide(p, r"""
- **SIMT**（Single Instruction, Multiple Threads）：多个线程执行同一段代码，各处理一份数据。
- **CPU 的多核可以按 SIMT 的方式工作**：i9-11900H 有 8 个核心，至多 16 个线程同时运行。
- **GPU 的运算核心比 CPU 多三个数量级**，并且有独立的显存：
""")
    p.table([
        ['运算核心', '8 个', '21760 个'],
        ['一个核心的组成', '运算单元、乱序执行、分支预测、缓存', '运算单元；控制逻辑由 32 个核心共用'],
        ['存储与峰值带宽', '内存 DDR4，51.2 GB/s', '显存 GDDR7，1792 GB/s'],
        ['设计目标', '单个线程的执行时间短', '大量线程的总吞吐量高'],
    ], headers=['', 'CPU：i9-11900H', 'GPU：RTX 5090'], widths=[18, 42, 40])
    slide(p, r"""
- **GPU 为图像渲染设计**：一帧画面的每个顶点、每个像素执行同一段程序，运算以矩阵与向量的乘加为主；神经网络推理的矩阵向量乘是同一类计算。
""")
    p.notes("""
SIMD 是一条指令处理多个数据，SIMT 是多个线程执行同一段代码。CPU 的多核并行属于后一种：每个核心运行 1 个线程，各自处理数据的一段。i9-11900H 有 8 个核心，每个核心 2 个硬件线程，至多 16 个线程同时运行；ollama 用 8 个线程实测 31.4 Token/s，是 1 个线程的 2.9 倍。
RTX 5090 的 21760 个运算核心是 NVIDIA 规格表中的 CUDA Cores，等于 170 个 SM 乘以每个 SM 的 128 个。一个 CUDA 核心只完成一个线程的一次运算，取指令、译码与调度由同组的 32 个核心共用一套逻辑；CPU 的每个核心各有一套完整的控制逻辑与缓存。两种核心的功能不同，数量的对比说明的是可以同时执行的线程数。
RTX 5090 的显存是 32 GB 的 GDDR7，总线宽度 512 位，每条数据线的速率是 28 Gbit/s，峰值带宽为 28 × 512 ÷ 8 = 1792 GB/s；内存带宽 51.2 GB/s 是「硬件上限」一页的数值。
实验 parallel-dot 第二步的参考结果在 i9-11900H 与 RTX 3060 Laptop（3840 个运算核心）上测得，32768 × 4096 的矩阵向量乘：CPU 1 个线程的标量循环 59.8 ms，CPU 16 个线程并使用向量指令 4.5 ms，GPU 0.81 ms，依次是 1 倍、13.4 倍、73.5 倍。
图像渲染中，顶点的坐标变换是 4 × 4 矩阵与向量的乘法，每个像素的颜色由同一段着色程序计算，两者都是对大量数据执行相同的运算。
""")


def gpu_arch(p):
    p.title('GPU 架构：Thread、Thread Block、Grid 与 Warp')
    slide(p, r"""
**硬件**（以 RTX 5090 为例）：
- **SM（Streaming Multiprocessor，流式多处理器）**：GPU 的运算部件，共 170 个；每个 SM 有 128 个运算核心与一块共享内存。
- **显存**：32 GB，所有 SM 共用；它与主机内存之间的数据经 PCIe 总线拷贝。

**程序启动的线程分为三级**：
- **Thread（线程）**：最基本的执行单元，每个线程把同一个函数（称为 kernel）执行一遍；
- **Thread Block（线程块）**：一组线程，整块分配给 1 个 SM，块内的线程共用这个 SM 的共享内存；
- **Grid（网格）**：一次启动的全部线程块。

**Warp（线程束）**：SM 把线程块中的线程每 32 个编为一组，同一个 Warp 的 32 个线程在同一时刻执行同一条指令。
""")
    p.notes("""
NVIDIA GPU 的硬件组成与 CUDA 程序的线程组织。RTX 5090 的数值来自 NVIDIA 的规格表：21760 个 CUDA 核心，合 170 个 SM；32 GB 显存；PCIe 5.0。它的计算能力（Compute Capability）是 12.0，CUDA 文档给出这一代的上限：Warp 为 32 个线程，一个线程块至多 1024 个线程，一个 SM 上至多同时有 1536 个线程（48 个 Warp，全卡 261120 个线程），每个 SM 的共享内存 100 KB，一个线程块至多使用 99 KB。
一个 SM 可以同时容纳多个线程块；线程块的数量多于 SM 能容纳的数量时，其余线程块等待，前面的线程块结束后再分配。线程块在执行期间不更换 SM，块内线程因此可以共用这个 SM 的共享内存。
SM 取出一条指令，交给同一个 Warp 的 32 个线程同时执行，这是 SIMT 在硬件上的做法。同一个 Warp 的线程在分支处走向不同路径时，SM 依次执行各条路径，不在当前路径上的线程等待，这种情况称为分支分化（Branch Divergence）。
""")


def gpu_arch_fig(p):
    p.title('GPU 架构：Thread、Thread Block、Grid 与 Warp')
    slide(p, r"""
**线程的三级组织与 GPU 硬件的对应**：
""")
    figure(p, "gpu-simt", 1120)
    p.notes("""
左半是程序启动的线程：Grid 由线程块组成，图中展开了 Block 0，它的 256 个线程分为 8 个 Warp，每个小方格是 1 个线程。右半是硬件：170 个 SM，图中展开了 SM 0，它有一块共享内存与 128 个运算核心；显存由所有 SM 共用，经 PCIe 总线与主机内存交换数据。
两个箭头是两条规则：一个线程块整块分配给 1 个 SM；同一个 Warp 的 32 个线程同时执行同一条指令。图中每块 256 个线程是下一页程序的取值，一个线程块的线程数由程序在启动时指定。
一个线程块的线程数可以多于 SM 的运算核心数：图中的线程块有 256 个线程，即 8 个 Warp，SM 的 128 个运算核心在同一时刻执行 4 个 Warp。线程块的 8 个 Warp 都驻留在这个 SM 上，每个线程的寄存器保存在 SM 的寄存器堆中；SM 的 128 个运算核心分为 4 组，每组 32 个核心配一个调度器，调度器每个时钟周期从驻留的 Warp 中选出 1 个已就绪的，发出它的下一条指令。8 个 Warp 因此轮流使用运算核心；一个 Warp 等待访存结果时，调度器执行其他 Warp，切换时不需要保存和恢复寄存器。
一个 SM 至多驻留 48 个 Warp（1536 个线程），每块 256 个线程时是 6 个线程块。程序执行到 __syncthreads() 时，先到达的 Warp 等待同一个线程块的其余 Warp。4 组、每组一个调度器的结构来自 NVIDIA 公布的 Volta 至 Ampere 各代的 SM 结构；RTX 5090 的每个 SM 同样有 128 个运算核心与 64 K 个寄存器。
""")


def cuda_kernel(p):
    p.title('CUDA 程序：每个线程执行的 kernel 函数')
    p.code('cuda', """__global__ void dot_kernel(const int *w, const int *x, int *block_sum, int n) {
    __shared__ int cache[256];                        // one array per thread block
    int tid = threadIdx.x;                            // this thread in its block: 0..255
    int idx = blockIdx.x * blockDim.x + threadIdx.x;  // this thread in the grid
    cache[tid] = (idx < n) ? w[idx] * x[idx] : 0;     // one product per thread
    __syncthreads();                                  // until all 256 products are stored
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {    // 256 products -> 1 sum in 8 steps
        if (tid < s) cache[tid] += cache[tid + s];
        __syncthreads();
    }
    if (tid == 0) block_sum[blockIdx.x] = cache[0];   // one sum per block
}""")
    slide(p, r"""
- `__global__` 标明这个函数是 kernel，函数体由每个线程各执行一遍。
- 函数中没有遍历 `n` 个元素的循环：线程 `idx` 只计算 `w[idx] * x[idx]` 一个乘积。
- 每个线程块的 256 个乘积在共享内存 `cache` 中折半相加，8 步得到 1 个和。
""")
    p.notes("""
内积的 kernel，取自 examples/dot_cuda.cu。CUDA 程序用 C++ 书写，__global__、__shared__、threadIdx、blockIdx、blockDim、__syncthreads 是 CUDA 增加的写法。
blockIdx.x 是线程块在 Grid 中的编号，blockDim.x 是每个线程块的线程数（这里是 256），threadIdx.x 是线程在块内的编号，三者算出线程在 Grid 中的编号 idx。线程总数是 256 的倍数，可以多于 n，idx 不小于 n 的线程存入 0。
__syncthreads 使块内的每个线程在这一行等待，直到块内全部线程都到达这一行。第一处保证 256 个乘积都已存入 cache，循环内的一处保证这一步的加法都已完成。
折半相加的每一步把 cache 的后一半加到前一半上：s 依次是 128、64、…、1，参与相加的元素个数依次是 256、128、…、2，共 8 步，cache[0] 是这个线程块的和，由块内编号为 0 的线程写入 block_sum。各线程块的和由主机端程序相加。
""")


def cuda_host(p):
    p.title('CUDA 程序：主机端的分配、拷贝与启动')
    p.code('cuda', """// 1. allocate device memory and copy w and x from host memory into it
CHECK(cudaMalloc(&dw, n * sizeof(int)));
CHECK(cudaMalloc(&dx, n * sizeof(int)));
CHECK(cudaMalloc(&dpart, blocks * sizeof(int)));
CHECK(cudaMemcpy(dw, w, n * sizeof(int), cudaMemcpyHostToDevice));
CHECK(cudaMemcpy(dx, x, n * sizeof(int), cudaMemcpyHostToDevice));
// 2. start the grid: blocks x threads threads, each runs dot_kernel once
dot_kernel<<<blocks, threads>>>(dw, dx, dpart, n);
CHECK(cudaGetLastError());
// 3. copy the sums of the blocks back and add them up on the CPU
CHECK(cudaMemcpy(part, dpart, blocks * sizeof(int), cudaMemcpyDeviceToHost));
for (int b = 0; b < blocks; b++)
    sum += part[b];""")
    slide(p, r"""
- `main` 函数在 CPU 上执行，kernel 只能读写显存：数据先拷入显存，结果再拷回内存。
- `<<<blocks, threads>>>` 指定线程块个数与每块线程数：65536 块，每块 256 个线程。
- 数据拷入显存的用时比 kernel 的用时长：只计算一次内积时，时间主要用在拷贝上。
""")
    p.notes("""
dot_cuda.cu 的 main 函数中调用 CUDA 的部分；CHECK 是文件中定义的宏，检查每次调用的返回值，出错时打印原因并退出。
cudaMalloc 在显存中分配空间，返回的指针 dw、dx、dpart 指向显存，只能传给 kernel 或 cudaMemcpy，主机端代码不能直接读写。cudaMemcpy 的最后一个参数给出拷贝方向。
kernel 的启动写成 函数名<<<线程块个数, 每块线程数>>>(参数)。启动之后 main 继续执行，第 3 步的 cudaMemcpy 等待 kernel 结束后再拷贝。
两个用时的实测值取自实验 parallel-dot 第二步的参考结果（RTX 3060 Laptop，元素个数同为 2²⁴）：把 w、x 共 128 MiB 拷入显存 24.4 ms（5.5 GB/s，普通 malloc 内存经 PCIe 的拷贝速率），kernel 0.66 ms，拷贝是 kernel 的 37 倍。矩阵向量乘的权重留在显存中，每个 Token 只拷贝输入与输出向量，拷贝所占的比例很小。
""")


def cuda_build(p):
    p.title('CUDA 编译：nvcc 把一个源文件编译为两种指令')
    slide(p, r"""
**`nvcc` 把 `.cu` 文件中的代码分为两部分**：
- **主机端代码**（`main` 等普通函数）：由 gcc 编译为 x86-64 指令，在 CPU 上执行；
- **设备端代码**（`__global__` 函数）：编译为 GPU 的指令，作为数据放入可执行文件，运行时由 CUDA 运行库装入 GPU。
""")
    p.demo('编译运行 dot_cuda.cu',
           """cd examples
nvcc -O2 -arch=native dot_cuda.cu -o dot_cuda
./dot_cuda""",
           output="""blocks  65536 x 256 threads
sum     41943040
check   41943040""",
           files=['examples/dot_cuda.cu'])
    slide(p, r"""
- `nvcc` 随 CUDA Toolkit 安装，运行 `dot_cuda` 需要 NVIDIA 显卡与驱动程序。
- `sum` 是 GPU 的结果，`check` 是 CPU 循环的结果，两者相同。
""")
    p.notes("""
nvcc 是编译驱动程序，它依次调用几个工具，nvcc --dryrun 列出全部步骤：gcc -E 预处理；cudafe++ 把主机端代码与设备端代码分开；cicc 把设备端代码编译为 PTX（GPU 的汇编语言）；ptxas 把 PTX 汇编为 GPU 的机器指令；fatbinary 把这些指令包装为数据；gcc 把主机端代码与这份数据编译为 x86-64 目标文件；最后由 g++ 链接 CUDA 运行库 libcudart。
-arch=native 使 ptxas 为本机的 GPU 生成指令（RTX 5090 是 sm_120）；不写这个选项时为较早的架构生成指令，较新的 GPU 可以执行。
查看两种中间结果：nvcc -O2 -ptx dot_cuda.cu -o dot_cuda.ptx 得到 PTX，其中 mul.lo.s32 是乘法，bar.sync 是 __syncthreads；cuobjdump -sass dot_cuda 列出可执行文件中的 GPU 机器指令。
需要 NVIDIA GPU 与 CUDA Toolkit。输出中 65536 x 256 是线程块个数与每块的线程数。
""")


def cuda_remote(p):
    p.title('远程运行：把 CUDA 程序提交到课程服务器')
    slide(p, r"""
- 没有 NVIDIA 显卡时，把 `.cu` 文件发送到课程服务器：服务器编译、运行，返回输出。
""")
    p.demo('把 dot_cuda.cu 提交到课程服务器',
           """cd examples
curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @dot_cuda.cu $GPU_SERVER/program""",
           output="""# received program.cu (2668 bytes)
# started limit=60s
blocks  65536 x 256 threads
sum     41943040
check   41943040
# done status=ok elapsed=1.2""",
           files=['examples/dot_cuda.cu'])
    slide(p, r"""
- `$GPU_SERVER` 是服务器的地址，`$GPU_TOKEN` 是课程口令；`# ` 开头的行由服务器给出。
- 一次提交 1 个源文件，至多 64 KiB；编译与运行合计至多 60 s；GPU 每次运行 1 个任务，其余任务排队。
- 程序在隔离环境中运行：不能读写服务器上的文件，不能访问网络。
""")
    p.notes("""
服务器是 lectures/parallel-dot-server，POST /program 接收一个完整的 CUDA 程序：含 kernel 与 main 的一个 .cu 文件。
服务器把请求正文保存为 program.cu，执行 nvcc -O2 -arch=sm_XY -o program program.cu，再执行 ./program；程序没有参数，标准输入为空。
回复逐行返回：# received 是收到的字节数；GPU 正在运行其他任务时有一行 # queued position=K；# started 给出时间限制；
随后是编译器的信息与程序的输出；最后一行是 # done status=… elapsed=…，elapsed 是编译与运行合计的秒数。
status 有四个取值：ok；error，编译失败或程序以非零状态退出，这时前一行是 # program: exit status N；
timeout，超过时间限制被终止；output-limit，输出超过 1 MiB 被终止。
提交被拒绝时回复一行原因：403 口令错误，413 文件为空或超过 64 KiB，429 提交过于频繁（每个地址每 600 s 至多 6 次），503 等待的任务已有 8 个。
隔离环境由 bubblewrap 建立：程序只能看到只读的 /usr（编译器与 CUDA 库）、GPU 设备节点、两个存放在内存中的临时目录 /work 与 /tmp；
没有 /etc 与 /home，没有网络接口。到时间限制时，隔离环境中的全部进程被终止，GPU 交给下一个任务。
curl 的选项：-sS 只显示错误信息；-N 关闭输出缓冲，服务器每返回一行就显示一行；-H 添加请求头；--data-binary @文件 把文件内容原样作为请求正文。
Windows 上在 Git Bash 中执行，curl 随 Git for Windows 安装。
实验 parallel-dot 第二步的 make remote 使用同一个服务器的 POST /run，提交的是两个 kernel 文件。
""")


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
    p.notes('将全节出现的五类核心机器级指令汇总为结构化速查表。')


def insn_summary_2(p):
    p.title('指令总览：本节核心机器级指令分类速查')
    slide(p, r"""
3. **状态比较与分支跳转**：
   - `cmpl`, `cmpq`（减法设置标志位）；`testl`, `testq`（与逻辑设置标志位）；
   - `jmp`（无条件跳转，直接/间接）；
   - `jX`（条件跳转：有符号 `jl/jle/jg/jge`，无符号 `jb/jbe/ja/jae`，零值 `je/jne`）；
   - `setX`（条件设置字节）；`cmovX`（条件传送，消除分支）。
4. **函数调用与运行时安全**：
   - `call`, `ret`（返回地址压栈跳转与出栈恢复）；
   - `%fs:40`, `__stack_chk_fail@PLT`（金丝雀栈溢出保护）。
5. **向量计算（AVX2 扩展）**：
   - `vmovdqu`, `vmovdqa`（非对齐/对齐 256 位向量加载与存储）；
   - `vpmulld`, `vpaddd`（8 通道 32 位整数并行乘加）；
   - `vpxor`（向量寄存器清零）。
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


def exercise(p):
    p.title('课后练习 parallel-dot：AVX2 内积函数与 CUDA kernel')
    slide(p, r"""
- 实验目录 `parallel-dot/` 分两步，对应方案 1 与方案 2，按顺序完成，都不计分。
""")
    p.table([
        ['第一步', '`simd/`', '`vec_dot_q4_0`：用 AVX2 实现 Q4_0 内积', '`make test` 输出 `all passed`'],
        ['第二步', '`cuda/`', '点积与矩阵向量乘的 4 个 CUDA kernel', '`make run` 的 `check` 全部是 `same`'],
    ], headers=['步骤', '目录', '要写的代码', '完成标准'], widths=[10, 10, 44, 36])
    p.code('bash', """cd parallel-dot/simd
make test       # step 1: check the result and the speed-up over the scalar reference
make run        # step 1: answer prompt.txt with the model built in nano-quant
cd ../cuda
make run        # step 2: build, run every variant, print the table
make remote SERVER=http://HOST:PORT    # step 2 without an NVIDIA GPU""")
    slide(p, r"""
- 第一步需要支持 AVX2 的 CPU；第二步需要 NVIDIA 显卡，或者提交到课程服务器。
- 每一步的任务、规则与期望输出见各自目录下的 `README.md`。
""")
    p.notes("""
parallel-dot 是第四部分的课后练习，两步各有自己的目录、Makefile 与 README.md，依次对应「解决方案」一页的方案 1（SIMD）与方案 2（GPU）。
第一步的程序是「实测」一页的 mini-ollama：它的内层循环成为函数 vec_dot_q4_0，即 Q4_0 一行权重与 float 向量的内积，框架给出每次处理一个权重的标量参考实现，要写的是 AVX2 版本，每一步处理一个 Q4_0 块的 32 个权重。make test 运行 dot-selftest，不需要模型文件：先对照定义检查结果，再与标量参考实现比较速度，达到 4 倍以上时输出 all passed。make run 读取第二讲实验 nano-quant 按 q4_0 配方得到的 q4_0.gguf，输出生成速度；i9-11900H 上 256 位的实现为 4.5 Token/s，标量参考实现为 0.76 Token/s（2026-10-05）。
第二步的 4 个 kernel 是 dot_kernel、dot_stride_kernel、matvec_row_kernel 与 matvec_block_kernel。dot_kernel 的做法与「CUDA 程序：每个线程执行的 kernel 函数」一页的 kernel 相同。make run 输出一张表，每一行是一个版本或一个阶段的耗时、相对 CPU 标量版本的加速比与结果是否和 CPU 一致。
没有 NVIDIA 显卡的机器用 make remote 把 kernels/ 下的两个文件提交到课程服务器，服务器编译、运行后返回同一张表，HOST:PORT 是课程服务器的地址。
""")
