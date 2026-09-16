"""第一讲《计算机系统基础（1）》的页面内容。

组织方式：先说明大模型时代学习本课程的原因与目标，再以一次 `ollama run`
请求为观察对象，说明应用与系统的关系（系统抽象、层次化设计）。中间四个部分
按各层的目标依次介绍硬件（计算的软件化）、汇编（快速开发程序）、工具链
（消除程序绑定）与操作系统（多个程序共享硬件），每部分末尾一页列出本课程
对应的章节与系统方法。最后回到这次请求，说明学完本课程之后能够分析、定位
问题，并用本课程的方法改进程序。

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

# ==============================================================================
# 课程概览
# ==============================================================================

def staff_and_textbooks(p):
    p.title("任课教师与课程教材")
    p.slide("""
- **任课教师**：古金宇 · 陈榕
- **电子邮件**：gujinyu@sjtu.edu.cn · rongchen@sjtu.edu.cn
- **办公电话**：18818214992 · 13661816826
- **办公室**：软件大楼 3203 · 3401，答疑请提前预约
- **教材**：*Computer Systems: A Programmer's Perspective*（CS:APP 第 3 版，2016）
- **教材**：*Operating Systems: Three Easy Pieces*（OSTEP 1.10 版，2023）
""").image_right("assets/instructors.png", width_px=260)
    p.sidenote(
        "第一讲建议阅读",
        "CS:APP 第 3 版第 1 章：**§1.1**（信息就是位加上下文）、**§1.2**（编译系统）、"
        "**§1.4**（处理器如何读并执行指令）、**§1.7**（操作系统管理硬件）、**§1.8**（网络）。",
    )
    p.cite(title="Computer Systems: A Programmer's Perspective", author="Bryant & O'Hallaron",
           year="2016", venue="Prentice Hall, 3rd ed.", key="csapp")
    p.cite(title="Operating Systems: Three Easy Pieces", author="Arpaci-Dusseau & Arpaci-Dusseau",
           year="2023", venue="Version 1.10", url="https://pages.cs.wisc.edu/~remzi/OSTEP/",
           key="ostep")


def worth_taking(p):
    p.title("AI 时代学习本课程的必要性")
    p.slide("""
AI 辅助编程（vibe coding）普及之后，需要回答三个问题：
1. 是否仍有必要把自己训练成 power programmer？
2. 程序员能否在某些方面胜过大模型？
3. 本课程讲授的内容中，是否有大模型不知道的部分？
""", autobold=False)
    p.slide("""
由大模型生成的代码，程序员往往没有读懂、没有掌握，也无法为其负责。
- 这些代码的**性能**、**边界条件下的正确性**与**安全性**，仍然需要程序员来保证
- 程序员承担的责任随生成代码的数量==同步增加==
""")
    p.highlight("代码由大模型生成，对代码行为负责的仍是程序员。", tone="orange")


def coding_vs_constraints(p):
    p.title("AI 辅助编程改变的内容与无法改变的系统约束")
    p.table(
        headers=["AI 辅助编程改变的内容", "AI 辅助编程无法改变的系统约束"],
        rows=[
            ["编写代码的方式", "局部性与存储层次"],
            ["编写代码的效率", "并行与依赖"],
            ["编写代码的成本", "有限的数值精度"],
            ["编写代码的风险", "并发与一致性、隔离与故障"],
        ],
        align=["left", "left"],
    )
    p.slide("""
