/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/clint.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-05-10 17:37:11
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/*******************************************************************************************
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/clint.h
 * @Description  : 核心本地中断控制器(Core Local Interruptor)头文件 ，用于定时器中断和软件中断的触发。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 16:29:49
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#ifndef CLINT_H
#define CLINT_H

#include "types.h"
#include "platform.h"

#define CLINT_BASE          0x02000000

#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP_BASE         (CLINT_BASE + (0x4000))

#define CLINT_MSIP(hartid)          (CLINT_BASE + 4*(hartid))

#define RELEASE_CORE(hartid)        (*(uint32_t*)CLINT_MSIP(hartid)=1)

/**
 * @brief 获取当前机器时间（Machine Time）
 *
 * 该函数用于获取当前的系统机器时间。机器时间是一个连续的计数器，从系统启动开始计时。
 *
 * @return 返回当前机器时间的64位无符号整数值。
 */
__SELF __INLINE uint64_t __clint_mtime_get()
{
    return *(uint64_t*)CLINT_MTIME;
}

/**
 * @brief 设置CLINT的时间比较寄存器
 *
 * 该函数用于将指定的hartid对应的时间比较寄存器设置为指定的值。
 *
 * @param hartid  hartid，表示要设置的hart的ID
 * @param value   要设置的值
 */
__SELF __INLINE void __clint_mtimecmp_set(uint32_t hartid,uint64_t value)
{
    uint64_t *clint_mtimecmp = (uint64_t*)CLINT_MTIMECMP_BASE;
    clint_mtimecmp[hartid] = value;
}


/**
 * @brief 向指定的 hart 发送 MSIP 中断
 *
 * 通过向指定的 hart 写入 1 来触发 MSIP 中断。
 *
 * @param hart_id 指定发送中断的 hart 的 ID
 */
__SELF __INLINE void __clint_send_ipi(hart_id_t hart_id) {
    volatile uint32_t *msip = (volatile uint32_t*)(CLINT_MSIP(hart_id));
    *msip = 1;  // 写1触发MSIP中断
}


/**
 * @brief 清除指定HART的MSIP中断
 *
 * 该函数用于清除指定HART（硬件线程）的MSIP（Machine Software Interrupt）中断。
 *
 * @param hart_id 要清除MSIP中断的HART ID
 */
__SELF __INLINE void __clint_clear_ipi(hart_id_t hart_id) {
    volatile uint32_t *msip = (volatile uint32_t*)(CLINT_MSIP(hart_id));
    *msip = 0;  // 写0清除MSIP中断
}

#endif 