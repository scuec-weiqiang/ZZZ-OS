#include <asm/ptrace.h>
#include <fs/cdev.h>
#include <fs/file.h>
#include <fs/types.h>
#include <os/devnode.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/mutex.h>
#include <os/sched.h>
#include <os/signal.h>
#include <os/string.h>
#include <os/syscall.h>
#include <os/tty.h>
#include <os/uaccess.h>
#include <os/wait.h>
#include <os/printk.h>
#include <uapi/asm-generic/ioctls.h>

LIST_HEAD(tty_drivers); /* linked list of tty drivers */
DEFINE_MUTEX(tty_mutex);
static struct class tty_class = { .name = "tty" };
static bool tty_class_ready;

struct ktermios tty_std_termios = {
    /* for the benefit of tty drivers  */
    .c_iflag = ICRNL | IXON,
    .c_oflag = OPOST | ONLCR,
    .c_cflag = B38400 | CS8 | CREAD | HUPCL,
    .c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN,
    .c_cc = INIT_C_CC,
    .c_ispeed = 38400,
    .c_ospeed = 38400,
    .c_line = N_TTY,
};

static size_t tty_port_default_receive_buf(struct tty_port *port, const u8 *cp,
                                           const u8 *fp, size_t count) {
    struct tty_struct *tty;

    if (!port)
        return 0;

    tty = port->tty;
    if (!tty || !tty->ldisc)
        return 0;

    return tty_ldisc_receive_buf(tty->ldisc, cp, fp, count);
}

static const struct tty_port_client_operations tty_port_default_client_ops = {
    .receive_buf = tty_port_default_receive_buf,
};

static struct tty_struct *tty_alloc_struct(struct tty_driver *driver, unsigned int index) {
    struct tty_struct *tty;
    tty = kmalloc(sizeof(*tty));
    if (!tty) {
        return NULL;
    }

    tty_init(tty, driver, NULL, index, NULL);
    return tty;
}

static void tty_free_struct(struct tty_struct *tty) {
    kfree(tty);
}

static int tty_install(struct tty_driver *driver, struct tty_struct *tty) {
    if (!driver || !tty)
        return -EINVAL;

    if (driver->ops && driver->ops->install)
        return driver->ops->install(driver, tty);

    return 0;
}

static void tty_remove(struct tty_driver *driver, struct tty_struct *tty) {

}

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

void tty_port_init(struct tty_port *port) {
    memset(port, 0, sizeof(*port));
    tty_buffer_init(port);
    mutex_init(&port->mutex);
    mutex_init(&port->buf_mutex);
    init_waitqueue_head(&port->open_wait, WAIT_OPEN);
    init_waitqueue_head(&port->close_wait, WAIT_CLOSE);
    init_waitqueue_head(&port->read_wait, WAIT_READ);
    init_waitqueue_head(&port->write_wait, WAIT_WRITE);
    spin_lock_init(&port->lock);
    port->client_ops = &tty_port_default_client_ops;
}

void tty_port_destroy(struct tty_port *port) {
    if (port == NULL) {
        return;
    }
    tty_buffer_free_all(port);
}

void tty_init(struct tty_struct *tty, struct tty_driver *driver,
                struct tty_port *port, int index,void *driver_data) {
    if (tty == NULL) {
        return;
    }

    memset(tty, 0, sizeof(*tty));
    spin_lock_init(&tty->lock);
    mutex_init(&tty->read_lock);
    mutex_init(&tty->write_lock);
    tty->index = index;
    tty->driver = driver;
    tty->port = port;
    tty->driver_data = driver_data;
    if (port != NULL) {
        port->tty = tty;
    }
    tty->termios = driver->termios[index];
    tty_apply_termios(tty);
    return;
}