本课程讲授编写程序时都需要的能力，借助大模型编程时同样适用：
- **发现**代码中的问题
- **设计**最合适的解决方案
- **验证**问题确实已经解决
""")
    p.highlight("大模型降低了编写代码的成本，系统约束对生成的代码同样成立。", tone="blue")


def course_features(p):
    p.gap(52)
    p.title("课程特点：2023 年与大模型时代的对比")
    p.table(
        headers=["", "2023 年（大模型出现之前）", "大模型时代"],
        rows=[
            ["持久的概念", "课程主线", "**重要性上升**"],
            ["程序员视角", "课程主线", "**重要性下降**"],
            ["学习方式", "主动学习", "主动学习**并加以运用**"],
            ["培养目标", "少数的 power programmer", "少数的 power **system** programmer"],
        ],
        align=["right", "left", "left"],
    )
    p.slide("代码越来越多地由大模型编写，理解系统行为的能力在培养目标中的比重随之上升。")


def course_goals(p):
    p.gap(52)
    p.title("课程目标：学习本课程的原因与收获")
    p.table(
        headers=["", "大模型出现之前", "大模型时代"],
        rows=[
            ["应用程序的编写者", "程序员", "大模型完成，或辅助程序员完成"],
            ["先修课程", "程序设计（C++）", "程序设计（C++）与 AI 辅助编程"],
        ],
        align=["right", "left", "left"],
    )
    p.slide("""
本课程的目标是成为少数的 power **system** programmer。学习本课程的收获：
- 理解应用程序在系统中的执行过程，其中包括由大模型编写的程序
- 能够分析、定位和解决程序中的问题，并掌握更多改进程序的方法
""")
    p.highlight("建立程序执行的系统模型，是达到这一目标的手段。", tone="blue")


def course_questions(p):
    p.title("本课程讨论的问题：应用程序如何在系统中执行")
    p.slide("""
- 「计算」是如何由 CPU 或 GPU 完成的？
- 「数据」是如何存储到内存和外存上的？
- 在新的硬件平台上，程序的性能会是什么样？
- 如果对程序的性能不满意，可以从哪里优化？
- 为什么这次运行时程序出错了？错误可能和什么相关？
- 程序是否安全？会不会泄露我的信息？
""", autobold=False)
    p.slide("""
回答这些问题的基础，是理解程序在系统中执行的能力：
- **系统建模**：对当前所处的系统建立模型，每个系统都不完全相同
- **推断与诊断**：据此推断程序的行为，诊断程序的问题
""")


# ==============================================================================
# AI 应用仍然通过程序执行实现
# ==============================================================================

def ai_app_request(p):
    p.title("AI 应用的工作方式：一次请求")
    p.slide("""
在对话界面或 Agent 中提出问题时，客户端本身并不执行计算：它把问题与上下文
组装成一次 **HTTP 请求**发给推理服务，再把返回的 **token 流**逐段显示出来。
""", autobold=False)
    p.image("assets/agent-request.svg", width_px=980,
            caption="实线为请求路径，虚线为结果返回路径")
    p.highlight("应用中的 AI 部分由推理服务完成，推理服务的工作仍是程序执行。", tone="blue")


def agent_loop(p):
    p.title("Agent：跨网络的请求与工具执行循环")
    p.slide("""
1. 客户端通过 HTTPS/TLS 向模型服务提交请求与上下文
2. 模型返回文本，或返回一个==结构化的工具调用请求==
3. 宿主检查权限与参数，在**本地**执行允许的工具，并将结果作为下一轮输入
""")
    p.image("assets/agent-loop.svg", width_px=470,
            caption="一种常见部署方式：本地工具运行时与云端模型服务")
    p.highlight("该循环复用了进程、网络、隔离机制，Agent 本身属于应用程序。", tone="blue")


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


def service_design(p):
    p.gap(52)
    p.title("推理服务沿用传统应用程序的设计")
    p.table(
        headers=["设计", "作用"],
        rows=[
            ["**客户端 / 服务器架构**", "前后端解耦：应用程序的写法与推理在远端还是本机执行无关"],
            ["**HTTP 协议**", "兼容浏览器、命令行与各语言 SDK 等使用方式，便于开发与测试"],
            ["**标准 API**", "推理引擎可以独立优化，引擎更新不影响应用程序"],
        ],
        align=["left", "left"],
    )
    p.slide("""
