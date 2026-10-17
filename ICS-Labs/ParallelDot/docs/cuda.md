# parallel-dot 第二步：CUDA

这是实验 `parallel-dot` 的第二步，不计分；实验的总览在上一级目录的 [README.md](../README.md) 中，第一步（SIMD）的说明在 [`simd.md`](simd.md) 中。

这一步的程序仍是第三讲的 `mini-ollama`，读取同一个模型文件，回答同一个问题；它的算子改为在 GPU 上运行。`ops/scalar.c` 是第三讲 `mini_ollama.c` 中的运算，在 CPU 的一个线程上每次处理一个元素；`ops/cuda.cu` 是同一组算子的 CUDA 版本，每个算子是一个 kernel。`cuda.cu` 中只有一个 kernel 的函数体没有写：Q4_0 矩阵与向量的乘积 `matvec_q4_0_kernel`，你来实现它。启动它的主机端函数是 `matvec_q4_0`，与第一步你写的函数同名，计算同一个乘积。这个 kernel 用讲义 `dot_cuda.cu` 中的块内折半求和。

## 分工

框架提供读取 GGUF 文件、分词、逐个 Token 生成的循环、显存的分配与拷贝、两套算子中除 `matvec_q4_0_kernel` 以外的全部函数，以及检查程序。你只修改 [ops/cuda.cu](../ops/cuda.cu) 中这个 kernel 的函数体，它初始时是一个 `todo()` 调用，函数上方的 `TODO` 注释给出启动配置与每个线程的分工。启动 kernel 的主机端函数与启动配置都已写好，你不修改它们。

[ops.h](../ops.h) 是框架与算子之间的接口：`struct ops` 中每个函数指针是一个算子或一项显存操作，注释写明它做什么；Q4_0 块的排布与读出缩放系数的函数 `q4_0_scale` 也在其中，`q4_0_scale` 在 CPU 与 GPU 上都可以调用。框架只通过这张表调用算子，你不修改这个文件。完成这一步你需要阅读 `ops.h`、`ops/cuda.cu` 与本说明，不需要阅读顶层的其他源文件。

| 算子 | 计算 | kernel 与启动配置 | 在 `cuda.cu` 中 |
| --- | --- | --- | --- |
| `matvec` | 矩阵向量乘 $y = Wx$，$W$ 为 Q4_0 | `matvec_q4_0` 启动 `matvec_q4_0_kernel<<<rows, 256>>>`：一个块算一行 | **kernel 你来实现** |
| `rmsnorm` | 若干个向量各自缩放到均方根为 1，再乘以增益 | `rmsnorm_kernel<<<n_vec, 256>>>`：一个块处理一个向量，折半求平方和 | 已给出 |
| `embed` | 把一行 Q4_0 权重还原为 float | `embed_kernel<<<n / 32, 32>>>`：一个线程还原一个权重 | 已给出 |
| `rope` | 按位置旋转每个向量的各对分量 | `rope_kernel<<<n_vec, d / 2>>>`：一个线程旋转一对分量 | 已给出 |
| `attn_scores` | 每个查询头与之前每个 Token 的键的内积 | `attn_scores_kernel`：一个线程算一个分数，查询先拷入共享内存 | 已给出 |
| `softmax` | 每一行分数变为和为 1 的权重 | `softmax_kernel<<<n_row, 256>>>`：一个块处理一行，两次折半求最大值与和 | 已给出 |
| `attn_values` | 按权重累加之前每个 Token 的值 | `attn_values_kernel<<<n_head, d>>>`：一个线程算一个输出元素 | 已给出 |
| `swiglu` | 前馈网络的门控 `silu(gate) · up` | `swiglu_kernel`：一个线程处理一个元素 | 已给出 |
| `add` | 残差相加 | `add_kernel`：一个线程处理一个元素 | 已给出 |
| `argmax` | 最大的分数所在的位置，即下一个 Token | `argmax_kernel<<<1, 1024>>>`：一个块折半求最大值与它的位置 | 已给出 |

已给出的 kernel 你可以当作例子阅读：`rmsnorm_kernel` 中每个线程先累加每第 256 个元素的平方、再在共享内存中折半求和的写法，正是你的 kernel 要用的，`softmax_kernel` 用同样的写法折半两次；`embed_kernel` 中从块中取出一个编码的写法，`matvec_q4_0_kernel` 也要用到。

