#pragma once

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ELF format according to
 * https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
 */

#define EI_MAG_SIZE 4

#define PT_NULL    0x00000000
#define PT_LOAD    0x00000001
#define PT_DYNAMIC 0x00000002
#define PT_INTERP  0x00000003
#define PT_NOTE    0x00000004
#define PT_SHLIB   0x00000005
#define PT_PHDR    0x00000006
#define PT_LOOS    0x60000000
#define PT_HIOS    0x6fffffff
#define PT_LOPROC  0x70000000
#define PT_HIRPOC  0x7fffffff

#define PF_ALL 0x7
#define PF_X   0x1
#define PF_W   0x2
#define PF_R   0x4

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

/*
 * This part of ELF header is endianness-independent.
 */
struct elf_indent {
	u8 ei_magic[4];
	u8 ei_class;
	u8 ei_data;
	u8 ei_version;
	u8 ei_osabi;
	u8 ei_abiversion;
	u8 ei_pad[7];
};

/*
 * ELF header format. One should check the `e_indent` to decide the endianness.
 */
struct elf_header {
	struct elf_indent e_indent;
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
};

/*
 * 32-Bit of the elf_header. Check the `e_indent` first to decide.
 */
struct elf_header_32 {
	struct elf_indent e_indent;
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u32 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;
};

struct elf_program_header {
	u32 p_type;
	u32 p_flags;
	u64 p_offset;
	u64 p_vaddr;
	u64 p_paddr;
	u64 p_filesz;
	u64 p_memsz;
	u64 p_align;
};
struct elf_program_header_32 {
	u32 p_type;
	u32 p_offset;
	u32 p_vaddr;
	u32 p_paddr;
	u32 p_filesz;
	u32 p_memsz;
	u32 p_flags;
	u32 p_align;
};

struct elf_section_header {
	u32 sh_name;
	u32 sh_type;
	u64 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u64 sh_size;
	u32 sh_link;
	u32 sh_info;
	u64 sh_addralign;
	u64 sh_entsize;
};
struct elf_section_header_32 {
	u32 sh_name;
	u32 sh_type;
	u32 sh_flags;
	u32 sh_addr;
	u32 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;
};

struct elf_info {
  u64 mem_size;
  u64 entry;
  u64 flags;
  u64 phentsize;
  u64 phnum;
  u64 phdr_addr;
  struct elf_program_header phdr[3]; /* currently 3 segments for procmgr */
};

struct elf_file {
	struct elf_header header;
	struct elf_program_header *p_headers;
	struct elf_section_header *s_headers;
};

struct elf_file *elf_parse_file(const char *code, struct elf_info*info);
void elf_free(struct elf_file *elf);

#ifdef __cplusplus
}
#endif
