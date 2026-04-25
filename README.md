Linux 用户进程创建的完整调用链
路径一：通过 fork + execve 创建（标准方式）

用户调用 execve()
  → do_execve()
    → do_execveat_common()          ← exec.c 核心入口
      ├── alloc_bprm()              分配 linux_binprm
      ├── bprm_mm_init()            为新进程分配 mm_struct
      ├── copy_strings()            把 argv/envp 从用户空间拷到内核
      ├── prepare_binprm()          读取 ELF 文件头 (128字节)
      └── exec_binprm()
          └── search_binary_handler()  ← 遍历已注册的 binfmt 链表
              └── fmt->load_binary()     ← 对 ELF 就是 load_elf_binary()

load_elf_binary()                  ← binfmt_elf.c 核心函数
  ├── 解析 ELF header + program headers
  ├── flush_old_exec()             ← 替换旧进程的 mm
  ├── setup_new_exec()             ← 设置新进程身份
  ├── setup_arg_pages()            ← 映射用户栈，拷贝 argv/envp
  ├── 循环映射 PT_LOAD segments
  ├── set_binfmt()                 ← 注册 binfmt
  ├── create_elf_tables()          ← 在栈上放置 auxv 等启动信息
  └── start_thread(regs, pc, sp)   ← 设置 pt_regs，准备返回用户态
路径二：内核直接创建用户进程（fork + exec 内核态版本）

kernel_execve(path)
  → do_execve(getname(path), ...)
      → ... (同上)
你的项目需要的关键函数
按依赖顺序，分三个阶段：

第一阶段：异常处理 + 用户模式入口（必须先做，否则用户进程一跑就 panic）
函数	位置	作用
swi_handler (完整实现)	arch/arm/kernel/vector.S	保存 r0-r12+sp+lr+cpsr 到内核栈上的 pt_regs，调用 do_syscall()
data_abort_handler (完整实现)	arch/arm/kernel/interrupt_handler.c	读 DFAR/DFSR，调用 do_page_fault()
prefetch_abort_handler (完整实现)	arch/arm/kernel/interrupt_handler.c	读 IFAR/IFSR，调用 do_page_fault()
arch_user_enter	需要新建	从 SVC 切换到 USR 模式的汇编代码：恢复 pt_regs 到 r0-r12/sp/lr，设置 SPSR=USR_MODE，用 subs pc, lr, #4 返回
ret_from_fork (修改)	arch/arm/kernel/ret.S	用户进程不走内核线程那条路，需要调用 arch_user_enter 返回用户态
核心难点：SVC→USR 模式切换。ARMv7-A 不允许直接从 SVC 模式用 movs pc, lr 切换到 USR，必须通过异常返回机制：


// 在 SVC 模式下：
msr spsr_cxsf, #USR_MODE    // 设置 SPSR 为 USR 模式
// 恢复寄存器...
subs pc, lr, #4              // 异常返回，切换到 USR
第二阶段：exec 实现 + 用户进程创建
函数	位置	作用
struct linux_binfmt	需要新建	二进制格式注册表结构（可简化为只支持 ELF）
do_execve()	需要新建 fs/exec.c	exec 入口：分配 bprm → 打开文件 → bprm_mm_init → 拷贝参数 → search_binary_handler
load_elf_binary()	需要新建 fs/elf_binfmt.c	解析 ELF → flush_old_exec → setup_arg_pages → 映射 segments → create_elf_tables → start_thread
start_thread() / arch_user_enter()	arch/arm/kernel/process.c	设置 pt_regs（cpsr=USR_MODE, pc=entry, sp=栈顶），标记进程为用户模式
copy_thread() (补充)	arch/arm/kernel/process.c	你已有框架，需要完善用户进程路径：复制父进程 pt_regs + 设置 r0=0
第三阶段：用户态 syscall 封装
函数	位置	作用
ARM 版 usys.S	user/usys.S	把 RISC-V 的 ecall 改为 ARM 的 svc #0，用 r7 传 syscall 号
ARM 版用户进程链接脚本	user_proc/link.ld	需要 ARM 版本（当前是 riscv64）
do_syscall()	需要新建 kernel/syscall.c	根据 pt_regs.r7 分发系统调用，结果写回 r0
建议的实现顺序

1. swi_handler 汇编 (保存 pt_regs + 调用 do_syscall)
2. do_syscall 分发框架 (先只实现 1-2 个 syscall，如 print/yield)
3. arch_user_enter 汇编 (SVC→USR 切换 + 寄存器恢复)
4. start_thread (设置 pt_regs)
5. 修改 ret_from_fork 支持用户进程返回
6. 修改 copy_thread 用户路径
7. 修改用户态 syscall 封装为 ARM 语法
8. ARM 用户进程链接脚本
9. 创建第一个测试用户进程 (从内核直接调用 do_execve)
10. 完善 load_elf_binary + do_execve
11. page_fault 完善 (按需分配物理页)
你觉得这个方案怎么样？有没有想调整的地方，或者想直接从哪个阶段开始实现？