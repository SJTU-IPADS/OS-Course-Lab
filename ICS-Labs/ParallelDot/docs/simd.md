# parallel-dot 第一步：SIMD

这是实验 `parallel-dot` 的第一步，不计分；实验的总览在上一级目录的 [README.md](../README.md) 中，第二步（CUDA）的说明在 [`cuda.md`](cuda.md) 中。

这一步的程序是第三讲的 `mini-ollama`：它读取模型文件 `q4_0.gguf`（Qwen3-VL-2B-Instruct 的语言模型，每个矩阵为 Q4_0 格式），逐个 Token 生成回答。程序中的每一项计算都是一个算子的调用，这一步用到算子的两种实现：`ops/scalar.c` 是第三讲 `mini_ollama.c` 中的运算，每次处理一个元素；`ops/simd.c` 是同一组算子的 AVX2 版本，每次处理 8 个 float。`simd.c` 中只有一个函数没有写：Q4_0 格式的矩阵与一个 float 向量的乘积 `matvec_q4_0`，你来实现它。生成一个 Token 要做 197 次矩阵向量乘，都由这个函数计算，它占运行时间的绝大部分。第二步（CUDA）写的是同一个算子的 GPU 版本。

这一步配合第三讲「程序的机器级表示」第五部分的 SIMD 内容：讲义中的 256 位向量寄存器与 `<immintrin.h>` 的 intrinsics，你在这里用来把一个真实模型的生成速度提高数倍。

## 分工

框架提供读取 GGUF 文件、分词、逐个 Token 生成的循环、两套算子中除 `simd.c` 的 `matvec_q4_0` 以外的全部函数，以及检查程序。你只修改 [ops/simd.c](../ops/simd.c) 中的一个函数：

```c
static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols);
```

[ops.h](../ops.h) 是框架与算子之间的接口：`struct ops` 中每个函数指针是一个算子或一项内存操作，注释写明它做什么；Q4_0 块的排布与读出缩放系数的函数 `q4_0_scale` 也在其中。框架只通过这张表调用算子，你不修改这个文件。完成这一步你需要阅读 `ops.h`、`ops/simd.c` 与本说明，不需要阅读顶层的其他源文件。

| 算子 | 计算 | 在 `simd.c` 中 |
| --- | --- | --- |
| `matvec` | Q4_0 矩阵与向量的乘积 | **`matvec_q4_0`，你来实现** |
| `embed` | 把一行 Q4_0 权重还原为 float | 已给出，每步还原一个块的 32 个权重 |
| `rmsnorm` | 若干个向量各自缩放到均方根为 1，再乘以增益 | 已给出 |
| `rope` | 按位置旋转每个向量的各对分量 | 已给出 |
| `attn_scores` | 每个查询头与之前每个 Token 的键的内积 | 已给出 |
| `softmax` | 每一行分数变为和为 1 的权重 | 已给出，`exp8` 每次计算 8 个指数 |
| `attn_values` | 按权重累加之前每个 Token 的值 | 已给出 |
| `swiglu` | 前馈网络的门控 `silu(gate) · up` | 已给出 |
| `add` | 残差相加 | 已给出 |
| `argmax` | 最大的分数所在的位置，即下一个 Token | 已给出 |

`simd.c` 的末尾还有算子表的五项内存操作（`init`、`alloc`、`upload`、`download`、`finish`），与 `scalar.c` 中的相同：两套 CPU 算子都直接使用程序的内存。

已给出的算子你可以当作例子阅读：`embed` 中还原一个块的写法，`dot`（`rmsnorm` 与 `attn_scores` 调用的 float 内积）中 8 路累加、最后用 `hsum8` 水平相加的写法，都是 `matvec_q4_0` 要用到的。

| 程序 | 作用 |
| --- | --- |
| `ops-selftest-simd` | 不需要模型文件。检查 `simd.c` 中每个算子的结果，然后比较 `matvec` 与标量版本的速度 |
| `mini-ollama-simd` | 读取模型文件，用 `simd.c` 中的算子回答 `prompt.txt` 中的问题，输出生成速度 |
| `mini-ollama-scalar` | 同上，用 `scalar.c` 中的算子 |

