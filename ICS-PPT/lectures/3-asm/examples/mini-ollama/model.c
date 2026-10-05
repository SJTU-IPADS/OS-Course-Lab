/* model.c -- bring a GGUF file into memory and find the tensors in it.

   GGUF version 3, every integer little endian:

     "GGUF", uint32 version, uint64 number of tensors, uint64 number of pairs
     the key-value pairs:    string key, uint32 type, value
     the tensor directory:   string name, uint32 n_dims, uint64 dims[n_dims],
                             uint32 type, uint64 offset in the data area
     padding to a multiple of general.alignment, then the data area

   A string is a uint64 length followed by that many bytes. The file is
   kept as it is: tensors, token texts and merges point into it. The
   integers are read with memcpy, so the machine has to be little endian. */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "mini_ollama.h"

enum { MAX_WIDTH = 1 << 16 };           /* the longest vector kept on the stack */

float fp16[1 << 16];

void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "mini-ollama: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

/* ---------------- reading the bytes ---------------- */

struct reader {
    const uint8_t *p, *end;
};

static const uint8_t *take(struct reader *r, uint64_t n)
{
    const uint8_t *p = r->p;
    if (n > (uint64_t)(r->end - r->p))
        die("the file ends inside its header");
    r->p += n;
    return p;
}

static uint32_t rd_u32(struct reader *r)
{
    uint32_t v;
    memcpy(&v, take(r, 4), 4);
    return v;
}

static uint64_t rd_u64(struct reader *r)
{
    uint64_t v;
    memcpy(&v, take(r, 8), 8);
    return v;
}

static struct str rd_str(struct reader *r)
{
    struct str s;
    s.len = rd_u64(r);
    s.p = (const char *)take(r, s.len);
    return s;
}

static int str_is(struct str s, const char *name)
{
    return s.len == strlen(name) && memcmp(s.p, name, s.len) == 0;
}

static int str_ends(struct str s, const char *tail)
{
    size_t n = strlen(tail);
    return s.len >= n && memcmp(s.p + s.len - n, tail, n) == 0;
}

/* ---------------- the key-value pairs ---------------- */

enum { U8, I8, U16, I16, U32, I32, F32, BOOL, STR, ARR, U64, I64, F64, N_TYPES };
static const int value_size[N_TYPES] = { 1, 1, 2, 2, 4, 4, 4, 1, 0, 0, 8, 8, 8 };

static void skip_value(struct reader *r, uint32_t type)
{
    if (type == STR) {
        rd_str(r);
    } else if (type == ARR) {
        uint32_t elem = rd_u32(r);
        for (uint64_t n = rd_u64(r); n > 0; n--)
            skip_value(r, elem);
    } else if (type < N_TYPES) {
        take(r, value_size[type]);
    } else {
        die("unknown value type %u", (unsigned)type);
    }
}

static int rd_int(struct reader *r, uint32_t type, struct str key)
{
    if (type != U32)
        die("%.*s is not a uint32", (int)key.len, key.p);
    return (int)rd_u32(r);
}

static float rd_float(struct reader *r, uint32_t type, struct str key)
{
    float v;
    if (type != F32)
        die("%.*s is not a float", (int)key.len, key.p);
    memcpy(&v, take(r, 4), 4);
    return v;
}

static struct str *rd_strings(struct reader *r, uint32_t type, struct str key, int *n)
{
    if (type != ARR || rd_u32(r) != STR)
        die("%.*s is not an array of strings", (int)key.len, key.p);
    uint64_t count = rd_u64(r);
    if (count > (uint64_t)(r->end - r->p) / 8)
        die("%.*s is longer than the file", (int)key.len, key.p);
    struct str *v = malloc((count + 1) * sizeof *v);
    if (!v)
        die("out of memory");
    for (uint64_t i = 0; i < count; i++)
        v[i] = rd_str(r);
    *n = (int)count;
    return v;
}

/* The numbers that describe the model carry the name of the architecture
   in front (qwen3vl.block_count), so they are matched by their ending. */
