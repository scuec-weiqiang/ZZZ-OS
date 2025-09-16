/**
 * @FilePath: /ZZZ/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-15 19:58:47
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printf.h"
#include "page_alloc.h"
#include "uart.h"
// #include "sched.h"
// #include "systimer.h"
#include "vm.h"
#include "virt_disk.h"
#include "time.h"


#include "riscv.h"
#include "plic.h"
#include "clint.h"
#include "maddr_def.h"
#include "interrupt.h"
#include "vfs.h"
#include "string.h"
#include "elf.h"

uint8_t is_init = 0;

/**
 * @brief 将BSS段中的所有数据清零
 *
 * 遍历BSS段的起始地址到结束地址之间的所有字节，并将它们置为零。
 *
 * BSS段通常用于存储未初始化的全局变量和静态变量，它们在程序启动时不会自动初始化为零。
 * 本函数通过手动遍历并清零这些变量，确保它们在程序启动时是干净的。
 */
void zero_bss() {
    for (char *p = _bss_start; p < _bss_end; p++) {
        *p = 0;
    }
}

pgtbl_t* user_pgd;
void init_kernel()
{  
    hart_id_t hart_id = 0;
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        zero_bss();
        uart_init();
        page_alloc_init();
        page_get_remain_mem();
        kernel_page_table_init();
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        virt_disk_init(); 

        vfs_init();
        user_pgd = page_alloc(1);
        uint8_t *user_stack = malloc(PAGE_SIZE);
    
        // vfs_test();
        vfs_file_t* f = vfs_open("/user.elf",0);
        char* buf = malloc(f->f_inode->i_size);
        ssize_t ret =  vfs_read(f,buf,f->f_inode->i_size);
        Elf64_Ehdr *prog = (Elf64_Ehdr*)buf;
        printf("elf phnum:%d\n",prog->e_phnum);
        printf("elf phoff:%x\n",prog->e_phoff);
        for(int i=0;i<prog->e_phnum;i++)
        {
            Elf64_Phdr *phdr = (Elf64_Phdr *)(buf + prog->e_phoff + i*prog->e_phentsize);
            if(phdr->p_type == PT_LOAD)
            {
                printf("phdr %d: vaddr:%x, memsz:%x, filesz:%x, offset:%x, flags:%x\n",i,phdr->p_vaddr,phdr->p_memsz,phdr->p_filesz,phdr->p_offset,phdr->p_flags);
                uint8_t *user_space = malloc(phdr->p_memsz); //程序加载到内存里需要的空间
                memset(user_space,0,phdr->p_memsz);
                memcpy(user_space,buf+phdr->p_offset,phdr->p_filesz);
                map_pages(user_pgd, phdr->p_vaddr, (uintptr_t)user_space, (phdr->p_memsz+PAGE_SIZE-1)/PAGE_SIZE * PAGE_SIZE, PTE_R|PTE_X|PTE_U);
                map_pages(user_pgd, 0x20000, (uintptr_t)user_stack, PAGE_SIZE,  PTE_W|PTE_R|PTE_U);
                page_table_init(user_pgd);
                sstatus_w(sstatus_r() & ~(1<<8));  
                sepc_w((uintptr_t)(prog->e_entry)); 
                sscratch_w(sp_r());
                satp_w(MAKE_SATP(user_pgd));
                asm volatile("mv sp,%0"::"r"(0x20000+PAGE_SIZE-1));
                
                asm volatile("sret"); 
                // map_pages(user_pgd,user_stack,user_stack, PAGE_SIZE, PTE_R | PTE_W | PTE_U);
            }
        }
        
        printf("now time:%x\n",get_current_unix_timestamp(UTC8));
        page_get_remain_mem();
        // elf_info_t *info = malloc(sizeof(elf_info_t));
        // elf_prase(user_user_program_elf, info);
        // pgtbl_t *user_program_page  = page_alloc(1);
        // memset(user_program_page,0,PAGE_SIZE);
        // uint8_t *user_space = malloc(info->segs[1].filesz);
        // uint8_t *user_stack = malloc(PAGE_SIZE);
        // map_pages(user_program_page,user_stack,user_stack, PAGE_SIZE, PTE_R | PTE_W | PTE_U);
        // memcpy(user_space,user_user_program_elf+info->segs[1].offset,info->segs[1].filesz);
        // map_pages(user_program_page, info->segs[1].vaddr, (uint64_t)user_space, info->segs[1].filesz, PTE_R |PTE_W | PTE_X|PTE_U);
        // map_pages(user_program_page,_trap_start,_trap_start, _trap_size, PTE_R | PTE_X | PTE_U);
        // sstatus_w(sstatus_r()&~(1<<8)|(1<<5));
        // sepc_w(info->entry);
        // asm volatile("mv sp,%0"::"r"(user_stack+PAGE_SIZE-1));
        // asm volatile("sfence.vma zero, zero");
        // asm volatile("csrw satp,%0"::"r"(MAKE_SATP(user_program_page)));
        // asm volatile("sret"::);
        // task_init();
        is_init = 1;

    }
   
    // ext2_create_dir_by_path(fs, "/a/b/");
    printf("hart_id:%d\n", hart_id);
    while (is_init == 0){}
    // wakeup_other_harts();
 
    s_global_interrupt_enable(); 
    //每个核心初始化自己的资源
    // systimer_init(hart_id,SYS_HZ_100);
    // sched_init(hart_id);
    // __clint_send_ipi(0);
    // sip_w(sip_r() | 2);
   
    while(1)
    {
        // printf("hart_id:%d\n", hart_id++);
    }
    // global_interrupt_enable();
    // M_TO_U(os_main);
 }