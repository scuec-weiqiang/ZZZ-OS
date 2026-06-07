# ZZZ-OS

ZZZ-OS 是一个仍在开发中的类 Unix 操作系统

项目目前已经具备较完整的内核雏形，包括启动、设备树、驱动模型、中断管理、内存管理、进程调度、VFS、ext2、ELF 用户态加载以及基于 newlib 的用户程序运行环境。

---

## 当前状态

目前仍属于开发中的OS 项目，架构与接口可能会变更。

## 功能概览

当前代码已经覆盖一个小型类 Unix 系统所需的几条主线：

- **多架构基础**：已具备 `arm` 与 `riscv64` 两套架构实现，包含各自的启动代码、异常入口、页表切换和上下文切换路径。
- **设备发现与驱动框架**：基于 Device Tree 完成 FDT 解析、OF 平台总线以及 `bus / device / driver` 模型，能够按设备树组织平台设备并完成驱动绑定。
- **中断与时间子系统**：已接入 GIC / PLIC，中断分发、clocksource、clockevent、timerqueue 和 timekeeping 可以协同工作。
- **基础设备支持**：已经实现串口、GPIO、pinctrl 以及部分平台相关外设驱动，可支撑基本交互和板级实验。
- **内存管理**：包含 memblock、页表、buddy、slab、VMA、page fault，以及 `mmap / munmap / mprotect / brk` 等用户地址空间接口。
- **进程与调度**：支持内核线程、用户进程、`fork / execve / exit / waitpid` 和 RR 调度，能够跑起基本的多进程用户态环境。
- **文件系统**：已有 VFS、ext2 和基础文件管理能力，可以挂载根文件系统并加载 ELF 用户程序。
- **用户态运行时**：提供 `crt0`、系统调用封装和 newlib 兼容层，用户程序可以按标准 C 运行时方式构建。
- **用户空间示例**：仓库内已经包含 `init`、`simple-c-shell`、`ls`、`cat`、`echo`、`mkdir`、`touch`、`env`、`proc_pipe` 等程序，可用于功能验证和回归测试。

---

## 支持平台

| 架构       | 平台           | 状态     |
| -------- | ------------ | ------ |
| ARMv7-A  | NXP i.MX6ULL | 推荐参考实现 |
| RISC-V64 | QEMU virt    | 持续完善中  |

---

## 仓库结构
 
```text
arch/           架构相关实现
drivers/        驱动与驱动框架
fs/             文件系统
include/        公共头文件
kernel/         内核核心
lib/            基础库
mm/             内存管理
tools/          构建与部署工具
user_runtime/   用户态运行时
user_proc/      用户程序
doc/            文档
```

**如果你想了解详细架构，请参考doc/下的文档**

---
## 详细部署流程
见 [doc/deploy.md](doc/deploy.md)
## 快速开始
你可以使用qemu快速体验，这里以**riscv64平台**为例：
### 准备依赖

建议至少准备以下工具：

```text
make
gcc
qemu-system-riscv64
```

以及对应架构的交叉工具链：``riscv-none-elf-gcc`` 或其他可以支持编译riscv64平台的编译器。

仓库自带 `dtc`，无需系统预装。

---

### 编译系统

构建时需要指定：

| 变量            | 说明      |
| ------------- | ------- |
| ARCH          | 目标架构    |
| BOARD         | 目标平台    |
| CROSS_COMPILE | 交叉工具链前缀 |

#### RISC-V64 (QEMU virt)

```bash
make \
    ARCH=riscv64 \
    BOARD=qemu_virt \
    CROSS_COMPILE=riscv-none-elf- \
    os -j$(nproc)
```

构建产物将输出到：

```text
build/deploy/<arch>/<board>/
```

例如：

```text
build/deploy/riscv64/qemu_virt/
```

其中包含：

```text
uImage
<board>.dtb
```

更多构建选项见：

```text
doc/build.md
```

---

### 部署系统

ZZZ-OS 当前推荐采用 GPT 双分区布局：

```text
GPT
├── boot
└── rootfs
```

* boot：存放 uImage、dtb、boot.scr
* rootfs：系统根文件系统（ext2）

对于 QEMU 平台，可直接使用部署脚本：

```bash
tools/deploy/create_disk_image.sh \
    --image build/images/qemu_virt.img

tools/deploy/install_disk_image.sh \
    --image build/images/qemu_virt.img \
    --arch riscv64 \
    --cross-compile riscv-none-elf-\
    --board qemu_virt
```
---

### 启动 QEMU

```bash
tools/deploy/run_qemu_riscv64.sh
```

---

### 清理

```bash
make clean
make distclean
```

---

详细启动过程见对应文档。

---

## 贡献指南

贡献入口：

```text
CONTRIBUTING.md
```

请优先遵循：

* 优先实现通用框架要求的架构接口
* 避免为兼容历史代码破坏通用层设计

---

## License

仓库当前尚未完成统一许可证整理。

如需长期维护、分发或基于本项目开发衍生版本，建议先与维护者确认授权策略。
