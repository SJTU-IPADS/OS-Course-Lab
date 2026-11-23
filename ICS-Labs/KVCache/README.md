# kvcache

注意：本实验**不计分**。

本实验的程序 `mini-ollama` 读取 Q4_0 格式的模型文件 `q4_0.gguf`，回答一段多轮对话，在 CPU 的一个线程上运行，每次运算处理一个元素。你将在 `src/kvcache.c` 中实现它的 KV Cache：对话中已有 token 的键（key）与值（value）保存在一段连续的内存中，每一步只为新 token 计算查询、键与值。使用 KV Cache 计算一个 token 的 `compute` 已在这个文件中写好，作为你理解整个流程的例子。KV Cache 的内存有上限，对话增长到放不下时，程序报告并停止。

## 程序做什么

`mini-ollama` 读取一个工作负载文件。文件的每一行是用户的一条消息，UTF-8 编码，比如 `workload.txt` 的第一条消息 `用一句话介绍你自己。`。各行依次是同一段对话的各轮，空行与 `#` 开头的行被跳过。每一轮对话分三步：

1. 把消息按模型的对话格式（`<|im_start|>user`、消息、`<|im_end|>`、`<|im_start|>assistant`）接在对话已有的文本之后；
2. 计算文本中还没有进入 KV Cache 的 token；
3. 逐个生成回答，每一步取分数最高的 token，直到模型给出结束符或回答达到 `--max-new` 个 token。回答的 token 接在对话的文本之后，下一轮从这里继续。

程序把每轮的消息与回答写到标准输出，把每轮计算的 token 数、缓存的 token 数与预留的块数写到标准错误，最后在标准错误输出整次运行的统计：计算的 token 数、KV Cache 的峰值，以及权重与中间结果所占的内存、运行时间。

## KV Cache 与内存上限

注意力层用新 token 的查询与每个已有 token 的键比较，再按比较的结果把各个 token 的值加权相加。已有 token 的键与值不随新 token 改变，所以 `mini-ollama` 把它们存入 KV Cache，每一步只为新 token 计算一行键与一行值，写入缓存，再读取缓存中的全部行。`--no-cache` 模式保存每个 token 在每层的输入向量，每一步由这些向量重新计算之前全部 token 的键与值。两种模式中每个 token 经过相同的运算，运算的次序也相同，所以回答逐字节相同，`make test` 用这一点检查你的代码。

KV Cache 占用内存。真实模型有 28 层，每层的 8 个键头各有 128 个 float，一个 token 在每层的键与值各是 1024 个 float，共占 2 × 28 × 1024 × 4 = 229376 字节（224 KiB），比如 100 个 token 占 21.9 MiB。KV Cache 的内存由框架在程序启动时按上限 `--kv-limit` 一次分配，是一段连续的内存，分成等大的块，一块存放一个 token，比如默认的上限 32 MiB 是 146 块。上限中不足一块的部分不使用。

每一轮开始时，框架调用你实现的 `kv_reserve`，为这一轮用得到的 token 预留块。对话的文本逐轮增长，一轮需要的块多于内存中的块时，这一轮放不下：`kv_reserve` 返回 0，程序报告并停止，这一轮与之后的各轮都不回答。

## 文件

你需要实现的函数都在 `src/kvcache.c` 中，`src/` 中只有这一个文件。头文件在 `include/` 中，你不能修改它们。框架代码在 `frame/` 中，检查程序与测试数据在 `tests/` 中，检查所用的两个脚本在 `tools/` 中。

| 文件 | 内容 |
| --- | --- |
| `src/kvcache.c` | KV Cache 的全部逻辑与使用它的 `compute`。`compute` 已经写好，作为例子，它之后的 5 个函数由你实现，初始时它们各自报告尚未实现并退出 |
| `include/kvcache.h` | `src/kvcache.c` 与框架共用的结构 `struct kv_cache`，函数的声明，`compute` 调用的三个框架函数，KV Cache 的内存与一块的布局图 |
| `include/mini_ollama.h` | 框架内部的接口 |
| `include/ops.h` | 算子的声明，以及每个算子计算的内容 |
| `frame/main.c` | 各轮对话（`serve`）、`compute` 调用的 `embed`、`attention` 与 `feed_forward`、`--no-cache` 的重新计算 |
| `frame/kvmem.c` | 按上限一次分配 KV Cache 的内存，设置 `base` 与 `n_blocks` |
| `frame/ops.c` | 算子：Q4_0 矩阵与向量的乘积、归一化、旋转位置编码、注意力的分数与加权和等，每次处理一个元素 |
| `frame/model.c` | 把 GGUF 文件读入内存，按名字找到每个张量 |
| `frame/tokenizer.c` | 文本转换为 token 编号，token 编号转换为文本 |
| `tests/selftest.c` | `kvcache-selftest`：不需要模型文件，按 `src/kvcache.c` 中的顺序检查你实现的每个函数 |
| `tests/` 中的其余文件 | `make test` 的工作负载检查：`workload.txt` 是一段 12 轮的对话，`check.py` 比较有缓存与无缓存的回答 |
| `tools/mkmodel.py` | 生成 `make test` 的小模型 `build/tiny.gguf` |
| `tools/kvmodel.py` | 按 `kv_reserve` 的规则重放每轮的 `capacity` 与放不下的一轮，由 `tests/check.py` 调用 |
| `workload.txt` | `make run` 的工作负载：一段 5 轮的对话 |
| `Makefile` | 编译选项与各条命令 |
| `q4_0.gguf` | 模型文件，不在发布的目录中 |

