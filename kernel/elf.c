/**
 * @FilePath: /ZZZ/kernel/elf.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-07-21 23:53:30
 * @LastEditTime: 2025-09-07 20:08:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "printf.h"
#include "elf.h"
// #include "ext2.h"
#include "page_alloc.h"
#include "check.h"

static int64_t elf_check(const uint8_t *elf)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf;
    CHECK(elf != NULL, "elf is NULL", return -1;);
    CHECK(ehdr != NULL, "ehdr is NULL", return -1;);
    CHECK(ehdr->e_phoff != 0, "e_phoff is 0", return -1;);
    CHECK(ehdr->e_phnum != 0, "e_phnum is 0", return -1;);
    CHECK(ehdr->e_phentsize != 0, "e_phentsize is 0", return -1;);
    CHECK(ehdr->e_entry != 0, "e_entry is 0", return -1;);

    CHECK(ehdr->e_ident[0] == 0x7f && ehdr->e_ident[1] == 'E' && ehdr->e_ident[2] == 'L' && ehdr->e_ident[3] == 'F', "Not a valid ELF file", return -1;);
    CHECK(ehdr->e_ident[4] == 2, "Not a 64-bit ELF file", return -1;);
    CHECK(ehdr->e_ident[5] == 1, "Not a little-endian ELF file", return -1;);
    CHECK(ehdr->e_type == 2, "Not an executable ELF file", return -1;);
    CHECK(ehdr->e_machine == 0xf3, "Not a RISC-V ELF file", return -1;);

    return 0; // Valid ELF file
}

elf_info_t* elf_parse(const uint8_t *elf)
{
    CHECK(elf != NULL, "elf is NULL", return NULL;);
    
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf;
    
    CHECK(elf_check(elf) == 0, "Invalid ELF file", return NULL;);

    elf_info_t *info = malloc(sizeof(elf_info_t));
    CHECK(info != NULL, "Failed to allocate memory for ELF info", return NULL;);
    memset(info, 0, sizeof(elf_info_t));
    info->data = (char *)elf;

    Elf64_Phdr *phdr = (Elf64_Phdr *)(elf + ehdr->e_phoff);
    info->entry = ehdr->e_entry;
    info->phnum = ehdr->e_phnum;
    printf("ELF entry point: %xu, program header count: %du\n", info->entry, info->phnum);

    for (int i = 0; i < ehdr->e_phnum; i++) 
    {
        // if (phdr[i].p_type == PT_LOAD) 
        // {
            info->segs[i].type = phdr[i].p_type;
            info->segs[i].vaddr = phdr[i].p_vaddr;
            info->segs[i].filesz = phdr[i].p_filesz;
            info->segs[i].memsz = phdr[i].p_memsz;
            info->segs[i].offset = phdr[i].p_offset;
            info->segs[i].flags = phdr[i].p_flags;
            printf("Segment %d: vaddr=%xu, filesz=%xu, memsz=%xu, offset=%xu, flags=%x\n", 
                   i, info->segs[i].vaddr, info->segs[i].filesz, info->segs[i].memsz, 
                   info->segs[i].offset, info->segs[i].flags);
        // }
    }

    return info; // Success
}



