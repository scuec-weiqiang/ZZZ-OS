/**
 * @FilePath: /ZZZ/kernel/proc.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-16 18:23:57
 * @LastEditTime: 2025-09-21 17:10:12
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "proc.h"
#include "vfs.h"
#include "elf.h"
#include "vm.h"
#include "check.h"
#include "string.h"
#include "platform.h"
#include "malloc.h"

struct list proc_list_head[MAX_HARTS_NUM];
u64 proc_count[MAX_HARTS_NUM];

static pid_t alloc_pid()
{
    static pid_t next_pid = 1;
    return next_pid++;
}

void proc_init()
{
    for(enum hart_id hart_id = HART_0;hart_id < MAX_HARTS_NUM; hart_id++)
    {
        INIT_LIST_HEAD(&proc_list_head[hart_id])
        proc_count[hart_id] = 0;
    }

}

struct proc* proc_create(char* path)
{
    CHECK(path != NULL, "proc create failed,path is NULL", return NULL;);

    struct file* f = open(path,0);
    char* elf = malloc(f->f_inode->i_size);
    ssize_t ret =  read(f,elf,f->f_inode->i_size);
    CHECK(ret >= 0, "vfs read failed", free(elf); return NULL;);

    struct elf_info *elf_info = elf_parse(elf);
    CHECK(elf_info != NULL, "elf parse failed", free(elf); return NULL;);

    struct proc* new_proc = (struct proc*)malloc(sizeof(struct proc));
    memset(new_proc,0,sizeof(struct proc));
    
    pgtbl_t* user_pgd = page_alloc(1);
    memset(user_pgd,0,sizeof(pgtbl_t));

    char *user_stack = (char*)malloc(PROC_STACK_SIZE);
    memset(user_stack,0,PROC_STACK_SIZE);

    char *kernel_stack = (char*)malloc(PROC_STACK_SIZE);
    memset(kernel_stack,0,PROC_STACK_SIZE);

    for(int i=0;i<elf_info->phnum;i++)
    {
        if(elf_info->segs[i].type== PT_LOAD)
        {
            printk("phdr %d: vaddr:%x, memsz:%x, filesz:%x, offset:%x, flags:%x\n",i,elf_info->segs[i].vaddr,elf_info->segs[i].memsz,elf_info->segs[i].filesz,elf_info->segs[i].offset,elf_info->segs[i].flags);
            char *user_space = (char*)malloc(elf_info->segs[i].memsz); //程序加载到内存里需要的空间
            memset(user_space,0,elf_info->segs[i].memsz);
            memcpy(user_space,elf+elf_info->segs[i].offset,elf_info->segs[i].filesz);
            map_range(user_pgd, elf_info->segs[i].vaddr, (uintptr_t)user_space, (elf_info->segs[i].memsz+PAGE_SIZE-1)/PAGE_SIZE * PAGE_SIZE, PTE_R|PTE_X|PTE_U);
        }
    }
    map_range(user_pgd, PROC_USER_STACK_BOTTOM, (uintptr_t)user_stack, PROC_STACK_SIZE,  PTE_W|PTE_R|PTE_U);
    page_table_init(user_pgd);

    new_proc->elf_info = elf_info;
    new_proc->user_sp = PROC_USER_STACK_TOP;
    new_proc->kernel_sp = (uintptr_t)kernel_stack + PROC_STACK_SIZE - sizeof(struct reg_context);
    new_proc->pgd = user_pgd;
    new_proc->context.sepc = elf_info->entry;
    new_proc->context.sp = new_proc->user_sp;
    new_proc->pid = alloc_pid();
    new_proc->status = 0;
    enum hart_id hart_id = tp_r(); // 现在只支持hart0
    list_add(&proc_list_head[hart_id], &new_proc->proc_lnode);
    proc_count[hart_id]++;

    return new_proc;
}

void proc_run(struct proc *p)
{
    if(p == NULL) return ;
    sscratch_w(sp_r());
    satp_w(make_satp(p->pgd));
    sstatus_w(sstatus_r() & ~(1<<8));  
    sepc_w((uintptr_t)(p->context.sepc)); 
    asm volatile("mv sp,%0"::"r"(p->user_sp));
    asm volatile("sret");
}