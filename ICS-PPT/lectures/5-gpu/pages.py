"""5-gpu 的页面，顺序与 lecture.py 中页面的顺序相同。

四个部分：GPU 的结构，CUDA 编程，线程块与 Warp 的执行，框架中的算子调用。
每个部分以一段注释开头，写明它的起点问题、给出的回答和页序。每个页面函数的注释写明它怎样承接上一页。
贯穿前三部分的程序是向量内积 dot：第二部分写出，第三部分分析它的执行。

插图的来源：
- 原 PPT 的数据图与照片：teapot-*、ue5.gif、h100-sm；
- 由 diagrams/ 生成：chip-area、alu-sharing、render-pipeline、gpu-overview、sm-structure、
  dgpu-igpu、peak-trend、mem-hierarchy、thread-index、host-device-1..6、warp-1..4、race-1..5、fold-1..7、block-to-sm、warp-switch、
  divergence-1..5、pytorch-layers、custom-op-flow。
"""

from lecturekit.demo import project_root
from lecturekit.env import demo_environment


def slide(p, md, **kw):
    """页面文案。粗体在文案里逐处写明，因此关闭自动加粗。"""
    return p.slide(md, autobold=False, **kw)


def columns(p, *md, **kw):
    return p.columns(*md, autobold=False, **kw)


def figure(p, name, width, ext="svg"):
    return p.image(f"assets/{name}.{ext}", width_px=width)


NVIDIA_GUIDE = "图：NVIDIA CUDA C++ Programming Guide"
FERMI_REDRAWN = "按 NVIDIA Fermi Compute Architecture 白皮书的图重画"

SERVER = "https://bomb.hydroiodic.site:18433"
# 提交页的口令取自环境变量 GPU_TOKEN，没有时取项目根目录的 .env，链接带 ?token= 打开页面。
TOKEN = demo_environment(project_root()).get("GPU_TOKEN", "")
SUBMIT_URL = f"{SERVER}/submit" + (f"?token={TOKEN}" if TOKEN else "")

# 提交一个文件到课程服务器的命令。执行时在后面接 TRIM，幻灯片上不显示（`show=`）：TRIM 去掉服务器的 # 行和第一个空行之后的全部内容，只留下程序的输出。
SUBMIT = 'curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @{} $GPU_SERVER/program'
TRIM = " | sed '/^#/d;/^$/,$d'"


# 演示程序的输出，是把 examples/ 下的文件提交到课程服务器得到的回复（见 README.md）。页面上的文字引用这些数字。
DIVERGE_OUTPUT = """n = 4194304 threads, 1024 multiply-adds per thread
kernel          branch on              time(ms)
branch_by_lane  threadIdx.x % 2           0.903
branch_by_warp  threadIdx.x / 32 % 2      0.459
ratio 1.97"""
DIVERGE_BOLD = [3, 4, 5]
DIVERGE_TEXT = r"""
- 两个核函数各有一半线程执行第一条路径、一半执行第二条路径，两条路径各含 1024 次相依的乘加，有用的工作量相同。
- 按 `threadIdx.x % 2` 划分时，每个 Warp 内两条路径都出现，Warp 依次执行两条路径，耗时 0.903 ms。按 `threadIdx.x / 32 % 2` 划分时，同一 Warp 的 32 个线程走同一条路径，耗时 0.459 ms。耗时之比为 1.97。
"""
DIVERGE_NOTES = """
`diverge.cu` 中 `branch_by_lane` 与 `branch_by_warp` 调用相同的两个设备函数：`branch_one` 执行 1024 次 `v = v * 0.999f + 0.001f`，`branch_two` 执行 1024 次 `v = v * v - 0.25f`。两个核函数都是一半线程走第一条路径、一半线程走第二条路径，区别在于划分的依据。
图中每个 Warp 一行，上方是 32 个线程各自所在的分支，蓝色为第一条路径、橙色为第二条路径，下方的条按同一时间尺度画出 Warp 执行路径的先后：左图两条路径依次执行，右图每个 Warp 只执行自己的一条，两个 Warp 同时进行。
前者每个 Warp 的 32 个线程中，偶数号走第一条、奇数号走第二条，Warp 先执行第一条路径（奇数号线程被屏蔽），再执行第二条路径（偶数号线程被屏蔽），每个 Warp 的执行时间是两条路径之和。后者整个 Warp 只执行自己的那一条路径，时间是一条路径的长度。
两个核函数的启动配置与总线程数相同，耗时之比约为 2。连续运行三次，耗时的差别在 2% 以内。
"""

DOT_OUTPUT = """blocks  65536 x 256 threads
sum     41943040
check   41943040
GPU     0.531 ms  253.0 GB/s
CPU     7.529 ms  17.8 GB/s
speedup 14.2x"""
DOT_BOLD = [4, 5, 6]
DOT_NOTES = """
`GPU` 行是 `dot_kernel` 单独的执行时间（CUDA 事件计时，20 次启动的平均值）与读取 `w`、`x` 两个数组共 128 MB 的带宽：253.0 GB/s，为该 GPU 峰值显存带宽 448 GB/s 的 56%。
`CPU` 行是同一个内积在一个 CPU 核上用循环计算一遍的时间（`clock_gettime` 计时），带宽 17.8 GB/s。两者读取的数据量相同，耗时之比为 14.2 倍。
GPU 的时间不含两个数组从内存复制到显存的时间，也不含部分和复制回内存并累加的时间。
"""

AXPY_SKELETON_OUTPUT = """blocks  39063 x 256 threads
y[n-1]  1.0
differ  9900000"""


def outline(p):
    p.title("课程提纲")
    slide(p, r"""
1. **GPU 的结构**：3D 渲染的计算特征，ALU 共用控制逻辑，流式多处理器，显存与存储层次。
2. **CUDA 编程**：主机与设备，三级线程组织，核函数的编写、启动与块内协作。
3. **线程块与 Warp 的执行**：线程块在 SM 上的驻留，Warp 调度与延迟隐藏，分支分化。
4. **框架中的算子调用**：PyTorch 的分层与 Dispatcher，自定义算子的登记与 Python 调用。
""")
    p.notes("""
本讲分为四个部分。第一部分说明 GPU 的硬件结构，包括 SM 与存储层次。第二部分介绍 CUDA 编程模型与基本语法，并写出向量加法与向量内积。第三部分分析核函数在 SM 上的执行。第四部分以 PyTorch 为例，说明一次矩阵乘法调用怎样经过 Dispatcher 分派到 CUDA 核函数，再把一个核函数登记为自定义算子并在 Python 中调用。
第一至第三部分以练习结束。第一部分在练习之前补充介绍 Tensor Core。
本讲中的所有示例程序均位于 examples/ 目录下，可提交到课程服务器编译运行。
""")


# 第一部分：GPU 的结构。
#
# 起点问题：交互式渲染每帧有数百万个三角形与像素，每秒 60 至 120 帧，每个元素执行同一段着色器，
# 这类计算由什么硬件执行。
# 回答：让一组 ALU 共用一套取指与译码，芯片面积主要分配给运算单元。许多个这样的单元（SM）组成 GPU。
# 线程以 32 个为一组（Warp）在 SM 上执行。SM 内有寄存器与共享内存，SM 外有 L2 与显存，
# 独立 GPU 经 PCIe 与 CPU 相连。
# 页序：渲染的规模（问题）→ 着色器的并行性 → ALU 共用控制逻辑（解法）→ 整卡的 SM → SM 的部件 → Warp →
# 显存与 PCIe → 存储层次 → 峰值算力（结果）→ sidebar：Tensor Core。

# 第一页：问题的起点，一帧图像由什么计算得到。
def render_task(p):
    p.title("3D 渲染")
    slide(p, r"""
- **3D 渲染**以三维场景的描述作为输入：
  - 物体表面的几何形状（如三角网格）；
  - 表面材质、光源位置与视角参数。
- 渲染的输出是该场景对应的二维图像，核心计算是确定每个三角形对图像中各个像素颜色的贡献。
""")
    (p.row(caption="茶壶的三角网格与渲染结果")
      .image("assets/teapot-mesh.png", height_px=200)
      .image("assets/teapot-render.png", height_px=200)
      .footnote("图：Henrik Wann Jensen"))
    p.notes("""
GPU（Graphics Processing Unit，图形处理器）最初专为 3D 图形渲染而设计。
三维物体的表面通常表示为由大量微小三角形拼接而成的三角网格（triangle mesh），左图展示了图形学中经典的犹他茶壶网格模型。
渲染过程根据每个三角形的空间位置与朝向、表面材质属性以及光源位置计算像素颜色，右图即为左图网格施加材质与光照计算后生成的图像。
""")


