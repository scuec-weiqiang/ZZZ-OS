#include "save_reg.s"

.text
.global _switch
_switch:
# 存储当前任务上下文内容的结构体的地址 保存在mscratch里
# 交换mscratch与t6的值（因为mscratch属于csr寄存器，无法直接用来寻址，必须用通用寄存器作为中介，这里我们选t6作为中介）
    csrrw t6,mscratch,t6 
    beq t6, zero, 1f # 如果mscratch值为0，则代表当前没有任务上下文需要保存（例如：第一个任务还没开始执行）
    store_reg t6
    csrr t5,mscratch #把原来t6的值放入t5
    STORE t5,31*REG_SIZE(t6)#再把原来t6的值存入结构体中

    csrr a3,mepc
    addi a3,a3,4//指向下一条要执行的指令
    sw a3,128(t6) //存储mepc
# 至此保存当前任务上下文完成，下面恢复下一个任务的上下文


# _switch_to会在sched.c文件中声明为一个函数 void _switch_to(reg_context_t* next)
# 参数reg_context_t* next指向保存着下个任务上下文内容的地址，它通过寄存器a0传递
1:   
    csrw mscratch ,a0 #下个任务上下文内容的地址保存进mscratch
    mv t6,a0 

    lw a0,128(t6)
    csrw mepc,a0 //恢复mepc

    load_reg t6 
    #注意：这里如果不用t6（例如直接用a0）,那么在执行宏 load_reg a0 时会执行 LOAD a0 ,18*REG_SIZE(a0)，
    #从此之后a0值会被覆盖掉，后面的操作都是从错误的地址加载值到寄存器，因此要保证宏load_reg的参数对应的寄存器最后被覆盖
    mret 
    
# .global __sw_without_save 
# __sw_without_save:
#     csrw mscratch ,a0 
#     mv t6,a0 
#     lw a0,128(t6)
#     csrw mepc,a0 
#     load_reg t6 
#     ret
    
.end

