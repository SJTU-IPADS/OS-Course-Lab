"""第一讲《计算机系统导论》的页面内容。

组织方式：以一次 `ollama run` 请求为观察对象，将其分解为五个系统层次，
自底向上逐层考察每一层承担的工作。操作系统是本讲的重点：进程、虚拟内存、
mmap 加载、页缓存、设备访问与调度，均以同一次请求为例，使「操作系统在
大模型推理过程中承担的职责」成为可以明确表述的结论。

用语要求：面向本科课程教学，采用陈述性的技术表述；不使用比喻、口语化措辞。

排版约定：
- 粗体后面不要紧跟全角冒号。`**词：**内容` 不符合 CommonMark 的闭合规则，
  会原样把 `**` 打到投影上；写成 `**词**：内容`。
- `==标记==` 只在 `p.slide(...)` 里展开，写进 highlight / 图注 / 表格会变成字面量。
- 一页最多一个 `p.highlight`，并且只给真正的结论用。
"""

# ==============================================================================
# 共享代码清单
# ==============================================================================

MINI_OLLAMA = """typedef struct { float weights[4]; } Model;    // a toy model: four weights

static float infer(const Model *m, const float x[4]) {
    float score = 0.0f;
    for (int i = 0; i < 4; ++i)
        score += m->weights[i] * x[i];         // one dot product: this is "inference"
    return score;
}

int main(int argc, char **argv) {
    Model model;
    float state[4] = {1.0f, 0.5f, -1.0f, 2.0f};

    FILE *f = fopen(argv[1], "rb");            // (1) needs data:    libc -> kernel
    fread(&model, sizeof(Model), 1, f);
    fclose(f);

    float r = infer(&model, state);            // (2) needs compute: compiled to CPU code
    printf("result = %.2f\\n", r);              // (3) needs output:  libc -> kernel
}"""

DISASSEMBLY = """1170:  movss   (%rdi,%rax,4), %xmm1    # load one weight into a register
1175:  mulss   (%rsi,%rax,4), %xmm1    # weight x input
117a:  addss   %xmm1, %xmm0            # accumulate into the sum
117e:  addq    $1, %rax                # i++
1182:  cmpq    $4, %rax                # i == 4 ?
1186:  jne     1170                    # not done -> jump back"""

CUDA_SAMPLE = """__global__ void dot_kernel(const float *w, const float *x, float *out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;   // which thread am I?
    if (i < n) atomicAdd(out, w[i] * x[i]);          // one element per thread
}

dot_kernel<<<blocks, 256>>>(d_w, d_x, d_out, n);     // tens of thousands of threads"""

COMPILER_ROWS = [
    ["**1 · 预处理**", "`gcc -E`", "`.c` → `.i`", "展开 `#include`、替换宏、移除注释"],
    ["**2 · 编译**", "`gcc -S`", "`.i` → `.s`", "将 C 语句翻译为文本形式的汇编"],
    ["**3 · 汇编**", "`gcc -c`", "`.s` → `.o`", "将汇编助记符翻译为二进制机器码"],
    ["**4 · 链接**", "`gcc`", "`.o` + 库 → 可执行文件", "布局各节、解析符号、记录动态依赖"],
]


# ==============================================================================
# 课程概览
# ==============================================================================

def staff_and_textbooks(p):
    p.title("任课教师与课程教材")
    p.slide("""
- **任课教师**：臧斌宇 · 古金宇
- **联系方式**：byzang@sjtu.edu.cn · gujinyu@sjtu.edu.cn
- **办公室**：软件学院 3 号楼 2 楼 IPADS 实验室，答疑请提前约时间
- **主要教材**：*Computer Systems: A Programmer's Perspective*（CS:APP 第 3 版）
- **语言参考**：*The C Programming Language*（K&R 第 2 版）
""").image_right("assets/instructors.png", width_px=260)
    p.sidenote(
        "第一讲建议阅读",
        "CS:APP 第 3 版第 1 章：**§1.1**（信息就是位加上下文）、**§1.2**（编译系统）、"
        "**§1.4**（处理器如何读并执行指令）、**§1.7**（操作系统管理硬件）、**§1.8**（网络）。",
    )
    p.cite(title="Computer Systems: A Programmer's Perspective", author="Bryant & O'Hallaron",
           year="2016", venue="Prentice Hall, 3rd ed.", key="csapp")
    p.cite(title="The C Programming Language", author="Kernighan & Ritchie",
           year="1988", venue="Prentice Hall, 2nd ed.", key="knr")


def course_goals(p):
    p.gap(52)
    p.title("课程目标：建立程序执行的系统模型")
    p.slide("""
先修基础：C++ 的基本语法——`for` 循环、函数、类、指针与 `std::vector`，
以及在终端中编译并运行一个程序。
""", autobold=False)
    p.slide("""
本课程讨论这些语法之下的执行机制：
- `for` 循环如何被编译为处理器执行的机器指令？
- `malloc` 返回的内存由谁分配？进程之间为何互相不可见？
- 程序性能受限时，==数据移动==的开销为何常常超过算术运算？
""")
    p.highlight("从程序员视角建立系统模型，用于解释和诊断程序行为。", tone="blue")



