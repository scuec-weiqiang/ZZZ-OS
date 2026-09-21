/**
 * @FilePath     : /ZZZ-OS/kernel/printk.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-18 22:48:55
 * @LastEditTime : 2026-09-19 18:09:44
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/types.h>
#include <os/printk.h>
#include <os/spinlock.h>
#include <os/wait.h>
#include <os/minmax.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/timekeeping.h>
#include <os/vsprintf.h>
#include <os/console.h>
#include <os/cpu.h>
#include <os/printk_buffer.h>
#include <stdarg.h>

/* 
主要搬运自linux 4.1.15，属于内核打印子系统的实现，主要是对日志缓冲区的管理和控制台输出的控制。
虽然是老版本的内核代码，但整体逻辑清晰，很适合参考，对于我这种简单os已经足够。
*/

int console_printk[4] = {
	CONSOLE_LOGLEVEL_DEFAULT,	/* console_loglevel */
	MESSAGE_LOGLEVEL_DEFAULT,	/* default_message_loglevel */
	CONSOLE_LOGLEVEL_MIN,		/* minimum_console_loglevel */
	CONSOLE_LOGLEVEL_DEFAULT,	/* default_console_loglevel */
};

enum log_flags {
	LOG_NOCONS	= 1,	/* already flushed, do not print to console */
	LOG_NEWLINE	= 2,	/* text ended with a newline */
	LOG_PREFIX	= 4,	/* text started with a prefix */
	LOG_CONT	= 8,	/* text is a fragment of a continuation line */
};

struct printk_log {
	u64 ts_nsec;		/* timestamp in nanoseconds */
	u16 len;		/* length of entire record */
	u16 text_len;		/* length of text buffer */
	u16 dict_len;		/* length of dictionary buffer */
	u8 facility;		/* syslog facility */
	u8 flags:5;		/* internal record flags */
	u8 level:3;		/* syslog level */
};

static SPINLOCK_DEFINE(logbuf_lock);

static  WAIT_QUEUE_HEAD(log_wait);
/* the next printk record to read by syslog(READ) or /proc/kmsg */
static u64 syslog_seq;
static u32 syslog_idx;
static enum log_flags syslog_prev;
static size_t syslog_partial;

/* 这些xxx_seq和xxx_idx本质上是维护了一段区间，表示日志缓冲区中有效数据的范围 */
/* index and sequence number of the first record stored in the buffer */
static u64 log_first_seq; // 缓冲区中没被覆盖的最早的日志记录的序列号
static u32 log_first_idx; // 缓冲区中没被覆盖的最早的日志记录的字节偏移量

/* index and sequence number of the next record to store in the buffer */
static u64 log_next_seq; // 缓冲区中下一个要写入的日志记录的序列号
static u32 log_next_idx; // 缓冲区中下一个要写入的日志记录的字节偏移量

/* the next printk record to write to the console */
static u64 console_seq; // 缓冲区中下一个要写入控制台的日志记录的序列号
static u32 console_idx; // 缓冲区中下一个要写入控制台的日志记录的字节偏移量
static enum log_flags console_prev;

/* the next printk record to read after the last 'clear' command */
static u64 clear_seq;// 缓冲区中下一个要读取的日志记录的序列号
static u32 clear_idx;// 缓冲区中下一个要读取的日志记录的字节偏移量

#define PREFIX_MAX		32
#define LOG_LINE_MAX		(1024 - PREFIX_MAX)

// 下面是静态缓冲区的定义
#define LOG_ALIGN 4
#define CONFIG_LOG_BUF_SHIFT 18
#define __LOG_BUF_LEN (1 << CONFIG_LOG_BUF_SHIFT)
static char __log_buf[__LOG_BUF_LEN] __aligned(LOG_ALIGN);
static char *log_buf = __log_buf;
static u32 log_buf_len = __LOG_BUF_LEN;


/* Return log buffer address */
char *log_buf_addr_get(void)
{
	return log_buf;
}

/* Return log buffer size */
u32 log_buf_len_get(void)
{
	return log_buf_len;
}

