/**
 * @FilePath: /ZZZ-OS/kernel/sched.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-01 02:29:14
 * @LastEditTime: 2025-10-06 18:53:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _SCHED_H
#define _SCHED_H

#include "list.h"
#include "riscv.h"
#include "proc.h"
#include "platform.h"

enum sched_state 
{ 
    SCHED_IDLE,
    SCHED_CONTINUE,
    SCHED_SWITCHING 
};

struct scheduler
{ 
    u64 task_num;
    struct list ready_queue;
    struct list wait_queue; 
    struct proc* current;
    struct context ctx;   // 调度器的上下文
};

extern struct scheduler scheduler[MAX_HARTS_NUM];

extern void sched_init(enum hart_id hart_id);
extern void sched();
extern void yield();

#endif