# ==============================================================================
# 问题的提出：从 AI 应用到一次推理请求
# ==============================================================================

def ai_app_request(p):
    p.title("AI 应用的工作方式：一次请求")
    p.slide("""
在对话界面或 Agent 中提出问题时，客户端本身并不执行计算：它把问题与上下文
组装成一次 **HTTP 请求**发给推理服务，再把返回的 **token 流**逐段显示出来。
""", autobold=False)
    p.image("assets/agent-request.svg", width_px=980,
            caption="实线为请求路径，虚线为结果返回路径")
    p.highlight("应用负责组织对话，计算发生在推理服务一侧。", tone="blue")


def openai_api(p):
    p.title("客户端与推理服务之间的接口：OpenAI 兼容 API")
    p.slide("这组 HTTP 接口最早由 OpenAI 定义，目前被绝大多数推理服务实现。", autobold=False)
    p.demo("调用 completions 接口", """curl http://localhost:11434/v1/chat/completions -H 'Content-Type: application/json' \\
    -d '{"model": "llama3.2", "stream": true,
         "messages": [{"role": "user", "content": "Why is the sky blue?"}]}'""")
    p.table(
        headers=["端点", "作用"],
        rows=[
            ["`/v1/chat/completions`", "多轮对话补全，当前的主要端点"],
            ["`/v1/completions`", "早期的单段文本补全接口"],
            ["`/v1/embeddings`", "将文本编码为向量"],
            ["`/v1/models`", "列出该服务可用的模型"],
        ],
    )
    p.aside("同一组接口下，后端可以是云端服务、集群上的 vLLM，或本机的 ollama。")


def ollama_intro(p):
    p.gap(30)
    p.title("Ollama：运行在本机的推理服务")
    p.slide("""
Ollama 在本地提供的正是这组接口。
- 启动后监听 `http://localhost:11434`，`/v1/...` 路径与云端服务一致
- 模型权重以文件形式存放在本机磁盘上
- 请求、加载、计算、返回这条完整路径，都可以在自己的机器上观察
""").image_right("assets/ollama-logo.png", width_px=100)
    p.demo("启动推理服务", "ollama serve", timeout=0)
    p.demo("下载模型权重", "ollama pull llama3.2", timeout=0)
    p.demo("提交一次请求", "ollama run llama3.2", timeout=0)
    p.highlight("本节以 ollama 为例，可以自行在 PC 上尝试。", tone="blue")


def one_command(p):
    p.title("从一个可观察的事件出发：执行一次 Ollama 请求")
    p.demo("执行一次推理请求", 'ollama run llama3.2 "Why is the sky blue?"',
           output="""The sky appears blue because of Rayleigh scattering. Sunlight contains all
visible wavelengths; as it passes through the atmosphere it interacts with
gas molecules much smaller than its wavelength. Shorter wavelengths scatter
far more strongly than longer ones, so blue light is redirected across the
whole sky while red light passes through more directly.""",
           timeout=0)
    p.slide("""
输出是逐 token 出现的。
- 命令本身不包含==任何==关于计算过程的描述
- 权重的存放位置、使用 CPU 或 GPU、内存不足时的处理，均未指定
""")
    p.highlight("本节将分解这一过程，确定上述决策由哪些系统层完成。", tone="orange")
    p.notes("可以在投影上实际执行一次该命令，在生成过程中展开讲解。")


def three_processes(p):
    p.gap(26)
    p.title("观察一：该命令对应三个进程")
    p.demo("列出相关进程", "ps -eo pid,comm,args | grep '[o]llama'",
           output="""1832  ollama         /usr/local/bin/ollama serve
1904  ollama-runner  ... --model ~/.ollama/models/blobs/sha256-...
2077  ollama         ollama run llama3.2""")
    p.slide("""
- **CLI**（2077）：提交请求，接收流式输出
- **服务进程**（1832）：常驻运行，查找模型元数据并调度请求
- **推理进程**（1904）：加载权重至内存，调用 CPU 或 GPU 后端
""")
    p.highlight("进程可以理解为独立运行的程序实例在操作系统中的执行单元。", tone="blue")
    p.aside("进程划分方式随版本变化，但所依赖的系统资源保持不变。")
    p.cite(title="Ollama", author="Ollama", venue="github.com/ollama/ollama",
           url="https://github.com/ollama/ollama", key="ollama")


