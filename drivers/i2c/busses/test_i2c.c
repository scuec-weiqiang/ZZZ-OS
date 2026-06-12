/**
 * @FilePath: /ZZZ-OS/drivers/i2c/busses/test_i2c.c
 * @Description: Virtual I2C adapter driver for testing
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2026-06-11
 * @Copyright: G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */
#include <os/i2c.h>
#include <os/bus.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/errno.h>

static int test_i2c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num) {
    for (int i = 0; i < num; i++) {
        printk("test_i2c: %s addr=0x%x len=%d\n",
               (msgs[i].flags & I2C_M_RD) ? "read" : "write",
               msgs[i].addr, msgs[i].len);
    }
    return num;
}

static int test_i2c_functionality(struct i2c_adapter *adap) {
    return 0;
}

static const struct i2c_algorithm test_i2c_algo = {
    .master_xfer   = test_i2c_master_xfer,
    .functionality = test_i2c_functionality,
};

static int i2c_test_probe(struct platform_device *pdev) {
    struct i2c_adapter *adap;

    adap = kmalloc(sizeof(struct i2c_adapter));
    if (!adap)
        return -ENOMEM;

    memset(adap, 0, sizeof(struct i2c_adapter));

    strcpy(adap->name, "test-i2c");
    adap->algo       = &test_i2c_algo;
    adap->dev.of_node = pdev->dev.of_node;
    adap->dev.parent  = &pdev->dev;

    platform_set_drvdata(pdev, adap);

    printk("test_i2c: probing virtual i2c adapter\n");

    i2c_add_adapter(adap);

    // adap = i2c_get_adapter(adap->nr);
    // unsigned char data = 0x42;
    // struct i2c_msg msg = {
    //     .addr = 0x50,
    //     .flags = 0,
    //     .len = 1,
    //     .buf = &data,
    // };

    // i2c_transfer(adap, &msg, 1);

    return 0;
}

static int i2c_test_remove(struct platform_device *pdev)
{
    struct i2c_adapter *adap = platform_get_drvdata(pdev);

    if (adap) {
        device_unregister(&adap->dev);
        kfree(adap);
    }
    return 0;
}

static const struct of_device_id i2c_test_of_match[] = {
    { .compatible = "test,i2c0" },
    { /* sentinel */ }
};

static struct platform_driver i2c_test_driver = {
    .name   = "test-i2c",
    .probe  = i2c_test_probe,
    .remove = i2c_test_remove,
    .driver = {
        .of_match_table = i2c_test_of_match,
    },
};

static int __init i2c_adap_test_init(void)
{
    return platform_driver_register(&i2c_test_driver);
}
subsys_initcall(i2c_adap_test_init);
