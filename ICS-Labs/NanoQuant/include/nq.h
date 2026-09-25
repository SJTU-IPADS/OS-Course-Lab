/* nq.h -- the framework's internal interface. Everything here is already
 * implemented; you do not need to change it.
 *
 * Reading safetensors, the types and recipes, the choice of tensors, types
 * and names, the .nq container, and the framework's own conversions and
 * formats. The helpers for your code are in nano_quant.h. */
#ifndef NQ_H
#define NQ_H

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "nano_quant.h"

/* ================= reading safetensors ================= */

/* One tensor of the file. dtype is BF16, F16 or F32. */
struct st_tensor {
    std::string           name;
    std::string           dtype;
    std::vector<uint64_t> shape;    /* row major: shape[0] varies slowest */
    uint64_t              beg;      /* byte offset from the start of the data area */
    uint64_t              end;
    uint64_t              n_elem;   /* product of shape */
    size_t                elem_size;/* 2 or 4 */
};

struct st_file {
    int                    fd = -1;
    uint64_t               file_size = 0;
    uint64_t               header_len = 0;
    uint64_t               data_base = 0;   /* 8 + header_len */
    std::vector<st_tensor> tensors;
    std::string            err;
};

/* Open the file and parse its header. The header length is assembled with
   rd_u64le from nano_quant.h, so this fails until part A works.
   Returns 0, or -1 with f->err filled in. */
int st_open(st_file *f, const char *path);
void st_close(st_file *f);

int              st_count(const st_file *f);
const st_tensor *st_get(const st_file *f, int i);
const st_tensor *st_find(const st_file *f, const char *name);

/* Read n_elem elements of tensor t, starting at element elem_off, into dst
   as floats: BF16 through bf16_to_f32, F16 through fp16_to_f32, F32 as is.
   Returns the number of elements read, 0 on error. */
size_t st_read_f32(const st_file *f, const st_tensor *t,
                   uint64_t elem_off, float *dst, size_t n_elem);

/* ================= types and recipes ================= */

/* The type of a tensor in the output. The values are GGUF type numbers. */
typedef enum {
    NQ_TYPE_F32  = 0,
    NQ_TYPE_F16  = 1,
    NQ_TYPE_Q4_0 = 2,
    NQ_TYPE_Q4_1 = 3,
    NQ_TYPE_Q4_K = 12,
    NQ_TYPE_Q6_K = 14,
} nq_type;

typedef enum {
    NQ_RECIPE_Q4_0,     /* every 2-D tensor in Q4_0 */
    NQ_RECIPE_Q4_1,     /* every 2-D tensor in Q4_1 */
    NQ_RECIPE_Q4_K,     /* every 2-D tensor in Q4_K */
    NQ_RECIPE_Q4_K_M,   /* llama.cpp's Q4_K_M: embeddings and some layers in Q6_K */
} nq_recipe;

struct nq_type_info {
    const char *name;
    int         block_elems;   /* elements per block; 1 for F32 and F16 */
    int         block_bytes;
};
const nq_type_info *nq_type_of(nq_type t);
uint64_t nq_type_bytes(nq_type t, uint64_t n_elem);   /* bytes of n_elem elements */

/* Quantize n_elem floats as type t into dst, which holds at least
   nq_type_bytes(t, n_elem) bytes. n_elem is a multiple of the block length.
   Q4_0, Q4_1 and Q4_K call the functions of nano_quant.h; F32, F16 and Q6_K
   are done by the framework. */
void nq_quantize(nq_type t, const float *src, uint64_t n_elem, uint8_t *dst);
void nq_dequantize(nq_type t, const uint8_t *src, uint64_t n_elem, float *dst);

/* ================= which tensors, which type, which name =================
 *
 * The model file has 625 tensors: 315 of the vision tower and 310 of the
 * language model. Only the language model goes into the output. README.md
 * states the rules these functions follow. */

/* Whether the tensor named name (a safetensors name) goes into the output. */
bool nq_tensor_selected(const char *name);

/* The layer number in a tensor name, or -1 for a tensor outside the layers:
   model.language_model.layers.17.mlp.down_proj.weight is in layer 17. */
int nq_layer_of(const char *name);

/* Write the GGUF name of a safetensors name into buf. Returns 0, or -1 when
   the name is not in the table or buf is too small. */
int nq_gguf_name(const char *hf_name, char *buf, size_t buf_size);

