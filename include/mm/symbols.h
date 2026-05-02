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
extern size_t text_size;
extern phys_addr_t sched_text_start;
extern phys_addr_t sched_text_end;
extern size_t sched_text_size;
extern phys_addr_t trap_start;
extern phys_addr_t trap_end;
extern size_t trap_size;
extern phys_addr_t rodata_start;
extern phys_addr_t rodata_end;
extern size_t rodata_size;
extern phys_addr_t data_start;
extern phys_addr_t data_end;
extern size_t data_size;
extern phys_addr_t bss_start;
extern phys_addr_t bss_end;
extern size_t bss_size;
extern phys_addr_t stack_start;
extern phys_addr_t stack_end;
extern size_t stack_size;
extern phys_addr_t heap_start;
extern phys_addr_t heap_end;
extern size_t heap_size;
extern phys_addr_t initcall_start;
extern phys_addr_t initcall_end;
extern size_t initcall_size;
extern phys_addr_t archinitcall_start;
extern phys_addr_t archinitcall_end;
extern size_t archinitcall_size;
extern phys_addr_t coreinitcall_start;
extern phys_addr_t coreinitcall_end;
extern size_t coreinitcall_size;
extern phys_addr_t fsinitcall_start;
extern phys_addr_t fsinitcall_end;
extern size_t fsinitcall_size;
extern phys_addr_t deviceinitcall_start;
extern phys_addr_t deviceinitcall_end;
extern size_t deviceinitcall_size;
extern phys_addr_t lateinitcall_start;
extern phys_addr_t lateinitcall_end;
extern size_t lateinitcall_size;
extern phys_addr_t exitcall_start;
extern phys_addr_t exitcall_end;
extern size_t exitcall_size;
extern phys_addr_t irqinitcall_start;
extern phys_addr_t irqinitcall_end;
extern size_t irqinitcall_size;
extern phys_addr_t irqexitcall_start;
extern phys_addr_t irqexitcall_end;
extern size_t irqexitcall_size;
extern phys_addr_t kernel_start;
extern phys_addr_t kernel_end;
extern size_t kernel_size;
extern phys_addr_t early_stack_start;
extern phys_addr_t early_stack_end;
extern size_t early_stack_size;
extern phys_addr_t early_pgtbl_start;
extern phys_addr_t early_pgtbl_end;
extern size_t early_pgtbl_size;

extern void symbols_init();
extern void print_section();
extern void zero_bss();

static inline int in_sched_functions(unsigned long ptr) {
    return (ptr >= sched_text_start) && (ptr < sched_text_end);
}

static inline int in_exception_text(unsigned long ptr) {
	return (ptr >= trap_start) && (ptr < trap_end);
}

#endif
