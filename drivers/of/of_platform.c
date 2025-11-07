#include <os/string.h>
#include <os/of.h>
#include <os/driver_model.h>
#include <os/malloc.h>

struct platform_device *platform_device_create_from_node(struct device_node *np) {
    if (!np) return 0;
    struct platform_device *pdev = NULL;
    struct device_prop *compatible = of_get_prop_by_name(np, "compatible");
    uint32_t *regs = of_get_reg(np);
    // struct device_prop *interrupts = of_get_prop_by_name(np, "interrupts");
    // int irq_count = interrupts ? 1:0;

    pdev = malloc(sizeof(struct platform_device));
    pdev->of_node = np;
    pdev->name  = strdup(compatible->name);
    
    if (regs) {
        pdev->resources = malloc(sizeof(struct resource));
        pdev->resources[0].type = RESOURCE_MEM;
        pdev->resources[0].start = regs[0]; // careful with 64-bit
        pdev->resources[0].size = regs[1];  // if your fdt_get_reg returns pair
        pdev->num_resources = 1;
    }


}