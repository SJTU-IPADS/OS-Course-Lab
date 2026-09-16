"""第二讲《数据的表示》的页面内容。

组织方式：以上一讲留下的模型权重文件为观察对象，把「1.9 GB 里的每个字节
表示什么」拆成四个问题——一个字节是什么、多字节对象如何排列、同一段位如何
得到不同的值、实数与低精度格式如何编码——逐节回答。每一节的结论都落回同一个
文件，最后一页把文件头逐字段读完。

用语要求：面向本科课程教学，采用陈述性的技术表述；不使用比喻、口语化措辞。

排版约定：
- 粗体后面不要紧跟全角冒号。`**词：**内容` 不符合 CommonMark 的闭合规则，
  会原样把 `**` 打到投影上；写成 `**词**：内容`。
- `==标记==` 只在 `p.slide(...)` 里展开，写进 highlight / 图注 / 表格会变成字面量。
- 一页最多一个 `p.highlight`，并且只给真正的结论用。
- 代码注释统一写英文，中英两版共用。
"""

# ==============================================================================
# 共享代码清单
# ==============================================================================

HEX_GROUPS = """0x173A4C
十六进制      1    7    3    A    4    C
二进制      0001 0111 0011 1010 0100 1100"""

READ_FIELD = """static uint32_t le32(const unsigned char *b) {          /* little endian */
    return (uint32_t) b[0]       | (uint32_t) b[1] << 8 |
           (uint32_t) b[2] << 16 | (uint32_t) b[3] << 24;
}
static uint32_t be32(const unsigned char *b) {          /* big endian */
    return (uint32_t) b[3]       | (uint32_t) b[2] << 8 |
           (uint32_t) b[1] << 16 | (uint32_t) b[0] << 24;
}"""

SHOW_BYTES = """typedef unsigned char *byte_pointer;      /* a pointer to plain bytes */

void show_bytes(byte_pointer start, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf(" %.2x", start[i]);            /* one byte at a time */
}"""

CAST_SAMPLE = """short          x  =  12345;                /* 0x3039 */
unsigned short ux = (unsigned short) x;    /* 0x3039, reads as 12345 */

short          y  = -12345;                /* 0xcfc7 */
unsigned short uy = (unsigned short) y;    /* 0xcfc7, reads as 53191 */"""

KERNEL_BUG = """void *memcpy(void *dest, const void *src, size_t n);   /* <string.h> */

#define KSIZE 1024
char kbuf[KSIZE];                       /* kernel memory the user may read */

int copy_from_kernel(void *user_dest, int maxlen) {
    int len = KSIZE < maxlen ? KSIZE : maxlen;   /* never more than the buffer */
    memcpy(user_dest, kbuf, len);                /* hand that many bytes over */
    return len;
}"""

MASK_SAMPLE = """x & 0x0f            /* keep the low four bits, clear the rest */
x | 0x80            /* set bit 7, leave the others alone  */
x & ~0x80           /* clear bit 7, leave the others alone */
x ^ y               /* the bits where x and y differ       */"""

NIBBLE_ACCESS = """static uint8_t lo4(uint8_t b) { return b & 0x0f; }        /* weight j      */
static uint8_t hi4(uint8_t b) { return b >> 4;   }        /* weight j + 16 */

static uint8_t pack(uint8_t lo, uint8_t hi) {
    return (uint8_t) (hi << 4 | (lo & 0x0f));             /* two into one byte */
}"""

NIBBLE_ADD = """static uint8_t add_naive(uint8_t a, uint8_t b) {
    return (uint8_t) (a + b);                          /* a carry crosses bit 3 */
}
static uint8_t add_packed(uint8_t a, uint8_t b) {
    uint8_t s = (uint8_t) ((a & 0x77) + (b & 0x77));   /* no carry leaves a lane */
    return (uint8_t) (s ^ ((a ^ b) & 0x88));           /* bit 3 added, carry dropped */
}"""

QUANT_CORE = """void quantize(const float *x, block_q4_0 *out) {
    float d  = extreme(x, 32) / -8.0f;         /* codes 0..15 mean -8..7 */
    float id = d ? 1.0f / d : 0.0f;
    out->d = f32_to_f16(d);                    /* the scale is stored as fp16 */
    for (int j = 0; j < 16; j++) {             /* weight j and weight j + 16 */
        int lo = clamp((long) (x[j]      * id + 8.5f));
        int hi = clamp((long) (x[j + 16] * id + 8.5f));
        out->qs[j] = (uint8_t) (hi << 4 | lo); /* two weights, one byte */
    }
}"""

K_SCALES = """static void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m) {
    if (j < 4) {                    /* 0..3: one whole low six bits */
        *sc = q[j] & 63;
        *m  = q[j + 4] & 63;
    } else {                        /* 4..7: split across two bytes */
        *sc = (q[j + 4] & 0x0f) | (q[j - 4] >> 6) << 4;
        *m  = (q[j + 4] >> 4)   | (q[j]     >> 6) << 4;
    }
}"""


BF16_ROUND = """uint16_t bf16 = (uint16_t) (f32 >> 16);   /* sign, exponent, top 7 fraction bits */
uint16_t rest = (uint16_t) f32;           /* the 16 dropped bits */
if (rest > 0x8000 || (rest == 0x8000 && (bf16 & 1)))
    bf16 += 1;                            /* round to nearest even; may carry into exp */"""


# ==============================================================================
# 回顾与本节的问题
# ==============================================================================

def course_info(p):
    p.gap(28)
    p.title("课程信息")
    p.slide("""
1. **课件下载**：https://github.com/SJTU-IPADS/OS-Course-Lab/tree/OS-Pre-Course-CSAPP
   便于实时更新、团队共建、师生共建
2. **课件使用**：问 AI 即可；欢迎丰富使用方式
3. **微信群**
""")
    p.slide("""
4. **课程安排**
   - 周六上课：从课程融合到实际课时压缩
   - 周中习题课改为讨论课
   - 从 A1 / A2 到 A0，再到 B / C：ICS 重回兵器谱前 XX，打造人气 TA
   - **臧斌宇老师**　ICS 课程在中国的创始人
     **伟大导师　·　伟大舵手　·　伟大奠基人　·　伟大引路人**
""").image_right("assets/ext/zang-binyu.jpg", width_px=160
    ).footnote("照片来自上海交通大学并行与分布式系统研究所（IPADS）成员页。")
    p.slide("""
5. **考核**：quiz + 期末 + lab，约 20 + 50 + 30；君子协定：对于 AI 的使用
""")


def ollama_intro(p):
    p.gap(30)
    p.title("回顾：用 Ollama 执行本地推理")
    p.slide("""
Ollama 是运行在本机的推理服务，提供 OpenAI 兼容的 HTTP 接口。
- 启动后监听 `http://localhost:11434`，`/v1/...` 路径与云端服务一致
- 模型权重以文件形式存放在本机磁盘上
- 请求、加载、计算、返回这条完整路径，都可以在自己的机器上观察
""").image_right("assets/ollama-logo.png", width_px=100)
    p.demo("启动推理服务", "ollama serve", timeout=0)
    p.demo("下载模型权重", "ollama pull llama3.2", timeout=0)
    p.demo("提交一次请求", "ollama run llama3.2 'How do u like CSAPP?'", timeout=0)


def recap_weights(p):
    p.gap(40)
    p.title("回顾：llama3.2 模型文件")
    p.demo("查看模型文件", """ls -lhS ~/.ollama/models/blobs | sed -n '2p'
file "$(ls -dS ~/.ollama/models/blobs/* | head -1)\"""",
           output="""-rw-r--r-- 1 ollama ollama 1.9G Sep  1 19:50 sha256-...
...sha256-...: data""")
    p.slide("""
我们以一次 `ollama run` 为例，自底向上考察了系统各层
- 模型权重是一个 1.9 GB 的文件，`file` 给出的结果是 `data`，即未能识别其类型
- 文件中不含任何可执行指令，全部内容是数据
- 操作系统按需把其中的页装入内存，但没有解释过任何一个字节的含义
""", reveal="items")
    p.highlight("这 1.9 GB 中的每一个字节，按什么规则解释？", tone="orange")
    p.notes("""
第一讲的落点是「模型只提供参数，执行过程由系统栈完成」。本节把「参数」这个词展开：
参数以什么格式写在文件里，读出来之后在内存中是什么，为什么是 1.9 GB 而不是 12.8 GB。
本讲的命令在 x86-64 的 Linux、arm64 的 macOS 与 Windows 的 WSL2 上都可以执行，
结果一致；查看汇编的两页随指令集变化，逐条差别见 README 的「跨平台」一节。
""")


def four_questions(p):
    p.gap(40)
    p.title("本节回答四个问题")
    p.slide("""
1. **什么是字节**：
   位与字节 · 二进制与十六进制 · C 数据类型的宽度 · 布尔值的存储
""")
    p.slide("""
2. **多字节组成数据对象**：
   内存按字节编址 · 字长与地址范围 · 字节序
""")
    p.slide("""
3. **整型与浮点编码**：
   无符号数与有符号数 · 强制转换与比较陷阱 · 位运算与移位
   IEEE 754 的规格化、非规格化与特殊值 · 舍入 · 低精度浮点格式
""")
    p.slide("""
4. **如何对浮点数进行量化**：
   Q4_0 的块结构与 4 位打包 · 量化粒度 · Q4_1 的偏移 · Q4_K 的两级缩放
""")


# ==============================================================================
# 第一部分 · 位与字节
# ==============================================================================

def hexdump(p):
    p.title("读一个模型文件的前 32 个字节")
    p.demo("按二进制查看", """cd examples
xxd -b -l 16 -c 8 tiny.gguf""",
           output="""00000000: 01000111 01000111 01010101 01000110 00000011 00000000 00000000 00000000  GGUF....
00000008: 00000001 00000000 00000000 00000000 00000000 00000000 00000000 00000000  ........""",
           description="每个字节写成 8 位二进制，一行 8 个字节")
    p.demo("按十六进制查看", """cd examples
xxd -l 32 tiny.gguf""",
           output="""00000000: 4747 5546 0300 0000 0100 0000 0000 0000  GGUF............
00000010: 0200 0000 0000 0000 1400 0000 0000 0000  ................""",
           description="每个字节写成 2 位十六进制")
    p.slide("""
`tiny.gguf` 按 GGUF 格式写出，只是规模很小，仅用作示例。
- 前四个字节 `47 47 55 46` 按 ASCII 解释为 `GGUF`，用于标识文件类型
- 其后的每一段字节表示什么，由格式规范逐字段规定
- 同一段字节脱离规范则没有确定的含义
""")
    p.notes("""
tiny.gguf 由 examples/make_gguf.py 生成，共 224 字节，含一个 8×4 的 fp16 张量与两条元数据。
课堂上可以让学生把这条命令换成自己机器上的真实权重文件，前 16 个字节的结构完全一致。
""")


def bits_to_value(p):
    p.title("位向量与它表示的数值")
    p.slide("""
$w$ 位的**位向量** $[x_{w-1},\\ldots,x_1,x_0]$ 按位权求和，得到一个非负整数：
""", autobold=False)
    p.slide(r"""
 $$
 B2U(\vec{x}) = \sum_{i=0}^{w-1} x_i \cdot 2^i
 $$
""", autobold=False)
    p.table(
        headers=["二进制", "按位权展开", "十进制", "十六进制"],
        rows=[
            ["`0000 1010`", "$2^3 + 2^1$", "10", "`0x0a`"],
            ["`0110 0110`", "$2^6+2^5+2^2+2^1$", "102", "`0x66`"],
            ["`1111 1111`", "$2^8-1$", "255", "`0xff`"],
        ],
        align=["left", "left", "right", "left"],
    )
    p.aside("反向转换按除二取余进行：102 = 51×2+0，51 = 25×2+1，依此类推。")


