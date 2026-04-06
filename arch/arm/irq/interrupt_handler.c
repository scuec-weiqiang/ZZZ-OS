/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/interrupt_handler.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-18 16:16:46
 * @LastEditTime : 2026-03-26 19:50:42
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/types.h>
#include <os/printk.h>
#include <asm/irq.h>

static inline uint32_t read_dfar(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c6,c0,0":"=r"(v));
    return v;
}

static inline uint32_t read_dfsr(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c5,c0,0":"=r"(v));
    return v;
}

static inline uint32_t read_ttbr0(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c2,c0,0":"=r"(v));
    return v;
}

static inline uint32_t read_ttbr1(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c2,c0,1":"=r"(v));
    return v;
}

static inline uint32_t read_ttbcr(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c2,c0,2":"=r"(v));
    return v;
}

static inline uint32_t read_dacr(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c3,c0,0":"=r"(v));
    return v;
}

static inline uint32_t read_ifar(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c6,c0,2":"=r"(v));
    return v;
}

static inline uint32_t read_ifsr(void)
{
    uint32_t v;
    asm volatile("mrc p15,0,%0,c5,c0,1":"=r"(v));
    return v;
}
static void exception(char *s) {
    printk("%s exception\n", s);
    while (1) {
    }
}

void reset_handler(void)
{
    exception("reset");
}

void reserved_handler(void)
{
    exception("reserved");
}

void undef_handler(void)
{
   exception("undef");
}

void swi_handler(void)
{
    exception("swi");
}

void prefetch_abort_handler(void)
{
    uint32_t ifar = read_ifar();
    uint32_t ifsr = read_ifsr();
    uint32_t fs = ((ifsr & 0xF) | ((ifsr >> 6) & 0x10));
    printk("PREFETCH ABORT ifar=%xu ifsr=%xu fs=%xu\n", ifar, ifsr, fs);
    exception("prefetch");
}


reg_t irq_handler(reg_t ctx)
{
    // printk("IRQ exception\n");
    if (handle_arch_irq) {
        // printk("handle_arch_irq registered, call it\n");
        return handle_arch_irq(&ctx);
    } else {
        printk("No arch irq handler registered\n");
        exception("irq");
        return ctx;
    }
        
}

void fiq_handler(void)
{
    exception("fiq");
}



void data_abort_handler(void)
{
    printk("DATA ABORT ");
    uint32_t addr = read_dfar();
    uint32_t stat = read_dfsr();
    uint32_t ttbr0 = read_ttbr0();
    uint32_t ttbr1 = read_ttbr1();
    uint32_t ttbcr = read_ttbcr();
    uint32_t dacr = read_dacr();

    printk("addr=%xu stat=%xu\n", addr, stat);
    printk("TTBR0=%xu TTBR1=%xu TTBCR=%xu DACR=%xu\n",
           ttbr0, ttbr1, ttbcr, dacr);


    while (1);
}
