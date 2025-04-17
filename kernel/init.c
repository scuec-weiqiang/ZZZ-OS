#include "printf.h"
#include "page.h"
#include "uart.h"
#include "riscv.h"
void main(int hartid)
{   
    if(hartid == 0)
    {


        uart_init();
        page_init();
        printf("core 0 runing\n");

        int v = 0;
        csrr a0, mstatus 
        ori a0, a0, 1<<7 # MPIE置1
        csrw mstatus, a0
    
        # 设置trap入口
        la a0, trap_entry
        csrw mtvec, a0
    }
    else if(hartid == 1)
    {

    }
    else
    {

    }
    
    while(1)
    {

    }
}