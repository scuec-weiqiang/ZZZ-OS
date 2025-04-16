#ifndef SCHED_H
#define SCHED_H

#include "list.h"
#include "riscv.h"
#include "task.h"


typedef struct{
    reg_context_t reg_context;
    uint8_t __attribute__((aligned(16))) stack[1024]; 
    list_t* running_queue;
    list_t* blocked_queue; 
}scheduler_t;

extern scheduler_t scheduler;

extern void sched_init();

#endif