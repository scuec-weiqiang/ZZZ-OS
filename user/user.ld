/* 链接器脚本：linker.ld */
OUTPUT_ARCH(riscv)
ENTRY(_start)  /* 指定入口点符号 */

SECTIONS
{
    /* 文本段 */
    . = 0x10000;        /* 设置当前位置为 0x10000 */
    .text : {
        *(.text .text.*) /* 所有文本段 */
    }
    
    /* 只读数据段 */
    .rodata : {
        *(.rodata .rodata.*)
    }
    
    /* 数据段 */
    .data : {
        *(.data .data.*)
    }
    
    /* BSS 段（未初始化数据）*/
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    
    /* 丢弃其他不需要的段 */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}