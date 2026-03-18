// #include "os/mm/pgtbl_types.h"
// #include "os/mm/vma_flags.h"
// #include "os/types.h"
#include <asm/pgtbl.h>
#include <os/printk.h>

#define UART1_BASE 0x02020000

#define UART_UTXD (*(volatile unsigned int *)(UART1_BASE + 0x40))
#define UART_UTS  (*(volatile unsigned int *)(UART1_BASE + 0xB4))

void putc(char c)
{
    if (c == '\n')
        putc('\r');

    while ((UART_UTS >> 4) & 1) {
    }

    UART_UTXD = c;
}

void puts(char *s)
{
    while (*s)
        putc(*s++);
}

// extern char _early_pgtbl_start[];
extern void vector_table(void);



void boot_main()
{
    putc('c');
    printk("into boot_main\n");
    uint32_t addr;
    __asm__ volatile( "mov %0, pc" : "=r"(addr) );
    printk("boot_main: current PC = %xu\n", addr);
    printk("vector=%xu\n", vector_table);
    uint32_t val = *(volatile uint32_t*)vector_table;
    printk("vector[0]=%xu\n", val);
    // ((void (*)(void))vector_table)();
    asm volatile(
    "ldr r0, =vector_table\n"
    "bx r0\n"
);
    while (1) {}
}
static inline uint32_t read_dfar(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c6,c0,0":"=r"(v));
    return v;
}

void default_exception(void)
{
    printk("default exception\n");
    while (1) {
    }
}
void reset_handler(void)
{
    default_exception();
}

void reserved_handler(void)
{
    default_exception();
}

void undef_handler(void)
{
    default_exception();
}

void swi_handler(void)
{
    default_exception();
}

void prefetch_abort_handler(void)
{
    default_exception();
}

void irq_handler(void)
{
    default_exception();
}

void fiq_handler(void)
{
    default_exception();
}

static inline uint32_t read_dfsr(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c5,c0,0":"=r"(v));
    return v;
}

void data_abort_handler(void)
{
    printk("DATA ABORT ");
    uint32_t addr = read_dfar();
    uint32_t stat = read_dfsr();

    printk("addr=%xu stat=%xu\n", addr, stat);

    while (1);
}