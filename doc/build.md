# 构建系统

[TOC]

## 1. 概述

ZZZ-OS 使用基于 Makefile 的构建系统，配合自定义的 `kbuild` 工具实现分层源码构建。支持 ARM 和 RISC-V 两种目标架构的交叉编译。

核心特性：
- **分层构建**：每个子目录通过 `objs.build` 文件声明需要编译的源文件
- **kbuild 工具**：递归扫描 `objs.build` 生成扁平的对象列表
- **设备树编译**：内置 DTC（Device Tree Compiler），从 `.dts` 编译 `.dtb`
- **磁盘镜像**：自动生成 disk.img，用于 QEMU 和真实硬件
- **U-Boot 包装**：将 kernel 包装为 U-Boot 可加载的 uImage 格式

## 2. 主要目标

### 2.1 构建完整系统

```bash
make all ARCH=arm BOARD=imx6ull
```

流程：
1. 编译 kbuild 工具
2. 扫描 `objs.build` → 生成 `build/objs.arm.mk`
3. 编译所有 `.c` / `.S` → `.o`
4. 链接 → `build/kernel.elf`
5. 提取二进制 → `build/kernel.bin`
6. 包装 uImage → `build/uImage`
7. 编译 DTS → `.dtb`
8. 创建 disk.img → 挂载 → 拷贝 uImage + dtb → 卸载

### 2.2 构建并部署到 TFTP

```bash
make os ARCH=arm BOARD=imx6ull
```

编译后将 uImage 和 dtb 拷贝到 `../linux/tftpboot/`，用于网络启动。同时构建用户程序。

### 2.3 QEMU 运行

```bash
make run ARCH=riscv64 BOARD=virt
```

启动 QEMU 模拟 RISC-V virt 机器，加载 kernel 和 disk.img。

QEMU 参数：
```
qemu-system-riscv64 -nographic -smp 1 -machine virt \
  -bios arch/riscv64/boot/u-boot.bin -cpu rv64,sstc=on \
  -drive file=disk.img,if=none,format=raw,id=disk0 \
  -device virtio-blk-device,drive=disk0,bus=virtio-mmio-bus.0 \
  -global virtio-mmio.force-legacy=false
```

## 3. kbuild 工具

`tools/kbuild/kbuild` 是一个 C 语言编写的构建扫描器，递归解析 `objs.build` 文件。

### 3.1 objs.build 语法

每个源文件目录包含一个 `objs.build` 文件，声明需要编译的源文件：

```makefile
# 通用源（所有架构都编译）
OBJ_Y += kernel.c
OBJ_Y += mm/buddy.c

# 架构特定源（仅指定架构编译）
OBJ_arm += arch/arm_entry.S
OBJ_riscv64 += arch/riscv_entry.S
```

### 3.2 工作流程

```
kbuild $(SRC_ROOT)
  → 递归查找所有 objs.build
  → 解析 OBJ_Y / OBJ_$(ARCH) 条目
  → 输出 OBJ_Y += path/to/file.c 扁平列表
  → Makefile include 此列表
  → 编译所有 .o
```

## 4. 编译器配置

### 4.1 交叉编译工具链

| 架构 | 工具链前缀 | 说明 |
|---|---|---|
| ARM | `arm-none-eabi-` | bare-metal ARM 工具链 |
| RISC-V | `riscv64-unknown-elf-` | bare-metal RISC-V 工具链 |

### 4.2 编译标志

```makefile
CFLAGS = -g -Wall -fno-builtin -std=c11 -ffreestanding -fno-pic -fno-pie -no-pie
```

- `-ffreestanding`：不依赖标准库
- `-fno-builtin`：不使用编译器内置函数
- `-std=c11`：C11 标准

### 4.3 链接脚本

各架构独立的链接脚本位于 `arch/<arch>/config/link.ld`，定义：
- 内核加载地址
- 段布局（`.text`、`.data`、`.bss`）
- initcall 段（`.initcall`、`.coreinitcall`、`.deviceinitcall` 等）
- 内核栈大小（32KB）
- 早期页表空间（16KB）

## 5. 设备树编译

```bash
make dtbs ARCH=riscv64 BOARD=virt
```

编译流程：
1. C 预处理器处理 `.dts`（支持 `#include` 和宏定义）
2. DTC 将 `.dts` 编译为 `.dtb` 二进制
3. 拷贝到 disk.img 中

DTC 位于 `tools/dtc/`，是完整的 Device Tree Compiler 实现（包含 lexer、parser、libfdt）。

## 6. 磁盘镜像

```bash
make disk
```

通过 `tools/mkdisk.sh` 创建 disk.img 并格式化为 ext2 文件系统。

挂载/卸载操作：
```bash
make mount    # 挂载 disk.img
make umount   # 卸载
make show     # 查看 disk.img 内容
```

## 7. 用户程序构建

```bash
make u    # 构建 user_proc/ 下所有用户程序
make uc   # 清理用户程序构建产物
```

遍历 `user_proc/proc*` 目录，对包含 Makefile 的子目录执行构建。

## 8. 调试

### 8.1 反汇编

```bash
make dump
```

生成 `build/disassembly.asm`，包含 kernel.elf 的完整反汇编。

### 8.2 GDB 调试

```bash
# 在 QEMU 中加 -s -S 参数启动 GDB server
# 然后使用 gdb-multiarch 连接
gdb-multiarch -tui -q -x gdbinit build/kernel.elf
```

## 9. 清理

```bash
make clean        # 清理编译产物
make distclean    # 清理所有输出（包括 disk.img、loop device）
```
