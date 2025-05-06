OUTPUT_ARCH("riscv")
ENTRY(_start)

STACK_SIZE = 4096;
NUM_CORES = 1;

RAM_BASE = 0x80000000;
RAM_SIZE = 128M;


MEMORY {
    RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE
}

SECTIONS
{
    . = ORIGIN(RAM);
    .text : {
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

   .kernel_reg_ctx (NOLOAD) : {
        . = ALIGN(8);
        _kernel_reg_ctx_start = .;
        . += 8 * 33;
        _kernel_reg_ctx_end = .;
   } > RAM

    . = ALIGN(4096);
    _heap_start = .;

    _stack_end = RAM_BASE + RAM_SIZE;
    _stack_start = _stack_end - STACK_SIZE * NUM_CORES;

    _heap_end = _stack_start;
    _heap_size = _heap_end - _heap_start;

}
