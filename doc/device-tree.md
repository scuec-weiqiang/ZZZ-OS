# 设备树（Device Tree）

[TOC]

## 1. 概述

设备树（Device Tree）是一种描述硬件资源的数据结构，ZZZ-OS 通过解析 Flattened Device Tree (FDT) 二进制格式，在启动时发现和配置硬件，避免将硬件信息硬编码在内核中。

核心功能：
- **FDT 解析**：将二进制 DTB 解析为 `device_node` 树
- **属性访问**：提供 `of_get_property()`、`of_property_read_u32()` 等访问接口
- **地址解析**：`of_address.c` 处理 `reg`、`ranges` 等地址属性
- **中断解析**：`of_irq.c` 处理 `interrupts`、`interrupt-parent` 等中断属性
- **平台填充**：`of_platform.c` 从设备树自动创建 platform_device

## 2. FDT 解析

### 2.1 二进制格式

FDT 由三个主要部分组成：

```
┌─────────────────────────┐
│    FDT Header           │  magic, totalsize, off_dt_struct, off_dt_strings ...
├─────────────────────────┤
│    Memory Reserve Map   │  保留内存区域列表
├─────────────────────────┤
│    Struct Block         │  节点树（tokens + 节点名 + 属性）
├─────────────────────────┤
│    Strings Block        │  属性名字符串表
└─────────────────────────┘
```

Struct Block 中的 Token 类型：
- `FDT_BEGIN_NODE` (0x1) — 节点开始
- `FDT_END_NODE` (0x2) — 节点结束
- `FDT_PROP` (0x3) — 属性
- `FDT_NOP` (0x4) — 空操作
- `FDT_END` (0x9) — 结束

### 2.2 解析流程

```c
fdt_init(dtb)
  → 校验 FDT_MAGIC
  → 定位 struct_block 和 strings
  → parse_struct_block()
    → FDT_BEGIN_NODE → fdt_new_node()，构建树形结构
    → FDT_PROP → fdt_new_prop()，挂到节点属性链表
    → FDT_END_NODE → 回到父节点
    → FDT_END → 返回根节点
  → phandle_table 缓存（加速后续查找）
```

解析结果：
- `fdt_root_node` — 设备树根节点
- `phandle_table[]` — phandle 到 device_node 的快速查找表

### 2.3 数据结构

```c
struct device_node {
    char *name;                  // 节点名
    char *full_path;             // 完整路径（如 /soc/uart@02020000）
    struct device_node *parent;  // 父节点
    struct device_node *children;// 子节点链表
    struct device_node *sibling; // 兄弟节点
    struct device_prop *properties; // 属性链表
    int depth;                   // 深度
};

struct device_prop {
    char *name;                  // 属性名
    u32 length;                  // 属性值长度
    void *value;                 // 属性值
    struct device_prop *next;    // 下一个属性
};
```

## 3. OF 辅助函数

### 3.1 属性访问

```c
// 获取属性
struct device_prop *of_get_property(const struct device_node *np,
                                    const char *name, u32 *len);

// 读取 u32 值
int of_property_read_u32(const struct device_node *np,
                         const char *propname, u32 *out_value);

// 读取 u32 数组
int of_property_read_u32_array(const struct device_node *np,
                               const char *propname, u32 *out_values, size_t count);

// 读取字符串
const char *of_get_string(const struct device_node *np, const char *propname);
```

### 3.2 地址映射（of_address.c）

```c
// 解析 reg 属性为 resource
int of_address_to_resource(const struct device_node *dev, int index,
                           struct resource *r);

// 映射设备寄存器（自动 ioremap）
virt_addr_t of_iomap(const struct device_node *dev, int index);
```

### 3.3 中断映射（of_irq.c）

```c
// 从设备树获取虚拟中断号
int of_irq_get(const struct device_node *dev, int index);

// 解析中断资源
int of_irq_to_resource(const struct device_node *dev, int index,
                       struct resource *r);
```

### 3.4 兼容匹配

```c
// 检查节点的 compatible 属性
int of_device_is_compatible(const struct device_node *device,
                            const char *compat);

// 匹配设备的 compatible 与驱动表
const struct of_device_id *of_match_node(const struct of_device_id *matches,
                                         const struct device_node *node);

// OF_DECLARE 宏（驱动自动注册）
#define IRQCHIP_DECLARE(name, compat, fn) \
    _OF_DECLARE(irqchip, name, compat, fn, __irqchip_of_table)

#define TIMERCHIP_DECLARE(name, compat, fn) \
    _OF_DECLARE(timerchip, name, compat, fn, __timerchip_of_table)
```

## 4. 平台设备填充

`of_platform_populate()` 递归遍历设备树，将 `"simple-bus"` 兼容的子节点转换为 `platform_device`：

```c
of_platform_populate(root, match_table, parent)
  → 遍历 root 的子节点
    → 对每个有 compatible 的节点：
      → of_device_alloc(np) — 解析 reg/interrupts 为 resource
      → platform_device_register(pdev) — 注册设备，触发驱动匹配
    → 对 compatible = "simple-bus" 的节点：
      → 递归 of_platform_populate()
```

## 5. CPU 和内存信息

- **of_cpu.c** — 从设备树 `"/cpus"` 节点获取 CPU 数量和 ID
- **of_memory.c** — 从 `"/memory"` 节点解析物理内存区域

## 6. 设备树源文件

| 架构 | 文件 | 描述 |
|---|---|---|
| ARM | `arch/arm/boot/dts/imx6ull.dts` | iMX6ULL 开发板设备树 |
| RISC-V | `arch/riscv64/boot/dts/qemu_virt.dts` | QEMU virt 设备树 |
| RISC-V | `arch/riscv64/boot/dts/virt_default.dts` | QEMU virt 默认设备树 |

编译过程：
```
.dts → (cpp 预处理) → .dts → dtc → .dtb → 打包到 uImage
```
