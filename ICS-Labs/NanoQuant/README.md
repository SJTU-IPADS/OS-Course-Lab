# nano-quant

本实验把 Qwen3-VL-2B-Instruct 的语言模型部分从 BF16 量化为 4 位，组装为 GGUF 文件，
再由 ollama 加载运行。实验配合第二讲「数据的表示」：讲义中的字节序、浮点格式与三种
4 位量化格式，在这里用于处理一个 4.25 GB 的真实模型文件。

本实验是课后练习，不计分。每一部分都配有检查程序，
输出与给出的期望值一致，说明这一部分的实现正确。

## 流程与分工

```
model/model.safetensors ── nano-quant quant ──> model/q4_k_m.nq ──┐
                                                                   ├─ nq2gguf ──> model/q4_k_m.gguf ──> ollama
model/config.json    ──┬── tools/mkmeta.py ──> model/meta.kv ─────┘
model/tokenizer.json ──┘
```

框架提供命令行、文件读写、张量的选择、每个张量的格式与 GGUF 名字、按行量化的循环、
Q6_K 格式、GGUF 组装与全部检查程序。需要你实现的是其中与数据表示有关的函数：
字节的组合、浮点格式的转换、三种量化格式的编码与还原。这些函数全部写在
[impl/nano_quant.cpp](impl/nano_quant.cpp) 中；它们的原型与精确的计算规则在
[include/nano_quant.h](include/nano_quant.h) 中，框架只通过这个头文件调用你的代码。
框架为这些函数准备的两个辅助函数 `nq_round` 与 `nq_q4_k_fit` 也声明在这个头文件中。
每个函数只处理自己的参数，完成实验只需要阅读这个头文件与本说明，不需要阅读 `src/` 下的框架代码。

| 部分 | 函数 | 框架的用途 |
| --- | --- | --- |
| A 读字节 | `rd_u64le` | 读取 safetensors 的头长度 |
| | `bf16_to_f32`、`fp16_to_f32`、`f32_to_fp16` | 读取权重；写出与读回块格式中的 fp16 字段 |
| B 三种量化格式 | `q4_0_quantize/dequantize`、`q4_1_…`、`q4_k_…` | 逐块量化每一行权重 |
| | `put_scale_min`（`get_scale_min` 已给出） | Q4_K 的 12 字节缩放段 |

## 目录

| 路径 | 内容 |
| --- | --- |
| `impl/nano_quant.cpp` | 需要实现的全部函数，初始时每个函数都报告尚未实现并退出 |
| `include/nano_quant.h` | 待实现函数的原型与计算规则，不要修改 |
| `include/nq.h` | 框架内部的接口，完成实验不需要阅读 |
| `include/json.h` | 框架使用的 JSON 解析器的接口 |
| `src/main_quant.cpp` | `nano-quant` 的命令行 |
| `src/main_nq2gguf.cpp` | `nq2gguf` 的命令行 |
| `src/main_selftest.cpp` | `nq-selftest` 的命令行与各组检查 |
| `src/st.cpp`、`src/json.cpp` | safetensors 头的解析与按段读取 |
| `src/fp16.cpp` | 框架自带的半精度与 BF16 转换，供 F16、Q6_K 两种格式使用，检查时作为对照 |
| `src/q4_k_fit.cpp` | `nq_q4_k_fit`：Q4_K 子块的拟合，由框架提供 |
| `src/q6_k.cpp` | Q6_K 格式，由框架提供 |
| `src/quant.cpp` | 类型表与按行量化的循环 |
| `src/nqfile.cpp` | `.nq` 文件的读写 |
| `src/recipe.cpp` | 张量的选择、各配方的格式规则、GGUF 名字的对照表 |
| `src/md5.cpp` | md5 |
| `src/file.cpp` | 按偏移读写文件，Linux 与 macOS 共用一种实现，Windows 用另一种 |
| `tools/mkmeta.py` | 从 `config.json` 与 `tokenizer.json` 生成 GGUF 的键值段 |
| `tools/mkfixtures.py` | 生成检查用的 safetensors 样例 |
| `tools/ggufdump.py` | 读取一个 GGUF，检查排布是否自洽 |
| `tools/fetch-model.sh` | 下载并校验三个模型文件 |
| `tools/install-ollama.sh` | 调用 ollama 的官方脚本安装 ollama |
| `tests/run.sh` | 用样例模型检查三个程序的脚本 |
| `tests/expected-blocks.txt` | `nq-selftest` 的期望值 |
| `tests/expected-tiny-nq.md5` | 样例模型量化结果的期望 md5 |
| `tests/expected-qwen3vl.txt` | 真实模型的期望值：量化结果的 md5 与字节数、GGUF 的字节数 |
| `model/` | 模型文件，需要自行下载，见 [model/README.md](model/README.md) |

