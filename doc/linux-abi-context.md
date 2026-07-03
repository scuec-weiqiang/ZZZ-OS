# Linux ABI Porting Context

This document records the current state of the Linux ABI porting work on the
`linux-abi` branch.

## Goal

The user-space direction is moving from the original custom ABI/newlib runtime
toward the RISC-V Linux ABI. The current target is static Linux user programs:
simple C programs, `init`, shell, Lua, then sbase/POSIX tools.

Dynamic linking is intentionally out of scope for now.

## Current Status

- Static glibc user programs can start.
- `printf` works.
- The system can enter the shell.
- `lua -v` works.
- There are still missing Linux/POSIX syscalls, but the main user-mode path is
  now working.

## User Stack And Auxv

Linux libc expects the initial user stack to contain:

```text
argc
argv[]
NULL
envp[]
NULL
auxv[]
AT_NULL
```

The old stack only had `argc/argv/envp`, which caused libc startup to scan
garbage as auxv.

`fs/exec.c` now builds a Linux-style auxv with:

```text
AT_PHDR
AT_PHENT
AT_PHNUM
AT_ENTRY
AT_PAGESZ
AT_UID
AT_EUID
AT_GID
AT_EGID
AT_SECURE
AT_RANDOM
AT_NULL
```

`include/fs/binfmt.h` added ELF aux fields to `struct linux_binprm`:

```c
unsigned long elf_entry;
unsigned long elf_phdr;
unsigned long elf_phent;
unsigned long elf_phnum;
```

`fs/exec.c` has `prepare_elf_aux()` to parse ELF header/program headers before
constructing the initial stack.

A key bug was that `copy_strings()` had been commented out, leaving
`arg_start/env_start` unset. This made argv/envp pointers become zero, so glibc
could not find auxv and `_dl_random` stayed zero. Restoring `copy_strings()` and
setting `bprm->arg_start` / `bprm->env_start` fixed that path.

## Linux Syscall Numbers

`include/os/syscall_num.h` has been switched from the old custom syscall numbers
to RISC-V Linux ABI numbers. The old numbers are not preserved on this branch.

Currently wired or partially wired:

```text
17  getcwd
23  dup
24  dup3
34  mkdirat
35  unlinkat
48  faccessat
49  chdir
56  openat
57  close
59  pipe2
61  getdents
62  lseek
63  read
64  write
79  newfstatat
80  fstat
93  exit
94  exit_group
113 clock_gettime
114 clock_getres
129 kill
134 sigaction
139 sigreturn
172 getpid
201 ps
214 brk
215 munmap
220 clone
221 execve
222 mmap
226 mprotect
260 wait4
```

`SYSCALL_MAX` is now `512`.

## Syscall Wrappers

`kernel/syscall.c` contains temporary Linux ABI wrappers around existing kernel
implementations:

```text
exit_group  -> exit
openat      -> open
newfstatat  -> stat
faccessat   -> access
mkdirat     -> mkdir
unlinkat    -> unlink/rmdir
pipe2       -> pipe, only flags == 0
dup3        -> dup2, only flags == 0
wait4       -> waitpid, rusage ignored
clone       -> fork-like clone
```

`clock_gettime()` and `clock_getres()` are minimally implemented:

```text
CLOCK_REALTIME  = 0
CLOCK_MONOTONIC = 1
```

Both currently use `monotonic_ns()`. `clock_getres()` returns 1 ns.

## clone/fork

On RISC-V Linux, glibc `fork()` uses syscall `clone = 220`.

glibc was observed passing:

```text
flags = 0x1200011
```

Meaning:

```text
0x0000011  SIGCHLD
0x0200000  CLONE_CHILD_CLEARTID
0x1000000  CLONE_CHILD_SETTID
```

`sys_clone()` currently accepts:

```c
CSIGNAL | CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID
```

and then calls `sys_fork(ctx)`.

This is enough for fork-like behavior, but it does not yet implement:

- child TID write
- clear child TID on exit
- futex wake
- thread-style `CLONE_VM`, `CLONE_THREAD`, `CLONE_SETTLS`

## Memory Syscalls

The following Linux syscall numbers are connected:

```text
brk    = 214
munmap = 215
mmap   = 222
mprotect = 226
```

This fixed an earlier glibc TLS startup issue where `_dl_early_allocate()` used
`brk/mmap`, but the kernel still had the old custom syscall numbers.

## FPU

Lua initially crashed in `setjmp` due to RISC-V floating-point instructions.
Enabling the RISC-V FPU state/status allowed Lua to run.

## TTY History

A TTY layer was added to support libc line input such as `fgets()`.

The original UART `read(size=1024)` behavior waited until the full size was
read, which made `fgets()` hang. The TTY layer provides canonical input:

- line buffering
- `\r` to `\n`
- backspace handling
- echo
- output `\n` to `\r\n`

TTY locking was restored after debugging. The later shell instability was not
caused by TTY.

## Multi-Core exit/wait Bug

There was a multi-core bug where a child process woke the parent in `do_exit()`
before switching away from its own kernel stack. The parent, running on another
CPU, could `waitpid()` and free the child stack while the child was still using
it.

The fix delayed task destruction:

- added `TASK_DEAD`
- added `task_struct.on_cpu`
- `waitpid()` marks zombie children `TASK_DEAD`
- if the child is not on CPU, destroy immediately
- otherwise `sched_tail(prev)` destroys it after it has switched away

## Current Good Milestone

The kernel can now run basic static glibc user-space, enter the shell, and run:

```sh
lua -v
```

## Recommended Next Steps

1. Add temporary unknown-syscall logging in `do_syscall()`:

   ```c
   if (nr >= SYSCALL_MAX || syscall_table[nr] == NULL)
       printk("unknown syscall nr=%u\n", nr);
   ```

2. Implement Linux process/thread cleanup basics:

   ```text
   set_tid_address
   futex
   gettid
   CLONE_CHILD_SETTID
   CLONE_CHILD_CLEARTID
   clear_child_tid on exit
   ```

3. Improve syscall wrapper semantics:

   ```text
   openat dirfd/flags/mode
   newfstatat flags
   faccessat flags
   unlinkat AT_REMOVEDIR
   mkdirat dirfd
   wait4 rusage
   ```

4. Verify ABI struct layouts:

   ```text
   struct stat
   linux_dirent64
   timespec
   errno returns
   ```

5. Start running sbase tools and fill missing Linux/POSIX behavior one failure
   at a time.

