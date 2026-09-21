# Ext2 实际磁盘镜像写入与目录创建测试

测试程序及记录由 OpenAI Codex 生成。结论：数据写入和重启后校验通过（使用小块读取），
但大 read 返回值与两类磁盘元数据计数存在错误，不能判定整体通过。

## 环境与安全范围

- RISC-V64、QEMU virt、1 CPU、256 MiB QEMU 内存，设备树声明 128 MiB。
- 当前内核编译生成 uImage，经 U-Boot 启动，实际经过 VFS、Ext2、BIO、VirtIO。
- 原镜像 `build/images/qemu_virt.img` 只读复制，所有写入在 `/tmp/zzz-ext2-write-XLet34/`。
- 64 MiB GPT 镜像，根分区起始扇区 4096，长度 126943 扇区；Ext2 块为 4 KiB。
- 测试前 e2fsck -fn 通过；没有修复测试后的镜像。
- 此处“实际磁盘”指真实 QEMU 文件后端块设备，不是模拟函数，也不是物理硬盘测试。

## 测试程序

源码：`test/fs/ext2_disk_write_test.c`，独立用户程序，不加入内核启动。

```sh
riscv64-unknown-linux-gnu-gcc -static -O2 -march=rv64gc -mabi=lp64d \
    test/fs/ext2_disk_write_test.c -o /tmp/zzz-ext2-write-XLet34/ext2-disk-write-test
```

将程序放入独立测试根分区的 `/bin/ext2-disk-write-test`。
第一次在新副本中运行：

```sh
/bin/ext2-disk-write-test create
```

该模式暴露大 read 错误并提前结束。保留失败镜像后，使用干净副本重新运行：

```sh
/bin/ext2-disk-write-test create small-reads
```

关闭 QEMU，再以相同镜像启动，运行只读校验模式：

```sh
/bin/ext2-disk-write-test verify small-reads
```

`small-reads` 只限制校验时每次 read 最多 512 B；写入仍使用原始大请求。
不能把该模式通过解释成大 read 已经正常。
测试目录固定为 `/codex-ext2-write-test`，重复 create 会失败，应使用新镜像副本，
不要在工作镜像或宿主机上直接运行 create。

## 实际结果

| 测试项 | 实际数据 | 结果 |
| --- | --- | --- |
| 嵌套目录创建 | 测试根目录、nested、deep，共 3 个 | 通过 |
| 空文件首次写入 | 100 B | 通过 |
| 跨页追加 | 追加 10000 B，最终 10100 B，约 9.86 KiB | 写入成功；大 read 校验失败，小 read 校验通过 |
| 局部覆盖 | 从偏移 1000 B 起覆盖 2500 B | 修改区域和前后未修改数据均通过逐字节比较 |
| 稀疏写入 | 偏移 8199 B 写 17 B，最终 8216 B | 前面空洞全零，末尾数据正确 |
| 批量目录项 | 创建 100 个文件，每个写 1 B | 全部按路径重新打开并校验通过 |
| 重启后校验 | 100 个小文件、主文件、稀疏文件 | 小 read 模式全部通过 |
| 离线 e2fsck -fn | 测试前退出码 0，测试后退出码 4 | 失败，发现下述元数据问题 |

创建阶段实际输出：

```text
PASS mkdir nested
PASS empty-file write 100 bytes
PASS cross-page append 10000 bytes
PASS create 100 directory entries
PASS partial overwrite and preserved bytes
PASS sparse gap zero-fill
ext2-disk-write-test: PASS mode=create read-mode=512-byte
```

重启后实际输出：

```text
PASS partial overwrite and preserved bytes
PASS sparse gap zero-fill
ext2-disk-write-test: PASS mode=verify read-mode=512-byte
```

## 发现的问题（本次未修改内核）

### 1. sys_read 返回最后一段长度，而不是总长度

`fs/file.c` 的 sys_read 使用 4096 B 分段，累计 `done`，但结尾返回 `ret`。
读取 10100 B 时，实际复制并推进了完整文件位置，却只向用户返回
`10100 % 4096 = 1908`。测试继续读时立即遇到 EOF。

原始失败输出：

```text
FAIL read /codex-ext2-write-test/nested/deep/data offset=1908
```

建议下一步修正正常退出时的累计返回值，并保留部分成功与错误的语义测试。

### 2. i_blocks 错用文件逻辑长度计算

`fs/ext2/ext2_inode.c` 当前写入：

```c
raw_inode->i_blocks = (inode->i_size + 511) / 512;
```

它应反映已分配磁盘空间的 512 B 扇区数，不能由文件大小直接推算，
还要考虑稀疏文件和间接块等元数据。

| 文件 | 当前 i_blocks | e2fsck 期望 |
| --- | ---: | ---: |
| 10100 B 主文件 | 20 | 24（三个 4 KiB 块） |
| 8216 B 稀疏文件 | 17 | 8（一个 4 KiB 块） |
| 每个 1 B 文件 | 1 | 8（一个 4 KiB 块） |

### 3. 块组目录计数不一致

新建三个目录后，e2fsck 报告：

```text
Directories count wrong for group #0 (10, counted=13).
```

需要检查创建和删除目录时 `bg_used_dirs_count` 的维护与持久化。

## 保留的现场与边界

- `failed-disk.img`、`failed-root.ext2`：首次大 read 失败现场。
- `disk.img`、`after-root.ext2`：完整小 read 校验测试后的磁盘和根分区。
- 均在 `/tmp/zzz-ext2-write-XLet34/`；临时文件可能随环境清理而消失。
- 所有 QEMU 测试进程已退出；没有修改原磁盘镜像，没有自动修复文件系统。
- 未测试物理掉电持久性、并发写入、间接块边界、跨块目录扩展、磁盘满、1 KiB Ext2 块。
- 重启验证的是本次正常完成写入在 QEMU 镜像中的保留，不是断电一致性保证。
