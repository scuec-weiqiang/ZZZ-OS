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

struct file;

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
    u32 type;
    u64 vaddr;
    u64 filesz;
    u64 memsz;
    u64 offset;
    u32 flags;
};

struct elf_info
{
    u8 elf_class;
    u16 machine;
    uintptr_t entry;
    u16 phnum;
    size_t file_size;
    struct elf_segment *segs;
};

struct __attribute__((packed)) Elf32_Ehdr
{
  char     e_ident[EI_NIDENT];
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

struct __attribute__((packed)) Elf32_Phdr
{
  u32 p_type;
  u32 p_offset;
  u32 p_vaddr;
  u32 p_paddr;
  u32 p_filesz;
  u32 p_memsz;
  u32 p_flags;
  u32 p_align;
};


struct __attribute__((packed)) Elf64_Ehdr
{
  char e_ident[EI_NIDENT];
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
  u16 e_shnum; // 节头表项数量
  u16 e_shstrndx;
};

struct __attribute__((packed)) Elf64_Phdr 
{
  u32 p_type;
  u32 p_flags;
  u64 p_offset;
  u64 p_vaddr;
  u64 p_paddr;
  u64 p_filesz;
  u64 p_memsz;
  u64 p_align;
} ;

extern struct elf_info *elf_parse_file(struct file *file);
extern void elf_free(struct elf_info *info);
extern int arch_elf_check(const struct elf_info *info);
#endif
