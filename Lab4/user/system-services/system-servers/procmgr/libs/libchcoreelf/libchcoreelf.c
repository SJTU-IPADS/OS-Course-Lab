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

/**
 * @file libchcoreelf.c
 *
 * @brief ChCore-specialized ELF loading library. See core function
 * load_elf_from_range to understand the implementation of this library.
 */
#include "libchcoreelf.h"
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>

#define ELF_HEADER_SIZE (sizeof(struct elf_header))

/**
 * @brief Defining a function which can be used to randomly access an
 * abstract "range". See load_elf_from_range() for more details.
 *
 * A call to this function should load all bytes from offset to offset+len
 * into the buffer buf. Otherwise this function should fail.
 *
 * @param param [In] custom parameter, this parameter can be used to
 * indicate the start of a range, see file_range_loader and memory_range_loader
 * for examples.
 * @param offset [In] offset in the range
 * @param len [In] length of the data to be loaded
 * @param buf [Out] buffer to store the loaded data. Caller should guarantee
 * that the buffer is large enough to hold the data.
 *
 * @return 0 on success, negative error code on failure.
 */
typedef int (*range_loader_t)(void *param, off_t offset, size_t len, char *buf);

static bool is_elf_magic(struct elf_indent *indent)
{
        return (indent->ei_magic[0] == 0x7F && indent->ei_magic[1] == 'E'
                && indent->ei_magic[2] == 'L' && indent->ei_magic[3] == 'F');
}

#define ELF_ENDIAN_LE(indent) ((indent).ei_data == 1)
#define ELF_ENDIAN_BE(indent) ((indent).ei_data == 2)

#define ELF_BITS_32(indent) ((indent).ei_class == 1)
#define ELF_BITS_64(indent) ((indent).ei_class == 2)

/**
 * Parse an ELF file header. We use the 64-bit structure `struct elf_header` as
 * the output structure.
 *
 * To support both 32-bit and 64-bit ELF files, we first try to parse the small
 * leading part elf_ident of elf_header which is 32/64-bit independent. Then we
 * parse 32/64-bit dependent part of elf_header.
 *
 * On error, the negative error code is returned.
 * On success, 0 is returned, and the header is written in the given parameter.
 */
int parse_elf_header(const char *code, struct elf_header *header)
{
        struct elf_header *header_64 = (struct elf_header *)code;
        struct elf_header_32 *header_32 = (struct elf_header_32 *)code;

        if (!is_elf_magic(&header_64->e_indent)) {
                return -EINVAL;
        }

        header->e_indent = *(struct elf_indent *)code;

        if (ELF_ENDIAN_LE(header->e_indent)) {
                /*
                 * For the first few bytes, both 32-bit and 64-bit ELF headers
                 * have the same field width. So, we simply use header_64 at
                 * first.
                 */
                header->e_type = le16toh(header_64->e_type);
                header->e_machine = le16toh(header_64->e_machine);
                header->e_version = le32toh(header_32->e_version);
                if (ELF_BITS_32(header->e_indent)) {
                        header->e_entry = le32toh(header_32->e_entry);
                        header->e_phoff = le32toh(header_32->e_phoff);
                        header->e_shoff = le32toh(header_32->e_shoff);
                        header->e_flags = le32toh(header_32->e_flags);
                        header->e_ehsize = le16toh(header_32->e_ehsize);
                        header->e_phentsize = le16toh(header_32->e_phentsize);
                        header->e_phnum = le16toh(header_32->e_phnum);
                        header->e_shentsize = le16toh(header_32->e_shentsize);
                        header->e_shnum = le16toh(header_32->e_shnum);
                        header->e_shstrndx = le16toh(header_32->e_shstrndx);
                } else if (ELF_BITS_64(header->e_indent)) {
                        header->e_entry = le64toh(header_64->e_entry);
                        header->e_phoff = le64toh(header_64->e_phoff);
                        header->e_shoff = le64toh(header_64->e_shoff);
                        header->e_flags = le32toh(header_64->e_flags);
                        header->e_ehsize = le16toh(header_64->e_ehsize);
                        header->e_phentsize = le16toh(header_64->e_phentsize);
                        header->e_phnum = le16toh(header_64->e_phnum);
                        header->e_shentsize = le16toh(header_64->e_shentsize);
                        header->e_shnum = le16toh(header_64->e_shnum);
                        header->e_shstrndx = le16toh(header_64->e_shstrndx);
                } else {
                        return -EINVAL;
                }
        } else if (ELF_ENDIAN_BE(header->e_indent)) {
                /*
                 * We use header_64 for the same reason as above.
                 */
                header->e_type = be16toh(header_64->e_type);
                header->e_machine = be16toh(header_64->e_machine);
                header->e_version = be32toh(header_32->e_version);
                if (ELF_BITS_32(header->e_indent)) {
                        header->e_entry = be32toh(header_32->e_entry);
                        header->e_phoff = be32toh(header_32->e_phoff);
                        header->e_shoff = be32toh(header_32->e_shoff);
                        header->e_flags = be32toh(header_32->e_flags);
                        header->e_ehsize = be16toh(header_32->e_ehsize);
                        header->e_phentsize = be16toh(header_32->e_phentsize);
                        header->e_phnum = be16toh(header_32->e_phnum);
                        header->e_shentsize = be16toh(header_32->e_shentsize);
                        header->e_shnum = be16toh(header_32->e_shnum);
                        header->e_shstrndx = be16toh(header_32->e_shstrndx);
                } else if (ELF_BITS_64(header->e_indent)) {
                        header->e_entry = be64toh(header_64->e_entry);
                        header->e_phoff = be64toh(header_64->e_phoff);
                        header->e_shoff = be64toh(header_64->e_shoff);
                        header->e_flags = be32toh(header_64->e_flags);
                        header->e_ehsize = be16toh(header_64->e_ehsize);
                        header->e_phentsize = be16toh(header_64->e_phentsize);
                        header->e_phnum = be16toh(header_64->e_phnum);
                        header->e_shentsize = be16toh(header_64->e_shentsize);
                        header->e_shnum = be16toh(header_64->e_shnum);
                        header->e_shstrndx = be16toh(header_64->e_shstrndx);
                } else {
                        return -EINVAL;
                }
        } else {
                return -EINVAL;
        }
        return 0;
}

