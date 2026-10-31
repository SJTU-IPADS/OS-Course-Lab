# ICS · GPU 与 CUDA 编程

独立的一讲：从图形渲染引出 GPU 的结构，再写 CUDA 程序，然后分析线程块与 Warp 的执行，最后以 PyTorch 为例说明一次矩阵乘法调用怎样经过 Dispatcher 分派到 CUDA 核函数，再把一个核函数登记为自定义算子并在 Python 中调用。素材来自 `lectures/ICS2/2-3-gpu`（Mingcong Han 的 GPU 一讲）与 `refs/ppts/CSP-gpu+attention.pdf`（Xingda Wei 的 More on GPUs and FlashAttention）。第四部分的调用链取自 PyTorch 的源码与文档。

中文是基线（写在 Python 里），英文是 `i18n/en.toml` 翻译覆盖层。

## 结构

封面、目录页之后分四个部分，第一至第三部分以练习结束（题目与答案各一页），扩展内容是标题为 `sidebar：…` 的页，最后是自动生成的参考文献页。部分之间用 `lecture.bridge(...)` 过渡。共 79 张幻灯片（动画的每一帧各占一页）。

结构在 CUDA 之前，行为在 CUDA 之后：第一部分只讲 GPU 的组成，使 CUDA 的线程、线程块与 SM 有对应的硬件。第三部分用 CUDA 程序的实测结果讲执行。前三部分的程序是向量内积 `dot`：第二部分写出，第三部分分析它的执行。第四部分以 PyTorch 的 `x @ w` 为例，说明框架怎样选出并启动 GPU 上的核函数，并把自己写的核函数登记为算子。

| 部分 | 页 | 落点 |
| --- | --- | --- |
| 第一部分 · GPU 的结构 | 4–16 | 3D 渲染 · 实时渲染 · 渲染管线与着色器 · CPU 与 GPU 的硬件结构 · 独立 GPU 与集成 GPU · 流式多处理器（SM）（2 页）· Warp 与 SIMT · GPU 存储层次 · 峰值算力与通用 GPU · sidebar：Tensor Core · 练习 |
| 第二部分 · CUDA 编程 | 17–48 | 主机与设备 · 核函数 · 线程组织与下标 · 线程块的调度 · 显存管理与核函数启动（动画 6 帧）· 共享内存与块内求和（不带同步的 `dot_kernel`）· 单个 Warp 的线程块（文字与 4 帧动画同一页）· 多个 Warp 的求和错误（5 帧动画）· 块内同步屏障（带同步的 `dot_kernel`，1 页 + 7 帧动画）· 向量内积的 CPU 与 GPU 耗时 · 练习（axpy，提交到课程服务器） |
| 第三部分 · 线程块与 Warp 的执行 | 49–73 | 线程块的驻留与执行（7 帧动画）· Warp 调度与延迟隐藏（9 帧动画）· 分支分化（5 帧动画 + 1 页）· 练习 |
| 第四部分 · 框架中的算子调用 | 74–79 | PyTorch 中的矩阵乘法调用 · PyTorch 的分层 · 自定义算子（开发步骤、C++ 封装、声明注册与调用，各 1 页，例子是向量加法）|

页码随修改变化，以上是 2026-10-31 的页码。页面函数在 `pages.py` 中，顺序与 `lecture.py` 相同，函数名把下划线换成连字符就是页面的 id。每个部分在 `pages.py` 中以一段注释开头，写明该部分从什么问题出发、建立什么答案、各页的顺序。每个页面函数的注释写明它如何承接上一页。

## 来源与取舍

两份课件讲同一件事的地方只保留一份，CUDA 基础编程配合实测演示程序重写，第四部分的 PyTorch 部分据源码与文档新写：