def weights_are_data(p):
    p.gap(26)
    p.title("观察二：模型权重是一个 1.9 GB 的数据文件")
    p.demo("查看权重文件", """ls -lhS ~/.ollama/models/blobs | head -2
file ~/.ollama/models/blobs/sha256-* | head -1""",
           output="""total 1.9G
-rw-r--r-- 1 ollama ollama 1.9G Sep  1 19:50 sha256...
...sha256-...: data""")
    p.slide("""
`file` 无法识别其类型：文件内容是==浮点数数组==，不含任何可执行指令。
- 需要由某一层将其从存储设备载入内存
- 需要将矩阵运算翻译为 CPU 或 GPU 可执行的指令
- 需要将生成结果传回终端
""")
    p.highlight("模型只提供参数，而执行过程由系统栈完成。", tone="orange")


def five_layers(p):
    p.title("Ollama 请求跨越的五个系统层次")
    # Layer names are kept short enough to sit on one line in the 140px label
    # gutter: a wrapped label makes every band taller and the stack overflows
    # the slide. They also match the summary diagram in `request_recap`.
    arch = p.architecture(caption="箭头表示依赖方向：每一层只使用下一层提供的接口", flow="down")
    arch.layer("5 · 应用", ["Ollama CLI", "对话界面", "工具调用"])
    arch.layer("4 · 运行时", ["模型加载", "张量算子", "libc / libstdc++", ...])
    arch.layer("3 · 操作系统", ["进程", "虚拟内存", "文件与页缓存", "设备驱动", "调度"])
    arch.layer("2 · 指令集", ["x86-64 指令", "SIMT / PTX"])
    arch.layer("1 · 硬件", ["CPU", "DRAM", "GPU", "存储", "网络"])
    p.notes("""
每一层向上层提供确定的接口，并隐藏其实现细节。
本图在本讲出现两次：此处自上而下介绍，末尾自下而上小结。
本讲重点是第 3 层；第 1、2 层是理解第 3 层的前提。
""")


# ==============================================================================
# 第 1 层 · 硬件
# ==============================================================================

def stored_program(p):
    p.title("存储程序体系结构：指令与数据同存于内存")
    p.slide("""
早期计算机更换程序需要==重新接线==。存储程序思想将指令与数据一同存放在内存中。
- 处理器的执行过程统一为：取指 → 译码 → 执行
- 更换程序即更换内存中的一段字节，无需改变硬件连接
""")
    p.image("assets/early-computers-clean.png", width_px=520,
            caption="ENIAC 的插接板编程（左）；冯·诺依曼与《EDVAC 报告初稿》（右）"
            ).footnote("ENIAC 照片来自 Wikimedia Commons。")
    p.aside("ollama 同样遵循这一模型：其机器码与 1.9 GB 权重共同驻留在 DRAM 中。")


def machine_parts(p):
    p.title("硬件组成：处理器、内存与互连")
    p.slide("""
- **CPU**：控制单元、算术逻辑单元与寄存器，顺序执行指令
- **DRAM**：存放当前使用的代码与数据，掉电后内容丢失
- **GPU**：大量并行执行单元，配备独立的高带宽设备内存
- **总线与 I/O**：连接上述部件与存储设备、网络接口
""", reveal="items").image_right("assets/intel-core-ultra-200-tiles.png", width_px=210)
    p.image("assets/hardware-bus.svg", width_px=830,
            caption="总线是各部件之间的公共通路；本讲涉及的三类资源均在此图中："
                    "计算能力、内存容量与访存带宽")
    p.notes("""
带宽数量级只作为比较用：内存总线约 100 GB/s，PCIe 5.0 x16 约 64 GB/s，
NVMe SSD 约 5 GB/s。这一递减关系是后面「权重从磁盘进入内存」一节的前提。
""")


def cpu_vs_gpu(p):
    p.title("CPU 与 GPU 的分工：延迟优化与吞吐优化")
    p.table(
        headers=["", "CPU", "GPU"],
        rows=[
            ["设计目标", "**低延迟**：单个任务尽快完成", "**高吞吐**：大量任务并行完成"],
            ["执行单元", "数量少、单元强，深流水线与大容量缓存", "数量大、单元简单，规模可达上万"],
            ["内存", "容量大、访问延迟低", "容量小、**带宽极高**"],
            ["承担的工作", "调度、分词与请求协调", "矩阵乘与注意力计算"],
        ],
        align=["right", "left", "left"],
    )
    p.slide("""
LLM 逐 token 解码时，每生成一个 token 需==完整读取一遍模型参数==。
- 在小批量下，性能主要受**访存带宽**限制
- **HBM**（High Bandwidth Memory，高带宽内存）的带宽因此成为加速器的关键指标
- 右图：NVIDIA 两代加速器的显存带宽，HBM3e 为 8 TB/s，HBM4 为 22 TB/s
""").image_right("assets/nvidia-rubin-memory-bandwidth.png", width_px=210)
    p.cite(title="Inside the NVIDIA Rubin Platform", author="NVIDIA", year="2026",
           venue="NVIDIA Technical Blog",
           url="https://developer.nvidia.com/blog/inside-the-nvidia-rubin-platform-six-new-chips-one-ai-supercomputer/",
           key="rubin")


