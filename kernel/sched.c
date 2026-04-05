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
#include <mm/pgtbl.h>

struct scheduler scheduler[MAX_cpuS_NUM];

static uint64_t         _check_expire(uint64_t now_time,uint64_t expire_time);
static struct proc* _get_next_task(int cpu_id);

extern void swtch(struct context* old, struct context* new);

void sched_init(int cpu_id) {
    INIT_LIST_HEAD(&scheduler[cpu_id].ready_queue);
    INIT_LIST_HEAD(&scheduler[cpu_id].wait_queue);
    list_splice(&proc_list_head[cpu_id],&scheduler[cpu_id].ready_queue);
    scheduler[cpu_id].current = NULL;
}

void yield() {
    int cpu_id = tp_r();
    struct proc* p = scheduler[cpu_id].current;
    if(p == NULL)
    {
        return;
    }
    p->status = PROC_RUNABLE;
    swtch(&p->context,&scheduler[cpu_id].ctx);
}

void sched() {
    int cpu_id = tp_r();
    scheduler[cpu_id].current = NULL;
    arch_timer_start();
    while(1)
    {
        struct proc* next = _get_next_task(cpu_id);
        if(!next) continue;
        current_mm_struct = next->mm;
        next->expire_time = next->time_slice + systick();
        scheduler[cpu_id].current = next;
        pgtbl_switch_to(next->mm->pgdir);
        pgtbl_flush();
        swtch(&scheduler[cpu_id].ctx,&next->context);
    }
} 

static struct proc* _get_next_task(int cpu_id) {
    if(list_empty(&scheduler[cpu_id].ready_queue))//如果就绪队列为空
    { 
        return NULL;
    }
   
    if(scheduler[cpu_id].current == NULL) //就绪队列不为空，但当前没有运行的任务，指定第一个任务为下一个运行任务
    {
       return list_entry(scheduler[cpu_id].ready_queue.next,struct proc,proc_lnode);
    }
    else 
    {
        if(scheduler[cpu_id].current->proc_lnode.next == &scheduler[cpu_id].ready_queue)
        {
            return list_entry(scheduler[cpu_id].ready_queue.next,struct proc,proc_lnode);
        }
        else
        {
           return list_entry(scheduler[cpu_id].current->proc_lnode.next,struct proc,proc_lnode);
        }
    }
}

static inline uint64_t _check_expire(uint64_t now_time,uint64_t expire_time) {
    return now_time >= expire_time?1:0;
}
