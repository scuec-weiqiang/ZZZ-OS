# IRQ 中断子系统

[TOC]

## 1. 概述

IRQ 子系统采用 Linux 风格的三层模型，将中断控制器硬件差异抽象化，为内核驱动提供统一的 IRQ 申请、使能、优先级管理和中断处理接口。

核心设计目标：
- **硬件无关**：驱动只需操作 virq，无需感知底层 IRQ 控制器实现
- **多控制器支持**：通过 `irq_chip` 抽象，支持 GIC、PLIC 等不同控制器
- **hw/virq 映射**：通过 `irq_domain` 管理硬件中断号到虚拟中断号的映射
- **SMP 支持**：支持 per-CPU 中断和 IPI（核间中断）

## 2. 三层架构

```
┌─────────────────────────────────────────┐
│           IRQ Core (irq.c)              │  ← 驱动调用层
│  irq_request / irq_enable / irq_disable  │
├─────────────────────────────────────────┤
│          IRQ Domain (irq_domain.c)       │  ← hwirq → virq 映射
│  hw_to_virq lookup table                │
├─────────────────────────────────────────┤
│          IRQ Chip (irq_chip.c)           │  ← 硬件抽象层
│  gic_chip.c / plic.c                    │
└─────────────────────────────────────────┘
```

### 2.1 irq_chip 层

`irq_chip` 是对中断控制器硬件的抽象，包含三组操作：

```c
struct irq_ops {
    void (*enable)(struct irq_chip *chip, int hwirq);
    void (*disable)(struct irq_chip *chip, int hwirq);
    void (*set_priority)(struct irq_chip *chip, int hwirq, int priority);
    int  (*get_priority)(struct irq_chip *chip, int hwirq);
    int  (*set_type)(struct irq_chip *chip, int hwirq, int type);
    int  (*set_affinity)(struct irq_chip *chip, int hwirq, int cpu_id);
};

struct cpuif_ops {
    int  (*claim)(struct irq_chip *chip);       // 应答中断
    void (*eoi)(struct irq_chip *chip, int hwirq);  // 中断结束
    void (*cpu_enable)(struct irq_chip *chip, int cpu_id);
    void (*set_priority_mask)(struct irq_chip *chip, int cpu_id, int priority);
};

struct ipi_ops {
    void (*send_ipi)(struct irq_chip *chip, int cpu_id, int ipi_id);
    void (*broadcast_ipi)(struct irq_chip *chip, int ipi_id);
};
```

**已实现的 IRQ 控制器驱动：**

| 驱动 | 文件 | 兼容字符串 | 架构 |
|---|---|---|---|
| ARM GIC | `drivers/irq_chip/gic/gic_chip.c` | `"arm,cortex-a7-gic"` | ARMv7 |
| RISC-V PLIC | `drivers/irq_chip/plic/plic.c` | `"riscv,plic"` | RISC-V64 |

驱动通过 `IRQCHIP_DECLARE()` 宏注册，在启动时由设备树 compatible 字符串自动匹配并初始化。

### 2.2 irq_domain 层

`irq_domain` 负责维护硬件中断号（hwirq）到虚拟中断号（virq）的映射关系。每个域关联一个设备树节点和一个 irq_chip。

核心接口：
- `irq_domain_map()` — 建立 hwirq → virq 映射
- `irq_domain_get_hwirq()` — 通过 virq 反查 hwirq
- `irq_domain_lookup()` — 通过 virq 查找所属 domain

### 2.3 IRQ Core 层

驱动直接使用的接口层，核心数据结构 `struct irq_data`：

```c
struct irq_data {
    int virq;                   // 虚拟中断号
    const char *name;           // 中断名称
    irq_handler_t handler;      // 中断处理函数
    void *dev_id;               // 传递给 handler 的私有数据
    struct irq_chip *chip;      // 关联的 irq_chip
    struct irq_domain *domain;  // 关联的 irq_domain
    int is_percpu;              // 是否为 per-CPU 中断
    irq_handler_t percpu_handler[IRQ_PERCPU_MAX_CPUS];
    void *percpu_dev_id[IRQ_PERCPU_MAX_CPUS];
};
```

### 2.4 核心 API

```c
// 申请普通中断
int irq_request(int virq, irq_handler_t handler, const char *name, void *dev_id);

// 申请 per-CPU 中断
int irq_percpu_request(int virq, int cpu, irq_handler_t handler,
                       const char *name, void *dev_id);

// 使能 / 禁止中断
void irq_enable(int virq);
void irq_disable(int virq);

// 优先级管理
void irq_set_priority(int virq, int priority);
int  irq_get_priority(int virq);

// 中断应答
void irq_acknowledge(int virq);

// IPI 发送
void irq_send_ipi(int cpu_id, int ipi_id);
```

## 3. 中断处理流程

```
硬件中断触发
  → CPU 进入异常模式（arch-specific trap/vector）
  → 架构代码读取当前活跃中断号（通过 cpuif_ops->claim）
  → irq_domain 查找 hwirq → virq 映射
  → do_irq(virq) 被调用
  → 从 irq_data 获取 handler 和 dev_id
  → 执行 handler(virq, dev_id)
  → 返回，执行 cpuif_ops->eoi
  → 异常返回（用户态/内核态）
```

## 4. 延迟工作（Deferred Work）

IRQ 子系统提供 `irq_deferred_work` 机制用于 bottom-half 处理。当中断处理中有一部分工作可以延迟执行（如数据处理、状态更新），可以通过 deferred work 将其推迟到更安全上下文中执行。

## 5. 示例：驱动中使用 IRQ

```c
// 从设备树获取中断号
int virq = of_irq_get(pdev->dev.of_node, 0);

// 申请中断
irq_request(virq, my_handler, "my-device", pdev);

// 使能中断
irq_enable(virq);

// 中断处理函数
reg_t my_handler(int virq, void *dev_id) {
    struct platform_device *pdev = dev_id;
    // 处理设备中断
    return 0;
}
```
