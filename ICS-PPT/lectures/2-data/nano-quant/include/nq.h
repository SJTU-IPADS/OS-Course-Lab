/* nq.h —— 框架自己用的接口。学生不需要实现这里的任何东西。
 *
 * 分四块：safetensors 读取、.nq 容器、助教提供的算子、配方与名字表的数据。 */
#ifndef NQ_H
#define NQ_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "nano_quant.h"

/* ================= safetensors 读取 ================= */

/* 文件里一个张量的元信息。dtype 只支持 BF16、F16、F32 三种。 */
struct st_tensor {
    std::string           name;
    std::string           dtype;
    std::vector<uint64_t> shape;    /* 行优先，shape[0] 变化最慢 */
    uint64_t              beg;      /* 相对数据区起点的字节偏移 */
    uint64_t              end;
    uint64_t              n_elem;   /* shape 的乘积 */
    size_t                elem_size;/* 2 或 4 */
};

struct st_file {
    int                    fd = -1;
    uint64_t               file_size = 0;
    uint64_t               header_len = 0;
    uint64_t               data_base = 0;   /* 8 + header_len */
    std::vector<st_tensor> tensors;
    std::string            err;
};

/* 打开并解析头。头长度用学生的 rd_u64le 合成，所以 A 部分没写完
   这一步就会失败。成功返回 0，失败返回 -1 并填 f.err。 */
int st_open(st_file *f, const char *path);
void st_close(st_file *f);

int              st_count(const st_file *f);
const st_tensor *st_get(const st_file *f, int i);
const st_tensor *st_find(const st_file *f, const char *name);

/* 从张量 t 的第 elem_off 个元素开始读 n_elem 个元素，转成 float 写进 dst。
   BF16 用学生的 bf16_to_f32，F16 用学生的 fp16_to_f32，F32 直接拷。
   返回实际读到的元素数，出错返回 0。 */
size_t st_read_f32(const st_file *f, const st_tensor *t,
                   uint64_t elem_off, float *dst, size_t n_elem);

/* ================= 类型表 ================= */

struct nq_type_info {
    const char *name;
    int         block_elems;   /* 一个块几个元素，F32/F16 记 1 */
    int         block_bytes;
};
const nq_type_info *nq_type_of(nq_type t);
uint64_t nq_type_bytes(nq_type t, uint64_t n_elem);   /* 元素数换算成字节数 */

/* 把 n_elem 个 float 按 t 量化进 dst，dst 至少 nq_type_bytes(t, n_elem) 字节。
   n_elem 必须是块长的整数倍。Q4_0/Q4_1/Q4_K 转调学生的函数，
   F32/F16/Q6_K 由框架自己完成。 */
void nq_quantize(nq_type t, const float *src, uint64_t n_elem, uint8_t *dst);
void nq_dequantize(nq_type t, const uint8_t *src, uint64_t n_elem, float *dst);

/* ================= .nq 容器 =================
 *
 *   [0,4)    "NQ01"
 *   [4,8)    uint32 张量数
 *   [8,16)   uint64 数据区起点
 *   [16,24)  uint64 数据区字节数
 *   [24,..)  逐个张量的记录：
 *              uint32 名字字节数
 *              char   名字
 *              uint32 类型（nq_type）
 *              uint32 维数
 *              uint64 各维长度（GGUF 顺序，第 0 维变化最快）
 *              uint64 数据区内偏移
 *              uint64 字节数
 *   数据区起点对齐到 32，区内每个张量也对齐到 32。
 *
 * 判定二用的就是数据区这一段的 md5，与文件头无关，也与机器无关。 */

struct nq_entry {
    std::string           name;      /* GGUF 名字 */
    nq_type               type;
    std::vector<uint64_t> ne;        /* GGUF 顺序 */
    uint64_t              offset;
    uint64_t              nbytes;
};

struct nq_index {
    std::vector<nq_entry> entries;
    uint64_t              data_offset = 0;
    uint64_t              data_bytes  = 0;
};

/* 顺序写出。先 nq_write_begin 建头，再逐个 nq_write_tensor，最后 nq_write_end。 */
struct nq_writer;
nq_writer *nq_write_begin(const char *path, const std::vector<nq_entry> &plan);
int        nq_write_tensor(nq_writer *w, int i, const uint8_t *data, uint64_t nbytes);
/* 分段写第 i 个张量。byte_off 是张量内的字节偏移。最后一段写完后
   调 nq_write_done 标记这个张量已经齐了。 */
int        nq_write_chunk(nq_writer *w, int i, uint64_t byte_off, const uint8_t *data, uint64_t n);
void       nq_write_done(nq_writer *w, int i);
int        nq_write_end(nq_writer *w);

int nq_read_index(const char *path, nq_index *idx);

#define NQ_ALIGN 32
static inline uint64_t nq_align_up(uint64_t x) { return (x + NQ_ALIGN - 1) & ~(uint64_t)(NQ_ALIGN - 1); }

/* ================= 助教提供的算子 ================= */

/* 与学生的 f32_to_fp16 / fp16_to_f32 同语义。框架内部只用这一对，
   这样学生 A 部分没写完时，判定仍然能跑起来并报出差在哪里。 */
uint16_t nq_f32_to_fp16(float f);
float    nq_fp16_to_f32(uint16_t h);
float    nq_bf16_to_f32(uint16_t h);

#define NQ_Q6_K_BLOCK_ELEMS 256
#define NQ_Q6_K_BLOCK_BYTES 210
void nq_q6_k_quantize(const float *x, uint8_t *blk);
void nq_q6_k_dequantize(const uint8_t *blk, float *x);

/* ================= 配方与名字表 ================= */

/* llama.cpp 的 Q4_K_M 判据：第 i 层（共 n 层）里 attn_v 与 ffn_down 是否升到 Q6_K。
   i < n/8 或 i >= 7n/8 或 (i - n/8) % 3 == 2 时为真。 */
bool nq_use_more_bits(int i, int n);

/* 名字映射表。suffix 是去掉 "model.language_model." 与层号之后的部分。 */
struct nq_name_rule {
    const char *hf;     /* 层内张量用 %d 占位，非层内张量是完整名字 */
    const char *gguf;
};
extern const nq_name_rule NQ_NAME_RULES[];
extern const int          NQ_NAME_RULE_COUNT;

/* 语言模型张量名的前缀。视觉塔的前缀是 model.visual. */
#define NQ_LM_PREFIX "model.language_model."

/* ================= 杂项 ================= */

std::string nq_md5_hex(const uint8_t *data, size_t n);
std::string nq_md5_file_range(const char *path, uint64_t off, uint64_t len);
const char *nq_recipe_name(nq_recipe r);
int         nq_recipe_parse(const char *s, nq_recipe *out);

#endif /* NQ_H */
