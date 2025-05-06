/*******************************************************************************************
 * @FilePath: /ZZZ/user/user.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 15:52:48
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#include "types.h"
#include "task.h"
#include "printf.h"
#include "systimer.h"
#include "swtimer.h"

uint64_t t = 0;
void task0_timer()
{
    t++;
    printf("t = %d\n",t);
}

void task0()
{
    uint32_t a = 0;
    // swtimer_create(task0_timer,100,5);
    // uint32_t b = 1;
    while (1)
    {
        a++;        
        task_delay(2000);
        printf("task0 running  %d\r\n",a);

        // printf("task0 running\r");

        // b = mhartid_r();
        // printf("hartid = %d\n",b);
    }
    
}

void task1()
{
    uint32_t a = 0;
    while (1)
    {
        a++;   
        task_delay(2000);    
        printf("task1 running  %d\r\n",a);
        // printf("task1 running\r");
    }
}

void task2()
{
    uint32_t a = 0;
    while (1)
    {
        a++;       
        task_delay(2000); 
        printf("task2 running  %d\r\n",a);
    }
}

void task3()
{
    uint32_t a = 0;
    while (1)
    {
        a++;        
        task_delay(2000);
        printf("task3 running  %d\r\n",a);
    }
}

void __attribute((aligned(4))) os_main()
{
    printf("into main!\n");
    task_create(task0,600*tick_ms,0);
    task_create(task1,1000*tick_ms,0);
    // printf("create 2 tasks \n");
    task_create(task2,250*tick_ms,0);
    task_create(task3,100*tick_ms,0);
    while(1){} 

}