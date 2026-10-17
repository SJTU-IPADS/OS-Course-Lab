# parallel-dot

第三讲第五部分给出提高运算速率的两个方案：一条指令完成多次乘加（SIMD），使用更多的运算单元（GPU）。这个实验分两步，每一步对应一个方案，你按顺序完成。两步的程序都是第三讲的 `mini-ollama`：它读取 Q4_0 格式的模型文件 `q4_0.gguf`，逐个 Token 生成回答，每一项计算都是一个算子的调用。

| 步骤 | 内容 | 你要写的代码 | 运行的机器 | 说明 |
| --- | --- | --- | --- | --- |
| 第一步：SIMD | 算子的 AVX2 版本，与标量版本比较生成速度 | `ops/simd.c` 中的函数 `matvec_q4_0` | 你的机器，CPU 支持 AVX2 | [`docs/simd.md`](docs/simd.md) |
| 第二步：CUDA | 算子的 CUDA 版本，与 CPU 上的标量版本比较生成速度 | `ops/cuda.cu` 中的 kernel `matvec_q4_0_kernel` | 有 NVIDIA 显卡的机器，或课程服务器 | [`docs/cuda.md`](docs/cuda.md) |

两步都是课后练习，不计分。两步写的是同一个算子：Q4_0 格式的矩阵与一个 float 向量的乘积 `matvec_q4_0`，它占生成时间的绝大部分；其他算子都已写好。本目录的顶层是 `mini-ollama` 的源码：`main.c` 是生成的循环，`ops.h` 是算子表 `struct ops`，`main.c` 只通过这张表调用算子。`ops/` 下是算子的三种实现，每种一个文件：

| 文件 | 算子表 | 实现方式 |
| --- | --- | --- |
| `ops/scalar.c` | `ops_scalar` | 第三讲 `mini_ollama.c` 中的运算，在 CPU 的一个线程上每次处理一个元素 |
| `ops/simd.c` | `ops_simd` | AVX2 intrinsics，在 CPU 的一个线程上每次处理 8 个 float |
| `ops/cuda.cu` | `ops_cuda` | 每个算子是一个 CUDA kernel，在 GPU 上运行 |

一个程序只含一套算子，程序名的后缀写明是哪一套：

| 程序 | 算子 | 作用 |
| --- | --- | --- |
| `mini-ollama-scalar` | `ops/scalar.c` | 读取模型文件，回答 `prompt.txt` 中的问题，输出生成速度；两步比较的基准 |
| `mini-ollama-simd` | `ops/simd.c` | 同上，第一步 |
| `mini-ollama-cuda` | `ops/cuda.cu` | 同上，第二步 |
| `ops-selftest-simd` | `ops/simd.c`，以 `ops/scalar.c` 为参照 | 不需要模型文件。检查 `simd.c` 中每个算子的结果，比较 `matvec` 与标量版本的速度 |
| `ops-selftest-cuda` | `ops/cuda.cu`，以 `ops/scalar.c` 为参照 | 同上，检查 `cuda.cu` |

`make` 把 `main.c` 与 `selftest.c` 按每套算子各编译一次，目标文件放在 `build/scalar/`、`build/simd/`、`build/cuda/` 下，程序写在本目录。两步的命令你都在本目录下执行。

## 模型文件

三个 `mini-ollama`（`make run-scalar`、`make run-simd`、`make run-cuda`）与 `make run-ollama` 读取模型文件 `q4_0.gguf`：Qwen3-VL-2B-Instruct 的语言模型，每个矩阵为 Q4_0 格式，974 196 320 字节。第二讲实验 `nano-quant` 以 `--recipe q4_0` 生成的同名文件也可以使用；你没有做过 `nano-quant` 时直接下载。两个检查程序与 `make remote` 不需要它。

你把它放在本目录下，文件名为 `q4_0.gguf`，两种方式任选一种：

- 从交大云盘下载：<https://pan.sjtu.edu.cn/web/share/a853ce437a176a0064df9a5af1942ecf>，把下载的 `q4_0.gguf` 移动到本目录。
- 你的机器上已有这个文件时（例如 `nano-quant` 实验中生成的 `../nano-quant/model/q4_0.gguf`），把它移动到本目录，或者在本目录建立指向它的链接：

```bash
mv ../nano-quant/model/q4_0.gguf .       # 移动
ln ../nano-quant/model/q4_0.gguf .       # 硬链接：两个路径须在同一个分区上，不占额外的磁盘空间
ln -s ../nano-quant/model/q4_0.gguf .    # 符号链接：Linux 与 WSL2
```

`ls -lL q4_0.gguf` 显示的大小应当是 974196320。`.gitignore` 忽略本目录下的 `*.gguf`。文件放在别处时你在命令中给出它的路径，例如 `make run-simd MODEL=路径`；`make run-ollama` 所用的 `Modelfile` 只读取本目录下的 `q4_0.gguf`。缺少模型文件时，`make run-*` 在编译之前报告 `no model file q4_0.gguf` 并退出。

## 第一步：SIMD

`ops/simd.c` 中的算子每次处理 8 个 float，除 Q4_0 矩阵与向量的乘积以外都已写好。你要写的是这个乘积：

```c
static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols);
```

它对矩阵的每一行计算这一行与 `x` 的内积，写入 `y[r]`；内积每一步处理一个块的 32 个权重。

```sh
make              # 生成 mini-ollama-scalar、mini-ollama-simd 与 ops-selftest-simd，只需要 gcc
make test-simd    # 检查每个算子的结果，并与标量版本比较速度；不需要模型文件
make run-scalar   # 用标量算子回答 prompt.txt 中的问题
make run-simd     # 用 AVX2 算子回答同一个问题
```