推理服务本身由计算、存储与网络三部分工作组成。
设计、实现与优化 AI 应用，仍然需要了解==程序在系统中如何执行==。
""", autobold=False)
    p.highlight("本节以「使用 Ollama 完成一次推理」为例考察这一过程。", tone="blue")


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
    p.highlight("下面的每一步都可以自行在 PC 上尝试。", tone="blue")


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
输出是逐 token 出现的，命令本身不包含==任何==关于计算过程的描述。
- 权重的存放位置、使用 CPU 或 GPU、内存不足时的处理，均未指定
- 这些工作由系统在应用背后完成：计算、存储、通信、异常处理与安全隔离
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
    p.notes("""
该命令在 Linux 与 macOS 上都可以执行，三个进程的对应关系一致；
macOS 的 `comm` 列显示可执行文件的完整路径，列宽与此处的输出不同。
Windows 上本课程使用 WSL2，命令与此处完全相同；直接运行 Windows 版 ollama 时，
在 PowerShell 中用 `Get-Process ollama*` 观察同样的三个进程。
""")
    p.cite(title="Ollama", author="Ollama", venue="github.com/ollama/ollama",
           url="https://github.com/ollama/ollama", key="ollama")


def weights_are_data(p):
    p.gap(26)
    p.title("观察二：模型权重是一个 1.9 GB 的数据文件")
    p.demo("查看权重文件", """ls -lhS ~/.ollama/models/blobs | sed -n '2p'
file "$(ls -dS ~/.ollama/models/blobs/* | head -1)\"""",
           output="""-rw-r--r-- 1 ollama ollama 1.9G Sep  1 19:50 sha256-...
...sha256-...: data""")
    p.slide("""
`file` 无法识别其类型：文件内容是==一组数值参数==，不含任何可执行指令。
- 需要由某一层将其从存储设备载入内存
- 需要将矩阵运算翻译为 CPU 或 GPU 可执行的指令
- 需要将生成结果传回终端
""")
    p.highlight("模型只提供参数，而执行过程由系统栈完成。", tone="orange")
    p.notes("""
`sed -n '2p'` 跳过 `ls` 的 total 行（macOS 按 512 字节块计数），`ls -dS` 按大小排序
取出最大的 blob，两条命令在 Linux 与 macOS 上输出一致。Windows 的 WSL2 中同样如此；
Windows 版 ollama 把权重放在 `%USERPROFILE%/.ollama/models/blobs`，PowerShell 中用
`Get-ChildItem <dir> | Sort-Object Length -Descending | Select-Object -First 1` 查看。
""")


def five_layers(p):
    p.title("Ollama 请求跨越的五个系统层次")
    # Layer names are kept short enough to sit on one line in the 140px label
    # gutter: a wrapped label makes every band taller and the stack overflows
    # the slide. They also match the summary diagram in `request_recap`.
    arch = p.architecture(caption="箭头表示依赖方向：每一层只使用下一层提供的接口", flow="down")
    arch.layer("应用", ["Ollama CLI", "对话界面", "工具调用"])
    arch.layer("运行时", ["模型加载", "张量算子", "libc / libstdc++", ...])
    arch.layer("操作系统", ["进程", "虚拟内存", "文件与页缓存", "设备驱动", "调度"])
    arch.layer("指令集", ["x86-64 指令", "SIMT / PTX"])
    arch.layer("硬件", ["CPU", "DRAM", "GPU", "存储", "网络"])
    p.notes("""
每一层向上层提供确定的接口，并隐藏其实现细节。
本图在本讲出现两次：此处自上而下介绍，末尾自下而上小结。
中间四个部分按各层要达到的目标依次介绍：硬件、汇编与指令集、工具链、操作系统。
工具链（消除程序绑定）放在操作系统（多个程序共享硬件）之前，与 CS:APP 第 7、8 章的顺序一致。
""")


def system_abstraction(p):
    p.title("系统抽象：应用与系统之间的分界线")
    p.slide("""
进程、虚拟内存与文件，是系统向应用提供的抽象。抽象的作用：
- **简化开发**：应用程序通过抽象使用硬件，无需了解硬件细节
- **独立演进**：抽象保持不变时，两侧的实现可以分别修改
- **隔离问题**：一侧的故障被限制在分界线以内
""")
    p.slide("""
抽象同时隐藏了实现，程序出现问题时，定位和解决问题因此==更困难==。
大模型时代加剧了这种隐藏：代码由大模型编写，所依赖的系统也更复杂。
""", autobold=False)
    p.highlight("抽象隐藏了实现，定位问题时因此更需要了解系统。", tone="orange")


