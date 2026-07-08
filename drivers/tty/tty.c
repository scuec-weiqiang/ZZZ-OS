#include "os/syscall.h"
#include "uapi/asm-generic/ioctls.h"
#include <asm/ptrace.h>
#include <fs/cdev.h>
#include <fs/file.h>
#include <fs/types.h>
#include <os/devnode.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/sched.h>
#include <os/signal.h>
#include <os/string.h>
#include <os/tty.h>
#include <os/uaccess.h>

ssize_t tty_read(struct tty_struct *tty, char *buf, size_t size);
ssize_t tty_write(struct tty_struct *tty, const char *buf, size_t size);
long tty_ioctl(struct tty_struct *tty, unsigned long request, unsigned long arg);

static void tty_apply_termios(struct tty_struct *tty) {
    tty->echo = (tty->termios.c_lflag & ECHO) != 0;
    tty->canonical = (tty->termios.c_lflag & ICANON) != 0;
}

static void tty_termios_to_user(struct termios *dst, const struct ktermios *src) {
    unsigned int i;

    dst->c_iflag = src->c_iflag;
    dst->c_oflag = src->c_oflag;
    dst->c_cflag = src->c_cflag;
    dst->c_lflag = src->c_lflag;
    dst->c_line = src->c_line;
    for (i = 0; i < NCCS; i++) {
        dst->c_cc[i] = src->c_cc[i];
    }
}

static void tty_termios_from_user(struct ktermios *dst, const struct termios *src) {
    unsigned int i;
    speed_t ispeed = dst->c_ispeed;
    speed_t ospeed = dst->c_ospeed;

    dst->c_iflag = src->c_iflag;
    dst->c_oflag = src->c_oflag;
    dst->c_cflag = src->c_cflag;
    dst->c_lflag = src->c_lflag;
    dst->c_line = src->c_line;
    for (i = 0; i < NCCS; i++) {
        dst->c_cc[i] = src->c_cc[i];
    }
    dst->c_ispeed = ispeed;
    dst->c_ospeed = ospeed;
}

static void tty_termios2_to_user(struct termios2 *dst, const struct ktermios *src) {
    unsigned int i;

    dst->c_iflag = src->c_iflag;
    dst->c_oflag = src->c_oflag;
    dst->c_cflag = src->c_cflag;
    dst->c_lflag = src->c_lflag;
    dst->c_line = src->c_line;
    for (i = 0; i < NCCS; i++) {
        dst->c_cc[i] = src->c_cc[i];
    }
    dst->c_ispeed = src->c_ispeed;
    dst->c_ospeed = src->c_ospeed;
}

static void tty_termios2_from_user(struct ktermios *dst, const struct termios2 *src) {
    unsigned int i;

    dst->c_iflag = src->c_iflag;
    dst->c_oflag = src->c_oflag;
    dst->c_cflag = src->c_cflag;
    dst->c_lflag = src->c_lflag;
    dst->c_line = src->c_line;
    for (i = 0; i < NCCS; i++) {
        dst->c_cc[i] = src->c_cc[i];
    }
    dst->c_ispeed = src->c_ispeed;
    dst->c_ospeed = src->c_ospeed;
}

static void tty_make_name(char *buf, size_t size, const char *prefix,
                          unsigned int index)
{
    char digits[16];
    unsigned int pos = 0;
    size_t len = 0;

    if (buf == NULL || size == 0) {
        return;
    }

    buf[0] = '\0';
    if (prefix == NULL) {
        return;
    }

    while (prefix[len] != '\0' && len + 1 < size) {
        buf[len] = prefix[len];
        len++;
    }

    do {
        digits[pos++] = (char)('0' + (index % 10));
        index /= 10;
    } while (index != 0 && pos < sizeof(digits));

    while (pos > 0 && len + 1 < size) {
        buf[len++] = digits[--pos];
    }
    buf[len] = '\0';
}

static int tty_fops_open(struct inode *inode, struct file *file)
{
    struct tty_struct *tty = file ? file->private_data : NULL;

    (void)inode;
    if (tty == NULL) {
        return -ENODEV;
    }

    tty->count++;
    if (tty->driver != NULL && tty->driver->ops != NULL &&
        tty->driver->ops->open != NULL) {
        return tty->driver->ops->open(tty, file);
    }

    return 0;
}

