#ifndef __OS_WORK_QUEUE_H
#define __OS_WORK_QUEUE_H

#include <os/types.h>
#include <os/spinlock.h>
#include <os/wait.h>

struct work_struct;
typedef void (*work_func_t)(struct work_struct *work);

struct work_struct {
    work_func_t func;
    struct list_head entry;
    int pending;
};

struct workqueue_struct {
    const char *name;
    struct list_head work_list;
    spinlock_t lock;
    struct wait_queue_head wait;
    struct task_struct *worker;
};

extern struct workqueue_struct *system_wq;

void init_work(struct work_struct *work,
               void (*func)(struct work_struct *));

struct workqueue_struct *create_workqueue(const char *name);

bool queue_work(struct workqueue_struct *wq,
                struct work_struct *work);


#endif