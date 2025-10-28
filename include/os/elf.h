/**
 * @FilePath: /vboot/elf.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:36
 * @LastEditTime: 2025-09-17 23:42:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef ELF_H
#define ELF_H

#include "types.h"

#define PT_LOAD 1

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
    // char name[64];
    char* data;
    uint64_t entry;
    uint16_t phnum;
    struct elf_segment segs[8];
};

struct __attribute__((packed)) Elf64_Ehdr
{
  char  e_ident[16];
  uint16_t e_type; // 0x01 可重定位文件 0x02 可执行文件
  uint16_t e_machine;// 0xf3 RISC-V
  uint32_t e_version;
  uint64_t e_entry;// 程序入口地址
  uint64_t e_phoff; // 程序头表在文件中的偏移
  uint64_t e_shoff; // 节头表在文件中的偏移
  uint32_t e_flags; // ELF文件标志
  uint16_t e_ehsize; // ELF头大小
  uint16_t e_phentsize; // 程序头表项大小
  uint16_t e_phnum; // 程序头表项数量
  uint16_t e_shentsize; // 节头表项大小
  uint16_t e_shnum; // 节头表项数量
  uint16_t e_shstrndx;
};

struct __attribute__((packed)) Elf64_Phdr 
{
  uint32_t p_type; // 1: 可加载程序段 2: 动态链接信息 3: 只读动态链接信息 4: 栈可读写 5: 栈可读写执行
  uint32_t p_flags;
  uint64_t p_offset;// 段在文件中的偏移
  uint64_t p_vaddr;// 段在内存中的虚拟地址
  uint64_t p_paddr;// 段在物理内存中的地址（一般不用，除非裸机）
  uint64_t p_filesz;// 段在文件中的大小
  uint64_t p_memsz;// 段在内存中的大小
  uint64_t p_align;// 段的对齐方式
} ;

extern struct elf_info* elf_parse(const char *elf);
#endif