`make test-simd` 的最后一行是 `all passed` 时这一步完成：你的函数的结果与定义一致，速度达到标量版本 `matvec_q4_0` 的 4 倍以上。Q4_0 块的排布、函数的规则、你可以使用的 intrinsics 与期望输出见 [`docs/simd.md`](docs/simd.md)。

## 第二步：CUDA

`ops/cuda.cu` 中的每个算子是一个 kernel；`mini-ollama-cuda` 先把全部权重拷入显存，此后每一项计算都在 GPU 上进行。主机端函数 `matvec_q4_0` 已写好，它启动 kernel `matvec_q4_0_kernel`，你要写这个 kernel 的函数体：

| kernel | 启动配置 | 做法 |
| --- | --- | --- |
| `matvec_q4_0_kernel` | 一个块算矩阵的一行，256 个线程 | 每个线程累加每第 256 列的乘积，块内在共享内存中折半求和，线程 0 写入 `y[row]` |

```sh
make cuda         # 有 NVIDIA 显卡：生成 mini-ollama-cuda 与 ops-selftest-cuda，需要 nvcc
make test-cuda    # 检查每个算子的结果，比较 matvec 在 GPU 与 CPU 上的速度
make run-cuda     # 用 GPU 上的算子回答 prompt.txt 中的问题
make remote       # 没有 NVIDIA 显卡：把 ops/cuda.cu 提交到课程服务器，服务器运行上面两步并返回输出；
                  # 服务器的地址与口令在环境变量 GPU_SERVER、GPU_TOKEN 中，本机只需要 Python 3
```

课程服务器暂时没有启动，`make remote` 现在不能使用；如果你没有 NVIDIA 显卡、确实需要运行这一步时，联系助教协调服务开放时间。

`make test-cuda`（或 `make remote` 的输出中）最后一行是 `all passed` 时这一步完成。kernel 的分工、两种运行方式、输出与参考结果见 [`docs/cuda.md`](docs/cuda.md)；提交到服务器的客户端 `tools/submit.py` 的参数、退出码与服务器的限制见同一文件的「课程服务器的客户端」一节。

## 两步的联系

- 三个 `mini-ollama` 运行同一份主循环、同一个模型、同一个问题，输出的 `eval rate` 可以直接比较：标量算子在 CPU 的一个线程上，第一步的算子在同一个线程上每次处理 8 个 float，第二步的算子在 GPU 的全部 SM 上。
- 两步计算同一个乘积，逐行的方式不同：第一步的 `matvec_q4_0` 用一个循环依次处理每一行；第二步的 `matvec_q4_0_kernel` 中一个线程块处理一行，全部行由启动的 `rows` 个线程块分担。
- 一行之内，第一步用 8 路累加器，每一路累加每第 8 个乘积；第二步中每个线程累加每第 256 列的乘积。
- 一行的最后，第一步用 `hsum8` 把 8 路累加器相加为一个数，第二步的块内折半求和把 256 个数相加为一个数，两者都是每一步两两相加，使数的个数减半。

## 目录

| 路径 | 内容 |
| --- | --- |
| `ops/simd.c` | 第一步：AVX2 版本的算子；你要实现其中的 `matvec_q4_0`，初始时它报告尚未实现并退出 |
| `ops/cuda.cu` | 第二步：CUDA 版本的算子；你要实现其中的 `matvec_q4_0_kernel`，初始时它报告尚未实现并使程序退出 |
| `ops/scalar.c` | 标量版本的算子：第三讲 `mini_ollama.c` 中的运算，在 CPU 的一个线程上每次处理一个元素，以 `-fno-tree-vectorize` 编译 |
| `ops.h` | 算子表 `struct ops`、每个算子与每项内存操作的规则、Q4_0 块的排布与 `q4_0_scale`，你不修改它 |
| `mini_ollama.h` | 框架内部的接口 |
| `main.c` | `mini-ollama` 的主循环：把权重放入算子所用的内存，一层之内的计算写成算子的调用 |
| `model.c` | 把 GGUF 文件载入内存（`mmap`，Windows 上是 `fread`），按名字找到每个张量 |
| `tokenizer.c` | 提示词转换为 Token 编号，Token 编号转换为文字 |
| `selftest.c` | `ops-selftest-simd` 与 `ops-selftest-cuda` |
| `run.sh` | 第二步：编译，运行 `ops-selftest-cuda`，模型文件存在时运行 `mini-ollama-cuda`；课程服务器为每次提交运行它 |
| `tools/submit.py` | 第二步：课程服务器的客户端，打包 `cuda.cu`，发送，打印服务器返回的每一行 |
| `prompt.txt` | `make run-*` 提出的问题，UTF-8 |
| `Modelfile` | ollama 加载本目录下 `q4_0.gguf` 所用的配置，见 `docs/simd.md`「与 ollama 对比」 |
| `q4_0.gguf` | 模型文件，不在发布的目录中，你按「模型文件」一节放入 |
| `Makefile` | 编译选项与各条命令 |
| `docs/` | 两步的说明：`simd.md`、`cuda.md` |

## 范围

- 矩阵只支持 Q4_0，一维张量为 F32。
- 每一步取分数最高的 Token，相当于 ollama 的 `temperature 0`。
- 分词器省略了按正则表达式切分单词的步骤，只保留其中「每个数字单独成为一个 Token」一条。普通句子的结果与模型自带的分词器相同；单词前有连续多个空格时，空格的切分位置不同。
- 整数按小端读取，程序在小端机器上运行。
- `mini-ollama-scalar` 与 `mini-ollama-simd` 只用 CPU 的 1 个线程，算子也不使用线程。