def layered_design(p):
    p.title("层次化设计：应用与系统是相对的")
    p.slide("""
相邻两层之间，上层是应用，下层是系统。理解复杂系统时，按层划分职责：
- **各层**：通过层间接口完成本层的工作
- **上层**：了解下层的行为，无需了解下层的具体实现
- **下层**：了解上层的需求，据此设计抽象、优化实现
""")
    p.table(
        headers=["上层（应用）", "层间接口", "下层（系统）"],
        rows=[
            ["Ollama CLI", "HTTP API", "Ollama 服务进程"],
            ["Ollama 服务进程", "系统调用", "操作系统"],
            ["操作系统", "指令集", "处理器"],
        ],
        align=["left", "center", "left"],
    )
    p.highlight("Ollama 服务进程对 CLI 是系统，对操作系统是应用。", tone="blue")


def ics_scope(p):
    p.gap(52)
    p.title("了解系统的程度与本课程的定位")
    p.slide("""
了解系统总是有益的，借助 AI 编程时同样如此；了解到什么程度，取决于将来在哪一层工作。
- **支持 AI**（构建系统）需要了解系统
- **使用 AI**（AI 辅助编程）同样需要了解系统，以控制开销、确认结果可靠
""")
    p.slide("""
本课程对各层做入门介绍，给出==一个典型系统==在各个方面的设计：
- **学习知识**（理解）：知识可以向 AI 询问，前提是知道问什么、怎么问
- **学习方法**（运用）：方法是本课程的目标，掌握知识是其必要前提
""")
    p.highlight("下面按硬件、汇编、工具链、操作系统的顺序介绍本课程的内容。", tone="blue")


# ==============================================================================
# 第 1 部分 · 硬件
# ==============================================================================

