/**
 * @FilePath: /ZZZ-OS/kernel/sched.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-01 15:16:19
 * @LastEditTime: 2025-12-04 22:08:25
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/mm.h>
#include <os/sched.h>
#include <os/proc.h>
#include <asm/platform.h>
#include <os/list.h>
#include <asm/arch_timer.h>

struct scheduler scheduler[MAX_HARTS_NUM];

static uint64_t         _check_expire(uint64_t now_time,uint64_t expire_time);
static struct proc* _get_next_task(int hart_id);

extern void swtch(struct context* old, struct context* new);

void sched_init(int hart_id) {
    INIT_LIST_HEAD(&scheduler[hart_id].ready_queue);
    INIT_LIST_HEAD(&scheduler[hart_id].wait_queue);
    list_splice(&proc_list_head[hart_id],&scheduler[hart_id].ready_queue);
    scheduler[hart_id].current = NULL;
}

void yield() {
    int hart_id = tp_r();
    struct proc* p = scheduler[hart_id].current;
    if(p == NULL)
    {
        return;
    }
    p->status = PROC_RUNABLE;
    swtch(&p->context,&scheduler[hart_id].ctx);
}

void sched() {
    int hart_id = tp_r();
    scheduler[hart_id].current = NULL;
    arch_timer_start();
    while(1)
    {
        struct proc* next = _get_next_task(hart_id);
        if(!next) continue;
        next->expire_time = next->time_slice + systick();
        scheduler[hart_id].current = next;
        pgtbl_switch(next->pgd);
        pgtbl_flush();
        swtch(&scheduler[hart_id].ctx,&next->context);
    }
} 

static struct proc* _get_next_task(int hart_id) {
    if(list_empty(&scheduler[hart_id].ready_queue))//如果就绪队列为空
    { 
        return NULL;
    }
   
    if(scheduler[hart_id].current == NULL) //就绪队列不为空，但当前没有运行的任务，指定第一个任务为下一个运行任务
    {
       return list_entry(scheduler[hart_id].ready_queue.next,struct proc,proc_lnode);
    }
    else 
    {
        if(scheduler[hart_id].current->proc_lnode.next == &scheduler[hart_id].ready_queue)
        {
            return list_entry(scheduler[hart_id].ready_queue.next,struct proc,proc_lnode);
        }
        else
        {
           return list_entry(scheduler[hart_id].current->proc_lnode.next,struct proc,proc_lnode);
        }
    }
}

static inline uint64_t _check_expire(uint64_t now_time,uint64_t expire_time) {
    return now_time >= expire_time?1:0;
}
