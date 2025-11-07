#include <os/of.h>

void of_irq_init(void) {
    if(of_find_node_by_compatible("riscv,clint")) {
        
    }
}