# 部署说明

本文说明 ZZZ-OS 的标准部署方式。

这套模型同时适用于真实硬件和虚拟平台。平台之间的差异，主要体现在：

- 使用哪一份设备树
- 使用哪一份 U-Boot
- U-Boot 读取启动文件时使用什么设备名和加载地址
# 总览
这个部分将教你：
1. 如何编译
2. 编译产物
3. os需要的磁盘分区
4. 根文件系统的要求
5. 如何使用uboot启动os

## 1. 编译

编译命令如下：

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os -j$(nproc)
```

当然，如果你不想写这么长，你可以像编译linux那样直接在顶层Makefile里指定这三个参数，然后直接:

```bash
make os -j$(nproc)
```

编译完成后，产物会整理到：

```text
build/deploy/<arch>/<board>/ （例如build/deploy/arm/imx6ull）
```

目录中通常包含：

- `uImage`
  U-Boot 直接通过 `bootm` 启动的内核镜像
- `<board>.dtb`
  当前平台对应的设备树
- `boot.cmd`
  明文 U-Boot 启动脚本
- `boot.scr`
  由 `boot.cmd` 打包得到的 U-Boot 启动脚本镜像

具体更多编译控制选项请参考 [build.md](./build.md)

## 2. U-Boot 相关
本项目提供了一些平台的uboot，你可以直接用于部署，当然你也可以自己编译u-boot用于部署。
仓库中的 U-Boot 制品和板级启动脚本统一放在：

```text
tools/bootloaders/<platform>/
```

例如：

```text
tools/bootloaders/qemu-riscv64/
tools/bootloaders/imx6ull/
```

这个目录里通常会放：

- `u-boot.bin`
- `boot.cmd`

1. `tools/bootloaders/<platform>/u-boot.bin`
   这是 U-Boot 本体
2. `build/deploy/<arch>/<board>/boot.scr`
   这是 U-Boot 启动 ZZZ-OS 时执行的脚本

## 3. `boot.cmd` 怎样生成
boot.cmd是uboot启动ZZZ-OS 时执行的脚本

`boot.scr` 由 `boot.cmd` 打包生成。

默认情况下，部署脚本会自动生成一份通用的 `boot.cmd`。  
如果存在下面这个文件：

```text
tools/bootloaders/<platform>/boot.cmd
```

则优先使用平台自己的脚本。

也就是说，用户如果需要自定义 U-Boot 的 `bootcmd`，最直接的做法就是修改对应平台目录下的 `boot.cmd`，然后重新执行：

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os
```

重新编译后，新的 `boot.cmd` 和 `boot.scr` 会出现在：

```text
build/deploy/<arch>/<board>/
```


## 4. 硬盘分区要求

当前推荐使用 GPT 双分区布局：

```text
GPT
├── 分区 1：boot
└── 分区 2：rootfs
```

### 4.1 `boot` 分区

作用：

- 给 U-Boot 读取启动文件

建议：

- 文件系统：`ext2`
- 大小：64 MiB 左右通常足够

分区内放：

- `uImage`
- `<board>.dtb`
- `boot.cmd`
- `boot.scr`

### 4.2 `rootfs` 分区

作用：

- 给内核挂载为根文件系统

建议：

- 文件系统：`ext2`
- 大小：按实际需要决定

分区内通常放：

- `/bin/init`（你可以自定义）
- `/bin/simple-c-shell`（你也可以自定义）
- 其他用户态程序
- `/etc/os-release`
- 其他根文件系统内容


## 5. 为什么当前推荐 `ext2`

当前推荐 `ext2`，原因很简单：

- 现有内核支持已经围绕 `ext2` 跑通
- 结构简单，便于调试
- 部署脚本默认也是按 `ext2` 创建和安装

这不是一个永久限制。以后如果项目支持别的文件系统，仍然建议保留“`boot` 分区放启动文件，`rootfs` 分区放系统文件”这一结构。


## 6. 根文件系统设备如何确定

当前内核通过设备树中的：

```text
/chosen/zzz,root-device
```

来决定挂载哪个设备作为根文件系统。

具体设备名称取决于驱动里在dev/下注册的名称。

例如当前仓库中的典型配置是：

- `riscv64/qemu_virt`：`/dev/virt_disk2` 因为qemu是通过virtio挂载img文件作为磁盘的，而virtio驱动里在dev/注册名为virt_disk
  
- `arm/imx6ull`：`/dev/usdhc12` 

这通常对应“第 2 个分区作为根文件系统”。

新增平台时，除了准备 `dtb` 和 `boot.cmd`，还要确认 `/chosen/zzz,root-device` 与实际块设备命名一致，否则内核启动后会找不到根文件系统。


## 7. U-Boot 应该怎样启动内核

当前编译出来的内核文件是 `uImage`，因此推荐使用：

```bash
bootm
```

一个典型的 U-Boot 启动流程如下：