本文中的命令你都在 `parallel-dot/` 目录下执行。

## 准备

| 软件 | 用途 |
| --- | --- |
| gcc | 编译三个程序。`make CC=clang` 改用 clang |
| GNU make | 编译 |
| 支持 AVX2 的 x86-64 处理器 | 运行 `simd.c` 中的算子。2013 年以后的 Intel 处理器（Haswell 起）与 2017 年以后的 AMD 处理器（Zen 起）都支持 |

你的处理器不支持 AVX2 时，程序在第一次执行向量指令处以「非法指令」（Illegal instruction）终止。Linux 上 `grep -o avx2 /proc/cpuinfo | head -1` 输出 `avx2` 即为支持。

你在 x86-64 的 Linux 与 x86-64 的 Windows 上都可以完成这一步。Linux 上你用发行版的包管理器安装 gcc 与 make。

### Windows 上的准备

Windows 上的命令你都在 Git Bash 中执行。你需要安装下面三个软件，每个软件安装之后，把表中所列的目录加入环境变量 `Path`：

| 软件 | 下载地址 | 加入 `Path` 的目录 |
| --- | --- | --- |
| Git for Windows | <https://git-scm.com/downloads/win> | 不需要 |
| GNU make（GnuWin32） | <https://gnuwin32.sourceforge.net/packages/make.htm> | `C:\Program Files (x86)\GnuWin32\bin` |
| gcc（WinLibs 的 MinGW-w64 GCC） | <https://winlibs.com/> | 解压目录下的 `mingw64\bin` |

- **Git for Windows** 提供 Git Bash，在其中运行的 `make` 用它的 `sh` 执行 `Makefile` 中的命令。
- **GNU make**：GnuWin32 提供的版本是 3.81，页面上 “Complete package, except sources” 一行的 Setup 是安装程序。
- **gcc**：你在 WinLibs 页面上选择 UCRT runtime、Win64 的压缩包，解压即可使用。这个压缩包中也有 GNU make，命令名为 `mingw32-make`，用法与 `make` 相同。llvm-mingw（<https://github.com/mstorsjo/llvm-mingw/releases>）的 clang 也可以，你选择 `ucrt-x86_64` 的 zip，把解压目录下的 `bin` 加入 `Path`，编译时写 `make CC=clang`。

Windows 上编译出的程序名为 `mini-ollama-simd.exe` 等，在 Git Bash 中仍然写作 `./mini-ollama-simd` 等。`Makefile` 在 Windows 上加入 `-static`，程序运行时不依赖编译器目录中的 DLL。Windows 上 `mini-ollama-simd` 用 `fread` 把整个模型文件（974 MB）读入内存，需要 1 GB 以上的空闲内存。

Windows 上你也可以在 WSL2（Ubuntu）中按 Linux 的方式完成这一步。

## 调用这个函数的地方

`simd.c` 末尾的算子表把 `matvec_q4_0` 填在 `matvec` 一项，`main.c` 经由这一项调用它，每次计算一个矩阵与一个向量的乘积。`main.c` 生成一个 Token 时依次经过模型的 28 层，每层调用 7 次，最后在输出层调用 1 次，共 197 次：

| 位置 | 矩阵 | `rows` × `cols` |
| --- | --- | --- |
| 每层的注意力部分 | `attn_q`、`attn_output` | 2048 × 2048 |
| | `attn_k`、`attn_v` | 1024 × 2048 |
| 每层的前馈部分 | `ffn_gate`、`ffn_up` | 6144 × 2048 |
| | `ffn_down` | 2048 × 6144 |
| 输出层（与词嵌入共用一个矩阵） | `token_embd` | 151936 × 2048 |

因此在 `mini-ollama-simd` 中你的函数收到的 `cols` 只有 2048 与 6144 两种，`x` 是这一层的输入向量；`ops-selftest-simd` 还用 32 与 96 检查块的边界。生成一个 Token 要计算 28 × 20480 + 151936 = 725376 行的内积，全部矩阵的每个权重都参与一次乘加（约 1.7 × 10⁹ 个）。其他算子每次只处理几千个数，所以生成速度主要由这个函数决定。

## Q4_0 的一行

