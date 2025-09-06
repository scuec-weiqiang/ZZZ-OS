/**
 * @FilePath: /ZZZ/kernel/elf.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-07-06 00:52:45
 * @LastEditTime: 2025-08-22 20:22:04
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "printf.h"
#include "elf.h"
// #include "ext2.h"
#include "page_alloc.h"




int64_t elf_prase(const uint8_t *elf, elf_info_t *info)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf;
    Elf64_Phdr *phdr = (Elf64_Phdr *)(elf + ehdr->e_phoff);
    
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' || ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') 
    {
        return -1; // Not a valid ELF file
    }
    
    info->entry = ehdr->e_entry;
    info->phnum = ehdr->e_phnum;
    printf("ELF entry point: %xu, program header count: %du\n", info->entry, info->phnum);

    for (int i = 0; i < ehdr->e_phnum; i++) 
    {
        if (phdr[i].p_type == PT_LOAD) 
        {
            info->segs[i].vaddr = phdr[i].p_vaddr;
            info->segs[i].filesz = phdr[i].p_filesz;
            info->segs[i].memsz = phdr[i].p_memsz;
            info->segs[i].offset = phdr[i].p_offset;
            info->segs[i].flags = phdr[i].p_flags;
            printf("Segment %d: vaddr=%xu, filesz=%xu, memsz=%xu, offset=%xu, flags=%x\n", 
                   i, info->segs[i].vaddr, info->segs[i].filesz, info->segs[i].memsz, 
                   info->segs[i].offset, info->segs[i].flags);
        }
    }

    return 0; // Success
}




