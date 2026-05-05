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
extern char _sched_text_start[], _sched_text_end[];
extern char _trap_start[], _trap_end[]; 
extern char _rodata_start[], _rodata_end[];
extern char _data_start[], _data_end[];
extern char _bss_start[], _bss_end[];
extern char _initcall_start[],_initcall_end[];
extern char _archinitcall_start[],_archinitcall_end[];
extern char _coreinitcall_start[],_coreinitcall_end[];
extern char _fsinitcall_start[],_fsinitcall_end[];
extern char _deviceinitcall_start[],_deviceinitcall_end[];
extern char _lateinitcall_start[],_lateinitcall_end[];
extern char _exitcall_start[],_exitcall_end[];
extern char _irqinitcall_start[],_irqinitcall_end[];
extern char _irqexitcall_start[],_irqexitcall_end[];
extern char _kernel_start[],_kernel_end[];
extern char _early_stack_start[], _early_stack_end[];
extern char _early_pgtbl_start[], _early_pgtbl_end[];

phys_addr_t text_start;
phys_addr_t text_end;
size_t text_size;

phys_addr_t sched_text_start;
phys_addr_t sched_text_end;
size_t sched_text_size;

phys_addr_t trap_start;
phys_addr_t trap_end;
size_t trap_size;

phys_addr_t rodata_start;
phys_addr_t rodata_end;
size_t rodata_size;

phys_addr_t data_start;
phys_addr_t data_end;
size_t data_size;

phys_addr_t bss_start;
phys_addr_t bss_end;
size_t bss_size;

phys_addr_t initcall_start;
phys_addr_t initcall_end;
size_t initcall_size;

phys_addr_t archinitcall_start;
phys_addr_t archinitcall_end;
size_t archinitcall_size;

phys_addr_t coreinitcall_start;
phys_addr_t coreinitcall_end;
size_t coreinitcall_size;

phys_addr_t fsinitcall_start;
phys_addr_t fsinitcall_end;
size_t fsinitcall_size;

phys_addr_t deviceinitcall_start;
phys_addr_t deviceinitcall_end;
size_t deviceinitcall_size;

phys_addr_t lateinitcall_start;
phys_addr_t lateinitcall_end;
size_t lateinitcall_size;

phys_addr_t exitcall_start;
phys_addr_t exitcall_end;
size_t exitcall_size;

phys_addr_t irqinitcall_start;
phys_addr_t irqinitcall_end;
size_t irqinitcall_size;

phys_addr_t irqexitcall_start;
phys_addr_t irqexitcall_end;
size_t irqexitcall_size;

phys_addr_t kernel_start;
phys_addr_t kernel_end;
size_t kernel_size;

phys_addr_t early_stack_start;
phys_addr_t early_stack_end;
size_t early_stack_size;

phys_addr_t early_pgtbl_start;
phys_addr_t early_pgtbl_end;
size_t early_pgtbl_size;