一个 Q4_0 块存 32 个权重，占 18 个字节；一行 `n` 个权重是 `n / 32` 个块首尾相接，`n` 是 32 的倍数。第二讲实验 `nano-quant` 写出的就是这种块。

| 字节 | 内容 |
| --- | --- |
| 0..1 | 缩放系数 `d`，半精度浮点数（1 位符号、5 位指数、10 位尾数），低字节在前 |
| 2 + j，j = 0..15 | 位 3..0 是第 `j` 个权重的编码 `q`，位 7..4 是第 `j + 16` 个权重的编码 `q` |

编码 `q` 取 0..15，表示的权重是 `d · (q − 8)`。`q4_0_scale(blk)` 读出块 `blk` 的 `d`，返回 float，由框架在 `ops.h` 中给出。

一行的第 `b` 个块（从这一行的起点数起第 `18b` 个字节）存第 `32b` 到 `32b + 31` 个权重。矩阵按行存放，`cols` 列的矩阵每行 `cols / 32 · 18` 个字节，第 `r` 行从 `w + r · (cols / 32 · 18)` 开始。

## 函数的规则

```c
static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols);
```

- `w` 指向 `rows` 行权重，每行 `cols / 32` 个块；`x` 指向 `cols` 个 float，`y` 指向 `rows` 个 float；`cols` 是 32 的正倍数，`rows` 不小于 1。
- 函数对每一行 `r` 写入 `y[r] = Σ W[r][c] · x[c]`（`c` = 0..cols−1），`W[r][c]` 是第 `r` 行第 `c` 个权重，以 float 计算。
- 乘积相加的次序不限。标量版本在一行之内按 `c` 的次序累加；向量版本分 8 路、按块累加，两者的结果只在最后几位有差别，检查程序允许这种差别。
- 函数只写 `y` 的 `rows` 个 float，不分配内存、不输出、不退出。
- 函数只读 `w` 的 `rows · cols / 32` 个块（`rows · cols / 32 · 18` 个字节）与 `x` 的 `cols` 个 float。一个块的编码只有 16 字节，用 256 位加载（`_mm256_loadu_si256`）读编码会多读 16 字节。Linux 上 `mini-ollama-simd` 把模型文件映射到内存（`mmap`），最后一行之后可能没有可读的内存，多读的字节会引起段错误；`ops-selftest-simd` 的缓冲区由 `malloc` 分配，检查中不出现这种错误。
- `w` 与 `x` 的地址都没有对齐的保证：一个块的编码从块的第 2 个字节开始，地址一般不是 16 的倍数；`x` 由 `calloc` 或 `malloc` 分配，地址不保证是 32 的倍数。你用非对齐加载 `_mm_loadu_si128` 与 `_mm256_loadu_ps`；对齐加载 `_mm_load_si128` 与 `_mm256_load_ps` 要求地址是 16 或 32 的倍数，可能使程序以段错误终止。
- `simd.c` 以 `-mavx2` 编译，你可以在其中使用 `<immintrin.h>` 中的 AVX2 intrinsics；其他文件不以 `-mavx2` 编译。

函数体初始时是两行：`(void)` 一行使未使用的参数不产生警告，`todo("matvec_q4_0")` 输出 `matvec_q4_0 is not implemented yet` 并以状态 3 退出程序。你把这两行换成你的实现。

`scalar.c` 中的同名函数是第三讲 `mini_ollama.c` 中的 `matvec` 与它逐行调用的 `vec_dot_q4_0` 写在一起：逐行计算内积，每次还原一个权重、相乘、累加：

```c
size_t row_bytes = cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES;
for (size_t r = 0; r < rows; r++) {
    const uint8_t *blk = w + r * row_bytes;
    float sum = 0;
    for (size_t c = 0; c < cols; c += 32, blk += 18) {
        float d = q4_0_scale(blk);
        for (int j = 0; j < 16; j++) {
            int lo = (blk[2 + j] & 15) - 8, hi = (blk[2 + j] >> 4) - 8;
            sum += d * lo * x[c + j] + d * hi * x[c + j + 16];
        }
    }
    y[r] = sum;
}
```

你的实现对每一行用一个寄存器保存 8 路累加器，每一步处理一个块的 32 个权重。函数上方的 `TODO` 注释给出一种做法：

