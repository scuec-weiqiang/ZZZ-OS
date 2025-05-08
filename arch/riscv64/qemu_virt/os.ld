OUTPUT_ARCH("riscv")
ENTRY(_start)

STACK_SIZE        4096          /* 单个核心栈大小（字节） */
CORE_NUM          2             /* 核心数量 */
RAM_BASE          0x80000000    /* RAM起始地址 */
RAM_SIZE          0x8000000     /* RAM大小（128MB） */

MEMORY {
    RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE
}

SECTIONS
{
    . = ORIGIN(RAM);
    .text : ALIGN(4) {
        _text_start = .;
        *(.text.init)
        *(.text.trap)
        *(.text .text.*)
        _text_end = .;
    } > RAM

    .rodata : ALIGN(8) {
       _rodata_start = .; 
       *(.rodata .rodata.*)
       _rodata_end = .;
    } > RAM

    .data : ALIGN(8) {
        _data_start = .;
        *(.data .data,*)
        . = ALIGN(8);
        _systimer_ctx = .;
        . += 5*8*CORE_NUM;
        . = ALIGN(8);
        _data_end = .; 
    } > RAM 

    .bss : ALIGN(8) {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _bss_end = .;
    } > RAM 

    /* 内核栈（每个核心独立，高地址向下增长，符合RISC-V栈约定） */
    . = RAM_BASE + RAM_SIZE;    /* 从RAM末端向上分配栈空间 */

    .kernel_stack : ALIGN(16) { /* 栈通常需要16字节对齐以满足ABI要求 */
        /* 每个核心栈独立，地址从高到低排列（core0栈顶在最高地址） */
        _kernel_stack_end = .;  /* 栈底（高地址，栈向下增长） */
        . -= STACK_SIZE * CORE_NUM;
        _kernel_stack_start = .; /* 栈起始地址（所有核心栈的基址） */
        /* 单个核心栈地址计算：
         * core_stack[hart_id] = _kernel_stack_start + hart_id * STACK_SIZE
         */
    } >RAM

    /* ========== 堆与保留区域 ========== */
    . = ALIGN(4096);            /* 堆起始地址4KB对齐（页大小） */
    _heap_start = .;

    /* 堆结束于内核栈起始地址之前 */
    _heap_end = _kernel_stack_start;
    _heap_size = _heap_end - _heap_start;

    /* 导出符号（方便C代码引用，明确用途） */
    PROVIDE_HIDDEN(_stack_size = STACK_SIZE);
    PROVIDE_HIDDEN(_core_num = CORE_NUM);

    PROVIDE_HIDDEN(_text_start = _text_start);
    PROVIDE_HIDDEN(_text_end = _text_end);

    PROVIDE_HIDDEN(_rodata_start = _rodata_start);
    PROVIDE_HIDDEN(_rodata_end = _rodata_end);

    PROVIDE_HIDDEN(_data_start = _data_start);
    PROVIDE_HIDDEN(_data_end = _data_end);

    PROVIDE_HIDDEN(_bss_start = _bss_start);
    PROVIDE_HIDDEN(_bss_end = _bss_end);

    PROVIDE_HIDDEN(_kernel_stack_start = _kernel_stack_start);
    PROVIDE_HIDDEN(_kernel_stack_end = _kernel_stack_end);

    PROVIDE_HIDDEN(_heap_start = _heap_start);
    PROVIDE_HIDDEN(_heap_end = _heap_end);
    PROVIDE_HIDDEN(_heap_size = _heap_size);

    PROVIDE_HIDDEN(_systimer_ctx = _systimer_ctx);
}
