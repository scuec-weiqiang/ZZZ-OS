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
    u32 type;
    u64 vaddr;
    u64 filesz;
    u64 memsz;
    u64 offset;
    u32 flags;
};
  
struct elf_info
{
    // char name[64];
    char* data;
    u64 entry;
    u16 phnum;
    struct elf_segment segs[8];
};

struct __attribute__((packed)) Elf64_Ehdr
{
  char  e_ident[16];
  u16 e_type; // 0x01 可重定位文件 0x02 可执行文件
  u16 e_machine;// 0xf3 RISC-V
  u32 e_version;
  u64 e_entry;// 程序入口地址
  u64 e_phoff; // 程序头表在文件中的偏移
  u64 e_shoff; // 节头表在文件中的偏移
  u32 e_flags; // ELF文件标志
  u16 e_ehsize; // ELF头大小
  u16 e_phentsize; // 程序头表项大小
  u16 e_phnum; // 程序头表项数量
  u16 e_shentsize; // 节头表项大小
  u16 e_shnum; // 节头表项数量
  u16 e_shstrndx;
};

struct __attribute__((packed)) Elf64_Phdr 
{
  u32 p_type; // 1: 可加载程序段 2: 动态链接信息 3: 只读动态链接信息 4: 栈可读写 5: 栈可读写执行
  u32 p_flags;
  u64 p_offset;// 段在文件中的偏移
  u64 p_vaddr;// 段在内存中的虚拟地址
  u64 p_paddr;// 段在物理内存中的地址（一般不用，除非裸机）
  u64 p_filesz;// 段在文件中的大小
  u64 p_memsz;// 段在内存中的大小
  u64 p_align;// 段的对齐方式
} ;

extern struct elf_info* elf_parse(const char *elf);
#endif