本文中的命令都在 `nano-quant/` 目录下执行。

## 准备

| 软件 | 用途 |
| --- | --- |
| g++ 或 clang++，支持 C++17 | 编译三个程序 |
| GNU make | 编译 |
| Python 3，只用标准库 | `mkmeta.py`、`mkfixtures.py`、`ggufdump.py` |
| sh 与 curl | 运行 `tests/run.sh`、`tools/fetch-model.sh` 与 `tools/install-ollama.sh` |
| ollama | 加载运行量化后的模型。本实验在 ollama 0.33.2 上验证 |

x86-64 的 Linux、arm64 的 macOS 与 x86-64 的 Windows 都可以完成本实验，
各项检查的期望值在这些机器上相同。Linux 上用发行版的包管理器安装前四项；
macOS 上运行 `xcode-select --install`，得到的命令行工具包含前四项。
ollama 的安装见「由 ollama 加载运行」一节。

### Windows 上的准备

Windows 上的命令都在 Git Bash 中执行。需要安装下面四个软件，
每个软件安装之后，把表中所列的目录加入环境变量 `Path`：

| 软件 | 下载地址 | 加入 `Path` 的目录 |
| --- | --- | --- |
| Git for Windows | <https://git-scm.com/downloads/win> | 不需要 |
| GNU make（GnuWin32） | <https://gnuwin32.sourceforge.net/packages/make.htm> | `C:\Program Files (x86)\GnuWin32\bin` |
| g++（WinLibs 的 MinGW-w64 GCC） | <https://winlibs.com/> | 解压目录下的 `mingw64\bin` |
| Python 3 | <https://www.python.org/downloads/windows/> | 安装时选择加入 `Path` 的选项 |

- **Git for Windows** 提供 Git Bash，其中有 `sh`、`curl`、`sha256sum`、`awk` 等命令。
  在 Git Bash 中运行的 `make` 用其中的 `sh` 执行 `Makefile` 中的命令。
- **GNU make**：GnuWin32 提供的版本是 3.81，页面上 “Complete package, except sources” 一行的
  Setup 是安装程序。
- **g++**：WinLibs 页面上选择 UCRT runtime、Win64 的压缩包，解压即可使用。
  这个压缩包中也有 GNU make，命令名为 `mingw32-make`，用法与 `make` 相同。
  MinGW-w64 的其他发行版列在 <https://www.mingw-w64.org/downloads/>。
- **clang++** 可以代替 g++。llvm-mingw（<https://github.com/mstorsjo/llvm-mingw/releases>）
  的压缩包包含 clang++ 与 MinGW-w64 的库，选择 `ucrt-x86_64` 的 zip，把解压目录下的 `bin`
  加入 `Path`，编译时写 `make CXX=clang++`。LLVM 官方的 Windows 安装程序
  （<https://github.com/llvm/llvm-project/releases>）中的 clang++ 使用 Visual Studio 的
  头文件与库，还需要安装 Visual Studio 生成工具
  （<https://visualstudio.microsoft.com/visual-cpp-build-tools/>）。
- **Python**：Git Bash 中 `python3 --version` 不输出版本号时，Python 的命令名是 `python`。
  这时编译检查写作 `make test PYTHON=python`，本说明中其余命令里的 `python3` 也换成 `python`。

Windows 上编译出的程序名为 `nano-quant.exe`、`nq2gguf.exe`、`nq-selftest.exe`，
在 Git Bash 中仍然写作 `./nano-quant` 等。`Makefile` 在 Windows 上加入 `-static`，
程序运行时不依赖编译器目录中的 DLL。

Windows 上也可以在 WSL2（Ubuntu）中按 Linux 的方式完成本实验。

## 编译与检查

```bash
make              # 生成 nano-quant、nq2gguf、nq-selftest
make fixtures     # 在 tests/fixtures/ 下生成样例文件，可随时重新生成
make test         # 编译、生成样例，然后运行 tests/run.sh
make clean        # 删除三个程序
```

`Makefile` 中的编译选项保持不变：

| 选项 | 作用 |
| --- | --- |
| `-std=c++17 -O2` | 语言版本与优化级别 |
| `-ffp-contract=off` | 禁止把 `a*b+c` 合并为一条 FMA 指令 |
| `-fno-fast-math` | 不允许编译器假定浮点运算满足交换律与结合律 |

后两条保证各项检查在不同的机器上成立。合并为 FMA 之后，`a*b` 的中间结果不再舍入到
float，量化边界上的取整随之改变，同一份代码在两台机器上会写出不同的字节。
期望值按这组选项生成。