1. 一条 128 位的加载指令取出块的 16 个编码字节（`_mm_loadu_si128`）；
2. 与 `0x0f` 按位与得到 16 个低 4 位，即第 0..15 个权重的编码；右移 4 位再按位与得到第 16..31 个权重的编码（`_mm_and_si128`、`_mm_srli_epi16`）；
3. 每 8 个编码扩展为 8 个 32 位整数（`_mm256_cvtepu8_epi32`），减 8（`_mm256_sub_epi32`），转换为 8 个 float（`_mm256_cvtepi32_ps`）；
4. 与 `x` 中对应的 8 个 float 相乘（`_mm256_loadu_ps`、`_mm256_mul_ps`），累加到块的部分和（`_mm256_add_ps`）；
5. 块的部分和乘以 `d`（`_mm256_set1_ps`），累加到整行的 8 路累加器；
6. 这一行的全部块处理完后，用 `hsum8` 把 8 路累加器的 8 个 float 相加为一个 float，写入 `y[r]`。

第 1 到 3 步与同一文件中的 `embed` 相同：`embed` 还原一个块，把 32 个权重写入内存；`matvec_q4_0` 还原之后直接与 `x` 相乘。第 4 步是讲义中 32 位整数内积向量循环的 float 版本：`_mm256_mullo_epi32` 与 `_mm256_add_epi32` 换为 `_mm256_mul_ps` 与 `_mm256_add_ps`。

第 6 步把一个寄存器中的 8 个 float 相加为 1 个，这一步称为水平相加，每一行只在全部块处理完之后做一次。`hsum8` 在 `simd.c` 的开头：

```c
__m128 s = _mm_add_ps(_mm256_castps256_ps128(v),       /* 低 128 位的 4 个 float */
                      _mm256_extractf128_ps(v, 1));    /* 加高 128 位的 4 个：8 个变为 4 个 */
s = _mm_hadd_ps(s, s);                                 /* 相邻两个相加：4 个变为 2 个 */
s = _mm_hadd_ps(s, s);                                 /* 2 个变为 1 个，位于最低的 32 位 */
return _mm_cvtss_f32(s);
```

每条 intrinsic 对应的指令与用法见 Intel Intrinsics Guide（<https://www.intel.com/content/www/us/en/docs/intrinsics-guide/>）。

## 编译与检查

```bash
make              # 生成 mini-ollama-scalar、mini-ollama-simd 与 ops-selftest-simd
make test-simd    # 编译，然后运行 ./ops-selftest-simd
make clean        # 删除全部程序与 build/ 下的目标文件
```

`make` 把每套算子的目标文件放在 `build/` 下各自的子目录中：`build/scalar/` 是 `mini-ollama-scalar` 的，`build/simd/` 是 `mini-ollama-simd` 与 `ops-selftest-simd` 的。`main.c` 与 `selftest.c` 以 `-DOPS=ops_simd` 等编译，这个宏指明程序使用哪一张算子表。你不改 `Makefile` 中的编译选项：

| 文件 | 选项 | 作用 |
| --- | --- | --- |
| `ops/simd.c` | `-O2 -mavx2` | 允许使用 AVX2 指令。不加 `-mfma`：讲义讲到 AVX2 为止，融合乘加在这些算子中的收益很小 |
| `ops/scalar.c` | `-O2 -fno-tree-vectorize` | GCC 12 起在 `-O2` 下自动向量化部分循环，这个选项使标量算子保持每次处理一个元素 |
| 其余文件 | `-O2` | |

`ops-selftest-simd` 分三组，每一项输出一行 `pass` 或 `FAIL`，最后输出 `all passed` 或 `N failed`，退出码为失败的项数。第一组不调用 `matvec`；你还没有实现 `matvec_q4_0` 时，程序在第二组第一次调用它时输出 `matvec_q4_0 is not implemented yet` 并以状态 3 退出。

