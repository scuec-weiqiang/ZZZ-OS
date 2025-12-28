/**
 * @FilePath: /ZZZ-OS/kernel/proc.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-16 18:23:57
 * @LastEditTime: 2025-12-04 22:05:13
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/mm/mm_types.h"
#include <os/proc.h>
#include <fs/vfs.h>
#include <os/elf.h>
#include <os/mm.h>
#include <os/check.h>
#include <os/string.h>
#include <asm/platform.h>
#include <os/kmalloc.h>
#include <asm/arch_timer.h>
#include <os/sched.h>
#include <os/mm/page.h>
#include <asm/pgtbl.h>
#include <os/kva.h>
#include <os/mm/vma_flags.h>
#include <os/mm/pgtbl.h>

struct list_head proc_list_head[MAX_HARTS_NUM];

uint64_t proc_count[MAX_HARTS_NUM];

static pid_t alloc_pid()
{
    static pid_t next_pid = 1;
    return next_pid++;
}


void proc_setup()
{
    struct proc* p = scheduler[tp_r()].current;
    p->expire_time = systick() + p->time_slice;
    p->status = PROC_RUNNING;
    sstatus_w(sstatus_r() & ~(1<<8));  
    sepc_w((uintptr_t)(p->trapframe->sepc)); 
    sscratch_w(p->kernel_sp);
    asm volatile("mv sp,%0"::"r"(p->user_sp));
    asm volatile("sret");
}

void proc_init() {
    for(int hart_id = 0;hart_id < MAX_HARTS_NUM; hart_id++)
    {
        INIT_LIST_HEAD(&proc_list_head[hart_id])
        proc_count[hart_id] = 0;
    }

}

struct proc* proc_create(char* path) {
    CHECK(path != NULL, "proc create failed,path is NULL", return NULL;);

    struct file* f = open(path,0);
    char* elf = kmalloc(f->f_inode->i_size);
    ssize_t ret =  read(f,elf,f->f_inode->i_size);
    CHECK(ret >= 0, "vfs read failed", kfree(elf); return NULL;);

    struct elf_info *elf_info = elf_parse(elf);
    CHECK(elf_info != NULL, "elf parse failed", kfree(elf); return NULL;);

    struct proc* new_proc = (struct proc*)kmalloc(sizeof(struct proc));
    memset(new_proc,0,sizeof(struct proc));

    struct mm_struct *mm = mm_create("user_pgtbl");

    char *user_stack = (char*)kmalloc(PROC_STACK_SIZE);
    memset(user_stack,0,PROC_STACK_SIZE);

    char *kernel_stack = (char*)kmalloc(PROC_STACK_SIZE);
    memset(kernel_stack,0,PROC_STACK_SIZE);

    for(int i=0,j=0;i<elf_info->phnum;i++)
    {
        if(elf_info->segs[i].type== PT_LOAD)
        {
            // printk("phdr %d: vaddr:%x, memsz:%x, filesz:%x, offset:%x, flags:%x\n",i,elf_info->segs[i].vaddr,elf_info->segs[i].memsz,elf_info->segs[i].filesz,elf_info->segs[i].offset,elf_info->segs[i].flags);
            char *user_space = (char*)kmalloc(elf_info->segs[i].memsz); //程序加载到内存里需要的空间
            memset(user_space,0,elf_info->segs[i].memsz);
            memcpy(user_space,elf+elf_info->segs[i].offset,elf_info->segs[i].filesz);
            map(mm->pgdir, elf_info->segs[i].vaddr, (uintptr_t)KERNEL_PA(user_space), elf_info->segs[i].memsz,VMA_W|VMA_R|VMA_X|VMA_USER);
            new_proc->code[j] = user_space;
            j++;
        }
    }
    map(mm->pgdir, PROC_USER_STACK_TOP, (uintptr_t)KERNEL_PA(user_stack), PROC_STACK_SIZE, VMA_W|VMA_R|VMA_USER);
    copy_kernel_mapping(mm);

    new_proc->elf_info = elf_info;
    new_proc->user_sp = PROC_USER_STACK_BOTTOM;
    new_proc->kernel_sp = (uintptr_t)kernel_stack + PROC_STACK_SIZE;
    new_proc->mm = mm;
    new_proc->trapframe = (struct trap_frame*)((uintptr_t)kernel_stack + PROC_STACK_SIZE - sizeof(struct trap_frame));
    new_proc->trapframe->sepc = elf_info->entry;
    new_proc->trapframe->sp = new_proc->user_sp;
    new_proc->context.ra = (reg_t)proc_setup;
    new_proc->context.sp = new_proc->kernel_sp;
    new_proc->pid = alloc_pid();
    new_proc->status = PROC_RUNABLE;
    new_proc->time_slice = PROC_DEFAULT_SLICE;
    int hart_id = tp_r();
    list_add(&proc_list_head[hart_id], &new_proc->proc_lnode);
    proc_count[hart_id]++;
    return new_proc;
}

struct proc* current_proc() {
    return scheduler[tp_r()].current;
}

int alloc_fd(struct proc* p, struct file* f) {
    CHECK(p != NULL && f != NULL, "alloc fd failed", return -1;);
    for(int i=0;i<256;i++)
    {
        if(p->fd_table[i] == NULL)
        {
            p->fd_table[i] = f;
            return i;
        }
    }
    return -1;
}

void free_fd(struct proc* p, int fd) {
    CHECK(p != NULL && fd >=0 && fd < 256, "kfree fd failed", return;);
    if(p->fd_table[fd] != NULL)
    {
        // close(p->fd_table[fd]);
        p->fd_table[fd] = NULL;
    }
}