实现时遵守两条约定：

1. 浮点运算全部使用 float，按头文件与本说明中公式给出的次序计算，不经过 double。
   检查比较的是字节，经过 double 中转会改变部分结果的最后一位。
2. 头文件与本说明中写作 round 的取整一律使用 `nq_round`（就近舍入，平局取偶）。
   `(int)(x + 0.5f)` 在负数与平局处的结果与它不同；头文件中确实需要这种截断写法的地方，
   公式中直接写出了这种写法。

未实现的函数被调用时输出 `nano_quant.cpp: <函数名> is not implemented yet` 并以状态 3 退出。
按 A、B 的顺序实现。两部分由 `nq-selftest` 按组检查，完成一组涉及的函数即可运行这一组。
`make test` 运行的 `tests/run.sh` 在 `nq-selftest` 之外，用一个四层的样例模型检查
读取、量化与组装 GGUF 的整个流程。

## A 读字节

```c
uint64_t rd_u64le(const uint8_t *p);   /* 8 个字节，小端 */
float    bf16_to_f32(uint16_t h);
float    fp16_to_f32(uint16_t h);
uint16_t f32_to_fp16(float f);         /* 就近舍入，平局取偶 */
```

safetensors 文件分为三段：8 字节的小端头长度 N、N 字节的 UTF-8 JSON 头、张量数据。
JSON 头的每个键是一个张量名，值给出 `dtype`、`shape` 与 `data_offsets`；
`data_offsets` 相对数据区的起点 8 + N，左闭右开。本实验的模型文件中 N = 76240。

- `rd_u64le` 用移位与按位或把 8 个字节组合为一个整数，`p[0]` 是最低字节。
  注意整数提升：`uint8_t` 参与移位之前先提升为 `int`。
  框架用它读取头长度；字节顺序写反时，读出的数值大于文件长度，
  `nano-quant` 报告 `header length ... is impossible` 并提示检查移位方向。
- `bf16_to_f32`：BF16 的符号与指数字段与 FP32 相同，7 位尾数是 FP32 尾数的高 7 位。
- `fp16_to_f32`：按 1-5-10 的字段组合，输入分为正规数、次正规数、零、无穷与 NaN 四类。
- `f32_to_fp16`：就近舍入，平局取偶；超出范围的值变为无穷，过小的值变为次正规数或零。
  三种量化格式的 fp16 字段都由它写出，它的一处错误会使整个量化结果与期望值不一致。

检查：

```bash
./nq-selftest a            # 默认按步长 251 抽取 float 检查 f32_to_fp16
./nq-selftest --full a     # 遍历全部 2^32 个 float，约 15 秒
```

| 项 | 内容 |
| --- | --- |
| `rd_u64le` | 三组字节，其中包括 8 个 `ff` 与只有 `p[4]` 非零的一组 |
| `bf16_to_f32`、`fp16_to_f32` | 全部 65536 个位型上与框架自带的实现相同；`fp16_to_f32` 的 NaN 只要求同为 NaN |
| `f32_to_fp16` | 与框架自带的实现相同，并且半精度能精确表示的每个值往返后不变 |

`run.sh` 第二组用 `rd_u64le` 读取两个样例文件的头：`tiny.safetensors` 的头长度按小端写入，
必须被接受；`tiny-be.safetensors` 的头长度按大端写入，必须被拒绝。

## B 三种量化格式

| 格式 | 每块权重数 | 每块字节数 | 位/权重 | 块的内容 |
| --- | --- | --- | --- | --- |
| Q4_0 | 32 | 18 | 4.5 | fp16 的 `d`，16 字节编码 |
| Q4_1 | 32 | 20 | 5.0 | fp16 的 `d` 与 `m`，16 字节编码 |
| Q4_K | 256 | 144 | 4.5 | fp16 的 `d` 与 `dmin`，12 字节缩放段，128 字节编码 |

每个 `quantize` 函数把一块 float 写为同名 ggml 格式的字节，每个 `dequantize` 函数把字节
还原为 float。一块只取决于它自己的输入。每一步取整都已写明，一个输入只有一个正确的输出。
三种格式共同的约定：

- fp16 字段用 `f32_to_fp16` 写出、用 `fp16_to_f32` 读回，低字节在前。
- 公式中的 round 是 `nq_round`；clamp(v, 0, 15) 把 v 限制在 0 到 15 之间。

### Q4_0 与 Q4_1

两种格式的公式在头文件中。实现时注意两处：

- 计算编码时使用舍入到 fp16 之前的 float `d`（Q4_1 还有 `lo`），写入块中的是它们舍入之后的值。
- 第 `j` 个编码字节的低 4 位存第 `j` 个权重，高 4 位存第 `j + 16` 个权重，`j` = 0..15。

