/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef LIBCHCOREELF_H
#define LIBCHCOREELF_H

#include <chcore/type.h>
#include <chcore/memory.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

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

/**
 * This struct represents **all** header content of an ELF file, including
 * ELF header, program headers, and section headers.
 */
struct elf_file {
        struct elf_header header;
        struct elf_program_header *p_headers;
        struct elf_section_header *s_headers;
};

#define ELF_PATH_LEN 255

/**
 * This struct represents some useful metadata coming from header of an
 * ELF file.
 */
struct process_metadata {
        /**
         * start virtual address of area containing program headers
         * This value can be used for AT_PHDR auxv entry.
         */
        vaddr_t phdr_addr;
        /** size of a program header, can be used for AT_PHENT */
        size_t phentsize;
        /** number of program headers, can be used for AT_PHNUM */
        size_t phnum;
        /** flags defined in ELF header */
        int flags;
        /** entrypoint */
        vaddr_t entry;
        /** type of the ELF file (EXEC, DYN, etc) */
        unsigned int type;
};

/**
 * This struct represents a loadable(PT_LOAD) segment in an ELF file.
 * It contains some metadata and the actual content of the segment, and
 * those data can be used to map this segment into an address space.
 */
struct user_elf_seg {
        /** cap of a PMO holding all content of the segment in memory */
        cap_t elf_pmo;
        /** in-memory size of the segment(p_memsz) */
        size_t seg_sz;
        /** start virtual address the segment should be mapped at */
        vaddr_t p_vaddr;
        /**
         * permission should be used when mapping the segment
         * It has been translated from ELF format into ChCore VMR permission.
         */
        vmr_prop_t perm;
};

/**
 * This struct represents an ELF file which has been completed loaded into
 * user's address space. It contains selected metadata of the ELF file, and all
 * of the loadable segments (PT_LOAD) of the ELF file.
 *
 * For metadata, we only store some necessary information in @elf_meta from
 * struct elf_header. For segments, each PT_LOAD segment is represented using a
 * struct user_elf_seg, which contains the PMO storing the content of the
 * segment, and users can use the PMO to map the segment into their address
 * space or other process's address space.
 */
struct user_elf {
        /** number of loadable segments in the ELF file */
        int segs_nr;
        struct user_elf_seg *user_elf_segs;
        char path[ELF_PATH_LEN + 1];
        struct process_metadata elf_meta;
};

/**
 * @brief free all resources used by an user_elf struct, including memory and
 * pmos
 *
 * @param user_elf
 */
void free_user_elf(struct user_elf *user_elf);

/**
 * @brief Load header of an ELF file from a file, can be used for detecting
 * dynamically linked ELF file with very low overhead.
 *
 * @param path [In]
 * @param elf_header [Out] returning pointer to newly loaded header if
 * successfully load and parse its content. The ownership of this pointer is
 * transfered to the caller, so the caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int load_elf_header_from_fs(const char *path, struct elf_header **elf_header);

/**
 * @brief Load the remaining content of an ELF file according to metadata in
 * elf_header, and parse it into an user_elf struct.
 *
 * @param path [In]
 * @param elf_header [In] this function only borrow the pointer, and will not
 * free it. It's the caller's responsibility to control the life cycle of this
 * pointer.
 * @param elf [Out] returning pointer to newly loaded user_elf struct if
 * success. The ownership of this pointer is transfered to the caller, so the
 * caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int load_elf_by_header_from_fs(const char *path, struct elf_header *elf_header,
                               struct user_elf **elf);

/**
 * @brief Load an ELF file and parse it into an user_elf struct.
 *
 * @param path [In]
 * @param elf [Out] returning pointer to newly loaded user_elf struct if
 * success. The ownership of this pointer is transfered to the caller, so the
 * caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int load_elf_from_fs(const char *path, struct user_elf **elf);

/**
 * @brief Read content of an ELF file already stored in memory, and parse it
 * into an user_elf struct.
 * @param code [In] pointer to the memory range storing the ELF file. It's the
 * caller's responsibility to ensure the memory range is valid. Otherwise the
 * behavior is undefined.
 * @param elf [Out] returning pointer to newly loaded user_elf struct if
 * success. The ownership of this pointer is transfered to the caller, so the
 * caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
int load_elf_from_mem(const char *code, struct user_elf **elf);

#ifdef __cplusplus
}
#endif

#endif /* LIBCHCOREELF_H */