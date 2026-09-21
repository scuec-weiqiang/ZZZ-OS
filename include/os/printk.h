/**
 * @FilePath     : /ZZZ-OS/include/os/printk.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-18 22:53:55
 * @LastEditTime : 2026-09-19 01:30:51
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef _OS_PRINTK_H
#define _OS_PRINTK_H

#define GREEN(msg)          msg 
#define RED(msg)            msg 
#define BLUE(msg)           msg 
#define YELLOW(msg)         msg 
#define CYAN(msg)           msg 
#define WHITE(msg)          msg 
#define BOLD(msg)           msg 
#define ITALIC(msg)         msg 
#define UNDERLINE(msg)      msg 
#define BLINK(msg)          msg 
#define INVERSE(msg)        msg 
#define HIDDEN(msg)         msg 
#define STRIKE(msg)         msg

// 主要搬运自linux 4.1.15
#include <os/kern_levels.h>
#include <os/types.h>
#include <os/vsprintf.h>
#include <stdarg.h>

/* printk's without a loglevel use this.. */
#define MESSAGE_LOGLEVEL_DEFAULT 4

/* We show everything that is MORE important than this.. */
#define CONSOLE_LOGLEVEL_SILENT  0 /* Mum's the word */
#define CONSOLE_LOGLEVEL_MIN	 1 /* Minimum loglevel we let people use */
#define CONSOLE_LOGLEVEL_QUIET	 4 /* Shhh ..., when booted with "quiet" */
#define CONSOLE_LOGLEVEL_DEFAULT 7 /* anything MORE serious than KERN_DEBUG */
#define CONSOLE_LOGLEVEL_DEBUG	10 /* issue debug messages */
#define CONSOLE_LOGLEVEL_MOTORMOUTH 15	/* You can't shut this one up */

extern int console_printk[];

#define console_loglevel (console_printk[0])
#define default_message_loglevel (console_printk[1])
#define minimum_console_loglevel (console_printk[2])
#define default_console_loglevel (console_printk[3])


static inline void console_silent(void)
{
	console_loglevel = CONSOLE_LOGLEVEL_SILENT;
}

static inline void console_verbose(void)
{
	if (console_loglevel)
		console_loglevel = CONSOLE_LOGLEVEL_MOTORMOUTH;
}

extern int printk(const char *fmt, ...);
extern int printk_emergency(const char *fmt, ...);
extern void panic(const char *fmt, ...);
extern int vprintk_emit(int facility, int level,
                 const char *dict, size_t dict_len,
                 const char *fmt, va_list args);

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define pr_emerg(fmt, ...) \
	printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...) \
	printk(KERN_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_crit(fmt, ...) \
	printk(KERN_CRIT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...) \
	printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warning(fmt, ...) \
	printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn pr_warning
#define pr_notice(fmt, ...) \
	printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...) \
	printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
	printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__)

#ifdef PRINTK_DEBUG
#define dprintk(fmt, ...) \
	printk(KERN_DEBUG "[DBG] %s:%d:%s: " fmt, \
	       __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define dprintk(fmt, ...) do { } while (0)
#endif

#define info(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#define here dprintk("here: %s:%d\n", __FILE__, __LINE__)


#endif