### Q4_K 的结构

Q4_K 以 256 个权重为一个超块，分为 8 个子块，每个子块 32 个权重。
子块 `j` 中的权重表示为

```
x ≈ D[j] · q − M[j]          q 取 0..15，D[j] ≥ 0，M[j] ≥ 0
```

这与 Q4_1 的 `x = d · q + m` 形式相同。每个子块的 `D[j]` 与 `M[j]` 再量化一次，
写作超块共用的 fp16 系数与子块的 6 位整数之积：

```
D[j] = d · sc[j]          M[j] = dmin · m[j]          sc[j]、m[j] 取 0..63
```

一个超块的位数：

```
d、dmin      2 × 16         =   32 位
sc、m        8 × (6 + 6)    =   96 位
编码 q        256 × 4        = 1024 位
合计                          1152 位 = 144 字节，每个权重 1152 / 256 = 4.5 位
```

8 个子块各存一对 fp16 的 `D[j]` 与 `M[j]` 需要 8 × 32 = 256 位，按上面的两级表示只需
32 + 96 = 128 位。省下的位数使 Q4_K 在与 Q4_0 相同的 4.5 位/权重下，每 32 个权重有各自的
缩放与偏移。

`q4_k_quantize` 分三步计算，步骤的编号与头文件相同。

### Q4_K 第 1 步：拟合每个子块

对子块 `j` 调用框架提供的函数：

```c
nq_q4_k_fit(x + 32*j, &s[j], &o[j]);     /* s[j] >= 0，o[j] >= 0 */
```

它求出 `s[j]` 与 `o[j]`，使子块中的 `x[i] ≈ s[j] · q[i] − o[j]`。这一步只需调用；
下面给出 `nq_q4_k_fit` 的计算过程，它与 ggml 的 `make_qkx2_quants` 相同，
实现在 `src/q4_k_fit.cpp` 中。以下公式中的 `i` 取 0..31，求和都是对子块内的 32 个权重。

**误差的权重**。拟合按加权平方误差计算，离 0 较远的权重在误差中的比重较大：

```
w[i] = sqrt(Σ x[i]² / 32) + |x[i]|
```

**起点**。把 16 个编码均匀分布在区间 `[b, hi]` 上，区间的下端不高于 0：

```
b      = min(min x[i], 0)          hi = max x[i]
iscale = 15 / (hi − b)             a  = 1 / iscale
l[i]   = clamp(round(iscale · (x[i] − b)), 0, 15)
best   = Σ w[i] · (a · l[i] + b − x[i])²
```

`hi = b` 时 32 个权重相等且不大于 0，结果直接是 `s[j] = 0`、`o[j] = −b`。

**搜索**。依次取 21 个候选，`k` = 0..20，每个候选按不同的编码间距重新确定编码：

```
iscale = (−1 + 0.1·k + 15) / (hi − b)          分子依次为 14.0, 14.1, …, 16.0
l[i]   = clamp(round(iscale · (x[i] − b)), 0, 15)
```

编码确定之后，用加权最小二乘求 `x ≈ a' · l + b'` 中的 `a'` 与 `b'`。记

```
Sw = Σ w[i]          Sl  = Σ w[i]·l[i]          Sll = Σ w[i]·l[i]²
Sx = Σ w[i]·x[i]     Slx = Σ w[i]·l[i]·x[i]
```

使 `Σ w[i] · (a' · l[i] + b' − x[i])²` 最小的 `a'`、`b'` 满足
`a'·Sll + b'·Sl = Slx` 与 `a'·Sl + b'·Sw = Sx`，解为

```
Δ  = Sw · Sll − Sl²                   Δ ≤ 0 时跳过这个候选
a' = (Sw · Slx − Sx · Sl) / Δ
b' = (Sll · Sx − Sl · Slx) / Δ
```

解出的 `b' > 0` 时，令 `b' = 0`，只拟合斜率：`a' = Slx / Sll`。
这个候选的加权误差 `Σ w[i] · (a' · l[i] + b' − x[i])²` 小于 `best` 时，
用它更新 `best`、`a` 与 `b`；此后的候选按更新后的 `b` 计算 `iscale` 与编码。

**结果**。

```
s[j] = max(a, 0)          o[j] = −b
```

搜索中 `b` 始终不大于 0，所以 `o[j]` 不小于 0，与 `x = D · q − M` 中 `M` 的符号一致。

### Q4_K 第 2 步：把 8 对 `s[j]`、`o[j]` 量化为 6 位

```
S  = max s[j]                        O    = max o[j]                  j = 0..7
d  = S / 63                          dmin = O / 63
is = S > 0 ? 63 / S : 0              io   = O > 0 ? 63 / O : 0
sc[j] = round(is · s[j])             m[j] = round(io · o[j])
```