def hexadecimal(p):
    p.title("十六进制等多种进制")
    p.slide("""
十六进制以 ==4 位二进制为一组==，用 `0`–`9` 与 `a`–`f` 共 16 个符号书写。
一个字节有 8 位，对应十六进制有 2 位，取值 `0x00` 到 `0xff`。
""", autobold=False)
    p.code("text", HEX_GROUPS)
    p.table(
        headers=["进制", "C 的写法", "表示 42"],
        rows=[
            ["十进制", "无前缀", "`42`"],
            ["十六进制", "`0x` / `0X`", "`0x2a`"],
            ["八进制", "前导 `0`", "`052`"],
            ["二进制", "`0b` / `0B`", "`0b101010`"],
        ],
        align=["left", "left", "left"],
    )
    p.notes("""
八进制的前导零是一类真实的缺陷来源：文件权限写 `0644` 是对的，把某个十进制常量
误写成 `010` 就会变成 8。C2y 增加了 `0o` 前缀，gcc 15 已作为扩展支持，C23 及以前无效。
C23 还允许用 `'` 分组数字，如 `0b1111'1111`。输出侧只有 `%x` 与 `%o`，没有二进制的转换说明符。
""")



def other_formats(p):
    p.title("其他模型格式的标识")
    p.demo("看五个文件的前 16 个字节", """cd examples
for f in tiny.*; do printf '%-17s ' $f; xxd -l 16 -g 2 $f | cut -c11- | tr -s ' '; done""",
           output="""tiny.gguf         4747 5546 0300 0000 0100 0000 0000 0000 GGUF............
tiny.onnx         0808 120a 6c65 6374 7572 656b 6974 3a58 ....lecturekit:X
tiny.pkl          8002 7d71 0058 0600 0000 7765 6967 6874 ..}q.X....weight
tiny.safetensors  3e00 0000 0000 0000 7b22 7765 6967 6874 >.......{"weight
tiny.zip          504b 0304 1400 0000 0000 0000 2158 77a0 PK..........!Xw.""",
           files=["examples/make_formats.py"])
    p.slide("""
- **魔数**：GGUF 的 `47 47 55 46`、zip 的 `50 4b 03 04`；`.pt` 自 1.6 起就是 zip 归档
- **自描述头部**：safetensors 开头 8 字节是头长 `3e`（62），随后即 JSON：`7b 22` 是 `{\"`
- **无标识**：ONNX 是 protobuf，`08` 是字段 1 的标签；旧 `.pt` 是 pickle 流，`80 02` 是协议操作码
""", reveal="items")
    p.highlight("文件类型来自约定：魔数、自描述头部或扩展名。", tone="blue")
    p.notes("""
`file` 命令的判断依据就是魔数表，因此它认得 zip 而认不出 GGUF 与 safetensors。
安全上的差别值得一提：pickle 在反序列化时执行任意代码，safetensors 只读取一段 JSON 与
一片连续数据，不含可执行内容，这是它被提出的直接原因。
五个样例由 examples/make_formats.py 生成，都按各自格式的规则写出，不是手工拼的字节。
""")


def c_data_sizes(p):
    p.title("C 数据类型的大小")
    p.table(
        headers=["C 类型", "32 位平台", "64 位平台", "说明"],
        rows=[
            ["`char`", "1", "1", "一个字节，也用于表示字符"],
            ["`int`", "4", "4", "各平台一致"],
            ["`long`", "4", "8", "随平台变化"],
            ["`void *`", "4", "8", "宽度即字长"],
            ["`float` / `double`", "4 / 8", "4 / 8", "IEEE 754 单精度与双精度"],
            ["`size_t`", "4", "8", "无符号，用于表示长度与大小"],
        ],
        align=["left", "center", "center", "left"],
    )
    p.demo("在 PC 上核对", """cd examples
gcc -O1 -o sizes sizes.c && ./sizes""",
           output="""char 1  short 2  int 4  long 8  void* 8
float 4  double 8  size_t 8  int32_t 4  int64_t 8""",
           files=["examples/sizes.c"])
    p.notes("""
需要固定宽度时使用 `<stdint.h>` 中的 int32_t / uint64_t，它们在各平台上宽度一致。
课堂上值得强调的是 long 与指针随平台变化：把指针存进 int 的代码在 64 位平台上会丢掉高位。
64 位的 Linux 与 macOS 采用 LP64，`long` 为 8 字节；原生 Windows 采用 LLP64，
`long` 为 4 字节而指针为 8 字节，此时表中「64 位」一列的 `long` 应读作 4。
WSL2 中的程序是 Linux 程序，与表中的 64 位一列一致。
""")


def bool_storage(p):
    p.gap(50)
    p.title("拓展：布尔值占用的空间")
    p.slide("""
C 语言没有内置的布尔类型，真假由整数的零与非零表示；C99 引入 `_Bool`，
`<stdbool.h>` 把 `bool` 定义为它的别名。C++ 中 `bool` 是一个内置类型。
- 两种语言中 `sizeof(bool)` 都是 ==1 个字节==：字节是最小的可寻址单位
- 一个布尔值只需要 1 位信息，其余 7 位不参与取值
- 赋值时任何非零值都被转换为 1，`bool b = 17;` 在内存中留下的字节是 `0x01`
""", reveal="items")
    p.demo("同一份代码分别按 C 与 C++ 编译", """cd examples
gcc -O1 -o bool_c bool_size.c && g++ -O1 -x c++ -o bool_cc bool_size.c
./bool_c && ./bool_cc""",
           output="""C    sizeof(bool) 1  value 1  byte 01
C++  sizeof(bool) 1  value 1  byte 01""",
           files=["examples/bool_size.c"])
    p.aside("如需节省空间，通过按位存放大量布尔值时，需要程序自己打包：C 用位域或整数掩码，C++ 可通过 `std::vector<bool>` 按位存储。")
    p.notes("""
C 里按位存放的写法是位域：`struct { unsigned a : 1, b : 1; }`，位的排列次序由实现决定，
因此位域不适合用来描述文件格式，跨编译器读同一段字节会得到不同结果。
""")


# ==============================================================================
# 第二部分 · 字节序
# ==============================================================================



def memory_as_bytes(p):
    p.title("内存模型：以字节为单位编址的数组")
    p.slide("""
程序看到的内存是一个==以字节为单位编址的数组==：每个地址对应一个字节。
- 地址从 0 开始连续编号，全体地址构成**地址空间**
- 一个多字节对象占用一段连续地址，其地址取这段地址中的**最小值**
""", reveal="items")
    p.image("assets/memory-bytes.svg", width_px=900)
    p.aside("字节是最小的可寻址单位，因此比字节更小的数据必须打包进一个字节。")


def word_size(p):
    p.title("字长：指针的宽度与地址空间的上限")
    p.slide(r"""
**字长**指一个指针占用的位数。$n$ 位的指针能表示 $2^n$ 个不同的地址，
每个地址对应一个字节，地址空间的大小因此就是 $2^n$ 字节。字长由硬件与操作系统共同确定，
是一个不能由程序选择的系统参数。
""", autobold=False)
    p.table(
        headers=["字长", "地址范围", "地址个数", "地址空间大小"],
        rows=[
            ["32 位", "`0x00000000` – `0xffffffff`",
             "$2^{32}$", "$4\\,294\\,967\\,296$ 字节 = 4 GB"],
            ["64 位", "`0x0000000000000000` – `0xffffffffffffffff`",
             "$2^{64}$", "约 $1.8 \\times 10^{19}$ 字节 = 16 EB"],
        ],
        align=["center", "left", "center", "left"],
    )
    p.image("assets/address-space.svg", width_px=700)
    p.notes("""
上一讲的 mmap 把整个权重文件映射进地址空间。这一步在 32 位系统上做不到，
32 位系统上可用的地址数量不足，与物理内存的容量无关——这是「地址」与「内存」的区别。
""")


def endianness(p):
    p.title("字节序：多字节对象的排列规则")
    p.slide("""
一个 4 字节的整数占用四个连续地址。哪个字节放在最低地址，由==字节序==规定。
- **小端**：最低有效字节位于最低地址。x86-64 与 ARM 的默认配置
- **大端**：最低有效字节位于最高地址。TCP/IP 首部与部分平台采用
- 采用哪一种由==处理器==确定：访存指令按哪个次序把内存中的字节组装成寄存器中的值，是硬件的设计选择，程序无法更改
""")
    p.image("assets/byte-order.svg", width_px=820,
            caption="同一个值在两种排列下占用同样的四个地址，字节的次序相反")
    p.notes("""
字节内部的位序不由程序观察，因此不涉及这一问题；讨论的对象始终是字节的次序。
x86-64 只有小端一种。ARM、RISC-V、PowerPC 是双端序处理器，两种次序都能执行，
实际使用哪一种在启动时由控制寄存器固定，Linux 在这些平台上通常运行于小端
文件格式与网络协议是另一类约定：它们自行规定字节序，与运行它们的处理器无关。
""")


def read_the_field(p):
    p.title("同一段字节的两种读法")
    p.code("c", READ_FIELD)
    p.demo("按两种字节序读同一个字段", """cd examples
gcc -O1 -o gguf_head gguf_head.c && ./gguf_head""",
           output="""magic          GGUF
version   LE   3
version   BE   50331648""",
           files=["examples/gguf_head.c"])
    p.highlight("字节序是一项约定：读写双方采用同一约定，字节才有确定的值。", tone="orange")


def show_bytes_page(p):
    p.title("观察任意对象的字节表示")
    p.code("c", SHOW_BYTES)
    p.demo("逐字节打印", """cd examples
gcc -O1 -o show_bytes show_bytes.c && ./show_bytes""",
           output="""int 12345      39 30 00 00
float 12345    00 e4 40 46
&ival          e8 49 1e 07 fc 7f 00 00""",
           files=["examples/show_bytes.c"])
    p.slide("""
- `12345` 的四个字节为 `39 30 00 00`，最低有效字节在最低地址，即小端
- 同一数值的 `float` 表示为 `00 e4 40 46`，与整数的位模式完全不同
- 指针本身也是一个 8 字节的对象，同样可以逐字节观察
""")
    p.notes("""
把地址转换为 `unsigned char *` 是绕过类型系统的标准做法：机器级程序本来就把对象
看作一段已知长度的字节，`show_bytes` 只是把这一视角显式写出来。
""")


def endianness_visible(p):
    p.gap(80)
    p.title("字节序在什么场合可见")
    p.slide("""
- **文件格式**：GGUF、ELF、PNG 各自规定字节序，跨机器读取才有确定结果
- **网络传输**：TCP/IP 首部采用大端，`htons` / `htonl` 完成本机序与网络序的转换
- **强制类型转换**：以另一种类型解释同一段字节时，排列方式随之暴露
- **反汇编输出**：指令编码中的立即数与地址按小端存放
""", reveal="items")
    p.aside("单字节序列不受影响：ASCII 字符串与 UTF-8 文本在任何平台上的字节次序都一致。")


def text_and_tokens(p):
    p.title("字符编码与 token 编号")
    p.slide("""
文本进入程序之前需要一次编码，这一步把符号映射为整数序列。
- **字符编码**：ASCII 用 7 位表示一个字符，UTF-8 用 1 到 4 个字节表示一个码点
- **分词器（tokenizer）**：一段文本被切成若干 token，每个 token 对应词表中的一个整数编号
- 两者的形式一致：先约定一张映射表，再以整数序列在系统中传递
""", reveal="items")
    p.demo("一段中文的 UTF-8 编码", "printf '你好' | xxd",
           output="00000000: e4bd a0e5 a5bd                           ......")
    p.highlight("模型接收的是 token 编号序列，字符在进入模型之前已经完成编码。", tone="blue")
    p.notes("""
两级编码在课堂上值得点明：字符 → 字节由 UTF-8 规定，文本 → token 由分词器的词表规定。
两者都是约定，都可以查表验证，都不涉及模型本身。
""")


# ==============================================================================
# 第三部分 · 整数
# ==============================================================================

