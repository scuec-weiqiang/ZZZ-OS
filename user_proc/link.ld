/* 链接器脚本：linker.ld */
OUTPUT_ARCH(arm)  /* 指定目标架构 */
ENTRY(main)  /* 指定入口点符号 */

SECTIONS
{
    /* 文本段 */
    . = 0x40000;        
    .text : {
        *(.text .text.*) /* 所有文本段 */
    }
    
    /* 只读数据段 */
    .rodata : {
        *(.rodata .rodata.* .srodata .srodata.*) /* 所有只读数据段 */
    }
    
    /* 数据段 */
    .data : {
        *(.data .data.* .sdata .sdata.*) /* 所有数据段 */
    }
    
    /* BSS 段（未初始化数据）*/
    .bss : {
        *(.bss .bss.* .sbss)
        *(COMMON)
    }
    
    /* 丢弃其他不需要的段 */
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}