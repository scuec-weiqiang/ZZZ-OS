OUTPUT_ARCH("riscv")
ENTRY(_start)

STACK_SIZE = 4096;          /* 单个核心栈大小（字节） */
CORE_NUM = 2;               /* 核心数量 */

/* 物理内存基地址 */
RAM_BASE = 0x80000000;
RAM_SIZE = 128M;

/* 内核虚拟地址基址（高半区） */
KERNEL_BASE = 0xffffffffc0200000;

MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH =  128M
}

SECTIONS
{
    . = KERNEL_BASE;

    .text : AT(RAM_BASE) {
        . = ALIGN(4096);
        _text_start = .;
        *(.text.init)
        _trap_start = .;
        *(.text.trap)
        _trap_end = .;
        *(.text .text.*)
        . = ALIGN(4096);
        _text_end = .;
    } > RAM

    .rodata : AT(RAM_BASE + (_rodata_start - KERNEL_BASE)) {
        . = ALIGN(4096);
        _rodata_start = .;
        *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4096);
        _rodata_end = .;
    } > RAM

    .data : AT(RAM_BASE + (_data_start - KERNEL_BASE)) {
        _data_start = .;
        . = ALIGN(4096);
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(8);
        _systimer_ctx = .;
        . += 5*8*CORE_NUM;
        . = ALIGN(4096);
        _data_end = .;
    } > RAM

    .bss : AT(RAM_BASE + (_bss_start - KERNEL_BASE)) {
        _bss_start = .;
        *(.bss .bss.* .sbss)
        *(COMMON)
        . = ALIGN(4096);
        _bss_end = .;
    } > RAM

    . = ALIGN(4096);
    _heap_start = .;

    _stack_end = KERNEL_BASE + RAM_SIZE;       /* 栈底（高地址） */
    _stack_start = _stack_end - STACK_SIZE * CORE_NUM;

    _heap_end = _stack_start;

    /* 符号导出（保持不变） */
    PROVIDE_HIDDEN(_core_num = CORE_NUM);

    PROVIDE_HIDDEN(__text_start = _text_start);
    PROVIDE_HIDDEN(__text_end = _text_end);


    PROVIDE_HIDDEN(__trap_start = _trap_start);
    PROVIDE_HIDDEN(__trap_end = _trap_end);


    PROVIDE_HIDDEN(__rodata_start = _rodata_start);
    PROVIDE_HIDDEN(__rodata_end = _rodata_end);


    PROVIDE_HIDDEN(__data_start = _data_start);
    PROVIDE_HIDDEN(__data_end = _data_end);

    PROVIDE_HIDDEN(__bss_start = _bss_start);
    PROVIDE_HIDDEN(__bss_end = _bss_end);
    PROVIDE_HIDDEN(__bss_size = _bss_end - _bss_start);

    PROVIDE_HIDDEN(__stack_start = _stack_start);
    PROVIDE_HIDDEN(__stack_end = _stack_end);


    PROVIDE_HIDDEN(__heap_start = _heap_start);
    PROVIDE_HIDDEN(__heap_end = _heap_end);

    PROVIDE_HIDDEN(__systimer_ctx = _systimer_ctx);
}