# 上一页给出一帧图像的计算内容。本页给出它的规模：实时渲染每秒要完成多少次。
def render_realtime(p):
    p.title("实时渲染")
    p.image("assets/ue5.gif", width_px=900, caption="Unreal Engine 5 实时渲染画面").footnote(
        "动图：Epic Games，Unreal Engine 5 演示")
    p.notes("""
交互式应用与电子游戏要求实时渲染：系统每秒需要生成 60 至 120 帧图像，每一帧都必须根据当前的摄像机视角重新计算整个场景。
图中的复杂场景包含数百万个三角形，图形处理器需要在十几个毫秒内完成全部三角形与数百万像素的计算。
""")


# 上一页给出规模。本页给出每个元素上执行的程序及其并行性：同一段着色器，元素之间相互独立。
def render_steps(p):
    p.title("渲染管线与着色器")
    p.side_image("assets/render-pipeline.svg", alt="contain", width="46%")
    slide(p, r"""
- **着色器**（Shader）是在渲染管线中对每个图元独立执行的程序：
  - **顶点着色器**：对每个顶点执行一次坐标变换；
  - **几何着色器**：对每个三角形执行一次几何处理；
  - **片元着色器**：对每个片元执行一次光照与颜色计算。
- 现代 3D 场景每帧包含**数百万个三角形**，以每秒 120 帧运行时，每秒需处理 $10^8$ 至 $10^9$ 个三角形，超出了通用 CPU 实时处理的能力。
""")
    p.notes("""
图中展示了图形渲染管线的主要阶段。顶点着色器通过 4 × 4 矩阵与向量的乘法，将每个顶点从三维世界坐标变换至屏幕坐标。几何着色器以三角形为单位处理图元。光栅化阶段（Rasterization）确定每个三角形覆盖的像素位置并生成片元（Fragment）。片元着色器计算每个片元的颜色，最后经深度测试与颜色混合（Tests and Blending）输出最终图像。
三个着色器阶段可由开发者编写程序定义计算逻辑。
在每一个可编程阶段中，所有顶点、三角形或片元均执行同一段着色器代码，且各元素之间的计算相互独立，具备高度的数据并行性。
""")


# 上一页的计算由大量独立元素组成，每个元素的程序相同。本页给出执行它的硬件：把芯片面积分给运算单元。
def gpu_cores(p):
    p.title("CPU 与 GPU 的硬件结构")
    (p.row()
      .image("assets/chip-area.svg", height_px=250)
      .image("assets/alu-sharing.svg", height_px=250)
      .footnote(NVIDIA_GUIDE))
    slide(p, r"""
- CPU 将较多芯片面积用于复杂的控制逻辑与多级高速缓存。GPU 将芯片面积主要用于运算单元，一组算术逻辑单元（ALU）共用同一套控制逻辑与缓存。
- 一条指令经一次取指和译码后，可驱动多个 ALU 同时执行：CPU 的向量指令通常驱动 4 至 8 个 ALU，而 GPU 可驱动数十个 ALU，从而在相同面积内容纳更多运算单元。
""")
    p.notes("""
左图对比了 CPU 与 GPU 的芯片面积分配：CPU 的每个核心配备独立的控制逻辑（如分支预测与乱序执行部件）和大容量多级缓存，以降低单线程的执行延迟。GPU 则将芯片面积集中用于高密度的运算单元阵列。
右图对比了三种执行方式：左侧为标量 CPU 核心，每次取指、译码仅驱动一个 ALU。中间为支持向量指令（SIMD）的 CPU 核心，一次取指、译码驱动多个 ALU 处理一组数据。右侧为 GPU，由一整组 ALU 共用取指与译码部件。
通过分摊取指与译码的硬件开销，GPU 得以在单颗芯片上集成数千个运算单元。
""")


# 上一页给出 CPU 与 GPU 把芯片面积分给不同的部件。本页给出 GPU 在系统中的位置：独立 GPU 的显存经 PCIe 与 CPU 相连，集成 GPU 与 CPU 共用内存。
def dgpu_igpu(p):
    p.title("独立 GPU 与集成 GPU")
    columns(p, r"""
**独立 GPU**（dGPU）
- 常见于台式机与计算服务器
- 独立的处理器芯片与专用高带宽显存（VRAM）
- 通过 PCIe 总线与 CPU 及主机内存相连
- 峰值算力与显存带宽更高
""", r"""
**集成 GPU**（iGPU）
- 常见于笔记本电脑与移动终端
- 与 CPU 核心集成在同一颗芯片封装内
- 与 CPU 共享物理系统内存
- 功耗更低，数据不经 PCIe 复制
""")
    (p.row()
      .image("assets/dgpu-rtx5090.jpg", height_px=170)
      .image("assets/dgpu-igpu.svg", width_px=520)
      .image("assets/igpu-lunarlake.jpg", height_px=170)
      .footnote("照片：Palit GeForce RTX 5090 GameRock（PantheraLeo1359531，CC BY 4.0）与 Intel Core Ultra 200V（FritzchensFritz，CC0），来自 Wikimedia Commons"))
    p.notes("""
左侧照片为独立显卡 RTX 5090，右侧照片为集成 GPU 所在的芯片封装 Intel Core Ultra 200V：计算芯片与两颗内存芯片封装在同一块基板上，CPU 核心与 GPU 核心位于同一颗计算芯片内。
图中连线的宽度与带宽成正比：PCIe 3.0 x16 为 16 GB/s，双通道 DDR4-3200 为 51.2 GB/s，GDDR6 显存为 448 GB/s。
左侧为独立 GPU 的系统结构：GPU 拥有独立的显存颗粒，通过 PCIe 总线与主机通信。右侧为集成 GPU 的系统结构，CPU 与 GPU 共享同一片系统内存。
在独立 GPU 上运行 CUDA 程序时，主机端程序需要先调用 `cudaMemcpy` 将输入数据经 PCIe 总线复制到独立显存中，设备端的核函数才能直接访问。PCIe 的带宽远低于显存带宽，数据在主机与显存之间的复制因此成为程序耗时的组成部分。
""")


# 上一页给出 GPU 在系统中的位置。本页给出 GPU 芯片自身的结构：由共用控制逻辑的 ALU 组（SM）组成。
def gpu_sm(p):
    p.title("流式多处理器（SM）")
    slide(p, r"""
- NVIDIA GPU 的核心计算部件是**流式多处理器**（SM，Streaming Multiprocessor）。图中的 Fermi 架构 GPU 集成 16 个 SM，所有 SM 共用片上的 L2 高速缓存。
- 数据中心级 **H100** GPU 包含 **132 个 SM**，消费级旗舰 **RTX 5090** GPU 包含 **170 个 SM**。
""")
    figure(p, "gpu-overview", 900).footnote(FERMI_REDRAWN)
    p.notes("""
Fermi 是 NVIDIA 于 2010 年发布的 GPU 架构。图中上下两排各排列 8 个 SM，中央为所有 SM 共用的 L2 高速缓存，两侧为 DRAM 接口，顶部为主机接口（PCIe）与线程块调度器（GigaThread）。
不同代际或同代不同型号的 GPU 均采用这种可扩展的模块化设计，型号之间的规模差异主要体现在 SM 的数量上。
""")


# 上一页给出整卡由 SM 组成。本页给出一个 SM 的内部部件。
def sm(p):
    p.title("流式多处理器（SM）")
    p.side_image("assets/sm-structure.svg", alt="contain", width="44%", valign="middle").footnote(
        FERMI_REDRAWN)
    slide(p, r"""
- 每个 SM 包含：
  - **CUDA 核心**：执行整数与浮点运算；
  - **加载/存储单元**（LD/ST）与**特殊功能单元**（SFU）；
  - **Warp 调度器**、寄存器堆、共享内存 / L1 高速缓存。
""")
    p.notes("""
图中展示了 Fermi 架构中单个 SM 的内部结构：顶部为指令缓存与两个 Warp 调度器，中部为容量 32768 个 32 位寄存器的寄存器堆，下方排列 32 个 CUDA 核心、16 个加载/存储单元与 4 个特殊功能单元，底部为 64 KB 的共享内存与 L1 高速缓存。Fermi 架构的单个 SM 含 32 个 CUDA 核心，而近年来的 Ampere、Hopper 与 Blackwell 架构（如 H100 与 RTX 5090）单个 SM 均包含 128 个 FP32 CUDA 核心。
特殊功能单元（SFU）计算超越函数与倒数平方根等。
每个线程拥有独立的寄存器（分配在 SM 的寄存器堆中），同一线程块的线程可以通过共享内存交换数据。
""")


