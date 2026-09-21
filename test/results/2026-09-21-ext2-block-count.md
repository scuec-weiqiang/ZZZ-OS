# Ext2 i_blocks 记账修复及复测

OpenAI Codex 生成。单位统一为 512 B，字段位于 VFS inode。

## 修改

- `ext2_alloc_inode_block()` 包装位图分配，成功后增加 `s_blocksize / 512`。
- `ext2_release_inode_block()` 包装位图释放，成功后减少相同数量，并检查计数下溢。
- 直接块和所有级别间接块的分配、释放都经过这两个函数。
- `read_or_alloc_block()` 和 `ext2_release_indirect_branch()` 接收 inode，沿调用链传递记账归属。
- 重复使用已有块映射不增加计数。
- 保留用户已完成的 inode 加载、初始化和写回接线，写回前检查磁盘 u32 字段上限。
- 两个 stat 实现从 inode->i_blocks 获取占用，不再根据 i_size 推算。
- 保留用户对 sys_read 累计返回值的修复。

## 函数级测试

```sh
riscv64-linux-gnu-gcc -std=gnu11 -D__KERNEL__ -static -O2 \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -fno-builtin -DSYS_BITS=64 -Iinclude \
    test/fs/ext2_blocks_test.c -o /tmp/zzz-block-count-g0govu/unit
qemu-riscv64 /tmp/zzz-block-count-g0govu/unit
```

实际输出：`ext2-blocks-test: PASS direct/single/double/triple/free/failure`。

分别在直接、一级、二级、三级间接区域分配一个数据块，累计计数依次为
8、24、48、80；重复映射不变。分配失败及第一次释放失败时计数不变，
完整释放后计数为零，重复释放空 inode 不产生下溢。
磁盘块和位图操作由模拟接口提供，映射及递归释放运行实际 Ext2 代码。

## 实际镜像测试

单核 QEMU RISC-V64，4 KiB Ext2，64 MiB 独立 GPT 镜像。
原始 `build/images/qemu_virt.img` 未修改，测试副本位于 `/tmp/zzz-block-count-g0govu/`。
用 debugfs 向副本安装新 uImage 和两个静态用户程序。
测试前根分区 `e2fsck -fn` 退出码 0。

用户程序编译：

```sh
riscv64-unknown-linux-gnu-gcc -static -O2 -march=rv64gc -mabi=lp64d \
    test/fs/ext2_disk_write_test.c -o /tmp/zzz-block-count-g0govu/write-test
riscv64-unknown-linux-gnu-gcc -static -O2 -march=rv64gc -mabi=lp64d \
    test/fs/ext2_disk_blocks_test.c -o /tmp/zzz-block-count-g0govu/blocks-test
```

安装到副本的 /bin 后，在 ZZZ-OS shell 中执行：

```sh
/bin/write-test create
/bin/blocks-test create
```

结束 QEMU，再用同一镜像重新启动，执行：

```sh
/bin/write-test verify
/bin/blocks-test verify
```

创建和重启校验均输出 PASS。本次使用默认大 read，没有使用 small-reads 绕过。

| 文件 | 逻辑长度 | 实际分配 | stat 实测 i_blocks |
| --- | --- | --- | ---: |
| 普通主文件 | 10100 B，约 9.86 KiB | 3 个数据块 | 24 |
| 稀疏文件 | 8216 B，约 8.02 KiB | 1 个数据块 | 8 |
| 单字节文件 | 1 B | 1 个数据块 | 8 |
| 一级间接文件 | 65537 B，64 KiB + 1 B | 17 个数据块 + 1 个间接块 | 144 |
| 二级间接稀疏文件 | 4243464 B，约 4.05 MiB | 1 个数据块 + 2 个间接块 | 24 |
| 删除测试文件，删除前 | 53248 B，52 KiB | 13 个数据块 + 1 个间接块 | 112 |

一级间接文件重复覆盖后计数不变。删除文件后路径无法打开；离线查看其 inode，
Size、Links、Blockcount 均为零，块指针已清空。重启后其他文件计数保持正确。

## 离线检查：计数已修正，但整盘仍有其他错误

实际执行：

```sh
dd if=/tmp/zzz-block-count-g0govu/disk.img \
   of=/tmp/zzz-block-count-g0govu/after.ext2 \
   bs=512 skip=4096 count=126943 status=none
e2fsck -fn /tmp/zzz-block-count-g0govu/after.ext2
```

不再报告任何 i_blocks 不匹配，也未报告块位图差异。
但仍然以 4 退出，报告：

```text
Inodes that were part of a corrupted orphan linked list found. Fix? no
Inode 167 was part of the orphaned inode list. IGNORED.
Directories count wrong for group #0 (10, counted=13).
```

目录计数是上次已发现、尚未处理的问题。orphan 报告是本次新增删除用例发现的
独立待查问题，不能仅凭名称推断一定存在完整 orphan 链表实现。
未修复镜像、未修改相关内核路径。所有测试 QEMU 已退出。

## 限制

- 已有镜像里历史错误的 i_blocks 不会由新代码自动重算；请在副本上另行检查修复。
- 本次是正常分配／释放的记账修复，不是事务或崩溃一致性改造。
- 现有位图与超级块更新、块初始化及映射写入失败可能发生部分修改，计数包装
  无法判断底层错误是否已经部分落盘；递归释放中途失败的映射清理与重试也仍需专门完善。
- 没有测试这些底层部分写入故障、掉电恢复、多核并发和其他块大小。
- 三级间接只做函数级测试，未在实际镜像上创建超过 4 GiB 的稀疏文件。
