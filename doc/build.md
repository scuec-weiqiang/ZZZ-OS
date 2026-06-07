# 构建系统

这份文档描述的是仓库当前代码所对应的构建方式，重点面向两类读者：

- 第一次尝试把系统跑起来的贡献者
- 需要修改 Makefile、工具链、镜像流程的开发者

## 1. 总览

ZZZ-OS 使用顶层 [Makefile](/home/wqqqq/ZZZ-OS/Makefile:1) 驱动编译流程，并配合仓库内工具完成：

- 递归扫描 `objs.build`
- 交叉编译内核
- 编译设备树
- 生成 `uImage`
- 生成 U-Boot 启动脚本 `boot.cmd` 与 `boot.scr`
- 构建用户态运行时与示例程序

当前默认参数是：

```makefile
ARCH ?= riscv64
BOARD ?= qemu_virt
```

如果你不显式指定参数，默认走的是 RISC-V64 + QEMU `virt` 路径。
`CROSS_COMPILE` 没有默认值，需要由用户自己传入。

不过更推荐像 Linux 内核那样显式传入变量：

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os -j$(nproc)
```

或者你可以在顶层Makefile里更改默认参数，这样可以直接：
```bash
make os -j$(nproc)
```

## 2. 依赖

建议准备以下工具：

- `make`
- `gcc`
- 类似`riscv-none-elf-gcc`、 `arm-none-eabi-gcc`等对应平台的编译器

仓库内已经带有：

- `tools/dtc`：用于编译dts文件
- `tools/kbuild`：用于收集所有编译需要的目标文件
- `tools/mkimage`：将编译生成的efl/bin 变成uimage镜像

## 3. 构建目标

### 3.1 构建用户态程序
例如：
```bash
make u ARCH=riscv64 CORSS_COMPILE=riscv-none-elf-gcc
```

或：

```bash
make u ARCH=arm CORSS_COMPILE=arm-none-eabi-gcc
```

这会遍历 `user_proc/*` 下所有带 `Makefile` 的目录，并先构建 `user_runtime/` 依赖的目标文件，再分别编译用户程序。

相关目录：

- [user_runtime/](/home/wqqqq/ZZZ-OS/user_runtime)
- [user_proc/](/home/wqqqq/ZZZ-OS/user_proc)

### 3.2 构建主机侧启动产物

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os
```

`make os` 会完成以下工作：

1. 构建 `tools/kbuild`
2. 根据 `objs.build` 生成 `objs.<arch>.mk`
3. 编译所有内核源文件为 `build/.../*.o`
4. 先链接临时 ELF，生成 kallsyms 数据，再链接正式 `build/kernel.elf`
5. 导出 `build/kernel.bin`
6. 通过 `tools/mkimage` 生成 `build/uImage`
7. 编译 `arch/<arch>/boot/dts/<board>.dts` 为 `build/.../*.dtb`
8. 生成 `build/deploy/<arch>/<board>/boot.cmd` 与 `boot.scr`

其中 `boot.scr` 由 `tools/mkimage` 根据 `boot.cmd` 生成；如果存在
`tools/bootloaders/<platform>/boot.cmd`，则会优先使用这个板级脚本。

`make os` 会把启动产物整理到：

```text
build/deploy/<arch>/<board>/
```

### 3.3 辅助目标

- `make dump`：导出 `build/disassembly.asm`
- 介质准备、QEMU 启动与 boot/rootfs 安装见 [deploy.md](/home/wqqqq/ZZZ-OS/doc/deploy.md)

## 4. 配置来源

默认配置文件来自：

- [arch/riscv64/config/defconfig](/home/wqqqq/ZZZ-OS/arch/riscv64/config/defconfig)
- [arch/arm/config/defconfig](/home/wqqqq/ZZZ-OS/arch/arm/config/defconfig)

首次构建时，如果对应架构的 `.config.<arch>` 不存在，Makefile 会把对应 `defconfig` 复制为该架构自己的配置文件。

当前可见的典型配置包括：

- RISC-V64：PLIC、virt disk、QEMU UART、ext2、ramfs
- ARM：GIC、i.MX6ULL UART/LED/USDHC、GPIO、pinctrl、ext2、ramfs

## 5. `objs.build` 与 kbuild

ZZZ-OS 不是直接在顶层 Makefile 里手写所有对象文件，而是通过 `objs.build` 描述目录下需要编译的源文件，再由 `tools/kbuild/kbuild` 展开成 `objs.mk`。

这带来两个直接好处：

- 新增文件时只需要在对应目录补 `objs.build`
- 架构相关和通用对象可以分层组织

如果你新增了源码但编译系统没有自动带上，第一件事应检查对应目录的 `objs.build`。

## 6. 用户态构建说明

用户态由两部分组成：

- `user_runtime/`
  负责 `crt0.S`、系统调用汇编封装和 `syscalls.c`
- `user_proc/`
  各个独立用户程序

当前用户态工具链配置位于：

- [user_proc/.toolchain/arm.mk](/home/wqqqq/ZZZ-OS/user_proc/.toolchain/arm.mk)
- [user_proc/.toolchain/riscv64.mk](/home/wqqqq/ZZZ-OS/user_proc/.toolchain/riscv64.mk)

如果你要新增用户程序，最简单的做法是参考已有目录：

- `user_proc/hello/`
- `user_proc/init/`
- `user_proc/simple-c-shell/`

## 7. 常见问题

### 7.1 新文件加了却没有参与编译

通常是以下原因之一：

- 忘了更新 `objs.build`
- 目标配置没有在对应 `defconfig` 中启用
- 文件放在了未被构建系统扫描到的目录