# ==============================================================================
# 第 2 层 · 指令集
# ==============================================================================

def isa_contract(p):
    p.title("指令集体系结构：软件与硬件之间的接口规范")
    p.slide("""
ISA 规定了软件可见的机器状态：指令集合、寄存器组织与数据格式。
- **对编译器**：以 ISA 为目标生成代码，无需依赖具体的微体系结构实现
- **对处理器**：缓存、流水线、乱序执行的实现方式可自由选择，只需保证==行为==符合规范
""")
    arch = p.architecture(flow="down")
    arch.layer("软件", ["ollama", "应用程序", "任意编译器的输出"])
    arch.layer("x86-64 ISA", ["寄存器", "指令", "寻址", "特权级"])
    arch.layer("微体系结构", ["AMD 的实现", "Intel 的实现"])
    p.highlight("这是本课程的第一个抽象层：接口固定，两侧实现独立演进。", tone="blue")


def x86_lineage(p):
    p.gap(26)
    p.title("x86 体系结构的演进与向后兼容")
    p.table(
        headers=["代际", "年份", "字长", "能寻址", "关键新增"],
        rows=[
            ["8086", "1978", "16 位", "1 MiB", "分段"],
            ["IA-32 / 386", "1985", "32 位", "4 GiB", "分页与保护机制，虚拟内存的硬件基础"],
            ["x86-64 / AMD64", "2003", "64 位", "由实现决定", "扩展寄存器数量，保留 32 位兼容"],
        ],
        align=["left", "center", "center", "center", "left"],
    )
    p.slide("""
四十余年间每一代都保留了前一代的指令编码，代价是实现复杂度持续累积。
- 向后兼容使已编译的程序可在不同世代的处理器上运行
- **386 引入的分页与保护**，是下一节讨论的操作系统机制的硬件前提
""")
    p.aside("32 位程序能否运行，还取决于操作系统与运行库是否提供 32 位支持。")


# ==============================================================================
# 第 3 层 · 操作系统
# ==============================================================================

def why_os(p):
    p.gap(52)
    p.title("操作系统的必要性")
    p.slide("浏览器、编辑器与 ollama 同时请求同一组处理器与内存资源，需要有机制完成分配与保护。")
    p.slide("""
若应用程序可以直接访问硬件：
- **缺乏隔离**：一个程序的越界写入可以破坏另一程序的内存
- **缺乏调度**：没有抢占机制时，单个程序可长期独占处理器
- **缺乏统一接口**：更换设备需要修改应用程序
""", reveal="items").image_right("assets/os-everywhere.png", width_px=280)
    p.highlight("操作系统复用硬件资源，并实施保护边界。", tone="blue")


def os_services(p):
    p.title("操作系统提供的四类抽象")
    p.slide("ollama 通过下列抽象访问硬件，不直接管理 CPU 时间、物理页、磁盘或网卡。")
    arch = p.architecture(caption="每类抽象对应一类硬件资源，也对应本课程后续的一章", flow="down")
    arch.layer("Ollama 服务与推理进程", ["提交请求", "加载模型", "调用计算后端"])
    arch.layer("操作系统的四组抽象", ["进程与调度", "虚拟内存", "文件与页缓存", "设备与网络接口"])
    arch.layer("硬件", ["CPU / GPU", "DRAM", "存储设备", "网卡"])
    p.notes("""
这四类抽象即接下来五页的顺序：进程 → 虚拟内存 → 文件与页缓存 → 设备与调度。
每一页均以同一个 ollama 实例为例，保持例子一致。
""")


def process_isolation(p):
    p.title("进程：执行与保护的基本单位")
    p.slide("""
前面 `ps` 输出中的 1832 / 1904 / 2077，各自拥有：
- 独立的**虚拟地址空间**：同一虚拟地址 `0x7fff...` 在不同进程中映射到不同的物理内存
- 独立的**打开文件表**与寄存器上下文
- 可被内核==抢占==的执行流
""")
    p.sidenote(
        "进程隔离限制故障传播范围",
        "`ollama-runner` 因段错误退出时，`ollama serve` 收到子进程结束通知，"
        "重新创建推理进程即可继续服务。**隔离的首要作用是把故障限制在一个进程之内；"
        "在此基础上，它同时构成安全边界。**",
    )
    p.highlight("进程是操作系统为一个运行中的程序划定的资源与保护边界。", tone="blue")


def virtual_address_space(p):
    p.title("进程虚拟地址空间的典型布局")
    p.slide("""
- **Text**：机器指令，通常映射为只读，写入将触发段错误
- **Data / BSS**：全局变量与静态变量
- **堆**：由 `malloc` / `new` 管理的动态内存，向高地址增长
- **栈**：局部变量、返回地址与调用现场，向低地址增长
- 两者之间：**共享库与内存映射区域**
""").image_right("assets/address-space.svg", width_px=330)
    p.highlight("下一节讨论的 1.9 GB 模型权重，映射在图中橙色区域。", tone="orange")
    p.aside("实际排列还受 ABI、ASLR、动态链接与线程影响。后续章节中会进行讨论。")