/**
 * Parse an ELF program header. We use the 64-bit structure
 * `struct elf_program_header` as the output structure.
 *
 * On error, the negative error code is returned.
 * On success, 0 is returned, and the header is written in the given parameter.
 */
static int parse_elf_program_header(const char *code,
                                    const struct elf_header *elf_header,
                                    struct elf_program_header *header)
{
        struct elf_program_header *header_64;
        struct elf_program_header_32 *header_32;

        if (ELF_ENDIAN_LE(elf_header->e_indent)) {
                if (ELF_BITS_32(elf_header->e_indent)) {
                        header_32 = (struct elf_program_header_32 *)code;
                        header->p_type = le32toh(header_32->p_type);
                        header->p_flags = le32toh(header_32->p_flags);
                        header->p_offset = le32toh(header_32->p_offset);
                        header->p_vaddr = le32toh(header_32->p_vaddr);
                        header->p_paddr = le32toh(header_32->p_paddr);
                        header->p_filesz = le32toh(header_32->p_filesz);
                        header->p_memsz = le32toh(header_32->p_memsz);
                        header->p_align = le32toh(header_32->p_align);
                } else if (ELF_BITS_64(elf_header->e_indent)) {
                        header_64 = (struct elf_program_header *)code;
                        header->p_type = le32toh(header_64->p_type);
                        header->p_flags = le32toh(header_64->p_flags);
                        header->p_offset = le64toh(header_64->p_offset);
                        header->p_vaddr = le64toh(header_64->p_vaddr);
                        header->p_paddr = le64toh(header_64->p_paddr);
                        header->p_filesz = le64toh(header_64->p_filesz);
                        header->p_memsz = le64toh(header_64->p_memsz);
                        header->p_align = le64toh(header_64->p_align);
                } else {
                        return -EINVAL;
                }
        } else if (ELF_ENDIAN_BE(elf_header->e_indent)) {
                if (ELF_BITS_32(elf_header->e_indent)) {
                        header_32 = (struct elf_program_header_32 *)code;
                        header->p_type = be32toh(header_32->p_type);
                        header->p_flags = be32toh(header_32->p_flags);
                        header->p_offset = be32toh(header_32->p_offset);
                        header->p_vaddr = be32toh(header_32->p_vaddr);
                        header->p_paddr = be32toh(header_32->p_paddr);
                        header->p_filesz = be32toh(header_32->p_filesz);
                        header->p_memsz = be32toh(header_32->p_memsz);
                        header->p_align = be32toh(header_32->p_align);
                } else if (ELF_BITS_64(elf_header->e_indent)) {
                        header_64 = (struct elf_program_header *)code;
                        header->p_type = be32toh(header_64->p_type);
                        header->p_flags = be32toh(header_64->p_flags);
                        header->p_offset = be64toh(header_64->p_offset);
                        header->p_vaddr = be64toh(header_64->p_vaddr);
                        header->p_paddr = be64toh(header_64->p_paddr);
                        header->p_filesz = be64toh(header_64->p_filesz);
                        header->p_memsz = be64toh(header_64->p_memsz);
                        header->p_align = be64toh(header_64->p_align);
                } else {
                        return -EINVAL;
                }
        } else {
                return -EINVAL;
        }
        return 0;
}

