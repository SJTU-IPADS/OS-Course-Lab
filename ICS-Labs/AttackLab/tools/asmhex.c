/* asmhex.c -- turn a line of assembly into the bytes to feed the program.
 *
 *     echo 'mov $2, %edi' | ./asmhex
 *     bf 02 00 00 00
 *     ./asmhex < code.s > code.hex        # then paste into exploit.txt
 *     ./asmhex < code.s | ./unhex | ./attack2
 *
 * The input is x86-64 assembly in AT&T syntax -- the syntax `objdump -d`
 * prints and the one the attack samples are written in: a `%` on registers, a
 * `$` on immediates, the source before the destination. One instruction per
 * line; several lines work too, and a label may be jumped to within one run,
 * since the bytes come out as one piece. Nothing is required around it: the
 * .text section of what the input assembles to is the whole output.
 *
 * The work is done by the assembler `as` of binutils -- the assembler gcc
 * itself drives, so it is already on a machine that can build this lab -- and
 * by `objcopy` next to it. Their messages, when the input does not assemble,
 * are left where they are printed.
 *
 * The output is two hex digits per byte, the form unhex reads; a newline byte
 * (0a) in it is called out in a last comment line, because the program reads
 * a prompt up to 0a and an attack that goes through a line cannot carry that
 * byte.
 */
#define _POSIX_C_SOURCE 200809L         /* mkstemp, popen */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static const char USAGE[] = "usage: asmhex < code.s";

static char obj_path[] = "/tmp/asmhex-obj-XXXXXX";
static char bin_path[] = "/tmp/asmhex-bin-XXXXXX";

static void fail(const char *message)
{
    unlink(obj_path);
    unlink(bin_path);
    fprintf(stderr, "asmhex: %s\n", message);
    exit(1);
}

int main(int argc, char **argv)
{
    char cmd[512], buf[4096];
    unsigned char *bytes = NULL;
    size_t len = 0, cap = 0, got;
    FILE *out, *in;
    int fd, status;

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            puts(USAGE);
            return 0;
        }
        fprintf(stderr, "asmhex: this tool takes no arguments\n%s\n", USAGE);
        return 2;
    }

    fd = mkstemp(obj_path);
    if (fd < 0)
        fail("cannot make a temporary file in /tmp");
    close(fd);
    fd = mkstemp(bin_path);
    if (fd < 0)
        fail("cannot make a temporary file in /tmp");
    close(fd);

    /* The assembler reads the input from its standard input, which is this
       pipe; its own messages go to standard error as they are. */
    snprintf(cmd, sizeof cmd, "as --64 -o %s -", obj_path);
    out = popen(cmd, "w");
    if (out == NULL)
        fail("cannot run as (is binutils installed?)");
    while ((got = fread(buf, 1, sizeof buf, stdin)) > 0) {
        if (fwrite(buf, 1, got, out) != got)
            break;
    }
    fputs("\n", out);                   /* end the last line politely */
    status = pclose(out);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
            fail("cannot run as (is binutils installed?)");
        fail("as could not assemble the input: its message is above");
    }

    /* Only the .text section: no ELF header, no padding, nothing added. */
    snprintf(cmd, sizeof cmd, "objcopy -O binary --only-section=.text %s %s",
             obj_path, bin_path);
    status = system(cmd);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        fail("objcopy could not write the assembled code: its message is above");

    in = fopen(bin_path, "rb");
    if (in == NULL)
        fail("cannot read the assembled code back");
    while ((got = fread(buf, 1, sizeof buf, in)) > 0) {
        if (len + got > cap) {
            unsigned char *bigger;
            cap = cap != 0 ? cap * 2 : 4096;
            if (cap < len + got)
                cap = len + got;
            bigger = realloc(bytes, cap);
            if (bigger == NULL)
                fail("out of memory");
            bytes = bigger;
        }
        memcpy(bytes + len, buf, got);
        len += got;
    }
    fclose(in);

    for (size_t i = 0; i < len; i++)
        printf("%02x%s", bytes[i], (i % 16 == 15 || i + 1 == len) ? "\n" : " ");
    if (memchr(bytes, '\n', len) != NULL)
        puts("# warning: these bytes hold 0a, which the program reads as "
             "the end of the prompt");
    free(bytes);
    unlink(obj_path);
    unlink(bin_path);
    return 0;
}