def loading_problem(p):
    p.gap(26)
    p.title("模型加载问题：1.9 GB 数据如何进入内存")
    p.slide("最直接的实现是将整个文件读入一块用户态缓冲区：", autobold=False)
    p.code("c", """void *buf = malloc(1900000000);          // ask for 1.9 GB of anonymous memory
read(fd, buf, 1900000000);               // then copy 1.9 GB off the disk""")
    p.slide("""
这一实现存在两个问题：
- 在物理内存为 8 GB 的机器上，该分配请求将失败
- 即使分配成功，每个字节需经**两次拷贝**（磁盘 → 内核缓冲区 → 用户缓冲区），启动延迟显著
""")
    p.highlight("实际实现并未采用这一方式：`ollama run` 的重复执行启动延迟极低。", tone="orange")


def model_loading(p):
    p.title("mmap：由操作系统按需完成数据装入")
    p.slide("""
另一种做法是建立**映射**：把文件的一段区间对应到进程地址空间的一段区间。
""", autobold=False)
    p.image("assets/mmap-overview.svg", width_px=840)
    p.highlight("模型加载的实质是一次地址空间映射与按需的缺页装入。", tone="orange")
    p.notes("""
本页只讲定性结论，虚拟内存一章会展开页表、缺页处理与回收策略。
课堂上按三个编号讲：① 映射建立时不发生拷贝；② 访问映射区触发缺页；
③ 内核装入所需的那一页。据此回答上一页的问题：物理内存为 8 GB 的机器
之所以能运行 1.9 GB 的模型，是因为常驻内存量取决于实际访问到的页。
""")


def mmap_in_practice(p):
    p.title("同一机制在实际系统中的效果：llama.cpp 的加载改动")
    p.slide("""
2023 年 llama.cpp 把权重加载从「读入用户缓冲区」改为 `mmap`，模型与算法均未变化。
""", autobold=False)
    p.image("assets/llama-cpp-mmap-pr.png", width_px=760, framed=True,
            caption="标题与三条说明都描述同一次改动的效果：加载更快、可加载的模型更大、"
                    "多个推理进程可并行")
    p.highlight("这一量级的差别由所使用的操作系统机制决定。", tone="orange")
    p.cite(title="Make loading weights 10-100x faster", author="Justine Tunney",
           year="2023", venue="ggml-org/llama.cpp PR #613",
           url="https://github.com/ggml-org/llama.cpp/pull/613",
           key="llama-mmap-pr")
    p.notes("""
该提交同时给出三点效果：加载显著加快、可加载的模型规模提高、多个推理进程可共享同一份页缓存。
三点都由同一个原因得到：权重页由内核的页缓存直接提供，不再复制到用户缓冲区。
ollama 使用 llama.cpp 作为推理后端，沿用了这一加载方式。
""")


def page_cache(p):
    p.gap(26)
    p.title("页缓存：重复启动时的加载开销")
    p.slide("""
因缺页装入 DRAM 的权重页，在进程退出后==并不立即释放==；
在内存充裕时，操作系统将其保留在**页缓存**中。
- 第二次启动时这些页直接命中，无需磁盘 I/O
- 内存压力上升时，内核按替换策略将其淘汰
""")
    p.slide("这解释了同一程序冷启动与热启动之间的性能差异。")
    p.highlight("同一份数据可能同时存在于存储设备、页缓存与设备内存中。", tone="blue")
    p.aside("多级存储中的数据副本问题，将在后续的存储层次一章继续讨论。")


def devices_and_drivers(p):
    p.gap(52)
    p.title("设备访问：GPU 同样经由操作系统")
    p.slide("""
推理进程不能直接向 GPU 提交命令，其访问路径为：
- 打开**设备文件**（Linux 上为 `/dev/nvidia*`）
- 经**驱动程序**申请设备内存、提交核函数并等待完成
- 由内核保证进程之间的设备内存互不可访问
""")
    p.slide("""
将张量传输至设备内存，是一次**跨越三级存储的数据移动**：
页缓存 → 进程地址空间 → 设备内存。
""")
    p.highlight("跨层数据移动的开销，是首 token 延迟的主要来源之一。", tone="orange")


def scheduling(p):
    p.gap(52)
    p.title("并发请求下的调度")
    p.slide("""
服务进程可能同时处理多个请求。在处理器核数有限的条件下，操作系统提供两项机制：
- **抢占式调度**：限制单个执行流连续占用处理器的时间
- **阻塞式等待**：等待磁盘、GPU 或网络的进程让出处理器
""")
    p.slide("""
推理服务在此之上实现自身的调度策略：
请求排队、连续批处理（continuous batching）与准入控制。
""")
    p.highlight("操作系统提供机制，上层服务决定策略，这一特征贯穿本课程。", tone="blue")


