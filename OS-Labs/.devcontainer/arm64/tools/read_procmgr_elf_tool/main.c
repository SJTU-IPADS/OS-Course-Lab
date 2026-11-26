#include "elf.h"
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)

#define be16_to_cpu(x) ((((x)&0xff) << 8) | (((x) >> 8) & 0xff))
#define be32_to_cpu(x) ((be16_to_cpu((x)) << 16) | (be16_to_cpu((x) >> 16)))
#define be64_to_cpu(x) ((be32_to_cpu((x)) << 32) | (be32_to_cpu((x) >> 32)))

#define be128ptr_to_cpu_hi(x) (be64_to_cpu(*(u64 *)(x)))
#define be128ptr_to_cpu_lo(x) (be64_to_cpu(*((u64 *)(x) + 1)))

#define be96ptr_to_cpu_hi(x) (be32_to_cpu(*(u32 *)(x)))
#define be96ptr_to_cpu_lo(x)                                                   \
  (((u64)(be32_to_cpu(*((u32 *)(x) + 1)))) << 32 |                             \
   (be32_to_cpu(*((u32 *)(x)) + 2)))

void get_elf_info(const char *binary, struct elf_info *info) {
  struct elf_file *elf;
  int i;
  u64 size = 0;
  u64 offset;

  elf = elf_parse_file(binary, info);
  elf->header.e_phentsize;
  if (!elf) {
    printf("parse elf fail\n");
    return;
  }
  for (i = 0; i < elf->header.e_phnum; ++i) {
    if (elf->p_headers[i].p_type != PT_LOAD)
      continue;
    size += elf->p_headers[i].p_memsz;
  }

  info->entry = elf->header.e_entry;
  info->flags = elf->header.e_flags;
  info->mem_size = size;
  info->phentsize = elf->header.e_phentsize;
  info->phnum = elf->header.e_phnum;
  info->phdr_addr = elf->p_headers[0].p_vaddr + elf->header.e_phoff;

  free(elf);
}

int main(int argc, char *argv[]) {
  int fd;
  struct stat st;
  char *buf;
  struct elf_info info;

  if (argc == 1) {
    printf("Need a path points to the procmgr.elf\n");
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("Can not open elf file!\n");
  }
  fstat(fd, &st);
  buf = malloc(st.st_size);
  read(fd, buf, st.st_size);
  get_elf_info(buf, &info);

  free(buf);
  close(fd);
  info.entry = be64_to_cpu(info.entry);
  info.flags = be64_to_cpu(info.flags);
  info.mem_size = be64_to_cpu(info.mem_size);
  info.phentsize = be64_to_cpu(info.phentsize);
  info.phnum = be64_to_cpu(info.phnum);
  info.phdr_addr = be64_to_cpu(info.phdr_addr);

  fd = open("./elf_info.temp", O_CREAT | O_RDWR);
  if (fd < 0) {
    printf("Create file failed!\n");
  }

  write(fd, (void *)&info, sizeof(struct elf_info));
  close(fd);

  return 0;
}