static int tty_fops_release(struct inode *inode, struct file *file)
{
    struct tty_struct *tty = file ? file->private_data : NULL;

    (void)inode;
    if (tty == NULL) {
        return 0;
    }

    if (tty->driver != NULL && tty->driver->ops != NULL &&
        tty->driver->ops->close != NULL) {
        tty->driver->ops->close(tty, file);
    }

    if (tty->count > 0) {
        tty->count--;
    }
    file->private_data = NULL;
    return 0;
}

static ssize_t tty_fops_read(struct file *file, char *buf, size_t size,
                             loff_t *offset)
{
    struct tty_struct *tty = file ? file->private_data : NULL;
    ssize_t ret;

    if (tty == NULL || buf == NULL || offset == NULL) {
        return -EINVAL;
    }

    ret = tty_read(tty, buf, size);
    if (ret > 0) {
        *offset += ret;
    }
    return ret;
}

static ssize_t tty_fops_write(struct file *file, const char *buf, size_t size,
                              loff_t *offset)
{
    struct tty_struct *tty = file ? file->private_data : NULL;
    ssize_t ret;

    if (tty == NULL || buf == NULL || offset == NULL) {
        return -EINVAL;
    }

    ret = tty_write(tty, buf, size);
    if (ret > 0) {
        *offset += ret;
    }
    return ret;
}

static long tty_fops_ioctl(struct file *file, unsigned long request,
                           unsigned long arg)
{
    struct tty_struct *tty = file ? file->private_data : NULL;

    if (tty == NULL) {
        return -ENOTTY;
    }

    return tty_ioctl(tty, request, arg);
}

static const struct file_operations tty_fops = {
    .open = tty_fops_open,
    .release = tty_fops_release,
    .read = tty_fops_read,
    .write = tty_fops_write,
    .ioctl = tty_fops_ioctl,
};

static int tty_current_fops_open(struct inode *inode, struct file *file)
{
    struct tty_struct *tty;

    (void)inode;
    if (file == NULL || current == NULL || current->signal == NULL ||
        current->signal->tty == NULL) {
        return -ENXIO;
    }

    tty = current->signal->tty;
    file->private_data = tty;
    return tty_fops_open(inode, file);
}

static const struct file_operations tty_current_fops = {
    .open = tty_current_fops_open,
    .release = tty_fops_release,
    .read = tty_fops_read,
    .write = tty_fops_write,
    .ioctl = tty_fops_ioctl,
};

static int tty_register_current_device(void)
{
    static int registered;
    dev_t dev;
    int ret;

    if (registered) {
        return 0;
    }

    ret = alloc_chrdev_region(&dev, 1);
    if (ret) {
        return ret;
    }

    ret = cdev_register("tty", dev, &tty_current_fops, NULL);
    if (ret) {
        return ret;
    }

    registered = 1;
    return 0;
}

int tty_register_driver(struct tty_driver *driver)
{
    if (driver == NULL || driver->name == NULL || driver->num <= 0 ||
        driver->ttys == NULL || driver->ports == NULL) {
        return -EINVAL;
    }

    return 0;
}

void tty_unregister_driver(struct tty_driver *driver) {
    (void)driver;
}

int tty_register_device(struct tty_driver *driver, int index) {
    char name[32];
    struct tty_struct *tty;
    dev_t dev;
    int ret;

    if (driver == NULL || driver->ttys == NULL || index < 0 ||
        index >= driver->num) {
        return -EINVAL;
    }

    tty = driver->ttys[index];
    if (tty == NULL) {
        return -ENODEV;
    }

    if (tty->dev != 0) {
        return 0;
    }

    ret = alloc_chrdev_region(&dev, 1);
    if (ret) {
        return ret;
    }

    tty->dev = dev;
    if (driver->major == 0) {
        driver->major = MAJOR(dev);
    }
    if (index == 0) {
        driver->minor_start = MINOR(dev);
    }

    tty_make_name(name, sizeof(name), driver->name, (unsigned int)index);
    ret = cdev_register(name, dev, &tty_fops, tty);
    if (ret) {
        tty->dev = 0;
        return ret;
    }

    if (index == 0) {
        devnode_register("console", DEV_CHAR, dev, &tty_fops, tty);
        ret = tty_register_current_device();
        if (ret) {
            return ret;
        }
    }

    return 0;
}