def os_evolution(p):
    p.title("操作系统的演进与不变的核心职责")
    p.image("assets/os-timeline.svg", width_px=610,
            caption="选取的若干里程碑，不构成单一的继承关系")
    p.slide("""
- **Multics**（1960 年代）系统地实现了分时与保护机制
- **Unix**（1969）强调紧凑接口与可组合的工具，1973 年以 C 重写，显著降低移植成本
- **Linux**（1991 至今）为持续演进的宏内核；**Redox**（2015 至今）以 Rust 实现微内核结构
- 实现语言与内核结构各不相同，需要解决的核心问题保持一致：==隔离、内存、文件与设备==
""")
    p.cite(title="Redox OS", venue="redox-os.org", url="https://www.redox-os.org/", key="redox")


def os_evolution_people(p):
    p.gap(52)
    p.title("操作系统的演进与不变的核心职责")
    p.slide("每一次结构性的改变，都伴随一组新的接口约定被确立下来。", autobold=False)
    # Four separate portraits, not a collage: the stitched GNU/Linux picture put
    # four subjects into one third of the row and read as a cramped strip.
    p.row()\
     .image("assets/corbato.jpg", height_px=225, caption="Corbató · Multics")\
     .image("assets/unix-creators.jpg", height_px=225, caption="Thompson 与 Ritchie · Unix")\
     .image("assets/stallman.jpg", height_px=225, caption="Stallman · GNU")\
     .image("assets/torvalds.jpg", height_px=225, caption="Torvalds · Linux")
    p.highlight("被广泛实现的接口，其存续时间长于实现它的具体系统。", tone="blue")


# ==============================================================================
# 第 4 层 · 工具链与运行时
# ==============================================================================

def source_is_bytes(p):
    p.title("源代码的表示：编码后的字节序列")
    p.slide("""
编译之前，源文件同样是数据：一段按字符编码规则存储的字节序列。
- ASCII 以 7 位表示一个字符：`'A'` 为 65，`'0'` 为 48，`'\\n'` 为 10
- `int main()` 在磁盘上的表示为 `69 6e 74 20 6d 61 69 6e 28 29`
""")
    p.image("assets/ascii-table.png", width_px=460,
            caption="一段 C 源码及其对应的字符编码")
    p.highlight("信息由位与上下文共同决定：字节的含义取决于解释它的程序。", tone="blue")


def why_c(p):
    p.title("本课程使用 C 的原因")
    p.slide("""
C 的抽象层较薄，系统细节在源码层面==直接可见==：
- 内存的申请与释放时机在代码中显式给出
- 指针值即进程虚拟地址空间中的地址，可用 `%p` 输出并与上一节的布局图对应
- 结构体的对齐与填充可直接观察，`sizeof` 反映对象的实际大小
- 多数操作系统接口与原生库以 C 兼容 ABI 对外，是跨语言互操作的公共边界
""")
    p.code("c", """struct Tensor { float *data; size_t length; };
printf("addr=%p, sizeof=%zu\\n", (void *)t.data, sizeof t);   // address and layout, visible""")
    p.highlight("C 在本课程中用作观察系统行为的工具。", tone="blue")


def mini_ollama(p):
    p.title("mini_ollama.c：跨越三类系统接口的示例程序")
    p.code("c", MINI_OLLAMA)
    p.notes("该程序仅为教学示例，不构成 LLM 实现；但它所使用的系统服务与实际的 ollama 属于同一类。")


def mini_ollama_boundaries(p):
    p.gap(52)
    p.title("mini_ollama.c：跨越三类系统接口的示例程序")
    p.slide("上一页中标记为 ①②③ 的三处调用，分别经过三条不同的执行路径。")
    p.image("assets/mini-boundaries.svg", width_px=760,
            caption="① 获取数据 · ② 执行运算 · ③ 输出结果")
    p.aside("接下来两页考察路径 ②：C 语句到机器指令的翻译过程。")


def compile_pipeline(p):
    p.title("编译的四个阶段：从源文件到可执行文件")
    p.table(headers=["阶段", "命令", "产物", "这一步做了什么"],
            rows=COMPILER_ROWS, align=["left", "left", "left", "left"])
    p.demo("gcc 编译的四个步骤", """cd examples
gcc -E mini_ollama.c -o mini_ollama.i     # see what the macros expanded to
gcc -S mini_ollama.i -o mini_ollama.s     # see what the assembly looks like
gcc -c mini_ollama.s -o mini_ollama.o     # see which symbols the object file has
gcc    mini_ollama.o -o mini_ollama       # link it into an executable
ls -l mini_ollama.i mini_ollama.s mini_ollama.o mini_ollama""")
    p.aside("`gcc a.c` 默认一次完成四个阶段；本课程要求能够分别观察每一阶段的输出。")


