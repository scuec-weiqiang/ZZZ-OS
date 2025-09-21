#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "types.h"
extern uintptr_t text_start;
extern uintptr_t text_end;
extern uintptr_t text_size;
extern uintptr_t rodata_start;
extern uintptr_t rodata_end;
extern uintptr_t rodata_size;
extern uintptr_t data_start;
extern uintptr_t data_end;
extern uintptr_t data_size;
extern uintptr_t bss_start;
extern uintptr_t bss_end;
extern uintptr_t bss_size;
extern uintptr_t stack_start;
extern uintptr_t stack_end;
extern uintptr_t stack_size;
extern uintptr_t heap_start;
extern uintptr_t heap_end;
extern uintptr_t heap_size;
extern uintptr_t core_num;

extern void symbols_init();
extern void zero_bss();

#endif