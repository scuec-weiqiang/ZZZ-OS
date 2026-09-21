#ifndef __KERNEL_CONSOLE_H__
#define __KERNEL_CONSOLE_H__

#include <os/types.h>
#include <os/printk_buffer.h>

struct console {
    const char *name;
    void (*write)(struct console *con, const char *text, size_t length);
    void *private;
    struct printk_cursor cursor; // console下一个要输出的日志记录的游标
    struct console *next;
};

extern struct console *console_get(const char *name);

extern int console_register(struct console *con);
extern void console_kick(void);
extern void console_flush_sync(void);

#endif // __KERNEL_CONSOLE_H__