| 程序 | 作用 |
| --- | --- |
| `ops-selftest-cuda` | 不需要模型文件。检查 `cuda.cu` 中每个算子的结果，然后比较 `matvec` 在 GPU 与 CPU 上的速度 |
| `mini-ollama-cuda` | 读取模型文件，用 `cuda.cu` 中的算子回答 `prompt.txt` 中的问题，输出生成速度 |
| `mini-ollama-scalar` | 同上，用 `scalar.c` 中的算子，在 CPU 的一个线程上运行；第一步也用它 |

本文中的命令你都在 `parallel-dot/` 目录下执行。

## 数据在显存中

GPU 上的 kernel 只能读写显存。`mini-ollama-cuda` 在生成第一个 Token 之前，用算子表中的 `upload` 把全部权重（974 MB）拷入显存，用 `alloc` 在显存中分配键值缓存、分数与一层计算用到的每个向量；此后每一项计算的输入与输出都在显存中，每生成一个 Token，CPU 只从 GPU 取回 `argmax` 得到的一个整数。CPU 上的两套算子（`scalar.c`、`simd.c`）的 `upload` 直接返回原来的地址，`mini-ollama-scalar` 与 `mini-ollama-simd` 用同一段主循环在内存中运行。

kernel 的启动在 kernel 运行之前就返回，CPU 接着启动下一个 kernel，GPU 按启动的次序依次运行它们。`finish` 等待已启动的全部 kernel 结束；`mini-ollama-cuda` 与 `ops-selftest-cuda` 在读时钟之前调用它，`argmax` 与 `download` 在取回结果时也等待 kernel 结束。

## 你要写的 kernel

```c
__global__ void matvec_q4_0_kernel(float *y, const uint8_t *w, const float *x, int cols);
```

主机端函数 `matvec_q4_0` 以 `<<<rows, THREADS>>>` 启动它，`THREADS` 为 256：第 `blockIdx.x` 个块计算 `y[blockIdx.x]`，即矩阵的这一行与 `x` 的内积。这一行从 `w + row · (cols / 32 · 18)` 开始，是 `cols / 32` 个 Q4_0 块（排布见 `ops.h` 或第一步的说明）。

1. 线程 `t` 在寄存器中累加第 `t, t + 256, t + 512, …` 列的乘积 `W[row][c] · x[c]`。第 `c` 列在这一行的第 `c / 32` 个块中，它的编码在这个块的第 `2 + c % 16` 个字节：`c % 32 < 16` 时是低 4 位，否则是高 4 位；权重是 `q4_0_scale(块) · (编码 − 8)`。
2. 每个线程把自己的部分和写入共享数组的第 `t` 个元素，块内 256 个部分和折半相加：每一步前一半的线程把后一半的元素加到自己的元素上，`__syncthreads()` 之后进行下一步，与讲义 `dot_cuda.cu` 中的 `dot_kernel` 相同，`rmsnorm_kernel` 中也是这样写的。
3. 线程 0 把总和写入 `y[row]`。

同一个 warp 的 32 个线程处理相邻的 32 列，即同一个 Q4_0 块：它们读的是相邻的字节与相邻的 float。

这个 kernel 与第一步的 `matvec_q4_0` 计算同一个乘积。第一步用一个循环依次处理每一行，一行之内用 8 路累加器、最后由 `hsum8` 相加；这里 `rows` 个线程块各算一行，一行之内 256 个线程各自累加、最后在共享内存中折半相加。

## 两种运行方式

| 机器 | 做法 |
| --- | --- |
| 有 NVIDIA 显卡与 CUDA Toolkit | `make test-cuda`、`make run-cuda`：在本机编译与运行 |
| 没有 NVIDIA 显卡 | `make remote`：把 `ops/cuda.cu` 发送到课程服务器，服务器编译、运行 `ops-selftest-cuda` 与 `mini-ollama-cuda`，返回两者的输出；本机只需要 Python 3 |

课程服务器暂时没有启动，`make remote` 与本文中向课程服务器提交的命令（`tools/submit.py`、curl、`/submit` 页面）现在都不能使用。你没有 NVIDIA 显卡、确实需要运行这一步时，联系助教协商。

