#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "elf.h"
#include "lzss.h"

// #define DEBUG
#define PREFIX "[ChCompress]"

extern int optind,opterr,optopt;
extern char *optarg;

#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#ifdef DEBUG
#define debug(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define PT_LOAD		1		/* Loadable program segment */

unsigned char verbose = 0;
unsigned char comp = 1;
unsigned char check = 0;
int secnum = 0;

#define SECMAX  32
#define SECNAME 16
typedef struct sectype
{
    unsigned int paddr;
    unsigned int raddr;
    unsigned int len;
    unsigned int comp;
    unsigned int check;
    unsigned int checksum;
    char name[SECNAME];
} tt;
struct sectype secarr[SECMAX];

struct exec
{
        unsigned long a_info;         /* Use macros N_MAGIC, etc for access */
        unsigned a_text;              /* length of text, in bytes */
        unsigned a_data;              /* length of data, in bytes */
        unsigned a_bss;               /* length of uninitialized data area for file, i n bytes */
        unsigned a_syms;              /* length of symbol table data in file, in bytes */
        unsigned a_entry;             /* start address */
        unsigned a_trsize;            /* length of relocation info for text, in bytes */
        unsigned a_drsize;            /* length of relocation info for data, in bytes */
};    

/* abort program if i is not a valid index in secarr */
static void checkseci(const int i)
{
        if (i < SECMAX) {
                return;
        }
        error("illegal section number %d. %d is the max.\n", i, SECMAX-1);
        exit(3);
}

unsigned checksum(char *buf, int size)
{
        unsigned checksum;
        for (int i = 0; i < size; i++) {
                checksum += buf[i];
        }
        return checksum;
}

void dump(int section_address, unsigned char *buffer, int count, FILE *dumpfile)
{
        int i;

        for (i = 0; i < count; i += 4)
        {
                fprintf(dumpfile, "\t.word\t0x%02x%02x%02x%02x\n",
                        buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3]);
        }
}
int dumpsec(char *buf, int section_address, int section_size,
	char *section_name, FILE * dumpfile)
{
        char cmd[512];
        char *lzss_buf;

        checkseci(secnum);
        secarr[secnum].paddr = section_address;
        secarr[secnum].len = section_size;
        secarr[secnum].comp = comp;
        strncpy (secarr[secnum].name, section_name, sizeof(secarr[secnum].name)-1);
        secarr[secnum].name[sizeof(secarr[secnum].name)-1] = 0;

        fprintf (dumpfile, "\n\t.global _section%1d\n", secnum);
        fprintf (dumpfile, "_section%1d:\n", secnum);

        secarr[secnum].check = 0;
        secarr[secnum].checksum = 0;
        if (check) {
                secarr[secnum].check = 1;
                secarr[secnum].checksum = checksum(buf, section_size);
        }

        if (comp) {
                lzss_buf = (char *)malloc(section_size + section_size / 2 + 256);
        }
        secnum++;
        if (comp) {
                section_size = lzss(buf, lzss_buf, section_size, 1);
                dump(section_address, lzss_buf, section_size + 13, dumpfile);
                free(buf);
                free(lzss_buf);
        }
        else {
                dump(section_address, buf, section_size, dumpfile);
                free(buf);
        }

        return 0;
}

