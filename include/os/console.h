#ifndef __KERNEL_CONSOLE_H__
#define __KERNEL_CONSOLE_H__

extern void console_register(void (*fn)(char c));
extern void console_puts(const char *str);
extern void console_flush(void);

#endif // __KERNEL_CONSOLE_H__