### 在本机运行

你需要 gcc、GNU make、CUDA Toolkit、NVIDIA 显卡与驱动，显存 2 GB 以上（权重 974 MB，`ops-selftest-cuda` 的最大矩阵 175 MB）。`nvcc` 默认在 `/usr/local/cuda/bin`，装在别处时你写 `make CUDA_HOME=/path/to/cuda`。这一步在 Linux、gcc 15.2、CUDA 13.4 上验证过。

```sh
make cuda         # 生成 mini-ollama-cuda 与 ops-selftest-cuda
make test-cuda    # 编译，然后运行 ./ops-selftest-cuda
make run-cuda     # 用 cuda.cu 中的算子回答 prompt.txt 中的问题
make run-scalar   # 用 scalar.c 中的算子回答同一个问题
make clean        # 删除全部程序与 build/ 下的目标文件
```

找不到 `nvcc` 时 `make cuda` 报告它查找的路径，以及两个办法：安装 CUDA Toolkit（或给出 `CUDA_HOME`），或者改用 `make remote`。

### 通过课程服务器运行

课程给出服务器的地址与口令。你把两者写入环境变量 `GPU_SERVER` 与 `GPU_TOKEN`（讲义「远程运行」一页的 curl 命令用的是同两个变量），然后：

```sh
export GPU_SERVER=https://主机:端口
export GPU_TOKEN=口令
make remote       # 提交 ops/cuda.cu，打印服务器返回的内容
```

`make remote SERVER=https://主机:端口` 可以代替 `GPU_SERVER`；两者都没有时 `make` 报错。`make remote` 运行的是客户端 `tools/submit.py`，你的机器没有安装 `make` 时直接运行它：

```sh
python3 tools/submit.py
```

服务器把你的 `cuda.cu` 放入实验目录的一份新拷贝，运行其中的 `sh run.sh`：编译这一步的两个程序，运行 `ops-selftest-cuda`，再用服务器上的模型文件运行 `mini-ollama-cuda`。返回的内容就是这些步骤的输出，与本机运行 `sh run.sh` 相同。客户端的参数、退出码与服务器的限制见下文「课程服务器的客户端」一节。

### Windows

- **没有 NVIDIA 显卡**：只需要 Python 3。你在 Git Bash 中执行命令，其中 `python3` 不存在时命令名是 `python`：`make remote PYTHON=python`（需要 GnuWin32 make，安装方法见第一步的「Windows 上的准备」），或者直接运行 `python tools/submit.py`。
- **有 NVIDIA 显卡**：你需要 CUDA Toolkit for Windows 与 Visual Studio Build Tools 中的 `cl.exe`。Windows 上的 nvcc 只与 `cl.exe` 配合，gcc 与 `cl.exe` 生成的目标文件不能链接在一起，因此 `Makefile` 在 Windows 上用 nvcc 编译 `build/cuda/` 下的全部目标文件（`.c` 文件由 nvcc 交给 `cl.exe`）；第一步的程序仍由 gcc 编译，目标文件在 `build/` 的另外两个子目录中。你在「x64 Native Tools Command Prompt」中启动 Git Bash 再运行 `make cuda`；`Makefile` 从 CUDA 安装程序设置的 `CUDA_PATH` 找 nvcc。**Windows 下本机的 GPU 构建未经实测**。你也可以在 WSL2（Ubuntu）中按 Linux 的方式完成，同样未经实测。

## 编译与检查

| 文件 | 编译器与选项 | 作用 |
| --- | --- | --- |
| `ops/cuda.cu` | `nvcc -O2 -arch=native` | 为本机显卡生成代码；`make cuda ARCH=sm_86` 指定架构 |
| `ops/scalar.c` | `gcc -O2 -fno-tree-vectorize` | GCC 12 起在 `-O2` 下自动向量化部分循环，这个选项使标量算子保持每次处理一个元素 |
| 其余 `.c` 文件 | `gcc -O2` | `main.c` 与 `selftest.c` 另加 `-DOPS=ops_cuda`，指明程序使用的算子表 |
| 链接 | `nvcc` | 链接 CUDA 运行时库 |

目标文件放在 `build/cuda/` 下。

