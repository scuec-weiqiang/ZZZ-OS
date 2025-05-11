/**
 * @FilePath: /ZZZ/kernel/task.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 21:02:39
 * @LastEditTime: 2025-05-09 21:57:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/*******************************************************************************************
 * @FilePath: /ZZZ/kernel/task.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 21:02:39
 * @LastEditTime: 2025-04-30 22:15:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#include "task.h"
#include "page_alloc.h"
#include "riscv.h"
#include "systimer.h"
#include "list.h"
#include "spinlock.h"

spinlock_t task_create_lock = SPINLOCK_INIT;

//这是个中介，作为链表头，需要加入调度的任务会挂载到这个链表上，调度器会从这个链表拆取任务合并到调度器自己的链表中
list_t need_add_task[MAX_HARTS_NUM] ;

__SELF uint64_t task_id = 1;

// __SELF void idle_task(void* param) {
//     while (1) {
//         // 低功耗指令（如WFI）
//         // asm volatile ("wfi");
//         printf("task idle running  \r\n");
//     }
// }

void task_init()
{
   for(hart_id_t hart_id = HART_0;hart_id < MAX_HARTS_NUM; hart_id++)
   {
       INIT_LIST_HEAD(&need_add_task[hart_id])
   }
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
task_handle_t task_create(hart_id_t hart_id, void (*task)(void* param),uint64_t time_slice,uint8_t priority)
{   
    if(IS_NULL_PTR(task)) return NULL;
    tcb_t* task_ctrl_block = page_alloc(1);
    if(IS_NULL_PTR(task_ctrl_block)) return NULL;

    spin_lock(&task_create_lock);
    task_ctrl_block->id = task_id++;
    spin_unlock(&task_create_lock);
    task_ctrl_block->task = task;
    task_ctrl_block->status = TASK_READY;
    task_ctrl_block->expire_time = 0;
    task_ctrl_block->priority = priority;
    task_ctrl_block->time_slice = time_slice;
    // task_ctrl_block->reg_context.ra = (uint64_t)task;
    // task_ctrl_block->reg_context.mepc = (uint64_t)task;
    // task_ctrl_block->reg_context.sp = (uint64_t)&task_ctrl_block->task_stack[TASK_STACK_SIZE-1];
    list_add_tail(&need_add_task[hart_id],&task_ctrl_block->node);

    return (task_handle_t)task_ctrl_block;
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
void task_delete(task_handle_t del_task)
{
    if(IS_NULL_PTR(del_task)) return;
    tcb_t* task_block = (tcb_t*)del_task;
    task_block->status = TASK_ZOMBIE;
}

// void task_exit(int exit_code)
//  {

//     current_task->state = TASK_ZOMBIE;
//     current_task->exit_code = exit_code;
//     list_del(&current_task->node);
//     add_to_zombie_list(current_task);
//     schedule(); // 触发调度器选择新任务
// }
/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
// void task_run()
// {   
//     if(NULL_PTR != task_head)
//     {
//         task_current = task_head;
//         // printf("task will run\n");
//         printf("taskRUn:%x\n",mstatus_r());
//         __task_entry(&task_current->reg_context);
//     }
//     panic("\n  no task to exec!");
// }
        

void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}