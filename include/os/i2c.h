#ifndef __OS_I2C_H
#define __OS_I2C_H
#include <os/types.h>
#include <os/device.h>
struct i2c_msg;
struct i2c_algorithm;
struct i2c_adapter;

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
    int (*probe)(struct i2c_client *client);     // 匹配成功后调用
    int (*remove)(struct i2c_client *client);
    const struct i2c_device_id *id_table;        // 传统 ID 匹配表
};

#endif