`ops-selftest-cuda` 分三组，前两组每一项输出一行 `pass` 或 `FAIL`，最后输出 `all passed` 或 `N failed`，退出码为失败的项数。输入在 CPU 上由固定种子的伪随机数生成，`upload` 到显存，结果 `download` 回来比较。第一组不启动你的 kernel；你还没有实现它时，程序在第二组第一次启动 `matvec_q4_0_kernel` 之后输出 `matvec_q4_0_kernel is not implemented yet` 并以状态 3 退出。

**第一组「operators」**检查已给出的 kernel：每个算子与 `scalar.c` 中的同名算子处理同一组输入，大小取模型中出现的值以及其他值（如 3 个长度 20 的向量、13 个元素，`rmsnorm` 的 3 × 100 与 2 × 1000 检查长度小于 256 与不是 256 的倍数的向量），`rmsnorm` 的每种形状还原地再运行一次（`y` 等于 `x`）；两个结果之差不超过输出中最大绝对值的 10⁻⁵ 即通过。这一组在你开始之前就全部通过。

**第二组「matvec」**检查你的 kernel，矩阵为 64 × 32、64 × 96、1000 × 2048（两组，其中一组 `x` 的量级为 64）与 64 × 6144，每一行的期望值按 `ops.h` 中的定义用 double 计算，差不超过这一行 `Σ |W[r][c] · x[c]|` 的 10⁻⁵ 即通过；第 0 行的 16 个字节是 `j | (15 − j) << 4`、`d = −1`，第 1 行的 `d` 有 0 与次正规数。

**第三组「speed」**用模型中的四种矩阵比较 `matvec_q4_0`：GPU 上你的 kernel，与 CPU 一个线程上的 `scalar.c`。每种至少运行 0.2 秒，取 3 次中最快的一次，输出每次矩阵向量乘的毫秒数、每秒读取的权重字节数（`GB/s`）与加速比。这一组不设门限。

你的 kernel 正确时的输出类似：

```
operators: cuda (ops/cuda.cu)
device: NVIDIA GeForce RTX 3060 Laptop GPU, 30 SMs, 5.7 GiB, peak 336 GB/s
1. operators against ops/scalar.c
  pass  embed: 32 and 2048 weights, error 0.0e+00 of the largest |value|
  pass  rmsnorm: 1 x 2048, 16 x 128, 8 x 128, 3 x 100, 2 x 1000, also in place, error 2.2e-07 of the largest |value|
  pass  rope: 16 x 128 at positions 0 and 511, 8 x 128 at 37, 3 x 20, error 9.3e-08 of the largest |value|
  pass  attn_scores: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 2.1e-07 of the largest |value|
  pass  softmax: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 4.3e-07 of the largest |value|
  pass  attn_values: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 1.6e-07 of the largest |value|
  pass  softmax: 37 scores between -100 and 100, error 2.2e-09 of the largest |value|
  pass  swiglu: 6144 and 13 values between -20 and 20, one at -100, error 1.2e-08 of the largest |value|
  pass  add: 2048 and 13 values, error 0.0e+00 of the largest |value|
  pass  argmax: 151936 values, two equal largest, the largest first and last
1. matvec against the definition
  pass  matvec, 64 x 32: error 4.3e-08 of the sum of |terms|
  pass  matvec, 64 x 96: error 6.8e-08 of the sum of |terms|
  pass  matvec, 1000 x 2048: error 3.2e-08 of the sum of |terms|
  pass  matvec, 1000 x 2048, x up to 64: error 2.8e-08 of the sum of |terms|
  pass  matvec, 64 x 6144: error 1.0e-08 of the sum of |terms|
1. speed of matvec: cuda against the scalar operator on one CPU thread
  matrix           rows x cols          MB  scalar ms  GB/s    cuda ms  GB/s  speed-up
  attn_q           2048 x 2048         2.4    3.453     0.7    0.047    50.0     73.2x
  ffn_gate, ffn_up 6144 x 2048         7.1   10.302     0.7    0.155    45.8     66.6x
  ffn_down         2048 x 6144         7.1   10.340     0.7    0.142    49.9     72.8x
  output           151936 x 2048     175.0  260.242     0.7    3.787    46.2     68.7x

all passed
```