**第一组「operators」**检查已给出的算子：每个算子与 `scalar.c` 中的同名算子处理同一组输入，输入由固定种子的伪随机数生成，大小取模型中出现的值（如 2048、16 个头 × 128、151936 个分数）以及不是 8 的倍数的值（如 3 个长度 20 的向量、13 个元素），`rmsnorm` 的每种形状还原地再运行一次（`y` 等于 `x`）；两个结果之差不超过输出中最大绝对值的 10⁻⁵ 即通过。这一组在你开始之前就全部通过；你修改了其他算子时，它报告哪一个算子与标量版本不一致。

**第二组「matvec」**检查你的函数，矩阵为 64 × 32、64 × 96、1000 × 2048（两组，其中一组 `x` 的量级为 64）与 64 × 6144，2048 是 `n_embd`（大部分矩阵的列数），6144 是 `n_ff`（`ffn_down` 的列数），32 与 96 检查只有一个块与块数不是 2 的幂的行；缩放系数有正有负，16 个编码值都出现。另外构造了两行：第 0 行的 16 个字节是 `j | (15 − j) << 4`、`d = −1`；第 1 行的 `d` 有 0 与次正规数。每一行的期望值按 `ops.h` 中的定义用 double 计算，结果与它的差不超过这一行 `Σ |W[r][c] · x[c]|` 的 10⁻⁵ 即通过。不同的累加次序带来的差别在 10⁻⁷ 量级；漏掉 2048 个权重中的一个，差别是 1/2048，与任何还原错误一样远大于这个界限。

**第三组「speed」**用模型中的四种矩阵比较 `simd.c` 与 `scalar.c` 的 `matvec_q4_0`：每种至少运行 0.2 秒，取 3 次中最快的一次，输出每次矩阵向量乘的毫秒数、每秒读取的权重字节数（`GB/s`）与加速比。第一种矩阵 `attn_q`（2048 × 2048，2.4 MB，在高速缓存之内）上的加速比不低于 4 倍即通过。这个门限的依据：仍然每次处理一个权重的实现在 `-mavx2` 下也只有 1.6 倍，每步处理 32 个权重的 256 位实现有 6 到 7 倍。

你的实现正确时的输出类似：

```
operators: simd (ops/simd.c)
device: CPU, 1 thread, AVX2
1. operators against ops/scalar.c
  pass  embed: 32 and 2048 weights, error 0.0e+00 of the largest |value|
  pass  rmsnorm: 1 x 2048, 16 x 128, 8 x 128, 3 x 100, 2 x 1000, also in place, error 2.2e-07 of the largest |value|
  pass  rope: 16 x 128 at positions 0 and 511, 8 x 128 at 37, 3 x 20, error 0.0e+00 of the largest |value|
  pass  attn_scores: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 3.8e-07 of the largest |value|
  pass  softmax: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 3.8e-07 of the largest |value|
  pass  attn_values: 16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13, error 0.0e+00 of the largest |value|
  pass  softmax: 37 scores between -100 and 100, error 7.2e-08 of the largest |value|
  pass  swiglu: 6144 and 13 values between -20 and 20, one at -100, error 1.2e-08 of the largest |value|
  pass  add: 2048 and 13 values, error 0.0e+00 of the largest |value|
  pass  argmax: 151936 values, two equal largest, the largest first and last
2. matvec against the definition
  pass  matvec, 64 x 32: error 3.5e-08 of the sum of |terms|
  pass  matvec, 64 x 96: error 4.0e-08 of the sum of |terms|
  pass  matvec, 1000 x 2048: error 4.2e-08 of the sum of |terms|
  pass  matvec, 1000 x 2048, x up to 64: error 4.6e-08 of the sum of |terms|
  pass  matvec, 64 x 6144: error 3.0e-08 of the sum of |terms|
3. speed of matvec: simd against the scalar operator on one CPU thread
  matrix           rows x cols          MB  scalar ms  GB/s    simd ms  GB/s  speed-up
  attn_q           2048 x 2048         2.4    3.323     0.7    0.520     4.5      6.4x
  ffn_gate, ffn_up 6144 x 2048         7.1    9.913     0.7    1.771     4.0      5.6x
  ffn_down         2048 x 6144         7.1   10.280     0.7    1.696     4.2      6.1x
  output           151936 x 2048     175.0  248.536     0.7   46.638     3.8      5.3x
  pass  speed-up on attn_q 6.4x, at least 4x required

all passed
```

