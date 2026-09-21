# 固定四页顺序预读

OpenAI Codex 实现及测试记录。

## 行为

- `pagecache_read_page()` 当前页命中直接返回，不额外扩展窗口。
- 当前页未命中、允许预读且文件系统提供 readpages 时，准备当前页及后面最多三页。
- 预读不跨 EOF；先取得引用，再按页号顺序锁页，避免分配触发回收时重复锁页。
- 跳过有效、脏及 Writeback 页；只将需要读入的页交给文件系统。
- 额外页分配失败停止扩展，保留当前页读取；没有 readpages 时使用单页接口。
- 批量成功后标记参与页 Uptodate；失败时只重读当前页，额外页不标有效。
- 额外页解锁并释放引用，当前页的引用交给调用者。
- 同步固定窗口，无动态增长、异步线程或命中时的提前补充。
- 沿用现有自旋页锁；没有解决睡眠 I/O 持锁、多核缓存查找竞争或并发截断问题。

## 实际验证

临时配置启用 `CONFIG_TEST_READAHEAD = y`，不修改工作区默认配置。
QEMU 单核、实际 VirtIO/Ext2 镜像副本，目录 `/tmp/zzz-ra-four-AKDmnq/`。
原磁盘镜像没有写入。测试后 QEMU 退出，构建恢复默认配置。

`test/fs/readahead_test.c` 已从旧动态窗口预期调整为固定四页：
首次读取一页不预读，下一次顺序读取形成四页批次；随机跳转只填当前页。
EOF 复制边界、批量错误注入后单页回退、601 页读取与缓存回收均通过。
最终输出 `readahead-test: PASS ret=0`。

## 性能实测

目标 `/bin/ls`，每轮 1,031,696 B（约 1007.52 KiB），每种模式三轮，交替开关预读。
每轮清空目标文件数据页缓存；宿主机缓存和间接块元数据已预热。
计时包含文件读取和逐字节校验和计算，不代表物理冷盘性能。

| 请求大小 | 关闭预读平均耗时 | 开启预读平均耗时 | 关闭吞吐 | 开启吞吐 |
| --- | ---: | ---: | ---: | ---: |
| 512 B | 45.821 ms | 36.099 ms | 21.47 MiB/s | 27.25 MiB/s |
| 4 KiB | 40.573 ms | 29.558 ms | 24.25 MiB/s | 33.29 MiB/s |
| 64 KiB | 41.644 ms | 29.701 ms | 23.63 MiB/s | 33.13 MiB/s |

所有轮次 checksum 均为 `e9e447d5a6074d7f`。
本次 4 KiB 请求平均耗时减少约 27.1%，吞吐增加约 37.3%；单次 QEMU 测量有波动。

原始输出：

```text
readahead-test: request=512 enabled=0 rounds=3 bytes=1031696 avg_us=45821 KiB_s=21987 checksum=e9e447d5a6074d7f
readahead-test: request=512 enabled=1 rounds=3 bytes=1031696 avg_us=36099 KiB_s=27909 checksum=e9e447d5a6074d7f
readahead-test: request=4096 enabled=0 rounds=3 bytes=1031696 avg_us=40573 KiB_s=24831 checksum=e9e447d5a6074d7f
readahead-test: request=4096 enabled=1 rounds=3 bytes=1031696 avg_us=29558 KiB_s=34085 checksum=e9e447d5a6074d7f
readahead-test: request=65536 enabled=0 rounds=3 bytes=1031696 avg_us=41644 KiB_s=24193 checksum=e9e447d5a6074d7f
readahead-test: request=65536 enabled=1 rounds=3 bytes=1031696 avg_us=29701 KiB_s=33921 checksum=e9e447d5a6074d7f
readahead-test: PASS ret=0
```
