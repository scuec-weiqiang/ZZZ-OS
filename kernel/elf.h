/**
 * @FilePath: /ZZZ/kernel/elf.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:36
 * @LastEditTime: 2025-09-14 13:41:18
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/**
 * @FilePath: /ZZZ/kernel/elf.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:36
 * @LastEditTime: 2025-09-04 15:59:29
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef ELF_H
#define ELF_H
#include "types.h"

#define PT_LOAD 1

typedef struct 
{
    uint64_t vaddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t offset;
    uint32_t flags;
}elf_segment_t;
  
typedef struct 
{
    uint64_t entry;
    uint16_t phnum;
    elf_segment_t segs[8];
} elf_info_t;

typedef struct 
{
  uint8_t  e_ident[16];
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
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct 
{
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

extern int64_t elf_prase(const uint8_t *elf, elf_info_t *info);
#endif