def two_readings(p):
    p.title("两种整型数的表示")
    p.slide(r"""
无符号数各位的权重都为正；有符号数把最高位的权重由 $+2^{w-1}$ 改为 $-2^{w-1}$，其余各位的权重不变。
""", autobold=False)
    p.slide(r"""
 $$
 B2U(\vec{x}) = \sum_{i=0}^{w-1} x_i 2^i
 \qquad\qquad
 B2T(\vec{x}) = -x_{w-1} 2^{w-1} + \sum_{i=0}^{w-2} x_i 2^i
 $$
""", autobold=False)
    p.table(
        headers=["位模式（$w=4$）", "无符号 $B2U$", "有符号 $B2T$", "两者之差"],
        rows=[
            ["`0111`", "$4+2+1 = 7$", "$4+2+1 = 7$", "0"],
            ["`1000`", "$8 = 8$", "$-8 = -8$", "16"],
            ["`1011`", "$8+2+1 = 11$", "$-8+2+1 = -5$", "16"],
            ["`1111`", "$8+4+2+1 = 15$", "$-8+4+2+1 = -1$", "16"],
        ],
        align=["left", "left", "left", "center"],
    )
    p.aside("最高位为 0 时两种解释一致，为 1 时相差 $2^w$。")
    p.notes("""
数字电子技术里有符号数的定义是「正数不变，负数取反加一」，用于让减法由加法电路完成。
这里给出的按位权求和是同一套编码的另一种描述，在读一段字节时更好用：
不需要先判断符号再决定怎么算，直接按权重相加即可。
两者等价可以当场验证：`1011` 取反得 `0100`，加一得 `0101` 即 5，所以原码表示 −5。
""")


def numeric_range(p):
    p.title("无符号数与有符号数的取值范围")
    p.table(
        headers=["", "最小值", "最大值", "$w=32$", "$w=64$"],
        rows=[
            ["无符号", "0", "$2^w-1$", "4294967295", "约 $1.8\\times10^{19}$"],
            ["有符号", "$-2^{w-1}$", "$2^{w-1}-1$", "2147483647", "约 $9.2\\times10^{18}$"],
        ],
        align=["left", "center", "center", "right", "right"],
    )
    p.slide("""
两条需要记住的关系：
- $|TMin| = TMax + 1$，负数比正数多一个
- `-1` 有符号数的位模式为全 1，与无符号的最大值位模式相同
""")


def casting(p):
    p.title("类型转换：位模式不变，解释改变")
    p.code("c", CAST_SAMPLE)
    p.slide("""
C 允许把一个类型的变量按另一个类型解释，转换有两种发生方式。
- **显式转换**：写出 `(unsigned short) x`
- **隐式转换**：赋值、函数传参、以及表达式中的类型提升都会发生
- 有符号与无符号之间的转换==不改变任何一位==，改变的只是解释规则
""")
    p.image("assets/same-bits.svg", width_px=690)


def comparison_trap(p):
    p.title("混合比较：有符号操作数被转换为无符号")
    p.slide("""
一个表达式中同时出现有符号与无符号操作数时，==有符号一侧被转换为无符号==，比较随之改变。
""", autobold=False)
    p.demo("核对几个比较的结果", """cd examples
gcc -O1 -o compare compare.c && ./compare""",
           output="""-1 < 0                 1
-1 < 0u                0
2147483647 > -2147483647-1     1
2147483647u > -2147483647-1    0
length - 1, as unsigned        4294967295
strlonger("ab", "abcd")        1""",
           files=["examples/compare.c"])
    p.aside("规则写在 C 标准里，编译器默认不为此报警，如需要检查可在编译时加上 `-Wsign-compare` 参数。")


def kernel_bug(p):
    p.title("找错误：内核向用户程序拷贝数据")
    p.slide("""
内核代用户程序拷贝数据：`copy_from_kernel` 把 `kbuf` 拷到 `user_dest`，最多 `maxlen` 字节。
""", autobold=False)
    p.code("c", KERNEL_BUG)
    p.aside("问题：调用方能否让它拷贝出 `kbuf` 这 1024 字节以外的内容？")


def kernel_bug_answer(p):
    p.title("找错误：内核向用户程序拷贝数据")
    p.slide("""
内核代用户程序拷贝数据：`copy_from_kernel` 把 `kbuf` 拷到 `user_dest`，最多 `maxlen` 字节。
""", autobold=False)
    p.code("c", KERNEL_BUG)
    p.slide("""
调用方传入负的 `maxlen` 时：
- `KSIZE < maxlen` 是有符号比较，结果为假，`len` 取到那个负值
- `memcpy` 的第三个参数是 `size_t`，`len` 转换成极大的无符号数，越过缓冲区边界
""")
    p.cite(title="Computer Systems: A Programmer's Perspective", author="Bryant, O'Hallaron",
           year="2015", venue="3rd edition, §2.2.6", key="csapp")
    p.notes("""
这段代码源自 FreeBSD 的 getpeername 实现中出现过的缺陷。`maxlen` 由调用方给出，
返回值是实际拷贝的字节数。课堂上可以指出：这类错误在编译时没有任何提示，
运行时也不一定立刻崩溃，只有在特定输入下才暴露。
审查生成的代码时，有符号与无符号的混用是需要优先检查的一类。
""")


def mixed_width_comparison(p):
    p.title("混合比较：宽度不同的操作数进行比较")
    p.slide("""
比较分两步进行：
1. **确定目标类型**：先把窄于 `int` 的类型提升为 `int`；提升后宽度不同取较宽者，宽度相同、符号不同取无符号
2. **逐个转换**：==补 0 还是补符号位由原类型决定==，得到的位模式按目标类型解释
""")
    p.table(
        headers=["比较", "目标类型", "被转换的操作数", "结果"],
        rows=[
            ["int −1 < unsigned short 1", "int", "1 补 0 得 `0x00000001`，读为 1", "1"],
            ["short −1 < unsigned short 1", "int", "两侧都提升为 `int`：−1 补符号位，1 补 0", "1"],
            ["short −1 < unsigned 1", "unsigned", "−1 补符号位得 `0xffffffff`，读为 $2^{32}-1$", "0"],
            ["int −1 < unsigned long long 1", "unsigned long long", "−1 补符号位得 64 个 1，读为 $2^{64}-1$", "0"],
            ["long long −1 < unsigned 1", "long long", "1 补 0 得 64 位的 1，读为 1", "1"],
        ],
        align=["left", "left", "left", "center"],
    )
    p.notes("""
决定补 0 还是补符号位的是操作数原来的类型，决定比较按有符号还是无符号进行的是目标类型，两件事分开。
最后一行成立的前提是 `long long` 能表示 `unsigned` 的全部取值，在三种平台上都满足；
若较宽的有符号类型不能表示较窄无符号类型的全部取值，两侧转为该有符号类型对应的无符号类型。
""")


def bit_operations(p):
    p.title("位运算")
    p.table(
        headers=["运算", "C 写法", "作用", "作为集合运算"],
        rows=[
            ["与", "`x & y`", "两位都为 1 时得 1", "交集"],
            ["或", "`x | y`", "任一位为 1 时得 1", "并集"],
            ["取反", "`~x`", "逐位取反", "补集"],
            ["异或", "`x ^ y`", "两位不同时得 1", "对称差"],
        ],
        align=["center", "left", "left", "left"],
    )
    p.code("c", MASK_SAMPLE)
    p.aside("位运算与逻辑运算 `&&` `||` `!` 不同：后者把非零值视为真，结果只有 0 或 1，并且会短路求值。")


def shifts(p):
    p.title("移位运算")
    p.slide("""
- **左移** `x << k`：各位左移 $k$ 位，右侧补 0，高位移出后丢弃
- **逻辑右移** `x >> k`：左侧补 0，用于无符号数
- **算术右移** `x >> k`：左侧补符号位，用于有符号数，使负数右移后仍为负数
- 移位位数达到或超过字长时，C 标准未定义其行为
- 不溢出时，`x << k` 等于 $x \\times 2^k$，`x >> k` 等于 $\\lfloor x / 2^k \\rfloor$；移位比乘除法快，能用移位表示时通常使用移位
""", reveal="items")
    p.image("assets/shifts.svg", width_px=900)
    p.notes("""
右移对应向下取整，C 的整数除法向零取整，两者在负数上结果不同：`-7 >> 1` 为 −4，`-7 / 2` 为 −3。
编译器把 `x / 2^k` 换成移位时，会先给负数加上 $2^k - 1$ 的偏置，使结果与除法一致。
""")


def operator_precedence(p):
    p.title("位运算符的优先级")
    p.slide("""
位运算符 `&`、`^`、`|` 的优先级==低于==比较运算符，移位运算符 `<<`、`>>` 的优先级
低于加减。这两处与直觉相反，是一类常见的书写错误。
""", autobold=False)
    p.table(
        headers=["优先级", "运算符", "结合性", "容易写错的表达式"],
        rows=[
            ["高", "`~` `!` `-`（一元）", "右", "`~x + 1` 是 `(~x) + 1`"],
            ["", "`*` `/` `%`", "左", ""],
            ["", "`+` `-`", "左", "`x << 1 + 2` 是 `x << 3`"],
            ["", "`<<` `>>`", "左", ""],
            ["", "`<` `<=` `>` `>=`", "左", ""],
            ["", "`==` `!=`", "左", "`x & 1 == 0` 是 `x & (1 == 0)`"],
            ["", "`&`，其后 `^`，其后 `|`", "左", "`x & 3 | 4` 是 `(x & 3) | 4`"],
            ["低", "`&&`，其后 `||`", "左", ""],
        ],
        align=["center", "left", "center", "left"],
    )
    p.notes("""
表中最后三行的写法在实际代码里都出现过。
优先级的次序有历史原因：`&` 与 `|` 早于 `&&` 与 `||` 出现，当时它们同时承担
按位与逻辑两种用途，优先级按逻辑运算的位置设定；`&&` 与 `||` 加入之后，
为了不破坏既有代码，`&` 与 `|` 的位置保留至今。Ritchie 本人在回顾 C 的发展时
把这一点列为遗留的缺陷。记忆规则不如括号可靠。
""")


def precedence_in_practice(p):
    p.title("缺少括号可能造成的结果")
    p.demo("三个表达式的实际结果，以及编译器的提示", """cd examples
gcc -O1 -Wall -o precedence precedence.c 2>&1 | grep -o 'warning.*'
./precedence""",
           output="""warning: suggest parentheses around comparison in operand of ‘&’ [-Wparentheses]
warning: suggest parentheses around ‘+’ inside ‘<<’ [-Wparentheses]
warning: suggest parentheses around arithmetic in operand of ‘|’ [-Wparentheses]
x & 1 == 0      0      parses as x & (1 == 0)
(x & 1) == 0    1      what was meant
x << 1 + 2      48     parses as x << (1 + 2)
(x << 1) + 2    14     what was meant
x & 3 | 4       6      parses as (x & 3) | 4""",
           files=["examples/precedence.c"])
    p.highlight("`-Wall` 能报出这三处错误，但建议含位运算的表达式一律加括号。", tone="orange")
    p.notes("""
`x` 取 6。三处警告与三处错误一一对应，因此这类问题只要打开 `-Wall` 就不会漏掉；
但警告只覆盖它认得的组合，加括号是更可靠的做法。
""")


# ==============================================================================
# 第四部分 · 浮点数的编码
# ==============================================================================

FRAC_BITS = """\
unsigned result_bits = 0, current_bit = 0x80000000;
for (i = 0; i < 32; i++) {
    x *= 2;
    if (x >= 1) {
        result_bits |= current_bit;
        if (x == 1)
            break;          /* the fraction is exact */
        x -= 1;
    }
    current_bit >>= 1;
}"""