def machine_code(p):
    p.gap(26)
    p.title("点积循环的一种 x86-64 指令实现")
    p.code("text", DISASSEMBLY)
    p.slide("""
以下六条指令对应上一页 `infer()` 中的完整循环：
- `movss` / `mulss` / `addss`：装载、相乘、累加，构成**乘加运算的基本形式**
- `cmpq` / `jne`：实现循环条件判断与控制转移；机器层面不存在 `for` 这一结构
""")
    p.aside("具体生成的指令取决于编译器、优化选项、ISA 扩展与目标处理器。")


def runtime_libraries(p):
    p.title("语言运行库：应用与系统调用之间的一层")
    p.slide("""
`ldd` 列出一个可执行文件在启动时需要装载的**共享库**，以及每个库在文件系统中的实际路径。
""", autobold=False)
    p.demo("列出动态链接库", """cd examples && g++ cpp_demo.cpp -o cpp_demo
ldd ./cpp_demo""",
           output="""libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f...)
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f...)""")
    p.slide("""
- 这两行说明 `cpp_demo` 的机器码==并不完整==：`printf` 等函数的实现在库文件中，装载时才补齐
- `printf` 由 **libc** 实现，`std::cout` 由 **libstdc++** 实现，后者最终调用前者提供的底层接口
- 库对输出进行**缓冲**：多次 `printf` 可能只产生一次 `write` 系统调用，两者不是一一对应
""")
    p.highlight("共享库是装载进进程地址空间的、已编译的机器码。", tone="blue")
    p.notes("""
`ldd` 的输出即动态链接器在启动时要完成的工作清单。可在课堂上对同一个程序
分别执行 `ldd` 与 `gcc -static` 后的 `ldd`，说明静态链接与动态链接的差别。
""")


def python_and_pytorch(p):
    p.gap(26)
    p.title("解释执行的程序同样由机器指令完成")
    p.slide("""
- Python 源码由**解释器**执行；解释器本身是一个原生的 ELF 可执行文件
- 在 PyTorch 中，Python 负责==组织==计算过程：构建模型并分派算子
- 算子的实际执行位于编译好的原生库中：`libtorch_cpu.so` / `libtorch_cuda.so`
""")
    p.image("assets/interpreter-path.svg", width_px=780,
            caption="Python 源码 → 解释器（原生可执行文件）→ 运行时与原生库 → CPU 机器指令")
    p.demo("查看 Python 解释器与 Torch 算子库", """file "$(command -v python3)"
ls "$(python3 -c 'import torch;print(torch.__path__[0])')"/lib/libtorch_*.so""",
           output="""python3: ELF 64-bit LSB pie executable, x86-64, ...
libtorch_cpu.so   libtorch_cuda.so""")
    p.highlight("解释执行改变的是计算的组织方式，而指令仍由处理器执行。", tone="orange")
    p.cite(title="The Python Language Reference: Execution model", author="Python Software Foundation",
           url="https://docs.python.org/3/reference/executionmodel.html", key="pyexec")


def cuda_kernel(p):
    p.gap(30)
    p.title("GPU 程序的编写与编译")
    p.code("cuda", CUDA_SAMPLE)
    p.slide("""
**SIMT（单指令、多线程）**：核函数描述==单个线程==的计算，启动配置给出线程网格的规模。
- 每个线程处理一个向量元素，GPU 按组将其调度到可用执行单元
- 实际的归约实现远比 `atomicAdd` 高效，此处仅用于说明执行模型
""")


def nvcc_compilation(p):
    p.title("GPU 程序的编写与编译")
    p.slide("""
一个 `.cu` 文件包含两部分代码：面向 CPU 的**主机代码**与面向 GPU 的**设备代码**。
`nvcc` 将两者分离，分别调用相应的编译器，并把结果组织到一起。
""")
    arch = p.architecture(caption="一份源文件，两条编译路径", flow="down")
    arch.layer("一份 .cu 源文件", ["主机 C++ 代码", "CUDA 设备代码"])
    arch.layer("nvcc 分别调用两个编译器", ["主机编译器", "CUDA 编译工具"])
    arch.layer("产物", ["主机机器码 → CPU", "PTX / cubin → GPU"])
    p.cite(title="CUDA Compiler Driver NVCC", author="NVIDIA", venue="CUDA Toolkit Documentation",
           url="https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/index.html", key="nvcc")


# ==============================================================================
# 第 5 层 · 应用与 Agent
# ==============================================================================

def agent_loop(p):
    p.title("Agent：跨网络的请求与工具执行循环")
    p.slide("""
1. 客户端通过 HTTPS/TLS 向模型服务提交请求与上下文
2. 模型返回文本，或返回一个==结构化的工具调用请求==
3. 宿主检查权限与参数，在**本地**执行允许的工具，并将结果作为下一轮输入
""")
    p.image("assets/agent-loop.svg", width_px=470,
            caption="一种常见部署方式：本地工具运行时与云端模型服务")
    p.highlight("该循环复用了进程、网络、权限与隔离机制\nAgent 本身只是应用程序，不构成操作系统。", tone="blue")


