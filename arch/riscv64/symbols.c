/**
 * @FilePath: /ZZZ-OS/arch/riscv64/symbols.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 13:05:59
 * @LastEditTime: 2025-10-23 20:05:56
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"

extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __data_end[];
extern char __bss_start[], __bss_end[];
extern char __stack_start[], __stack_end[];
extern char __heap_start[], __heap_end[];


uintptr_t text_start;
uintptr_t text_end;
uintptr_t text_size;
uintptr_t rodata_start;
uintptr_t rodata_end;
uintptr_t rodata_size;
uintptr_t data_start;
uintptr_t data_end;
uintptr_t data_size;
uintptr_t bss_start;
uintptr_t bss_end;
uintptr_t bss_size;
uintptr_t stack_start;
uintptr_t stack_end;
uintptr_t stack_size;
uintptr_t heap_start;
uintptr_t heap_end;
uintptr_t heap_size;


void symbols_init()
{
    text_start = (uintptr_t)&__text_start;
    text_end = (uintptr_t)&__text_end;
    text_size = (text_end - text_start);
    rodata_start = (uintptr_t)&__rodata_start;
    rodata_end = (uintptr_t)&__rodata_end;
    rodata_size = (rodata_end - rodata_start);
    data_start = (uintptr_t)&__data_start;
    data_end = (uintptr_t)&__data_end;
    data_size = (data_end - data_start);
    bss_start = (uintptr_t)&__bss_start;
    bss_end = (uintptr_t)&__bss_end;
    bss_size = (bss_end - bss_start);
    stack_start = (uintptr_t)&__stack_start;
    stack_end = (uintptr_t)&__stack_end;
    stack_size = (stack_end - stack_start);
    heap_start = (uintptr_t)&__heap_start;
    heap_end = (uintptr_t)&__heap_end;
    heap_size = (heap_end - heap_start);
}

/**
 * @brief 将BSS段中的所有数据清零
 *
 * 遍历BSS段的起始地址到结束地址之间的所有字节，并将它们置为零。
 *
 * BSS段通常用于存储未初始化的全局变量和静态变量，它们在程序启动时不会自动初始化为零。
 * 本函数通过手动遍历并清零这些变量，确保它们在程序启动时是干净的。
 */
void zero_bss() 
{
    for (char *p = (char*)bss_start; p < (char*)bss_end; p++) 
    {
        *p = 0;
    }
}