| 内容 | 取自 | 取舍说明 |
| --- | --- | --- |
| 渲染、芯片面积、峰值算力的趋势 | 2-3-gpu 第 4–12 页 | 芯片面积与 ALU 共享控制逻辑两图合为 `gpu-cores` 一页（`chip-area.svg`、`alu-sharing.svg`），渲染管线重画为 `render-pipeline.svg`，GPGPU 一页并入趋势一页的一条文字 |
| CUDA 编程模型、向量加法、块内折半求和 | 2-3-gpu 第 23–39 页、CSP 第 27 页 | 精简后移到 GPU 结构之后，配 `vecadd.cu`、`dot.cu` 实测程序与 `thread-index.svg`、`fold-1.svg` … `fold-7.svg`、`race-1.svg` … `race-5.svg` 三组图，课程服务器与 axpy 练习为新增 |
| 独立与集成 GPU、SM 的部件 | 2-3-gpu 第 13–16 页 | SM 与 Fermi 芯片图重画为 `sm-structure.svg`、`gpu-overview.svg`，独立与集成 GPU 的原照片换成自绘的 `dgpu-igpu.svg`，并在两侧各加一张新近的照片（RTX 5090 显卡与 Lunar Lake 封装），H100 的 SM 图放在 Tensor Core 的 sidebar |
| Warp 的切换 | 2-3-gpu 第 18 页的文字，图按 CSP 第 18 页的调度说明重画（`warp-switch-1.svg` … `warp-switch-9.svg`） | — |
| 分支分化 | 2-3-gpu 第 19 页，重画为 5 帧动画 | CSP 第 20–25 页的同一内容 |
| 存储层次 | CSP 第 28 页的图 | — |
| 线程块分配到 SM | 2-3-gpu 第 41 页与 CSP 第 17 页（同一张图），按 NVIDIA 编程指南重画（`block-to-sm.svg`） | CSP 的图带有外校的标志 |
| PyTorch 的分层、自定义算子 | PyTorch 的源码（`native_functions.yaml`）与文档（Custom C++ and CUDA Operators、`torch.library`、`cpp_extension`） | 新写，两张矢量图（`pytorch-layers.svg`、`custom-op-flow.svg`）。自定义算子的代码取自文档，没有在 GPU 上运行过，核对记在 `lectures/PENDING.md` |

## 课堂演示

`examples/` 下是五个自包含的 `.cu` 文件，其中四个在幻灯片上运行，`vecadd.cu` 只作为源文件查看。幻灯片上的演示不在本机编译：命令把文件提交到课程服务器，页面显示服务器返回的结果。实际执行的命令在 curl 之后接 `sed '/^#/d;/^$/,$d'`，幻灯片上用 `show=` 不显示这一段：它去掉服务器的 `#` 行，并删去第一个空行及其后的全部行，只留下程序的输出（`time` 的三行不显示）。页面上的输出全部录于课程服务器的同一块 GPU，所以各页的数字互相对应（例如峰值带宽 448 GB/s 是 `dot` 读取带宽的参照）。

| 页 | 程序 | 内容 |
| --- | --- | --- |
| 向量内积的 CPU 与 GPU 耗时（46） | `dot.cu` | GPU 上的向量内积（每块在共享内存中折半求和），与同一内积在一个 CPU 核上的循环比较耗时与读取带宽，并核对结果 |
| 练习：axpy 核函数（47） | `axpy.cu` | 核函数体为空的骨架，补全后 `differ 0`，参考实现在助教一侧，不在本目录 |
| 分支分化（71） | `diverge.cu` | 同一 Warp 内按奇偶分支与按 Warp 分支的耗时 |

命令的形式（`$GPU_SERVER` 与 `$GPU_TOKEN` 由 `.env` 注入，见 `docs/usage.md`）：

```bash
cd examples
curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @dot.cu $GPU_SERVER/program | sed '/^#/d;/^$/,$d'
```

输出每个程序连续运行三次，取第一次。

- 演示的数字绑定课程服务器的 GPU（计算能力 8.6，40 个 SM）。服务器更换 GPU 后重新录制。正文中涉及的硬件参数举例（H100、RTX 5090）与练习题采用公开型号。
- 计时在不同运行之间有差别：连续三次的耗时在 3% 以内。
- `dot.cu` 的 `dot_kernel` 使用 10 个寄存器（`nvcc --resource-usage`，服务器的 CUDA 12.8 与 `sm_86` 下复核）。

### 课程服务器