def fractional_binary(p):
    p.title("从十进制小数到二进制小数")
    p.slide(r"""
十进制小数 $d_m \cdots d_1 d_0 . d_{-1} \cdots d_{-n}$ 的第 $i$ 位权重为 $10^{i}$。
科学计数法把它写成 $d.dd\ldots \times 10^{k}$，例如 $12.375 = 1.2375 \times 10^{1}$。
二进制小数按同样的规则书写，第 $i$ 位权重为 $2^{i}$：$12.375 = 1100.011_2 = 1.100011_2 \times 2^{3}$。
""", autobold=False)
    p.table(
        headers=["$b_m$", "$b_{m-1}$", "$\\cdots$", "$b_2$", "$b_1$", "$b_0$", ".",
                 "$b_{-1}$", "$b_{-2}$", "$b_{-3}$", "$\\cdots$", "$b_{-n}$"],
        rows=[["$2^m$", "$2^{m-1}$", "$\\cdots$", "4", "2", "1", ".",
               "1/2", "1/4", "1/8", "$\\cdots$", "$2^{-n}$"]],
        align=["center"] * 12,
    )
    p.slide(r"""
- 二进制小数点右侧的位表示 2 的负次幂
- 表示的有理数为 $\sum_{i=-n}^{m} b_i \times 2^{i}$
""")
    p.notes(r"""
表头是各位，下一行是该位的权重。小数点左侧每向左一位权重乘 2，右侧每向右一位权重除以 2。
$12.375$ 的整数部分 $12 = 8 + 4$，小数部分 $0.375 = \frac{1}{4} + \frac{1}{8}$，合起来是 $1100.011_2$。
科学计数法中小数点移到最高位之后：十进制移动 1 位得到 $10^{1}$，二进制移动 3 位得到 $2^{3}$。
""")


def frac_examples(p):
    p.title("乘二取整法：转换到二进制小数")
    p.slide(r"""
小数部分按==乘二取整==转换：反复乘以 2，依次取出整数部分作为小数点后的各位。以 0.2 为例：
""", autobold=False)
    p.table(
        headers=["乘以 2", "取出的位", "剩余的小数部分"],
        rows=[
            ["$0.2 \\times 2 = 0.4$", "0", "0.4"],
            ["$0.4 \\times 2 = 0.8$", "0", "0.8"],
            ["$0.8 \\times 2 = 1.6$", "1", "0.6"],
            ["$0.6 \\times 2 = 1.2$", "1", "0.2"],
        ],
        align=["left", "center", "left"],
    )
    p.slide(r"""
- 剩余部分回到 0.2，此后循环：$0.2 = 0.00110011[0011]\ldots_2$
- $0.111\ldots1_2$ 形式的数略小于 1.0，记作 $1.0-\varepsilon$
- 二进制小数只能精确表示 $x/2^{k}$ 形式的数，其余的数的位模式无限循环
""")
    p.notes(r"""
$0.2 = 1/5$，分母含因子 5，方括号内的 0011 无限重复。
可以让学生当场算 $0.375$：$0.75$ 取 0，$1.5$ 取 1 剩 $0.5$，$1.0$ 取 1 剩 0 结束，得 $0.011_2$。
""")


def frac_to_bits(p):
    p.title("小数转换为二进制位")
    p.code("c", FRAC_BITS)
    p.demo("转换三个小数", """cd examples
gcc -O1 -o frac_bits frac_bits.c && ./frac_bits""",
           output="""0.75   0.11
0.625  0.101
0.2    0.00110011001100110011001100110011 ...""",
           files=["examples/frac_bits.c"])
    p.notes(r"""
这段循环就是上一页的乘二取整。
每次乘 2 后整数部分为 1，就在当前位写 1 并减去 1；`current_bit` 每轮右移一位，指向下一位。
乘 2 后恰好等于 1 时小数已经取尽，提前结束；0.2 在 32 轮内始终取不尽。
""")


def ieee_history(p):
    p.title("IEEE 754 标准的由来")
    p.slide("""
- 20 世纪 80 年代以前
  - 浮点格式各不相同：速度快，实现简单，精度较低
- IEEE 754
  - 由 W. Kahan 为 Intel 处理器设计，Kahan 于 1989 年获得图灵奖
  - 建立在一小组一致的原则之上：简洁，易于理解，难以高效实现
""").image_right("assets/ext/kahan.jpg", width_px=155
    ).footnote("William Kahan 是 IEEE 754 的主要设计者，1989 年图灵奖得主。照片来自 Wikimedia Commons，摄影 George Bergman，CC BY-SA 4.0。")
    p.notes("""
标准出现以前，不同厂商的机器上同一段数值程序可能得到不同的结果。
IEEE 754 规定了格式、舍入与特殊值，此后主流处理器都遵循它。
""")


def ieee_form(p):
    p.title("IEEE 浮点表示的数值形式与编码")
    p.image("assets/bit-fields.svg", width_px=820)
    p.slide(r"""
数值形式为 $V = (-1)^{s} \times M \times 2^{E}$，编码分为 $s$、$exp$、$frac$ 三个字段：
- 符号位 $s$ 决定数的正负
- $exp$ 字段编码阶码 $E$，$E$ 以 2 的幂对数值加权
- $frac$ 字段编码尾数 $M$，$M$ 通常是区间 $[1.0, 2.0)$ 内的小数
""")
    p.table(
        headers=["精度", "总位数", "exp 位数", "frac 位数"],
        rows=[
            ["单精度", "32", "8", "23"],
            ["双精度", "64", "11", "52"],
        ],
        align=["center", "center", "center", "center"],
    )
    p.notes(r"""
图中是 3.1415927 的 FP32 编码：符号位 0，阶码字段 128，尾数 $M = 1.5708$，$E = 1$。
这与二进制科学计数法 $1.bb\ldots \times 2^{E}$ 的写法一致：符号单独拿出来，$M$ 限定在 $[1.0, 2.0)$ 内，
每个非零数因此只有一种写法。三个字段从高位到低位依次排列，符号位是最高位。
C 语言中 `float` 对应单精度，`double` 对应双精度。两个字段的编码规则在后面几页讲。
""")


def normalized_exp(p):
    p.title("规格化值：阶码")
    p.slide(r"""
- 条件：$exp \neq 000\ldots0$ 且 $exp \neq 111\ldots1$
- 阶码按偏置值编码：$E = Exp - Bias$
  - $Exp$：$exp$ 字段表示的无符号数
  - $Bias$：偏置值，一般为 $2^{m-1}-1$，$m$ 为 $exp$ 的位数
""")
    p.table(
        headers=["精度", "$Bias$", "$Exp$ 的范围", "$E$ 的范围"],
        rows=[
            ["单精度", "127", "1 … 254", "−126 … 127"],
            ["双精度", "1023", "1 … 2046", "−1022 … 1023"],
        ],
        align=["center", "center", "center", "center"],
    )
    p.notes(r"""
$exp$ 全 0 与全 1 两种取值留给非规格化值和特殊值，规格化值只用中间的部分。
$Exp$ 是无符号数，减去偏置后 $E$ 覆盖正负两个方向，比较两个数的阶码时可以直接按无符号数比较。
""")


def normalized_frac(p):
    p.title("规格化值：尾数")
    p.slide(r"""
- 尾数带有隐含的前导 1：$M = 1.xxx\ldots x_2$
  - $xxx\ldots x$：$frac$ 字段的各位
  - $frac = 000\ldots0$ 时取最小值，$M = 1.0$
  - $frac = 111\ldots1$ 时取最大值，$M = 2.0 - \varepsilon$
  - 前导的 1 不占位，相当于多得到一位
""")
    p.notes(r"""
规格化的数最高位总是 1，存下来不携带信息，因此格式规定它隐含存在。
单精度的 23 位 $frac$ 因此表示的是 24 位有效数字中的后 23 位。
""")


def normalized_example(p):
    p.title("规格化编码的例子")
    p.demo("把 12345 编码为单精度浮点数", """cd examples
gcc -O1 -o normalized normalized.c && ./normalized""",
           output="""value      12345 (hex 0x3039)
binary     11000000111001
normalized 1.1000000111001 x 2^13
frac       10000001110010000000000
exp        10001100 (140 = 127 + 13)
encoding   0100 0110 0100 0000 1110 0100 0000 0000
hex        4640E400""",
           files=["examples/normalized.c"])
    p.slide(r"""
- 小数点左移 13 位，$E = 13$，$Exp = 13 + 127 = 140$
- 去掉前导 1，小数点后的 13 位补 0 到 23 位，得到 $frac$
- 依次拼接 $s = 0$、$exp$、$frac$，得到 `0x4640E400`
""")
    p.notes("""
程序用 `memcpy` 把 float 的 4 个字节拷进 unsigned，再按位取出三个字段，输出与手算的每一步对应。
""")


def denormalized(p):
    p.title("非规格化值")
    p.slide(r"""
- 条件：$exp = 000\ldots0$
- 阶码 $E = 1 - Bias$，尾数 $M = 0.xxx\ldots x_2$，$xxx\ldots x$ 为 $frac$ 字段的各位
- $frac = 000\ldots0$：表示 0，$+0$ 与 $-0$ 是两个不同的值
- $frac \neq 000\ldots0$：非常接近 0.0 的数
""")
    p.demo("解码三个阶码字段全 0 的单精度位模式", """cd examples
gcc -O1 -o denormalized denormalized.c && ./denormalized""",
           output="""bits       s  exp      frac                     M          E     as float
80000000   1  00000000 00000000000000000000000  0          -126  -0
00400000   0  00000000 10000000000000000000000  0.5        -126  5.87747e-39
00000001   0  00000000 00000000000000000000001  1.192e-07  -126  1.4013e-45""",
           files=["examples/denormalized.c"])
    p.notes(r"""
$E$ 取 $1 - Bias$ 而非 $0 - Bias$，使最大的非规格化数与最小的规格化数相邻，中间不出现间隙。
单精度中非规格化数的 $E = -126$，$M < 1$。
demo 的三行：符号位为 1、其余全 0 是 $-0$；$frac$ 最高位为 1 时 $M = 0.5$，值为 $2^{-127}$；
$frac$ 只有最低位为 1 时 $M = 2^{-23}$，值为 $2^{-149}$，是最小的正数。
""")


def special_values(p):
    p.title("特殊值")
    p.slide(r"""
- 条件：$exp = 111\ldots1$
- $frac = 000\ldots0$：表示无穷 $\infty$
  - 运算溢出时得到
  - 分为 $+\infty$ 与 $-\infty$
  - 例：$1.0/0.0 = -1.0/{-0.0} = +\infty$，$1.0/{-0.0} = -\infty$
- $frac \neq 000\ldots0$：非数（NaN，Not-a-Number）
  - 表示无法确定数值的运算结果
  - 例：$\sqrt{-1}$，$\infty - \infty$，$\infty \times 0$
- NaN 与任何值的 `<`、`<=`、`>`、`>=`、`==` 均为假，`!=` 为真，`x != x` 因此可用于判断 NaN
""")
    p.notes(r"""
单精度中 $+\infty$ 的位模式是 `0x7F800000`，$-\infty$ 是 `0xFF800000`。
NaN 的 $frac$ 可以取任意非零值，因此 NaN 对应的位模式有 $2 \times (2^{23} - 1)$ 个。
$1.0/{-0.0} = -\infty$ 说明 $+0$ 与 $-0$ 按浮点比较相等，参与除法时结果的符号不同。
""")


def float_encoding(p):
    p.title("浮点数编码小结")
    p.spacer(60)
    p.table(
        headers=["阶码字段", "名称", "$E$", "$M$", "覆盖的数"],
        rows=[
            ["非全 0 且非全 1", "规格化", "$e-127$", "$1.f$", "绝大多数数值"],
            ["全 0", "非规格化", "$-126$", "$0.f$", "零，以及接近零的极小数"],
            ["全 1，$f=0$", "无穷", "—", "—", "$+\\infty$ 与 $-\\infty$"],
            ["全 1，$f \\neq 0$", "NaN", "—", "—", "$0/0$ 一类无定义的结果"],
        ],
        align=["center", "center", "center", "center", "left"],
    )
    p.notes("""
全 0 与全 1 被保留，8 位阶码的实际指数范围是 −126 到 127。
非规格化数取消前导的 1，使零附近的可表示值保持均匀间隔，下溢因此逐步发生。
非规格化数的 $E$ 取 $-126$ 而非 $-127$，是为了让最大的非规格化数与最小的
规格化数首尾相接，中间不出现间隙。低精度格式沿用同一套规则：FP8 E4M3 的
可表示值里，非规格化数占了相当一部分。
""")


# ==============================================================================
# 第五部分 · 浮点数的精度与低精度格式
# ==============================================================================