void print_section() {
    printk("text:        start= %xu, end=  %xu, size=  %xu\n", text_start, text_end, text_size);
    printk("trap:        start= %xu, end=  %xu, size=  %xu\n", trap_start, trap_end, trap_size);
    printk("rodata:      start= %xu, end=  %xu, size=  %xu\n", rodata_start, rodata_end, rodata_size);
    printk("data:        start= %xu, end=  %xu, size=  %xu\n", data_start, data_end, data_size);
    printk("bss:         start= %xu, end=  %xu, size=  %xu\n", bss_start, bss_end, bss_size);
    printk("initcall:    start= %xu, end=  %xu, size=  %xu\n", initcall_start, initcall_end, initcall_size);
    printk("archinit:    start= %xu, end=  %xu, size=  %xu\n", archinitcall_start, archinitcall_end, archinitcall_size);
    printk("coreinit:    start= %xu, end=  %xu, size=  %xu\n", coreinitcall_start, coreinitcall_end, coreinitcall_size);
    printk("fsinitcall:  start= %xu, end=  %xu, size=  %xu\n", fsinitcall_start, fsinitcall_end, fsinitcall_size);
    printk("deviceinit:  start= %xu, end=  %xu, size=  %xu\n", deviceinitcall_start, deviceinitcall_end, deviceinitcall_size);
    printk("lateinit:    start= %xu, end=  %xu, size=  %xu\n", lateinitcall_start, lateinitcall_end, lateinitcall_size);
    printk("irqinitcall: start= %xu, end=  %xu, size=  %xu\n", irqinitcall_start, irqinitcall_end, irqinitcall_size);
    printk("exitcall:    start= %xu, end=  %xu, size=  %xu\n", exitcall_start, exitcall_end, exitcall_size);
    printk("irqexitcall: start= %xu, end=  %xu, size=  %xu\n", irqexitcall_start, irqexitcall_end, irqexitcall_size);
    printk("early_stack: start= %xu, end=  %xu, size=  %xu\n", early_stack_start, early_stack_end, early_stack_size);
    printk("early_pgtbl: start= %xu, end=  %xu, size=  %xu\n", early_pgtbl_start, early_pgtbl_end, early_pgtbl_size);
    printk("kernel:      start= %xu, end=  %xu, size=  %xu\n", kernel_start, kernel_end, kernel_size);
}


void zero_bss() 
{
    for (char *p = (char*)(&_bss_start); p < (char*)(&_bss_end); p++) 
    {
        *p = 0;
    }
}

void symbols_init()
{
    zero_bss();
    text_start = (phys_addr_t)&_text_start;
    text_end = (phys_addr_t)&_text_end;
    text_size = (text_end - text_start);

    sched_text_start = (phys_addr_t)&_sched_text_start;
    sched_text_end = (phys_addr_t)&_sched_text_end;
    sched_text_size = sched_text_end - sched_text_start;

    trap_start = (phys_addr_t)&_trap_start;
    trap_end = (phys_addr_t)&_trap_end;
    trap_size = (trap_end - trap_start);

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

    archinitcall_start = (phys_addr_t)&_archinitcall_start;
    archinitcall_end = (phys_addr_t)&_archinitcall_end;
    archinitcall_size = (archinitcall_end - archinitcall_start);

    coreinitcall_start = (phys_addr_t)&_coreinitcall_start;
    coreinitcall_end = (phys_addr_t)&_coreinitcall_end;
    coreinitcall_size = (coreinitcall_end - coreinitcall_start);

    fsinitcall_start = (phys_addr_t)&_fsinitcall_start;
    fsinitcall_end = (phys_addr_t)&_fsinitcall_end;
    fsinitcall_size = (fsinitcall_end - fsinitcall_start);

    deviceinitcall_start = (phys_addr_t)&_deviceinitcall_start;
    deviceinitcall_end = (phys_addr_t)&_deviceinitcall_end;
    deviceinitcall_size = (deviceinitcall_end - deviceinitcall_start);

    lateinitcall_start = (phys_addr_t)&_lateinitcall_start;
    lateinitcall_end = (phys_addr_t)&_lateinitcall_end;
    lateinitcall_size = (lateinitcall_end - lateinitcall_start);


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

    early_pgtbl_start = (phys_addr_t)&_early_pgtbl_start;
    early_pgtbl_end = (phys_addr_t)&_early_pgtbl_end;
    early_pgtbl_size = early_pgtbl_end - early_pgtbl_start;
    print_section();
}

/**
 * @brief 将BSS段中的所有数据清零
 *
 * 遍历BSS段的起始地址到结束地址之间的所有字节，并将它们置为零。
 *
 * BSS段通常用于存储未初始化的全局变量和静态变量，它们在程序启动时不会自动初始化为零。
 * 本函数通过手动遍历并清零这些变量，确保它们在程序启动时是干净的。
 */