unsigned int ldelf (FILE *file, FILE *dumpfile)
{
        Elf32_Ehdr fh; 
        Elf32_Shdr sh, ssh;
        Elf32_Phdr *ph;
        char *strtab;
        char *mem;
        unsigned int i, k, vaddr;

        fseek(file, 0, SEEK_SET);
        if (fread(&fh, sizeof(fh), 1, file) != 1) {
                return -2;
        }

        if ((fh.e_ident[EI_MAG0] != 0x7f)
                || (fh.e_ident[EI_MAG1] != 'E')
                || (fh.e_ident[EI_MAG2] != 'L')
                || (fh.e_ident[EI_MAG3] != 'F')) {
                return -2;
        }

        fh.e_machine = ntohs(fh.e_machine);
        if (fh.e_machine != EM_SPARC) {
                error("not a SPARC executable (%d)\n", fh.e_machine);
                return -2;
        }

        fh.e_entry = ntohl(fh.e_entry);
        fh.e_shoff = ntohl(fh.e_shoff);
        fh.e_phoff = ntohl(fh.e_phoff);
        fh.e_phnum = ntohs(fh.e_phnum);
        fh.e_shnum = ntohs(fh.e_shnum);
        fh.e_phentsize = ntohs(fh.e_phentsize);
        fh.e_shentsize = ntohs(fh.e_shentsize);
        fh.e_shstrndx = ntohs(fh.e_shstrndx);
        fseek(file, fh.e_shoff + ((fh.e_shstrndx) * fh.e_shentsize), SEEK_SET);
        if (fread(&ssh, sizeof (ssh), 1, file) != 1) {
                error("header: file read error\n");
                return -1;
        }
        ssh.sh_name = ntohl(ssh.sh_name);
        ssh.sh_type = ntohl(ssh.sh_type);
        ssh.sh_offset = ntohl(ssh.sh_offset);
        ssh.sh_size = ntohl(ssh.sh_size);
        strtab = (char *)malloc(ssh.sh_size);
        fseek(file, ssh.sh_offset, SEEK_SET);
        if (fread(strtab, ssh.sh_size, 1, file) != 1) {
                error("string tab: file read error\n");
                return -1;
        }

        ph = calloc(fh.e_phnum, sizeof(Elf32_Phdr));
        if (NULL == ph) {
                error("pheader: memory allocation error");
                return -1;
        }
        for (i = 0; i < fh.e_phnum; i++) {
                fseek(file, fh.e_phoff+(i*fh.e_phentsize), SEEK_SET);
                if (fread(&ph[i], fh.e_phentsize, 1, file) != 1) {
                        error("pheader: file read error\n");
                        return -1;
                }
                ph[i].p_type = ntohl(ph[i].p_type);
                ph[i].p_offset = ntohl(ph[i].p_offset);
                ph[i].p_vaddr = ntohl(ph[i].p_vaddr);
                ph[i].p_paddr = ntohl(ph[i].p_paddr);
                ph[i].p_filesz = ntohl(ph[i].p_filesz);
                ph[i].p_memsz = ntohl(ph[i].p_memsz);
                if (verbose) {
                        debug("phead%d: type: %x, off: %d, vaddr: %x,"
                                "paddr: %x, fsize: %d, msize: %d\n", 
                                i, ph[i].p_type, ph[i].p_offset,ph[i].p_vaddr,
                                ph[i].p_paddr,ph[i].p_filesz, ph[i].p_memsz);
                }
        }

        for (i = 1; i < fh.e_shnum; i++) {
                fseek(file, fh.e_shoff + (i * fh.e_shentsize), SEEK_SET);
                if (fread(&sh, sizeof(sh), 1, file) != 1) {
                        error("section header: file read error\n");
                        return -1;
                }
                sh.sh_name = ntohl(sh.sh_name);
                sh.sh_addr = ntohl(sh.sh_addr);
                sh.sh_size = ntohl(sh.sh_size);
                sh.sh_type = ntohl(sh.sh_type);
                sh.sh_offset = ntohl(sh.sh_offset);
                sh.sh_flags = ntohl(sh.sh_flags);
                if ((sh.sh_type == 1) && (sh.sh_size > 0) && (sh.sh_flags & 2)) {
                        for (k = 0; k < fh.e_phnum; k++) {
                                if ((ph[k].p_type == PT_LOAD) &&
                                        (sh.sh_offset >= ph[k].p_offset) &&
                                        ((sh.sh_offset + sh.sh_size) <= (ph[k].p_offset + ph[k].p_memsz)) &&
                                        ((sh.sh_offset + sh.sh_size) <= (ph[k].p_offset + ph[k].p_filesz))) {
                                /*
                                        if ((sh.sh_addr >= ph[k].p_vaddr) && 
                                                ((sh.sh_addr+sh.sh_size) <= (ph[k].p_vaddr + ph[k].p_filesz))) {
                                */
                                        vaddr = sh.sh_addr;
                                        sh.sh_addr = sh.sh_addr - ph[k].p_vaddr + ph[k].p_paddr;
                                        sh.sh_addr = sh.sh_offset - ph[k].p_offset + ph[k].p_paddr;
                                        if ((verbose) && (vaddr != sh.sh_addr)) {
                                                printf( "relocating %s (%d): %08x -> %08x\n",
                                                        &strtab[sh.sh_name], k, vaddr, sh.sh_addr); 
                                        }
                                        // sprintf(tdebug, "%08x (%08x)\n", sh.sh_addr, ph[k].p_paddr; dprint(tdebug);
                                        break;
                                }
                        }
                        
                        if (verbose)
                                printf ("section: %s at 0x%x, size %d bytes\n",
                                        &strtab[sh.sh_name], sh.sh_addr, sh.sh_size);

                        mem = (char *)malloc(sh.sh_size);
                        if (mem != (char *)-1) {
                                if (sh.sh_type == 1) {
                                        fseek(file, sh.sh_offset, SEEK_SET);
                                        if (fread(mem, sh.sh_size, 1, file) != 1)
                                        	return -1;
                                        dumpsec(mem, sh.sh_addr, sh.sh_size,
                                                &strtab[sh.sh_name], dumpfile);
                                }
                        }
                        else {
                                printf("load address outside physical memory\n");
                                printf("load aborted\n");
                                return -1;
                        }
                }
        }

        free (ph);
        free (strtab);
        return (fh.e_entry);
}