`error` 是各项中最大的相对误差，毫秒数与 `GB/s` 因机器而异。

## 在真实模型上运行

### 模型文件

`mini-ollama-simd` 与 `mini-ollama-scalar` 读取本目录下的 `q4_0.gguf`。你从交大云盘下载它，或者把机器上已有的这个文件移动或链接到本目录，做法见上一级目录 [README.md](../README.md) 的「模型文件」一节；`ops-selftest-simd` 不需要它。文件放在别处时你写 `make run-simd MODEL=路径`。其他格式的文件在加载时报告 `... is not Q4_0` 并退出。

### 生成与计时

```bash
make run-simd     # 用 simd.c 中的算子回答 prompt.txt 中的问题
make run-scalar   # 用 scalar.c 中的算子回答同一个问题
```

两条命令分别运行

```
./mini-ollama-simd MODEL.gguf PROMPT.txt [MAX_NEW_TOKENS]
./mini-ollama-scalar MODEL.gguf PROMPT.txt [MAX_NEW_TOKENS]
```

- 回答写到标准输出，每生成一个 Token 输出一次；六行统计写到标准错误。
- `operators` 是这次运行使用的一套算子，`device` 是算子运行的设备。
- `prompt eval rate` 是读入提示词的速度，`eval rate` 是生成回答的速度，两个名字与 `ollama run --verbose` 的输出相同。提示词的最后一个 Token 之前的各轮计入前者，此后每一轮生成一个 Token，计入后者。
- `matvec rate` 是全部矩阵向量乘的运算速率，1 次乘加计 2 次运算。
- `weight bandwidth` 是每秒读取的 Q4_0 权重字节数，每个权重 18/32 字节，生成一个 Token 读取全部权重一次。
- `MAX_NEW_TOKENS` 是回答的最大 Token 数，默认 64。提示词与回答合计不超过 512 个 Token。
- 问题写在文件中，UTF-8，末尾的换行不计入；你换一个问题时写 `make run-simd PROMPT=另一个文件`。

标量算子的输出：

```
我是一个虚拟助手，没有实体，但我可以为你提供帮助和解答。
operators:            scalar (ops/scalar.c)
device:               CPU, 1 thread
prompt eval rate:     0.82 tokens/s
eval rate:            0.75 tokens/s
matvec rate:          2.49 GFLOPS
weight bandwidth:     0.7 GB/s
```

你的实现正确时，`make run-simd` 的回答与之相同，`eval rate` 高出数倍：参考答案在同一台机器上为 4.3–4.7 Token/s（2026-10-17，i9-11900H，接通电源）。两套算子的累加次序不同，各项结果的最后几位有差别；这个差别可能改变某一步分数最高的 Token，使回答从那一步起不同，这与你的实现是否正确无关。你的实现是否正确以 `ops-selftest-simd` 为准。

第一次运行时模型文件从磁盘读入，`prompt eval rate` 包含这部分时间；文件已在系统的页缓存中时（第二次运行起），这部分时间可以忽略。同一台机器上的速率在多次运行之间有百分之几的差别，你比较时各运行两次。

### 与 ollama 对比

`Modelfile` 的第一行 `FROM ./q4_0.gguf` 让 ollama 加载本目录下的同一个文件，其余几行使用相同的对话格式，每一步取分数最高的 Token，只使用 CPU、1 个线程。`make run-ollama` 在本目录下执行下面两条命令，需要 `ollama serve` 已在运行：

```bash
ollama create nq-q4-0 -f Modelfile
ollama run nq-q4-0 --verbose "用一句话介绍你自己。"
```

同一台机器上，1 个线程的 ollama 生成速度约为 10.7 Token/s，是标量算子的 14 倍。它计算 Q4_0 内积的函数 `ggml_vec_dot_q4_0_q8_0` 先把激活值量化为 8 位整数，再用 `vpmaddubsw`、`vpmaddwd` 或 `vpdpbusd` 做整数乘加，每个块的指令条数少于这一步的 float 实现。量化激活值使各步的分数与 `mini-ollama` 略有差别，两个程序的回答可以不同。检查结束后你用 `ollama rm nq-q4-0` 删除这个模型。
