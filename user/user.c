/*******************************************************************************************
 * @FilePath     : /ZZZ/user/user.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-19 01:26:53
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#include "types.h"
#include "task.h"
// #include "clint.h"
#include "printf.h"

void task0()
{
    uint32_t a = 0;
    uint32_t b = 1;
    while (1)
    {
        a++;        
        task_delay(5000);
        printf("task0 running  %d\r\n",a);
        b = mhartid_r();
        printf("hartid = %d\n",b);
    }
    
}

void task1()
{
    uint32_t a = 0;
    while (1)
    {
        a++;   
        task_delay(5000);    
        printf("task1 running  %d\r\n",a);
    }
}

void task2()
{
    uint32_t a = 0;
    while (1)
    {
        a++;       
        // task_delay(5000); 
        printf("task2 running  %d\r\n",a);
    }
}

void task3()
{
    uint32_t a = 0;
    while (1)
    {
        a++;        
        // task_delay(5000);
        printf("task3 running  %d\r\n",a);
    }
}

void __attribute((aligned(4))) os_main()
{
    printf("into main!\n");
    task_create(task0,500,0);
    task_create(task1,1000,0);
    while(1){} 
    // task_create(task2,100,0);
    // task_create(task3,100,0);
}