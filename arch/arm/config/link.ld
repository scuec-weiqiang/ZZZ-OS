OUTPUT_ARCH("arm")
ENTRY(_start)

STACK_SIZE = 0x8000;    /* 所有模式一共 32KB 的栈空间 ，每个8KB */
RAM_BASE = 0xc0200000; 
SECTIONS
{
    . = RAM_BASE;
    .text : ALIGN(4096)  {
        __kernel_start = .;
        __text_start = .;
        *(.text.entry)
        __sched_text_start = .;
        *(.text.sched)
        __sched_text_eend = .;
        *(.text .text.*)
        . = ALIGN(4096);
        __text_end = .;
    }

    .trap : ALIGN(4096) {
        __trap_start = .;  /* trap处理函数起始地址 */
        *(.trap* )
        __trap_end = .;    /* trap处理函数结束地址 */
    }

    .initcall : ALIGN(4096) {
        __initcall_start = .;
        KEEP(*(.initcall))     /* 保留所有 initcall 段内容 */
        __initcall_end = .;
    }

    .archinitcall : ALIGN(4096) {
        __archinitcall_start = .;
        KEEP(*(.archinitcall))
        __archinitcall_end = .;
    }

    .coreinitcall : ALIGN(4096) {
        __coreinitcall_start = .;
        KEEP(*(.coreinitcall))
        __coreinitcall_end = .;
    }

    .fsinitcall : ALIGN(4096) {
        __fsinitcall_start = .;
        KEEP(*(.fsinitcall))
        __fsinitcall_end = .;
    }

    .deviceinitcall : ALIGN(4096) {
        __deviceinitcall_start = .;
        KEEP(*(.deviceinitcall))
        __deviceinitcall_end = .;
    }

    .lateinitcall : ALIGN(4096) {
        __lateinitcall_start = .;
        KEEP(*(.lateinitcall))
        __lateinitcall_end = .;
    }

    .exitcall : ALIGN(4096)  {
        __exitcall_start = .;
        KEEP(*(.exitcall))     /* 保留所有 exitcall 段内容 */
        __exitcall_end = .;
    }
    
    .irqinitcall :  ALIGN(4096) {
        __irqinitcall_start = .;
        __irqchip_of_table = .;
        KEEP(*(__irqchip_of_table))     /* 保留所有 initcall 段内容 */
        __timerchip_of_table = .;
        KEEP(*(__timerchip_of_table))
        __irqinitcall_end = .;
    }
    
    .irqexitcall : ALIGN(4096) {
        __irqexitcall_start = .;
        KEEP(*(.irqexitcall))     /* 保留所有 exitcall 段内容 */
        __irqexitcall_end = .;
    }
    
    .rodata : ALIGN(4096) {
        . = ALIGN(4096);
       __rodata_start = .; 
       *(.rodata .rodata.* .srodata .srodata.*)
        . = ALIGN(4096);
       __rodata_end = .;
    }

    .data : ALIGN(4096) {
        __data_start = .;
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(4096);
        __data_end = .; 
    } 

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss .bss.* .sbss )
        *(COMMON)
        . = ALIGN(4096);
        __bss_end = .;
    } 

    .stack : ALIGN(4096) {
        __early_stack_start = .;
        /* 为主核心分配独立的栈空间，用来做基本的初始化，其他核心等操作系统初始化完毕后再分配 */
        . = . + STACK_SIZE;
        __early_stack_end = .;  /* 记录早期栈的结束地址 */
    }

    .early_pgtbl : ALIGN(4096*4) {
        . = ALIGN(4096*4);
        __early_pgtbl_start = .;
        . = . + (4096 * 4);  /* 为早期页表分配16KB空间，足够映射4GB内存 */
        __early_pgtbl_end = .;
        __kernel_end = .;  /* 记录内核结束地址 */
    }

    /DISCARD/ : {
        *(.note*)
        *(.comment)
        *(.note)         /* 如果需要保留其他 note，请保留 */
        *(.eh_frame)     /* 通常不需要 */
    }
    
    PROVIDE_HIDDEN(_text_start = __text_start);
    PROVIDE_HIDDEN(_text_end = __text_end);

    PROVIDE_HIDDEN(_sched_text_start = __sched_text_start);
    PROVIDE_HIDDEN(_sched_text_end = __sched_text_end);

    PROVIDE_HIDDEN(_initcall_start = __initcall_start);
    PROVIDE_HIDDEN(_initcall_end = __initcall_end);

    PROVIDE_HIDDEN(_archinitcall_start = __archinitcall_start);
    PROVIDE_HIDDEN(_archinitcall_end = __archinitcall_end);

    PROVIDE_HIDDEN(_coreinitcall_start = __coreinitcall_start);
    PROVIDE_HIDDEN(_coreinitcall_end = __coreinitcall_end);

    PROVIDE_HIDDEN(_fsinitcall_start = __fsinitcall_start);
    PROVIDE_HIDDEN(_fsinitcall_end = __fsinitcall_end);

    PROVIDE_HIDDEN(_deviceinitcall_start = __deviceinitcall_start);
    PROVIDE_HIDDEN(_deviceinitcall_end = __deviceinitcall_end);

    PROVIDE_HIDDEN(_lateinitcall_start = __lateinitcall_start);
    PROVIDE_HIDDEN(_lateinitcall_end = __lateinitcall_end);

    PROVIDE_HIDDEN(_exitcall_start = __exitcall_start);
    PROVIDE_HIDDEN(_exitcall_end = __exitcall_end);

    PROVIDE_HIDDEN(_irqinitcall_start = __irqinitcall_start);
    PROVIDE_HIDDEN(_irqinitcall_end = __irqinitcall_end);

    PROVIDE_HIDDEN(_irqexitcall_start = __irqexitcall_start);
    PROVIDE_HIDDEN(_irqexitcall_end = __irqexitcall_end);
    PROVIDE_HIDDEN(_trap_start = __trap_start);
    PROVIDE_HIDDEN(_trap_end = __trap_end);

    PROVIDE_HIDDEN(_rodata_start = __rodata_start);
    PROVIDE_HIDDEN(_rodata_end = __rodata_end);

    PROVIDE_HIDDEN(_data_start = __data_start);
    PROVIDE_HIDDEN(_data_end = __data_end);

    PROVIDE_HIDDEN(_bss_start = __bss_start);
    PROVIDE_HIDDEN(_bss_end = __bss_end);

    PROVIDE_HIDDEN(_kernel_start = __kernel_start);
    PROVIDE_HIDDEN(_kernel_end = __kernel_end);

    PROVIDE_HIDDEN(_early_stack_start = __early_stack_start);
    PROVIDE_HIDDEN(_early_stack_end = __early_stack_end); 

    PROVIDE_HIDDEN(_early_pgtbl_start = __early_pgtbl_start);
    PROVIDE_HIDDEN(_early_pgtbl_end = __early_pgtbl_end);
}
