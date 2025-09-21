/**
 * @FilePath: /ZZZ/kernel/proc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:23
 * @LastEditTime: 2025-09-20 17:03:25
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include "types.h"
#include "riscv.h"
#include "vm.h"
#include "list.h"
#include "elf.h"

#define PROC_STACK_SIZE PAGE_SIZE
#define PROC_USER_STACK_TOP 0x80000000
#define PROC_USER_STACK_BOTTOM (PROC_USER_STACK_TOP - PROC_STACK_SIZE)

typedef int pid_t;

struct proc
{
    uintptr_t kernel_sp; //内核态栈顶
    uintptr_t user_sp;   //用户态栈顶
    pgtbl_t* pgd;       //页表
    struct reg_context context; //寄存器上下文
    struct elf_info* elf_info; //程序信息
    int pid;            //进程ID
    int status;         //进程状态
    struct list proc_lnode;
};


extern void proc_init();
extern struct proc* proc_create(char* path);
extern void proc_run(struct proc *p);

#endif // KERNEL_PROC_H