/**
 * Parse an ELF section header. We use the 64-bit structure
 * `struct elf_section_header` as the output structure.
 *
 * On error, the negative error code is returned.
 * On success, 0 is returned, and the header is written in the given parameter.
 */
static int parse_elf_section_header(const char *code,
                                    const struct elf_header *elf_header,
                                    struct elf_section_header *header)
{
        struct elf_section_header *header_64;
        struct elf_section_header_32 *header_32;

        if (ELF_ENDIAN_LE(elf_header->e_indent)) {
                if (ELF_BITS_32(elf_header->e_indent)) {
                        header_32 = (struct elf_section_header_32 *)code;
                        header->sh_name = le32toh(header_32->sh_name);
                        header->sh_type = le32toh(header_32->sh_type);
                        header->sh_flags = le32toh(header_32->sh_flags);
                        header->sh_addr = le32toh(header_32->sh_addr);
                        header->sh_offset = le32toh(header_32->sh_offset);
                        header->sh_size = le32toh(header_32->sh_size);
                        header->sh_link = le32toh(header_32->sh_link);
                        header->sh_info = le32toh(header_32->sh_info);
                        header->sh_addralign = le32toh(header_32->sh_addralign);
                        header->sh_entsize = le32toh(header_32->sh_entsize);
                } else if (ELF_BITS_64(elf_header->e_indent)) {
                        header_64 = (struct elf_section_header *)code;
                        header->sh_name = le32toh(header_64->sh_name);
                        header->sh_type = le32toh(header_64->sh_type);
                        header->sh_flags = le64toh(header_64->sh_flags);
                        header->sh_addr = le64toh(header_64->sh_addr);
                        header->sh_offset = le64toh(header_64->sh_offset);
                        header->sh_size = le64toh(header_64->sh_size);
                        header->sh_link = le32toh(header_64->sh_link);
                        header->sh_info = le32toh(header_64->sh_info);
                        header->sh_addralign = le64toh(header_64->sh_addralign);
                        header->sh_entsize = le64toh(header_64->sh_entsize);
                } else {
                        return -EINVAL;
                }
        } else if (ELF_ENDIAN_BE(elf_header->e_indent)) {
                if (ELF_BITS_32(elf_header->e_indent)) {
                        header_32 = (struct elf_section_header_32 *)code;
                        header->sh_name = be32toh(header_32->sh_name);
                        header->sh_type = be32toh(header_32->sh_type);
                        header->sh_flags = be32toh(header_32->sh_flags);
                        header->sh_addr = be32toh(header_32->sh_addr);
                        header->sh_offset = be32toh(header_32->sh_offset);
                        header->sh_size = be32toh(header_32->sh_size);
                        header->sh_link = be32toh(header_32->sh_link);
                        header->sh_info = be32toh(header_32->sh_info);
                        header->sh_addralign = be32toh(header_32->sh_addralign);
                        header->sh_entsize = be32toh(header_32->sh_entsize);
                } else if (ELF_BITS_64(elf_header->e_indent)) {
                        header_64 = (struct elf_section_header *)code;
                        header->sh_name = be32toh(header_64->sh_name);
                        header->sh_type = be32toh(header_64->sh_type);
                        header->sh_flags = be64toh(header_64->sh_flags);
                        header->sh_addr = be64toh(header_64->sh_addr);
                        header->sh_offset = be64toh(header_64->sh_offset);
                        header->sh_size = be64toh(header_64->sh_size);
                        header->sh_link = be32toh(header_64->sh_link);
                        header->sh_info = be32toh(header_64->sh_info);
                        header->sh_addralign = be64toh(header_64->sh_addralign);
                        header->sh_entsize = be64toh(header_64->sh_entsize);
                } else {
                        return -EINVAL;
                }
        } else {
                return -EINVAL;
        }
        return 0;
}