8 个 `s[j]` 中最大的一个对应 `sc = 63`，其余按比例落在 0..63 之内，`m[j]` 同理。
`d` 与 `dmin` 用 `f32_to_fp16` 写入 `blk[0..1]` 与 `blk[2..3]`，
8 对 `(sc[j], m[j])` 用 `put_scale_min` 写入 `blk[4..15]`。
8 个子块的 `s[j]` 都为 0 时，`d`、`is` 与全部 `sc[j]` 都为 0，`dmin` 一侧同理。

### Q4_K 第 3 步：重新计算 4 位编码

还原时读到的是经过两级量化的 `D[j]` 与 `M[j]`，它们与第 1 步拟合出的 `s[j]`、`o[j]`
有差别，所以编码按还原时的值重新计算：

```
d'    = fp16_to_f32(f32_to_fp16(d))      dmin' = fp16_to_f32(f32_to_fp16(dmin))
D[j]  = d' · sc[j]                       M[j]  = dmin' · m[j]
q[i]  = D[j] ≠ 0 ? clamp(round((x[i] + M[j]) / D[j]), 0, 15) : 0
```

`d'` 与 `dmin'` 是 `d`、`dmin` 经过 fp16 往返之后的值，也就是 `q4_k_dequantize`
从块中读到的值。

### Q4_K 的还原

```
x[i] = (d · sc[j]) · q[i] − dmin · m[j]          x[i] 属于子块 j
```

`d`、`dmin` 从 `blk[0..3]` 读出，`sc[j]`、`m[j]` 用 `get_scale_min` 读出。
按括号中的次序计算：先求 `d · sc[j]` 与 `dmin · m[j]`，再乘以编码、相减。

### Q4_K 的字节排布

| 字节 | 内容 |
| --- | --- |
| 0..1 | `d`，fp16，低字节在前 |
| 2..3 | `dmin`，fp16，低字节在前 |
| 4..15 | 缩放段 `scales[0..11]`：8 对 `(sc, m)` |
| 16..143 | 编码段 `qs[0..127]`：256 个 4 位编码 |

编码段以 64 个权重（两个相邻的子块）为一组，每组 32 字节：

```
qs[32g + l] = q[64g + l] | q[64g + 32 + l] << 4          g = 0..3，l = 0..31
```

第 `g` 组字节的低 4 位存子块 `2g`，高 4 位存子块 `2g + 1`。
Q4_0 与 Q4_1 的第 `j` 个字节存第 `j` 与第 `j + 16` 个权重，两者的配对方式不同。

### Q4_K 的缩放段

8 个 `sc` 与 8 个 `m` 各 6 位，共 96 位，存放在 12 个字节 `scales[0..11]` 中
（`scales` 即 `blk + 4`）。前 4 对各占一个字节的低 6 位；后 4 对各拆为低 4 位与高 2 位，
低 4 位放在 `scales[8..11]`，高 2 位放在前 8 个字节的最高 2 位。`k` 取 0..3：

| 字节 | 位 7..6 | 位 5..0 |
| --- | --- | --- |
| `scales[k]` | `sc[k+4]` 的位 5..4 | `sc[k]` |
| `scales[k+4]` | `m[k+4]` 的位 5..4 | `m[k]` |

| 字节 | 位 7..4 | 位 3..0 |
| --- | --- | --- |
| `scales[k+8]` | `m[k+4]` 的位 3..0 | `sc[k+4]` 的位 3..0 |

`impl/nano_quant.cpp` 中给出的 `get_scale_min` 按这两张表读出第 `j` 对。
你需要实现与它互逆的 `put_scale_min`：写入第 `j` 对之后，`get_scale_min` 读出第 `j` 对
得到刚写入的值，其余 7 对保持原值。检查时 12 个字节的初值是任意的，8 对的写入次序每轮不同，
因此写入第 `j` 对时只改动属于这一对的位。

### 检查

```bash
./nq-selftest q4_0
./nq-selftest q4_1
./nq-selftest scale_min
./nq-selftest q4_k         # 同时运行 scale_min
```

每种格式在 512 个超块（131072 个权重）上量化，比较编码字节的 md5（`*_bytes`）与还原值的
md5（`*_back`）。前 8 个超块是专门构造的：全零、全相等、含一个离群值、全负、全正、
量级 1e-7、关于 0 对称、只有一个非零点。它们落在公式的边界上：`d` 或 `D` 为 0 时的分支、
`min()` 与 clamp 的截断、fp16 字段下溢为次正规数或 0，这些是实现中最容易出错的地方。
其余 504 个超块是随机数，量级从 1e-4 到 1e2。