static int tty_inbuf_full(struct tty_struct *tty) {
    return tty->inbuf_count == TTY_BUF_SIZE;
}

static int tty_inbuf_empty(struct tty_struct *tty) {
    return tty->inbuf_count == 0;
}

static void tty_inbuf_push(struct tty_struct *tty, char ch) {
    if (tty_inbuf_full(tty)) {
        return;
    }

    tty->inbuf[tty->head] = ch;
    tty->head = (tty->head + 1) % TTY_BUF_SIZE;
    tty->inbuf_count++;
}

static int tty_inbuf_pop(struct tty_struct *tty, char *ch) {
    if (tty_inbuf_empty(tty)) {
        return 0;
    }

    *ch = tty->inbuf[tty->tail];
    tty->tail = (tty->tail + 1) % TTY_BUF_SIZE;
    tty->inbuf_count--;
    return 1;
}

static void tty_putc(struct tty_struct *tty, char ch) {
    if (tty->driver != NULL && tty->driver->ops != NULL &&
        tty->driver->ops->write != NULL) {
        tty->driver->ops->write(tty, &ch, 1);
    }
}

static void tty_echo_char(struct tty_struct *tty, char ch) {
    if (!tty->echo) {
        return;
    }

    if (ch == '\n') {
        tty_putc(tty, '\r');
        tty_putc(tty, '\n');
    } else {
        tty_putc(tty, ch);
    }
}

static void tty_flush_line(struct tty_struct *tty) {
    unsigned int i;

    for (i = 0; i < tty->line_len; i++) {
        tty_inbuf_push(tty, tty->linebuf[i]);
    }
    tty->line_len = 0;
}

static void tty_flush_input(struct tty_struct *tty) {
    tty->head = 0;
    tty->tail = 0;
    tty->inbuf_count = 0;
    tty->line_len = 0;
}

static int tty_sleep_if_empty(struct tty_struct *tty) {
    struct task_struct *task = current;
    struct tty_port *port;
    struct rq *rq;
    unsigned long rq_flags;
    unsigned long tty_flags;
    unsigned long wq_flags;

    if (task == NULL || task->status != TASK_RUNNING || tty == NULL ||
        tty->port == NULL) {
        return 0;
    }

    port = tty->port;
    wq_flags = spin_lock_irqsave(&port->read_wait.lock);

    tty_flags = spin_lock_irqsave(&tty->lock);
    if (!tty_inbuf_empty(tty)) {
        spin_unlock_irqrestore(&tty->lock, tty_flags);
        spin_unlock_irqrestore(&port->read_wait.lock, wq_flags);
        return 0;
    }
    spin_unlock_irqrestore(&tty->lock, tty_flags);

    task->status = TASK_SLEEPING;
    rq = this_rq();
    rq_flags = spin_lock_irqsave(&rq->lock);
    task->sched_class->dequeue_task(rq, task);
    spin_unlock_irqrestore(&rq->lock, rq_flags);

    task->wait.private = task;
    wait_queue_add(&port->read_wait, &task->wait);

    spin_unlock_irqrestore(&port->read_wait.lock, wq_flags);

    sched();
    return 1;
}

void tty_port_init(struct tty_port *port) {
    if (port == NULL) {
        return;
    }

    memset(port, 0, sizeof(*port));
    spin_lock_init(&port->lock);
    init_waitqueue_head(&port->open_wait);
    init_waitqueue_head(&port->close_wait);
    init_waitqueue_head(&port->read_wait);
    init_waitqueue_head(&port->write_wait);
    port->read_wait.wait_reason = get_wait_reason_name(WAIT_TTY_READ);
}