def float_distribution(p):
    p.title("浮点数在实数轴上的分布")
    p.image("assets/float-spacing.svg", width_px=720)
    p.slide("""
- 阶码选定区间 $[2^E, 2^{E+1})$，尾数把它等分为 $2^n$ 份；每跨过一个 2 的幂，步长加倍
- 步长与数值成比例：绝对精度随数值增大而下降，==相对精度==保持不变
""", reveal="items")
    p.table(
        headers=["FP32 的区间", "步长 $2^{E-23}$", "步长 / 区间起点"],
        rows=[
            ["$[1, 2)$", "$2^{-23} \\approx 1.2 \\times 10^{-7}$", "$2^{-23}$"],
            ["$[2^{10}, 2^{11})$", "$2^{-13} \\approx 1.2 \\times 10^{-4}$", "$2^{-23}$"],
            ["$[2^{24}, 2^{25})$", "$2^{1} = 2$", "$2^{-23}$"],
        ],
        align=["center", "center", "center"],
    )
    p.notes(r"""
图中用的是尾数只有 3 位的简化格式，FP32 的每个区间被等分成 $2^{23}$ 份，道理相同。
表中每个区间的长度是 $2^E$，除以 $2^{23}$ 得到步长 $2^{E-23}$；步长除以区间起点 $2^E$，结果总是 $2^{-23}$。
绝对精度随数值增大而下降：$2^{24}$ 以上相邻两个 FP32 相差 2，奇数已经无法表示；
$10^9$ 附近的步长是 64，一个用 float 记录的计数器数到十亿之后，加一不再改变它的值。
零附近的非规格化数是唯一一段等距区域，它填补了最小规格化数与零之间的空隙。
""")


def float_rounding(p):
    p.title("舍入：取最近的可表示值")
    p.demo("三个整数与一个小数", """cd examples
gcc -O1 -o rounding rounding.c && ./rounding""",
           output="""16777216 as float   16777216.0
16777217 as float   16777216.0
16777219 as float   16777220.0
0.1 as double       0.10000000000000000555
0.1 as float        0.10000000149011611938""",
           files=["examples/rounding.c"])
    p.slide("""
- $2^{24}$ 以上相邻两个 FP32 相差 2，奇数正好落在两个可表示值的中点
- 中点处取尾数末位为偶的那个，称为==舍入到最近的偶数==，使误差不朝一个方向累积
- `0.1` 在二进制下是无限循环小数，有限位的格式只能存一个近似值
""")
    p.notes("""
舍入到最近的偶数是 IEEE 754 的默认模式，下一页列出全部四种模式。
两种规则都保证单次舍入的误差不超过半个步长，这是量化误差上界的依据；
量化那一节会说明编码区间两端不对称时这个上界如何失效。
""")


def rounding_modes(p):
    p.title("IEEE 754 的四种舍入模式")
    p.demo("五个值在四种模式下取整", """cd examples
gcc -O1 -o rounding_modes rounding_modes.c -lm && ./rounding_modes""",
           output="""mode               1.40   1.60   1.50   2.50  -1.50
downward              1      1      1      2     -2
upward                2      2      2      3     -1
toward zero           1      1      1      2     -1
to nearest even       1      2      2      2     -2""",
           files=["examples/rounding_modes.c"])
    p.slide("""
- **向下**取不大于真值的最近值，**向上**取不小于真值的最近值，**向零**丢弃小数部分
- **舍入到最近的偶数**是默认模式；只在中点处与四舍五入不同，1.50 与 2.50 都得到 2
- 其余三种模式的误差总是偏向同一侧，对大量数值求和时结果会系统性地偏大或偏小
""")
    p.notes("""
程序用 `fesetround` 切换模式，`nearbyint` 按当前模式取整。
舍入到最近的偶数在中点处一半向上、一半向下，因此求和时误差的期望为零。
十进制下的例子：保留两位小数时，1.2349999 得 1.23，1.2350001 得 1.24，
1.2350000 得 1.24（中点，向上），1.2450000 得 1.24（中点，向下）。
""")


def binary_rounding(p):
    p.title("二进制小数的舍入")
    p.slide("以保留到小数点后 2 位（步长 1/4）为例：", autobold=False)
    p.table(
        headers=["值", "二进制", "舍入结果", "动作", "舍入后的值"],
        rows=[
            ["$2\\frac{3}{32}$", "`10.00011`", "`10.00`", "向下", "$2$"],
            ["$2\\frac{3}{16}$", "`10.00110`", "`10.01`", "向上", "$2\\frac{1}{4}$"],
            ["$2\\frac{7}{8}$", "`10.11100`", "`11.00`", "向上", "$3$"],
            ["$2\\frac{5}{8}$", "`10.10100`", "`10.10`", "向下", "$2\\frac{1}{2}$"],
        ],
        align=["center", "center", "center", "center", "center"],
    )
    p.slide("""
- 舍入位置之后为 `100…0` 时是==中点==，取末位为 0 的一侧；大于中点向上，小于中点向下
- 第 3 行向上时产生进位，整数部分由 `10` 变为 `11`
""")
    p.notes("""
第 1 行舍去的是 `011`，小于中点 `100`；第 2 行舍去 `110`，大于中点；
第 3 行与第 4 行舍去的都恰好是 `100`：`10.11` 末位为 1，向上到 `11.00`；`10.10` 末位已是 0，保持不变。
浮点数的尾数按同样的规则舍入，后面 FP32 转换为 BF16 时用的就是这条规则。
""")


def patriot_missile(p):
    p.title("爱国者导弹：0.1 的舍入误差随时间累积")
    p.demo("计算 100 小时后的时钟误差", """cd examples
gcc -O1 -o patriot patriot.c && ./patriot""",
           output="""x (23 bits)       0.0999999046
0.1 - x           9.54e-08 s
ticks in 100 h    3600000
clock error       0.343 s
Scud at 2000 m/s  686.6 m""",
           files=["examples/patriot.c"])
    p.slide("""
- 1991 年 2 月 25 日，爱国者导弹未能拦截飞毛腿导弹，28 人死亡
- 时钟每 0.1 秒计数一次，程序用 24 位二进制小数近似 0.1
- 每次计数少约 $10^{-7}$ 秒，连续运行 100 小时后误差 0.343 秒
""").image_right("assets/ext/patriot-launch.jpg", width_px=150
    ).footnote("爱国者导弹发射。照片来自 Wikimedia Commons，美国陆军拍摄，公有领域。")
    p.notes("""
$0.1 = 0.0\\overline{0011}_2$，截断到小数点后 23 位得到 $x = 0.00011001100110011001100_2$，
$0.1 - x = 0.00000000000000000000000[1100]\\ldots_2 \\approx 2^{-20} \\times 0.1$。
100 小时共 $100 \\times 3600 \\times 10 = 3.6 \\times 10^6$ 次计数，乘以单次误差得到 0.343 秒。
飞毛腿导弹的速度约 2000 m/s，0.343 秒内飞行约 687 米，超出了雷达的跟踪窗口。
当时软件的部分代码已改用精确的时间计算，另一部分仍使用旧的近似值，两者相减后误差没有抵消。
""")


def float_not_real(p):
    p.title("有限位的表示不满足实数的运算律")
    p.demo("三个例子", """cd examples
gcc -O1 -o float_law float_law.c && ./float_law""",
           output="""(3.14+1e20)-1e20          0
3.14+(1e20-1e20)          3.14
0.1 + 0.2 == 0.3          0""",
           files=["examples/float_law.c"])
    p.slide("""
- 浮点加法==不满足结合律==：改变加法次序会改变结果
- 十进制的有限小数在二进制下可能是无限循环，`0.1` 与 `0.2` 都需要舍入
""")
    p.highlight("`float` 是有限位的近似，其运算不满足实数的运算律。", tone="orange")


def float_casts(p):
    p.title("C 中整数与浮点数之间的转换")
    p.demo("x = 2^24 + 1，f = 0.1f，d = 0.1", """cd examples
gcc -O1 -o float_casts float_casts.c && ./float_casts""",
           output="""x == (int)(float)x      0
x == (int)(double)x     1
f == (float)(double)f   1
d == (float)d           0
2/3 == 2/3.0            0
(int)2.7  (int)-2.7     2  -2
(int)3e9                -2147483648""",
           files=["examples/float_casts.c"])
    p.slide("""
- `int` 转 `double` 是精确的；转 `float` 时超过 $2^{24}$ 的值需要舍入
- 浮点转整数==向零截断==小数部分；超出 `int` 范围时 C 标准未定义结果，x86-64 上得到 $-2^{31}$
- `2/3` 是整数除法，结果为 0；`2/3.0` 是浮点除法
""")
    p.notes("""
`float` 转 `double` 不丢失信息，再转回 `float` 得到原值；`double` 转 `float` 丢掉尾数的后 29 位，0.1 的近似值因此改变。
x86-64 上越界的转换由 `cvttsd2si` 指令完成，结果固定为 `0x80000000`，称为 integer indefinite；其他平台可能饱和到最大或最小值。
量化时把浮点数转为整数必须显式舍入：`(int)` 向零截断，正负两侧的误差都偏向 0；
本讲的 `quant_compare.c` 用 `lrintf`，它按当前舍入模式（默认舍入到最近的偶数）取整。
""")


def precision_formats(p):
    p.title("低精度浮点格式")
    p.image("assets/float-formats.svg", width_px=640)
    p.slide("""
每种格式的差别只在三段位如何分配。
- **BF16** 与 FP32 的阶码同为 8 位，范围相同，尾数少 16 位
- **FP16** 阶码 5 位，最大值 65504，数值较大时溢出
- **FP8** 有 E4M3 与 E5M2 两种分法，前者精度优先，后者范围优先
""")
    p.slide("""
低精度舍弃尾数末尾的若干位，换来存储与访存量按位数同比例下降，以及更高的乘加吞吐。
前提是模型的输出质量对尾数末位的变化不敏感。
""", autobold=False)
    p.cite(title="FP8 Formats for Deep Learning", author="Micikevicius et al.",
           year="2022", venue="arXiv:2209.05433",
           url="https://arxiv.org/abs/2209.05433", key="fp8")
    p.notes("""
FP64 的分配为 1 · 11 · 52，用于科学计算，深度学习中很少出现，图中未列。
三项收益可以分开说：权重文件与显存占用按位数同比例下降；每个 token 的访存量同比例下降；
加速器上低精度乘加单元的吞吐高于高精度单元。
""")


def bf16_truncation(p):
    p.title("FP32 转换为 BF16：保留前 16 位再舍入")
    p.code("c", BF16_ROUND)
    p.demo("三个值的转换", """cd examples
gcc -O1 -o bf16_round bf16_round.c && ./bf16_round""",
           output="""x            top 16 bits          rounded              vs __bf16
3.14159274   0 10000000 1001001   0 10000000 1001001   same
0.333333343  0 01111101 0101010   0 01111101 0101011   same
1.99999988   0 01111111 1111111   0 10000000 0000000   same""",
           files=["examples/bf16_round.c", "examples/bf16.c"])
    p.slide("""
- 转换保留符号位、阶码与尾数的前 7 位，再按被舍去的 16 位舍入
- 尾数全 1 时加一会==进位到阶码==：1.99999988 转换后阶码由 127 变为 128，值为 2
""")
    p.notes("""
第一行被舍去的最高位是 0，舍入与截断的结果相同；第二行被舍去的部分大于一半，尾数加一；
第三行尾数本来全 1，加一后尾数清零、阶码加一。三行都与编译器对 `__bf16` 的转换一致。
判断条件就是前面二进制舍入那一页的规则：`rest` 大于 `0x8000` 时超过一半，等于 `0x8000` 时是中点，取末位为偶。
`0x7f7f` 再进位得到 `0x7f80`，即无穷，与舍入到最近值时溢出的规定一致；NaN 的情形这段代码没有处理。
`examples/bf16.c` 只做截断，打印 π 被舍去的部分，可以对照阅读。
BF16 在训练中的作用：与 FP32 范围一致，因此不需要 loss scaling 一类的额外处理；
代价是相对精度下降到约三位十进制有效数字。
""")


