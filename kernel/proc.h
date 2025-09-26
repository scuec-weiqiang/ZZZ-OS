/**
 * @FilePath: /ZZZ-OS/kernel/proc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 13:50:23
 * @LastEditTime: 2025-09-23 20:40:10
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
#include "vfs_types.h"
#include "platform.h"

#define PROC_STACK_SIZE PAGE_SIZE

#define PROC_USER_STACK_BOTTOM (0xfffffffffull)
#define PROC_USER_STACK_TOP (PROC_USER_STACK_BOTTOM - PROC_STACK_SIZE + 1)

#define PROC_DEFAULT_SLICE 500

struct trap_frame
{
    reg_t ra, sp, gp, tp;
    reg_t t0, t1, t2, s0, s1;
    reg_t a0, a1, a2, a3, a4, a5, a6, a7;
    reg_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    reg_t t3, t4, t5, t6;
    reg_t sepc, sstatus, scause, stval;
};

// 内核上下文，只需要保存 callee-saved 寄存器
struct context 
{
    reg_t ra, sp;
    reg_t s0, s1, s2, s3;
    reg_t s4, s5, s6, s7;
    reg_t s8, s9, s10,s11;
};

typedef u64 pid_t;

enum proc_status
{
    PROC_SLEEPING = -1,
    PROC_RUNABLE = 0,
    PROC_RUNNING = 1,
};

struct proc
{
    char *code[8];
    struct file *fd_table[256];
    uintptr_t kernel_sp; //内核态栈顶
    uintptr_t user_sp;   //用户态栈顶
    pgtbl_t* pgd;       //页表
    struct trap_frame *trapframe; //寄存器上下文
    struct context context; //内核上下文
    struct elf_info* elf_info; //程序信息
    pid_t pid;            //进程ID
    enum proc_status status;         //进程状态
    u64 expire_time;
    u64 time_slice;
    struct list proc_lnode;
};

extern struct list proc_list_head[MAX_HARTS_NUM];
extern void proc_init();
extern struct proc* proc_create(char* path);

#endif // KERNEL_PROC_H