标准错误上另外输出每种格式的相对均方根误差与最大绝对误差，实现正确时为：

```
q4_0  relative rms error 0.08408, max abs error 8
q4_1  relative rms error 0.07855, max abs error 5.98438
q4_k  relative rms error 0.07146, max abs error 5.87793
q6_k  relative rms error 0.01793, max abs error 1.53906
```

`q6_k` 组检查框架自带的 Q6_K，用于确认编译环境，它在未实现任何函数时也应当一致。

`run.sh` 第三组把样例模型按 `q4_k_m` 配方量化，比较数据区的 md5；
第四组把量化结果组装为 GGUF，用 `ggufdump.py` 检查排布。

## 在真实模型上运行

A、B 两部分都通过 `make test` 之后，在真实模型上完成整个流程。

### 下载模型文件

把 `model.safetensors`、`config.json` 与 `tokenizer.json` 三个文件下载到 `model/` 下：

```bash
sh tools/fetch-model.sh
```

脚本默认从镜像 `https://hf-mirror.com` 下载，每个文件下载之后校验 sha256，
输出一行 `文件名: OK` 或 `文件名: FAILED`；下载中断时，再次运行同一条命令即从断点继续。
更换下载源与其他下载方式见 [model/README.md](model/README.md)。

### 张量的选择、格式与名字

量化整个模型时，`nano-quant` 先为每个张量确定三项：是否量化、使用哪种格式、在 GGUF 中的名字，
然后逐行调用 B 部分的函数。这三项由框架在 `src/recipe.cpp` 中确定，规则如下。

模型文件中有 625 个张量，按名字前缀分为两部分：

| 前缀 | 张量数 | 字节数 | 本实验 |
| --- | --- | --- | --- |
| `model.language_model.` | 310 | 3 441 149 952 | 全部量化 |
| `model.visual.` | 315 | 813 914 112 | 全部跳过 |

310 = 28 层 × 11 + `embed_tokens` + `norm`。一维张量（各种归一化系数）一律保持 F32，
二维张量的格式由 `--recipe` 指定的配方确定：

| 配方 | 规则 |
| --- | --- |
| `q4_0` | 全部 Q4_0 |
| `q4_1` | 全部 Q4_1 |
| `q4_k` | 全部 Q4_K |
| `q4_k_m` | `embed_tokens` 用 Q6_K；部分层的 `v_proj` 与 `down_proj` 用 Q6_K；其余 Q4_K |

`q4_k_m` 与 llama.cpp 的同名配方相同。「部分层」的判据取自 llama.cpp 的 `use_more_bits`：
共 n 层时，第 i 层满足下面任一条即提高到 Q6_K（除法为整数除法）。

```
i < n/8        或        i >= 7n/8        或        (i - n/8) % 3 == 2
```

n = 28 时符合的是 0、1、2、5、8、11、14、17、20、23、24、25、26、27 共 14 层。
`embed_tokens` 用 Q6_K，因为这个模型的词表矩阵同时充当输出层：`config.json` 中
`tie_word_embeddings` 为真，文件中没有 `lm_head.weight`，GGUF 中也没有 `output.weight`，
llama.cpp 在缺少这一项时使用 `token_embd.weight`。

张量名按 llama.cpp 的约定改写。层外的两个张量改写为 `token_embd.weight` 与
`output_norm.weight`；第 N 层的张量改写为 `blk.N.<名字>.weight`，例如 `self_attn.q_proj`
为 `attn_q`，`mlp.down_proj` 为 `ffn_down`，`input_layernorm` 为 `attn_norm`。
完整的对照表在 `src/recipe.cpp` 中。框架按 safetensors 头中的顺序写出张量。

`nano-quant plan` 只读取 safetensors 头，按上面的规则输出清单，一行一个张量：

```
token_embd.weight Q6_K 2048 151936 255252480
blk.0.attn_norm.weight F32 2048 8192
blk.0.ffn_down.weight Q6_K 6144 2048 10321920
blk.0.ffn_gate.weight Q4_K 2048 6144 7077888
```

字段依次是 GGUF 名字、格式、各维长度（按 GGUF 的顺序，`ne[0]` 在前）、字节数。
清单输出到标准输出，汇总输出到标准错误：

```
recipe q4_k_m, 28 layers, 310 tensors, 1720574976 weights
source 3441149952 bytes, output tensor data 1101457408 bytes, 5.1213 bits/weight, 3.124x smaller
skipped 315 tensors, 813914112 bytes
```

清单可以直接作为其他程序的输入，例如求字节总数：

```bash
./nano-quant plan model/model.safetensors | awk '{s += $NF} END {print s}'
```

