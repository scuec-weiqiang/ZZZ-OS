OUTPUT_ARCH("riscv")
ENTRY(_start)

STACK_SIZE = 8192;          /* 单个核心栈大小（字节） */
CORE_NUM = 2;               /* 核心数量 */
RAM_BASE = 0xffffffffc0200000;      /* RAM起始地址 */
RAM_SIZE = 126M;       /* RAM大小（128MB） */

MEMORY {
    RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE
}

SECTIONS
{
    . = ORIGIN(RAM);
    .text : ALIGN(4) {
        _text_start = .;
        *(.text.entry)
        _trap_start = .;  /* trap处理函数起始地址 */
        *(.text.trap)
        _trap_end = .;    /* trap处理函数结束地址 */
        *(.text .text.*)
        . = ALIGN(4096);
        _text_end = .;
    } > RAM

    .rodata : ALIGN(8) {
        . = ALIGN(4096);
       _rodata_start = .; 
       *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4096);
       _rodata_end = .;
    } > RAM

    .data : ALIGN(8) {
        _data_start = .;
        . = ALIGN(4096);
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(4096);
        _data_end = .; 
    } > RAM 

    .bss : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.* .sbss )
        *(COMMON)
        . = ALIGN(4096);
        _bss_end = .;
    } > RAM 

    . = ALIGN(4096);
    _heap_start = .;

    _stack_end = RAM_BASE + RAM_SIZE;       /* 栈底（高地址） */
    _stack_start = _stack_end - STACK_SIZE * CORE_NUM;

    _heap_end = _stack_start;

    /* 符号导出（保持不变） */
    PROVIDE_HIDDEN(__core_num = CORE_NUM);

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

    PROVIDE_HIDDEN(__stack_start = _stack_start);
    PROVIDE_HIDDEN(__stack_end = _stack_end);

    PROVIDE_HIDDEN(__heap_start = _heap_start);
    PROVIDE_HIDDEN(__heap_end = _heap_end);
}
