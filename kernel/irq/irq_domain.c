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
#include <os/malloc.h>

struct list_head irq_domains = LIST_HEAD_INIT(irq_domains);
static unsigned int next_virq_base = 0;

int irq_domain_alloc_virq_base(unsigned int count) {
    unsigned int virq_base = next_virq_base;
    next_virq_base += count;
    return virq_base;
}

struct irq_domain *irq_domain_create(struct irq_chip *chip, unsigned int virq_base, unsigned int hw_irq_count) {
    struct irq_domain *domain;
    unsigned int i;

    domain = (struct irq_domain *)malloc(sizeof(struct irq_domain));
    if (!domain)
        return NULL;

    domain->chip = chip;
    domain->virq_base = virq_base;
    domain->hw_irq_count = hw_irq_count;

    domain->hw_to_virq = (int *)malloc(sizeof(unsigned int) * hw_irq_count);
    if (!domain->hw_to_virq) {
        free(domain);
        return NULL;
    }

    for (i = 0; i < hw_irq_count; i++) {
        domain->hw_to_virq[i] = -1;
    }

    list_add(&irq_domains, &domain->link);
    chip->priv = domain;

    return domain;
}

void irq_domain_destroy(struct irq_domain *domain) {
    if (!domain)
        return;

    list_del(&domain->link);
    free(domain->hw_to_virq);
    free(domain);
}

int irq_domain_add_mapping(struct irq_domain *domain, unsigned int hwirq) {
    if (hwirq >= domain->hw_irq_count) {
        return -1; // Invalid hwirq
    }
    domain->hw_to_virq[hwirq] = domain->virq_base + hwirq;
    return domain->hw_to_virq[hwirq];
}

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

int irq_domain_get_virq(struct irq_chip *chip, unsigned int hwirq) {
    struct list_head *pos;
    struct irq_domain *domain;

    list_for_each(pos, &irq_domains) {
        domain = list_entry(pos, struct irq_domain, link);
        if (domain->chip == chip) {
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