这个总数与 `quant` 报告的数据区字节数精确相等。`ls -l` 显示的文件长度比它多出
`.nq` 的文件头与对齐填充。

### 量化与组装

```bash
python3 tools/mkmeta.py model -o model/meta.kv
./nano-quant quant model/model.safetensors --recipe q4_k_m -o model/q4_k_m.nq
./nq2gguf model/q4_k_m.nq --meta model/meta.kv -o model/q4_k_m.gguf
python3 tools/ggufdump.py model/q4_k_m.gguf
```

`quant` 在标准输出写一行：数据区的 md5、输出文件、数据区字节数。
md5 与字节数应当与 `tests/expected-qwen3vl.txt` 中的 `q4_k_m_nq_md5`、`q4_k_m_nq_bytes`
相同，`model/q4_k_m.gguf` 的字节数应当为 `q4_k_m_gguf_bytes`。把 `--recipe` 换为 `q4_0`
即得到另一组期望值。这个 md5 覆盖全部 310 个张量的数据，任何一个张量的字节有误，
md5 都不一致。

`quant` 每次读取 4 Mi 个元素（16 MB），峰值内存只与这个长度有关，与最大的张量无关。
量化整个模型的峰值常驻内存约 48 MB，q4_0 用时约 8 秒，q4_k_m 约 1 分 30 秒。

### 由 ollama 加载运行

安装 ollama：

| 系统 | 安装方法 |
| --- | --- |
| Linux、macOS、Windows 的 WSL2 | `sh tools/install-ollama.sh` |
| Windows | 运行 <https://ollama.com/download/windows> 提供的 `OllamaSetup.exe`，要求 Windows 10 及以上 |

`tools/install-ollama.sh` 下载 ollama 的官方安装脚本 <https://ollama.com/install.sh> 并运行它，
默认安装本实验验证过的 0.33.2；写作 `OLLAMA_VERSION=0.34.4 sh tools/install-ollama.sh`
时安装指定的版本，`OLLAMA_VERSION` 为空时安装最新版本。官方脚本需要 sudo，
会替换已安装的 ollama。

在 Windows 上量化得到的 GGUF 也可以交给 WSL2 中的 ollama 加载。在 WSL2 中运行
`sh tools/install-ollama.sh` 安装 ollama，然后在 WSL2 中进入 Windows 上的实验目录：
Windows 的 C 盘挂载在 `/mnt/c` 下，例如 `C:\Users\<用户名>\nano-quant` 对应
`/mnt/c/Users/<用户名>/nano-quant`。之后的命令在这个目录中执行。

先启动 ollama（`ollama serve`，或安装时注册的系统服务；Windows 上是安装后自动运行的程序），然后：

```bash
cat > model/Modelfile <<'EOF'
FROM ./q4_k_m.gguf
TEMPLATE """<|im_start|>user
{{ .Prompt }}<|im_end|>
<|im_start|>assistant
"""
PARAMETER stop <|im_end|>
PARAMETER temperature 0
EOF
ollama create nq-q4km -f model/Modelfile
ollama run nq-q4km "用一句话介绍你自己。"
```

`FROM` 中的相对路径从 `Modelfile` 所在的目录算起，因此这个 `Modelfile` 在 Linux、macOS、
Windows 与 WSL2 上相同。

按 [model/README.md](model/README.md) 准备的文件生成的 `meta.kv` 中没有对话模板，
`TEMPLATE` 一段按 Qwen 的对话格式组装提示词；
缺少这一行时 ollama 把提示词当作一段文本续写。`temperature 0` 使解码每一步取概率最大的词，
同一个模型文件上重复运行得到相同的输出。模型输出通顺的中文，说明整个流程正确。
检查结束后用 `ollama rm nq-q4km` 删除这个模型。

## 程序与工具

### nano-quant

```
nano-quant plan  <model.safetensors> [--recipe q4_0|q4_1|q4_k|q4_k_m]
nano-quant quant <model.safetensors> [--recipe ...] -o <out.nq> [--limit N]
```

`--recipe` 省略时为 `q4_k_m`。`--limit N` 只处理清单中的前 N 个张量，用于调试；
它写出的文件与期望值不可比较。

### nq2gguf

```
nq2gguf <in.nq> --meta <meta.kv> -o <out.gguf>
```

把 `.nq` 的张量数据区与 `meta.kv` 的键值段组装为 GGUF。`meta.kv` 已经是 GGUF 的键值编码，
`nq2gguf` 把它原样写入，另外补充 `general.file_type` 与 `general.quantization_version`
两个与量化有关的键。张量数据区原样复制，因此 GGUF 中的张量字节与 `.nq` 的数据区相同。

### nq-selftest