static void elf_free(struct elf_file *elf)
{
        if (elf->s_headers)
                free(elf->s_headers);
        if (elf->p_headers)
                free(elf->p_headers);
        free(elf);
}

static struct user_elf *new_user_elf(void)
{
        struct user_elf *elf;

        elf = malloc(sizeof(*elf));
        if (!elf)
                return NULL;
        memset(elf, 0, sizeof(*elf));
        return elf;
}

void free_user_elf(struct user_elf *user_elf)
{
        int segs = user_elf->segs_nr;
        if (segs > 0) {
                for (int i = 0; i < segs; i++) {
                        if (user_elf->user_elf_segs[i].elf_pmo > 0) {
                                usys_revoke_cap(
                                        user_elf->user_elf_segs[i].elf_pmo,
                                        false);
                        }
                }
                free(user_elf->user_elf_segs);
        }
        free(user_elf);
}

/**
 * @brief Treat a file as a randomly accessiable sequence of bytes, i.e. a
 * "range", by combining usage of read() and lseek()
 *
 * @param param [In] fd of the file to be read
 */
static int file_range_loader(void *param, off_t offset, size_t len, char *buf)
{
        int fd = (int)(long)param;
        ssize_t ret;
        if (lseek(fd, offset, SEEK_SET) < 0) {
                return -errno;
        }

        if ((ret = read(fd, buf, len)) < 0) {
                return -errno;
        }

        /**
         * We are accessing a file, so the read() call is expected to read all
         * required data. If it doesn't, it's an error.
         */
        if (ret != len) {
                return -EINVAL;
        }

        return 0;
}

static int memory_range_loader(void *param, off_t offset, size_t len, char *buf)
{
        memcpy(buf, (char *)param + offset, len);
        return 0;
}

#define MALLOC_OR_FAIL(ptr, size)       \
        do {                            \
                (ptr) = malloc(size);   \
                if (!(ptr)) {           \
                        ret = -ENOMEM;  \
                        goto out_nomem; \
                }                       \
        } while (0)

