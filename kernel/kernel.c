#include "printf.h"
#include "page.h"
#include "uart.h"

void main(int hartid)
{   
    if(hartid == 0)
    {
        uart_init();
        page_init();
        printf("core 0 runing\n");
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