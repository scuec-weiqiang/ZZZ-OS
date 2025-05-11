.section .text.init
.global _start
_start:
    # 关闭中断
    csrw mie, zero

    csrr tp, mhartid # 读取hartid暂存到tp中
    bnez tp, wait_hart

    # 栈设置
    la t0 , _stack_size 
    mul t1 , tp , t0 # t1 = hartid * STACK_SIZE 
    la t2, _stack_end
    sub sp,t2,t1 # 栈底 = _stack_end - hartid * STACK_SIZE

    # # 初始化mscratch指向栈顶（中断入口用）
    # csrw mscratch, _kernel_stack_start

    # 跳转到C函数
    call init
    j .

wait_hart:
    wfi
    j wait_hart

.end



