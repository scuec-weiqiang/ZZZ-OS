/* 链接器脚本：RISC-V 用户态 */
OUTPUT_ARCH(riscv)
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
        PROVIDE(__global_pointer$ = . + 0x800);
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
