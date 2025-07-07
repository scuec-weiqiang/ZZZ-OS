#include "uart.h"
#include "printf.h"
#include "riscv.h"
#include "systimer.h"
#include "swtimer.h"
#include "plic.h"
#include "uart.h"
#include "sched.h"
#include "clint.h"
#include "vm.h"

extern void kernel_trap_entry(void); //定义在trap.S文件中
extern void machine_timer_trap_entry(void); //定义在trap.S文件中

reg_t timer_interrupt_handler(reg_t epc);
void extern_interrupt_handler();
void page_fault_handler(addr_t addr);
reg_t soft_interrupt_handler(reg_t epc);

void trap_init()
{
    stvec_w((reg_t)kernel_trap_entry);
    mtvec_w((reg_t)machine_timer_trap_entry);
}


/***************************************************************
 * @description: 
 * @param {uint32_t} mcause [in/out]:  
 * @param {uint32_t} mtval [in/out]:  
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t trap_handler(reg_t epc,reg_t cause,reg_t ctx)
{
    reg_t return_epc = epc;
    uint64_t cause_code = cause & MCAUSE_MASK_CAUSECODE;
    // printf("in trap_handler, epc is %x\n",epc);
    if((cause & MCAUSE_MASK_INTERRUPT))
    {
        switch (cause_code)
        {
            case 1:
                // printf("Supervisor software interruption!\n");
                return_epc = soft_interrupt_handler(epc);
                // return_epc += 4;
                // printf("out trap_handler, return_epc is %x\n",return_epc);
                break;
            case 3:
                printf("Machine software interruption!\n");
                // return_epc = soft_interrupt_handler(epc);
                break;
            case 5:
                // printf("timer interruption!\n");
                return_epc =  timer_interrupt_handler(epc);
            case 7:
                // printf("timer interruption!\n");
                return_epc =  timer_interrupt_handler(epc);
                break;
            case 8:
                panic("sys_exit!\n");
                break;
            // printf("timer interruption!\n");
            return_epc =  timer_interrupt_handler(epc);
                break;
            case 11:
                // printf("external interruption!\n");
                // extern_interrupt_handler();
                break;
            default:
                printf("unknown async exception!\n cause code is %l\n",cause_code);
                printf("mstatus:%x,mie:%x\n",mstatus_r(),mie_r());
                break;
        }
    }
    else
    {
        printf("\nstval is %x\n",stval_r());
        printf("occour in %x\n",epc);
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
                printf("Breakpiont!\n");
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
                // printf("Environment call from U-mode\n");
                extern do_syscall(reg_context_t *ctx);
                do_syscall(ctx);
                return_epc += 4;
                break;
            case 9:
                // printf("Environment call from S-mode\n");
                panic("Environment call from S-mode\n");
                break;
            case 11:
                // printf("Environment call from M-mode");
                panic("Environment call from M-mode\n");
                break;
            case 12:
                panic("Instruction page fault\n");
                break;
            case 13:
                panic("Load page fault\n");
                page_fault_handler(stval_r());
                break;
            case 15:
                panic("Store/AMO page fault\n");
                page_fault_handler(stval_r());
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
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
void extern_interrupt_handler()
{
    hart_id_t hart_id = mhartid_r();
    uint32_t irqn = __plic_claim(hart_id);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
        break;

        default:
            printf("unexpected extern interrupt!");
        break;
    }
    if(irqn)
    {
        __plic_complete(0,irqn);
    }
}

/***************************************************************
 * @description: 
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t timer_interrupt_handler(reg_t epc )
{
    // reg_t r;
    // hart_id_t hart_id = tp_r();
    // printf("hart %d timer interrupt!\n",hart_id);
    // sip_w(sip_r() | SIP_SSIP);
    // uint64_t now_time = systimer_get_time();
    // systimer_tick++;

    // r = sched(epc,now_time,hart_id);
    
    // swtimer_check();
    return epc;
    // return r;
}

reg_t soft_interrupt_handler(reg_t epc)
{
    sip_w(sip_r() & ~SIP_SSIP);
    
    // reg_t r;
    // hart_id_t hart_id = tp_r();
    // printf("hart %d timer interrupt!\n",hart_id);
    // uint64_t now_time = systimer_get_time();
    // printf("now time is %d\n",now_time);
    // systimer_tick++;

    // r = sched(epc,now_time,hart_id);
    
    // swtimer_check();
    
    return epc;
    
}

void page_fault_handler(addr_t addr)
{
    satp_w(satp_r() & ~(SATP_MODE)); // 切换到bare模式
    // 检查是否为合法地址
    if (addr < RAM_BASE || addr >= (RAM_BASE + RAM_SIZE)) 
    {
        printf("Invalid page fault at %x\n", addr);
        panic("Page fault");
    }
    
    // 分配物理页并建立映射
    // addr_t pa = (addr_t)page_alloc(1);
    // if (!pa) panic("Out of memory");
    
    // 设置映射 (RW权限)
    map_pages(kernel_pgd, addr, addr, PAGE_SIZE, PTE_R | PTE_W);
    
    printf("Handled page fault: VA=%x -> PA=%x\n", addr, addr);
    satp_w(satp_r() | SATP_MODE); 
    // 刷新TLB
    asm volatile("sfence.vma");
}