/* 链接器脚本：linker.ld */
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
    /* 文本段 */
    . = 0x10000;        
    .text ALIGN(8): {
        *(.text.startup)
        *(.text .text.*)
    }
    
    /* 只读数据段 */
    .rodata ALIGN(8): {
        *(.rodata .rodata.* .srodata .srodata.*) /* 所有只读数据段 */
    }
    
    /* 数据段 */
    .data ALIGN(8): {
        *(.data .data.* .sdata .sdata.*) /* 所有数据段 */
    }
    
    /* BSS 段（未初始化数据）*/
    .bss ALIGN(8): {
        *(.bss .bss.* .sbss)
        *(COMMON)
    }

    . = ALIGN(8);
    _end = .;
    
    /* 丢弃其他不需要的段 */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