```
./nq-selftest [--expect FILE] [--emit] [--full] [组...]
```

| 用法 | 作用 |
| --- | --- |
| `./nq-selftest` | 运行全部组，与 `tests/expected-blocks.txt` 对照 |
| `./nq-selftest a q4_0` | 只运行指定的组 |
| `./nq-selftest --full a` | `f32_to_fp16` 改为遍历全部 2^32 个 float |
| `./nq-selftest --expect FILE` | 使用另一份期望值文件 |
| `./nq-selftest --emit` | 不对照，按期望值文件的格式输出本程序得到的结果 |

组名有 `a`、`scale_min`、`q4_0`、`q4_1`、`q4_k`、`q6_k`。不指定组名时运行全部组，
遇到尚未实现的函数即退出；指定组名时只运行这些组。每一项输出一行，依次是项名、
结果（`ok`、`wrong` 或一个 md5）与结论 `matches` 或 `DIFFERS`；
有一项结论为 `DIFFERS` 时退出码为 1。

### tools/mkmeta.py

```bash
python3 tools/mkmeta.py model -o model/meta.kv
```

从 `model/config.json` 与 `model/tokenizer.json` 生成 24 个键值对，
包括架构参数、词表、合并规则与特殊词元。

### tools/ggufdump.py

```
python3 tools/ggufdump.py model.gguf            只输出概况
python3 tools/ggufdump.py model.gguf --kv       同时输出键值对
python3 tools/ggufdump.py model.gguf --tensors  同时输出张量目录
```

检查三项：目录中的偏移都对齐到 `general.alignment`；相邻张量的区间首尾相接；
最后一个张量的末端与文件长度相符。任一条不成立则以非零状态退出。

### tools/mkfixtures.py

```bash
python3 tools/mkfixtures.py tests/fixtures
```

`make fixtures` 调用的就是它。生成的文件：

| 文件 | 内容 |
| --- | --- |
| `tiny.safetensors` | 四层的小模型，张量名与 Qwen3-VL 一致；另含 3 个 `model.visual.` 张量，量化时与真实模型的视觉塔一样被跳过 |
| `tiny-be.safetensors` | 与 `tiny` 相同，但头长度按大端写入 |
| `tiny-meta/` | `tiny` 对应的 `config.json` 与最小词表 |

内容由固定种子的伪随机数生成，在任何机器上字节完全相同。

### tests/run.sh

```
sh tests/run.sh          检查当前目录下的三个程序
sh tests/run.sh DIR      检查 DIR 目录下的三个程序
```

环境变量 `PYTHON` 是 Python 3 的命令名，未设置时为 `python3`；`make test PYTHON=python`
把它传给脚本。

需要先运行 `make fixtures`。脚本分四组，每组一项，每一项输出一行 `pass` 或 `FAIL`，
一项失败时继续检查其余各项，最后输出 `all passed` 或 `N failed`，退出码为失败的项数。

| 组 | 项 | 需要实现的部分 |
| --- | --- | --- |
| 1. block formats and bits | `nq-selftest` 全部组与期望值一致 | A、B |
| 2. reading safetensors | 小端头长度被接受，大端头长度被拒绝 | `rd_u64le` |
| 3. quantizing the test model | 样例模型按 `q4_k_m` 量化，数据区 md5 与 `tests/expected-tiny-nq.md5` 相同 | A、B |
| 4. GGUF assembly | 第三组的量化结果组装为 GGUF，`ggufdump.py` 检查排布自洽 | A、B |

样例模型的 46 个张量在 `q4_k_m` 配方下有 F32、Q4_K、Q6_K 三种格式，第三组的 md5
覆盖它们的全部数据；Q4_0 与 Q4_1 由第一组检查。`quant` 在写入张量数据之前已按清单
写出整个文件的长度，中途退出时留下的文件长度也正确，因此第四组只组装运行结束的
`quant` 写出的文件；第三组的 `quant` 中途退出时，第四组输出
`nothing to assemble: quant stopped with status N`。未实现任何函数时 4 项全部失败。

## `.nq` 文件

```
[0,4)    "NQ01"
[4,8)    uint32 张量数
[8,16)   uint64 数据区起点
[16,24)  uint64 数据区字节数
[24,..)  逐个张量：名字长度、名字、格式、维数、各维长度、区内偏移、字节数
```

数据区起点对齐到 32，区内每个张量也对齐到 32，空隙填零。
`quant` 报告的 md5 只计算数据区，与文件头无关。

safetensors 的 `shape` 是行优先的，`shape[0]` 变化最慢；GGUF 的 `ne` 相反，`ne[0]` 变化最快。
两者的字节序列相同，区别只在于各维长度的书写顺序。