#define M_SPARC 3
#define N_MACHTYPE(exec) (((exec).a_info >> 16) & 0xff)

static int ldaout(FILE *fp, FILE * dumpfile)
{
        char *mem, *memtext, *memdata;
        struct exec fh;

        fseek(fp, 0, SEEK_SET);
        if (fread(&fh, sizeof(fh), 1, fp) != 1) {
                error("file read error\n");
                return(-1);
        }
        
        fh.a_info = ntohl(fh.a_info);
        if (N_MACHTYPE(fh) != M_SPARC) {
                return(-2);
        }
        
        fh.a_text = ntohl(fh.a_text);
        fh.a_data = ntohl(fh.a_data);
        fh.a_bss  = ntohl(fh.a_bss);
        fh.a_entry = ntohl(fh.a_entry);

        if (verbose)
                printf("section: .text at 0x%08x, size %d bytes\n", 
                        fh.a_entry, fh.a_text);
        if (verbose)
                printf("section: .data at 0x%08x, size %d bytes\n", 
                        fh.a_entry + fh.a_text, fh.a_data);
        if (verbose)
                printf("section: .bss  at 0x%08x, size %d bytes\n", 
                        fh.a_entry + fh.a_text + fh.a_data, fh.a_bss);
        
        mem = calloc((fh.a_text+fh.a_data) / 4 + 1, 4);
        if (mem != NULL) {
                if (fread(mem, fh.a_text+fh.a_data, 1, fp) != 1) {
                        if (verbose)
                                printf("file read error\n");
                        free(mem);
                        return(-1);
                }
        
                memtext = calloc((fh.a_text)/4+1, 4);
                memdata = calloc((fh.a_data)/4+1, 4);
                memcpy(memtext, mem, fh.a_text);
                memcpy(memdata, mem+fh.a_text, fh.a_data);
                
                dumpsec (memtext, fh.a_entry, fh.a_text,
                        ".text", dumpfile);
                dumpsec (memdata, fh.a_entry + fh.a_text, fh.a_data,
                        ".data", dumpfile);
                
                /*MemLoadFn(lib, fh.a_entry, mem, fh.a_text + fh.a_data, verify);*/
                free(mem);
                return fh.a_entry;
        } 
        else {
                if (verbose)
                        printf("not enough memory\n");
                return (-1);
        }
}