static void rd_pair(struct model *m, struct reader *r, uint32_t *alignment)
{
    struct str key = rd_str(r);
    uint32_t type = rd_u32(r);

    if (str_ends(key, ".block_count"))
        m->n_layer = rd_int(r, type, key);
    else if (str_ends(key, ".embedding_length"))
        m->n_embd = rd_int(r, type, key);
    else if (str_ends(key, ".feed_forward_length"))
        m->n_ff = rd_int(r, type, key);
    else if (str_ends(key, ".attention.head_count"))
        m->n_head = rd_int(r, type, key);
    else if (str_ends(key, ".attention.head_count_kv"))
        m->n_head_kv = rd_int(r, type, key);
    else if (str_ends(key, ".attention.key_length"))
        m->head_dim = rd_int(r, type, key);
    else if (str_ends(key, ".attention.layer_norm_rms_epsilon"))
        m->eps = rd_float(r, type, key);
    else if (str_ends(key, ".rope.freq_base"))
        m->rope_base = rd_float(r, type, key);
    else if (str_is(key, "general.alignment"))
        *alignment = (uint32_t)rd_int(r, type, key);
    else if (str_is(key, "tokenizer.ggml.eos_token_id"))
        m->eos = rd_int(r, type, key);
    else if (str_is(key, "tokenizer.ggml.tokens"))
        m->vocab = rd_strings(r, type, key, &m->n_vocab);
    else if (str_is(key, "tokenizer.ggml.merges"))
        m->merges = rd_strings(r, type, key, &m->n_merges);
    else
        skip_value(r, type);
}

/* ---------------- the tensor directory ---------------- */

enum { TYPE_F32 = 0, TYPE_Q4_0 = 2 };

struct entry {
    struct str name;
    uint32_t   n_dims, type;
    uint64_t   dims[2], offset;
};

struct directory {
    struct entry  *entry;
    uint64_t       n;
    const uint8_t *data, *end;          /* the data area */
};

static void rd_entry(struct reader *r, struct entry *e)
{
    e->name = rd_str(r);
    e->n_dims = rd_u32(r);
    if (e->n_dims < 1 || e->n_dims > 2)
        die("%.*s has %u dimensions", (int)e->name.len, e->name.p, (unsigned)e->n_dims);
    e->dims[1] = 1;
    for (uint32_t i = 0; i < e->n_dims; i++)
        e->dims[i] = rd_u64(r);
    e->type = rd_u32(r);
    e->offset = rd_u64(r);
}

/* The tensor called name (layer is put in for %d), which has to be a Q4_0
   matrix of rows x cols, or a float vector of cols when rows is 1. */
static struct tensor find(const struct directory *dir, const char *name, int layer,
                          int cols, int rows)
{
    char full[64];
    snprintf(full, sizeof full, name, layer);

    for (uint64_t i = 0; i < dir->n; i++) {
        const struct entry *e = &dir->entry[i];
        if (!str_is(e->name, full))
            continue;
        if (e->dims[0] != (uint64_t)cols || e->dims[1] != (uint64_t)rows)
            die("%s is %llu x %llu, expected %d x %d", full,
                (unsigned long long)e->dims[0], (unsigned long long)e->dims[1], cols, rows);
        if (rows > 1 && e->type != TYPE_Q4_0)
            die("%s is not Q4_0: build the model with nano-quant --recipe q4_0", full);
        if (rows == 1 && e->type != TYPE_F32)
            die("%s is not a float vector", full);
        if (rows > 1 && cols % 32 != 0)
            die("%s: a row is not a whole number of Q4_0 blocks", full);

        uint64_t bytes = rows > 1 ? (uint64_t)rows * (cols / 32) * 18 : (uint64_t)cols * 4;
        uint64_t room = (uint64_t)(dir->end - dir->data);
        if (e->offset > room || bytes > room - e->offset)
            die("%s lies outside the file", full);
        struct tensor t = { (size_t)cols, (size_t)rows, dir->data + e->offset };
        return t;
    }
    die("the file has no tensor %s", full);
    return (struct tensor){ 0, 0, NULL };
}

static void find_tensors(struct model *m, const struct directory *dir)
{
    int e = m->n_embd, f = m->n_ff, d = m->head_dim;
    int q = m->n_head * d, kv = m->n_head_kv * d;

    m->token_embd  = find(dir, "token_embd.weight", 0, e, m->n_vocab);
    m->output_norm = find(dir, "output_norm.weight", 0, e, 1);
    m->logits = malloc((size_t)m->n_vocab * sizeof *m->logits);
    m->layer = calloc((size_t)m->n_layer, sizeof *m->layer);
    if (!m->logits || !m->layer)
        die("out of memory");

    for (int l = 0; l < m->n_layer; l++) {
        struct layer *L = &m->layer[l];
        L->attn_norm   = find(dir, "blk.%d.attn_norm.weight", l, e, 1);
        L->attn_q      = find(dir, "blk.%d.attn_q.weight", l, e, q);
        L->attn_k      = find(dir, "blk.%d.attn_k.weight", l, e, kv);
        L->attn_v      = find(dir, "blk.%d.attn_v.weight", l, e, kv);
        L->attn_q_norm = find(dir, "blk.%d.attn_q_norm.weight", l, d, 1);
        L->attn_k_norm = find(dir, "blk.%d.attn_k_norm.weight", l, d, 1);
        L->attn_output = find(dir, "blk.%d.attn_output.weight", l, q, e);
        L->ffn_norm    = find(dir, "blk.%d.ffn_norm.weight", l, e, 1);
        L->ffn_gate    = find(dir, "blk.%d.ffn_gate.weight", l, e, f);
        L->ffn_up      = find(dir, "blk.%d.ffn_up.weight", l, e, f);
        L->ffn_down    = find(dir, "blk.%d.ffn_down.weight", l, f, e);
        L->k_cache = malloc((size_t)MAX_CTX * kv * sizeof(float));
        L->v_cache = malloc((size_t)MAX_CTX * kv * sizeof(float));
        if (!L->k_cache || !L->v_cache)
            die("out of memory");
    }
}

