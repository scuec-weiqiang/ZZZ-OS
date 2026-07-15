/**
 * @FilePath     : /ZZZ-OS/drivers/tty/n_tty.c
 * @Description  :
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-14 00:42:09
 * @LastEditTime : 2026-07-15 16:38:59
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */
#include <os/errno.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/n_tty.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/tty.h>
#include <os/tty_driver.h>
#include <os/tty_ldisc.h>
#include <uapi/linux/termios.h>
#include <uapi/linux/tty.h>

static void n_tty_kick_reader(struct tty_struct *tty) {
    if (tty && tty->port)
        wake_up_one(&tty->port->read_wait);
}

static ssize_t n_tty_driver_write(struct tty_struct *tty, const char *buf, size_t count) {
    if (!tty || !tty->driver || !tty->driver->ops || !tty->driver->ops->write)
        return -EIO;

    return tty->driver->ops->write(tty, (const u8 *)buf, count);
}

static void n_tty_echo_char(struct tty_struct *tty, unsigned char ch) {
    if (!(tty->termios.c_lflag & ECHO))
        return;

    if (ch == '\n' && (tty->termios.c_oflag & ONLCR)) {
        n_tty_driver_write(tty, "\r\n", 2);
        return;
    }

    n_tty_driver_write(tty, (const char *)&ch, 1);
}

static int n_tty_open(struct tty_struct *tty) {
    struct n_tty_data *ldata;

    if (!tty)
        return -EINVAL;

    ldata = kzalloc(sizeof(*ldata));
    if (!ldata)
        return -ENOMEM;

    if (ringbuffer_init(&ldata->read_buf, ldata->read_buf_data, sizeof(ldata->read_buf_data))) {
        kfree(ldata);
        return -ENOMEM;
    }

    spin_lock_init(&ldata->read_lock);
    tty->disc_data = ldata;
    tty->receive_room = N_TTY_BUF_SIZE - 1;
    return 0;
}

static void n_tty_close(struct tty_struct *tty) {
    if (!tty)
        return;

    kfree(tty->disc_data);
    tty->disc_data = NULL;
}

static void n_tty_flush_buffer(struct tty_struct *tty) {
    struct n_tty_data *ldata;
    unsigned long flags;

    if (!tty || !tty->disc_data)
        return;

    ldata = tty->disc_data;
    flags = spin_lock_irqsave(&ldata->read_lock);
    ringbuffer_reset(&ldata->read_buf);
    ldata->canon_head = 0;
    ldata->line_start = 0;
    ldata->line_count = 0;
    tty->receive_room = N_TTY_BUF_SIZE - 1;
    spin_unlock_irqrestore(&ldata->read_lock, flags);
}

static int n_tty_receive_char(struct tty_struct *tty, struct n_tty_data *ldata, unsigned char ch,
                              unsigned char *echo_ch, bool *echo_erase) {
    int wake = 0;

    *echo_ch = 0;
    *echo_erase = false;

    if ((tty->termios.c_iflag & ICRNL) && ch == '\r')
        ch = '\n';

    if ((tty->termios.c_lflag & ICANON) && (ch == '\b' || ch == tty->termios.c_cc[VERASE])) {
        /*
         * This is intentionally conservative: only erase bytes that have not
         * yet completed a canonical line.
         */
        if (ldata->read_buf.count != ldata->canon_head) {
            if (ldata->read_buf.head == 0)
                ldata->read_buf.head = ldata->read_buf.capacity - 1;
            else
                ldata->read_buf.head--;
            ldata->read_buf.count--;
            tty->receive_room++;
            *echo_erase = true;
        }
        return 0;
    }

    if ((tty->termios.c_lflag & ICANON) && ch == tty->termios.c_cc[VEOF]) {
        ldata->line_count++;
        wake = 1;
        return wake;
    }

    if (!ringbuffer_put(&ldata->read_buf, ch))
        return 0;

    if (tty->receive_room > 0)
        tty->receive_room--;

    if (tty->termios.c_lflag & ICANON) {
        if (ch == '\n' || ringbuffer_full(&ldata->read_buf)) {
            ldata->canon_head = ldata->read_buf.count;
            ldata->line_count++;
            wake = 1;
        }
    } else {
        wake = 1;
    }

    *echo_ch = ch;
    return wake;
}