/* 获得日志的文本开始地址 */
/* human readable text of the record */
static char *log_text(const struct printk_log *msg)
{
	return (char *)msg + sizeof(struct printk_log);
}

/* optional key/value pair dictionary attached to the record */
static char *log_dict(const struct printk_log *msg)
{
	return (char *)msg + sizeof(struct printk_log) + msg->text_len;
}

/* 由缓冲区里的字节偏移量获取日志结构体，idx必须指向有效的消息 */
/* get record by index; idx must point to valid msg */
static struct printk_log *log_from_idx(u32 idx)
{
	struct printk_log *msg = (struct printk_log *)(log_buf + idx);

	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer.
	 */
	if (!msg->len)
		return (struct printk_log *)log_buf;
	return msg;
}

/* 由缓冲区里的字节偏移量获取下一个日志结构体，idx必须指向有效的消息 */
/* get next record; idx must point to valid msg */
static u32 log_next(u32 idx)
{
	struct printk_log *msg = (struct printk_log *)(log_buf + idx);

	/* length == 0 indicates the end of the buffer; wrap */
	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer as *this* one, and
	 * return the one after that.
	 */
	if (!msg->len) {
		msg = (struct printk_log *)log_buf;
		return msg->len;
	}
	return idx + msg->len;
}

/*
 * Check whether there is enough free space for the given message.
 *
 * The same values of first_idx and next_idx mean that the buffer
 * is either empty or full.
 *
 * If the buffer is empty, we must respect the position of the indexes.
 * They cannot be reset to the beginning of the buffer.
 */
static int logbuf_has_space(u32 msg_size, bool empty)
{
	u32 free;

	if (log_next_idx > log_first_idx || empty)
		free = max(log_buf_len - log_next_idx, log_first_idx);
	else
		free = log_first_idx - log_next_idx;

	/*
	 * We need space also for an empty header that signalizes wrapping
	 * of the buffer.
	 */
	return free >= msg_size + sizeof(struct printk_log);
}

static int log_make_free_space(u32 msg_size)
{
    // 缩减之前缓存的日志，直到有足够的空间存储新的日志消息
	while (log_first_seq < log_next_seq) {
		if (logbuf_has_space(msg_size, false))
			return 0;
		/* drop old messages until we have enough contiguous space */
		log_first_idx = log_next(log_first_idx);
		log_first_seq++;
	}

	/* sequence numbers are equal, so the log buffer is empty */
	if (logbuf_has_space(msg_size, true))
		return 0;

	return -ENOMEM;
}

/* compute the message size including the padding bytes */
static u32 msg_used_size(u16 text_len, u16 dict_len, u32 *pad_len)
{
	u32 size;

	size = sizeof(struct printk_log) + text_len + dict_len;
	*pad_len = (-size) & (LOG_ALIGN - 1);
	size += *pad_len;

	return size;
}

/*
 * Define how much of the log buffer we could take at maximum. The value
 * must be greater than two. Note that only half of the buffer is available
 * when the index points to the middle.
 */
#define MAX_LOG_TAKE_PART 4
static const char trunc_msg[] = "<truncated>";

/* 
* 定义了一个日志最大可占用的缓冲区比例 ，超出了这个比例的日志会被截断 
* 这里是4，表示一个日志最大可占用缓冲区的1/4，返回值是截断后的日志长度，截断后的日志会在末尾加上<truncated>提示
*/
static u32 truncate_msg(u16 *text_len, u16 *trunc_msg_len,
			u16 *dict_len, u32 *pad_len)
{
	/*
	 * The message should not take the whole buffer. Otherwise, it might
	 * get removed too soon.
	 */
	u32 max_text_len = log_buf_len / MAX_LOG_TAKE_PART;
	if (*text_len > max_text_len)
		*text_len = max_text_len;
	/* enable the warning message */
	*trunc_msg_len = strlen(trunc_msg);
	/* disable the "dict" completely */
	*dict_len = 0;
	/* compute the size again, count also the warning message */
	return msg_used_size(*text_len + *trunc_msg_len, 0, pad_len);
}

