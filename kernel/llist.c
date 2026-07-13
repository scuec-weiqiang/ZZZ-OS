#include <os/llist.h>

struct llist_node *llist_del_first(struct llist_head *head)
{
    struct llist_node *entry;
    struct llist_node *next;

    entry = smp_load_acquire(&head->first);
    do {
        if (entry == NULL)
            return NULL;

        next = READ_ONCE(entry->next);
    } while (!try_cmpxchg(&head->first, &entry, next));

    return entry;
}

bool llist_del_first_this(struct llist_head *head, struct llist_node *this)
{
    struct llist_node *entry;
    struct llist_node *next;

    entry = smp_load_acquire(&head->first);
    do {
        if (entry != this)
            return false;

        next = READ_ONCE(entry->next);
    } while (!try_cmpxchg(&head->first, &entry, next));

    return true;
}

struct llist_node *llist_reverse_order(struct llist_node *head)
{
    struct llist_node *new_head = NULL;

    while (head) {
        struct llist_node *tmp = head;
        head = head->next;
        tmp->next = new_head;
        new_head = tmp;
    }

    return new_head;
}