static void n_tty_receive_buf(struct tty_struct *tty, const unsigned char *cp,
                              const unsigned char *fp, size_t count) {
    struct n_tty_data *ldata;
    unsigned long flags;
    size_t i;
    int wake = 0;

    if (!tty || !tty->disc_data || !cp)
        return;

    ldata = tty->disc_data;

    flags = spin_lock_irqsave(&ldata->read_lock);
    for (i = 0; i < count; i++) {
        unsigned char echo_ch;
        bool echo_erase;

        if (fp && fp[i] != TTY_NORMAL)
            continue;

        wake |= n_tty_receive_char(tty, ldata, cp[i], &echo_ch, &echo_erase);
        spin_unlock_irqrestore(&ldata->read_lock, flags);

        if (echo_erase && (tty->termios.c_lflag & ECHO))
            n_tty_driver_write(tty, "\b \b", 3);
        if (echo_ch)
            n_tty_echo_char(tty, echo_ch);

        flags = spin_lock_irqsave(&ldata->read_lock);
    }
    spin_unlock_irqrestore(&ldata->read_lock, flags);

    if (wake)
        n_tty_kick_reader(tty);
}

static size_t n_tty_receive_buf2(struct tty_struct *tty, const unsigned char *cp,
                                 const unsigned char *fp, size_t count) {
    n_tty_receive_buf(tty, cp, fp, count);
    return count;
}

static ssize_t n_tty_read(struct tty_struct *tty, struct file *file, char *buf, size_t nr) {
    struct n_tty_data *ldata;
    unsigned long flags;
    size_t copied = 0;
    bool canonical;

    (void)file;

    if (!tty || !buf)
        return -EINVAL;

    ldata = tty->disc_data;
    if (!ldata)
        return -EIO;

    canonical = (tty->termios.c_lflag & ICANON) != 0;

    while (nr > 0) {
        unsigned char ch;

        flags = spin_lock_irqsave(&ldata->read_lock);

        if (ringbuffer_empty(&ldata->read_buf)) {
            if (canonical && ldata->line_count > 0) {
                ldata->line_count--;
                spin_unlock_irqrestore(&ldata->read_lock, flags);
                break;
            }

            spin_unlock_irqrestore(&ldata->read_lock, flags);
            if (copied)
                break;
            if (tty->port)
                sleep_on(&tty->port->read_wait);
            continue;
        }

        if (canonical && ldata->line_count == 0) {
            spin_unlock_irqrestore(&ldata->read_lock, flags);
            if (copied)
                break;
            if (tty->port)
                sleep_on(&tty->port->read_wait);
            continue;
        }

        if (!ringbuffer_get(&ldata->read_buf, &ch)) {
            spin_unlock_irqrestore(&ldata->read_lock, flags);
            break;
        }

        tty->receive_room++;
        spin_unlock_irqrestore(&ldata->read_lock, flags);

        buf[copied++] = ch;
        nr--;

        if (canonical && ch == '\n') {
            flags = spin_lock_irqsave(&ldata->read_lock);
            if (ldata->line_count > 0)
                ldata->line_count--;
            spin_unlock_irqrestore(&ldata->read_lock, flags);
            break;
        }
    }

    return (ssize_t)copied;
}

static ssize_t n_tty_write(struct tty_struct *tty, struct file *file, const char *buf, size_t nr) {
    size_t copied = 0;

    (void)file;

    if (!tty || (!buf && nr))
        return -EINVAL;

    while (copied < nr) {
        ssize_t ret;

        if ((tty->termios.c_oflag & OPOST) && (tty->termios.c_oflag & ONLCR) &&
            buf[copied] == '\n') {
            ret = n_tty_driver_write(tty, "\r\n", 2);
            if (ret < 0)
                return copied ? (ssize_t)copied : ret;
            copied++;
            continue;
        }

        ret = n_tty_driver_write(tty, &buf[copied], 1);
        if (ret < 0)
            return copied ? (ssize_t)copied : ret;
        copied++;
    }

    return (ssize_t)copied;
}

static ssize_t n_tty_chars_in_buffer(struct tty_struct *tty) {
    struct n_tty_data *ldata;
    unsigned long flags;
    size_t count;

    if (!tty || !tty->disc_data)
        return 0;

    ldata = tty->disc_data;
    flags = spin_lock_irqsave(&ldata->read_lock);
    count = ringbuffer_count(&ldata->read_buf);
    spin_unlock_irqrestore(&ldata->read_lock, flags);
    return (ssize_t)count;
}

static struct tty_ldisc_ops n_tty_ops = {
    .num = N_TTY,
    .name = "n_tty",
    .read = n_tty_read,
    .write = n_tty_write,
    .receive_buf = n_tty_receive_buf,
    .receive_buf2 = n_tty_receive_buf2,
    .flush_buffer = n_tty_flush_buffer,
    .chars_in_buffer = n_tty_chars_in_buffer,
    .open = n_tty_open,
    .close = n_tty_close,
};

int n_tty_init(void) {
    atomic_set(&n_tty_ops.refcnt, 0);
    return tty_register_ldisc(&n_tty_ops);
}

core_initcall(n_tty_init);