def bf16_classes(p):
    p.title("BF16 的规格化数与非规格化数")
    p.demo("对若干个位模式按 BF16 解释", """cd examples
gcc -O1 -o bf16_classes bf16_classes.c && ./bf16_classes""",
           output="""0x0000   0 00000000 0000000   0
0x0001   0 00000000 0000001   9.18354962e-41
0x007f   0 00000000 1111111   1.1663108e-38
0x0080   0 00000001 0000000   1.17549435e-38
0x3f80   0 01111111 0000000   1
0x7f7f   0 11111110 1111111   3.38953139e+38
0x7f80   0 11111111 0000000   inf
0x7fc0   0 11111111 1000000   nan""",
           files=["examples/bf16_classes.c"])
    p.slide("""
- 1 位符号、8 位阶码、7 位尾数，偏置 127；规格化数为 $(-1)^s \\times 1.f \\times 2^{e-127}$
- 阶码全 0 为==非规格化数==，值为 $(-1)^s \\times 0.f \\times 2^{-126}$，与规格化数首尾相接
- 阶码全 1 为特殊值：尾数为 0 表示无穷，尾数非 0 表示 NaN
""")
    p.notes("""
BF16 的规则与 FP32 完全相同，只是尾数从 23 位减到 7 位，阶码的 8 位保持不变。
阶码全 0 时 $E$ 取 $1-127=-126$ 而非 $-127$：最大的非规格化数 `0x007f` 为 $127 \\times 2^{-133}$，
最小的规格化数 `0x0080` 为 $128 \\times 2^{-133} = 2^{-126}$，两者相差一个步长 $2^{-133}$。
`0x0080` 的值与 FP32 最小的规格化数相同，`0x7f7f` 也与 FP32 的最大值在同一数量级，这是阶码相同的结果。
程序把位模式复制进 GCC 的 `__bf16` 变量再按浮点数输出，解码由编译器完成。
""")


def range_and_precision(p):
    p.title("范围与精度相互独立")
    p.demo("同一个值在三种格式下", """cd examples
gcc -O1 -o fp16_range fp16_range.c && ./fp16_range""",
           output="""65504     as fp16   65504
100000    as fp16   inf
100000    as bf16   99840
1.0/3     as fp32   0.33333334
1.0/3     as fp16   0.33325195
1.0/3     as bf16   0.33398438""",
           files=["examples/fp16_range.c"])
    p.slide("""
- 100000 超出 FP16 的范围，结果为 `inf`；BF16 的阶码更宽，仍能表示
- $1/3$ 在 FP16 下比在 BF16 下更接近真值，因为 FP16 的尾数多 3 位
- 训练中梯度的动态范围很大，因此偏向 BF16；推理中数值范围较窄，FP16 可用
""")
    p.highlight("阶码位数决定范围，尾数位数决定相对精度。", tone="blue")
    p.notes("""
总位数固定时，阶码与尾数各占多少是格式设计上的取舍：FP16 与 BF16 都是 16 位，
BF16 把 3 位从尾数挪给阶码，因此范围与 FP32 相同而精度更低。
上面 100000 与 $1/3$ 这两行分别对应这两端。
""")


def model_size(p):
    p.title("拓展：精度决定模型的体积与逐 token 的访存量")
    p.table(
        headers=["格式", "每权重位数", "3.21 B 参数的体积", "用途"],
        rows=[
            ["FP32", "32", "12.85 GB", "训练的基准精度"],
            ["BF16 / FP16", "16", "6.43 GB", "训练与高精度推理"],
            ["FP8", "8", "3.21 GB", "较新加速器上的推理"],
            ["4 位量化（Q4_0）", "4.5", "1.81 GB", "本地推理的常见选择"],
        ],
        align=["left", "center", "right", "left"],
    )
    p.slide("""
- 4 位量化的每权重位数是 ==4.5== 而非 4：4 位是编码本身，另外的 0.5 位是每 32 个权重
  共用的 FP16 缩放系数（2 字节 ÷ 32 个权重 = 0.5 位）摊到每个权重上的份额
- 每生成一个 token 需完整读一遍参数，格式的位数同比例决定了==访存量==
""")
    p.notes("""
表中按十进制 GB 计算：3.21 B × 4.5 位 ÷ 8 = 1.81 GB。这一行说的是单一 4 位格式。
实际下载到的那个文件是几种格式的混合（llama.cpp 称为 Q4_K_M）：多数张量为 Q4_K（4.5 位），
词嵌入表与部分层的 attn_v、ffn_down 提到 Q6_K（6.56 位），归一化系数保持 F32，
加权平均 5.01 位，合 2.02 GB。第一讲 `ls -lh` 显示的 1.9G 是 GiB，2.02 GB 即 1.88 GiB。
有学生追问对得上对不上时可以给这组数；`examples/gguf_bits.py` 能把整个文件统计一遍。
""")


# ==============================================================================
# 第六部分 · 量化：原理与 Q4_0
# ==============================================================================

def why_quantize(p):
    p.title("量化的动机：逐 token 的访存量")
    p.slide("""
自回归生成每产生一个 token，都要把全部权重读进运算单元一次。权重用多少位存放，直接决定这一次读多少字节。
""", autobold=False)
    p.table(
        headers=["格式", "1.24 B 参数的体积", "89.6 GB/s 下的上限", "1008 GB/s 下的上限"],
        rows=[
            ["BF16（16 位）", "2.47 GB", "36 token/s", "408 token/s"],
            ["Q8_0（8.5 位）", "1.31 GB", "68 token/s", "768 token/s"],
            ["Q4_K（4.5 位）", "0.70 GB", "129 token/s", "1450 token/s"],
        ],
        align=["left", "right", "right", "right"],
    )
    p.slide("""
- 89.6 GB/s 是双通道 DDR5-5600 的峰值带宽，1008 GB/s 是 RTX 4090 显存的峰值带宽
- 一次只处理一个请求时，每个权重只参与一次乘加，读一个字节只做一次左右的运算
""")
    p.highlight("单请求推理的瓶颈在访存，权重的位数几乎线性地决定生成速度。", tone="orange")
    p.notes("""
表里的数是带宽除以体积，是上限而非实测值：真实系统还有 KV 缓存的读写、算子启动、
以及带宽利用率达不到峰值。参数量 1 235 814 400 取自 Llama-3.2-1B-Instruct 的
safetensors 头部，本节后面的实验用的就是这个模型。
DDR5-5600 双通道：5600 MT/s × 8 字节 × 2 = 89.6 GB/s。
批量推理是另一种情形：一次前向服务多个请求时，同一份权重被多次使用，
瓶颈会从访存转向算力，量化带来的收益随之下降。
""")


def memory_bound_measured(p):
    p.title("访存瓶颈的实测：编码器与解码器的对照")
    p.image("assets/ext/memory-wall-profile.png", width_px=940
            ).footnote("图片来自 Gholami et al., AI and Memory Wall（IEEE Micro 2024）图 3(b)(d)，"
                       "CC BY 4.0；裁去两幅子图的标题后横向拼合。")
    p.slide("""
- 左图为一次前向的访存量，右图为同一次前向的时延，均在单请求下测得
- 128 到 4096，GPT-2 的访存量增至 45.7 倍，时延增至 52.1 倍
- 同为 4096，BERT-Base 与 GPT-2 的计算量相近，时延却是 84 与 2344
""", reveal="items")
    p.highlight("计算量相近而时延相差 28 倍，时延跟随的是访存量。", tone="orange")
    p.cite(title="AI and Memory Wall", author="Gholami et al.", year="2024",
           venue="IEEE Micro", url="https://arxiv.org/abs/2403.14123", key="memory-wall")
    p.notes("""
原图共四幅，本页取其中两幅：图 3(b) 是一次前向的访存量（GMOPs），图 3(d) 是同一次前向的
端到端时延，以 BERT-Base 在序列长 128 时为 1 归一化，均在 batch size 1 下测得。
同为序列长 4096，BERT-Base 的计算量是 1324 GFLOPs，GPT-2 是 1012 GFLOPs。
未取的图 3(c) 给出算术强度：GPT-2 在各序列长度上都是 2 FLOP/字节，BERT 是 117 到 266。
算术强度即每读一个字节所做的浮点运算次数，它决定了一次计算受限于带宽还是受限于算力。
原文的结论是 decoder 模型在小批量下的瓶颈是访存而非计算。
""")


def what_to_quantize(p):
    p.title("量化的对象：权重、激活值与 KV 缓存")
    p.slide("""
==激活值==是前向计算中每一层算出的中间结果，随每次输入而变；==KV 缓存==是注意力为已生成的每个 token 保存的一份键与值，供后续 token 反复读取，随上下文增长。
""", autobold=False)
    p.table(
        headers=["对象", "何时确定", "量化的时机", "常见位宽"],
        rows=[
            ["权重", "训练结束后不再变化", "离线做一次，存进文件", "4 – 8 位"],
            ["激活值", "随每次输入变化", "运行时逐张量做", "8 – 16 位"],
            ["KV 缓存", "随上下文增长", "运行时逐 token 做", "8 位居多"],
        ],
        align=["left", "left", "left", "center"],
    )
    p.slide("""
- 权重是静态的，量化一次就固定下来，出错也只影响这一次转换，因此最先被压到 4 位
- 激活值的取值范围随输入变化，量化的系数要在运行时求，且误差会沿网络逐层累积
- 记号 `W4A16` 表示权重 4 位、激活值 16 位，是本地推理最常见的组合
""", reveal="items")
    p.highlight("本节只讨论权重的量化，文件里存的也只有权重。", tone="blue")
    p.notes("""
KV 缓存的量化在长上下文下收益明显：缓存的体积随上下文长度线性增长，
上下文足够长时会超过权重本身。llama.cpp 的 `--cache-type-k q8_0` 就是做这件事。
激活值量化的难点在离群值，最后一部分的那张图讲的正是这个问题。
""")


def quantization_map(p):
    p.title("量化是一个仿射映射")
    p.slide(r"""
一组权重共用两个浮点参数：步长 $d$ 与偏移 $m$。编码 $q$ 是一个 4 位无符号整数，还原时按 $\hat{w} = d \cdot q + m$ 计算。
""", autobold=False)
    p.slide("""
- **区间**：多少个权重共用同一对 $d$ 与 $m$，即量化的粒度
- **级数**：编码占几位，4 位给出 16 级
- **偏移**：$m$ 固定为 0 时称为==对称量化==，$m$ 可取该组的最小值时称为==仿射量化==
""", reveal="items")
    p.slide("""
在这三项都定下来之后，量化本身只是一次除法与一次舍入，误差不超过半个步长。
""", autobold=False)
    p.aside("""
对称量化并不需要另一套式子：$m$ 取 $-8d$ 时，编码 0 到 15 就表示 $-8$ 到 $7$ 个步长，
差别只在 $m$ 是否单独存储、能否独立取值。
""")
    p.highlight("量化的全部选择都在这三项上，格式之间的差别也在这三项上。", tone="blue")
    p.notes("""
Q4_0 是对称的：$m = 0$，编码 0 到 15 表示 −8 到 7 乘以步长。
写成 $\\hat{w} = d(q - 8)$ 与写成 $\\hat{w} = dq + m$（$m = -8d$）是同一件事，
区别在于 $m$ 是否单独存储、能否独立取值。
「误差不超过半个步长」的例外是饱和的那一端，前一页的备注里讲过。
""")


def granularity(p):
    p.title("粒度：多少个权重共用一个缩放系数")
    p.image("assets/granularity.svg", width_px=1000)
    p.slide("""
- 系数本身要占空间：每权重多出的位数 = 系数的位数 ÷ 组的大小，fp16 分到 32 个权重上是 0.5 位
- 组越小，组内的取值范围越窄，步长越小；代价是系数的份额越大
""")
    p.highlight("粒度是精度与体积之间的一个可调参数。", tone="blue")
    p.notes("""
图中三种粒度的位数开销分别是 0、16/256 = 0.0625、16/32 = 0.5 位。
per-tensor 一档在这张图上画成虚线，是因为它的系数由整个张量共用，
不属于图中这 256 个权重。
""")


