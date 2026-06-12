/**
 * @FilePath: /ZZZ-OS/drivers/i2c/busses/test_i2c_client.c
 * @Description: Virtual I2C client driver for testing I2C framework
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2026-06-12
 * @Copyright: G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */
#include <os/i2c.h>
#include <os/init.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/errno.h>

static int test_i2c_client_probe(struct i2c_client *client,
                                 const struct i2c_device_id *id)
{
    printk("test_i2c_client: probed, addr=0x%x, name=%s, adapter=%s\n",
           client->addr, client->name, client->adapter->name);

    /* Test write transfer */
    unsigned char wbuf[] = {0x00, 0x42};
    struct i2c_msg wmsg = {
        .addr  = client->addr,
        .flags = 0,
        .len   = 2,
        .buf   = wbuf,
    };
    int ret = i2c_transfer(client->adapter, &wmsg, 1);
    printk("test_i2c_client: write test %s (ret=%d)\n",
           ret == 1 ? "OK" : "FAIL", ret);

    /* Test read transfer */
    unsigned char rbuf[1] = {0};
    struct i2c_msg rmsg = {
        .addr  = client->addr,
        .flags = I2C_M_RD,
        .len   = 1,
        .buf   = rbuf,
    };
    ret = i2c_transfer(client->adapter, &rmsg, 1);
    printk("test_i2c_client: read test %s (ret=%d)\n",
           ret == 1 ? "OK" : "FAIL", ret);

    return 0;
}

static int test_i2c_client_remove(struct i2c_client *client)
{
    printk("test_i2c_client: removed (addr=0x%02x)\n", client->addr);
    return 0;
}

static const struct of_device_id test_i2c_client_of_match[] = {
    { .compatible = "test,i2c-client" },
    { /* sentinel */ }
};

static const struct i2c_device_id test_i2c_client_id[] = {
    { "test-i2c-dev", 0 },
    { /* sentinel */ }
};

static struct i2c_driver test_i2c_client_driver = {
    .driver = {
        .name            = "test-i2c-client",
        .of_match_table  = test_i2c_client_of_match,
    },
    .probe    = test_i2c_client_probe,
    .remove   = test_i2c_client_remove,
    .id_table = test_i2c_client_id,
};

module_i2c_driver(test_i2c_client_driver);
