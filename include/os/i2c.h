#ifndef __OS_I2C_H
#define __OS_I2C_H

#include <os/types.h>
#include <os/device.h>
#include <os/container_of.h>

struct i2c_msg;
struct i2c_algorithm;
struct i2c_adapter;

struct i2c_device_id {
	char name[32];
	unsigned long driver_data;	/* Data private to the driver */
};

#define I2C_M_RD 0x0001

struct i2c_msg {
    u16 addr;
    u16 flags;

    u16 len;
    u8 *buf;
};

struct i2c_algorithm {
    int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
    int (*functionality)(struct i2c_adapter *adap);
};

struct i2c_adapter {
    char name[32];
    struct device dev;
    int nr;
    const struct i2c_algorithm *algo;
};

struct i2c_client {
      unsigned short addr;          // 从设备地址（7-bit 或 10-bit）
      unsigned short flags;         // I2C_CLIENT_TEN 等标志位
      char name[32];                // 设备名称
      struct i2c_adapter *adapter;  // 所属的 I2C 总线
      struct device dev;            // 内嵌 device，挂在 i2c_bus_type 上
};

struct i2c_driver {
    struct device_driver driver;                 // 内嵌 driver core
    int (*probe)(struct i2c_client *client,const struct i2c_device_id *id);     // 匹配成功后调用
    int (*remove)(struct i2c_client *client);
    const struct i2c_device_id *id_table;        // 传统 ID 匹配表
};

#define to_i2c_adapter(d) container_of(d, struct i2c_adapter, dev)
#define to_i2c_client(d) container_of(d, struct i2c_client, dev)
#define to_i2c_driver(d) container_of(d, struct i2c_driver, driver)

extern int i2c_add_adapter(struct i2c_adapter *adap);
extern int i2c_del_adapter(struct i2c_adapter *adap);
extern struct i2c_adapter* i2c_get_adapter(int nr);
extern int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
extern int i2c_init(void);

extern int i2c_driver_register(struct i2c_driver *driver);
extern int i2c_driver_unregister(struct i2c_driver *driver);

#define module_i2c_driver(__i2c_driver) \
    module_driver(__i2c_driver, i2c_driver_register, i2c_driver_unregister)
#endif