def granularity_measured(p):
    p.title("三种粒度在真实权重上的误差")
    p.demo("同一组权重，三种粒度", """cd examples
gcc -O1 -o quant_compare quant_compare.c -lm && ./quant_compare down""",
           output="""ext/w-down-proj.bf16   4096 weights   range -0.0569 .. +0.0635   rms 0.01707
  scheme        bytes  bits/w   rmse       rel.rmse  max err
  per-tensor     2050   4.00   0.002306    13.50%   0.003967
  per-256        2080   4.06   0.001904    11.15%   0.005310
  Q4_0           2304   4.50   0.001438     8.42%   0.004730
  Q4_1           2560   5.00   0.001312     7.69%   0.003284
  Q4_K           2304   4.50   0.001314     7.70%   0.003308""",
           files=["examples/quant_compare.c"])
    p.slide("""
权重取自 Llama-3.2-1B 第 0 层的 `ffn_down`，共 4096 个。`rel.rmse` 是误差的均方根与权重本身均方根之比。
""", autobold=False)
    p.highlight("多花 0.5 位把粒度收到 32，相对误差从 13.5% 降到 8.4%。", tone="orange")
    p.notes("""
这组数据近似关于 0 对称，因此 Q4_0 与 Q4_1 的差别不大，粒度是主要因素。
下一节的另一组数据不是这样。表里 Q4_1 与 Q4_K 两行现在还没有讲到，
课上可以先只看前三行；`quant_compare.c` 的源码在按钮里，五种做法都在同一个文件中。
样本文件与它的授权说明见 `examples/ext/`。
""")


def quantization_idea(p):
    p.title("量化：用整数编码近似一组浮点值")
    p.image("assets/quantize-line.svg", width_px=900)
    p.slide("""
把一组权重的取值范围划分为等距的若干级，每个权重记录==离它最近的那一级的编号==。
- 4 位编码可表示 16 级，一组共用一个缩放系数 $d$
- 还原时按 $w \\approx (q - 8) \\cdot d$ 计算，$q$ 为编码
- 除去饱和的一端，单个权重的误差不超过半个步长，步长与该组的取值范围成正比
""")
    p.notes("""
「除去饱和的一端」是一句必要的限定。Q4_0 的 16 个编码表示 −8 到 7，两端不对称：
缩放系数取 $d = \\max / (-8)$，组内的极值恰好落在编码 0 上；与它反号且幅度相近的权重
算出的编码超过 15，被截到 15，还原为 $7d$ 而非 $8d$，误差可达一个完整步长。
这与整数一节的 $|TMin| = TMax + 1$ 是同一件事。
""")


def q4_block(p):
    p.title("Q4_0 的块结构：18 字节存放 32 个权重")
    p.image("assets/q4-block.svg", width_px=880)
    p.slide("""
- 每 32 个权重为一组，共用一个 FP16 的缩放系数，占 2 字节
- 32 个 4 位编码打包进 16 字节，每字节存放两个权重
- 16 个编码表示 $-8$ 到 $7$，与有符号数一样两端不对称
- 合计 18 字节，平均每个权重 ==4.5 位==；3.21 B 参数据此约为 1.8 GB
""")


def quantize_code(p):
    p.title("量化的实现")
    p.code("c", QUANT_CORE)
    p.demo("量化一组权重并还原", """cd examples
gcc -O1 -o quantize quantize.c -lm && ./quantize""",
           output="""first four weights   +0.2742 +0.3699 +0.3010 +0.1094
after quantization   +0.2272 +0.3408 +0.3408 +0.1136
qs[0] = 0x76         low -> w[0] code 6, high -> w[16] code 7
bytes                128 -> 18   (4.50 bits per weight)
largest error        0.0564  (6.2% of the largest weight)""",
           files=["examples/quantize.c"])
    p.notes("""
两处容易写错。一是缩放系数取块内绝对值最大的那个权重、连同它的符号除以 −8，
使该权重编码为 0，16 个码字全部可达；若改用 absmax/7，编码只落在 1..15。
二是配对方式：低半字节存第 j 个权重，高半字节存第 j+16 个，配对跨越块的前后两半。
llama.cpp 用浮点的 d 量化、按 FP16 存回 d，还原时两者略有出入，实际误差因此
可以略微超过半个步长。
""")


def nibble_packing(p):
    p.title("4 位数据的存取：掩码与移位")
    p.slide("""
字节是最小的可寻址单位，4 位的数据没有独立地址，必须==两个一组打包进一个字节==。
""", autobold=False)
    p.code("c", NIBBLE_ACCESS)
    p.slide("""
- 取低 4 位用掩码 `& 0x0f`，取高 4 位用移位 `>> 4`
- 写回时先把高 4 位左移到位，再与低 4 位按位或
- Q4_0 配对的是第 $j$ 与第 $j+16$ 个权重，一个字节里的两半来自块的前后两段
""")


def nibble_add(p):
    p.title("打包之后的运算：每个字段需要独立的溢出")
    p.slide("""
一个字节里的两个 4 位字段各自是一个数。直接相加时，低位字段的进位会越过第 3 位加到高位字段上。
""", autobold=False)
    p.code("c", NIBBLE_ADD)
    p.slide("""
- `& 0x77` 清掉每个字段的最高位，低三位相加最多进到第 3 位，进位停在字段内
- `(a ^ b) & 0x88` 把清掉的那两位用异或补回去，异或不产生进位
- 高位字段的第 3 位若溢出，同样被丢弃，不会越过字节边界
""")
    p.highlight("低位字段的进位必须停在字段边界内，不能计入高位字段。", tone="orange")
    p.notes("""
这一页是第三部分位运算内容的落点。以 `0x29 + 0x38` 为例（字段 9+8 与 2+3）：
直接相加得 `0x61`，低字段 9+8=17 溢出并把 1 带进高字段；`add_packed` 得 `0x51`，
低字段回绕成 1，高字段仍是 5。`examples/int4.c` 对全部 65536 组输入验证过这两种写法，
直接相加错 30720 组，`add_packed` 错 0 组。
""")


def int4_hardware(p):
    p.title("拓展：4 位运算在硬件上的支持")
    p.slide("""
矩阵乘有专门的运算单元：每个 4 位数单独取出来参与乘法，乘积逐个加进同一个 32 位寄存器，这个寄存器称为==累加器==（accumulator）。
""", autobold=False)
    p.image("assets/int4-accumulate.svg", width_px=680)
    p.slide("""
- 两个 4 位数不再共用一个字节，各自单独送进乘法器，进位无处可越
- 4 位数相乘最多得 64，32 个乘积相加不超过 2048，32 位的累加器装得下
- Turing（2018）起的 Tensor Core 提供这种单元；元素级的 4 位加减仍要用上一页的写法
""", reveal="items")
    p.highlight("4 位是权重的存放格式，运算在 32 位的累加器里完成。", tone="blue")
    p.cite(title="NVIDIA Turing GPU Architecture Whitepaper", author="NVIDIA",
           year="2018", venue="WP-09183-001",
           url="https://images.nvidia.com/aem-dam/en-zz/Solutions/design-visualization/technologies/turing-architecture/NVIDIA-Turing-Architecture-Whitepaper.pdf",
           key="turing")
    p.notes("""
累加器就是运算单元里存放累加中间结果的那个寄存器，矩阵乘要把一行与一列的 32 对乘积依次加起来，
加到哪里，哪里就是累加器。它比输入宽得多，是硬件避开溢出的常规做法。
指令写作 `mma.sync…s32.s4.s4.s32`：两个 `s4` 是 4 位输入，`s32` 是 32 位累加器；
A100 的 INT4 峰值为 1248 TOPS，是同一颗芯片 FP16 的 4 倍。
上一页的困难只出现在结果也要存回 4 位这种情形：两个数挤在一个字节里，低位的进位无处可去。
INT4 的 Tensor Core 出现在 2018 年的 Turing 上，比 4 位量化在大模型推理中被真正用起来
早了约四年；等到用法成熟，NVIDIA 已在 Hopper 上把 INT4 标为弃用。
Blackwell 的第五代 Tensor Core 改用 FP4，4 位中留出阶码，
NVFP4 再配 16 个元素一组的细粒度缩放，方向与本节讲的分组缩放一致。
CUDA 的 `__vadd4` 是元素级 SIMD 加法的下限：它把 32 位当作四个字节分别相加，没有更窄的通道。
""")


# ==============================================================================
# 第七部分 · 量化格式：Q4_1 与 Q4_K
# ==============================================================================

def zero_point(p):
    p.title("Q4_1：级不必关于 0 对称")
    p.image("assets/zero-point.svg", width_px=1000)
    p.slide("""
- 对称量化的步长由组内绝对值最大的权重决定，一组权重离 0 越远，步长越大
- Q4_1 另存一个 fp16 的最小值，步长改由这组数据自身的宽度决定，16 个级全部落在数据上
- 每块 20 字节，平均 ==5.0 位==
""")
    p.highlight("对称量化把 16 个级摊在 [−max, max] 上，其中一半可能没有数据。", tone="orange")
    p.notes("""
图中这一块的 32 个权重取值在 1.539 与 2.625 之间。Q4_0 的步长是 2.625/8 = 0.328，
16 个级里只有 4 个落在这个区间内；Q4_1 的步长是 (2.625 − 1.539)/15 = 0.0724，
16 个级全部落在区间内。两者相差 4.5 倍。
这也是 GGUF 把归一化系数保持为 F32 的原因之一：这类张量的值集中在 1 附近，
对称量化在它们上面浪费得最厉害，而它们的参数量又很小，不值得为省几 KB 引入误差。
""")


def q4_1_measured(p):
    p.title("Q4_1：偏移在全正权重上的效果")
    p.demo("一组全为正的权重", """cd examples
gcc -O1 -o quant_compare quant_compare.c -lm && ./quant_compare norm""",
           output="""ext/w-final-norm.bf16   2048 weights   range +0.0417 .. +2.9219   rms 2.36650
  scheme        bytes  bits/w   rmse       rel.rmse  max err
  per-tensor     1026   4.01   0.094299     3.98%   0.181641
  per-256        1040   4.06   0.100978     4.27%   0.181641
  Q4_0           1152   4.50   0.100460     4.25%   0.175781
  Q4_1           1280   5.00   0.022288     0.94%   0.084106
  Q4_K           1152   4.50   0.051072     2.16%   0.094694""",
           files=["examples/quant_compare.c"])
    p.slide("""
这 2048 个数全为正，均值 2.35。多花 0.5 位记住每块的最小值，相对误差从 4.25% 降到 0.94%。
""", autobold=False)
    p.highlight("这组数据上，缩小粒度几乎没有用，把偏移放开才有用。", tone="orange")
    p.notes("""
per-tensor 反而略好于 per-256 与 Q4_0，是因为对称量化在这里的浪费是结构性的：
步长由「离 0 多远」决定，分组分得再细也改不了这一点，而整个张量共用一个系数时，
最大值恰好比各块的最大值更大一点，步长反而没有变差多少。
误差比 4.25/0.94 = 4.5 与上一页算出的步长比 0.328/0.0724 = 4.5 一致。
""")

    
def superblock(p):
    p.title("Q4_K：两级缩放与超块")
    p.image("assets/q4-k-block.svg", width_px=1000)
    p.slide(r"""
- 8 个子块各有自己的步长与偏移，共 16 个系数，因此保持了 Q4_1 的贴合程度
- 这 16 个系数各量化成 6 位，共占 12 字节
- 还原一个权重按 $\hat{w} = d \cdot sc \cdot q - d_{min} \cdot m$ 计算，$sc$ 与 $m$ 是该子块的两个 6 位系数
""")
    p.highlight("算一算：Q4_K 布局下平均每权重占用多少位，与 Q4_1 相差多少？", tone="orange")
    p.notes("""
两级的意思是：超块存两个 fp16，分别作为 8 个子块步长的公共缩放与 8 个子块偏移的公共缩放；
子块的步长写成「6 位整数 × 超块步长」，偏移同理。还原一个权重要先还原它所在子块的系数。
llama.cpp 里这个结构叫 K-quants，Q2_K 到 Q6_K 都是同一套超块布局，
区别只在编码的位数与子块的划分。
""")