首行是这次检查的一套算子，`device` 一行是显卡的名称、SM 数、显存与峰值带宽（显存时钟 × 2 × 总线宽度）。`error` 是各项中最大的相对误差，毫秒数与 `GB/s` 因机器而异。

## 在真实模型上运行

模型文件与第一步相同，是本目录下的 `q4_0.gguf`：你从交大云盘下载它，或者把机器上已有的这个文件移动或链接到本目录，做法见上一级目录 [README.md](../README.md) 的「模型文件」一节。文件放在别处时你写 `make run-cuda MODEL=路径`。用 `make remote` 时你不需要模型文件，服务器用它自己的一份。

```bash
make run-cuda     # ./mini-ollama-cuda MODEL.gguf prompt.txt
make run-scalar   # ./mini-ollama-scalar MODEL.gguf prompt.txt
```

回答写到标准输出，六行统计写到标准错误，含义与第一步相同：`device` 是算子运行的设备，`weight bandwidth` 是每秒读取的 Q4_0 权重字节数，每个权重 18/32 字节，生成一个 Token 读取全部权重一次。

你的 kernel 正确时 `make run-cuda` 的输出（同上机器，模型文件已在页缓存中）：

```
我是一个虚拟助手，没有实体，但我可以为你提供帮助和解答。
operators:            cuda (ops/cuda.cu)
device:               NVIDIA GeForce RTX 3060 Laptop GPU, 30 SMs, 5.7 GiB, peak 336 GB/s
prompt eval rate:     56.75 tokens/s
eval rate:            44.03 tokens/s
matvec rate:          154.50 GFLOPS
weight bandwidth:     43.5 GB/s
```

`make run-scalar` 的回答相同，统计为：

```
operators:            scalar (ops/scalar.c)
device:               CPU, 1 thread
prompt eval rate:     0.82 tokens/s
eval rate:            0.75 tokens/s
matvec rate:          2.49 GFLOPS
weight bandwidth:     0.7 GB/s
```

两套算子的求和次序不同，各项结果的最后几位有差别；这个差别可能改变某一步分数最高的 Token，使回答从那一步起不同，这与你的 kernel 是否正确无关。你的 kernel 是否正确以 `ops-selftest-cuda` 为准。

### 问题

- `eval rate` 一行，GPU 是 CPU 一个线程的多少倍？第一步中 AVX2 的算子是多少倍？
- `weight bandwidth` 与 `device` 一行的峰值带宽比较，占百分之几？`ops-selftest-cuda` 第三组中四种矩阵的 `GB/s` 与它接近吗？

## 课程服务器的客户端

`make remote` 调用的客户端是 `tools/submit.py`：它把 `ops/cuda.cu` 打包发送到课程服务器，再把服务器返回的每一行原样打印。它只用 Python 标准库，你的机器上不需要 CUDA、gcc 或 make，Windows、macOS 与 Linux 相同。

```sh
python3 tools/submit.py --server https://服务器:端口 ops/cuda.cu
```

参数是 `cuda.cu` 或存放它的目录，默认为 `ops/cuda.cu`；只有名为 `cuda.cu` 的文件会被发送。`--server` 省略时取环境变量 `GPU_SERVER`，再省略时为 `http://localhost:8080`。服务器要求口令时你写 `--token 口令`，或设置环境变量 `GPU_TOKEN`。Git Bash 中 `python3` 不存在时命令名是 `python`。

服务器返回的内容与本机 `sh run.sh` 的标准输出和标准错误合并后相同：以 `# ` 开头的行是服务器的状态（收到的文件、排队位置、开始、结束）与失败的步骤，以 `run ` 开头的行是 `run.sh` 的步骤，其余是编译器与两个程序的输出。一次提交的输出示例（2026-10-17，服务器与 RTX 3060 Laptop GPU 在同一台机器上，省略了 `ops-selftest-cuda` 中间的各行；`# queued` 一行只在排队时出现）：

