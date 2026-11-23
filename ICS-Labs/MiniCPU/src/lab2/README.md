# 实验二：组相联缓存

注意：本实验**计分**。

这个实验里，你将在 `cache.c` 中实现处理器与内存之间的一级组相联缓存，以替换实验一使用的 `frame/cpu/nocache.c`。实验一中，`nocache.c` 不模拟缓存，每次访存用 `memcpy` 读写 `dram.img` 的映射中所访问的字节。而本实验之后，内存只能以 64 字节的行为单位读写，所有访存都经过你的缓存：命中时在缓存的行中完成，缺失时先从内存中读入一整行。

缓存的要求：组相联，写回（write back），写分配（write allocation），组内按 LRU 替换，一行 64 字节。

`cache.c` 已经给出缓存行的结构体 `cache_line` 与保存全部行的数组 `cache_lines`。文件中的第一个函数 `cache_init` 已经写出，作为参考：它建立一个空的缓存，示范代码怎样使用 `cache_lines`、`cache_set_count` 与 `cache_way_count`。你需要实现其余十个函数，每个函数的函数体是一段 `TODO` 注释加一个 `todo("名字")` 调用，每个 `TODO` 注释写明函数的返回值、步骤、哪个计数器在什么时候改变，以及边界情况。

## 访存的路径

```
src/lab1/interp.c    解释器，经 frame/cpu/x86.c 的函数访存：
                       x86_begin 取指，调用 fetch_bytes
                       load、store 等函数读写数据，调用 mem_read、mem_write
frame/os/sysbase.c   系统调用读写程序的缓冲区，调用 frame/os/copy.c 的 guest_read、guest_write：
                       按行切分缓冲区，每段调用一次 mem_read 或 mem_write
       │
       v
frame/cpu/mem.c      CPU 的访存单元：fetch_bytes、mem_read、mem_write 在行边界处切开一次访存，
                     每一行调用一次 cache_read 或 cache_write
       │
       v
src/lab2/cache.c     cache_read、cache_write：缓存的行是宿主内存中的数组
       │             缺失时读入一行，替换脏行时写回一行
       v
frame/cpu/dram.c     dram_line_read、dram_line_write：内存是文件 dram.img 的映射，每一行都读写宿主的内存
```

`frame/cpu/` 中的代码模拟机器的硬件，`frame/os/` 中的代码是在这台机器上运行的软件。取指、读写数据、系统调用读写缓冲区都经过缓存。程序运行时，只有 `frame/cpu/mem.c` 调用 `cache_read` 与 `cache_write`。

处理器的一次访问不跨过缓存行。一条指令的访存跨过行边界时，访存单元把它拆成每行一次调用，比如从行内偏移 60 处读 8 字节是两次调用，先读这一行的最后 4 字节，再读下一行的前 4 字节。系统调用复制一段任意长的缓冲区时，`guest_read` 与 `guest_write` 像 `memcpy` 的循环一样在行边界处切分，每段调用一次 `mem_read` 或 `mem_write`。

因此 `cache_read` 与 `cache_write` 收到的字节总在同一行内：长度 `nbytes` 为 1 到 64，满足 `pa % 64 + nbytes <= 64`，一次调用恰好是一次缓存访问。框架检查每一次调用，收到跨行的调用时打印 `framework error` 并以状态 4 退出。

## 地址的划分

一行固定为 64 字节，即 `include/mcpu.h` 中的 `LINE_SIZE`，而命令行参数 `--cache S,W` 只设置组数 S 与每组的行数 W。地址分为三段，默认的 `--cache 64,4` 有 64 组，三段的位置如下：

```
 63                       12 11           6 5            0
+---------------------------+--------------+--------------+
|            tag            |     组号      |   行内偏移    |
+---------------------------+--------------+--------------+
```

| 字段 | 计算 | 含义 |
| --- | --- | --- |
| 行内偏移 | `pa % 64` | 字节在行中的位置 |
| 行号 | `pa / 64` | 内存中的第几行 |
| 组号 | 行号 % 组数 | 这一行只能放在这一组中 |
| tag | 行号 / 组数 | 区分放在同一组中的不同行 |