// 接收日志消息的各个参数，存储到日志缓冲区中，并返回实际存储的文本长度
/* insert record into the buffer, discard old ones, update heads */
static int log_store(int facility, int level,
		     enum log_flags flags, u64 ts_nsec,
		     const char *dict, u16 dict_len,
		     const char *text, u16 text_len)
{
	struct printk_log *msg;
	u32 size, pad_len;
	u16 trunc_msg_len = 0;

	/* number of '\0' padding bytes to next message */
	size = msg_used_size(text_len, dict_len, &pad_len);

	if (log_make_free_space(size)) {
		// 没有足够空间容纳新日志，日志会被截断
		/* truncate the message if it is too long for empty buffer */
		size = truncate_msg(&text_len, &trunc_msg_len,
				    &dict_len, &pad_len);
		/* survive when the log buffer is too small for trunc_msg */
		if (log_make_free_space(size))
			return 0;
	}

	// 判断是否触发回环，如果触发回环，则在缓冲区末尾写入一个空的日志头，表示回环，并将下一个写入位置重置为缓冲区开头
	// 这里为什么没有判断缓冲区剩余空间是否能容纳struct printk_log的大小呢？
	// log_make_free_space已经保证了有足够的空间容纳新的日志消息，所以这里不需要再判断
	if (log_next_idx + size + sizeof(struct printk_log) > log_buf_len) {
		/*
		 * This message + an additional empty header does not fit
		 * at the end of the buffer. Add an empty header with len == 0
		 * to signify a wrap around.
		 */
		memset(log_buf + log_next_idx, 0, sizeof(struct printk_log));
		log_next_idx = 0;
	}

	/* fill message */
	msg = (struct printk_log *)(log_buf + log_next_idx);
	memcpy(log_text(msg), text, text_len);
	msg->text_len = text_len;
	if (trunc_msg_len) {
		// 被截断的日志会在末尾加上<truncated>提示
		memcpy(log_text(msg) + text_len, trunc_msg, trunc_msg_len);
		msg->text_len += trunc_msg_len;
	}

	// 我这里没有用日志字典
	if (dict_len)
    	memcpy(log_dict(msg), dict, dict_len);

	msg->dict_len = dict_len;
	msg->facility = facility;
	msg->level = level & 7;
	msg->flags = flags & 0x1f;

	/* A zero timestamp means the clocksource was not ready yet. */
	msg->ts_nsec = ts_nsec;
	memset(log_dict(msg) + dict_len, 0, pad_len);
	msg->len = size;

	/* insert message */
	log_next_idx += msg->len;
	log_next_seq++;

	return msg->text_len;
}


// 下面两个函数是日志缓冲区与console的交互接口

// 将读游标初始化到当前最早的有效记录
void printk_cursor_init(struct printk_cursor *cursor) 
{
	unsigned long flags;
	spin_lock_irqsave(&logbuf_lock, &flags);
	cursor->seq = log_first_seq;
	cursor->idx = log_first_idx;
	spin_unlock_irqrestore(&logbuf_lock, flags);
}

// 读取并复制一条记录，同时推进 cursor。返回 1：读到记录；0：当前没有新记录。
// 这里会短暂持有logbuf_lock锁，保证读取的记录是有效的
int printk_read_record(struct printk_cursor *cursor,struct printk_record *record) 
{
	unsigned long flags;
	spin_lock_irqsave(&logbuf_lock, &flags);
	if (cursor->seq >= log_next_seq) {
		spin_unlock_irqrestore(&logbuf_lock, flags);
		return 0;
	} 

	// console 太慢，它还没读的记录已经被覆盖
	if (cursor->seq < log_first_seq) {
		cursor->seq = log_first_seq;
		cursor->idx = log_first_idx;
	}

	// 已经读完当前所有记录
	if (cursor->seq == log_next_seq) {
		spin_unlock_irqrestore(&logbuf_lock, flags);
		return 0;
	}
    	

	struct printk_log *msg = log_from_idx(cursor->idx);

	record->ts_nsec = msg->ts_nsec;
	record->text_len = msg->text_len;
	record->level = msg->level;
	record->newline = (msg->flags & LOG_NEWLINE) != 0;

	memcpy(record->text, log_text(msg), record->text_len);
	record->text[record->text_len] = '\0';

	cursor->idx = log_next(cursor->idx);
	cursor->seq++;

	spin_unlock_irqrestore(&logbuf_lock, flags);

	return 1;
}