def stored_program(p):
    p.title("硬件的目标是计算的软件化：存储程序体系结构")
    p.slide("""
早期计算机更换程序需要==重新接线==。冯·诺依曼架构将程序与数据一同存放在内存中。
- 程序 + 数据 → CPU + 内存：处理器的执行过程统一为取指 → 译码 → 执行
- 更换计算任务即更换内存中的一段字节，计算因此**可重复**、**可编程**，无需改变硬件
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


def hardware_in_ics(p):
    p.gap(52)
    p.title("本课程中的硬件：计算、存储与通信")
    p.table(
        headers=["方面", "讨论的内容", "CS:APP"],
        rows=[
            ["**计算**", "CPU：「计算」如何实现；GPU：不同的计算需求对处理器设计的影响", "第 4 章"],
            ["**存储**", "层次化设计（内存、磁盘、HBM）；速度、容量与成本之间的取舍", "第 6 章"],
            ["**通信**", "层次化设计（系统总线、内存总线、I/O 总线）；网络", "第 6、11 章"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("""
这些内容使用的系统方法：**设计取舍**、**流水线**、**数据依赖**、**局部性**（Cache / TLB）。
""", autobold=False)


# ==============================================================================
# 第 2 部分 · 汇编与指令集
# ==============================================================================

def isa_contract(p):
    p.title("汇编的目标是快速开发程序：指令集体系结构")
    p.slide("""
汇编给出硬件的**模型**（寄存器、内存地址）与**接口**（指令集，ISA），简化程序开发。
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
- **386 引入的分页与保护**，是虚拟内存与进程隔离的硬件前提
""")
    p.aside("32 位程序能否运行，还取决于操作系统与运行库是否提供 32 位支持。")


def isa_in_ics(p):
    p.gap(52)
    p.title("本课程中的汇编：编码设计、抽象分层与向后兼容")
    p.table(
        headers=["系统方法", "本课程中的例子"],
        rows=[
            ["**编码设计**", "整数与浮点数的二进制编码、指令的编码格式"],
            ["**抽象分层**", "门电路 → 微指令 → 指令 → 函数 → 程序 → 任务"],
            ["**向后兼容**", "x86 四十余年保留前一代的指令编码"],
        ],
        align=["left", "left"],
    )
    p.slide("这部分内容对应 CS:APP 第 2 章（信息的表示与处理）与第 3 章（程序的机器级表示）。")


# ==============================================================================
# 第 3 部分 · 工具链与运行时
# ==============================================================================

def program_binding(p):
    p.gap(26)
    p.title("工具链的目标是消除程序绑定：四类绑定")
    p.table(
        headers=["绑定", "消除绑定的手段"],
        rows=[
            ["**硬件绑定**", "编程语言与应用：从汇编逐步上升到 Prompt / Agent（见下）"],
            ["**实现绑定**", "函数库、算子库"],
            ["**功能绑定**", "动态加载库、解释执行、系统框架、推理引擎"],
            ["**性能绑定**", "性能分析、问题定位、优化方法"],
        ],
        align=["left", "left"],
    )
    p.slide("""
消除硬件绑定的过程：
1. **汇编**：绑定处理器架构
2. **低级语言**（C）：便于学习系统
3. **高级语言**：可移植、高效、安全
4. **应用**（Excel、数据库、Photoshop）→ **Prompt / Agent**：智能化
""", autobold=False)


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
- 指针值即进程虚拟地址空间中的地址，可用 `%p` 直接输出
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
    p.aside("路径 ② 执行的机器指令，由编译器从 C 语句翻译得到。")


def machine_code(p):
    p.gap(26)
    p.title("点积循环的一种 x86-64 指令实现")
    p.code("text", DISASSEMBLY)
    p.slide("""
以下六条指令对应 `mini_ollama.c` 中 `infer()` 的完整循环：
- `movss` / `mulss` / `addss`：装载、相乘、累加，构成**乘加运算的基本形式**
- `cmpq` / `jne`：实现循环条件判断与控制转移；机器层面不存在 `for` 这一结构
""")
    p.aside("具体生成的指令取决于编译器、优化选项、ISA 扩展与目标处理器。")
    p.notes("""
在 arm64 的 Mac 上编译同一段代码，得到的是 AArch64 指令：装载用 `ldr`，
乘加用 `fmul` / `fadd`（或合并为 `fmadd`），循环用 `cmp` 与 `b.ne`。
指令名称随指令集变化，此处的六步结构保持一致。本课程的机器级程序部分使用 x86-64。
x86 的 Windows 上指令集与此处相同，编译得到的指令序列一致；调用约定为 Microsoft x64，
参数所用的寄存器与 System V 的约定不同，该差别在第 2 部分说明。
""")


def runtime_libraries(p):
    p.title("语言运行库：应用与系统调用之间的一层")
    p.slide("""
`ldd` 列出一个可执行文件在启动时需要装载的**共享库**，以及每个库在文件系统中的实际路径。
""", autobold=False)
    p.demo("列出动态链接库", """cd examples && g++ cpp_demo.cpp -o cpp_demo
ldd ./cpp_demo 2>/dev/null || otool -L ./cpp_demo    # Linux: ldd; macOS: otool -L""",
           output="""libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f...)
libc.so.6 => /usr/lib/x86_64-linux-gnu/libc.so.6 (0x00007f...)""",
           files=["examples/cpp_demo.cpp"])
    p.slide("""
- 这两行说明 `cpp_demo` 的机器码==并不完整==：`printf` 等函数的实现在库文件中，装载时才补齐
- `printf` 由 **libc** 实现，`std::cout` 由 **libstdc++** 实现，后者最终调用前者提供的底层接口
- 库对输出进行**缓冲**：多次 `printf` 可能只产生一次 `write` 系统调用，两者不是一一对应
- macOS 上同一程序列出的是 `libc++.1.dylib` 与 `libSystem.B.dylib`，层次关系相同
""")
    p.highlight("共享库是装载进进程地址空间的、已编译的机器码。", tone="blue")
    p.notes("""
`ldd` 的输出即动态链接器在启动时要完成的工作清单。可在课堂上对同一个程序
分别执行 `ldd` 与 `gcc -static` 后的 `ldd`，说明静态链接与动态链接的差别。
Windows 的 WSL2 中命令与此处相同；直接在 Windows 上编译时共享库是 DLL，
MSYS2 中同样有 `ldd`，原生工具链用 `objdump -p` 或 `dumpbin /dependents`，
列出的是 `libstdc++-6.dll`、`msvcrt.dll` 与 `KERNEL32.dll`。
""")