/**
 * @brief Load a small heading part of the range, and try to parse it into
 * an elf_header struct.
 *
 * @param loader [In] abstract range loader function
 * @param param [In] custom parameter for @loader
 * @param elf_header [Out] returning pointer to newly loaded header if
 * successfully load and parse its content. The ownership of this pointer is
 * transfered to the caller, so the caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
static int __load_elf_header(range_loader_t loader, void *param,
                             struct elf_header **elf_header)
{
        int ret;
        char buf[ELF_HEADER_SIZE];
        struct elf_header *header;

        MALLOC_OR_FAIL(header, sizeof(struct elf_header));

        ret = loader(param, 0, sizeof(struct elf_header), buf);
        if (ret < 0) {
                goto out_fail;
        }

        ret = parse_elf_header(buf, header);
        if (ret < 0) {
                goto out_fail;
        }

        *elf_header = header;
        return 0;
out_fail:
        free(header);
out_nomem:
        return ret;
}

/**
 * @brief Load program headers and section headers from the range,
 * and try to parse it into an elf_file struct.
 *
 * @param elf_header [In] this function only borrow the pointer, and will not
 * free it. It's the caller's responsibility to control the life cycle of this
 * pointer.
 * @param loader [In] abstract range loader function
 * @param param [In] custom parameter for @loader
 * @param elf_header [Out] returning pointer to newly loaded header if
 * successfully load and parse its content. The ownership of this pointer is
 * transfered to the caller, so the caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
static int __load_elf_ph_sh(struct elf_header *header, range_loader_t loader,
                            void *param, struct elf_file **elf_file)
{
        int ret = 0;
        struct elf_program_header *ph_buf = NULL;
        struct elf_section_header *sh_buf = NULL;
        struct elf_file *elf = NULL;
        char *ph_file_buf = NULL;
        char *sh_file_buf = NULL;

        MALLOC_OR_FAIL(ph_buf,
                       header->e_phnum * sizeof(struct elf_program_header));
        MALLOC_OR_FAIL(sh_buf,
                       header->e_shnum * sizeof(struct elf_section_header));
        MALLOC_OR_FAIL(elf, sizeof(struct elf_file));
        MALLOC_OR_FAIL(ph_file_buf,
                       header->e_phnum * (size_t)header->e_phentsize);
        MALLOC_OR_FAIL(sh_file_buf,
                       header->e_shnum * (size_t)header->e_shentsize);

        /**
         * ph_file_buf would be an array of e_phnum elements, size of each
         * element is e_phentsize if success. Each element is a program header
         * in the ELF file.
         */
        ret = loader(param,
                     header->e_phoff,
                     header->e_phnum * (size_t)header->e_phentsize,
                     ph_file_buf);
        if (ret < 0) {
                goto out_fail;
        }

        for (int i = 0; i < header->e_phnum; i++) {
                ret = parse_elf_program_header(
                        ph_file_buf + (ptrdiff_t)header->e_phentsize * i,
                        header,
                        &ph_buf[i]);
                if (ret < 0) {
                        goto out_fail;
                }
        }

        /**
         * sh_file_buf would be an array of e_shnum elements, size of each
         * element is e_shentsize if success. Each element is a section header
         * in the ELF file.
         */
        ret = loader(param,
                     header->e_shoff,
                     header->e_shnum * (size_t)header->e_shentsize,
                     sh_file_buf);
        if (ret < 0) {
                goto out_fail;
        }

        for (int i = 0; i < header->e_shnum; i++) {
                ret = parse_elf_section_header(
                        sh_file_buf + (ptrdiff_t)header->e_shentsize * i,
                        header,
                        &sh_buf[i]);
                if (ret < 0) {
                        goto out_fail;
                }
        }

        elf->p_headers = ph_buf;
        elf->s_headers = sh_buf;
        elf->header = *header;

        free(ph_file_buf);
        free(sh_file_buf);

        *elf_file = elf;
        return 0;
out_fail:
out_nomem:
        if (ph_buf) {
                free(ph_buf);
        }
        if (sh_buf) {
                free(sh_buf);
        }
        if (elf) {
                elf_free(elf);
        }
        if (ph_file_buf) {
                free(ph_file_buf);
        }
        if (sh_file_buf) {
                free(sh_file_buf);
        }
        return ret;
}

/**
 * @brief Translate the permission bits in p_flags into ChCore VMR
 * permissions.
 */
#define PFLAGS2VMRFLAGS(PF)                                     \
        (((PF)&PF_X ? VM_EXEC : 0) | ((PF)&PF_W ? VM_WRITE : 0) \
         | ((PF)&PF_R ? VM_READ : 0))

#define OFFSET_MASK 0xfff

/**
 * @brief Load each loadable (PT_LOAD) segment in elf_file into a ChCore
 * pmo from the range. Content of each segment is stored in memory backed
 * by its corresponding pmo. Those pmos, stored in user_elf->user_elf_segs,
 * can be mapped into the address space of a process later, but this library
 * is only used for loading ELF file content, not constructing ELF memory
 * image in the caller's address space, so we don't do that here.
 *
 * @param elf_file [In] this function only borrow the pointer, and will not
 * free it. It's the caller's responsibility to control the life cycle of this
 * pointer.
 * @param loader [In] abstract range loader function
 * @param param [In] custom parameter for @loader
 * @param user_elf [Out] returning pointer to newly loaded header if
 * successfully load and parse its content. The ownership of this pointer is
 * transfered to the caller, so the caller should free it after use.
 * @return 0 if success, otherwise -errno is returned. All memory resources
 * consumed by this function are guaranteed to be freed if not success.
 */
static int __load_elf_into_pmos(struct elf_file *elf_file,
                                range_loader_t loader, void *param,
                                struct user_elf **user_elf)
{
        int ret = 0;
        int loadable_segs = 0;
        struct user_elf *elf = NULL;
        struct elf_program_header *cur_ph;
        struct user_elf_seg *cur_user_elf_seg;
        size_t seg_sz, seg_map_sz, seg_file_sz;
        u64 p_vaddr;
	unsigned long phdr_addr = 0;
        cap_t seg_pmo;
        char *seg_buf, *seg_load_start;