每组的行数 W 即相联度。默认的 `--cache 64,4` 共 256 行，数据 16 KiB。由 tag 与组号可以重建行的起始地址 `(tag * 组数 + 组号) * 64`，写回与读入都使用这个地址，比如 64 组时，`pa = 0x12345` 的行内偏移为 `0x05`，行号为 `0x48d`，组号为 13，tag 为 `0x12`，行的起始地址为 `(0x12 * 64 + 13) * 64 = 0x12340`。

## 数据结构

`cache.c` 开头的注释画出了缓存的结构：`cache_set_count` 组，每组 `cache_way_count` 行。一行是一个 `cache_line`：

| 字段 | 含义 |
| --- | --- |
| `valid` | 1 表示这一行保存了内存中某一行的副本，0 表示这一行是空的 |
| `dirty` | 1 表示副本读入之后被写过，内存中的内容较旧，这一行离开缓存之前必须写回 |
| `tag` | 副本所在地址的 tag |
| `last_use` | 最近一次访问时 `cache_use_clock` 的值 |
| `data` | 64 字节的内容 |

全部行放在一个数组 `cache_lines` 中，一组的各行相邻：第 `set_index` 组的第 `way` 行是 `cache_lines[set_index * cache_way_count + way]`。`cache_use_clock` 每访问一行加一，因此同一组中 `last_use` 最小的行是最近最少使用的行。

`frame/cpu/dram.c` 中定义的三个计数器由你的函数增加：

| 变量 | 含义 |
| --- | --- |
| `cache_hits` | 命中的行数 |
| `cache_misses` | 缺失的行数 |
| `cache_writebacks` | 写回内存的行数，包括替换时与 `cache_flush` 时的写回 |

计数以行为单位，`cache_read` 与 `cache_write` 的每次调用访问一行，计一次。

## 内存与缓存的读写

模拟机器的内存是文件 `dram.img`，缓存的行保存在数组 `cache_lines` 中，这个数组在 mini-cpu 进程的内存里，由于它很小，可以认为它常驻在 CPU 的缓存中。两级的读写方式如下：

| 操作 | 实现 |
| --- | --- |
| 命中：在缓存的行中读写 | 读写数组 `cache_lines`，通常在宿主 CPU 的缓存中命中 |
| 读写内存中的一行：`dram_line_read` 或 `dram_line_write` | 在 `dram.img` 的映射中读写这一行，并用 `clflush` 指令把它移出宿主 CPU 的缓存：读之前移出，读到的是宿主内存中的字节，写之后移出，写入的字节到达宿主的内存 |

`--cache 0` 让同一个程序在没有缓存的机器上运行：带有这个命令行参数时，框架会绕过你的缓存实现，一次访存涉及的每一行都会实际读写一次内存，读出所读的字节，或只写入所写的字节。这时 `--stats` 的 `cache` 一行为 `cache off`。

## 运行与检查

```bash
make LAB=2
./mini-cpu --stats guest/matmul
./mini-cpu --stats --cache 0 guest/matmul
```

实验二起 `--stats` 增加两行。正确实现下某次运行结果如下：

```
sum = 15420672
insns 215568  syscalls 3  exit 0
cache accesses 277529  hits 277291  misses 238  hit rate 99.91%  writebacks 218
dram  line reads 238  line writes 218
```

同一程序加 `--cache 0` 时后两行为：

```
cache off
dram  line reads 275790  line writes 1739
```

没有缓存时 277529 次行访问全部读写内存，有缓存时只有 238 次读入与 218 次写回，读写内存的行数相差约 600 倍。其他程序的结果：

