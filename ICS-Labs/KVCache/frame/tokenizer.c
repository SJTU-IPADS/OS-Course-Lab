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

static uint32_t hash(const char *text, size_t len)
{
    uint32_t value = 2166136261u;
    for (size_t i = 0; i < len; i++)
        value = (value ^ (unsigned char)text[i]) * 16777619u;
    return value;
}

/* The number of the key equal to text[0..len), or -1. */
static int lookup(const struct table *table, const char *text, size_t len)
{
    for (uint32_t i = hash(text, len) & table->mask; table->slot[i]; i = (i + 1) & table->mask) {
        const struct str *key = &table->key[table->slot[i] - 1];
        if (key->len == len && memcmp(key->text, text, len) == 0)
            return table->slot[i] - 1;
    }
    return -1;
}

static struct table *index_of(const struct str *keys, int count)
{
    struct table *table = malloc(sizeof *table);
    uint32_t size = 16;
    while (size < 2 * (uint32_t)count)
        size *= 2;
    if (!table || !(table->slot = calloc(size, sizeof *table->slot)))
        die("out of memory");
    table->key = keys;
    table->mask = size - 1;
    for (int idx = 0; idx < count; idx++) {
        uint32_t i = hash(keys[idx].text, keys[idx].len) & table->mask;
        while (table->slot[i])
            i = (i + 1) & table->mask;
        table->slot[i] = idx + 1;
    }
    return table;
}

/* ---------------- bytes and their characters ---------------- */

enum { N_CHARS = 256 + 68 };

static int           byte_token[256];   /* the token of a single byte */
static unsigned char char_byte[N_CHARS];/* the byte a character stands for */

static int printable(int byte)
{
    return (byte >= 33 && byte <= 126) || (byte >= 161 && byte <= 172) || byte >= 174;
}

void tokenizer_init(struct model *model)
{
    model->vocab_index = index_of(model->vocab, model->n_vocab);
    model->merge_index = index_of(model->merges, model->n_merges);

    for (int chr = 0; chr < 256; chr++)
        char_byte[chr] = (unsigned char)chr;
    for (int byte = 0, next = 256; byte < 256; byte++) {
        int chr = printable(byte) ? byte : next++;
        char utf8[2] = { (char)chr, 0 };
        if (chr >= 128) {               /* two bytes: 110xxxxx 10xxxxxx */
            utf8[0] = (char)(0xc0 | chr >> 6);
            utf8[1] = (char)(0x80 | (chr & 63));
        }
        char_byte[chr] = (unsigned char)byte;
        byte_token[byte] = lookup(model->vocab_index, utf8, chr < 128 ? 1 : 2);
        if (byte_token[byte] < 0)
            die("the vocabulary has no token for the byte %d", byte);
    }
}

/* ---------------- text to tokens ---------------- */

static int is_digit(const struct model *model, int token)
{
    const struct str *str = &model->vocab[token];
    return str->len == 1 && str->text[0] >= '0' && str->text[0] <= '9';
}

/* Where the pair (left, right) stands in the list of merges, or -1;
   *joined is the token the pair becomes. */
static int merge_rank(const struct model *model, int left, int right, int *joined)
{
    const struct str *left_str = &model->vocab[left], *right_str = &model->vocab[right];
    char buf[256];
    if (is_digit(model, left) || is_digit(model, right) ||
        left_str->len + right_str->len + 1 > sizeof buf)
        return -1;
    memcpy(buf, left_str->text, left_str->len);
    buf[left_str->len] = ' ';
    memcpy(buf + left_str->len + 1, right_str->text, right_str->len);
    int rank = lookup(model->merge_index, buf, left_str->len + 1 + right_str->len);
    if (rank < 0)
        return -1;
    memmove(buf + left_str->len, buf + left_str->len + 1, right_str->len);
    *joined = lookup(model->vocab_index, buf, left_str->len + right_str->len);
    return *joined < 0 ? -1 : rank;
}

/* text[0..len) as tokens; tokens has room for len of them. */
static int bpe(const struct model *model, const char *text, size_t len, int *tokens)
{
    int count = (int)len;
    for (int i = 0; i < count; i++)
        tokens[i] = byte_token[(unsigned char)text[i]];

    for (;;) {
        int at = -1, best = 0, joined = 0;
        for (int i = 0; i + 1 < count; i++) {
            int pair, rank = merge_rank(model, tokens[i], tokens[i + 1], &pair);
            if (rank >= 0 && (at < 0 || rank < best)) {
                at = i;
                best = rank;
                joined = pair;
            }
        }
        if (at < 0)
            return count;
        tokens[at] = joined;
        memmove(tokens + at + 1, tokens + at + 2, (size_t)(count - at - 2) * sizeof *tokens);
        count--;
    }
}

/* The special token written at text, such as <|im_end|>, or -1; *len is
   the length of its text. */
static int special(const struct model *model, const char *text, size_t *len)
{
    const char *closing = text[0] == '<' && text[1] == '|' ? strstr(text + 2, "|>") : NULL;
    if (!closing)
        return -1;
    *len = (size_t)(closing + 2 - text);
    return lookup(model->vocab_index, text, *len);
}

/* text as tokens, into tokens, which has room for room of them; the number
   of tokens, or -1 when they do not fit. A special token is written in the
   text as itself (<|im_start|>), and the text between two special tokens is
   one piece for bpe(). */
int tokenize(const struct model *model, const char *text, int *tokens, int room)
{
    size_t size = strlen(text) + 1, len = 0;
    int *piece = malloc(size * sizeof *piece);
    int count = 0;

    if (!piece)
        die("out of memory");
    for (const char *at = text, *from = text; ; at++) {
        int token = *at ? special(model, at, &len) : -1;
        if (*at && token < 0)
            continue;
        int n_piece = bpe(model, from, (size_t)(at - from), piece);
        if (count + n_piece + (*at != 0) > room) {
            count = -1;
            break;
        }
        memcpy(tokens + count, piece, (size_t)n_piece * sizeof *tokens);
        count += n_piece;
        if (!*at)
            break;
        tokens[count++] = token;
        at += len - 1;
        from = at + 1;
    }
    free(piece);
    return count;
}

/* ---------------- tokens to text ---------------- */

/* Write the bytes the token stands for. A token may end in the middle of a
   UTF-8 character; the next token then brings the rest. */
void print_token(const struct model *model, int token)
{
    const struct str *str = &model->vocab[token];
    for (size_t i = 0; i < str->len; i++) {
        int chr = (unsigned char)str->text[i];
        if (chr >= 0xc0 && chr < 0xe0 && i + 1 < str->len)
            chr = (chr & 31) << 6 | ((unsigned char)str->text[++i] & 63);
        putchar(chr < N_CHARS ? char_byte[chr] : '?');
    }
    fflush(stdout);
}
