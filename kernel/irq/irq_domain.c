/**
 * @FilePath: /ZZZ-OS/kernel/irq/irq_domain.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-01 00:18:54
 * @LastEditTime: 2025-11-13 21:49:20
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <os/irq_domain.h>
#include <os/kmalloc.h>
#include <os/irq.h>

struct list_head irq_domains = LIST_HEAD_INIT(irq_domains);
static unsigned int next_virq_base = 0;

int irq_domain_alloc_virq_base(unsigned int count) {
    unsigned int virq_base = next_virq_base;
    next_virq_base += count;
    return virq_base;
}

struct irq_domain *irq_domain_create(struct device_node *np, unsigned int virq_base, unsigned int hw_irq_count) {
    struct irq_domain *domain;
    unsigned int i;

    domain = (struct irq_domain *)kmalloc(sizeof(struct irq_domain));
    if (!domain)
        return NULL;

    domain->np = np;
    domain->virq_base = virq_base;
    domain->hw_irq_count = hw_irq_count;

    domain->hw_to_virq = (int *)kmalloc(sizeof(unsigned int) * hw_irq_count);
    if (!domain->hw_to_virq) {
        kfree(domain);
        return NULL;
    }

    for (i = 0; i < hw_irq_count; i++) {
        domain->hw_to_virq[i] = -1;
    }

    list_add(&irq_domains, &domain->link);

    return domain;
}

void irq_domain_destroy(struct irq_domain *domain) {
    if (!domain)
        return;

    list_del(&domain->link);
    kfree(domain->hw_to_virq);
    kfree(domain);
}

/*
    * 将硬件中断号hwirq映射到虚拟中断号virq_base + hwirq，并返回映射后的虚拟中断号。
    * 如果hwirq无效（大于等于hw_irq_count），则返回-1。
*/
int irq_domain_add_mapping(struct irq_domain *domain, unsigned int hwirq) {
    if (hwirq >= domain->hw_irq_count) {
        return -1; // Invalid hwirq
    }
    domain->hw_to_virq[hwirq] = domain->virq_base + hwirq;
    return domain->hw_to_virq[hwirq];
}

int irq_set_hwirq_and_chip(struct irq_domain *domain, unsigned int hwirq, struct irq_chip *chip) {
    if (hwirq >= domain->hw_irq_count) {
        return -1; // Invalid hwirq
    }
    int virq = domain->virq_base + hwirq;
    struct irq_data *data = irq_data_get(virq);
    if (!data) {
        return -1; // Invalid virq
    }
    data->virq = virq;
    data->domain = domain;
    data->chip = chip;
    return 0;
}

// int irq_domain_of_add_mapping(struct device_node *np, unsigned int hwirq) {
//     struct irq_domain *domain = irq_find_host(np);
//     if (!domain) {
//         return -1; // Domain not found
//     }
//     return irq_domain_add_mapping(domain, hwirq);
// }

struct irq_domain *irq_domain_lookup(unsigned int virq) {
    struct irq_domain *domain;
    struct list_head *pos;

    list_for_each(pos, &irq_domains) {
        domain = list_entry(pos, struct irq_domain, link);
        if (virq >= domain->virq_base && virq < domain->virq_base + domain->hw_irq_count) {
            return domain;
        }
    }

    return NULL;
}

struct irq_domain* irq_find_host(struct device_node *np) {
    struct list_head *pos;
    struct irq_domain *domain;

    list_for_each(pos, &irq_domains) {
        domain = list_entry(pos, struct irq_domain, link);
        if (domain->np == np) {
            return domain;
        }
    }

    return NULL;
}

int irq_domain_get_virq(struct device_node *np, unsigned int hwirq) {
    struct list_head *pos;
    struct irq_domain *domain;

    list_for_each(pos, &irq_domains) {
        domain = list_entry(pos, struct irq_domain, link);
        if (domain->np == np) {
            if (hwirq < domain->hw_irq_count) {
                return domain->hw_to_virq[hwirq];
            } else {
                return -1; // Invalid hwirq
            }
        }
    }

    return -1; // Domain not found
}

int irq_domain_get_hwirq(struct irq_domain *domain, int virq ) {
    for (int i = 0; i<domain->hw_irq_count; i++) {
        if (domain->hw_to_virq[i] == virq) {
            return i;
        }
    }
    return -1;
}