def python_and_pytorch(p):
    p.title("解释执行的程序同样由机器指令完成")
    p.slide("""
- Python 源码由**解释器**执行；解释器本身是编译好的原生可执行文件（ELF、Mach-O 或 PE）
- 在 PyTorch 中，Python 负责==组织==计算过程：构建模型并分派算子
- 算子的实际执行位于编译好的原生库中：`libtorch_cpu.so` / `libtorch_cuda.so`
""")
    p.image("assets/interpreter-path.svg", width_px=640)
    p.demo("查看 Python 解释器与 Torch 算子库", """file "$(command -v python3)"
ls "$(python3 -c 'import torch;print(torch.__path__[0])')"/lib | grep -E 'libtorch_(cpu|cuda)'""",
           output="""python3: ELF 64-bit LSB pie executable, x86-64, ...
libtorch_cpu.so
libtorch_cuda.so""")
    p.highlight("解释执行改变的是计算的组织方式，而指令仍由处理器执行。", tone="orange")
    p.notes("""
两条命令在 Linux 与 macOS 上都可以执行。macOS 上 `file` 输出的是
Mach-O arm64 可执行文件，算子库为 `libtorch_cpu.dylib`，其中没有 CUDA 版本。
Windows 的 WSL2 中与 Linux 一致；Windows 版 Python 中没有 `file`，
用 `Get-Command python` 定位解释器，算子库位于 `torch/lib` 下，
名为 `torch_cpu.dll` 与 `torch_cuda.dll`，没有 `lib` 前缀。
""")
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


def toolchain_in_ics(p):
    p.gap(52)
    p.title("本课程中的工具链：模块化、语言设计与链接")
    p.table(
        headers=["系统方法", "本课程中的例子"],
        rows=[
            ["**模块化**", "分别编译的目标文件、函数库与算子库"],
            ["**编程语言设计**", "表达能力与易用性之间的取舍"],
            ["**灵活性**", "链接：静态链接与动态加载"],
            ["**问题分析**", "性能分析与问题定位"],
        ],
        align=["left", "left"],
    )
    p.slide("这部分内容对应 CS:APP 第 7 章（链接）与第 5 章（优化程序性能）。")


# ==============================================================================
# 第 4 部分 · 操作系统
# ==============================================================================

def why_os(p):
    p.gap(52)
    p.title("操作系统的目标是多个程序共享硬件")
    p.slide("浏览器、编辑器与 ollama 同时请求同一组处理器与内存资源，需要**调度**、**并发**与**隔离**机制。")
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
    p.slide("这些抽象长期保持不变，它们的实现随硬件与需求持续变化。")
    p.notes("""
本部分按这四类抽象依次举例：进程隔离 → 模型加载（mmap）→ 设备访问 → 调度。
每一页均以同一个 ollama 实例为例，保持例子一致。
""")


