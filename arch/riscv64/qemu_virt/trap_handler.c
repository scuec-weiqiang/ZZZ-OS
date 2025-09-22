#include "uart.h"
#include "printk.h"
#include "riscv.h"
#include "systimer.h"
#include "swtimer.h"
#include "plic.h"
#include "uart.h"
#include "sched.h"
#include "clint.h"
#include "vm.h"

extern void kernel_trap_entry(void); //定义在trap.S文件中

reg_t timer_interrupt_handler(reg_t epc);
void extern_interrupt_handler();
void page_fault_handler(uintptr_t addr);
reg_t soft_interrupt_handler(reg_t epc);

void trap_init()
{
    stvec_w((reg_t)kernel_trap_entry);
}


/***************************************************************
 * @description: 
 * @param {u32} mcause [in/out]:  
 * @param {u32} mtval [in/out]:  
 * @param {u32} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t trap_handler(reg_t epc,reg_t cause,reg_t ctx)
{
    reg_t return_epc = epc;
    u64 cause_code = cause & MCAUSE_MASK_CAUSECODE;
    // printk("in trap_handler, epc is %x\n",epc);
    if((cause & MCAUSE_MASK_INTERRUPT))
    {
        switch (cause_code)
        {
            case 1:
                // printk("Supervisor software interruption!\n");
                return_epc = soft_interrupt_handler(epc);
                // return_epc += 4;
                // printk("out trap_handler, return_epc is %x\n",return_epc);
                break;
            case 5:
                // printk("timer interruption!\n");
                return_epc =  timer_interrupt_handler(epc);
            case 7:
                // printk("timer interruption!\n");
                return_epc =  timer_interrupt_handler(epc);
                break;
            case 8:
                panic("sys_exit!\n");
                break;
            // printk("timer interruption!\n");
            return_epc =  timer_interrupt_handler(epc);
                break;
            case 11:
                // printk("external interruption!\n");
                // extern_interrupt_handler();
                break;
            default:
                printk("unknown async exception!\n cause code is %l\n",cause_code);
                printk("mstatus:%x,mie:%x\n",mstatus_r(),mie_r());
                break;
        }
    }
    else
    {
        printk("\nstval is %xu\n",stval_r());
        printk("occour in %xu\n",epc);
        switch (cause_code)
        {
            case 0:
                panic("Instruction address misaligned!\n");
                break;
            case 1:
                panic("Instruction access fault!\n");
                break;
            case 2:
                panic("Illegal instruction !\n");
                break;
            case 3:
                printk("Breakpiont!\n");
                break;
            case 4:
                panic("Load address misaligned\n");
                break;
            case 5:
                panic("Load access fault\n");
                break;
            case 6:
                panic("Store/AMO address misaligned\n");
                break;
            case 7:
                // panic("\033[32mStore/AMO access fault\n\033[0m");
                panic("Store/AMO access fault\n");
                break;    
            case 8:
                // printk("Environment call from U-mode\n");
                // extern do_syscall(struct reg_context *ctx);
                // do_syscall(ctx);
                return_epc += 4;
                break;
            case 9:
                // printk("Environment call from S-mode\n");
                panic("Environment call from S-mode\n");
                break;
            case 11:
                // printk("Environment call from M-mode");
                panic("Environment call from M-mode\n");
                break;
            case 12:
                panic("Instruction page fault\n");
                break;
            case 13:
                panic("Load page fault\n");
                // page_fault_handler(stval_r());
                break;
            case 15:
                panic("Store/AMO page fault\n");
                // page_fault_handler(stval_r());
                break;
            default:
                panic("unknown sync exception!\ntrap!\n");
                break;
        }
    }
    return return_epc;
}

/***************************************************************
 * @description: 
 * @param {u32} mepc [in/out]:  
 * @return {*}
***************************************************************/
void extern_interrupt_handler()
{
    enum hart_id hart_id = mhartid_r();
    u32 irqn = __plic_claim(hart_id);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
        break;

        default:
            printk("unexpected extern interrupt!");
        break;
    }
    if(irqn)
    {
        __plic_complete(0,irqn);
    }
}

/***************************************************************
 * @description: 
 * @param {u32} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t timer_interrupt_handler(reg_t epc )
{
    reg_t r;
    enum hart_id hart_id = tp_r();
    // printk("hart %d timer interrupt!\n",hart_id);
    // sip_w(sip_r() | SIP_SSIP);
    // u64 now_time = systimer_get_time();
    systimer_tick++;

    systimer_load(hart_id,systimer_hz[hart_id]);

    // r = sched(epc,now_time,hart_id);
    
    // swtimer_check();
    return epc;
    // return r;
}

reg_t soft_interrupt_handler(reg_t epc)
{
    sip_w(sip_r() & ~SIP_SSIP);
    
    // reg_t r;
    // enum hart_id hart_id = tp_r();
    // printk("hart %d timer interrupt!\n",hart_id);
    // u64 now_time = systimer_get_time();
    // printk("now time is %d\n",now_time);
    systimer_tick++;

    // r = sched(epc,now_time,hart_id);
    
    // swtimer_check();
    
    return epc;
    
}

void page_fault_handler(uintptr_t addr)
{
    // satp_w(satp_r() & ~(SATP_MODE)); // 切换到bare模式
    // // 检查是否为合法地址
    // if (addr < RAM_BASE || addr >= (RAM_BASE + RAM_SIZE)) 
    // {
    //     printk("Invalid page fault at %x\n", addr);
    //     panic("Page fault");
    // }
    
    // // 分配物理页并建立映射
    // // uintptr_t pa = (uintptr_t)page_alloc(1);
    // // if (!pa) panic("Out of memory");
    
    // // 设置映射 (RW权限)
    // map_range(kernel_pgd, addr, addr, PAGE_SIZE, PTE_R | PTE_W);
    
    // printk("Handled page fault: VA=%x -> PA=%x\n", addr, addr);
    // satp_w(satp_r() | SATP_MODE); 
    // // 刷新TLB
    // asm volatile("sfence.vma");
}