`frame/main.c` 中与 KV Cache 有关的代码是 `serve`、`reserve`、`compute_token` 与 `attention` 四个函数，你可以从这里读起。`serve` 处理一轮对话：它经 `reserve` 调用 `kv_reserve`，再按文本的顺序对每个作为输入经过模型的 token 经 `compute_token` 调用已给出的 `compute`，从第一个尚未缓存的 token 起。`reserve` 在 `kv_reserve` 返回之后检查结果：返回 0 时 `capacity` 不变，返回 1 时 `capacity` 没有减少、不小于 `n_prompt`、不大于 `n_blocks`，两种情况下 `n_cached` 与框架设置的字段都不变。`compute_token` 在调用 `compute` 之前检查 `n_cached` 等于这个 token 的位置并且小于 `capacity`，在返回之后检查 `n_cached` 恰好增加了 1。`attention` 先检查各行的地址都在第 0 到第 `capacity - 1` 块之内，再把新 token 的键与值写入 `keys[pos]` 与 `values[pos]` 指向的位置，然后按两个数组中的地址读取第 0 到第 `pos` 个 token 的各行，计算分数与加权和，之前各 token 的行从缓存中直接读取。这些检查不符时，框架报告错误并结束程序。

## 数据的布局

`struct kv_cache` 是整个缓存。框架设置其中的四个字段：`n_layer` 是模型的层数，`kv_dim` 是一个 token 在一层的键的 float 数，也是值的 float 数，`base` 是 KV Cache 的内存中第一个字节的地址，`n_blocks` 是内存中的块数。你实现的函数维护另外两个字段：`n_cached` 是已缓存的 token 数，`capacity` 是已预留的块数。内存中的各块分为三段：

- 第 0 到 `n_cached - 1` 块是已缓存 token 的块，其中的键与值有效；
- 第 `n_cached` 到 `capacity - 1` 块是预留的块，留给这一轮中之后计算的 token，其中的内容还没有意义；
- 第 `capacity` 到 `n_blocks - 1` 块还没有使用。

```
struct kv_cache: n_layer, kv_dim, base, n_blocks, n_cached, capacity

base
|
v
+---------+---------+---------+---------+---------+---------+-----+---------+
| block 0 | block 1 | block 2 | block 3 | block 4 | block 5 | ... |  last   |
| token 0 | token 1 | token 2 | token 3 | token 4 | token 5 |     |         |
+---------+---------+---------+---------+---------+---------+-----+---------+
|<---------- cached --------->|<---- reserved --->|<----- not used yet ---->|
0                             n_cached 3          capacity 5         n_blocks

one block = one token
+-------------+---------------+-------------+---------------+-----+---------------------+
| layer 0 key | layer 0 value | layer 1 key | layer 1 value | ... | last layer's value  |
+-------------+---------------+-------------+---------------+-----+---------------------+
  kv_dim floats in each part, with nothing between them
```

一块存放一个 token 在全部层的键与值：依次是第 0 层的键、第 0 层的值、第 1 层的键、第 1 层的值，直到最后一层的值，每段 `kv_dim` 个 float。各块按 token 的编号依次相接，块与块之间没有间隔，第 `tok` 个 token 的块从 `base` 之后 `tok` 块的位置开始。找到一行分为两步：先由 token 的编号算出它的块的起始地址，再加上这一层的键或值在块内的偏移。`base` 的类型是 `unsigned char *`，在它上面加的是字节数，块内的偏移以 float 计，比如 `n_layer` 为 2、`kv_dim` 为 10 时一块有 40 个 float，即 160 字节，token 2 的块从 `base + 320` 开始，块中在第 1 层的值之前有第 0 层的键与值和第 1 层的键，共 30 个 float，所以 token 2 在第 1 层的值的地址为 `base + 320 + 30 × 4`，即 `base + 440`。