void tty_receive_char(struct tty_struct *tty, char ch) {
    unsigned char c = (unsigned char)ch;

    if (tty == NULL || tty->ldisc == NULL || tty->ldisc->ops == NULL) {
        return;
    }

    if (tty->ldisc->ops->receive_buf2) {
        tty->ldisc->ops->receive_buf2(tty, &c, NULL, 1);
        return;
    }

    if (tty->ldisc->ops->receive_buf)
        tty->ldisc->ops->receive_buf(tty, &c, NULL, 1);
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
        if (tty->driver->ops != NULL && tty->driver->ops->set_termios != NULL) {
            tty->driver->ops->set_termios(tty, &old_termios);
        }
        spin_unlock_irqrestore(&tty->lock, flags);
        return 0;
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

static int tty_fops_open(struct inode *inode, struct file *file) {
    struct tty_driver *driver = file->private_data;
    struct tty_struct *tty;
    unsigned int index;
    bool new_tty = false;
    int ret;

    if (!driver)
        return -ENODEV;

    index = MINOR(inode->i_rdev) - driver->minor_start;
    if (index >= driver->num)
        return -ENODEV;

    tty = driver->ttys[index];

    if (!tty) {
        tty = tty_alloc_struct(driver, index);
        if (!tty)
            return -ENOMEM;

        new_tty = true;

        /*
         * serial_install:
         * tty->port = &state[index].port;
         * tty->driver_data = &state[index];
         * port->tty = tty;
         */
        ret = driver->ops->install(driver, tty);
        if (ret)
            goto err_free_tty;

        /*
         * 此时 tty->port 已经有效，再安装 N_TTY。
         */
        tty_ldisc_init(tty);
    }

    /*
     * 从这里开始，file->private_data 不再是 tty_driver。
     */
    file->private_data = tty;

    /*
     * serial_open() 根据 port->open_count 判断是否第一次启动硬件。
     */
    ret = driver->ops->open(tty, file);
    if (ret)
        goto err_open;

    tty->count++;
    return 0;

err_open:
    file->private_data = driver;

    if (!new_tty)
        return ret;

    tty_ldisc_deinit(tty);

err_free_tty:
    driver->ttys[index] = NULL;
    tty_free_struct(tty);
    return ret;
}

static int tty_fops_release(struct inode *inode, struct file *file) {
    struct tty_struct *tty = file ? file->private_data : NULL;

    (void)inode;
    if (tty == NULL) {
        return 0;
    }

    if (tty->driver != NULL && tty->driver->ops != NULL && tty->driver->ops->close != NULL) {
        tty->driver->ops->close(tty, file);
    }

    if (tty->count > 0) {
        tty->count--;
    }

    // 最后一次使用
    if (tty->count == 0) {
        if (tty->driver->termios && tty->index < tty->driver->num) {
            tty->driver->termios[tty->index] = tty->termios;
        }

        if (tty->driver != NULL && tty->driver->ops != NULL &&
            tty->driver->ops->remove != NULL) {
            tty->driver->ops->remove(tty->driver, tty);
        }

        tty_ldisc_deinit(tty);

        tty->driver->ttys[tty->index] = NULL;

        tty_free_struct(tty);
    }
    file->private_data = NULL;
    return 0;
}

static ssize_t tty_fops_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
    struct tty_struct *tty = file->private_data;
    ssize_t ret;

    (void)ppos;

    if (!tty || !tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->read)
        return -EIO;

    mutex_lock(&tty->read_lock);

    ret = tty->ldisc->ops->read(tty, file, buf, count);

    mutex_unlock(&tty->read_lock);
    return ret;
}

static ssize_t tty_fops_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    struct tty_struct *tty = file->private_data;
    ssize_t ret;

    (void)ppos;

    if (!tty || !tty->ldisc || !tty->ldisc->ops || !tty->ldisc->ops->write)
        return -EIO;

    mutex_lock(&tty->write_lock);

    ret = tty->ldisc->ops->write(tty, file, buf, count);

    mutex_unlock(&tty->write_lock);
    return ret;
}

