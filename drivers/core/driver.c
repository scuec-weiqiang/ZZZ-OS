/**
 * @FilePath: /ZZZ-OS/drivers/core/driver.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-10 21:34:46
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/types.h"
#include <os/driver_model.h>
#include <os/list.h>
#include <os/of.h>
#include <os/string.h>

static struct list_head driver_list = LIST_HEAD_INIT(driver_list);
struct list_head platform_device_list = LIST_HEAD_INIT(platform_device_list);

bool device_matches_driver(struct platform_device *pdev, struct platform_driver *drv) {
    const struct of_device_id *m = drv->of_match_table;
    for (; m && m->compatible; m++) {
        struct device_prop *prop = of_get_prop_by_name(pdev->of_node, "compatible");
        if (!prop) continue;
        if (strcmp(prop->value, m->compatible) == 0)
            return true;
        // if compatible is list, check each
    }
    return false;
}

int platform_driver_register(struct platform_driver *drv) {
    if (!drv) {
        return -1;
    }
    list_add(&driver_list, &drv->link);

    struct platform_device *pdev;
    list_for_each_entry(pdev, &platform_device_list, struct platform_device, link) {
        if (device_matches_driver(pdev, drv)) {
            if (!pdev->dev->driver_data) {
                if (drv->probe(pdev) == 0) {
                    pdev->dev->driver_data = drv;
                }
            }
        }
    }
    return 0;
}