/* ---------------- the whole file ---------------- */

/* Half precision: 1 sign bit, 5 exponent bits, 10 fraction bits. Q4_0 scales
   are finite, so infinity and NaN are not told apart from large numbers. */
static void fill_fp16(void)
{
    for (int h = 0; h < 1 << 16; h++) {
        int exp = h >> 10 & 31, frac = h & 1023;
        float v = exp == 0 ? ldexpf((float)frac, -24)
                           : ldexpf((float)(frac + 1024), exp - 25);
        fp16[h] = h >> 15 ? -v : v;
    }
}

#ifdef _WIN32

/* Windows has no mmap: read the whole file into memory. ftell() returns a
   long, which is 32 bits there, so the file has to be shorter than 2 GB. */
static const uint8_t *load_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        die("cannot open %s", path);
    fseek(f, 0, SEEK_END);
    long long n = ftell(f);
    rewind(f);
    uint8_t *buf = n > 0 ? malloc((size_t)n) : NULL;
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n)
        die("cannot read %s", path);
    fclose(f);
    *size = (size_t)n;
    return buf;
}

#else

/* Map the file read-only. Nothing is copied: a page of the file comes into
   memory when the program first touches it. */
static const uint8_t *load_file(const char *path, size_t *size)
{
    struct stat st;
    int fd = open(path, O_RDONLY);
    if (fd < 0 || fstat(fd, &st) != 0)
        die("cannot open %s", path);
    void *p = st.st_size > 0
            ? mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)
            : MAP_FAILED;
    if (p == MAP_FAILED)
        die("cannot map %s", path);
    close(fd);                          /* the mapping stays */
    *size = (size_t)st.st_size;
    return p;
}

#endif

struct model *load_model(const char *path)
{
    size_t size;
    const uint8_t *file = load_file(path, &size);
    struct reader r = { file, file + size };
    struct model *m = calloc(1, sizeof *m);
    struct directory dir;
    uint32_t alignment = 32;

    if (!m)
        die("out of memory");
    if (memcmp(take(&r, 4), "GGUF", 4) != 0 || rd_u32(&r) != 3)
        die("%s is not a GGUF version 3 file", path);
    dir.n = rd_u64(&r);
    uint64_t n_pairs = rd_u64(&r);

    m->eos = -1;
    for (uint64_t i = 0; i < n_pairs; i++)
        rd_pair(m, &r, &alignment);
    if (m->n_layer <= 0 || m->n_embd <= 0 || m->n_ff <= 0 || m->n_head <= 0 ||
        m->n_head_kv <= 0 || m->head_dim <= 0 || m->n_head % m->n_head_kv != 0 ||
        m->head_dim % 2 != 0 || m->rope_base <= 0 || !m->vocab || !m->merges ||
        m->eos < 0 || m->eos >= m->n_vocab || alignment == 0 ||
        m->n_embd > MAX_WIDTH || m->n_ff > MAX_WIDTH ||
        m->n_head > MAX_WIDTH / m->head_dim)
        die("%s does not describe a model this program can run", path);

    if (dir.n > (uint64_t)(r.end - r.p) / 24)
        die("the tensor directory is longer than the file");
    dir.entry = malloc((dir.n + 1) * sizeof *dir.entry);
    if (!dir.entry)
        die("out of memory");
    for (uint64_t i = 0; i < dir.n; i++)
        rd_entry(&r, &dir.entry[i]);

    uint64_t header = (uint64_t)(r.p - file);
    uint64_t start = (header + alignment - 1) / alignment * alignment;
    if (start > size)
        die("the file ends before its data area");
    dir.data = file + start;
    dir.end = file + size;

    find_tensors(m, &dir);
    free(dir.entry);
    fill_fp16();
    tokenizer_init(m);
    return m;
}
