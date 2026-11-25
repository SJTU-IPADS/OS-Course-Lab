#include <string.h>
#include <stdio.h>

#define PREFIX "[ChCompress]"

#define DEBUG
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define debug(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

extern unsigned char verbose;

typedef struct
{
    char MAGIC;
    unsigned char PARAMS;
    unsigned char CHECKSUM;
    unsigned char dummy;
    unsigned char ENCODED_SIZE[4];
    unsigned char DECODED_SIZE[4];
} packet_header;

#define N   4096
#define F   18
#define THRESHOLD  2
#define NIL  N
#define MAGIC_NUMBER '\xaa'
#define EOP '\x55'
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

unsigned char text_buf[N + F - 1];
int match_position, match_length, lson[N + 1], rson[N + 257], dad[N + 1];
unsigned long textsize = 0, codesize = 0, printcount = 0;
unsigned char CHECKSUM;

int PutPacketInfo(char *buf)
{
        packet_header PH;

        PH.MAGIC = MAGIC_NUMBER;
        PH.PARAMS = (unsigned char) (((N >> 6) & 0xf0) |
                        ((((F / 18) % 3) << 2) & 0x0c) | (THRESHOLD - 1));
        PH.CHECKSUM = CHECKSUM;
        PH.ENCODED_SIZE[0] = (codesize >> 24);
        PH.ENCODED_SIZE[1] = (codesize >> 16);
        PH.ENCODED_SIZE[2] = (codesize >> 8);
        PH.ENCODED_SIZE[3] = codesize;
        PH.DECODED_SIZE[0] = textsize >> 24;
        PH.DECODED_SIZE[1] = textsize >> 16;
        PH.DECODED_SIZE[2] = textsize >> 8;
        PH.DECODED_SIZE[3] = textsize;
        memcpy (buf, &PH, sizeof (packet_header));
        return 0;
}

void InitTree(void)
{
        int i;

        for (i = N + 1; i <= N + 256; i++)
        rson[i] = NIL;
        for (i = 0; i < N; i++)
        dad[i] = NIL;
}

void InsertNode(int r)
{
        int i, p, cmp;
        unsigned char *key;

        cmp = 1;
        key = &text_buf[r];
        p = N + 1 + key[0];
        rson[r] = lson[r] = NIL;
        match_length = 0;
        for (;;) {
                if (cmp >= 0) {
                        if (rson[p] != NIL)
                                p = rson[p];
                        else {
                                rson[p] = r;
                                dad[r] = p;
                                return;
                        }
                }
                else {
                        if (lson[p] != NIL)
                                p = lson[p];
                        else {
                                lson[p] = r;
                                dad[r] = p;
                                return;
                        }
                }
                for (i = 1; i < F; i++)
                if ((cmp = key[i] - text_buf[p + i]) != 0)
                        break;
                if (i > match_length) {
                        match_position = p;
                        if ((match_length = i) >= F)
                                break;
                }
        }
        dad[r] = dad[p];
        lson[r] = lson[p];
        rson[r] = rson[p];
        dad[lson[p]] = r;
        dad[rson[p]] = r;
        if (rson[dad[p]] == p)
                rson[dad[p]] = r;
        else
                lson[dad[p]] = r;
        dad[p] = NIL;
}

void DeleteNode(int p)
{
        int q;

        if (dad[p] == NIL)
                return;
        if (rson[p] == NIL)
                q = lson[p];
        else if (lson[p] == NIL)
                q = rson[p];
        else {
                q = lson[p];
                if (rson[q] != NIL) {
                        do {
                                q = rson[q];
                        } while (rson[q] != NIL);
                        rson[dad[q]] = lson[q];
                        dad[lson[q]] = dad[q];
                        lson[q] = lson[p];
                        dad[lson[p]] = q;
                }
                rson[q] = rson[p];
                dad[rson[p]] = q;
        }
        dad[q] = dad[p];
        if (rson[dad[p]] == p)
                rson[dad[p]] = q;
        else
                lson[dad[p]] = q;
        dad[p] = NIL;
}

void
Encode (unsigned char *inbuf, 
        unsigned char *outbuf,
        int buflen, int oindex)
{
        int i, c, len, r, s, last_match_length, code_buf_ptr;
        unsigned char code_buf[17], mask;

        int lindex = 0;

        CHECKSUM = 0xff;
        InitTree();
        code_buf[0] = 0;
        code_buf_ptr = mask = 1;
        s = 0;
        r = N - F;
        for (i = s; i < r; i++)
                text_buf[i] = ' ';
        for (len = 0; len < F && (lindex < buflen); len++) {
                c = inbuf[lindex++];
                CHECKSUM ^= c;
                text_buf[r + len] = c;
        }
        if ((textsize = len) == 0)
                return;
        for (i = 1; i <= F; i++)
                InsertNode (r - i);
        InsertNode (r);
        do {
                if (match_length > len)
                        match_length = len;
                if (match_length <= THRESHOLD) {
                        match_length = 1;
                        code_buf[0] |= mask;
                        code_buf[code_buf_ptr++] = text_buf[r];
                }
                else {
                        code_buf[code_buf_ptr++] = (unsigned char) match_position;
                        code_buf[code_buf_ptr++] = (unsigned char)
                        (((match_position >> 4) & 0xf0)
                        | (match_length - (THRESHOLD + 1)));
                }
                if ((mask <<= 1) == 0) {
                        memcpy (&outbuf[oindex], code_buf, code_buf_ptr);
                        oindex += code_buf_ptr;
                        codesize += code_buf_ptr;
                        code_buf[0] = 0;
                        code_buf_ptr = mask = 1;
                }
                last_match_length = match_length;
                for (i = 0; i < last_match_length && (lindex < buflen); i++) {
                        c = inbuf[lindex++];
                        CHECKSUM ^= c;
                        DeleteNode (s);
                        text_buf[s] = c;
                        if (s < F - 1)
                        text_buf[s + N] = c;
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);
                        InsertNode (r);
                }
                if ((textsize += i) > printcount) {
                        printcount += 1024;
                }
                while (i++ < last_match_length) {
                        DeleteNode (s);
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);
                        if (--len)
                        InsertNode (r);
                }
        }
        while (len > 0);
        if (code_buf_ptr > 1) {
                memcpy (&outbuf[oindex], code_buf, code_buf_ptr);
                oindex += code_buf_ptr;
                codesize += code_buf_ptr;
        }
        outbuf[oindex++] = EOP;
        if (verbose) {
                printf ("Uncoded stream length: %ld bytes\n", textsize);
                printf ("Coded stream length: %ld bytes\n", codesize);
                printf ("Compression Ratio: %.3f\n", (double) textsize / codesize);
        }
}

int lzss(char *inbuf, char *outbuf, int len, int comp)
{
        int index;

        textsize = 0;
        codesize = 0;
        printcount = 0;

        if (comp) {
                index = sizeof(packet_header);
                Encode (inbuf, outbuf, len, index);
                if (PutPacketInfo (outbuf)) {
                        error("Error:couldn't write packet header\n");
                }
        }
        return (codesize);
}