void tty_init(struct tty_struct *tty, struct tty_driver *driver,
              struct tty_port *port, int index, void *driver_data) {
    if (tty == NULL) {
        return;
    }

    memset(tty, 0, sizeof(*tty));
    spin_lock_init(&tty->lock);
    tty->index = index;
    tty->driver = driver;
    tty->port = port;
    tty->driver_data = driver_data;
    if (port != NULL) {
        port->tty = tty;
        port->driver_data = driver_data;
    }
    tty->termios.c_iflag = ICRNL | IXON;
    tty->termios.c_oflag = OPOST;
    tty->termios.c_cflag = CS8 | CREAD | HUPCL;
    tty->termios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL |
                           ECHOKE | IEXTEN;
    tty->termios.c_cc[VINTR] = 3;
    tty->termios.c_cc[VQUIT] = 28;
    tty->termios.c_cc[VERASE] = 127;
    tty->termios.c_cc[VKILL] = 21;
    tty->termios.c_cc[VEOF] = 4;
    tty->termios.c_cc[VTIME] = 0;
    tty->termios.c_cc[VMIN] = 1;
    tty->winsize.ws_row = 24;
    tty->winsize.ws_col = 80;
    tty->pgrp = 0;
    tty_apply_termios(tty);
}

void tty_receive_char(struct tty_struct *tty, char ch) {
    unsigned long flags;
    int wake = 0;
    int echo_backspace = 0;
    char echo_ch = 0;
    int sig = 0;
    pid_t pgrp = 0;

    if (tty == NULL) {
        return;
    }

    if ((tty->termios.c_iflag & ICRNL) && ch == '\r') {
        ch = '\n';
    }

    flags = spin_lock_irqsave(&tty->lock);

    if ((tty->termios.c_lflag & ISIG) && ch == tty->termios.c_cc[VINTR]) {
        sig = SIGINT;
        pgrp = tty->pgrp;
        if (!(tty->termios.c_lflag & NOFLSH)) {
            tty_flush_input(tty);
        }
        spin_unlock_irqrestore(&tty->lock, flags);

        if (tty->echo && (tty->termios.c_lflag & ECHOCTL)) {
            tty_putc(tty, '^');
            tty_putc(tty, 'C');
            tty_putc(tty, '\r');
            tty_putc(tty, '\n');
        }
        if (pgrp > 0) {
            send_signal_to_pgrp(pgrp, sig);
        }
        return;
    }

    if (tty->canonical && (ch == '\b' || ch == tty->termios.c_cc[VERASE])) {
        if (tty->line_len > 0) {
            tty->line_len--;
            echo_backspace = 1;
        }
        spin_unlock_irqrestore(&tty->lock, flags);

        if (echo_backspace && tty->echo) {
            tty_putc(tty, '\b');
            tty_putc(tty, ' ');
            tty_putc(tty, '\b');
        }
        return;
    }

    if (tty->canonical) {
        if (tty->line_len < TTY_LINE_SIZE) {
            tty->linebuf[tty->line_len++] = ch;
            echo_ch = ch;
        }

        if (ch == '\n' || tty->line_len == TTY_LINE_SIZE) {
            tty_flush_line(tty);
            wake = 1;
        }
    } else {
        tty_inbuf_push(tty, ch);
        echo_ch = ch;
        wake = 1;
    }

    spin_unlock_irqrestore(&tty->lock, flags);

    if (echo_ch) {
        tty_echo_char(tty, echo_ch);
    }

    if (wake) {
        if (tty->port != NULL) {
            wake_up_one(&tty->port->read_wait);
        }
    }
}

ssize_t tty_read(struct tty_struct *tty, char *buf, size_t size) {
    size_t read = 0;

    if (tty == NULL || buf == NULL) {
        return -EINVAL;
    }

    if (size == 0) {
        return 0;
    }

    while (read < size) {
        unsigned long flags;
        char ch;

        flags = spin_lock_irqsave(&tty->lock);
        if (!tty_inbuf_pop(tty, &ch)) {
            spin_unlock_irqrestore(&tty->lock, flags);
            if (read > 0) {
                break;
            }
            tty_sleep_if_empty(tty);
            continue;
        }
        spin_unlock_irqrestore(&tty->lock, flags);

        buf[read++] = ch;
        if (tty->canonical && ch == '\n') {
            break;
        }
    }

    return (ssize_t)read;
}

