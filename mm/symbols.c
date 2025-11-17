/**
 * @FilePath: /ZZZ-OS/mm/symbols.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 13:05:59
 * @LastEditTime: 2025-11-17 21:10:56
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/printk.h>

extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __data_end[];
extern char __bss_start[], __bss_end[];
extern char __stack_start[], __stack_end[];
extern char __heap_start[], __heap_end[];
extern char __initcall_start[],__initcall_end[];
extern char __exitcall_start[],__exitcall_end[];
extern char __irqinitcall_start[],__irqinitcall_end[];
extern char __irqexitcall_start[],__irqexitcall_end[];
extern char __kernel_start[],__kernel_end[];
extern char _early_stack_start[], _early_stack_end[];

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
uintptr_t initcall_start;
uintptr_t initcall_end;
uintptr_t initcall_size;
uintptr_t exitcall_start;
uintptr_t exitcall_end;
uintptr_t exitcall_size;

uintptr_t irqinitcall_start;
uintptr_t irqinitcall_end;
uintptr_t irqinitcall_size;
uintptr_t irqexitcall_start;
uintptr_t irqexitcall_end;
uintptr_t irqexitcall_size;

uintptr_t kernel_start;
uintptr_t kernel_end;
uintptr_t kernel_size;

uintptr_t early_stack_start;
uintptr_t early_stack_end;
uintptr_t early_stack_size;

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

    initcall_start = (uintptr_t)&__initcall_start;
    initcall_end = (uintptr_t)&__initcall_end;
    initcall_size = (initcall_end - initcall_start);

    
    irqinitcall_start = (uintptr_t)&__irqinitcall_start;
    irqinitcall_end = (uintptr_t)&__irqinitcall_end;
    irqinitcall_size = (irqinitcall_end - irqinitcall_start);

    exitcall_start = (uintptr_t)&__exitcall_start;
    exitcall_end = (uintptr_t)&__exitcall_end;
    exitcall_size = (exitcall_end - exitcall_start);
    
    irqexitcall_start = (uintptr_t)&__irqexitcall_start;
    irqexitcall_end = (uintptr_t)&__irqexitcall_end;
    irqexitcall_size = (irqexitcall_end - irqexitcall_start);

    kernel_start = (uintptr_t)&__kernel_start;
    kernel_end = (uintptr_t)&__kernel_end;
    kernel_size = kernel_end - kernel_start;

    early_stack_start = (uintptr_t)&_early_stack_start;
    early_stack_end = (uintptr_t)&_early_stack_end;
    early_stack_size = early_stack_end - early_stack_start;


    printk("text:        start= %xu, end=  %xu, size=  %xu\n", text_start, text_end, text_size);
    printk("rodata:      start= %xu, end=  %xu, size=  %xu\n", rodata_start, rodata_end, rodata_size);
    printk("data:        start= %xu, end=  %xu, size=  %xu\n", data_start, data_end, data_size);
    printk("bss:         start= %xu, end=  %xu, size=  %xu\n", bss_start, bss_end, bss_size);
    printk("stack:       start= %xu, end=  %xu, size=  %xu\n", stack_start, stack_end, stack_size);
    printk("heap:        start= %xu, end=  %xu, size=  %xu\n", heap_start, heap_end, heap_size);
    printk("initcall:    start= %xu, end=  %xu, size=  %xu\n", initcall_start, initcall_end, initcall_size);
    printk("irqinitcall: start= %xu, end=  %xu, size=  %xu\n", irqinitcall_start, irqinitcall_end, irqinitcall_size);
    printk("exitcall:    start= %xu, end=  %xu, size=  %xu\n", exitcall_start, exitcall_end, exitcall_size);
    printk("irqexitcall: start= %xu, end=  %xu, size=  %xu\n", irqexitcall_start, irqexitcall_end, irqexitcall_size);
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