本讲的全部演示与第二部分的 axpy 练习使用 `labs/parallel-dot-server` 的服务器：`POST /program` 接收一个 `.cu` 文件（至多 256 KiB，运行至多 30 s），`/submit` 页面在浏览器中提交。页面的口令不显示在幻灯片上：命令用环境变量 `$GPU_TOKEN`，链接的 `?token=` 在 `pages.py` 中由 `GPU_TOKEN` 读入（环境变量，没有则取项目根目录的 `.env`，`.env.example` 列出变量名）。服务器已由授课人开启（2026-10-30 `GET /submit` 返回 200）。2026-10-30 经 `POST /program` 提交了 `vecadd.cu`（`differ 0`）、`axpy.cu` 骨架（`differ 9900000`）与参考实现（`differ 0`），三次都是 `status=ok`。每个客户端默认每 600 秒至多提交 6 次（`per-client`），全班经同一出口地址提交时要注意这个限额。

## 图

`diagrams/` 下的脚本生成本节自制的图，全部用 `svgkit.py` 绘制（`race.py`、`fold.py`、`warp.py` 共用 `syncfig.py` 的表格，`race.py` 与 `fold.py` 还共用代码框，两组动画的尺寸相同）：

```bash
cd diagrams && sh render.sh
```

`render.sh` 运行每个脚本，再由 `check_bounds.py` 检查文字是否超出画布。生成的 `assets/*.svg` 不手改。