def q4_k_budget(p):
    p.title("Q4_K：两级缩放与超块")
    p.table(
        headers=["字段", "内容", "字节"],
        rows=[
            ["`qs`", "256 个 4 位编码", "128"],
            ["`scales`", "8 个 6 位步长与 8 个 6 位偏移，16 × 6 = 96 位", "12"],
            ["`d`、`dmin`", "两个 fp16，16 个 6 位系数的公共缩放", "4"],
            ["合计", "256 个权重", "144"],
        ],
        align=["left", "left", "right"],
    )
    p.slide("""
- 144 × 8 ÷ 256 = ==4.5 位/权重==，与 Q4_0 相同
- 这 16 个系数若按 fp16 存放，超块为 128 + 32 = 160 字节，合 5.0 位/权重，与 Q4_1 相同
- 两者之差 16 字节，全部来自把 16 个系数从 16 位压到 6 位
""", reveal="items")
    p.highlight("系数本身也被量化，Q4_1 的贴合程度因此落进了 Q4_0 的体积。", tone="blue")
    p.notes("""
160 字节这一档不是假想的：Q4_1 的块是 32 个权重 20 字节，8 个块正好 160 字节，
两者的系数数量也一样多。Q4_K 与 Q4_1 的区别只在这 16 个系数怎么存。
`d` 与 `dmin` 这 4 字节是为压缩系数而付出的代价，摊到 256 个权重上是 0.125 位。
""")


def k_scales_layout(p):
    p.title("Q4_K：6 位量化系数的字节表示")
    p.image("assets/k-scales.svg", width_px=830)
    p.slide("""
16 × 6 = 96 位，正好是 12 字节。前 8 个字节各放一个完整的 6 位值，剩下的两位借给后 4 个值的高半段。
- 6 与 8 不整除，填满 12 字节就必须让一部分值跨字节存放
- 后 4 个 scale 与后 4 个 min 因此各分成两段，高 2 位与低 4 位存在不同的字节里
""")
    p.highlight("没有一位是空的，也因此没有一种按字节的读法能直接读对。", tone="orange")
    p.notes("""
这是本讲位运算内容在真实格式里最密的一处：没有任何一位是空的，
因此也没有一种「顺手」的读法能读对，必须按定义取位。
容易写错的地方是后四个值：它的低 4 位在第 8 到 11 字节，高 2 位在第 0 到 7 字节，
两段来自不同的字节，拼接的顺序反了不会报错，只会让还原出的权重整体偏大或偏小。
""")


def k_scales_code(p):
    p.title("Q4_K：取出第 j 个子块的系数")
    p.code("c", K_SCALES)
    p.slide(r"""
- 取出的 $sc$ 与 $m$ 都是无符号的 6 位整数，$d_{min}$ 也非负，因此偏移项 $-d_{min} \cdot m$ 不大于 0
- 每个子块能表示的最低一级落在 0 或 0 以下，这是这个格式在全为正的权重上的一处限制
""")
    p.highlight("`& 63` 取低六位，`>> 6` 取借出的两位，`<< 4` 把它移回高半段。", tone="blue")
    p.notes("""
举一个例子：若 `sc[0] = 63`、`sc[4] = 62`，则第 0 字节是 `0xff`，低 6 位是 63，
高 2 位 `11` 是 62 的高两位；第 8 字节 `0x9e` 的低 4 位 `e` 是 62 的低四位。
写法与读法必须成对，`examples/k_scales.c` 里两个函数都在，课上有人问起可以现跑。
偏移不大于 0 这一条是 llama.cpp 的量化器与这个存放格式共同决定的：
`make_qkx2_quants` 里有一句 `if (min > 0) min = 0;`，而格式本身也只能存非负的 `m`。
""")


def q4_k_measured(p):
    p.title("三种 4 位格式的对照")
    p.table(
        headers=["格式", "每块", "位/权重", "近似对称的一组", "全为正的一组"],
        rows=[
            ["Q4_0", "32 个权重 · 18 字节", "4.50", "8.42%", "4.25%"],
            ["Q4_1", "32 个权重 · 20 字节", "5.00", "7.69%", "0.94%"],
            ["Q4_K", "256 个权重 · 144 字节", "4.50", "7.70%", "2.16%"],
        ],
        align=["left", "left", "center", "right", "right"],
    )
    p.slide("""
- Q4_K 在两组数据上都接近 Q4_1 的精度，占的空间却与 Q4_0 相同
- 它在全为正的那一组上不及 Q4_1：它的偏移项不大于 0，最低一级必须落在 0 或 0 以下
""")
    p.highlight("同样是 4.5 位，把系数组织得更好就能换来更小的误差。", tone="orange")
    p.notes("""
偏移限制为不大于 0 是 llama.cpp 的选择（`if (min > 0) min = 0;`）：
绝大多数权重张量都跨过 0，放开这个限制收益很小，而限制之后 0 一定可以被精确表示。
本页两列误差就是前面两次实测的最后三行。
llama.cpp 真正的 Q4_K 量化器还会在几个候选的 (步长, 偏移) 上搜索，
比这里按最小值与最大值直接算的版本再好一点。
""")


def mixed_recipe(p):
    p.title("Q4_K_M：同一个权重文件里的多种格式")
    p.slide("""
下表是第一讲用 `ollama pull llama3.2` 取到的那个 1.9 GB 文件的实际组成。
""", autobold=False)
    p.table(
        headers=["张量", "占参数量", "格式", "位/权重"],
        rows=[
            ["词嵌入表 `token_embd`", "较大", "Q6_K", "6.56"],
            ["部分层的 `attn_v`、`ffn_down`", "约六分之一", "Q6_K", "6.56"],
            ["其余权重矩阵", "绝大多数", "Q4_K", "4.50"],
            ["归一化系数", "很小", "F32", "32"],
        ],
        align=["left", "left", "center", "center"],
    )
    p.slide("""
- 加权平均 ==5.01 位==，3.21 B 参数合 2.02 GB = 1.88 GiB，即第一讲 `ls -lh` 显示的 1.9G
- 提到 6 位的是量化误差影响最大的那些张量，其余保持 4 位
""")
    p.highlight("配方的自由度在于为每个张量单独选择格式。", tone="blue")
    p.notes("""
表中数据由 `examples/gguf_bits.py` 统计得到，它默认读 ollama 下载的最大的那个 blob，即第一讲的 llama3.2（3.21 B 参数），
统计得到 255 个张量、加权平均 5.009 位、张量数据 2 011 539 712 字节、元数据 7 837 658 字节。
255 个张量是 28 层 × 9 + 词嵌入 + 输出归一化 + `rope_freqs`。
llama.cpp 的命名里 `_S`、`_M`、`_L` 就是提精度的张量多少之别，
选哪些张量提精度靠的是在校准集上逐张量试，不是从格式本身推出来的。
""")


def quantization_cost(p):
    p.title("量化的代价与边界")
    p.slide("""
- 误差随组内取值范围的扩大而增大，少数异常值会抬高整组的缩放系数
- 分组越细，缩放系数占用的空间越多，平均位数随之上升
- 权重可以量化到 4 位，激活值与注意力的中间结果通常需要更高精度
- 反量化本身消耗计算，收益来自访存量的下降
""", reveal="items")
    p.image("assets/ext/llm-int8-fig2.svg", width_px=700,
            caption="工业界的一种做法：少数异常特征列保留 16 位，其余列按整数乘法完成"
            ).footnote("图片来自 Dettmers et al., LLM.int8()（NeurIPS 2022）图 2，CC BY 4.0。")
    p.cite(title="LLM.int8(): 8-bit Matrix Multiplication for Transformers at Scale",
           author="Dettmers et al.", year="2022", venue="NeurIPS",
           url="https://arxiv.org/abs/2208.07339", key="llm-int8")
    p.cite(title="GPTQ: Accurate Post-Training Quantization for Generative Pre-trained Transformers",
           author="Frantar et al.", year="2023", venue="ICLR",
           url="https://arxiv.org/abs/2210.17323", key="gptq")


def quantization_methods(p):
    p.title("拓展：线性量化以及它之外的做法")
    p.slide("""
本节给出的 Q4_0 是最基本的一种：级在取值区间上等距排列，一组权重共用一个缩放系数，这称为==线性量化==。实际使用的方法在四个方向上更复杂。
""", autobold=False)
    p.slide("""
- **零点**：Q4_0 的级关于 0 对称，Q4_1 另存一个最小值，级可以偏向权重实际的分布区间
- **分组**：Q4_K 每 256 个权重再分 8 个子块，子块各有缩放与最小值，这些系数本身也被量化
- **校准**：GPTQ 按逐层的二阶误差选级，AWQ 先按激活的重要性缩放再量化
- **非均匀**：NF4 按正态分布的分位点取级，级不等距，用于权重接近正态的情形
""", reveal="items")
    p.highlight("线性量化只用一个缩放系数把区间等分，其余方法都在放宽这一条。", tone="blue")
    p.cite(title="QLoRA: Efficient Finetuning of Quantized LLMs", author="Dettmers et al.",
           year="2023", venue="NeurIPS", url="https://arxiv.org/abs/2305.14314", key="qlora")
    p.notes("""
上一页实测到的 Q4_K_M 正是「分组」这一条的产物：它在 Q4_K 的基础上，
把量化误差影响最大的那些张量整体提到 6 位。
NF4 出自 QLoRA，级取自标准正态的分位点，对已经归一化过的权重块误差更小。
AWQ 与 GPTQ 都需要一小批校准数据，属于训练后量化；量化感知训练是另一条路线，
代价是要重新训练，本节不展开。
""")


def lab_preview(p):
    p.gap(20)
    p.title("练习：把一个 BF16 模型量化到 4 位")
    p.slide("""
从 Hugging Face 取 `Qwen/Qwen3-VL-2B-Instruct`，读取它的 safetensors 文件，
把其中 310 个 BF16 张量按本节的三种格式量化，度量误差，最后跑一次生成。
""", autobold=False)
    p.slide("""
- **读**：8 字节的头长度按小端合成，JSON 头给出每个张量的类型、形状与字节区间
- **转**：BF16 到 FP32 是一次左移 16 位，本讲第五部分讲过
- **量化**：Q4_0、Q4_1、Q4_K 各写一个 `quantize` 与 `dequantize`
- **验**：账目与 `ls -l` 精确相等，误差满足上界，最后以学号为输入生成一段文字
""", reveal="items")
    p.highlight("框架由助教提供，需要自己写的是读位、拼位与量化这几步。", tone="blue")
    p.notes("""
实验说明另发。模型文件 4.25 GB，Apache-2.0 许可，无需申请，校园网下用校内镜像。
文件里 625 个张量，其中 315 个属于视觉塔，实验只处理语言模型的那 310 个。
产物约 1.10 GB。生成用贪心解码，输出文本的 md5 每人一个，用来自查。
这一讲的每一节在实验里都有一次落地：
小端合成对应第二部分，BF16 的位布局对应第五部分，掩码与移位对应本节。
""")


# ==============================================================================
# 综合
# ==============================================================================

def summary(p):
    p.gap(30)
    p.title("本节小结")
    p.slide("""
- **一个字节是什么**：8 个两值信号，写作两位十六进制，是最小的可寻址单位
- **多字节对象如何排列**：由字节序规定；GGUF 与 x86-64 都采用小端
- **同一段位如何得到不同的值**：无符号数与有符号数的差别在最高位的权重，转换处最易出错
- **实数如何编码**：符号、阶码、尾数三段分配位数；量化把一个权重压到 4 位上下
""", reveal="items")
    p.slide("""
位本身没有含义，含义来自解释它的规则；规则写在格式规范、类型系统与硬件约定里。
类型转换处的错误不会在编译时报告，需要在阅读与审查代码时主动检查。
""", autobold=False)
    p.highlight("读一段字节之前，先确定按什么规则读。", tone="blue")
    p.notes("""
这四条合起来就是把 1.9 GB 字节读成一个模型所需的全部约定。
还可以补一句与后续内容的连接：机器级程序里立即数与地址同样按小端存放，
存储层次一节里数据的位宽直接决定每个 token 的访存量，
链接与动态加载处理的目标文件同样是有规范的字节序列，用的是同一套方法。
""")
