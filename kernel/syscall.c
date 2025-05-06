/**
 * @FilePath: /ZZZ/kernel/syscall.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-02 18:11:27
 * @LastEditTime: 2025-05-02 18:59:49
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "riscv.h"
#include "usys.h"
#include "printf.h"

uint64_t sys_get_hart_id(void)
{
    return mhartid_r(); // 返回当前线程的 ID
}

typedef uint64_t (*sysfuncPtr)(void); // 定义函数指针类型 FuncPtr
static sysfuncPtr syscalls[] = {
    [SYSCALL_NUM_GET_HART_ID] = sys_get_hart_id, // 初始化函数指针数组，将系统调用号与对应的处理函数关联起来
};

void do_syscall(reg_context_t *ctx)
{
    uint64_t num = ctx->a7; // 获取系统调用号
    if (num < SYSCALL_NUM_END)
    {
        ctx->a0 = syscalls[num](); // 执行对应的处理函数，并将返回值存入 a0 中
    }
    else
    {
        printf("syscall %l not implemented\n", num);
        ctx->a0 = -1; // 如果系统调用号超出范围，则返回错误码 -1
    }
}
