/**
 * @FilePath: /ZZZ-OS/drivers/timer/timer_chip.c
 * @Description:
 */
#include <os/of.h>
#include <os/printk.h>
#include <os/timer_chip.h>

extern struct of_device_id __timerchip_of_table[];

void timer_chip_init(void) {
    struct device_node *np;

    for_each_matching_node(np, __timerchip_of_table) {
        const struct of_device_id *match;
        timerchip_init_cb_t cb;
        int ret;

        match = of_match_node(__timerchip_of_table, np);
        if (!match) {
            continue;
        }

        cb = (timerchip_init_cb_t)match->data;
        if (!cb) {
            continue;
        }

        ret = cb(np, NULL);
        if (ret) {
            printk("%s: timer chip init failed\n", np->full_path);
        }
    }
}
