# Ext2 读取入口调整

OpenAI Codex 生成。此次仅接通 `allow_readahead` 参数，不实现预读调度。

- `ext2_file_read()` 直接调用 `pagecache_read_page()`。
- 同一次读取跨页后允许预读；`disabled` 仍然优先。
- 仅实际返回数据后更新 `prev_end`，记录实际读取终点。
- 文件读取长度先按 EOF 截断，每次复制再按当前页剩余空间截断。
- 页缓存读取拒绝空文件和文件最后一页之后的页号，在分配页面前检查。
- 删除旧接口声明并迁移旧预读测试的调用；旧测试依赖真实预读，目前仍不能通过，不代表此次入口测试失败。

## 实际验证

`make -s -j4 build/kernel.elf` 成功。

独立测试命令（仓库根目录）：

```sh
riscv64-linux-gnu-gcc -std=gnu11 -D__KERNEL__ -static -O2 \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -fno-builtin -DSYS_BITS=64 -Iinclude \
    test/fs/file_read_entry_test.c -o /tmp/zzz-file-read-entry-test
qemu-riscv64 /tmp/zzz-file-read-entry-test
```

实际输出：`file-read-entry-test: PASS`。

| 模拟检查 | 结果 |
| --- | --- |
| 首次大读取跨三页 | 首次不允许预读，后续两页允许 |
| 请求超过 EOF | 仅返回文件有效长度，前后保护字节不变 |
| 从页末最后一个字节开始读两个字节 | 分两页复制，内容正确 |
| 相邻两次读取 | 第二次入口允许预读 |
| 禁用预读 | 所有调用均不允许 |
| 第一页读取失败 | 保留原读取历史与文件位置 |
| 后续页读取失败 | 返回已读长度，历史更新到实际终点 |
| 负偏移、空文件、EOF 读取 | 不请求缓存页 |

测试替换了页缓存为模拟接口，验证的是实际 Ext2 读取函数；不是实际磁盘、真实页缓存或并发截断测试。
尚未启动 QEMU 内核运行端到端文件读取，也未测量性能。
