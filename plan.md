我建议计划分两条线，但先以 静态 Linux/POSIX 用户态兼容 为目标，不碰动态链接。

阶段 0：整理现状
先做干净地基：

给 user_proc/sbase/ 清理掉 upstream 生成物，比如 .o 文件不要纳入版本管理。
保留 upstream Makefile，另写 Makefile.zzz 或顶层包装 Makefile。
把你的 include/uapi/dirent.h 分清用途：
include/uapi/linux_dirent.h：内核/ABI 结构
include/dirent.h：给用户程序 include 的 libc 风格头
<dirent.h> 应该在 include/dirent.h，否则 sbase 还是会拿到 newlib 那个报错头。

阶段 1：先跑 sbase 静态工具
目标不是 Linux ABI，而是先补 POSIX 层。

第一批工具：

true false echo pwd cat
第二批：

touch mkdir rm rmdir chmod
第三批：

ls wc head tail tee
你需要先补：

opendir/readdir/closedir
fcntl 基础
chmod
rename
getuid/getgid stub
umask
/dev/null
测试标准：

echo hello
cat /etc/xxx
pwd
ls /
touch a
rm a
阶段 2：建立 Linux ABI 兼容层
不要改掉现有 SYSCALL_LIST。新增一套 Linux syscall 分发表，例如：

linux_syscall_table[]
RISC-V Linux 常用号优先支持：

openat      56
close       57
getdents64  61
lseek       62
read        63
write       64
fstat       80
exit        93
exit_group  94
brk         214
execve      221
mmap        222
munmap      215
mprotect    226
wait4       260
很多可以转调你现有实现：

openat(AT_FDCWD, path, flags, mode) -> sys_open
getdents64 -> 复用 iterate_dir，换 linux_dirent64 格式
wait4 -> waitpid 包装
exit_group -> exit
阶段 3：支持静态 musl 小程序
目标：

riscv64-linux-musl-gcc -static hello.c -o hello
要补：

Linux ELF 初始栈 auxv
AT_PAGESZ
AT_RANDOM
AT_UID/EUID/GID/EGID
AT_NULL
以及：

uname
clock_gettime
gettimeofday
ioctl stub
fcntl
先跑：

hello
cat
echo
简单文件读写程序
阶段 4：termios / tty / shell 基础
为 dash/busybox ash 做准备：

ioctl(TCGETS/TCSETS)
canonical/raw mode
echo on/off
Ctrl-C
Ctrl-D
/dev/tty
pipe2
dup3
FD_CLOEXEC
测试：

静态 dash 非交互脚本
静态 busybox ash 非交互脚本
阶段 5：再考虑动态链接
这个先别碰。动态链接需要：

PT_INTERP
ld.so
shared object mmap
relocation
GOT/PLT
TLS relocation
完整 auxv
现在做会分散火力。

我建议你的近期里程碑就定成：

M1: sbase true/false/echo/pwd/cat 跑起来
M2: sbase ls/touch/rm/mkdir 跑起来
M3: Linux syscall table 支持静态 hello
M4: 静态 musl/sbase 工具跑起来
M5: dash/busybox ash 非交互脚本
这样每一步都有明确产物，也不会陷进“我要一次性兼容 Linux”的大坑里。