# 上一页给出 SM 的内部部件。本页给出线程怎样使用这些部件：线程块整体分配给一个 SM，线程块中的线程每 32 个组成一个 Warp。
def simt(p):
    p.title("Warp 与 SIMT")
    figure(p, "gpu-simt", 900).footnote(FERMI_REDRAWN)
    slide(p, r"""
- **核函数**（Kernel）由 GPU 上的许多线程各执行一次。这些线程组成 **Grid**，Grid 划分为若干**线程块**，一个线程块整体分配给一个 SM。
- 线程块中每连续 **32 个**线程组成一个 **Warp**（线程束），同一 Warp 的线程在每个周期执行同一条指令（**SIMT**）。
""")
    p.notes("""
图左侧为线程的层次：一次核函数启动产生一个 Grid，Grid 由若干线程块组成，每个线程块（图中为 256 个线程）划分为 8 个 Warp，每个 Warp 含 32 个线程。图右侧为硬件：线程块调度器把整个线程块分配给一个 SM，线程块的全部线程在这个 SM 上一直执行到结束。
CUDA 核心本身不包含独立的取指与译码部件。Warp 调度器每次取指、译码一条指令，并发往一组 CUDA 核心同时执行，每个 CUDA 核心处理一个线程的数据，这种执行模式即单指令多线程（SIMT）。
图中的 SM 取自 Fermi：有两个 Warp 调度器，各管一组 16 个 CUDA 核心。每个调度器选出一个就绪的 Warp，把它的下一条指令发往自己的那组核心，因此同一时刻这个 SM 上有两个 Warp 各在执行一条指令。一个 Warp 有 32 个线程而一组只有 16 个核心，所以这条指令分两批完成，每批 16 个线程。
""")


# 上一页给出 Warp 与 SIMT。本页给出程序可见的存储：每一级归谁使用，位于芯片的哪里。
def mem_hierarchy(p):
    p.title("GPU 存储层次")
    figure(p, "mem-hierarchy", 520)
    slide(p, r"""
- **寄存器**（Registers）：每个线程私有，访问速度最快。**共享内存**（Shared Memory）：每个线程块拥有一份，位于 SM 片上。**全局内存**（Global Memory）：位于片外显存，供整个 Grid 的所有线程共享读写。
- **本地内存**（Local Memory）：逻辑上每个线程私有，物理上位于片外显存，用于存放寄存器容纳不下的局部变量或数组。L1 与 L2 高速缓存由硬件自动管理。
""")
    p.notes("""
左侧为软件线程层次（线程、线程块、Grid）各自对应的存储空间。右侧为各级存储在硬件上的物理位置：寄存器堆、共享内存与 L1 数据缓存位于 SM 芯片内部，L2 高速缓存由所有 SM 共享，而全局显存（GPU DRAM）位于芯片外部。
在现代 NVIDIA GPU 中，共享内存与 L1 数据缓存共用同一块片上物理 SRAM，二者的容量配比可由程序按需配置（例如 RTX 5090 每个 SM 配备 128 KB 片上存储，H100 每个 SM 配备 256 KB 片上存储）。
""")


# 前几页给出结构。本页给出结构带来的结果：峰值浮点算力比同期 CPU 高一个数量级，因此这类硬件被用于图形之外的计算。
def flops_trend(p):
    p.title("峰值算力与通用 GPU")
    p.side_image("assets/peak-trend.svg", alt="contain", width="50%", valign="middle").footnote(
        "数据：NVIDIA 与 Intel 的公开规格，峰值算力为计算值")
    slide(p, r"""
- 2025 年的 GeForce RTX 5090 单精度峰值算力约为 104.8 TFLOP/s，2024 年的 Intel Core Ultra 9 285K 约为 2.6 TFLOP/s，前者约为后者的 41 倍。
- **通用 GPU 计算**（GPGPU）：利用 GPU 的大规模并行算力执行图形渲染以外的计算任务，如神经网络训练与推理、科学数值模拟等。
""")
    (p.row()
      .image("assets/gpgpu-opencl.png", caption="*OpenCL*")
      .image("assets/gpgpu-lerobot.png", caption="*LeRobot*")
      .image("assets/gpgpu-ggml.png", caption="*ggml*")
      .footnote("图标：Wikimedia Commons（OpenCL），GitHub 仓库（LeRobot、ggml）"))
    p.notes("""
图中纵轴表示处理器的理论峰值单精度浮点算力，单位为 GFLOP/s（每秒十亿次浮点运算），采用对数坐标。橙色折线为各代 GeForce 旗舰 GPU，从 2006 年的 8800 GTX 到 2025 年的 RTX 5090。蓝色折线为各代 Intel Core 旗舰 CPU（主流桌面平台），从 2006 年的 Core 2 Extreme QX6700 到 2024 年的 Core Ultra 9 285K。
处理器的理论峰值算力由「运算核心数 × 时钟频率 × 单核每周期浮点运算次数」决定。GPU 的数值为 CUDA 核心数 × 2 次运算（一次乘加）× 加速频率，RTX 5090 为 21760 × 2 × 2.407 GHz = 104.8 TFLOP/s。CPU 的数值为核心数 × 基础频率 × 每周期单精度运算次数，数值按基础频率计算。每个核心每周期的运算次数随指令集加宽：SSE 为 8，AVX 为 16，AVX2 与两个 FMA 单元为 32。Core Ultra 9 285K 有 8 个 P 核与 16 个 E 核，P 核与 E 核每周期均为 32 次，基础频率分别为 3.7 GHz 与 3.2 GHz，合计 2.6 TFLOP/s。
自 2006 年起，GPU 的峰值算力增长约 300 倍，CPU 约 30 倍，两者的差距由 4 倍扩大到 41 倍。CPU 单核时钟频率受功耗限制趋于平稳，算力增长主要依赖核心数量与向量宽度，而 GPU 持续增加并行运算单元。
2007 年 NVIDIA 发布 CUDA 平台后，开发者可以直接使用 C/C++ 在 GPU 上编写通用并行程序。神经网络中的矩阵乘法与卷积等计算同样是对大规模数据重复执行相同的算术运算，因此在硬件特性上高度契合 GPU。
页面下方为三个使用 GPU 通用计算的项目的标志：Khronos Group 的 OpenCL、Hugging Face 的 LeRobot 与 ggml。
""")


# 上一页给出各代 GPU 的峰值算力。扩展内容：SM 内另有专用的矩阵单元 Tensor Core，本讲的示例程序不使用它。
def tensor_core(p):
    p.title("sidebar：Tensor Core")
    p.side_image("assets/h100-sm.png", alt="contain", width="38%").footnote(
        "图：NVIDIA H100 Tensor Core GPU Architecture 白皮书")
    slide(p, r"""
- 自 Volta 架构（2017）起，SM 内部除常规 CUDA 核心外还集成了专用矩阵运算单元 **Tensor Core**，单条指令即可完成一个小规模矩阵的乘加运算 $D = A \times B + C$。
- 右图 H100 的单个 SM 划分为 4 个处理块，每块配备 32 个 FP32 核心与 1 个 Tensor Core。
- Tensor Core 使用 FP16、FP8 等低精度数值格式，在 H100 上提供的矩阵算力比 FP32 CUDA 核心高出一个数量级。
- 现代深度学习框架中的矩阵乘法主要由 Tensor Core 加速。本讲的示例程序使用常规 CUDA 核心。
""")
    p.cite(title="NVIDIA H100 Tensor Core GPU Architecture", author="NVIDIA", year="2022")
    p.notes("""
右图为 H100 GPU 的单个 SM 结构图。顶部的 L1 指令缓存下方划分为 4 个独立的计算分区，每个分区包含 1 个 Warp 调度器、16384 个 32 位寄存器、16 个 INT32 单元、32 个 FP32 CUDA 核心、16 个 FP64 单元以及 1 个第四代 Tensor Core。底部为 256 KB 的 L1 数据缓存与共享内存。
以 H100 SXM 版本为例，其 FP32 CUDA 核心的峰值算力约为 67 TFLOP/s，而 FP16 Tensor Core 的稠密峰值算力高达 990 TFLOP/s。
执行 Tensor Core 指令时，同一 Warp 的 32 个线程协同工作，每个线程寄存器持有子矩阵的部分元素并共同完成一次矩阵乘加。cuBLAS 与 cuDNN 等算子库在输入为半精度或混合精度格式时会自动调用 Tensor Core。
""")


EXERCISE_STRUCTURE = r"""
判断下列计算任务是否适合在 GPU 上执行，并说明理由。

1. 将一幅 4K 图像（3840 × 2160 像素）中每个像素的亮度值乘以 1.2。
2. 沿单向链表依次遍历各个节点，累加节点中保存的数值。
3. 计算两个 4096 × 4096 浮点矩阵的乘积。
4. 使用 `gcc` 将一个 C 语言源文件编译为可执行程序。
"""


def exercise_structure(p):
    p.title("练习：适合 GPU 的计算")
    slide(p, EXERCISE_STRUCTURE)
    p.notes("""
判断依据是三项特征：数据规模、运算一致性与元素之间的独立性。
""")


