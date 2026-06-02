/* 链接器脚本：ARM 用户态 */
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;
    .text ALIGN(8): {
        *(.text.startup)
        *(.text .text.*)
    }

    .rodata ALIGN(8): {
        *(.rodata .rodata.* .srodata .srodata.*)
    }

    .data ALIGN(8): {
        *(.data .data.* .sdata .sdata.*)
    }

    .bss ALIGN(8): {
        *(.bss .bss.* .sbss)
        *(COMMON)
    }

    . = ALIGN(8);
    _end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
    }
}
