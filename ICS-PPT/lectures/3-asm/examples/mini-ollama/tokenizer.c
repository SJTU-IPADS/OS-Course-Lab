/* tokenizer.c -- text to token numbers and back.

   The vocabulary is byte-level BPE, as in GPT-2. The text of a token is
   written with one character per byte: a printable byte stands for itself
   and each of the other 68 bytes for one character from U+0100 on (a space
   is U+0120). A text is first cut into its bytes, each a token of its own;
   then the pair of neighbours that comes earliest in the list of merges is
   joined into one token, again and again until no pair is in the list.

   The tokenizer of the model first cuts the text into words with a regular
   expression and joins pairs inside one word only. That step is left out
   here, except for its rule that a digit stays a token of its own. Ordinary
   sentences come out the same; a run of spaces in front of a word may be
   cut in a different place. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mini_ollama.h"

/* ---------------- a table from strings to their numbers ---------------- */

struct table {
    const struct str *key;
    int              *slot;             /* number + 1, or 0 for an empty slot */
    uint32_t          mask;
};

static uint32_t hash(const char *p, size_t len)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++)
        h = (h ^ (unsigned char)p[i]) * 16777619u;
    return h;
}

/* The number of the key equal to p[0..len), or -1. */
static int lookup(const struct table *t, const char *p, size_t len)
{
    for (uint32_t i = hash(p, len) & t->mask; t->slot[i]; i = (i + 1) & t->mask) {
        const struct str *k = &t->key[t->slot[i] - 1];
        if (k->len == len && memcmp(k->p, p, len) == 0)
            return t->slot[i] - 1;
    }
    return -1;
}

static struct table *index_of(const struct str *key, int n)
{
    struct table *t = malloc(sizeof *t);
    uint32_t size = 16;
    while (size < 2 * (uint32_t)n)
        size *= 2;
    if (!t || !(t->slot = calloc(size, sizeof *t->slot)))
        die("out of memory");
    t->key = key;
    t->mask = size - 1;
    for (int k = 0; k < n; k++) {
        uint32_t i = hash(key[k].p, key[k].len) & t->mask;
        while (t->slot[i])
            i = (i + 1) & t->mask;
        t->slot[i] = k + 1;
    }
    return t;
}

/* ---------------- bytes and their characters ---------------- */

enum { N_CHARS = 256 + 68 };

static int           byte_token[256];   /* the token of a single byte */
static unsigned char char_byte[N_CHARS];/* the byte a character stands for */

static int printable(int b)
{
    return (b >= 33 && b <= 126) || (b >= 161 && b <= 172) || b >= 174;
}

void tokenizer_init(struct model *m)
{
    m->vocab_index = index_of(m->vocab, m->n_vocab);
    m->merge_index = index_of(m->merges, m->n_merges);

    for (int c = 0; c < 256; c++)
        char_byte[c] = (unsigned char)c;
    for (int b = 0, next = 256; b < 256; b++) {
        int c = printable(b) ? b : next++;
        char utf8[2] = { (char)c, 0 };
        if (c >= 128) {                 /* two bytes: 110xxxxx 10xxxxxx */
            utf8[0] = (char)(0xc0 | c >> 6);
            utf8[1] = (char)(0x80 | (c & 63));
        }
        char_byte[c] = (unsigned char)b;
        byte_token[b] = lookup(m->vocab_index, utf8, c < 128 ? 1 : 2);
        if (byte_token[b] < 0)
            die("the vocabulary has no token for the byte %d", b);
    }
}

/* ---------------- text to tokens ---------------- */

static int is_digit(const struct model *m, int tok)
{
    const struct str *s = &m->vocab[tok];
    return s->len == 1 && s->p[0] >= '0' && s->p[0] <= '9';
}

/* Where the pair (a, b) stands in the list of merges, or -1; *joined is the
   token the pair becomes. */
static int merge_rank(const struct model *m, int a, int b, int *joined)
{
    const struct str *sa = &m->vocab[a], *sb = &m->vocab[b];
    char buf[256];
    if (is_digit(m, a) || is_digit(m, b) || sa->len + sb->len + 1 > sizeof buf)
        return -1;
    memcpy(buf, sa->p, sa->len);
    buf[sa->len] = ' ';
    memcpy(buf + sa->len + 1, sb->p, sb->len);
    int rank = lookup(m->merge_index, buf, sa->len + 1 + sb->len);
    if (rank < 0)
        return -1;
    memmove(buf + sa->len, buf + sa->len + 1, sb->len);
    *joined = lookup(m->vocab_index, buf, sa->len + sb->len);
    return *joined < 0 ? -1 : rank;
}

/* text[0..len) as tokens; tok has room for len of them. */
static int bpe(const struct model *m, const char *text, size_t len, int *tok)
{
    int n = (int)len;
    for (int i = 0; i < n; i++)
        tok[i] = byte_token[(unsigned char)text[i]];

    for (;;) {
        int at = -1, best = 0, joined = 0;
        for (int i = 0; i + 1 < n; i++) {
            int j, rank = merge_rank(m, tok[i], tok[i + 1], &j);
            if (rank >= 0 && (at < 0 || rank < best)) {
                at = i;
                best = rank;
                joined = j;
            }
        }
        if (at < 0)
            return n;
        tok[at] = joined;
        memmove(tok + at + 1, tok + at + 2, (size_t)(n - at - 2) * sizeof *tok);
        n--;
    }
}

/* The special token written at s, such as <|im_end|>, or -1. */
static int special(const struct model *m, const char *s, size_t *len)
{
    const char *close = s[0] == '<' && s[1] == '|' ? strstr(s + 2, "|>") : NULL;
    if (!close)
        return -1;
    *len = (size_t)(close + 2 - s);
    return lookup(m->vocab_index, s, *len);
}

/* The prompt, put into the chat format the model was trained on, as tokens.
   tok has room for MAX_CTX of them. This is the template in the Modelfile
   of the nano-quant lab. */
int tokenize(const struct model *m, const char *prompt, int *tok)
{
    static const char template[] =
        "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n";
    size_t size = strlen(prompt) + sizeof template, len = 0;
    char *text = malloc(size);
    int *piece = malloc(size * sizeof *piece);
    int n = 0;

    if (!text || !piece)
        die("out of memory");
    snprintf(text, size, template, prompt);

    /* the text between two special tokens is one piece for bpe() */
    for (const char *s = text, *from = text; ; s++) {
        int id = *s ? special(m, s, &len) : -1;
        if (*s && id < 0)
            continue;
        int k = bpe(m, from, (size_t)(s - from), piece);
        if (n + k + 1 >= MAX_CTX)
            die("the prompt is longer than %d tokens", MAX_CTX);
        memcpy(tok + n, piece, (size_t)k * sizeof *tok);
        n += k;
        if (!*s)
            break;
        tok[n++] = id;
        s += len - 1;
        from = s + 1;
    }
    free(text);
    free(piece);
    return n;
}

/* ---------------- tokens to text ---------------- */

/* Write the bytes the token stands for. A token may end in the middle of a
   UTF-8 character; the next token then brings the rest. */
void print_token(const struct model *m, int tok)
{
    const struct str *s = &m->vocab[tok];
    for (size_t i = 0; i < s->len; i++) {
        int c = (unsigned char)s->p[i];
        if (c >= 0xc0 && c < 0xe0 && i + 1 < s->len)
            c = (c & 31) << 6 | ((unsigned char)s->p[++i] & 63);
        putchar(c < N_CHARS ? char_byte[c] : '?');
    }
    fflush(stdout);
}