def ai_os_challenges(p):
    p.gap(26)
    p.title("AI 负载对既有机制的复用与新的策略问题")
    p.table(
        headers=["AI 负载带来的问题", "复用的既有机制", "新出现的策略问题"],
        rows=[
            ["模型数据横跨磁盘 / DRAM / 显存", "映射、页缓存、DMA", "量化、放置、卸载"],
            ["多个请求并发生成", "进程、线程、调度", "连续批处理、准入控制"],
            ["KV Cache 的容量与复用", "分配器、局部性", "分页式 KV、淘汰策略"],
            ["Agent 在本地执行工具", "权限、隔离、审计", "最小权限、参数校验"],
        ],
        align=["left", "left", "left"],
    )
    p.highlight("机制部分将在本课程中介绍，策略部分属于当前的研究问题。", tone="green")
    p.notes("本页面向学有余力的学生，可在此提及课程项目（nano-ollama）的方向。")


# ==============================================================================
# 收束
# ==============================================================================

def request_recap(p):
    p.title("小结：一次请求在各层的执行过程")
    arch = p.architecture(flow="up", caption="各层在本次请求中承担的工作")
    arch.layer("5 · 应用", ["API 收请求", "token 流式回传"])
    arch.layer("4 · 运行时", ["解析 gguf", "张量算子", "libc 转系统调用"])
    arch.layer("3 · 操作系统", ["进程隔离", "mmap 1.9 GB", "缺页装入", "页缓存", "驱动与显存"])
    arch.layer("2 · 指令集", ["movss / mulss / addss", "PTX / cubin"])
    arch.layer("1 · 硬件", ["CPU 执行", "DRAM 存页", "GPU 算矩阵", "HBM 供带宽"])
    p.notes("第 3 层是本课程的教学重点，收尾时可再复述一遍该层的五项工作。")


def failures_between_layers(p):
    p.gap(26)
    p.title("模型的应用：从可观察现象定位系统层次")
    p.table(
        headers=["观察到的现象", "首先考察的层次", "判断依据"],
        rows=[
            ["模型无法加载", "操作系统 · 内存", "`free`、`dmesg`、设备内存占用"],
            ["首 token 延迟偏高", "操作系统 · I/O", "冷启动与热启动对比、页缓存命中情况"],
            ["GPU 利用率偏低", "运行时 · 数据移动", "批大小、拷贝耗时、依赖关系"],
            ["程序段错误", "操作系统 · 虚拟内存", "出错地址、映射与权限"],
            ["Agent 请求长时间未返回", "应用 · 网络 / 阻塞 I/O", "抓包分析、进程状态"],
        ],
        align=["left", "left", "left"],
    )
    p.highlight("诊断的第一步是确定该现象由哪一层负责。", tone="orange")


def four_themes(p):
    p.gap(52)
    p.title("贯穿本课程的四个主题")
    p.slide("""
- **抽象**：接口分离规范与实现，两侧可独立演进（ISA、系统调用、语言库）
- **表示**：整数、浮点数、字符与模型权重均编码为二进制位，含义由解释方式决定
- **资源管理**：CPU 时间、内存、带宽与能量均为有限资源，需要分配与调度
- **局部性与并发**：减少数据移动、重叠等待时间、协调共享状态
""", reveal="items")
    p.aside("后续每一章都可对照这四个主题，确定其讨论的内容。")


def course_map(p):
    p.gap(26)
    p.title("课程内容安排")
    p.table(
        headers=["", "主题", "与本讲内容的对应"],
        rows=[
            ["01", "数据表示 · 位 / 整数 / 浮点", "模型权重的存储规模，以及量化的含义"],
            ["02", "机器级程序 · x86-64 / 控制流 / 栈", "点积循环的指令实现从何而来"],
            ["03", "处理器体系结构 · 流水线 / 冒险", "一条指令的实际执行开销"],
            ["04", "存储层次 · 缓存 / 局部性", "访存带宽成为瓶颈的原因"],
            ["05", "链接与操作系统 · 进程 / 虚拟内存 / 分配器", "mmap 与页缓存的完整机制"],
        ],
        align=["center", "left", "left"],
    )
    p.highlight("实验环节使每一层的行为成为可观察、可测量的对象。", tone="blue")


def course_goal(p):
    p.gap(52)
    p.title("课程学习成果")
    p.slide("""
- **从可观察行为出发**：输出、延迟、内存占用、利用率与故障均可测量
- **追踪执行路径**：源码 → 编译 · 链接 · 装载 → 机器指令 → 硬件
- **定位负责层次**：运行时、操作系统、存储层次或处理器
- **依据证据作出判断**：以测量数据支持结论
""", reveal="items")
    p.highlight("能够说明执行 `ollama run` 时，系统各层分别承担了什么工作。", tone="orange")