ssize_t tty_write(struct tty_struct *tty, const char *buf, size_t size) {
    size_t written = 0;

    if (tty == NULL || buf == NULL) {
        return -EINVAL;
    }

    while (written < size) {
        if (buf[written] == '\n') {
            tty_putc(tty, '\r');
        }
        tty_putc(tty, buf[written]);
        written++;
    }

    return (ssize_t)written;
}

long tty_ioctl(struct tty_struct *tty, unsigned long request, unsigned long arg) {
    unsigned long flags;
    void *argp = (void *)arg;
    struct termios termios;
    struct termios2 termios2;
    struct ktermios old_termios;
    struct winsize winsize;
    int pgrp;

    if (tty == NULL) {
        return -ENOTTY;
    }

    switch (request) {
    case TCGETS:
        if (argp == NULL) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        tty_termios_to_user(&termios, &tty->termios);
        spin_unlock_irqrestore(&tty->lock, flags);

        if (copy_to_user(argp, (char *)&termios, sizeof(termios)) < 0) {
            return -EFAULT;
        }
        return 0;

    case TCGETS2:
        if (argp == NULL) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        tty_termios2_to_user(&termios2, &tty->termios);
        spin_unlock_irqrestore(&tty->lock, flags);

        if (copy_to_user(argp, (char *)&termios2, sizeof(termios2)) < 0) {
            return -EFAULT;
        }
        return 0;

    case TCSETS:
    case TCSETSW:
    
    case TCSETSF:
        if (argp == NULL) {
            return -EFAULT;
        }

        if (copy_from_user((char *)&termios, argp, sizeof(termios)) < 0) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        old_termios = tty->termios;
        tty_termios_from_user(&tty->termios, &termios);
        tty_apply_termios(tty);
        spin_unlock_irqrestore(&tty->lock, flags);

        if (tty->driver != NULL && tty->driver->ops != NULL &&
            tty->driver->ops->set_termios != NULL) {
            tty->driver->ops->set_termios(tty, &old_termios);
        }
        return 0;

    case TCSETS2:
    case TCSETSW2:
    case TCSETSF2:
        if (argp == NULL) {
            return -EFAULT;
        }

        if (copy_from_user((char *)&termios2, argp, sizeof(termios2)) < 0) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        old_termios = tty->termios;
        tty_termios2_from_user(&tty->termios, &termios2);
        tty_apply_termios(tty);
        spin_unlock_irqrestore(&tty->lock, flags);

        if (tty->driver != NULL && tty->driver->ops != NULL &&
            tty->driver->ops->set_termios != NULL) {
            tty->driver->ops->set_termios(tty, &old_termios);
        }
        return 0;

    case TIOCGWINSZ:
        if (argp == NULL) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        winsize = tty->winsize;
        spin_unlock_irqrestore(&tty->lock, flags);

        if (copy_to_user(argp, (char *)&winsize, sizeof(winsize)) < 0) {
            return -EFAULT;
        }
        return 0;

    case TIOCSWINSZ:
        if (argp == NULL) {
            return -EFAULT;
        }

        if (copy_from_user((char *)&winsize, argp, sizeof(winsize)) < 0) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        tty->winsize = winsize;
        spin_unlock_irqrestore(&tty->lock, flags);
        return 0;

    case TIOCSCTTY:
        current->signal->tty = tty;
        tty->session = current->signal->session;
        tty->pgrp = current->signal->pgrp;
        return 0;

    case TIOCGPGRP:
        if (argp == NULL) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        pgrp = tty->pgrp ? tty->pgrp : current->pid;
        spin_unlock_irqrestore(&tty->lock, flags);

        if (copy_to_user(argp, (char *)&pgrp, sizeof(pgrp)) < 0) {
            return -EFAULT;
        }
        return 0;

    case TIOCSPGRP:
        if (argp == NULL) {
            return -EFAULT;
        }

        if (copy_from_user((char *)&pgrp, argp, sizeof(pgrp)) < 0) {
            return -EFAULT;
        }

        flags = spin_lock_irqsave(&tty->lock);
        tty->pgrp = pgrp;
        spin_unlock_irqrestore(&tty->lock, flags);
        return 0;

    default:
        return -ENOTTY;
    }
}
