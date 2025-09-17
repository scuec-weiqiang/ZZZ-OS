#include "types.h"

extern char __text_start[], __text_end[];
extern char __trap_start[], __trap_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __data_end[];
extern char __bss_start[], __bss_end[];
extern char __stack_start[], __stack_end[];
extern char __heap_start[], __heap_end[];
extern char __systimer_ctx[];

uintptr_t _text_start;
uintptr_t _text_end;
uintptr_t _text_size;
uintptr_t _trap_start;
uintptr_t _trap_end;
uintptr_t _trap_size;
uintptr_t _rodata_start;
uintptr_t _rodata_end;
uintptr_t _rodata_size;
uintptr_t _data_start;
uintptr_t _data_end;
uintptr_t _data_size;
uintptr_t _bss_start;
uintptr_t _bss_end;
uintptr_t _bss_size;
uintptr_t _stack_start;
uintptr_t _stack_end;
uintptr_t _stack_size;
uintptr_t _heap_start;
uintptr_t _heap_end;
uintptr_t _heap_size;
uintptr_t _systimer_ctx[5];

void maddr_def_init()
{
    _text_start = (uintptr_t)&__text_start;
    _text_end = (uintptr_t)&__text_end;
    _text_size = (_text_end - _text_start);
    _trap_start = (uintptr_t)&__trap_start;
    _trap_end = (uintptr_t)&__trap_end;
    _trap_size = (_trap_end - _trap_start);
    _rodata_start = (uintptr_t)&__rodata_start;
    _rodata_end = (uintptr_t)&__rodata_end;
    _rodata_size = (_rodata_end - _rodata_start);
    _data_start = (uintptr_t)&__data_start;
    _data_end = (uintptr_t)&__data_end;
    _data_size = (_data_end - _data_start);
    _bss_start = (uintptr_t)&__bss_start;
    _bss_end = (uintptr_t)&__bss_end;
    _bss_size = (_bss_end - _bss_start);
    _stack_start = (uintptr_t)&__stack_start;
    _stack_end = (uintptr_t)&__stack_end;
    _stack_size = (_stack_end - _stack_start);
    _heap_start = (uintptr_t)&__heap_start;
    _heap_end = (uintptr_t)&__heap_end;
    _heap_size = (_heap_end - _heap_start);
    _systimer_ctx[0] = (uintptr_t)&__systimer_ctx;
    _systimer_ctx[1] = (uintptr_t)&__systimer_ctx + 1;
    _systimer_ctx[2] = (uintptr_t)&__systimer_ctx + 2;
    _systimer_ctx[3] = (uintptr_t)&__systimer_ctx + 3;
    _systimer_ctx[4] = (uintptr_t)&__systimer_ctx + 4;
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
    for (char *p = _bss_start; p < _bss_end; p++) {
        *p = 0;
    }
}