对话增长时 `kv_reserve` 只增大 `capacity`：新 token 的块紧接在已有的块之后，已有的块的地址与内容都不变，所以增长不需要分配、移动或复制。

## 你需要实现的函数

`src/kvcache.c` 中的 `compute` 已经写好，作为例子：它示范一个 token 经过模型的完整流程，以及你实现的函数在其中怎样被使用。你需要先阅读这个函数，再按文件中的顺序实现它之后的 5 个函数，这 5 个函数前的注释各自写明了步骤与边界情况。`kvcache-selftest` 也按这个顺序检查这 5 个函数，每项检查只用到它之前的函数。

| 函数 | 作用 |
| --- | --- |
| `compute(cache, token, state)`（已给出） | 使位置为 `n_cached` 的 token 经过模型的全部层：在每层用 `kv_key` 与 `kv_value` 得到各行的地址，最后用 `kv_append` 把它记为已缓存 |
| `kv_block_bytes(cache)` | 一块的字节数，即一个 token 在全部层的键与值，用 `size_t` 计算。框架用它把 KV Cache 的内存分成块 |
| `kv_key(cache, layer, tok)` | 第 `tok` 个 token 在这一层的键的地址：由 `base` 与 token 的编号算出它的块，再加上块内前面各层的键与值 |
| `kv_value(cache, layer, tok)` | 第 `tok` 个 token 在这一层的值的地址：同一块中，紧接在这一层的键之后 |
| `kv_append(cache)` | `attention` 已把一个 token 在各层的键与值写入第 `n_cached` 块：`n_cached` 加 1 |
| `kv_reserve(cache, n_prompt, max_answer)` | 算出这一轮用得到的块数，放得下时预留这些块，放不下时返回 0 |

`compute` 已经写好，你需要先读懂它：它把 KV Cache 与模型的计算连接起来，你实现的 `kv_key`、`kv_value` 与 `kv_append` 都由它调用。框架按文本的顺序对每个作为输入经过模型的 token 各调用它一次，从第一个尚未缓存的 token 起，所以这个 token 的位置 `pos` 就是调用时的 `n_cached`，它在各层都相同。`compute` 先用 `embed` 得到这个 token 的向量 `state`，再使它依次经过各层。在每一层，它用 `kv_key` 与 `kv_value` 把第 0 到第 `pos` 个 token 各行的地址收集到两个指针数组中，交给 `attention`，再调用 `feed_forward`。`attention` 先写入第 `pos` 个 token 的键与值，再读取第 0 到第 `pos` 个 token 的各行，所以你实现的 `kv_key` 与 `kv_value` 既决定新 token 的键与值写到哪里，也决定已有 token 的键与值从哪里读出。全部层完成之后，它调用一次 `kv_append`，这个 token 从此属于已缓存的 token。返回时 `state` 是最后一层之后的向量，框架据此预测下一个 token。`compute` 使用框架的三个函数，它们在 `include/kvcache.h` 中声明：

- `embed(token, state)`：读取嵌入矩阵中这个 token 的一行，把它写入 `state`；
- `attention(layer, pos, state, keys, values)`：`keys[tok]` 与 `values[tok]` 是第 `tok` 个 token 在这一层的键与值的地址，`tok` 从 0 到 `pos`。它读取 `state`，把这个 token 的键与值写到 `keys[pos]` 与 `values[pos]` 指向的位置，再读取两个数组所指的各行，把注意力的输出加到 `state` 上；
- `feed_forward(layer, state)`：读取 `state`，把这一层前馈网络的输出加到 `state` 上。

框架在每一轮开始时调用 `kv_reserve` 一次，给出两个量：`n_prompt` 是接上本轮消息之后对话文本的 token 数，之前各轮的文本也计算在内。`max_answer` 是这一轮最多生成的回答 token 数，不小于 1。规则如下：

1. 你需要先算出 `n_tokens`，即这一轮结束时最多缓存的 token 数。它由三个事实决定：一个 token 作为输入经过 `compute` 时才被缓存，回答的每个 token 是模型根据前一个 token 的 `state` 给出的输出，回答在生成第 `max_answer` 个 token 时停止，也可能因结束符提前停止。你需要恰好预留这一轮用得到的块；
2. `capacity` 不小于 `n_tokens` 时，块已经足够，返回 1，不改变任何字段；
3. `n_tokens` 大于 `n_blocks` 时，这一轮放不下，返回 0，不改变任何字段；
4. 其余情况下，把 `capacity` 改为 `n_tokens`，返回 1。`n_cached` 不变，内存中的内容也不变。