def exercise_structure_2(p):
    p.title("练习：适合 GPU 的计算")
    slide(p, EXERCISE_STRUCTURE)
    slide(p, r"""
答案：
1. **适合**。约 830 万个像素均执行相同的乘法运算，且各像素之间无数据依赖。
2. **不适合**。后继节点的地址依赖当前节点的读取结果，各步访问存在严格的串行依赖。
3. **适合**。结果矩阵的 1600 多万个元素各自对应一行与一列的内积，可彼此独立地并行计算。
4. **不适合**。词法与语法分析按顺序处理源代码，控制流分支复杂且缺乏统一的数据并行运算。
""")
    p.notes("""
1. 4K 图像共有 3840 × 2160 = 8294400 个像素，每个像素独立执行一次浮点乘法。
2. 链表遍历的每一步都必须等待前一步读取出 `next` 指针，多个核心无法同时访存。若将数据连续存放在数组中，求和本身可以通过并行规约完成。
3. 结果矩阵共有 4096 × 4096 = 16777216 个元素，各元素的内积计算相互独立。
4. 编译器的执行路径高度依赖输入源码的语法结构，条件分支密集，不具备数据并行特征。
""")


# 第二部分：CUDA 编程。
#
# 起点问题：GPU 有许多 SM，每个 SM 有许多核心，怎样让数百万个线程各做一件事。
# 回答：按单个线程的视角写核函数。线程组织为 Grid 与线程块，用内置变量计算全局下标。
# 主机管理显存并启动核函数。同一线程块内的线程通过共享内存协作。
# 运行的例子先是向量加法（线程之间互不通信），再是向量内积（线程之间需要汇总）。
# 页序：主机与设备 → 核函数 → 线程组织与下标 → 线程块的调度 → 显存管理与启动 →
# 共享内存与块内同步 → 练习。

# 上一部分给出 GPU 的结构。本页给出使用它的程序结构：CPU 上的主机代码与 GPU 上的设备代码。
def cuda_model(p):
    p.title("主机与设备")
    slide(p, r"""
- **CUDA**（Compute Unified Device Architecture）是 NVIDIA 推出的通用并行计算平台，支持使用 C/C++ 直接编写在 GPU 上执行的程序。
- 一个 CUDA 程序由两部分协同组成：
  - **主机**（Host）：CPU 及其系统内存，负责程序控制流与普通函数执行；
  - **设备**（Device）：GPU 及其显存，负责大规模数据并行计算。
""")
    p.table(
        headers=["函数修饰符", "执行位置", "调用方", "用途"],
        rows=[
            ["无", "主机（CPU）", "主机", "`main` 与普通 C/C++ 函数"],
            ["`__global__`", "设备（GPU）", "主机（`<<<...>>>` 启动）", "**核函数**（Kernel）：返回 `void`，由成组线程并行执行"],
            ["`__device__`", "设备（GPU）", "设备", "设备端辅助函数，供核函数或其他设备函数调用"],
        ],
        widths=[18, 16, 34, 32],
    )
    p.notes("""
在 CUDA 编程中，主机端代码与设备端代码编写在同一个 `.cu` 源文件中。
不带修饰符的普通函数默认在主机（CPU）上执行。以 `__global__` 修饰的函数称为核函数（Kernel），由主机端通过 `<<<...>>>` 执行配置语法启动。每次启动会在 GPU 上创建大量线程，所有线程并发执行同一个核函数。
以 `__device__` 修饰的函数在 GPU 上执行，仅能由核函数或其他 `__device__` 函数调用，可以有返回值。
""")


VECADD_CODE = """// CPU: one thread runs the loop, i goes from 0 to n - 1
void add_cpu(const float *a, const float *b, float *c, int n) {
    for (int i = 0; i < n; i++)
        c[i] = a[i] + b[i];
}

// GPU: every thread runs this function once and computes one element
__global__ void add_kernel(const float *a, const float *b, float *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;    // this thread in the grid
    if (i < n)                                        // the last block has threads past n
        c[i] = a[i] + b[i];
}"""


# 上一页给出核函数的语法位置。本页以向量加法给出核函数的写法：循环体成为每个线程执行的函数。
def vecadd_kernel(p):
    p.title("核函数")
    p.code("cuda", VECADD_CODE)
    slide(p, r"""
- CPU 函数 `add_cpu` 由单个线程顺序执行 `for` 循环，下标 `i` 从 `0` 遍历至 `n - 1`。
- GPU 核函数 `add_kernel` 按单个线程的视角编写，将循环展开为并行线程，每个线程负责计算一个元素 `c[i] = a[i] + b[i]`。源文件：[vecadd.cu](file:examples/vecadd.cu)
""")
    p.notes("""
以向量加法 `c[i] = a[i] + b[i]` 为例对比 CPU 与 GPU 的编程方式。CPU 版本通过循环迭代处理 `n` 个元素。GPU 版本则消除了外层循环，启动至少 `n` 个线程，核函数体即为原循环体的一次迭代。
所有线程执行完全相同的核函数代码，通过内置变量 `blockIdx`、`blockDim` 与 `threadIdx` 计算各自唯一的全局下标 `i`，进而访问数组的不同元素。这种编程模式称为单程序多数据（SPMD，Single Program, Multiple Data）。
""")


# 上一页的核函数用 blockIdx、blockDim、threadIdx 计算下标。本页给出这三个变量的含义与下标公式。
def thread_index(p):
    p.title("线程组织与下标")
    slide(p, r"""
- 一次核函数启动创建一个 **Grid**（网格）。Grid 包含若干个规模相同的**线程块**（Thread Block），每个线程块包含若干个**线程**（Thread），一个线程块至多 1024 个线程。
- 线程的全局下标为 `i = blockIdx.x * blockDim.x + threadIdx.x`。当数组长度 `n` 非块大小的整数倍时，向上取整启动 `(n + threads - 1) / threads` 个线程块，并在核函数中用 `if (i < n)` 过滤越界线程。
""")
    figure(p, "thread-index", 1080)
    p.notes("""
启动核函数时需指定两级尺寸：Grid 中的线程块数量（`gridDim.x`）与每个线程块内的线程数量（`blockDim.x`）。
图示包含 4 个线程块、每块 8 个线程，合计 32 个线程。`blockIdx.x` 表示当前线程块在 Grid 中的编号（0 至 3），`threadIdx.x` 表示线程在块内的编号（0 至 7）。第 2 号线程块的第 3 号线程对应的全局下标为 `i = 2 × 8 + 3 = 19`，负责计算 `c[19]`。
若数组长度 `n = 29`，向上取整需启动 `(29 + 7) / 8 = 4` 个线程块（共 32 个线程）。最后一个线程块中编号为 29、30、31 的三个线程不满足 `i < n`，因此在条件判断后直接退出，不会发生越界访存。
Grid 与线程块也可以组织为二维、三维。本页只用一维。
""")


# 上一页给出线程怎样编号。本页给出线程块与 SM 的关系：块整体分配到 SM，块之间相互独立，程序随 SM 数量扩展。
def block_to_sm(p):
    p.title("线程块的调度")
    figure(p, "block-to-sm", 840).footnote("按 NVIDIA CUDA C++ Programming Guide 的图重画")
    slide(p, r"""
- 一个线程块会被整体分配到某一个 SM 上执行，直到执行完毕都不会迁移。同一线程块的线程可以通过该 SM 的共享内存协作，不同线程块的线程之间没有同步手段。
- 各个线程块之间的执行顺序相互独立：硬件调度器将待执行的线程块依次分配给有空闲资源的 SM。同一程序在拥有更多 SM 的 GPU 上运行时，能够自动获得更高的并行度。
""")
    p.notes("""
图中展示了包含 8 个线程块的 Grid 在不同规模 GPU 上的调度过程：在拥有 2 个 SM 的 GPU 上，每个 SM 先后执行 4 个线程块。在拥有 4 个 SM 的 GPU 上，每个 SM 仅需执行 2 个线程块，应用程序代码无需作任何修改。
线程块之间没有执行顺序的保证，也没有同步指令：核函数不能依赖某个线程块先于另一个线程块执行。
""")


