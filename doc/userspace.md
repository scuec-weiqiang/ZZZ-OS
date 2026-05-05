# 用户态支持

[TOC]

## 1. 概述

ZZZ-OS 支持运行基于 **newlib** C 库的用户态程序。通过提供 newlib 所需的 syscall 接口 stub，用户程序可以使用标准 C 库函数（printf、malloc、open 等）而无需直接处理内核系统调用。

核心组件：
- **内核 syscall 层**：`kernel/syscall.c` — 系统调用分发
- **用户 runtime 层**：`user_runtime/` — newlib stubs、crt0、链接脚本
- **用户程序**：`user_proc/` — init、shell、测试程序

## 2. 系统调用

### 2.1 内核侧

系统调用通过单一数据源 `SYSCALL_LIST` 宏定义：

```c
// include/os/syscall_num.h
#define SYSCALL_LIST \
    X(1,  exit)    \
    X(2,  fork)    \
    X(3,  read)    \
    X(4,  write)   \
    X(5,  open)    \
    X(6,  close)   \
    X(20, getpid)  \
    X(45, brk)     \
    X(46, creat)   \
    X(47, mkdir)   \
    X(59, execve)  \
    X(106, waitpid) \
    X(141, getdents) \
```

该宏同时生成：
- 系统调用号枚举（`SYSCALL_exit = 1`, ...）
- 内核处理函数声明（`sys_exit()`, `sys_read()`, ...）
- 系统调用分发表（`syscall_table[]`）

新增系统调用只需在 `SYSCALL_LIST` 中添加一行。

### 2.2 用户侧 Stub

`user_runtime/syscalls.c` 实现了 newlib 所需的 `_` 前缀函数：

```c
// 文件 I/O
int _write(int fd, const char *buf, int nbytes);
int _read(int fd, char *buf, int nbytes);
int _open(const char *pathname, int flags, ...);
int _close(int fd);
int _creat(const char *pathname, mode_t mode);

// 进程控制
int _fork(void);
void _exit(int status);
int _execve(const char *path, char *const argv[], char *const envp[]);
int _wait(int *status);
int _getpid(void);

// 内存管理
void *_sbrk(int nbytes);  // 对应内核的 sys_brk

// 其他
int _fstat(int fd, struct stat *buf);
int _isatty(int fd);
int getdents(int fd, struct dirent *buf, int count);
```

每个 stub 通过内联汇编触发对应的系统调用号，参数通过架构特定的寄存器传递。

## 3. 用户程序启动流程

```
内核 do_execve("/bin/init")
  → 解析 ELF，建立 VMA 映射
  → 设置 PC = ELF 入口点, SP = 用户栈顶
  → 异常返回到用户态

用户态 crt0.S
  → 设置栈指针
  → 调用 main()
  → main() 返回后调用 _exit()
```

### 3.1 C Runtime（crt0.S）

用户程序入口点，负责：
1. 设置用户态栈指针
2. 调用 `main()` 函数
3. `main()` 返回后调用 `_exit()` 终止进程

### 3.2 用户态链接脚本

```ld
/* user_runtime/user.ld */
OUTPUT_FORMAT("elf32-littlearm")
ENTRY(_start)

SECTIONS {
    . = 0x10000;  /* 用户态起始地址 */
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
```

## 4. ELF 加载器

`fs/elf.c` 和 `fs/elf_binfmt.c` 实现 ELF 可执行文件加载：

1. 验证 ELF magic (`\x7fELF`)
2. 解析 ELF header 获取入口点、段信息
3. 遍历 program header，为 `PT_LOAD` 段建立 VMA 映射
4. 设置用户态寄存器的入口点和栈指针
5. 注册为 `linux_binfmt`，通过 `do_execve()` 调用

采用 **lazy map** 策略：仅在 VMA 中记录映射关系，实际页面在缺页异常中按需分配和映射。

## 5. 用户态程序

### 5.1 init（第一个用户进程）

`user_proc/init/init.c` 是内核启动的第一个用户态程序：

```c
int main(void) {
    // 初始化 stdio
    // fork + execve 启动 shell
    pid_t pid = fork();
    if (pid == 0) {
        execve("/bin/sh", argv, NULL);
    }
    // 等待 shell 退出
    waitpid(pid, &status, 0);
    return 0;
}
```

### 5.2 hello

`user_proc/hello/hello.c` — 简单的 "hello world" 测试程序。

### 5.3 simple-c-shell

`user_proc/simple-c-shell/` — 基于 git submodule 的简易 C 语言 shell，支持：
- 命令执行
- 后台任务
- 基本 shell 交互

## 6. 进程隔离

每个用户进程拥有独立的：
- **地址空间**：独立的 `mm_struct` 和页表
- **文件描述符表**：独立的 `files_struct`
- **工作目录**：独立的 `fs_struct`（cwd、root）
- **PID**：全局唯一的进程 ID

内核通过 VMA 的 lazy map 和 page fault 机制确保用户态内存访问的安全隔离。

## 7. 构建用户程序

```bash
# 构建所有用户程序
make u

# 清理
make uc
```

每个用户程序在 `user_proc/` 下有独立的 `Makefile`，指定交叉编译器和链接脚本。