def exclusive_use(p):
    p.gap(52)
    p.title("操作系统的核心思想：每个程序独占使用硬件")
    p.table(
        headers=["资源", "独占使用的实现方式", "程序使用的抽象"],
        rows=[
            ["CPU", "时间片：内核轮流调度，切换时保存与恢复上下文", "进程"],
            ["内存", "虚拟地址：每个进程拥有独立的地址空间", "虚拟内存"],
            ["设备", "输入输出模式：统一的 `open` / `read` / `write`", "文件"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("""
- 程序按独占硬件的方式编写，**容易开发**；资源的实际分配由操作系统掌握，**容易监管**
- **通用性**：操作系统以公共服务的形式提供这些抽象，设计上==机制与策略分离==
""")


def process_isolation(p):
    p.title("进程：执行与保护的基本单位")
    p.slide("""
前面 `ps` 输出中的 1832 / 1904 / 2077，各自独占一份执行环境：
- 独立的**虚拟地址空间**：同一虚拟地址 `0x7fff...` 在不同进程中映射到不同的物理内存
- 独立的**打开文件表**与寄存器**上下文**
- 可被内核==抢占==的执行流
""")
    p.sidenote(
        "进程隔离限制故障传播范围",
        "`ollama-runner` 因段错误退出时，`ollama serve` 收到子进程结束通知，"
        "重新创建推理进程即可继续服务。**隔离的首要作用是把故障限制在一个进程之内；"
        "在此基础上，它同时构成安全边界。**",
    )
    p.highlight("进程是操作系统为一个运行中的程序划定的资源与保护边界。", tone="blue")


def mmap_in_practice(p):
    p.title("操作系统影响应用性能：llama.cpp 的加载改动")
    p.slide("""
2023 年 llama.cpp 把 1.9 GB 权重的加载从「`malloc` 缓冲区再 `read` 整个文件」改为 `mmap`：文件映射进进程的地址空间，访问到哪一部分才装入哪一部分。模型与算法均未变化。
""", autobold=False)
    p.image("assets/llama-cpp-mmap-pr.png", width_px=680, framed=True,
            caption="标题与三条说明都描述同一次改动的效果：加载更快、可加载的模型更大、"
                    "多个推理进程可并行")
    p.highlight("这一量级的差别由所使用的操作系统机制决定。", tone="orange")
    p.cite(title="Make loading weights 10-100x faster", author="Justine Tunney",
           year="2023", venue="ggml-org/llama.cpp PR #613",
           url="https://github.com/ggml-org/llama.cpp/pull/613",
           key="llama-mmap-pr")
    p.notes("""
`read` 的做法需要一块与文件同样大的缓冲区，每个字节经过两次拷贝（磁盘 → 内核缓冲区 → 用户缓冲区）；
`mmap` 建立映射时不拷贝数据，访问映射区时由缺页处理装入对应的页。
该提交同时给出三点效果：加载显著加快、可加载的模型规模提高、多个推理进程可共享同一份页缓存。
三点都由同一个原因得到：权重页由内核的页缓存直接提供，不再复制到用户缓冲区。
映射、页表与缺页处理在虚拟内存一章展开。
ollama 使用 llama.cpp 作为推理后端，沿用了这一加载方式。
""")


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
存储设备 → 内存 → 设备内存。
""")
    p.highlight("跨层数据移动的开销，是首 token 延迟的主要来源之一。", tone="orange")


def scheduling(p):
    p.gap(26)
    p.title("并发请求下的调度：机制与策略")
    p.slide("""
服务进程可能同时处理多个请求。在处理器核数有限的条件下，操作系统提供两项机制：
- **抢占式调度**：限制单个执行流连续占用处理器的时间
- **阻塞式等待**：等待磁盘、GPU 或网络的进程让出处理器
""")
    p.slide("""
推理服务在此之上实现自身的调度策略与优化：
- **批处理**：连续批处理（continuous batching）合并多个请求的计算
- **负载均衡**：把请求分配到多个推理进程或多块 GPU
- 请求排队与准入控制
""")
    p.highlight("操作系统决定程序能做什么，应用决定什么时候做、怎么做。", tone="blue")


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


def os_in_ics(p):
    p.title("本课程中的操作系统：进程、虚拟内存、文件与并发")
    p.table(
        headers=["系统方法", "本课程中的例子", "CS:APP"],
        rows=[
            ["**并发**", "进程切换时保存与恢复上下文", "第 8 章"],
            ["**软硬件协同**", "页表由操作系统维护，由 MMU 查询", "第 9 章"],
            ["**分页与按需加载**", "以固定大小的页管理内存以减少碎片，缺页时装入数据", "第 9 章"],
            ["**共享**", "页缓存、共享库、打开文件的引用计数", "第 9、10 章"],
            ["**隔离**", "独立的地址空间与权限检查", "第 8、9 章"],
            ["**有状态与无状态**", "文件偏移量由内核记录；HTTP 请求不依赖之前的请求", "第 10、11 章"],
            ["**并行**", "线程、同步与并发编程", "第 12 章"],
        ],
        align=["left", "left", "left"],
    )
    p.slide("机制与策略分离贯穿上述设计：操作系统提供机制，应用决定策略。")


# ==============================================================================
# 回到例子：一次模型推理
# ==============================================================================

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
    arch.layer("应用", ["API 收请求", "token 流式回传"])
    arch.layer("运行时", ["解析 gguf", "张量算子", "libc 转系统调用"])
    arch.layer("操作系统", ["进程隔离", "mmap 1.9 GB", "缺页装入", "页缓存", "驱动与显存"])
    arch.layer("指令集", ["movss / mulss / addss", "PTX / cubin"])
    arch.layer("硬件", ["CPU 执行", "DRAM 存页", "GPU 算矩阵", "HBM 供带宽"])
    p.notes("操作系统是本讲篇幅最长的部分，收尾时可再复述一遍该层的五项工作。")


def failures_between_layers(p):
    p.gap(26)
    p.title("分析、定位与解决问题：从可观察现象定位系统层次")
    p.table(
        headers=["观察到的现象", "首先考察的层次", "判断依据"],
        rows=[
            ["模型无法加载", "操作系统 · 内存", "`free` / `vm_stat`、内核日志、设备内存占用"],
            ["首 token 延迟偏高", "操作系统 · I/O", "冷启动与热启动对比、页缓存命中情况"],
            ["GPU 利用率偏低", "运行时 · 数据移动", "批大小、拷贝耗时、依赖关系"],
            ["程序段错误", "操作系统 · 虚拟内存", "出错地址、映射与权限"],
            ["Agent 请求长时间未返回", "应用 · 网络 / 阻塞 I/O", "抓包分析、进程状态"],
        ],
        align=["left", "left", "left"],
    )
    p.highlight("诊断的第一步是确定该现象由哪一层负责。", tone="orange")
    p.notes("""
表中的命令按平台取其一：内存用量在 Linux 上用 `free`，macOS 上用 `vm_stat`，
Windows 上用任务管理器或 `Get-Counter` 的内存计数器；内核日志在 Windows 上是事件查看器。
判断依据一列所指的现象与层次，在三个平台上相同。
""")


def ai_infra_from_ics(p):
    p.gap(26)
    p.title("改进程序的方法：AI 基础设施中的本课程内容")
    p.table(
        headers=["AI 基础设施中的技术", "对应的本课程内容", "CS:APP"],
        rows=[
            ["数据并行、张量并行、流水线并行（DP / TP / PP）", "程序性能优化与并行", "第 5 章"],
            ["流水线并行中的气泡与依赖", "处理器流水线中的气泡与数据依赖", "第 4 章"],
            ["KV Cache", "缓存与局部性", "第 6 章"],
            ["PagedAttention", "分页式虚拟内存", "第 9 章"],
            ["张量的数值格式（FP4 至 FP64）", "浮点数的表示", "第 2 章"],
            ["Prompt 注入", "缓冲区溢出与代码注入攻击", "§3.10"],
        ],
        align=["left", "left", "center"],
    )
    p.highlight("本课程讲授的系统方法，同样用于设计与改进 AI 基础设施。", tone="green")
    p.cite(title="Efficient Memory Management for Large Language Model Serving with PagedAttention",
           author="Kwon et al.", year="2023", venue="SOSP", url="https://arxiv.org/abs/2309.06180",
           key="pagedattention")


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