# 上一页给出线程块由硬件调度。本页给出主机端怎样准备数据并启动网格：显存的分配、复制与核函数的启动。
def vecadd_host(p):
    p.title("显存管理与核函数启动")
    slide(p, r"""
- 主机内存与设备显存采用独立的地址空间：主机代码不能直接解引用显存指针 `da`，核函数也不能解引用主机指针 `a`。
- `cudaMalloc` 与 `cudaFree` 负责分配和释放显存。`cudaMemcpy` 在主机与显存之间同步复制数据，最后一个参数指明传输方向。
- `add_kernel<<<blocks, threads>>>(...)` 按指定的网格规模与块大小在设备上启动核函数。
""")
    p.frames(*(f"assets/host-device-{k}.svg" for k in range(1, 7)), width_px=1000)
    p.notes("""
动画逐步执行 `vecadd.cu` 的 `main` 函数，每一帧新加的箭头是这一步的数据流向，前面各步的箭头变淡。初始状态：输入 `a`、`b` 在主机内存，设备显存是空的，`c` 尚无内容。
① `cudaMalloc` 在设备显存中分配 `da`、`db`、`dc`，此时三块显存没有内容。② `cudaMemcpy` 经 PCIe 把 `a`、`b` 复制到 `da`、`db`。③ 启动核函数，GPU 读取 `da`、`db`，把结果写入 `dc`。启动之后紧跟 `CHECK(cudaGetLastError())`。④ `cudaMemcpy` 把 `dc` 复制回主机内存的 `c`。⑤ `cudaFree` 释放三块显存。
`cudaMalloc` 接收指针变量的地址（`float **`），将分配到的显存起始地址写入 `da`、`db`、`dc`。这些地址位于设备显存空间，只能传递给核函数或 `cudaMemcpy`，若在 CPU 代码中直接读写 `*da`，将触发段错误。
`cudaMemcpy` 是同步接口，待数据传输全部完成后才向调用方返回，其第四个参数通过 `cudaMemcpyHostToDevice` 或 `cudaMemcpyDeviceToHost` 指明传输方向。
""")


# 向量加法中各线程互不通信。本页给出需要汇总的计算：向量内积，同一线程块的线程通过共享内存交换中间结果。
def dot_kernel(p):
    p.title("共享内存与块内求和")
    p.code("cuda", DOT_KERNEL_NOSYNC)
    slide(p, r"""
- 向量内积 $\sum_{i=0}^{n-1} w[i] \cdot x[i]$ 需将各线程计算的乘积汇总求和。
- `__shared__ int cache[256]` 声明块内共享数组，每个线程块拥有一份独立副本，由块内 256 个线程共用。
- 每个线程先把自己的乘积存入 `cache[tid]`。通过折半求和，最终 `cache` 中所有数字的和落在 `cache[0]`，即当前线程块的部分和，由 0 号线程写入显存数组 `block_sum[blockIdx.x]`。核函数结束后，将 `block_sum` 拷贝回内存，由 CPU 对各部分和求和，得到点积最终结果。
""")
    p.notes("""
在向量加法中，各线程独立写入各自的 `c[i]`，线程之间无需通信。而向量内积要求将所有线程计算的乘积 `w[idx] * x[idx]` 累加为一个标量。
`dot_kernel` 采用分块归约策略：每个线程块首先在共享内存 `cache` 中将本块 256 个线程的乘积求和，由 0 号线程将块内部分和写入显存数组 `block_sum[blockIdx.x]`，最后在主机端将各线程块的部分和累加。
块内求和采用折半求和（树形规约）：第一步 s = 128，线程 0 至 127 分别累加后 128 个位置的元素，之后参与加法的线程数逐步减半，经过 log₂ 256 = 8 步，`cache[0]` 是全部 256 个乘积之和。
""")


# 上一页给出内积的核函数，其块内求和由多个线程读写同一个 cache。本页取线程块只有一个 Warp 的情形：核函数的结果正确。
def dot_warp(p):
    p.title("单个 Warp 的线程块")
    slide(p, r"""
- 当线程块只有 32 个线程时，整个线程块运行在 1 个 Warp 上，每个周期执行同一条指令。
- 折半求和的每一步，Warp 的全部线程一起执行读取指令和写入指令。下一步的读取指令在这一步的写入指令之后才发射，读到的都是已经计算好的值，因此核函数的结果正确。
- 当线程块含多个 Warp 时，各个 Warp 的推进速度可以不同。
""")
    p.frames(*(f"assets/warp-{k}.svg" for k in range(1, 5)), width_px=1120)
    p.notes("""
动画以 8 个线程为例，假定它们组成 1 个 Warp。左侧只有一份循环代码：8 个线程共用一个程序计数器，同一时刻执行同一条指令，橙色底的一行是 Warp 当前执行的行。右侧是 cache 的各行。
s = 4：加法由 8 个线程一起执行，线程 0 至 3 写入 cache[0] 至 cache[3]，线程 4 至 7 不满足 `tid < s`，不写入。这一条指令完成之后，下一步才开始。
s = 2：线程 0、1 读取 cache[2]、cache[3]，此时这两个位置已经是 6 和 7，得到 cache[0] = 14，cache[1] = 17。
s = 1：线程 0 得到 cache[0] = 14 + 17 = 31。每一步的读取都在上一步的写入之后，求和的结果正确。
页面文字的前提是线程块恰好 32 个线程。此时 `dot_kernel` 的循环从 s = 16 开始，线程 0 至 15 参与加法，线程 16 至 31 在同一条指令中被屏蔽，不写入。
一个 Warp 只有一个指令流，写入指令先于下一步的读取指令发射，所以不会出现“一部分线程已经读取，另一部分线程还没有写入”的情况。
每块 256 个线程的 `dot_kernel` 含 8 个 Warp，这些 Warp 由调度器分别调度，推进速度不同。
""")


# 上一页说明单个 Warp 的结果正确。本页取线程块含多个 Warp 的情形，给出求和结果的错误。
def dot_race(p):
    p.title("多个 Warp 的求和错误")
    p.valign("middle")
    p.frames(*(f"assets/race-{k}.svg" for k in range(1, 6)), width_px=1120)
    p.notes("""
动画以 8 个线程为例，并假定每个 Warp 有 2 个线程：线程 0、1 是 Warp 0，线程 2、3 是 Warp 1。这样线程块含多个 Warp。下方是线程 0 至 3 各自执行的循环，橙色底的一行是该线程当前所在的行，`s` 是该线程循环变量的当前值。不同 Warp 的线程并不同步推进：Warp 0 的线程 0、1 比 Warp 1 的线程 2、3 先完成 s = 4 的加法，得到 cache[0] = 8、cache[1] = 10。
线程 0、1 不等待线程 2、3，直接进入 s = 2，读取 cache[2]、cache[3]。这两个位置还没有被线程 2、3 写入，仍是旧值 4 和 1，于是 cache[0] = 8 + 4 = 12，cache[1] = 10 + 1 = 11。
线程 2、3 随后才完成 s = 4，写入 6 和 7，但线程 0、1 已经用了旧值。最后线程 0 执行 s = 1，得到 cache[0] = 12 + 11 = 23，正确的和是 31。
每块 256 个线程分属 8 个 Warp，各个 Warp 的推进速度可以不同，所以这个核函数的求和结果不可靠。需要让先到达的 Warp 等待后到达的 Warp。
""")


# 第 37 页左侧的两个箭头各指向代码的一行：行高 27 px，说明的行高 40 px。
SYNC_ARROW_1, SYNC_ARROW_GAP = 131, 26


# 上一页给出 Warp 之间不等待造成的错误。本页给出解决方案：块内同步屏障 __syncthreads()。
def dot_sync(p):
    p.title("块内同步屏障")
    with p.flex("row", widths=[1, 20]):
        with p.flex("column"):
            p.spacer(SYNC_ARROW_1)
            slide(p, "**→**").halign("right")
            p.spacer(SYNC_ARROW_GAP)
            slide(p, "**→**").halign("right")
        p.code("cuda", DOT_KERNEL)
    slide(p, r"""
- `__syncthreads()` 为块内同步屏障：块内所有线程必须全部到达该行后才能继续向下执行，先到达的线程在该行等待。
- 循环中每一步加法之后放一个屏障，下一步的读取就在这一步的全部写入完成之后。循环之前的屏障保证进入循环时 256 个乘积都已存入 `cache`。
""")
    p.notes("""
上一页的错误来自 Warp 之间没有等待。在核函数中加入 `__syncthreads()` 即可：循环体内的屏障确保当前步的所有加法均已完成，再进入下一轮折半累加。循环之前的屏障确保块内 256 个线程均已将乘积写入 `cache`。
""")


# 上一页给出带屏障的核函数。本页用与上一组相同的 8 个线程、每个 Warp 2 个线程，给出先到达的 Warp 怎样等待。
def dot_fold(p):
    p.title("块内同步屏障")
    p.valign("middle")
    p.frames(*(f"assets/fold-{k}.svg" for k in range(1, 8)), width_px=1120)
    p.notes("""
动画与上一组相同：8 个线程，每个 Warp 2 个线程，下方是线程 0 至 3 各自执行的循环，循环中多了 `__syncthreads()`。橙色底的一行是该线程当前所在的行，红色底表示线程停在屏障上等待。
s = 4：Warp 0 先完成加法，到达屏障后等待。Warp 1 随后完成加法，写入 6 和 7，全部线程到达屏障，才一起进入下一轮。
s = 2：线程 2、3 不满足 `tid < s`，不执行加法，先到达屏障并等待 Warp 0。Warp 0 完成加法时读到的 cache[2]、cache[3] 已是 6 和 7，得到 cache[0] = 14，cache[1] = 17。
s = 1：线程 0 求得 cache[0] = 14 + 17 = 31，与带屏障的折半求和的结果一致。线程 4 至 7 在这些步骤中不执行加法，图中没有画出。
在每块 256 个线程的 dot_kernel 中，步长 s 依次取 128、64、32、16、8、4、2、1，经过 log₂ 256 = 8 步即可完成 256 个元素的求和。
对于长度 n = 2²⁴ 的向量，共启动 65536 个线程块计算出 65536 个部分和，由主机端通过 cudaMemcpy 复制回内存并完成最终累加。
""")