int elf_load (char *filename, FILE *dumpfile)
{
        int ret;
        FILE *file;
        char errmsg[256];

        file = fopen(filename, "r");

        if (file == NULL) {
                error("Unable to open %s\n", filename);
                exit(1);
        }

        ret = ldelf(file, dumpfile);

        if (ret == -1) {
                error("%s is a not a valid ELF file!\n", filename);
                exit(1);
        }

        if (ret != -2)
                return ret;

        ret = ldaout(file, dumpfile);

        if (ret == -1) {
                error("%s is a not a valid aout file!\n", filename);
                exit(1);
        }

        if (ret == -2) {
                error("%s must be in SPARC elf32 or aout format!\n", filename);
                exit(1);
        }

        return ret;
}

void print_help()
{
        printf("Usage: chcompress [-v] [-o OUTPUT_FILE] INPUT_FILE\n");
        printf("Options:\n");
        printf("  -v\t\t\tDisplay verbose information\n");
        printf("  -c\t\t\tCompute checksum for each section\n");
        printf("  -o <filename>\t\tSpecify output file name\n");
        printf("  -h\t\t\tPrint this help message\n");
}

int main(int argc, char *argv[])
{
        char *filename;
        FILE *dumpfile;
        int entry, opt;
        char outfile[128] = "dump.s";

        while ((opt = getopt(argc, argv, "cho:v")) != -1) {
                switch (opt) {
                case 'c':
                        check = 1;
                        break;
                case 'o':
                        strcpy(outfile, optarg);
                        break;
                case 'h':
                        print_help();
                        break;
                case 'v':
                        verbose = 1;
                        break;
                default:
                        printf("Unknown option: %c\n", optopt);
                        print_help();
                }
        }
        if (optind < 2) {
                printf("Please specify an input file name\n");
                print_help();
                return -1;
        }
        filename = argv[optind];

        if (verbose) {
                printf("Input file: %s, output file: %s\n", filename, outfile);
        }

        dumpfile = fopen(outfile, "w+");
        if (dumpfile == NULL) {
                error("Unable to create dumpfile\n");
                exit(1);
        }

        entry = elf_load(filename, dumpfile);
        fprintf (dumpfile, "\n\t.global _entry\n");
        fprintf (dumpfile, "_entry:\n");
        fprintf (dumpfile, "\t.word\t0x%x\n", entry);   

        fprintf (dumpfile, "\n\t.text\n");
        fprintf (dumpfile, "\n\t.global filename\n");
        fprintf (dumpfile, "filename:\n");
        fprintf (dumpfile, "\t.string\t\"%s\"\n", filename);
        fprintf (dumpfile, "\n\t.align 64\n");
        fprintf (dumpfile, "\t.global sections\n");
        fprintf (dumpfile, "sections:\n");
        for (int i = 0; i < secnum; i++) {
                checkseci(i);
                fprintf(dumpfile, "\t.word\t0x%x\n", secarr[i].paddr); 
                fprintf(dumpfile, "\t.word\t_section%d\n", i);
                fprintf(dumpfile, "\t.word\t0x%x\n", secarr[i].len);
                fprintf(dumpfile, "\t.word\t0x%x\n", secarr[i].comp);
                fprintf(dumpfile, "\t.word\t0x%x\n", secarr[i].check);
                fprintf(dumpfile, "\t.word\t0x%x\n", secarr[i].checksum);
                if (strlen(secarr[i].name) > 14) {
                        char b[16];
                        memset(b, 0, sizeof(b));
                        strncpy(b, secarr[i].name, 12);
                        strcat(b,"...");
                        fprintf(dumpfile, "\t.string\t\"%s\"\n", b);
                } else {
                        fprintf(dumpfile, "\t.string\t\"%s\"\n", secarr[i].name);
                }
                fprintf (dumpfile, "\n\t.align 64\n");
        }
        fprintf (dumpfile, "\t.word\t0\n");
        fprintf (dumpfile, "\t.word\t0\n");

        fclose(dumpfile);
        return 0;
}