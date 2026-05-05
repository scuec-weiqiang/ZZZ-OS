# 驱动框架

[TOC]

## 1. 概述

驱动框架采用 Linux 风格的 bus-device-driver 模型，所有硬件设备通过统一的总线机制注册、匹配和绑定驱动。核心设计理念：

- **设备与驱动分离**：设备描述硬件资源，驱动提供操作逻辑
- **自动匹配**：基于设备树 compatible 字符串自动匹配驱动
- **platform 总线**：所有片上设备（SoC peripherals）挂载在 platform 总线上
- **资源管理**：统一抽象寄存器地址、中断号等资源

## 2. 核心数据结构

### 2.1 设备（struct device）

```c
struct device {
    const char *name;
    struct device *parent;
    struct device_node *of_node;      // 设备树节点
    struct device_driver *driver;
    struct list_head node;            // 挂在 bus 的设备链表
    long driver_data;
};
```

### 2.2 驱动（struct device_driver）

```c
struct device_driver {
    const char *name;
    struct bus_type *bus;
    const struct of_device_id *of_match_table;  // 兼容字符串匹配表
    int (*probe)(struct device *dev);            // 探测函数
    int (*remove)(struct device *dev);           // 移除函数
    struct list_head node;
};
```

### 2.3 总线（struct bus_type）

```c
struct bus_type {
    const char *name;
    int (*match)(struct device *dev, const struct device_driver *drv);
    struct list_head devices;
    struct list_head drivers;
};
```

## 3. Platform 总线

Platform 总线是 SoC 片上设备的主要总线类型。

### 3.1 匹配逻辑

```c
static int platform_match(struct device *dev, const struct device_driver *drv) {
    return of_match_node(drv->of_match_table, dev->of_node) != NULL;
}
```

通过比较设备树节点的 `compatible` 属性与驱动的 `of_match_table` 进行匹配。

### 3.2 Platform 设备

```c
struct platform_device {
    struct device dev;
    int id;
    int num_resources;
    struct resource *resources;   // 内存资源 + IRQ 资源
};
```

### 3.3 Platform 驱动

```c
struct platform_driver {
    int (*probe)(struct platform_device *pdev);
    int (*remove)(struct platform_device *pdev);
    struct device_driver driver;
};
```

### 3.4 资源获取 API

```c
// 获取寄存器资源
struct resource *platform_get_resource(struct platform_device *pdev,
                                       unsigned int type, unsigned int index);

// 获取寄存器地址（自动 ioremap）
virt_addr_t platform_ioremap_resource(struct platform_device *pdev, unsigned int index);

// 获取中断号（从设备树解析）
int platform_get_irq(struct platform_device *dev, unsigned int index);
```

## 4. 驱动注册流程

```
driver_register()
  → bus_add_driver()          // 将驱动挂到总线的驱动链表
  → driver_attach()           // 遍历总线上所有未绑定的设备
    → bus_match()             // 调用 platform_match 匹配
    → driver_probe_device()
      → drv->probe()          // 调用驱动的 probe 函数
```

驱动也可以通过 `module_driver()` 宏自动注册：

```c
static int my_drv_probe(struct platform_device *pdev) { ... }
static int my_drv_remove(struct platform_device *pdev) { ... }

static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-device" },
    { /* sentinel */ }
};

static struct platform_driver my_driver = {
    .probe = my_drv_probe,
    .remove = my_drv_remove,
    .driver = {
        .name = "my-device",
        .of_match_table = my_of_match,
    },
};

module_driver(my_driver, platform_driver_register, platform_driver_unregister);
```

`module_driver()` 将驱动注册函数放入 `.deviceinitcall` 段，在启动时自动调用。

## 5. 设备树平台设备填充

`of_platform_populate()` 遍历设备树中 `compatible = "simple-bus"` 的节点，递归创建 `platform_device`：

```
fdt (设备树二进制)
  → fdt_init()                // 解析为 device_node 树
  → of_platform_populate()    // 为每个设备节点创建 platform_device
    → of_device_alloc()       // 解析 reg / interrupts 属性为 resource
    → platform_device_register()
      → device_register()     // 触发 driver 匹配
```

## 6. 设备节点（devnode）

字符设备和块设备通过 `devnode` 机制注册到 VFS：

```c
struct devnode {
    dev_t dev;                // 设备号
    mode_t mode;              // 设备类型（字符/块）
    const struct file_operations *fops;  // 文件操作
    struct list_head list;
};
```

通过 `devnode_register()` / `devnode_unregister()` 注册/注销。

## 7. 已实现的驱动

| 驱动 | 文件 | 类型 | 兼容字符串 |
|---|---|---|---|
| UART (iMX6ULL) | `drivers/imx6ull_uart.c` | 字符设备 | `"fsl,imx6ull-uart"` |
| UART (QEMU) | `drivers/qemu_riscv_uart.c` | 字符设备 | `"ns16550a"` |
| RTC (iMX6ULL) | `drivers/imx6ull_rtc.c` | 字符设备 | `"fsl,imx6ull-snvs-rtc"` |
| LED (iMX6ULL) | `drivers/imx6ull_led.c` | 字符设备 | — |
| RAM Disk | `drivers/mem_disk/mem_disk.c` | 块设备 | — |
| Virtio Block | `drivers/virt_disk/virtio.c`, `virt_disk.c` | 块设备 | `"virtio,mmio"` |
| GIC | `drivers/irq_chip/gic/gic_chip.c` | IRQ 控制器 | `"arm,cortex-a7-gic"` |
| PLIC | `drivers/irq_chip/plic/plic.c` | IRQ 控制器 | `"riscv,plic"` |
| ARM Timer | `drivers/clocksource/arm/arm_arch_timer.c` | Timer | `"arm,armv7-timer"` |
| GPIO (i.MX) | `drivers/gpio_chip/gpio-mxc.c` | GPIO | `"fsl,imx35-gpio"` |
| GPIO (MMIO) | `drivers/gpio_chip/gpio-generic.c` | GPIO | `"basic-mmio-gpio"` |

## 8. Pin Control

简单的 pinctrl 框架支持引脚复用和配置：

```c
struct pinctrl_device {
    int (*set_mux)(struct pinctrl_device *pdev, int pin, int function);
    int (*set_config)(struct pinctrl_device *pdev, int pin, u32 config);
};
```

支持基于状态的配置（named states），可设置 pin 的 mux 功能、上下拉、驱动强度等参数。