        /**
         * Calculate the number of loadable segments. We only need to
         * load those segments.
         */
        for (int i = 0; i < elf_file->header.e_phnum; i++) {
                if (elf_file->p_headers[i].p_type == PT_LOAD) {
                        loadable_segs++;
                }
        }

        elf = new_user_elf();
        if (!elf) {
                ret = -ENOMEM;
                goto out_nomem;
        }

        elf->segs_nr = loadable_segs;
        MALLOC_OR_FAIL(elf->user_elf_segs,
                       loadable_segs * sizeof(struct user_elf_seg));
        memset(elf->user_elf_segs,
               0,
               loadable_segs * sizeof(struct user_elf_seg));

        /**
         * Loop over all segments of elf_file(controlled by i), find and load
         * each loadable segment(controlled by j), and fill its user_elf_seg
         * structure.
         */
        for (int i = 0, j = 0; i < elf_file->header.e_phnum; i++) {
                cur_ph = &elf_file->p_headers[i];
                cur_user_elf_seg = &elf->user_elf_segs[j];
                if (cur_ph->p_type != PT_LOAD && cur_ph->p_type != PT_PHDR) {
                        continue;
                }

		if (cur_ph->p_type == PT_PHDR) {
			phdr_addr = cur_ph->p_vaddr;
			continue;
		}

                seg_sz = cur_ph->p_memsz;
                p_vaddr = cur_ph->p_vaddr;
                seg_file_sz = cur_ph->p_filesz;

                /** seg_sz and p_vaddr may not page aligned */
                seg_map_sz = ROUND_UP(seg_sz + p_vaddr, PAGE_SIZE)
                             - ROUND_DOWN(p_vaddr, PAGE_SIZE);

                ret = usys_create_pmo(seg_map_sz, PMO_ANONYM);
                if (ret < 0) {
                        goto out_fail;
                }

                seg_pmo = ret;
                seg_buf = chcore_auto_map_pmo(
                        seg_pmo, seg_map_sz, VMR_READ | VMR_WRITE);
                if (!seg_buf) {
                        ret = -errno;
                        usys_revoke_cap(seg_pmo, false);
                        goto out_fail;
                }

                /*
                 * OFFSET_MASK is for calculating the final offset for loading
                 * different segments from ELF.
                 * ELF segment can specify not aligned address.
                 */
                seg_load_start = seg_buf + (p_vaddr & OFFSET_MASK);

                /**
                 * With former chcore_auto_map_pmo, we directly load content of
                 * the range into the pmo, without an intermediate buffer.
                 */
                ret = loader(
                        param, cur_ph->p_offset, seg_file_sz, seg_load_start);
                if (ret < 0) {
                        chcore_auto_unmap_pmo(
                                seg_pmo, (vaddr_t)seg_buf, seg_map_sz);
                        usys_revoke_cap(seg_pmo, false);
                        goto out_fail;
                }

                cur_user_elf_seg->elf_pmo = seg_pmo;
                cur_user_elf_seg->seg_sz = seg_sz;
                cur_user_elf_seg->p_vaddr = p_vaddr;
                cur_user_elf_seg->perm = PFLAGS2VMRFLAGS(cur_ph->p_flags);

                if (cur_user_elf_seg->perm & VMR_EXEC) {
                        usys_cache_flush((unsigned long)seg_load_start,
                                         seg_file_sz,
                                         SYNC_IDCACHE);
                }

                chcore_auto_unmap_pmo(seg_pmo, (vaddr_t)seg_buf, seg_map_sz);

                j++;
        }

        /**
         * Calculation of AT_PHDR address is a little unintuitive. It seems that
         * we have to use a separate pmo to store and map the program header
         * table. But actually, the PT_LOAD segments have already stored all
         * data required in runtime of the ELF file. Especially, the first
         * PT_LOAD segment would contains the ELF header and program headers of
         * the ELF file, i.e., the p_offset of this segment would be 0. So the
         * virtual address of this segment is the position of the start of the
         * ELF file in the memory, and the address of program header table can
         * be calculated by adding this virtual address with phoff from the ELF
         * header.
         */
        elf->elf_meta.phdr_addr = phdr_addr ? phdr_addr : elf_file->p_headers[0].p_vaddr + elf_file->header.e_phoff;
        elf->elf_meta.phentsize = elf_file->header.e_phentsize;
        elf->elf_meta.phnum = elf_file->header.e_phnum;
        elf->elf_meta.flags = elf_file->header.e_flags;
        elf->elf_meta.entry = elf_file->header.e_entry;
        elf->elf_meta.type = elf_file->header.e_type;