/* The type of a tensor under recipe r. n_dims is its number of dimensions,
   layer its layer number (-1 outside the layers), n_layers the number of
   layers in the model. */
nq_type nq_recipe_pick(nq_recipe r, const char *name, int n_dims, int layer, int n_layers);

const char *nq_recipe_name(nq_recipe r);
int         nq_recipe_parse(const char *s, nq_recipe *out);

/* ================= the .nq container =================
 *
 *   [0,4)    "NQ01"
 *   [4,8)    uint32 number of tensors
 *   [8,16)   uint64 start of the data area
 *   [16,24)  uint64 bytes in the data area
 *   [24,..)  one record per tensor:
 *              uint32 name length in bytes
 *              char   name
 *              uint32 type (nq_type)
 *              uint32 number of dimensions
 *              uint64 length of each dimension (GGUF order, dimension 0 fastest)
 *              uint64 offset in the data area
 *              uint64 bytes
 *   The data area starts on a multiple of 32, and so does every tensor in it.
 *
 * nano-quant quant reports the md5 of the data area alone, which depends
 * neither on the header nor on the machine. */

struct nq_entry {
    std::string           name;      /* GGUF name */
    nq_type               type;
    std::vector<uint64_t> ne;        /* GGUF order */
    uint64_t              offset;
    uint64_t              nbytes;
};

struct nq_index {
    std::vector<nq_entry> entries;
    uint64_t              data_offset = 0;
    uint64_t              data_bytes  = 0;
};

/* Sequential writing: nq_write_begin writes the header, nq_write_tensor each
   tensor in turn, nq_write_end finishes the file. */
struct nq_writer;
nq_writer *nq_write_begin(const char *path, const std::vector<nq_entry> &plan);
int        nq_write_tensor(nq_writer *w, int i, const uint8_t *data, uint64_t nbytes);
/* Write tensor i in pieces; byte_off is the offset inside the tensor. After
   the last piece, nq_write_done marks the tensor as complete. */
int        nq_write_chunk(nq_writer *w, int i, uint64_t byte_off, const uint8_t *data, uint64_t n);
void       nq_write_done(nq_writer *w, int i);
int        nq_write_end(nq_writer *w);

int nq_read_index(const char *path, nq_index *idx);

#define NQ_ALIGN 32
static inline uint64_t nq_align_up(uint64_t x) { return (x + NQ_ALIGN - 1) & ~(uint64_t)(NQ_ALIGN - 1); }

/* ================= the framework's own conversions and formats ================= */

/* The same functions as f32_to_fp16, fp16_to_f32 and bf16_to_f32 in
   nano_quant.h. The F16 and Q6_K formats use these, and nq-selftest compares
   yours against them. Reading weights from a safetensors file uses yours. */
uint16_t nq_f32_to_fp16(float f);
float    nq_fp16_to_f32(uint16_t h);
float    nq_bf16_to_f32(uint16_t h);

#define NQ_Q6_K_BLOCK_ELEMS 256
#define NQ_Q6_K_BLOCK_BYTES 210
void nq_q6_k_quantize(const float *x, uint8_t *blk);
void nq_q6_k_dequantize(const uint8_t *blk, float *x);

/* ================= files =================
 *
 * Reading and writing at a byte offset, the same on Linux, macOS and Windows
 * (src/file.cpp). A file is a descriptor; offsets are 64-bit, since the model
 * file is larger than 4 GiB. The functions that return a number return -1
 * on error. */

int     nq_open_read(const char *path);
int     nq_open_write(const char *path);      /* created, or emptied if it exists */
int64_t nq_file_size(int fd);
/* 0 once all n bytes are read or written; -1 on an error, and for a read
   also when the file ends first. Writing past the end lengthens the file,
   and the bytes skipped over read as zero. */
int     nq_read_at(int fd, void *buf, size_t n, uint64_t off);
int     nq_write_at(int fd, const void *buf, size_t n, uint64_t off);
int     nq_close(int fd);
bool    nq_is_terminal(int fd);
/* Lines on standard output end in \n on Windows too, so each program writes
   the same bytes on every machine. Called first thing in main. */
void    nq_stdout_binary(void);

/* ================= miscellaneous ================= */

std::string nq_md5_hex(const uint8_t *data, size_t n);
std::string nq_md5_file_range(const char *path, uint64_t off, uint64_t len);

#endif /* NQ_H */
