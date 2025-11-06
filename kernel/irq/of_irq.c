#include <drivers/of/fdt.h>

void of_irq_init(void) {
    if(fdt_find_node_by_compatible("riscv,clint")) {
        
    }
}