| 脚本 | 图 | 页 |
| --- | --- | --- |
| `chip_area.py` | `chip-area.svg` | 同样面积的芯片中，控制逻辑、缓存与运算单元所占的比例 |
| `alu_sharing.py` | `alu-sharing.svg` | 标量 CPU 核心、SIMD CPU 核心与 GPU 的取指译码与 ALU 的对应 |
| `render_pipeline.py` | `render-pipeline.svg` | 渲染管线，可编程的着色器阶段与固定功能阶段 |
| `gpu_overview.py` | `gpu-overview.svg` | GPU 芯片：SM、L2、DRAM 接口、主机接口与线程块调度器，按 Fermi 白皮书的图重画 |
| `sm_structure.py` | `sm-structure.svg` | SM 的内部：调度器、寄存器堆、CUDA 核心、LD/ST、SFU、共享内存与 L1 |
| `dgpu_igpu.py` | `dgpu-igpu.svg` | 独立 GPU 与集成 GPU 的存储连接，自绘 |
| `gpu_simt.py` | `gpu-simt.svg` | 线程层次（Grid、线程块、Warp、线程）与 Fermi GPU 的对应：线程块调度器、SM 内的两个 Warp 调度器与 32 个核心、显存与主机内存，按 Fermi 白皮书重画 |
| `peak_trend.py` | `peak-trend.svg` | 第 13 页：2006 至 2025 年 GeForce 旗舰 GPU 与 Intel Core 旗舰 CPU（主流桌面）的单精度峰值算力折线图（对数坐标），自绘，数据与算式写在脚本的注释与数据表中 |
| `mem_hierarchy.py` | `mem-hierarchy.svg` | 线程层次与存储空间的对应，按 Xingda Wei 的 GPU 课件的图重画 |
| `thread_index.py` | `thread-index.svg` | 线程组织与全局下标 `i = blockIdx.x * blockDim.x + threadIdx.x` |
| `host_device.py` | `host-device-1.svg` … `host-device-6.svg` | 第 22 页（`p.frames`，6 帧）：主机（CPU、内存）与设备（GPU、显存），第 1 帧是初始状态，其后每帧加一步的箭头：`cudaMalloc` 分配、`cudaMemcpy` 复制输入、启动核函数、`cudaMemcpy` 复制结果、`cudaFree` 释放；当前一步的箭头为全色，前面各步变淡；输入蓝色，结果橙色，尚无内容的数组为虚线框 |
| `fold.py` | `fold-1.svg` … `fold-7.svg` | 第 39–45 页（`p.frames`，7 帧）：带 `__syncthreads()` 的折半求和，同样 8 个线程、每个 Warp 2 个线程。上方是 cache 的各行，下方是线程 0 至 3 各自的一份循环代码，橙色底标出各线程执行到的行，红色底与「等待」标出停在屏障上的线程：Warp 0 先完成 s = 4 的加法，在屏障等待 Warp 1；s = 2 时线程 2、3 不加，先到屏障等待 Warp 0；最后 cache[0] = 31。表格与代码框的几何由 `syncfig.py` 提供，与 `race.py` 相同，两组图切换时不跳动 |
| `warp.py` | `warp-1.svg` … `warp-4.svg` | 第 29–32 页（`p.frames`，4 帧，位于该页文字之下）：线程块只有 1 个 Warp 时的折半求和。左边是一份循环代码（8 个线程共用一个程序计数器，橙色底标出执行到的行），右边是 cache 的各行，每步一条指令，结果 cache[0] = 31，没有屏障。画布 1120 × 292，放在文字下面，所以尺寸与 `race.py`、`fold.py` 不同；每帧的说明写在表格下面，代码框和表格这部分不随帧变化，画布的窗口（`left`）按它们移动，使图水平居中，没有题注 |
| `block_life.py` | `block-life-1.svg` … `block-life-7.svg` | 第 50–56 页（`p.frames`，7 帧，位于该页文字之下）：16 个线程块、2 个 SM。线程块整块分配给 SM 并驻留，每个 SM 的 4 个 Warp 调度器每周期各发射一个 Warp（橙色），B2 完成后释放资源，B12 补入空位。画布 1120 × 368，与其余动画的尺寸无关 |
| `race.py` | `race-1.svg` … `race-5.svg` | 第 33–37 页（`p.frames`，5 帧）：没有屏障的折半求和，假定线程块 8 个线程、每个 Warp 2 个线程（右上角写明，各份代码标出所属的 Warp）。上方是 cache 的各行，Warp 0 的线程 0、1 先于 Warp 1 的线程 2、3 进入 s = 2，读到 cache[2]、cache[3] 的旧值（红色），最后 cache[0] = 23，正确的和是 31；下方是线程 0 至 3 各自的一份循环代码，橙色底标出各线程当前执行到的行和各自的 s。图上没有屏障，屏障在之后的页面引入 |
| `block_to_sm.py` | `block-to-sm.svg` | 线程块分配到 SM，按 NVIDIA CUDA C++ Programming Guide 的 Automatic Scalability 图重画 |
| `warp_switch.py` | `warp-switch-1.svg` … `warp-switch-9.svg` | 第 57–65 页（`p.frames`，9 帧，位于该页文字之下）：左边是 SM（一个 Warp 调度器、4 个 Warp）与显存，读显存画成箭头，数据返回画成反向箭头；右边是与左边逐行对应的时间轴。每帧两个周期，周期 8、9 没有就绪的 Warp。画布 1120 × 372 |
| `divergence.py` | `divergence-1..5.svg` | 分支分化（动画） |
| `diverge_compare.py` | `diverge-compare.svg` | 按线程奇偶与按 Warp 奇偶划分的两个核函数，两个 Warp 上的路径与耗时（第 71 页） |
| `pytorch_layers.py` | `pytorch-layers.svg` | 第 76 页：一次算子调用经过的层，自上而下是 Python API、Python 绑定、ATen、Dispatcher、各 key 的核函数、库与 CUDA 核函数、CUDA 运行时、GPU。橙色标出 `x @ w` 经过的 Autograd 核函数、CUDA 核函数与 cuBLAS |
| `custom_op_flow.py` | `custom-op-flow.svg` | 第 77 页：从核函数到 Python 调用的四步，① 核函数与 C++ 封装，② 声明与注册，③ 编译与加载，④ Python 调用。蓝色是 `.cu` 文件中的步骤，灰色是编译，绿色是 Python，括号标出前两步在同一个文件、后两步在 Python 脚本中 |

同一页上的图共用一套颜色，全讲沿用：橙色是执行或被跟踪的线程、线程块，以及 SM 内的运算部件（ALU、CUDA 核心、LD/ST、SFU、可编程的着色器阶段）。蓝色是其余的线程与线程块。绿色是片外存储（显存、系统内存）。灰色的框是 SM、SM 内的存储、L2 与 CPU 的缓存。紫色是控制逻辑。白底深色边框是芯片、封装、主机接口与固定功能阶段。灰色虚线是等待。PyTorch 的分层图中，蓝色是 Python，灰色是 C++ 的各层，紫色是 Dispatcher，橙色是 `x @ w` 的调用经过的核函数与库，白底深色边框是 GPU。自定义算子的步骤图中，蓝色是 `.cu` 文件，灰色是编译，绿色是 Python。

