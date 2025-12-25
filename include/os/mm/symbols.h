/**
 * @FilePath: /ZZZ-OS/include/os/mm/symbols.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-01 16:47:45
 * @LastEditTime: 2025-11-17 21:12:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <os/types.h>
extern phys_addr_t text_start;
extern phys_addr_t text_end;
extern phys_addr_t text_size;
extern phys_addr_t rodata_start;
extern phys_addr_t rodata_end;
extern phys_addr_t rodata_size;
extern phys_addr_t data_start;
extern phys_addr_t data_end;
extern phys_addr_t data_size;
extern phys_addr_t bss_start;
extern phys_addr_t bss_end;
extern phys_addr_t bss_size;
extern phys_addr_t stack_start;
extern phys_addr_t stack_end;
extern phys_addr_t stack_size;
extern phys_addr_t heap_start;
extern phys_addr_t heap_end;
extern phys_addr_t heap_size;
extern phys_addr_t initcall_start;
extern phys_addr_t initcall_end;
extern phys_addr_t initcall_size;
extern phys_addr_t exitcall_start;
extern phys_addr_t exitcall_end;
extern phys_addr_t exitcall_size;
extern phys_addr_t irqinitcall_start;
extern phys_addr_t irqinitcall_end;
extern phys_addr_t irqinitcall_size;
extern phys_addr_t irqexitcall_start;
extern phys_addr_t irqexitcall_end;
extern phys_addr_t irqexitcall_size;
extern phys_addr_t kernel_start;
extern phys_addr_t kernel_end;
extern phys_addr_t kernel_size;
extern phys_addr_t early_stack_start;
extern phys_addr_t early_stack_end;
extern phys_addr_t early_stack_size;

extern void symbols_init();
extern void zero_bss();

#endif