`n_tokens` 恰好等于 `n_blocks` 时放得下，这一轮用到全部的块。`kv_reserve` 返回 0 时，`mini-ollama` 输出 `turn ... does not fit: ... the program stops here (give a larger --kv-limit)`，以状态 1 结束。`capacity` 恰好取 `n_tokens`。`tools/kvmodel.py` 按同样的规则计算每轮的 `capacity` 与放不下的一轮，与你的代码比较，多留一块或少留一块都不能通过检查。

你实现的函数可以调用 `die`（`include/mini_ollama.h`，按 `printf` 的格式输出消息并结束程序）。`embed`、`attention`、`feed_forward` 已由给出的 `compute` 调用，你实现的函数不需要调用它们。

## 模型文件

`make run` 读取模型文件 `q4_0.gguf`：Qwen3-VL-2B-Instruct 的语言模型，每个矩阵为 Q4_0 格式，974196320 字节。`make test` 与 `kvcache-selftest` 不需要它。你需要把它放在本目录下，文件名为 `q4_0.gguf`，两种方式任选一种：

- 从交大云盘下载：<https://pan.sjtu.edu.cn/web/share/a853ce437a176a0064df9a5af1942ecf>，把下载的 `q4_0.gguf` 移动到本目录；
- 你的机器上已有这个文件时（比如 `parallel-dot` 或 `nano-quant` 实验中的 `q4_0.gguf`），把它移动到本目录，或者在本目录建立指向它的链接。

```sh
ln -s ../parallel-dot/q4_0.gguf .    # 符号链接：Linux 与 WSL2
ln ../parallel-dot/q4_0.gguf .       # 硬链接：两个路径须在同一个分区上
```

`ls -lL q4_0.gguf` 显示的大小应当是 974196320。文件放在别处时，你需要在命令中给出它的路径，比如 `make run MODEL=路径`。缺少模型文件时，`make run` 在编译之前报告 `no model file q4_0.gguf` 并退出。`.gitignore` 忽略本目录下的 `*.gguf`。

## 命令

```sh
make              # 生成 mini-ollama 与 kvcache-selftest
./kvcache-selftest   # 按顺序检查 src/kvcache.c 中你实现的每个函数，不需要模型文件
make test         # kvcache-selftest，然后在小模型上运行 tests/workload.txt，与 --no-cache 比较，需要 Python 3
make run          # 用真实模型回答 workload.txt：有缓存与 --no-cache 各一次
make clean
```

`mini-ollama` 也可以直接运行：

```sh
./mini-ollama [--kv-limit SIZE] [--max-new N] [--no-cache] [--trace FILE] MODEL.gguf WORKLOAD.txt
```

| 选项 | 作用 |
| --- | --- |
| `--kv-limit SIZE` | KV Cache 的内存的上限，单位为字节，可以带后缀 `K`、`M`、`G`（1024 的幂），默认 `32M`。内存中的块数是这个上限中整块的数目 |
| `--max-new N` | 每轮回答最多的 token 数，默认 8 |
| `--no-cache` | 不使用 KV Cache，每一步重新计算之前全部 token 的键与值。它不调用 `src/kvcache.c`，你实现之前就可以运行 |
| `--trace FILE` | 把 KV Cache 的块数，每轮 `kv_reserve` 的两个参数与调用之后的 `capacity`，以及最后的峰值写入 `FILE`，供 `tools/kvmodel.py` 重放 |

`make run` 的变量：`MODEL`、`WORKLOAD`、`LIMIT`（默认 `32M`）与 `MAX_NEW`（默认 8），在命令中给出，比如 `make run MODEL=路径`。Python 不叫 `python3` 时，你需要运行 `make test PYTHON=python`。

## `make test` 的输出

`make test` 先运行 `kvcache-selftest`。初始的 `src/kvcache.c` 在你需要实现的第一个函数 `kv_block_bytes` 处停止：

```
./kvcache-selftest
kvcache.c on a cache of 3 layers with rows of 8 floats: 192 bytes per token
kv_block_bytes is not implemented yet
make: *** [Makefile:86: test] Error 3
```

