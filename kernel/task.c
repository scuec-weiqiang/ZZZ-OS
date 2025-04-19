#include "task.h"

//这是个中介，作为链表头，需要加入调度的任务会挂载到这个链表上，调度器会从这个链表拆取任务合并到调度器自己的链表中
LIST_HEAD(need_add_task)
//这是个中介，作为链表头，需要销毁的任务会挂载到这个链表上，调度器会从这个链表读取任务，并从调度器自己的链表中拆除该任务，并释放内存
LIST_HEAD(need_del_task)

static task_id = 1;

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
void task_create(void (*task)(void* param),uint64_t time_slice,uint8_t priority)
{   
    if(IS_NULL_PTR(task)) return;
    tcb_t* task_ctrl_block = page_alloc(1);
    if(IS_NULL_PTR(task_ctrl_block)) return;

    task_ctrl_block->id = task_id;
    task_ctrl_block->task = task;
    task_ctrl_block->status = TASK_READY;
    task_ctrl_block->expire_time = 0;
    task_ctrl_block->priority = priority;
    task_ctrl_block->time_slice = time_slice;
    task_ctrl_block->reg_context.ra = (uint64_t)task;
    task_ctrl_block->reg_context.mepc = (uint64_t)task;
    task_ctrl_block->reg_context.sp = (uint64_t)&task_ctrl_block->task_stack[TASK_STACK_SIZE-1];
    list_add_tail(&need_add_task,&task_ctrl_block->node);
    task_id++;
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
void task_distory(void (*del_task)(void))
{
    if(IS_NULL_PTR(del_task)) return;
    tcb_t* task_block = list_entry(del_task,tcb_t,task);
    list_add_tail(&need_del_task,&task_block->node);
}

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