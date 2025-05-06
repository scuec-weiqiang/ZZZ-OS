/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/platform.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 23:59:39
 * @LastEditTime: 2025-05-01 22:25:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef PALTFORM_H
#define PALTFORM_H

#define LENGTH_RAM 128*1024*1024

typedef enum hart_id{
    HART_0,
    HART_1,
    MAX_HARTS_NUM,
}hart_id_t;

#define SYS_CLOCK_FREQ 10000000

#endif