// vsnprintf在linux里实现比较复杂，直接搬过来比较重，这里简化一下
#define PRINTK_TEXT_BUF_SIZE 1024

static char printk_text_buf[MAX_CPUS][PRINTK_TEXT_BUF_SIZE] __aligned(8);
// static char console_text_buf[PRINTK_TEXT_BUF_SIZE + 1] __aligned(8);

static int printk_cpu_id(void)
{
    int cpu = get_cpuid();

    if (cpu < 0 || cpu >= MAX_CPUS)
        return 0;

    return cpu;
}

// 解析日志前缀，判断日志的级别和是否有前缀 
static int printk_parse_prefix(const char **text, int *text_len, int *level)
{
    const char *str = *text;

    if (*text_len < 2 || str[0] != KERN_SOH_ASCII)
        return 0;

    if (str[1] >= '0' && str[1] <= '7') {
        if (*level == LOGLEVEL_DEFAULT)
            *level = str[1] - '0';

        *text += 2;
        *text_len -= 2;
        return 1;
    }

    if (str[1] == 'd') {
        *text += 2;
        *text_len -= 2;
        return 1;
    }

    return 0;
}


static u64 printk_timestamp(void)
{
    struct clocksource *clocksource = sys_clocksource();

    if (!clocksource ||
        !clocksource->read_counter ||
        !clocksource->freq_hz)
        return 0;

    return monotonic_ns();
}

int vprintk_emit(int facility, int level,
                 const char *dict, size_t dict_len,
                 const char *fmt, va_list args)
{
    enum log_flags log_flags = 0;
    unsigned long irq_flags;
    const char *text;
    char *buffer;
    u64 timestamp;
    int text_len;
    int formatted_len;
    int cpu;

    // 当前每个 CPU 只有一个格式化缓冲区，因此格式化期间先关闭本地中断，避免同一 CPU 的中断嵌套 printk 覆盖 buffer。
    irq_flags = arch_local_irq_save();

    cpu = printk_cpu_id();
    buffer = printk_text_buf[cpu];

    text_len = vscnprintf(buffer, sizeof(printk_text_buf[cpu]),
                          fmt, args);
    formatted_len = text_len;
    text = buffer;

    // 正文不保存结尾换行，换行作为记录属性保存。
	// 如果正文以换行结尾，则将正文长度减一，并设置日志标志 LOG_NEWLINE，说明日志正文以换行结尾。
    if (text_len > 0 && text[text_len - 1] == '\n') {
        text_len--;
        log_flags |= LOG_NEWLINE;
    }

    if (printk_parse_prefix(&text, &text_len, &level))
        log_flags |= LOG_PREFIX;

    if (level == LOGLEVEL_DEFAULT)
        level = default_message_loglevel;

    if (dict_len > UINT16_MAX)
        dict_len = UINT16_MAX;

    timestamp = printk_timestamp();

    // 只有提交结构化记录时才持有全局日志锁。
    spin_lock(&logbuf_lock);

    log_store(facility, level, log_flags, timestamp,
              dict, (u16)dict_len,
              text, (u16)text_len);

    spin_unlock(&logbuf_lock);
    arch_local_irq_restore(irq_flags);

	console_kick();

    return formatted_len;
}

int printk(const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);

    length = vprintk_emit(0, LOGLEVEL_DEFAULT,
                          NULL, 0, fmt, args);

    va_end(args);

    return length;
}

int printk_emergency(const char *fmt, ...)
{
    va_list args;
    int length;

    va_start(args, fmt);
    length = vprintk_emit(0, LOGLEVEL_EMERG, NULL, 0, fmt, args);
    va_end(args);

    return length;
}

void panic(const char *fmt, ...)
{
    static char panic_text[PRINTK_TEXT_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vscnprintf(panic_text, sizeof(panic_text), fmt, args);
    va_end(args);

	printk(KERN_EMERG "panic: %s", panic_text);
    while (1) {
    }
}
