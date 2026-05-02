/**
 * @FilePath: /ZZZ-OS/include/os/irq.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 14:39:47
 * @LastEditTime: 2025-11-13 23:44:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQ_H
#define __KERNEL_IRQ_H

#include <os/types.h>
#include <os/irqreturn.h>

struct irq_chip;
struct irq_domain;

typedef irqreturn_t (*irq_handler_t)(int virq, void *dev_id);
typedef void (*irq_deferred_fn_t)(void *arg);

#ifndef IRQ_PERCPU_MAX_CPUS
#define IRQ_PERCPU_MAX_CPUS 4
#endif

struct irq_data {
    const char *name;                   // 调试名称
    int virq;                     // 逻辑中断号
    struct irq_chip *chip;        // 指向控制器私有数据
    struct irq_domain *domain;    // 所属中断域
    irq_handler_t handler;        // 中断处理函数
    void *dev_id;                  // 设备标识符         
    int is_percpu;
    irq_handler_t percpu_handler[IRQ_PERCPU_MAX_CPUS];
    void *percpu_dev_id[IRQ_PERCPU_MAX_CPUS];
};

#ifndef IRQ_DEFERRED_WORK_MAX
#define IRQ_DEFERRED_WORK_MAX 16
#endif

struct irq_deferred_work {
    irq_deferred_fn_t fn;
    void *arg;
    volatile int pending;
};

extern void irq_init(void);
extern struct irq_data *irq_data_get(int virq);
extern int irq_request(int virq, irq_handler_t handler, const char *name, void *dev_id);
extern int irq_percpu_request(int virq, int cpu, irq_handler_t handler, const char *name, void *dev_id);
extern void irq_enable(int virq);
extern void irq_disable(int virq);
extern reg_t do_irq(reg_t ctx,void *arg);
extern void irq_set_priority(int virq, int priority);
extern int irq_get_priority(int virq);
extern void irq_send_ipi(int cpu_id, int ipi_id);
extern int irq_deferred_work_register(struct irq_deferred_work *work,
                                      irq_deferred_fn_t fn, void *arg);
extern void irq_deferred_work_queue(struct irq_deferred_work *work);
extern void irq_run_deferred_works(void);


#endif // __KERNEL_IRQ_H