你每实现一个函数，检查就前进一段。`kvcache-selftest` 中 KV Cache 的内存是检查程序中的一个数组，`base` 不是 0，也不是 8 的倍数，所以 `kv_key` 需要从 `base` 算起。`kvcache-selftest` 全部通过之后，`tests/check.py` 用 `tools/mkmodel.py` 生成的小模型回答 `tests/workload.txt`。小模型的权重是随机数，有 3 层，一块占 768 字节，回答是没有含义的 ASCII 字符。`check.py` 先以 `--no-cache` 运行一次，得到参照的回答，再在 4 个上限下各运行一次，每次检查两项：

- 回答与 `--no-cache` 逐字节相同；
- `tools/kvmodel.py` 重放这次运行的记录，每轮 `kv_reserve` 之后的 `capacity`、最后的峰值，以及放不下的一轮，都与规则一致。

上限 1M 的空间有余，这次运行结束时的 `capacity` 就是整段对话需要的块数。第二个上限由这个块数乘以一块的字节数得出，恰好放下整段对话，最后一轮用到全部的块，比如下面的输出中是 276 块，即 207K。第三个上限比第二个少一个字节，在最后一轮差一块，放不下。1M 的一次没有通过时，这两个上限无法算出，`check.py` 不运行它们，输出一行 `FAIL`。60K 在对话的中途放不下，比如下面的输出中是第 4 轮。放不下时，程序应当在规则给出的那一轮停止，停止之前的回答与 `--no-cache` 相同。全部完成时的输出如下：

```
./kvcache-selftest
kvcache.c on a cache of 3 layers with rows of 8 floats: 192 bytes per token
  pass  kv_block_bytes: 192 bytes here, 229376 bytes for the real model (28 layers, rows of 1024)
  pass  kv_block_bytes: 4096 layers with rows of 262144 floats, beyond the largest int
  pass  kv_key: block tok lies tok blocks after base, then the layer's place in it
  pass  kv_value: in the block of the token, right after the key of the same layer
  pass  kv_append: three calls, three tokens cached, nothing else changes
  pass  kv_reserve, max_answer 1: the first turn gets blocks for what it can cache, no more
  pass  kv_reserve, max_answer 4: the first turn gets blocks for what it can cache, no more
  pass  kv_reserve, 4 tokens cached: a later turn gets blocks for what it can cache, no more
  pass  kv_reserve: growing leaves the cached tokens where they are, nothing moved or copied
  pass  kv_reserve: a turn that fits in the capacity changes nothing
  pass  kv_reserve: the conversation can grow until it has every block of the cache
  pass  kv_reserve: returns 0 and changes nothing for more tokens than the cache has blocks
all passed
python3 tests/check.py ./mini-ollama ./build/tiny.gguf tests/workload.txt ./build
workload: workload.txt on tiny.gguf, 6 answer tokens per turn, against --no-cache
  pass  limit 1M: answers as --no-cache; 12 turns, capacity 276 of 1365 blocks at the end: as the rules give
  pass  limit 207K: answers as --no-cache; 12 turns, capacity 276 of 276 blocks at the end: as the rules give
  pass  limit 211967: 12 turns, the conversation does not fit at turn 12: as the rules give
  pass  limit 60K: 4 turns, the conversation does not fit at turn 4: as the rules give
all passed
```

小模型的回答与数学函数库的实现有关，在其他机器上回答的长度可以不同，第二、第三个上限、各轮的 `capacity` 与放不下的一轮随之不同，每一行都应当是 `pass`。最后一行是 `all passed` 时实验完成。某一行是 `FAIL` 时，它写明哪一轮与规则或参照不同，比如 `FAIL  limit 1M: the answers differ from --no-cache from the turn [turn 1] What is memory? on`。

## 实验：有缓存与无缓存

`make run` 用真实模型回答 `workload.txt` 的 5 轮对话，上限 32 MiB 即 146 块，每轮回答最多 8 个 token，依次运行有缓存与 `--no-cache` 两次。两次运行的回答逐字节相同：

```
[turn 1] 用一句话介绍你自己。
我是一个虚拟助手，没有实体，

[turn 2] 什么是缓存？
缓存（Cache）是计算机中

[turn 3] 它为什么快？
因为缓存是计算机中用于存储

[turn 4] 内存和硬盘有什么区别？
内存（RAM）和硬盘（H

[turn 5] 哪一个更便宜？
内存（RAM）通常比硬盘（
```

下表是一次 `make run` 的统计，两次运行共约 13 分钟，时间随机器而异。

| 运行 | 计算的 token | 重新计算键与值的次数 | KV Cache 的峰值 | 时间 |
| --- | --- | --- | --- | --- |
| 有缓存 | 110 | 无 | 110 块，24.06 MiB | 116.8 s |
| `--no-cache` | 110 | 5995 | 无 | 653.0 s |
