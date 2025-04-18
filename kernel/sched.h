#ifndef SCHED_H
#define SCHED_H

#include "list.h"
#include "riscv.h"
#include "task.h"


typedef struct{ 
    // uint64_t task_num;
    list_t running_queue;
    list_t blocked_queue; 
    tcb_t *current_task;
}scheduler_t;

extern scheduler_t scheduler;

extern void sched_init();
extern reg_t sched(reg_t epc);

#endif