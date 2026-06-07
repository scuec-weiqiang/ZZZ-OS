# 文档总览

这里收集的是 ZZZ-OS 的设计说明、构建说明和子系统笔记。

需要先提醒一点：项目仍在快速重构中，部分历史文档可能先于代码设计、也可能滞后于当前实现。阅读时请始终以源码为准，尤其是以下目录：

- `arch/`
- `kernel/`
- `mm/`
- `fs/`
- `drivers/`

## 推荐阅读顺序

如果你是第一次接触这个仓库，推荐按下面顺序阅读：

1. 顶层 [README.md](/home/wqqqq/ZZZ-OS/README.md)
2. [build.md](/home/wqqqq/ZZZ-OS/doc/build.md)
3. [deploy.md](/home/wqqqq/ZZZ-OS/doc/deploy.md)
4. [device-tree.md](/home/wqqqq/ZZZ-OS/doc/device-tree.md)
5. [driver.md](/home/wqqqq/ZZZ-OS/doc/driver.md)
6. [irq.md](/home/wqqqq/ZZZ-OS/doc/irq.md)
7. [mm.md](/home/wqqqq/ZZZ-OS/doc/mm.md)
8. [sched.md](/home/wqqqq/ZZZ-OS/doc/sched.md)
9. [fs.md](/home/wqqqq/ZZZ-OS/doc/fs.md)
10. [userspace.md](/home/wqqqq/ZZZ-OS/doc/userspace.md)

## 各文档说明

- [build.md](/home/wqqqq/ZZZ-OS/doc/build.md)
  说明当前 Makefile、工具链、镜像生成和 QEMU 运行方式。

- [deploy.md](/home/wqqqq/ZZZ-OS/doc/deploy.md)
  说明标准 boot/rootfs 分区布局、U-Boot 引导方式和辅助脚本工作流。

- [device-tree.md](/home/wqqqq/ZZZ-OS/doc/device-tree.md)
  介绍设备树解析、OF 辅助接口和平台设备展开方式。

- [driver.md](/home/wqqqq/ZZZ-OS/doc/driver.md)
  介绍 bus / device / driver 框架及驱动接入思路。

- [irq.md](/home/wqqqq/ZZZ-OS/doc/irq.md)
  介绍 IRQ 框架、控制器驱动与中断处理路径。

- [mm.md](/home/wqqqq/ZZZ-OS/doc/mm.md)
  介绍页表、物理页管理、slab、VMA 与用户态内存访问。

- [sched.md](/home/wqqqq/ZZZ-OS/doc/sched.md)
  介绍进程、线程、调度器以及上下文切换相关内容。

- [fs.md](/home/wqqqq/ZZZ-OS/doc/fs.md)
  介绍 VFS、挂载、ext2、ramfs 及文件对象流程。

- [userspace.md](/home/wqqqq/ZZZ-OS/doc/userspace.md)
  介绍用户态运行时、系统调用封装和用户程序组织方式。

## 阅读建议

- 想理解系统从哪里“活起来”，先看 `kernel/kernel.c`
- 想看架构启动与异常入口，先看 `arch/<arch>/boot/` 与 `arch/<arch>/kernel/`
- 想给 RISC-V64 做重构，请优先同时对照 `arch/arm/` 的现有接口

## 文档维护原则

如果你提交的改动会改变下面任一内容，请尽量同步更新文档：

- 构建命令
- 目录结构
- 子系统入口
- 对外接口
- 贡献流程

文档不要求一次写得很大，但至少要保证后来者不会被错误信息误导。