static long tty_fops_ioctl(struct file *file, unsigned long request, unsigned long arg) {
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

// 这只是给已分配的设备号建立cdev对应映射,但并没有对cdev完全初始化,比如建立/dev/下的文件要到后面
int tty_cdev_add(struct tty_driver *driver, dev_t dev, unsigned int index, unsigned int count) {
    driver->cdevs[index] = cdev_alloc();
    if (!driver->cdevs[index])
        return -ENOMEM;
    driver->cdevs[index]->fops = &tty_fops;
    driver->cdevs[index]->private = driver;
    int err = cdev_add(driver->cdevs[index], dev, count);
    return err;
}

int tty_register_device(struct tty_driver *driver, int index, struct device *parent) {
    dev_t dev;
    char name[32];

    if (!driver)
        return -EINVAL;

    if (index >= driver->num)
        return -EINVAL;

    dev = MKDEV(driver->major, driver->minor_start + index);

    snprintk(name, sizeof(name), "%s%u", driver->name, index);

    driver->devices[index] = device_create(&tty_class, parent, dev,
                                           S_IFCHR | 0600, driver, name);
    if (IS_ERR(driver->devices[index])) {
        int ret = PTR_ERR(driver->devices[index]);
        driver->devices[index] = NULL;
        return ret;
    }
    return 0;
}

int tty_unregister_device(struct tty_driver *driver, int index)
{
    dev_t dev;

    if (!driver || index < 0 || index >= driver->num)
        return -EINVAL;
    if (!driver->devices[index])
        return -ENODEV;

    dev = MKDEV(driver->major, driver->minor_start + index);
    device_destroy(&tty_class, dev);
    driver->devices[index] = NULL;
    return 0;
}

int tty_register_driver(struct tty_driver *driver) {
    int error;
    dev_t dev;

    unsigned int i;

    if (!tty_class_ready) {
        error = class_register(&tty_class);
        if (error && error != -EEXIST)
            return error;
        tty_class_ready = true;
    }

    for (i = 0; i < driver->num; i++)
        driver->termios[i] = driver->init_termios;

    if (!driver->major) {
        error = alloc_chrdev_region(&dev, driver->minor_start, driver->num, driver->name);
        if (!error) {
            driver->major = MAJOR(dev);
            driver->minor_start = MINOR(dev);
        }
    } else {
        dev = MKDEV(driver->major, driver->minor_start);
        error = register_chrdev_region(dev, driver->num, driver->name);
    }
    if (error < 0)
        goto err;

    error = tty_cdev_add(driver, dev, 0, driver->num);
    if (error)
        goto err_unreg_char;

    mutex_lock(&tty_mutex);
    list_add(&tty_drivers,&driver->tty_drivers);
    mutex_unlock(&tty_mutex);

    return 0;

err_unreg_char:
    unregister_chrdev_region(dev, driver->num);
err:
    return error;
}

// 暂时不实现
void tty_unregister_driver(struct tty_driver *driver) {
    (void)driver;
}

void tty_put_driver(struct tty_driver *driver) {
    if (!driver)
        return;

    kfree(driver->ports);
    kfree(driver->ttys);
    kfree(driver->termios);
    kfree(driver->cdevs);
    kfree(driver->devices);
    kfree(driver);
}

struct tty_driver *tty_alloc_driver(unsigned int lines) {
    struct tty_driver *driver;
    unsigned int cdevs = 1;
    int err;

    driver = kzalloc(sizeof(*driver));
    if (!driver)
        return ERR_PTR(-ENOMEM);

    driver->num = lines;

    driver->ttys = kzalloc(lines * sizeof(*driver->ttys));
    driver->termios = kzalloc(lines * sizeof(*driver->termios));
    if (!driver->ttys || !driver->termios) {
        err = -ENOMEM;
        goto err_free_all;
    }

    driver->ports = kzalloc(lines * sizeof(*driver->ports));
    if (!driver->ports) {
        err = -ENOMEM;
        goto err_free_all;
    }
    driver->cdevs = kzalloc(cdevs * sizeof(*driver->cdevs));
    if (!driver->cdevs) {
        err = -ENOMEM;
        goto err_free_all;
    }

    driver->devices = kzalloc(lines * sizeof(*driver->devices));
    if (!driver->devices) {
        err = -ENOMEM;
        goto err_free_all;
    }

    return driver;
err_free_all:
    tty_put_driver(driver);
    return ERR_PTR(err);
}