```bash
setenv kernel_addr_r <kernel-load-addr>
setenv fdt_addr_r <dtb-load-addr>
ext2load <boot-media> ${kernel_addr_r} /uImage
ext2load <boot-media> ${fdt_addr_r} /<board>.dtb
bootm ${kernel_addr_r} - ${fdt_addr_r}
```

平台之间真正需要调整的只有三项：

- `boot-media`
- `kernel_addr_r`
- `fdt_addr_r`

如果 U-Boot 可以自动执行 `boot.scr`，则这些命令通常不需要手工输入。


## 8. 标准部署步骤

标准部署可以按下面的顺序进行。

### 第一步：编译

```bash
make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os -j$(nproc)
```

这一步完成后，检查：

```text
build/deploy/<arch>/<board>/
```

确认里面已经有：

- `uImage`
- `<board>.dtb`
- `boot.cmd`
- `boot.scr`

### 第二步：准备启动介质
你的部署设备上需要有个磁盘，可以是sd卡，emmc或其他能作为磁盘的东西。

**当前，目前对磁盘驱动的支持还不完善，恐怕你得自己写个磁盘驱动。:)**

准备一个符合以下要求的磁盘、镜像或分区布局：

- 使用 GPT
- 第 1 分区为 `boot`
- 第 2 分区为 `rootfs`
- 当前推荐两个分区都使用 `ext2`
  
**如果你不知道如何生成，请让你的ai朋友帮助你。:)**

### 第三步：向 `boot` 分区复制启动文件

从：

```text
build/deploy/<arch>/<board>/
```

复制下列文件到 `boot` 分区：

- `uImage`
- `<board>.dtb`
- `boot.cmd`
- `boot.scr`

### 第四步：准备根文件系统

在 `rootfs` 分区中至少放入：

- `/bin/init`

通常还需要：

- `/bin/simple-c-shell`
- 其他用户态程序
- `/etc/os-release`

### 第五步：准备 U-Boot

确认 U-Boot 本体已经正确写入目标介质或目标板能识别的位置，并且它能够：

- 访问 `boot` 分区
- 读取 `uImage`
- 读取 `dtb`
- 自动执行 `boot.scr`，或者接受手工 `bootm` 命令


## 9. 用仓库脚本快速生成一个示例镜像

仓库中的脚本只是一个“标准示例”，方便快速验证部署流程。

因为 `.img` 文件可能比较大，会占用不少磁盘空间，所以这里只建议把它当作参考实现，而不是唯一做法。实际项目中，用户完全可以按本文给出的标准，使用自己的方式创建分区和根文件系统。

### 9.1 创建空白镜像

```bash
tools/deploy/create_disk_image.sh --image build/images/example.img
```

这个脚本会：

1. 创建原始镜像文件
2. 写入 GPT
3. 创建 `boot` 和 `rootfs` 两个分区
4. 将两个分区格式化为 `ext2`

### 9.2 安装当前构建结果

```bash
tools/deploy/install_disk_image.sh \
  --image build/images/example.img \
  --arch <arch> \
  --board <board> \
  --cross-compile <toolchain-prefix>
```

这个脚本会：

1. 挂载镜像中的两个分区
2. 将 `build/deploy/<arch>/<board>/` 中的启动文件复制到 `boot`
3. 执行 `make u`
4. 将用户态程序安装到 `rootfs` 的 `/bin`
5. 写入一个简单的 `/etc/os-release`

如果你只是想验证当前构建是否能够形成一个完整镜像，这两个脚本已经足够。


## 10. 最低需要满足什么

只要满足下面这些条件，部署流程就是成立的：

1. 你的机器上烧写了U-Boot
2. 有一个 U-Boot 能读取的 `boot` 分区
3. `boot` 分区里有 `uImage`、`dtb`、`boot.scr`
4. 有一个可挂载的 `rootfs` 分区
5. `rootfs` 中至少有 `/bin/init`
6. 设备树中的 `/chosen/zzz,root-device` 指向正确根设备
7. U-Boot 的启动命令能正确加载 `uImage` 和 `dtb`

满足这几条以后，你可以自由选择：

- 手工分区
- 自己编写打包脚本
- 使用现有系统分区
- 使用宿主机目录树生成 rootfs


## 12. 新平台接入时建议检查的项目

新增平台时，建议至少检查下面这些内容：

1. `make ARCH=<arch> CROSS_COMPILE=<toolchain-prefix> BOARD=<board> os` 是否成功
2. `build/deploy/<arch>/<board>/` 是否生成了完整启动文件
3. `tools/bootloaders/<platform>/` 中是否已经准备好 U-Boot 制品
4. `tools/bootloaders/<platform>/boot.cmd` 是否符合该平台的 U-Boot 读盘方式
5. `/chosen/zzz,root-device` 是否与实际根设备一致
6. `boot` 分区中是否放入了正确版本的 `uImage` 和 `dtb`
7. `rootfs` 分区中是否存在 `/bin/init`

这些项目全部通过以后，部署链路通常就已经完整了。