| 命令 | `cache` 一行 | `dram` 一行 |
| --- | --- | --- |
| `./mini-cpu --stats guest/sum` | `cache accesses 578  hits 569  misses 9  hit rate 98.44%  writebacks 1` | `dram  line reads 9  line writes 1` |
| `./mini-cpu --stats --cache 0 guest/sum` | `cache off` | `dram  line reads 571  line writes 7` |
| `./mini-cpu --stats guest/rowcol rows` | `cache accesses 1619325  hits 1610053  misses 9272  hit rate 99.43%  writebacks 1035` | `dram  line reads 9272  line writes 1035` |
| `./mini-cpu --stats guest/rowcol cols` | `cache accesses 1619308  hits 1487179  misses 132129  hit rate 91.84%  writebacks 1035` | `dram  line reads 132129  line writes 1035` |
| `./mini-cpu --stats --cache 0 guest/rowcol rows` | `cache off` | `dram  line reads 1602919  line writes 16406` |
| `./mini-cpu --stats --cache 0 guest/rowcol cols` | `cache off` | `dram  line reads 1602902  line writes 16406` |
| `./mini-cpu --stats --cache 64,1 guest/matmul` | `cache accesses 277529  hits 272686  misses 4843  hit rate 98.25%  writebacks 287` | `dram  line reads 4843  line writes 287` |
| `./mini-cpu --stats --cache 16,4 guest/matmul` | `cache accesses 277529  hits 275588  misses 1941  hit rate 99.30%  writebacks 246` | `dram  line reads 1941  line writes 246` |

程序的输出与指令数与实验一相同。计数与命令中程序的路径有关，路径字符串在客户机的栈上，因此你需要按表中的写法运行。

运行时间由 `time` 测量，它与宿主机有关。下表是一次测量的结果，取五次中最快的一次，保留两位小数。最后一列是 `make LAB=1` 构建的实验一的机器：

| 命令 | 有缓存 | `--cache 0` | 实验一的机器 |
| --- | --- | --- | --- |
| `time ./mini-cpu guest/matmul` | 0.01 秒 | 0.06 秒 | 0.01 秒 |
| `time ./mini-cpu guest/rowcol rows` | 0.06 秒 | 0.31 秒 | 0.06 秒 |
| `time ./mini-cpu guest/rowcol cols` | 0.09 秒 | 0.33 秒 | 0.06 秒 |

`./mini-unit cache` 不运行客户机程序，像访存单元一样每行调用一次，直接调用框架使用的四个函数：一行末尾与下一行开头的访问，只在替换与 `cache_flush` 时写回，组内最近最少使用的行，第二次 `cache_init`，3 组的缓存。每个失败的检查打印一行（最多 12 行），最后一行是 `cache: N checks, M failed`，没有失败时退出状态为 0。

## 评测

```bash
make check                 # 或 sh tests/lab2/run.sh
```

`tests/lab2/run.sh` 共 100 分。给定一个访问序列，写回、写分配与组内 LRU 的规则决定了每一个计数。每次运行以 `--memtrace` 写出访问轨迹，`tools/cachemodel.py S W` 按同样的规则重放这条轨迹，`--stats` 的 `cache` 与 `dram` 两行必须与它打印的两行相同。最后三项检查缓存的作用，前两项比较读写内存的行数，即 `dram` 一行的行读取数与行写入数之和，第三项比较运行时间。项目名是 `make check` 打印的名字。

