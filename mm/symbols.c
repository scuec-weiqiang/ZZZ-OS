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

extern char _text_start[], _text_end[];
extern char _rodata_start[], _rodata_end[];
extern char _data_start[], _data_end[];
extern char _bss_start[], _bss_end[];
extern char _initcall_start[],_initcall_end[];
extern char _exitcall_start[],_exitcall_end[];
extern char _irqinitcall_start[],_irqinitcall_end[];
extern char _irqexitcall_start[],_irqexitcall_end[];
extern char _kernel_start[],_kernel_end[];
extern char _early_stack_start[], _early_stack_end[];

phys_addr_t text_start;
phys_addr_t text_end;
phys_addr_t text_size;
phys_addr_t rodata_start;
phys_addr_t rodata_end;
phys_addr_t rodata_size;
phys_addr_t data_start;
phys_addr_t data_end;
phys_addr_t data_size;
phys_addr_t bss_start;
phys_addr_t bss_end;
phys_addr_t bss_size;
phys_addr_t initcall_start;
phys_addr_t initcall_end;
phys_addr_t initcall_size;
phys_addr_t exitcall_start;
phys_addr_t exitcall_end;
phys_addr_t exitcall_size;

phys_addr_t irqinitcall_start;
phys_addr_t irqinitcall_end;
phys_addr_t irqinitcall_size;
phys_addr_t irqexitcall_start;
phys_addr_t irqexitcall_end;
phys_addr_t irqexitcall_size;

phys_addr_t kernel_start;
phys_addr_t kernel_end;
phys_addr_t kernel_size;

phys_addr_t early_stack_start;
phys_addr_t early_stack_end;
phys_addr_t early_stack_size;

void symbols_init()
{
    text_start = (phys_addr_t)&_text_start;
    text_end = (phys_addr_t)&_text_end;
    text_size = (text_end - text_start);
    rodata_start = (phys_addr_t)&_rodata_start;
    rodata_end = (phys_addr_t)&_rodata_end;
    rodata_size = (rodata_end - rodata_start);
    data_start = (phys_addr_t)&_data_start;
    data_end = (phys_addr_t)&_data_end;
    data_size = (data_end - data_start);
    bss_start = (phys_addr_t)&_bss_start;
    bss_end = (phys_addr_t)&_bss_end;
    bss_size = (bss_end - bss_start);

    initcall_start = (phys_addr_t)&_initcall_start;
    initcall_end = (phys_addr_t)&_initcall_end;
    initcall_size = (initcall_end - initcall_start);

    
    irqinitcall_start = (phys_addr_t)&_irqinitcall_start;
    irqinitcall_end = (phys_addr_t)&_irqinitcall_end;
    irqinitcall_size = (irqinitcall_end - irqinitcall_start);

    exitcall_start = (phys_addr_t)&_exitcall_start;
    exitcall_end = (phys_addr_t)&_exitcall_end;
    exitcall_size = (exitcall_end - exitcall_start);
    
    irqexitcall_start = (phys_addr_t)&_irqexitcall_start;
    irqexitcall_end = (phys_addr_t)&_irqexitcall_end;
    irqexitcall_size = (irqexitcall_end - irqexitcall_start);

    kernel_start = (phys_addr_t)&_kernel_start;
    kernel_end = (phys_addr_t)&_kernel_end;
    kernel_size = kernel_end - kernel_start;

    early_stack_start = (phys_addr_t)&_early_stack_start;
    early_stack_end = (phys_addr_t)&_early_stack_end;
    early_stack_size = early_stack_end - early_stack_start;

    printk("text:        start= %xu, end=  %xu, size=  %xu\n", text_start, text_end, text_size);
    printk("rodata:      start= %xu, end=  %xu, size=  %xu\n", rodata_start, rodata_end, rodata_size);
    printk("data:        start= %xu, end=  %xu, size=  %xu\n", data_start, data_end, data_size);
    printk("bss:         start= %xu, end=  %xu, size=  %xu\n", bss_start, bss_end, bss_size);
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