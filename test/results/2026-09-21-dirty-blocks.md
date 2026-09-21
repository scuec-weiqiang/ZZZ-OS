# 页内脏块位图：初步实现与测试

本实现及独立测试由 OpenAI Codex 生成。

## 设计范围

`page.dirty_blocks` 的每位表示一个文件系统块，而不是磁盘扇区。
最多支持每页 8 块；块大小和块数从文件系统推导，不在每页重复存储。

| PageDirty | dirty_blocks | 含义 |
| --- | --- | --- |
| 0 | 0 | 干净页 |
| 1 | 非零 | 仅位图选中的块需要写回 |
| 1 | 0 | 兼容旧调用：写回页内全部已映射块 |

普通文件写入使用 `pagecache_mark_dirty_range()`；目录原来的
`SetPageDirty()` 保持整页语义。已标记整页脏的页不会被局部标记缩小范围。
写回成功后由 Page Cache 清除页脏标记及位图；失败保留全部脏位，允许重试。
部分 batch 已成功而后续失败时，也保留全部脏位，不追踪逐 batch 完成情况。
显式标脏的块若没有磁盘映射，返回错误，不在写回阶段分配。

现有单页函数的函数体和调用者仍按单页工作，因此将未完成的
`ext2_rwpages(struct page **, ...)` 签名暂时恢复为匹配函数体的
`ext2_rwpage(struct page *, ...)`。没有实现跨页写回或重构预读。

## 独立模拟测试

从仓库根目录运行，需要 RISC-V Linux 交叉编译器和用户态 QEMU：

```sh
riscv64-linux-gnu-gcc -std=gnu11 -D__KERNEL__ -static -O2 \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -fno-builtin -DSYS_BITS=64 -Iinclude \
    test/fs/dirty_blocks_test.c -o /tmp/zzz-dirty-blocks-test
qemu-riscv64 /tmp/zzz-dirty-blocks-test
```

测试直接包含现有 Ext2 单页实现，以模拟块映射和磁盘写入替代真实设备。
不接入内核启动，不访问磁盘镜像。实际执行输出：

```text
dirty-blocks-test: PASS
```

| 检查项 | 实际结果 |
| --- | --- |
| 1 KiB 块，修改页内 900～1199 字节 | 位图为 0x03 |
| 再修改块 3 | 位图为 0x0b |
| 越界范围、不支持的块大小 | 返回 EINVAL，原位图不变 |
| 零长度修改 | 不设置脏标记 |
| 512 B 块，整页修改 | 位图为 0xff |
| 4 KiB 块，修改最后一个字节 | 位图为 0x01 |
| 整页脏后再标记局部 | 保留整页脏语义 |
| 脏块 0、1、3；磁盘块号 100～103 | 提交两次，分别为 2 KiB、1 KiB，跳过块 2 |
| 第二次提交失败 | 返回 EIO，保留 0x0b 脏位 |
| 显式脏块没有磁盘映射 | 返回 EIO |
| 旧整页脏调用，尾部存在空洞 | 只写三个已映射块，共 3 KiB |

相关 Ext2 与 Page Cache 对象文件编译通过。
完整内核构建仍受现有预读重构影响：`ext2_file_read()` 调用的
`pagecache_read_file_page()` 当前没有定义；本次不代替用户完成这部分重构。

## 尚未验证或解决

- 没有进行真实文件创建、落盘后重新读取、重启恢复等端到端测试。
- 没有性能测量，不能声称已有吞吐提升。
- 沿用现有页锁；它仍是自旋锁，且写回期间持锁，多核与可睡眠 I/O 锁设计需单独处理。
- 普通文件修改时仅将设置位图和内存复制放在同一个现有页锁内，不能据此宣称整个文件系统并发安全。
- 不改变现有 `i_size` 更新、磁盘块分配和文件扩展失败语义。