# 前几页给出带 __syncthreads() 的核函数。本页运行完整的核函数，并与同一内积的 CPU 循环比较耗时。
def dot_run(p):
    p.title("向量内积的 CPU 与 GPU 耗时")
    p.valign("middle")
    p.demo("提交 dot.cu 到课程服务器",
           SUBMIT.format("dot.cu") + TRIM,
           show=SUBMIT.format("dot.cu"),
           pwd="examples",
           output=DOT_OUTPUT,
           bold=DOT_BOLD,
           files=["examples/dot.cu"])
    slide(p, r"""
执行相同的内积运算，仅对运算速度计时的情况下，GPU 对 CPU 单核的加速比为 14.2 倍。
""")
    p.notes("""
程序先用 GPU 计算内积，得到 `sum`，再用 CPU 循环计算一遍得到 `check`，两者相等。
65536 个线程块计算出 65536 个部分和，由主机端复制回内存并完成最终累加。
""" + DOT_NOTES)


EXERCISE_CUDA = r"""
编写核函数 `axpy`，对长度为 `n = 10000000` 的单精度浮点数组计算 `y[i] = a * x[i] + y[i]`。

1. 写出 `__global__ void axpy(float a, const float *x, float *y, int n)` 的函数体。
2. 每个线程块包含 256 个线程时，主机端应当启动多少个线程块？写出核函数启动语句。
3. 在最后一个线程块中，有多少个线程满足 `i < n` 并执行乘加运算，有多少个线程直接返回？
"""


DOT_KERNEL = """__global__ void dot_kernel(const int *w, const int *x, int *block_sum, int n) {
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
}"""


# 同一个核函数，去掉其中的两处 __syncthreads()：第一页给出计算，同步在之后才引入。
DOT_KERNEL_NOSYNC = "\n".join(line for line in DOT_KERNEL.split("\n")
                              if "__syncthreads" not in line)


# 部分末尾的课堂练习：补全骨架 axpy.cu 的核函数体，在课程服务器上运行，输出 differ 0。
def exercise_cuda(p):
    p.title("练习：axpy 核函数")
    slide(p, EXERCISE_CUDA)
    p.demo("提交 axpy.cu 到课程服务器运行",
           SUBMIT.format("axpy.cu") + TRIM,
           show=SUBMIT.format("axpy.cu"),
           pwd="examples",
           output=AXPY_SKELETON_OUTPUT,
           bold=[3],
           files=["examples/axpy.cu"])
    p.notes("""
axpy（a times x plus y）是线性代数基础库 BLAS 中的标准逐元素运算，其线程下标计算与边界判断逻辑与 add_kernel 相同。
examples/axpy.cu 是骨架：main 与主机端的显存分配、复制、启动、结果核对已写好，核函数体只有一行 TODO 注释。函数体为空时 differ 等于元素个数。课堂上让学生补全后提交到课程服务器，服务器返回的内容含 differ 0 即通过。
""")


AXPY_ANS = """__global__ void axpy(float a, const float *x, float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
        y[i] = a * x[i] + y[i];
}"""


def exercise_cuda_2(p):
    p.title("练习：axpy 核函数")
    slide(p, EXERCISE_CUDA)
    with p.flex("row", widths=[1, 12]):
        slide(p, r"**答案：**")
        p.code("cuda", AXPY_ANS)
    slide(p, r"""
2. 数量为 `(10000000 + 255) / 256` = **39063**，
启动语句为 `axpy<<<39063, 256>>>(a, dx, dy, n);`。
3. 前 39062 个线程块共处理 $39062 \times 256 = 9999872$ 个元素。最后一个线程块中有 **128 个**线程满足 `i < n`，其余 **128 个**线程跳过计算直接返回。
""")
    p.notes("""
39063 个线程块共创建 $39063 \\times 256 = 10000128$ 个线程，其中全局下标为 0 至 9999999 的线程更新对应的 `y[i]`，下标为 10000000 至 10000127 的 128 个线程因不满足边界条件而直接退出。
""")


# 第三部分：线程块与 Warp 的执行。
#
# 起点问题：dot_kernel 为 2^24 个元素启动 65536 个线程块、每块 256 个线程。GPU 只有几十个 SM，
# SM 同时能容纳多少线程块，等待显存时在做什么，核函数中的一个 if 会发生什么。
# 回答：每个 SM 按线程数、寄存器、共享内存三项资源驻留若干线程块。SM 在驻留的 Warp 之间零开销切换，
# 用其他 Warp 的指令填补访存等待。同一 Warp 内的分支分化使两条路径串行执行。
# 页序：线程块的驻留与执行 → Warp 调度 → 分支分化 → 实测 → 练习。

# 上一部分写出了 dot_kernel，它启动 65536 个线程块。本页用动画给出线程块与 SM 的关系：等待、分配、驻留、执行、完成，并区分驻留与执行。
def block_stages(p):
    p.title("线程块的驻留与执行")
    slide(p, r"""
- 线程块整块分配给一个 SM。**驻留**指线程块占有该 SM 的线程数、寄存器与共享内存，直到线程块结束。一个 SM 可以同时驻留多个线程块，装不下的线程块等待。
- **执行**指 SM 的 Warp 调度器每个周期从驻留的 Warp 中选出几个，各发射一条指令，其余驻留的 Warp 本周期不发射。
""")
    p.frames(*(f"assets/block-life-{k}.svg" for k in range(1, 8)), width_px=1120)
    p.notes("""
动画中有 16 个线程块和 2 个 SM，每个线程块 256 个线程，即 8 个 Warp，每个 SM 最多驻留 1536 个线程，即 6 个线程块。
第 1 帧：线程块都在等待，没有分配到 SM。第 2 帧：硬件把线程块整块分配给 SM，每个 SM 驻留 6 个线程块，线程数用满，B12 至 B15 继续等待。
第 3、4 帧：驻留与执行是两个层次。驻留是资源的占用：线程块被分配到 SM 后，它的线程数、寄存器和共享内存一直归这个线程块，直到它结束。执行是指令的发射：每个 SM 有 4 个 Warp 调度器，每个周期各从驻留的 Warp 中选出一个就绪的，发射它的下一条指令。图中橙色是本周期发射的 4 个 Warp。每个周期调度器都重新选择，从指令已就绪的 Warp 中挑一个，具体策略由硬件决定：同一个 Warp 的后续指令都就绪时可以连续几个周期被选中，遇到等待显存之类的停顿才轮到其他 Warp。没有被选中的驻留 Warp 不前进，但寄存器状态一直保留在寄存器堆中。因此驻留的 48 个 Warp 不是同时在运算。
第 5 至 7 帧：B2 的全部线程结束后，它占用的资源整块释放，SM 0 空出一个线程块的位置，硬件调度器把等待的 B12 分配进来。线程块之间没有执行顺序的保证。
""")


# 上一页给出线程块在 SM 上驻留、调度器每个周期只发射部分 Warp。本页给出 SM 在驻留的 Warp 之间怎样切换，以及切换如何填补访存等待。
def warp_switch(p):
    p.title("Warp 调度与延迟隐藏")
    slide(p, r"""
- 当某个 Warp 因等待显存读取结果而阻塞时，调度器立即切换执行其他就绪的 Warp。所有驻留线程的寄存器状态始终保存在寄存器堆中，切换没有上下文保存与恢复的开销。
""")
    p.frames(*(f"assets/warp-switch-{k}.svg" for k in range(1, 10)), width_px=1120)
    slide(p, r"""
通过 Warp 调度，隐藏了因读取显存而导致的延迟，SM 的资源利用率得到提高。
""")
    p.notes("""
动画展示单个 SM 上 4 个驻留 Warp 的轮流执行。左边是 SM：一个 Warp 调度器和 4 个 Warp，右边的方块是显存。每个 Warp 先发射两条指令，第二条是读显存的 ld，请求用箭头送往显存，之后需要等待 8 个周期才能获得返回的数据。右边的时间轴与左边的 Warp 一一对应：橙色格是发射了指令的周期，虚线格是等待。
周期 0 至 7：调度器依次选中 Warp 0、1、2、3，每个 Warp 发出读请求后进入等待。Warp 0 在等待时，调度器无缝切换并发射其他就绪 Warp 的指令。
周期 8、9：4 个 Warp 都在等待，调度器没有指令可发，周期空出。8 个周期的等待需要另外 4 个 Warp 各发射两条指令才能盖住，这里只有 3 个，所以空出 2 个周期。
周期 10 起：数据依次返回，Warp 0、1、2、3 重新就绪并继续发射。
CPU 在切换线程时需要通过操作系统或硬件将寄存器内容保存至内存，再从内存加载目标线程的上下文，而 GPU 为所有驻留线程预留了独立的物理寄存器，因此 Warp 之间的切换可以在单周期内完成，开销为零。
驻留的就绪 Warp 数量越多，调度器越容易在每个周期找到可执行的指令，从而有效掩盖显存的长访问延迟。
""")


