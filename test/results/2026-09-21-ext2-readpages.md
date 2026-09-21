# Ext2 多页读取完善与模拟测试

由 OpenAI Codex 生成。本次不实现 Page Cache 预读调度，不修改单页读取路径。

## 接口约定

上层提供同一个 mapping 下、按 index 严格递增的页面，不要求页号连续。
所有页面必须是允许重新填充的页面：不能为 Uptodate、Dirty 或 Writeback。
后续 ASSERT 改造后，空页、不同 mapping、上述页状态和不递增页号属于内部调用错误，
会触发 panic；EOF 范围和 I/O 等运行时错误仍返回错误码。
上层持有引用，并负责读入期间的页面独占及防止并发截断。
函数在清零页面前校验整个页数组及文件范围，拒绝 EOF 之后的页和重复页。
函数不修改页状态。返回错误时，部分页可能已被修改，上层不能将整批标为有效。

每批最多使用 16 个 BIO vec，不再使用预读窗口常量。更多页自动分批。
磁盘不连续、出现空洞、vec 已满或超过 max_hw_sectors 时提交当前 batch。
max_hw_sectors 为零表示未指定长度限制；非零但小于一个文件系统块时返回 EINVAL，
本版不进一步将一个文件系统块拆为多个请求。
映射到的磁盘块还会检查是否落在目标分区内。

VirtIO 驱动原先把 max_hw_sectors 固定为 1，与已有多扇区功能不符。
本次改为 virtqueue 描述符数减 2：按一个扇区占一个数据描述符的最坏情况
保守估计，预留请求头和状态描述符。这是软件保守上限，不是设备能力探测结果。
队列尚未提供通用 max_segments 字段，未来接入其他驱动还需完善该约束。

## 验证

内核编译链接成功。模拟测试不访问真实磁盘：

```sh
riscv64-linux-gnu-gcc -std=gnu11 -D__KERNEL__ -static -O2 \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -fno-builtin -DSYS_BITS=64 -Iinclude \
    test/fs/ext2_readpages_test.c -o /tmp/zzz-ext2-readpages-test
qemu-riscv64 /tmp/zzz-ext2-readpages-test
```

实际输出：`ext2-readpages-test: PASS`。

| 检查项 | 实际结果 |
| --- | --- |
| 连续 20 页，无长度限制 | 两批，64 KiB 和 16 KiB |
| 连续两页，长度限制 2 KiB | 四批，每批 2 KiB |
| 文件空洞 | 对应内存保持为零 |
| 文件大小为 4 KiB + 17 B | 末页前 17 B 为读取内容，其余为零 |
| 第二批失败 | 返回 EIO，不设置页面 Uptodate |
| 页数组包含脏页 | 在清零和提交前拒绝，已有数据保持不变 |
| 有效页、重复页、mapping 不一致、EOF 外页面 | 拒绝 |
| 磁盘块越过分区末尾 | 返回 EIO |
| 块映射失败 | 返回错误 |

这是模拟块映射与 BIO 提交的函数级测试，不是实际设备端到端测试。
尚未验证多核并发、并发截断，也未进行性能测量。

## ASSERT 改造后复测

模拟测试通过替换 panic 为非局部跳转验证断言分支，不会真的挂起 QEMU。
空数组、脏页、有效页、重复页和 mapping 不一致均触发断言；EOF 外页仍返回 EINVAL，
磁盘映射及提交失败仍返回 EIO。默认编译和额外添加 `-DNDEBUG` 编译均输出
`ext2-readpages-test: PASS`，同时确认断言条件只求值一次。

`include/os/check.h` 现在提供 `ASSERT(expr, msg)`，始终生效，失败打印表达式、
文件、行号、函数及消息并进入 panic。旧 CHECK 已移除：内核源码中 53 处明确的
内部约定转为 ASSERT，其余 201 处展开为普通 if，保留清理和返回逻辑。
第三方 tools/dtc 的同名宏不在本次替换范围。
RISC-V 内核编译链接通过；没有进行内核启动或实际磁盘回归测试。
