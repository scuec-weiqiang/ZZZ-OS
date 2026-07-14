#include "fs/cdev.h"
#include <os/tty.h>
#include <os/mutex.h>
#include <os/kmalloc.h>
#include <os/device.h>
#include <os/printk.h>

struct ktermios tty_std_termios = {	/* for the benefit of tty drivers  */
	.c_iflag = ICRNL | IXON,
	.c_oflag = OPOST | ONLCR,
	.c_cflag = B38400 | CS8 | CREAD | HUPCL,
	.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK |
		   ECHOCTL | ECHOKE | IEXTEN,
	.c_cc = INIT_C_CC,
	.c_ispeed = 38400,
	.c_ospeed = 38400,
	/* .c_line = N_TTY, */
};

LIST_HEAD(tty_drivers);			/* linked list of tty drivers */
DEFINE_MUTEX(tty_mutex);

static int tty_fops_open(struct inode *inode, struct file *file) {
    struct tty_struct *tty = file ? file->private_data : NULL;

    (void)inode;
    if (tty == NULL) {
        return -ENODEV;
    }

    tty->count++;
    if (tty->driver != NULL && tty->driver->ops != NULL && tty->driver->ops->open != NULL) {
        return tty->driver->ops->open(tty, file);
    }

    return 0;
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
    file->private_data = NULL;
    return 0;
}

static ssize_t tty_fops_read(struct file *file, char *buf, size_t size, loff_t *offset) {
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

static ssize_t tty_fops_write(struct file *file, const char *buf, size_t size, loff_t *offset) {
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

int tty_register_device(struct tty_driver *driver, unsigned int index)
{
    dev_t dev;
    char name[32];

    if (!driver)
        return -EINVAL;

    if (index >= driver->num)
        return -EINVAL;

    dev = MKDEV(driver->major,
                driver->minor_start + index);

    snprintk(name, sizeof(name),
             "%s%u",
             driver->name,
             index);

    return devnode_register(name,
                            DEV_CHAR,
                            dev,
                            &tty_fops,
                            driver);
}

int tty_register_driver(struct tty_driver *driver) {
	int error;
	dev_t dev;

	if (!driver->major) {
		error = alloc_chrdev_region(&dev, driver->minor_start,
						driver->num, driver->name);
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
	list_add(&driver->tty_drivers, &tty_drivers);
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

/**
 * __tty_alloc_driver - allocate tty driver
 * @lines: count of lines this driver can handle at most
 * @owner: module which is responsible for this driver
 * @flags: some of enum tty_driver_flag, will be set in driver->flags
 *
 * This should not be called directly, tty_alloc_driver() should be used
 * instead.
 *
 * Returns: struct tty_driver or a PTR-encoded error (use IS_ERR() and friends).
 */
struct tty_driver *__tty_alloc_driver(unsigned int lines, unsigned long flags)
{
	struct tty_driver *driver;
	unsigned int cdevs = 1;
	int err;

	driver = kzalloc(sizeof(*driver));
	if (!driver)
		return ERR_PTR(-ENOMEM);

	driver->num = lines;
	driver->flags = flags;

    driver->ttys = kmalloc(lines*sizeof(*driver->ttys));
    driver->termios = kmalloc(lines*sizeof(*driver->termios));
    if (!driver->ttys || !driver->termios) {
        err = -ENOMEM;
        goto err_free_all;
    }
	
    driver->ports = kmalloc(lines*sizeof(*driver->ports));
    if (!driver->ports) {
        err = -ENOMEM;
        goto err_free_all;
    }
    cdevs = lines;
	

	driver->cdevs = kmalloc(cdevs*sizeof(*driver->cdevs));
	if (!driver->cdevs) {
		err = -ENOMEM;
		goto err_free_all;
	}

	return driver;
err_free_all:
	kfree(driver->ports);
	kfree(driver->ttys);
	kfree(driver->termios);
	kfree(driver->cdevs);
	kfree(driver);
	return ERR_PTR(err);
}