# 上一页给出 Warp 之间的切换与延迟隐藏。本页给出同一 Warp 内的线程走向不同分支时的执行方式。
def divergence(p):
    p.title("分支分化")
    slide(p, r"""
- 同一 Warp 内的 32 个线程共用同一个程序计数器，在每个时钟周期执行相同的指令。
- 当条件分支语句使同一 Warp 内的线程走向不同执行路径时，即发生**分支分化**（Branch Divergence）：硬件依次串行执行各条分支路径，当前路径之外的线程被临时屏蔽。
""")
    p.frames(*(f"assets/divergence-{k}.svg" for k in range(1, 6)), width_px=900)
    slide(p, r"""
- 在分支分化期间，被屏蔽线程对应的运算单元处于空闲状态，整个 Warp 完成该条件语句所需的周期数等于各条分支路径的周期数之和。
""")
    p.notes("""
左侧为条件分支代码及其指令地址，右侧每列对应 Warp 执行的一条指令，每行对应一个线程（实心箭头表示执行，虚线表示被屏蔽等待）。
第 100、101 行指令由全体 32 个线程共同执行。进入 `if-else` 后，第 102、103 行仅由偶数号线程执行，奇数号线程被屏蔽。随后第 104 行仅由奇数号线程执行，偶数号线程被屏蔽。到达第 105 行时，分支重新汇合，全体线程恢复同步执行。
若分支条件仅依赖 Warp 编号（例如 `if ((threadIdx.x / 32) % 2 == 0)`），则同一 Warp 内的 32 个线程始终走向同一分支，不会引发分支分化。
""")


# 上一页给出分化时两条路径串行。本页对同一组分支，按线程奇偶划分与按 Warp 奇偶划分，比较耗时。
def diverge_run(p):
    p.title("分支分化")
    p.demo("同样的两条分支路径，按线程奇偶与按 Warp 奇偶划分",
           SUBMIT.format("diverge.cu") + TRIM,
           show=SUBMIT.format("diverge.cu"),
           pwd="examples",
           output=DIVERGE_OUTPUT,
           bold=DIVERGE_BOLD,
           files=["examples/diverge.cu"])
    figure(p, "diverge-compare", 1120)
    p.notes(DIVERGE_NOTES)


EXERCISE_EXEC = r"""
某 GPU 的每个 SM 最多同时驻留 1536 个线程，`dot_kernel` 每个线程块含 256 个线程。

1. 单个 SM 最多同时驻留多少个线程块、多少个 Warp？
2. 当步长 `s` 分别为 `128` 和 `16` 时，一个线程块的 8 个 Warp 中分别有多少个发生分支分化？
"""

EXERCISE_EXEC_3 = r"""
3. 在 `dot_kernel` 计算完成 `block_sum` 写入后，能否在末尾调用 `__syncthreads()` 后直接汇总 `block_sum` 得到点积的最终结果？
"""

DOT_LOOP = """for (int s = blockDim.x / 2; s > 0; s >>= 1) {    // 256 products -> 1 sum in 8 steps
    if (tid < s) cache[tid] += cache[tid + s];
    __syncthreads();
}"""

DOT_TOTAL = """if (tid == 0) block_sum[blockIdx.x] = cache[0];   // each block writes its sum
__syncthreads();                                  // block 0 waits here for the other blocks?
if (blockIdx.x == 0 && tid == 0)                  // block 0 adds up all the sums
    for (int b = 1; b < gridDim.x; b++) block_sum[0] += block_sum[b];"""


def exercise_exec(p):
    p.title("练习：线程块驻留与分支分化")
    slide(p, EXERCISE_EXEC)
    p.code("cuda", DOT_LOOP)
    slide(p, EXERCISE_EXEC_3)
    p.code("cuda", DOT_TOTAL)
    p.notes("""
第 1 题根据给定的每 SM 最大驻留线程数计算驻留上限。第 2 题注意块内线程按连续 32 个编号划分为一个 Warp（`tid` 0 至 31 为 Warp 0，依此类推）。第 3 题考察 `__syncthreads()` 的作用范围与线程块调度机制。
""")


def exercise_exec_2(p):
    p.title("练习：线程块驻留与分支分化")
    slide(p, r"""
1. 单个 SM 最多驻留 $1536 \div 256 =$ **6 个线程块**，合 $6 \times 256 \div 32 =$ **48 个 Warp**。
2. `s = 128` 时满足 `tid < 128` 的恰好是 Warp 0 至 3 的全部线程，Warp 4 至 7 全部跳过加法，因此 **0 个** Warp 发生分化。`s = 16` 时仅 Warp 0 内的前 16 个线程执行加法、后 16 个线程等待，因此 **1 个** Warp 发生分化。
""")
    p.code("cuda", DOT_LOOP)
    slide(p, r"""
3. **不能**。`__syncthreads()` 仅能同步同一个线程块内部的线程。此时即使 `sync` 结束，其他线程块大概率还没有完成对应的部分和计算，甚至有一些可能从未被调度到 SM 上执行。
""")
    p.code("cuda", DOT_TOTAL)
    p.notes("""
2. 当 s ≥ 32 时，`tid < s` 的真假值以 32 的倍数为界，每个 Warp 内部的所有线程行为完全一致，无分支分化。仅在 s = 16、8、4、2、1 的最后 5 步中，Warp 0 内部出现分化。
3. 跨线程块汇总数据需借助全局显存：可以在主机端累加、启动第二个核函数继续规约，或使用全局原子指令 `atomicAdd`（对显存地址执行不可分割的加法）。
""")


# 第四部分：框架中的算子调用。
#
# 起点问题：Python 中的一行 `y = x @ w` 没有写核函数，GPU 上执行的核函数由谁选出，自己写的核函数怎样接入同一套机制。
# 回答：PyTorch 分为 Python API、Python 绑定、ATen、Dispatcher、各 dispatch key 的核函数、cuBLAS 等库。
# 每个算子的签名与各 key 的核函数登记在 Dispatcher 的一张表中，调用时按张量的设备等属性查表。
# 自己写的核函数用同样的方式登记：C++ 函数封装核函数，TORCH_LIBRARY 声明，TORCH_LIBRARY_IMPL 按 key 登记，
# cpp_extension.load 编译加载，torch.ops 调用。
# 页序：问题（x @ w）→ 分层 → 自定义算子（以向量加法为例：开发步骤、C++ 封装、登记与调用）→

TORCH_MATMUL = """import torch

x = torch.randn(2048, 2048, device="cuda", requires_grad=True)
w = torch.randn(2048, 2048, device="cuda", requires_grad=True)

y = x @ w    # float32 matrices on the GPU, no kernel is named here"""


# 本部分的第一页：给出一行 PyTorch 代码与它需要回答的三个问题。
def torch_matmul(p):
    p.title("PyTorch 中的矩阵乘法调用")
    p.code("python", TORCH_MATMUL)
    slide(p, r"""
- 这一行把两个 CUDA 张量相乘。代码中没有核函数，也没有 `<<<...>>>`。
- GPU 上的核函数由框架选出并启动。选出它要回答三个问题：
  1. Python 的 `@` 怎样变成对 C++ 函数的调用？
  2. 同一个算子在 CPU、CUDA、稀疏张量上各有实现，CUDA 的实现由谁选出？
  3. `requires_grad=True` 时反向传播所需的信息由谁记录？
""")
    p.notes("""
本部分以 PyTorch 为例，说明深度学习框架怎样把一次算子调用变成 GPU 上的核函数。函数名与文件名取自 PyTorch 2.x 的源码，版本之间可能有变动，文件路径均相对于源码仓库的根目录。
`x @ w`、`torch.matmul(x, w)` 与 `x.matmul(w)` 调用的是同一个 ATen 算子 `matmul`。两个张量位于 GPU 上，数据类型为 float32，都要求梯度。
这三个问题依次对应下面的三类机制：Python 绑定，Dispatcher 的分派，Autograd 核函数。
""")