```text
# received cuda.cu
# started limit=180s
run make
run ops-selftest-cuda
operators: cuda (ops/cuda.cu)
device: NVIDIA GeForce RTX 3060 Laptop GPU, 30 SMs, 5.7 GiB, peak 336 GB/s
1. operators against ops/scalar.c
  pass  embed: 32 and 2048 weights, error 0.0e+00 of the largest |value|
...
all passed
run mini-ollama-cuda
我是一个虚拟助手，没有实体，但我可以为你提供帮助和解答。
operators:            cuda (ops/cuda.cu)
device:               NVIDIA GeForce RTX 3060 Laptop GPU, 30 SMs, 5.7 GiB, peak 336 GB/s
prompt eval rate:     54.94 tokens/s
eval rate:            43.01 tokens/s
matvec rate:          150.44 GFLOPS
weight bandwidth:     42.3 GB/s
# done status=ok elapsed=9.4
```

你的 kernel 尚未实现时，输出中有 `matvec_q4_0_kernel is not implemented yet` 一行，最后是 `# run.sh: exit status 3` 与 `# done status=error`。

### 退出码

| 退出码 | 含义 |
| --- | --- |
| 0 | 服务器报告 `status=ok`：`ops-selftest-cuda` 全部通过，`mini-ollama-cuda` 正常结束 |
| 1 | 作业中有程序以非零状态退出：编译错误、某个 kernel 尚未实现（`... is not implemented yet`），或 `ops-selftest-cuda` 有 `FAIL` |
| 2 | 作业被服务器终止：超过时间限制（`status=timeout`）或输出超过限制（`status=output-limit`） |
| 3 | 服务器拒绝了提交（文件不合规、过大、提交过于频繁、队列已满、口令错误），或无法连接服务器。原因打印在标准错误 |

### 服务器的限制

服务器同时运行的作业数有上限，其余作业排队；队列满时服务器拒绝提交，你稍后重试。几个作业同时运行时它们共用 GPU，输出中的时间随服务器的负载变化。每个作业有时间限制（默认 180 秒），你的 kernel 死循环时作业被终止，返回 `status=timeout`。服务器可以限制每个客户端地址在一段时间内的提交次数，`per-client=off` 表示不限制。具体数值在 `GET /` 的回复中：

```sh
curl https://服务器:端口/
```

`submit.py` 之外你也可以用 tar 与 curl 提交，返回的内容相同：

```sh
tar cf - -C ops cuda.cu | curl -sS --data-binary @- -H 'Content-Type: application/x-tar' -H "X-Token: 口令" https://服务器:端口/run
```

### 运行单个 CUDA 程序

讲义中的完整程序（一个含 kernel 与 `main` 的 `.cu` 文件，如 `dot_cuda.cu`）你用 curl 直接发送给服务器，服务器编译、运行后返回它打印的内容：

```sh
curl -sS -N -H "X-Token: 口令" --data-binary @dot_cuda.cu https://服务器:端口/program
```

`-N` 让 curl 逐行打印服务器的回复；服务器没有设置口令时你省略 `-H "X-Token: 口令"`。讲义「远程运行：把 CUDA 程序提交到课程服务器」一页把地址与口令写成环境变量：`curl -sS -N -H "X-Token: $GPU_TOKEN" --data-binary @dot_cuda.cu $GPU_SERVER/program`，与 `submit.py` 读取的 `GPU_SERVER`、`GPU_TOKEN` 是同两个变量。Windows 上你在 Git Bash 中运行这条命令（curl 随 Git for Windows 安装）。文件至多 256 KiB；服务器用 `nvcc -O2` 编译，运行时没有参数，标准输入为空。回复的格式与 `/run` 相同：

```text
# received program.cu (2668 bytes)
# started limit=30s
blocks  65536 x 256 threads
sum     41943040
check   41943040

real	0m0.468s
user	0m0.058s
sys	0m0.348s
# done status=ok elapsed=1.3
```

服务器在 `time` 下运行你的程序：程序的输出之后、`# done` 之前，是一个空行与 `real`、`user`、`sys` 三行，即程序运行的时间，不含编译（与终端中 `time ./dot_cuda` 的输出相同）。你也可以在浏览器中打开 `https://服务器:端口/submit`，填入口令、选择文件提交，回复逐行显示在页面上。

编译错误时 nvcc 的信息原样返回，随后是 `# program: exit status N` 与 `# done status=error`；程序超过时间限制（默认 30 秒）时 `status=timeout`，输出超过限制时 `status=output-limit`。