其余插图来自两份课件与 PyTorch 文档，出处写在各页的 `.footnote(...)`：

- NVIDIA 的文档：`h100-sm.png`（H100 白皮书）。`flops-trend.png`（CUDA C++ Programming Guide，2014 年版）数据过旧，已不被页面引用；
- Henrik Wann Jensen：`teapot-mesh.png`、`teapot-render.png`；
- Wikimedia Commons 的照片（2026-10-31 取得，页脚写明作者与授权）：`dgpu-rtx5090.jpg`（Palit GeForce RTX 5090 GameRock，PantheraLeo1359531，CC BY 4.0，原图为 `Palit GeForce RTX 5090 Gamerock 20250530 HOF3879 RAW-Export.png`）、`igpu-lunarlake.jpg`（Intel Core Ultra 200V 封装，FritzchensFritz，CC0，原图为 `…LunarLake…DSC04588DSC04588.jpg`，裁去四周的背景）；
- 第 13 页的三个项目标志（图片裁掉多余的白边，三张都是 504 × 252 的白底画布，标志居中，按宽度等比缩放，不拉伸，无边框，页面上每个标志下带斜体的项目名称）：`gpgpu-opencl.png`（Wikimedia Commons 的 `OpenCL logo.svg`）、`gpgpu-lerobot.png`（`huggingface/lerobot` 仓库的 `media/readme/lerobot-logo-thumbnail.png`，仓库为 Apache-2.0，横幅形状）、`gpgpu-ggml.png`（`ggml-org/media` 仓库的 `logo/ggml-logo.jpg`，ggml 仓库为 MIT）；
- `ue5.gif`（脚注写 Epic Games 的演示）。按旧课件重画的图（`render-pipeline.svg`、`mem-hierarchy.svg`）和 `alu-sharing.svg` 不在页面上注明旧课件。

`assets/` 中的 `cpu-vs-gpu.png`、`gpu-cores.png`、`render-steps.png`、`fermi-gpu.png`、`fermi-sm.png`、`dgpu.png`、`igpu.png`、`dgpu-card.png`、`igpu-die.png`、`mem-hierarchy.png`、`grid-2d.png` 已由上表的矢量图取代，页面不再引用。`dgpu-card.png` 与 `igpu-die.png` 是照片，不再使用。文件保留在原处，待讲者确认后删除（见 `lectures/PENDING.md`）。

`ue5.gif` 约 6 MB。

## 用语与排版约定

与其他讲相同，改这一讲时请一并维持：

- **用语**：采用大学课堂讲义的规范学术表述，语句自然连贯，不使用比喻、口语化措辞与「不是……而是……」句式。
- **页标题**：用简短的名词短语概括本页的核心知识点，不使用完整句子。连续多页讲解同一知识点时使用完全相同的标题，由框架在大纲中自动合并折叠。幻灯片、讲稿与图注不回指前文，也不指向后文，不引用其他讲。先于讲解出现的术语在出现处用一句话说明。
- **代码**：`p.code`、`p.demo` 不翻译，注释用英文。例子的源文件只用 ASCII。
- **演示**只用于运行的程序。页面上的代码节选用 `p.code`，节选所在的源文件用文件按钮 `[name](file:examples/…)` 或 `files=` 给出。
- **粗体后面的全角冒号**写在 `**` 之外。一页至多一个 `p.highlight`（本节没有用）。
- **图的出处脚注**挂在图块上（`p.image`、`p.row`、`p.side_image`），不挂在同页的 `slide`，脚注标记因此落在图上。侧图的标记画在图列靠文字一侧的顶角，图在列内居中时标记比图略高。
- 修改之后用单独的输出目录渲染检查，`--pages` 只渲染一部分时不要写到整套的目录中：

```bash
python3 -m lecturekit.cli render lectures/5-gpu --to png --out build/5gpu-check
```

1280 × 720 下内容不越过 y = 700。图与文字合计超过页高时，页面会把图的框压缩，图与下方的文字重叠，这时缩小图的宽度、改为侧图（`p.side_image`）或删减文字。侧图页的标题只占左栏，要短到一行。