# 上一页给出三个问题。本页给出 PyTorch 的分层，每个问题由其中一层回答。
def torch_layers(p):
    p.title("PyTorch 的分层")
    p.side_image("assets/pytorch-layers.svg", alt="contain", width="52%", side="left", valign="middle")
    slide(p, r"""
- **Python API**：`Tensor.__matmul__` 接收 `x @ w`。
- **Python 绑定**：解析 Python 参数，释放 GIL，调用 ATen 的 `matmul`。
- **ATen**：张量库。每个算子一个 C++ 函数，签名声明在 YAML 文件中。
- **Dispatcher**：按张量的 dispatch key 选择算子的核函数。
- **核函数**：算子在每个 key 下各有一个实现。CUDA 的实现调用 cuBLAS、cuDNN 等库，或启动自写的 `.cu` 核函数。
""")
    p.notes("""
图中自上而下是一次算子调用经过的层，橙色标出 `x @ w` 经过的核函数：Autograd 与 CUDA，以及 CUDA 核函数调用的 cuBLAS。Autocast、CPU 等 key 的核函数同样登记在 Dispatcher 中，这次调用不使用它们。
Python 绑定位于 `torch/csrc`。它把 Python 对象转换为 `at::Tensor`，在调用 C++ 函数之前释放 GIL，使其他 Python 线程可以运行。
ATen 的算子声明在 `aten/src/ATen/native/native_functions.yaml`，核函数的源码在 `aten/src/ATen/native/` 与 `aten/src/ATen/native/cuda/`。Dispatcher 的实现在 `aten/src/ATen/core/dispatch/`。
cuBLAS 与 cuDNN 是 NVIDIA 提供的库，PyTorch 通过 `aten/src/ATen/cuda/` 中的封装调用它们。CUDA 运行时负责把核函数提交给 GPU。
""")


# 上一页给出 PyTorch 的分层，Dispatcher 按 key 选出核函数。本页给出自己写的核函数接入这套机制、成为 Python 可调用的算子要经过的四步。
def custom_op_flow(p):
    p.title("自定义算子的开发步骤")
    figure(p, "custom-op-flow", 1090)
    slide(p, r"""
- 目标：让 Python 像调用 `torch.add` 一样调用自己写的 CUDA 核函数。以向量加法为例，算子名为 `mylib::myadd`。
- ① C++ 函数检查参数，分配输出张量，在当前 CUDA 流上启动核函数。
- ② `TORCH_LIBRARY` 声明算子，`TORCH_LIBRARY_IMPL` 为 CUDA 登记这个 C++ 函数。
- ③ `torch.utils.cpp_extension.load` 编译 `.cu` 文件并加载动态库，加载时注册算子。
- ④ 调用 `torch.ops.mylib.myadd`，Dispatcher 在表中找到登记的函数。
""")
    p.cite(title="Custom C++ and CUDA Operators", author="PyTorch Contributors", year="2025",
           venue="PyTorch Tutorials", url="https://docs.pytorch.org/tutorials/advanced/cpp_custom_ops.html")
    p.notes("""
本页给出四步的全貌，每一步的代码在自定义算子的各页中给出。前两步在同一个 `.cu` 文件中，后两步在 Python 脚本中。
`mylib` 是算子的命名空间，由 `TORCH_LIBRARY(mylib, m)` 创建，算子的全名是 `mylib::myadd`，在 Python 中通过 `torch.ops.mylib.myadd` 访问。
开发需要安装 CUDA Toolkit（提供 nvcc）与 PyTorch，`load` 在运行时即时编译，第一次调用较慢，结果缓存在构建目录中。
""")


CUSTOM_OP_CUDA = """__global__ void add_kernel(const float *a, const float *b, float *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}
at::Tensor myadd_cuda(const at::Tensor &a, const at::Tensor &b) {
    TORCH_CHECK(a.is_cuda() && b.is_cuda() && a.is_contiguous() && b.is_contiguous());
    TORCH_CHECK(a.dtype() == at::kFloat && b.dtype() == at::kFloat);
    TORCH_CHECK(a.sizes() == b.sizes());
    at::Tensor c = at::empty_like(a);
    int n = a.numel(), threads = 256, blocks = (n + threads - 1) / threads;
    add_kernel<<<blocks, threads, 0, c10::cuda::getCurrentCUDAStream()>>>(
        a.data_ptr<float>(), b.data_ptr<float>(), c.data_ptr<float>(), n);
    C10_CUDA_KERNEL_LAUNCH_CHECK();
    return c;
}"""

CUSTOM_OP_REGISTER = """TORCH_LIBRARY(mylib, m) {
    m.def("myadd(Tensor a, Tensor b)"
          " -> Tensor");
}
TORCH_LIBRARY_IMPL(mylib, CUDA, m) {
    m.impl("myadd", &myadd_cuda);
}"""

CUSTOM_OP_LOAD = """import torch
from torch.utils.cpp_extension import load

load(name="myadd", sources=["myadd.cu"],
     is_python_module=False, verbose=True)

a = torch.randn(1 << 20, device="cuda")
b = torch.randn(1 << 20, device="cuda")
c = torch.ops.mylib.myadd(a, b)
print(torch.equal(c, a + b))"""

# 左侧的两个箭头各指向代码的一行：核函数的定义与它的启动。行高 27 px，说明的行高 40 px。
CUSTOM_ARROW_1, CUSTOM_ARROW_GAP = 1, 209


# 上一页给出四步的全貌。本页写出第一步：向量加法的 C++ 函数，负责检查参数、分配输出并启动核函数。
def custom_op_cuda(p):
    p.title("核函数的 C++ 封装")
    with p.flex("row", widths=[1, 20]):
        with p.flex("column"):
            p.spacer(CUSTOM_ARROW_1)
            slide(p, "**→**").halign("right")
            p.spacer(CUSTOM_ARROW_GAP)
            slide(p, "**→**").halign("right")
        p.code("cuda", CUSTOM_OP_CUDA)
    slide(p, r"""
- `TORCH_CHECK` 检查设备、数据类型、内存布局与形状，不满足时在 Python 中抛出异常。核函数按一维下标读取 `data_ptr`，输入必须是 contiguous 的。
""")
    p.notes("""
核函数 `add_kernel` 是一维的向量加法，与 PyTorch 无关。封装函数只处理张量的接口：`at::Tensor` 是 PyTorch 的 C++ 张量类型，`data_ptr<float>()` 取出设备内存的指针，`numel()` 取出元素个数。
`at::empty_like(a)` 在 `a` 所在的设备上分配与 `a` 形状、数据类型相同的输出张量。`getCurrentCUDAStream()` 返回当前线程的 CUDA 流。PyTorch 的算子默认都在这个流上排队，所以核函数的启动是异步的：C++ 函数返回时，核函数可能还没有执行完，之后用到结果的算子在同一个流上排在它后面。
核函数在同一个流上排队，与其他算子按顺序执行。`C10_CUDA_KERNEL_LAUNCH_CHECK()` 检查启动本身是否出错，不等待核函数完成。多块 GPU 时还需要用 `c10::cuda::CUDAGuard` 切换到输入所在的设备，这里省略。
""")


# 上一页写出 C++ 函数。本页把它登记到 Dispatcher，从 Python 加载并调用。
def custom_op_register(p):
    p.title("声明、注册与调用")
    with p.flex("row"):
        p.code("cpp", CUSTOM_OP_REGISTER)
        p.code("python", CUSTOM_OP_LOAD)
    slide(p, r"""
- `m.def` 声明算子的名称与签名，`m.impl` 为 CUDA 这个 dispatch key 登记函数。调用时 Dispatcher 按张量所在的设备选出它。
- `load` 编译 `.cu` 文件并加载动态库，加载时执行左侧的注册代码。
- 注册后算子位于 `torch.ops.mylib`，调用写法与内置算子相同。
""")
    p.notes("""
左侧是 `.cu` 文件末尾的注册代码，右侧是 Python 脚本。`TORCH_LIBRARY` 创建命名空间并声明签名，签名的写法是 PyTorch 的 schema 语言，`Tensor` 表示张量参数，`->` 之后是返回值。`TORCH_LIBRARY_IMPL(mylib, CUDA, m)` 为 CUDA 这一个 key 登记实现。登记的 key 之外的调用不会进入这个函数：对 CPU 张量调用会报错，指出没有 CPU 后端的实现。
`load` 的第一个参数是扩展的名称，也是构建目录的名称。`verbose=True` 打印 nvcc 的命令行。动态库加载完成时，静态对象的构造函数执行了 `TORCH_LIBRARY` 与 `TORCH_LIBRARY_IMPL` 中的代码，算子因此出现在 Dispatcher 中。
逐元素的单精度加法在 GPU 上的结果与 `a + b` 逐位相同，所以用 `torch.equal` 比较。
""")
