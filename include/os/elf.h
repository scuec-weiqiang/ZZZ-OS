/**
 * @FilePath: /vboot/os/elf.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:36
 * @LastEditTime: 2025-09-17 23:42:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef ELF_H
#define ELF_H

#include <os/types.h>

#define EI_NIDENT 16

#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATA2LSB 1

#define ET_EXEC 2

#define EM_ARM     40
#define EM_AARCH64 183
#define EM_RISCV   243

#define PT_LOAD 1

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

struct elf_segment
{
    uint32_t type;
    uint64_t vaddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t offset;
    uint32_t flags;
};
  
struct elf_info
{
    char *data;
    size_t size;
    uint8_t elf_class;
    uint16_t machine;
    uint64_t entry;
    uint16_t phnum;
    struct elf_segment segs[8];
};

struct __attribute__((packed)) Elf32_Ehdr
{
  char     e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct __attribute__((packed)) Elf32_Phdr
{
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};

struct __attribute__((packed)) Elf64_Ehdr
{
  char e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum; // 节头表项数量
  uint16_t e_shstrndx;
};

struct __attribute__((packed)) Elf64_Phdr 
{
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} ;

extern struct elf_info *elf_parse_image(const char *elf, size_t len);
extern struct elf_info* elf_parse(const char *elf);
#endif
