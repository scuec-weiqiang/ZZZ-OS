# ZZZ-OS 内核测试

测试按照内核子系统分类，通过内核配置项独立启用。正确性测试首先输出
`START`，结束时输出一条 `PASS` 或 `FAIL` 记录，其中包含实际执行的用例数、
校验字节数和耗时。

当前测试包括：

- `CONFIG_TEST_MM_ALLOCATOR`：覆盖 Slab 大小边界、对象释放与重新分配、
  数据 pattern 校验以及 Buddy 多页分配。
- `CONFIG_TEST_MM_BENCH`：分别测试 Slab 小对象和 Buddy 页分配的单次
  alloc/free 与批量分配性能，记录每组“分配后释放”的耗时和每秒完成组数。
- `CONFIG_TEST_MM_FRAGMENTATION`：最多占用约 96 MiB 内存，以交错释放方式制造
  order-0 隔离页，再持续申请 2 MiB 连续块直到失败；记录各 order 空闲块、
  外部碎片率和完全释放后的内存恢复情况。
- `CONFIG_TEST_BLOCK_IO`：执行只读块设备测试，覆盖磁盘扇区边界、内存页边界、
  非对齐缓冲区和跨页缓冲区；读取结果与对齐基准数据比较，并使用前后 guard
  检查越界写入。
- `CONFIG_TEST_FS_READ`：测试不同分块大小、Ext2 间接块边界、固定 seed 随机读取，
  以及重复打开、读取和关闭文件；所有结果都与基准文件数据比较。
- `CONFIG_BLK_READ_BENCH`：块设备和文件读取性能测试。
- `CONFIG_TEST_MEMORY_USAGE`：在测试前后采集内存快照，区分保留内存、Buddy
  空闲页、PCP 空闲页、已使用页、Slab 页和 Page Cache 页。

`memory-usage` 中各字段含义：

| 字段 | 含义 |
| --- | --- |
| `total_KiB` | 设备树向内核声明的物理内存 |
| `reserved_KiB` | 未交给 Buddy 管理的内核、Memblock、NOMAP 等内存 |
| `managed_KiB` | Buddy 管理的内存总量 |
| `used_KiB` | 已经从 Buddy/PCP 分配出去的受管内存 |
| `free_KiB` | Buddy 和所有 PCP 中仍可分配的内存 |
| `slab_KiB` | 已使用页中属于 Slab 的诊断性统计 |
| `pagecache_KiB` | 已使用页中属于 Page Cache 的诊断性统计 |

`slab_KiB` 和 `pagecache_KiB` 已包含在 `used_KiB` 中，不能重复相加。

碎片测试输出的 `fragmentation_bp` 以万分之一为单位，例如 `6750` 表示
67.50%。它表示空闲页中位于目标 order 以下、当前无法直接满足目标连续内存
请求的比例。`large_block_exhausted` 阶段若仍有大量空闲页，而目标 order 已经
分配失败，即为外部碎片的直接表现。`recovered` 的空闲页数必须与测试前一致。

块设备和文件系统正确性测试目前都是只读测试。带有写入操作的破坏性测试必须
使用单独的测试磁盘，禁止直接操作根文件系统所在磁盘。

每次具有参考价值的测试结果保存在 `test/results/` 中。修改 Buddy、Slab、BIO、
VirtIO、Page Cache 或文件系统后，可以与已有结果比较正确性、checksum 和性能变化。