        *user_elf = elf;

        return 0;
out_fail:
out_nomem:
        if (elf) {
                free_user_elf(elf);
        }
        return ret;
}

int load_elf_header_from_fs(const char *path, struct elf_header **elf_header)
{
        int ret = 0, fd;
        ret = open(path, O_RDONLY);

        if (ret < 0) {
                return -errno;
        }

        fd = ret;
        ret = __load_elf_header(
                file_range_loader, (void *)(long)fd, elf_header);

        close(fd);

        return ret;
}

int load_elf_by_header_from_fs(const char *path, struct elf_header *elf_header,
                               struct user_elf **elf)
{
        int ret = 0, fd;
        struct elf_file *elf_file;
        ret = open(path, O_RDONLY);

        if (ret < 0) {
                ret = -errno;
                goto out;
        }

        fd = ret;
        ret = __load_elf_ph_sh(
                elf_header, file_range_loader, (void *)(long)fd, &elf_file);
        if (ret < 0) {
                goto out_ph_sh_failed;
        }

        ret = __load_elf_into_pmos(
                elf_file, file_range_loader, (void *)(long)fd, elf);

        if (ret == 0) {
                strncpy((*elf)->path, path, ELF_PATH_LEN);
        }

        elf_free(elf_file);
out_ph_sh_failed:
        close(fd);
out:
        return ret;
}

/**
 * @brief This function illustrates the steps required to load an ELF file
 * from a large, abstract "range". A range is a sequence of bytes, and can
 * be accessed randomly. For example, a memory buffer can be regarded as a
 * "range", and a file, combining read() and lseek(), is also a "range" of
 * course.
 *
 * Random access to the range is abstracted through @loader parameter. All
 * internal operations also use it to perform abstracted random access. So
 * the caller just need to implement a specific loader to load ELF file
 * content from a new source.
 *
 * To load an ELF file, we have to perform 3 steps.
 * Step-1: Load header of the ELF file. From the header, we can determine
 * the ELF is 32-bit or 64-bit, the offset of program headers and section
 * headers.
 * Step-2: Load program headers and section headers. Program headers indicates
 * the memory image of the ELF file. We should use those headers to load
 * remaining ELF content into memory.
 * Step-3: Load every loadable(PT_LOAD) segment into memory. A PT_LOAD segment
 * indicaties a range in the ELF file which should be loaded into a specific
 * virtual address range. However, this function do not map each segment into
 * virtual address range they specified. See comments of __load_elf_into_pmos
 * for more details.
 *
 * @param loader [In] abstracted range random access function
 * @param param [In] custom parameter which would be passed to @loader. It can
 * be used to store information about the start of the range.
 * @param elf [Out] returning pointer to newly loaded user_elf struct if
 * success. The ownership of this pointer is transfered to the caller, so the
 * caller should free it after use.
 * @return 0 if success, otherwise -errno is returned.
 */
static int load_elf_from_range(range_loader_t loader, void *param,
                               struct user_elf **elf)
{
        int ret = 0;
        struct elf_header *elf_header;
        struct elf_file *elf_file;

        ret = __load_elf_header(loader, param, &elf_header);
        if (ret < 0) {
                goto out;
        }

        ret = __load_elf_ph_sh(elf_header, loader, param, &elf_file);
        if (ret < 0) {
                goto out_ph_sh_failed;
        }

        ret = __load_elf_into_pmos(elf_file, loader, param, elf);

        elf_free(elf_file);
out_ph_sh_failed:
        free(elf_header);
out:
        return ret;
}

int load_elf_from_fs(const char *path, struct user_elf **elf)
{
        int ret = 0, fd;
        ret = open(path, O_RDONLY);
        if (ret < 0) {
                return -errno;
        }

        fd = ret;

        ret = load_elf_from_range(file_range_loader, (void *)(long)fd, elf);

        if (ret == 0) {
                strncpy((*elf)->path, path, ELF_PATH_LEN);
        }

        close(fd);

        return ret;
}

int load_elf_from_mem(const char *code, struct user_elf **elf)
{
        return load_elf_from_range(memory_range_loader, (void *)code, elf);
}