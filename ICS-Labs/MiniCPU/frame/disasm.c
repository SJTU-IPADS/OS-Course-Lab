/* mini-cpu --disasm FILE: the code of an ELF file as assembly.
 *
 * Every section with code (SHF_EXECINSTR) is translated from its first
 * byte to its last by translate() of frame/translate.c. One line per
 * instruction:
 *
 *   401000:<tab>48 8b 3c 24<tab>mov    (%rsp),%rdi
 *
 * The instruction text is that of objdump -d without the symbol names in
 * <...>, so the two listings can be compared line by line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcpu.h"

int disasm(const char *path, FILE *f)
{
    FILE *in = fopen(path, "rb");
    if (!in) { perror(path); return -1; }
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);
    uint8_t *data = size > 0 ? malloc((size_t)size) : NULL;
    if (!data || fread(data, 1, (size_t)size, in) != (size_t)size) {
        fprintf(stderr, "%s: cannot read\n", path);
        fclose(in);
        free(data);
        return -1;
    }
    fclose(in);

    Elf64_Ehdr *eh = (Elf64_Ehdr *)data;
    if ((size_t)size < sizeof *eh || memcmp(eh->e_ident, ELFMAG, SELFMAG) ||
        eh->e_ident[EI_CLASS] != ELFCLASS64 ||
        eh->e_shoff + (uint64_t)eh->e_shnum * sizeof(Elf64_Shdr) > (uint64_t)size) {
        fprintf(stderr, "%s: not a 64-bit ELF file\n", path);
        free(data);
        return -1;
    }
    Elf64_Shdr *sh = (Elf64_Shdr *)(data + eh->e_shoff);
    for (unsigned s = 0; s < eh->e_shnum; ++s) {
        if (sh[s].sh_type != SHT_PROGBITS || !(sh[s].sh_flags & SHF_EXECINSTR) ||
            sh[s].sh_offset + sh[s].sh_size > (uint64_t)size)
            continue;
        const uint8_t *code = data + sh[s].sh_offset;
        uint64_t len;
        for (uint64_t off = 0; off < sh[s].sh_size; off += len) {
            char text[256], bytes[3 * MAX_INSN + 1];
            unsigned left = sh[s].sh_size - off < MAX_INSN ?
                            (unsigned)(sh[s].sh_size - off) : MAX_INSN;
            len = translate(sh[s].sh_addr + off, code + off, left, text, sizeof text);
            int k = 0;
            for (uint64_t j = 0; j < len; ++j)
                k += snprintf(bytes + k, sizeof bytes - (size_t)k, "%02x ", code[off + j]);
            fprintf(f, "%8llx:\t%-21s\t%s\n",
                    (unsigned long long)(sh[s].sh_addr + off), bytes, text);
        }
    }
    free(data);
    return 0;
}
