#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "riscv.h"
#include "list.h"

#define TASK_STACK_SIZE 1024 //每个任务分配1k的栈大小

typedef enum {
    RUNNING,
    READY,
    BLOCKED,
    ZOMBIE
}task_status_t;

typedef struct task_ctrl_block
{   
    uint64_t id;
    uint64_t time_slice;
    uint8_t priority;
    task_status_t status;
    reg_context_t reg_context;
    uint8_t  __attribute__((aligned(16))) task_stack[TASK_STACK_SIZE];
    void (*task)(void *param);
    list_t node;
}tcb_t;

//这是个中介，作为链表头，需要加入调度的任务会挂载到这个链表上，调度器会从这个链表拆取任务合并到调度器自己的链表中
extern THIS_IS_LIST_HEAD(need_add_task);
extern THIS_IS_LIST_HEAD(need_del_task);

extern void task_create(void (*task)(void *param),uint64_t time_slice,uint8_t priority);
extern void task_distory(void (*task)(void));
extern void task_run();

extern void task_delay(volatile int count);

#endif