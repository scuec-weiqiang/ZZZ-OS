#ifndef __OS_TTY_BUFFER_H
#define __OS_TTY_BUFFER_H

#include <os/workqueue.h>
#include <os/llist.h>
#include <os/mutex.h>

// 普通字符区: data[0 ... size-1]
// flag 区:    data[size ... 2*size-1]

struct tty_buffer {
    union {
        struct tty_buffer *next;
        struct llist_node free;
    };

    // 0 <= read <= commit <= used <= size
    unsigned int used;    // producer 写入位置
    unsigned int size;    // buffer 容量
    unsigned int commit;  // consumer 可见位置
    unsigned int read;    // consumer 已读位置
    unsigned int lookahead; // consumer lookahead 位置

    bool flags;           // 是否存在 flag buffer
    u8 data[] __aligned(sizeof(unsigned long));
};

struct tty_bufhead {
    struct mutex lock;        // 保护 consumer / flush_to_ldisc
    struct work_struct work;

    struct tty_buffer *head;  // consumer 当前读的 buffer
    struct tty_buffer *tail;  // producer 当前写的 buffer

    struct llist_head free;    // 小 buffer 缓存
    atomic_t priority;        // 优先级计数器，>0时禁止 flush_to_ldisc
    atomic_t mem_used;
    unsigned int mem_limit;

    struct tty_buffer sentinel;
};


static inline u8 *char_buf_ptr(struct tty_buffer *b, unsigned int ofs)
{
	return (u8 *)b->data + ofs;
}

static inline u8 *flag_buf_ptr(struct tty_buffer *b, unsigned int ofs)
{
	return char_buf_ptr(b, ofs) + b->size;
}

/*
 * When a break, frame error, or parity error happens, these codes are
 * stuffed into the flags buffer.
 */
#define TTY_NORMAL	0
#define TTY_BREAK	1
#define TTY_FRAME	2
#define TTY_PARITY	3
#define TTY_OVERRUN	4

#endif /* __OS_TTY_BUFFER_H */