| 分值 | 项目 | 检查的内容 |
| --- | --- | --- |
| 20 | `unit: cache_init, cache_read, cache_write, cache_flush` | `./mini-unit cache` 没有失败的检查 |
| 10 | `matmul: counts of the model, --cache 64,4` | 默认参数下 `guest/matmul` 的两行与模型相同 |
| 5 | `sum: counts of the model, --cache 64,4` | `guest/sum` 的两行 |
| 10 | `matmul: --cache 64,1 and 16,4` | 两组参数下 `guest/matmul` 的两行 |
| 5 | `block 8: --cache 16,1` | `guest/block 8` 的两行与输出 |
| 5 | `dram: one line per miss and per writeback` | `--cache 64,4`、`16,1`、`4,2` 下 `guest/matmul` 的缺失数等于行读取数，写回数等于行写入数 |
| 5 | `programs: same results for four other caches` | `--cache 1,1`、`8,1`、`16,2`、`256,8` 下 `guest/matmul` 与 `guest/simd` 的输出 |
| 15 | `random caches: counts of tools/cachemodel.py` | 两次随机的 `--cache S,W`（S 为 1 到 256 之间 2 的幂，W 为 1 到 8）运行随机种子的 `guest/tinytf`：输出与 `reference.py tinytf` 相同，两行与模型相同 |
| 10 | `speedup: DRAM line reads and writes with the cache at most 1/10 of --cache 0` | `guest/matmul` 与 `guest/rowcol rows`：默认缓存下读写内存的行数不超过 `--cache 0` 时的 1/10 |
| 5 | `locality: rowcol by columns at least 3 times the DRAM line reads and writes by rows` | 默认缓存下 `guest/rowcol cols` 读写内存的行数至少是 `guest/rowcol rows` 的 3 倍 |
| 10 | `wall clock: rowcol with the cache at least 2 times faster than --cache 0` | `guest/rowcol rows` 有缓存与 `--cache 0` 各运行三次，取最快的一次，`--cache 0` 的时间至少是有缓存时的 2 倍 |

运行时间一项不通过时，失败信息打印测得的时间。在模拟 x86-64 的环境中，比如 arm64 macOS 上的 x86-64 容器，`clflush` 可能不起作用，读写内存与命中的用时相近，比较时间没有意义，此时不再检查运行时间。

调试计数时你可以自己运行与测试相同的比较：

```bash
./mini-cpu --cache 16,2 --stats --memtrace t.txt guest/sum
python3 tools/cachemodel.py 16 2 < t.txt
```

随机参数由 `make check` 第一行打印的种子决定，`SEED=种子 make check` 重复同一次评测。测试之前 `tests/srccheck.sh` 检查源文件：你的代码只能使用 `tests/symbols.txt` 中 `lab2` 与 `libc` 两行的名字，规则见 [`../../README.md`](../../README.md) 的“源文件检查”。

## 附：框架中与本实验有关的文件

| 文件 | 作用 |
| --- | --- |
| `src/lab2/cache.c` | 你的代码：缓存行的结构、已经写出的 `cache_init` 与由你实现的十个函数，开头的注释画出了访存的路径、地址的划分与缓存的结构 |
| `include/mcpu.h` | 四个函数的接口，`LINE_SIZE`，`dram_line_read`、`dram_line_write` 的声明 |
| `frame/cpu/dram.c` | 内存，即文件 `dram.img`：`dram_line_read`、`dram_line_write` 一次读写一整行，用 `clflush` 让这一行绕过宿主 CPU 的缓存，并计数。另有 `--cache 0` 时的 `uncached_read` 与 `uncached_write`，每次调用读写一次内存，打印 `--stats` 两行的 `memory_stats`，三个计数器 `cache_hits`、`cache_misses`、`cache_writebacks` 的定义 |
| `frame/cpu/mem.c` | CPU 的访存单元，所有访存的入口：`fetch_bytes`、`mem_read`、`mem_write` 在行边界处切开一次访存，每行调用一次 `cache_read` 或 `cache_write` |
| `frame/os/copy.c` | `guest_read`、`guest_write`：系统调用复制缓冲区时按行切分，逐段调用 `mem_read`、`mem_write` |
| `frame/memtrace.c` | 写出 `--memtrace` 的轨迹，检查每次 `cache_read`、`cache_write` 都在一行之内，`--cache 0` 时把它们转给 `uncached_read`、`uncached_write` |
| `frame/cpu/nocache.c` | 本实验之前的替代实现，用 `dram_ptr` 直接读写 `dram.img` 的映射 |
| `tools/cachemodel.py` | 由轨迹重放缓存，轨迹中每条读写都在一行之内，打印 `--stats` 的两行，`python3 tools/cachemodel.py 0` 计算没有缓存时的两行 |
| `guest/rowcol.c` | 按行或按列对矩阵求和 |
| `guest/block.c` | 分块矩阵乘法，块大小由命令行参数给出 |
