OUTPUT_ARCH("riscv")
ENTRY(_start)

STACK_SIZE = 4096;          /* 单个核心栈大小（字节） */
CORE_NUM = 2;               /* 核心数量 */
RAM_BASE = 0x80000000;      /* RAM起始地址 */
RAM_SIZE = 0x8000000;       /* RAM大小（128MB） */

MEMORY {
    RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE
}

SECTIONS
{
    . = ORIGIN(RAM);
    .text : ALIGN(4) {
        . = ALIGN(4096);
        _text_start = .;
        *(.text.init)
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
       *(.rodata .rodata.*)
        . = ALIGN(4096);
       _rodata_end = .;
    } > RAM

    .data : ALIGN(8) {
        _data_start = .;
        . = ALIGN(4096);
        *(.data .data.* .sdata)
        . = ALIGN(8);
        _systimer_ctx = .;
        . += 5*8*CORE_NUM;  /* 为每个核心预留定时器上下文空间 */
        . = ALIGN(4096);
        _data_end = .; 
    } > RAM 

    .bss : ALIGN(8) {
        _bss_start = .;
        *(.bss .bss.* .sbss)
        *(COMMON)
        . = ALIGN(4096);
        _bss_end = .;
    } > RAM 

    . = ALIGN(4096);
    _heap_start = .;

    _stack_end = ORIGIN(RAM) + LENGTH(RAM);  /* 栈底（高地址） */
    _stack_start = _stack_end - STACK_SIZE * CORE_NUM;

    _heap_end = _stack_start;
    _heap_size = _heap_end - _heap_start;

    /* ========== 导出符号 ========== */
    
    PROVIDE_HIDDEN(_core_num = CORE_NUM);

    PROVIDE_HIDDEN(_text_start = _text_start);
    PROVIDE_HIDDEN(_text_end = _text_end);
    PROVIDE_HIDDEN(_text_size = _text_end - _text_start);

    PROVIDE_HIDDEN(_trap_start = _trap_start);
    PROVIDE_HIDDEN(_trap_end = _trap_end);
    PROVIDE_HIDDEN(_trap_size = _trap_end - _trap_start);

    PROVIDE_HIDDEN(_rodata_start = _rodata_start);
    PROVIDE_HIDDEN(_rodata_end = _rodata_end);
    PROVIDE_HIDDEN(_rodata_size = _rodata_end - _rodata_start);

    PROVIDE_HIDDEN(_data_start = _data_start);
    PROVIDE_HIDDEN(_data_end = _data_end);
    PROVIDE_HIDDEN(_data_size = _data_end - _data_start);

    PROVIDE_HIDDEN(_bss_start = _bss_start);
    PROVIDE_HIDDEN(_bss_end = _bss_end);
    PROVIDE_HIDDEN(_bss_size = _bss_end - _bss_start);

    PROVIDE_HIDDEN(_stack_start = _stack_start);
    PROVIDE_HIDDEN(_stack_end = _stack_end);
    PROVIDE_HIDDEN(_stack_size = STACK_SIZE);

    PROVIDE_HIDDEN(_heap_start = _heap_start);
    PROVIDE_HIDDEN(_heap_end = _heap_end);
    PROVIDE_HIDDEN(_heap_size = _heap_size);

    PROVIDE_HIDDEN(_systimer_ctx = _systimer_ctx);
}