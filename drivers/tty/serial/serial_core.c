#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/serial_core.h>
#include <os/tty.h>

static struct uart_state *serial_get_state(struct tty_struct *tty) {
    if (!tty)
        return NULL;

    return tty->driver_data;
}

static struct uart_port *serial_get_port(struct tty_struct *tty) {
    struct uart_state *state = serial_get_state(tty);

    if (!state)
        return NULL;

    return state->uart_port;
}

// 把新创建的 tty_struct 和对应的 uart_state/tty_port 关联起来。
static int serial_install(struct tty_driver *driver, struct tty_struct *tty) {
    struct uart_driver *uart_driver;
    struct uart_state *state;
    struct tty_port *tty_port;
    unsigned int index;

    if (!driver || !tty)
        return -EINVAL;

    uart_driver = driver->driver_state;
    if (!uart_driver)
        return -ENODEV;

    index = tty->index;

    if (index >= uart_driver->nr)
        return -ENODEV;

    state = &uart_driver->state[index];

    /*
     * uart_add_one_port() 尚未绑定真实硬件端口。
     */
    if (!state->uart_port)
        return -ENODEV;

    tty_port = &state->port;

    /*
     * 建立 TTY core 到 serial core 的关联。
     */
    tty->driver = driver;
    tty->port = tty_port;
    tty->driver_data = state;
    tty->ops = driver->ops;

    /*
     * 建立 tty_port 到当前 tty_struct 的反向关联。
     *
     * 如果你有 tty_port_tty_set()，优先使用那个函数，
     * 因为它可以在内部处理锁或引用计数。
     */
    tty_port->tty = tty;

    /*
     * tty_driver 的索引表如果存在，也同步记录。
     */
    if (driver->ports)
        driver->ports[index] = tty_port;

    if (driver->ttys)
        driver->ttys[index] = tty;

    return 0;
}

static void serial_remove(struct tty_driver *driver, struct tty_struct *tty) {
    struct uart_state *state;
    struct tty_port *port;
    unsigned int index;

    if (!driver || !tty)
        return;

    index = tty->index;
    state = tty->driver_data;
    port = tty->port;

    if (driver->ttys && index < driver->num) {
        if (driver->ttys[index] == tty)
            driver->ttys[index] = NULL;
    }

    /*
     * ports[] 指向的是长期存在的 uart_state.port，
     * 通常在 remove tty_struct 时不必清空。
     *
     * 它应该在 uart_remove_one_port() 时清空。
     */

    if (port && port->tty == tty)
        port->tty = NULL;

    tty->driver_data = NULL;
    tty->port = NULL;

    (void)state;
}

static int serial_open(struct tty_struct *tty, struct file *filp) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;
    bool first_open;
    int ret = 0;

    (void)filp;

    if (!tty)
        return -EINVAL;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port || !port->ops)
        return -ENODEV;

    /*
     * open_count 最好属于 tty_port。
     *
     * 这里只用 port->lock 保护计数。假设 startup() 不能在
     * 自旋锁内调用，因为它可能申请 IRQ 或执行较慢操作。
     */
    flags = spin_lock_irqsave(&port->lock);

    first_open = (state->port.open_count == 0);
    state->port.open_count++;

    spin_unlock_irqrestore(&port->lock, flags);

    if (!first_open)
        return 0;

    if (port->ops->startup) {
        ret = port->ops->startup(port);
        if (ret) {
            /*
             * startup 失败必须回滚 open_count。
             */
            flags = spin_lock_irqsave(&port->lock);

            if (state->port.open_count > 0)
                state->port.open_count--;

            spin_unlock_irqrestore(&port->lock, flags);

            return ret;
        }
    }

    /*
     * 硬件启动后应用当前 termios。
     */
    if (port->ops->set_termios)
        port->ops->set_termios(port, &tty->termios, NULL);

    return 0;
}

static void serial_close(struct tty_struct *tty, struct file *filp) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;
    bool last_close = false;

    (void)filp;

    if (!tty)
        return;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port || !port->ops)
        return;

    flags = spin_lock_irqsave(&port->lock);

    if (state->port.open_count == 0) {
        spin_unlock_irqrestore(&port->lock, flags);
        return;
    }

    state->port.open_count--;

    if (state->port.open_count == 0)
        last_close = true;

    spin_unlock_irqrestore(&port->lock, flags);

    if (!last_close)
        return;

    /*
     * 防止关闭后仍触发 TX 中断。
     */
    if (port->ops->stop_tx)
        port->ops->stop_tx(port);

    if (port->ops->shutdown)
        port->ops->shutdown(port);
}

static ssize_t serial_write(struct tty_struct *tty, const u8 *buf, size_t count) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;
    size_t written;

    if (!tty || (!buf && count != 0))
        return -EINVAL;

    if (count == 0)
        return 0;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port || !port->ops)
        return -ENODEV;

    flags = spin_lock_irqsave(&port->lock);

    written = ringbuffer_write(&state->tx_buf, buf, count);

    spin_unlock_irqrestore(&port->lock, flags);

    /*
     * 放到锁外调用，避免硬件 start_tx() 与当前锁递归。
     *
     * 你需要规定 start_tx() 自己负责必要的硬件锁保护。
     */
    if (written && port->ops->start_tx)
        port->ops->start_tx(port);

    return (ssize_t)written;
}

static int serial_write_room(struct tty_struct *tty) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;
    size_t room;

    if (!tty)
        return 0;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port)
        return 0;

    flags = spin_lock_irqsave(&port->lock);

    room = ringbuffer_space(&state->tx_buf);

    spin_unlock_irqrestore(&port->lock, flags);

    // if (room > UINT_MAX)
    //     return UINT_MAX;

    return (unsigned int)room;
}

static void serial_set_termios(struct tty_struct *tty, const struct ktermios *old) {
    struct uart_port *port;

    if (!tty)
        return;

    port = serial_get_port(tty);
    if (!port || !port->ops)
        return;

    if (port->ops->set_termios)
        port->ops->set_termios(port, &tty->termios, old);
}

static int serial_chars_in_buffer(struct tty_struct *tty) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;
    size_t count;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port)
        return 0;

    flags = spin_lock_irqsave(&port->lock);

    count = ringbuffer_count(&state->tx_buf);

    spin_unlock_irqrestore(&port->lock, flags);

    // if (count > UINT_MAX)
    //     return UINT_MAX;

    return (unsigned int)count;
}

static void serial_flush_buffer(struct tty_struct *tty) {
    struct uart_state *state;
    struct uart_port *port;
    unsigned long flags;

    state = serial_get_state(tty);
    port = serial_get_port(tty);

    if (!state || !port)
        return;

    flags = spin_lock_irqsave(&port->lock);

    ringbuffer_reset(&state->tx_buf);

    spin_unlock_irqrestore(&port->lock, flags);

    if (port->ops && port->ops->stop_tx)
        port->ops->stop_tx(port);
}

static const struct tty_operations uart_tty_ops = {
    .install = serial_install,
    .remove = serial_remove,

    .open = serial_open,
    .close = serial_close,

    .write = serial_write,
    .write_room = serial_write_room,

    .set_termios = serial_set_termios,
    .chars_in_buffer = serial_chars_in_buffer,
    .flush_buffer = serial_flush_buffer,
};

void uart_port_init(struct uart_port *port) {
    if (port == NULL) {
        return;
    }

    spin_lock_init(&port->lock);
    port->irq = 0;
    port->uartclk = 0;
    port->mapbase = 0;
    port->membase = NULL;
    port->regshift = 0;
    port->ops = NULL;
    port->private_data = NULL;
}

// 初始化uart_driver里的uart_state数组和tty_driver结构体，注册tty_driver
int uart_register_driver(struct uart_driver *drv) {
    struct tty_driver *normal;
    int i, retval = -ENOMEM;

    if (!drv || !drv->driver_name || !drv->dev_name || drv->nr == 0)
        return -EINVAL;
    if (drv->state || drv->tty_driver)
        return -EBUSY;

    /*
     * Maybe we should be using a slab cache for this, especially if
     * we have a large number of ports to handle.
     */
    drv->state = kzalloc(sizeof(struct uart_state) * drv->nr);
    if (!drv->state)
        return -ENOMEM;

    normal = alloc_tty_driver(drv->nr);
    if (!normal)
        goto out_kfree;

    drv->tty_driver = normal;

    normal->driver_name = drv->driver_name;
    normal->name = drv->dev_name;
    normal->major = drv->major;
    normal->minor_start = drv->minor;

    normal->init_termios = tty_std_termios;
    normal->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
    normal->init_termios.c_ispeed = normal->init_termios.c_ospeed = 9600;

    normal->driver_state = drv;
    tty_set_operations(normal, &uart_tty_ops);

    /*
     * Initialise the UART state(s).
     */
    for (i = 0; i < drv->nr; i++) {
        struct uart_state *state = drv->state + i;
        struct tty_port *port = &state->port;
        tty_port_init(port); //port里对应的tty_struct还并没有分配
        ringbuffer_init(&state->tx_buf, state->tx_buf_data,
                        sizeof(state->tx_buf_data));
    }

    retval = tty_register_driver(normal);
    if (retval >= 0)
        return retval;

    for (i = 0; i < drv->nr; i++)
        tty_port_destroy(&drv->state[i].port);
    tty_put_driver(normal);
    drv->tty_driver = NULL;
out_kfree:
    kfree(drv->state);
    drv->state = NULL;
    return retval < 0 ? retval : -ENOMEM;
}

void uart_insert_char(struct uart_port *port, unsigned char ch)
{
    struct tty_port *tty_port;

    if (!port || !port->state)
        return;

    tty_port = &port->state->port;
    tty_insert_flip_char(tty_port, ch, TTY_NORMAL);
}

void uart_flip_buffer_push(struct uart_port *port)
{
    if (!port || !port->state)
        return;
    tty_flip_buffer_push(&port->state->port);
}

void uart_write_wakeup(struct uart_port *port)
{
    if (!port || !port->state)
        return;
    if (port->state->port.client_ops &&
        port->state->port.client_ops->write_wakeup)
        port->state->port.client_ops->write_wakeup(&port->state->port);
}

void uart_unregister_driver(struct uart_driver *driver) {
    unsigned int i;

    if (driver == NULL) {
        return;
    }

    if (driver->tty_driver != NULL) {
        tty_unregister_driver(driver->tty_driver);
    }

    if (driver->state != NULL) {
        for (i = 0; i < driver->nr; i++)
            tty_port_destroy(&driver->state[i].port);
        kfree(driver->state);
        driver->state = NULL;
    }

    if (driver->tty_driver != NULL) {
        tty_put_driver(driver->tty_driver);
        driver->tty_driver = NULL;
    }
}

int uart_add_one_port(struct uart_driver *driver, struct uart_port *uart_port) {
    struct uart_state *state;
    int ret;

    if (driver == NULL || uart_port == NULL) {
        return -EINVAL;
    }

    if (driver->state == NULL || driver->tty_driver == NULL || uart_port->line >= driver->nr) {
        return -EINVAL;
    }

    if (driver->state[uart_port->line].uart_port != NULL) {
        return -EEXIST;
    }

    state = &driver->state[uart_port->line];

    // 关联 uart_port 和 uart_state
    uart_port->state = state;
    state->uart_port = uart_port;

    // 注册 tty_port 到 tty_driver
    driver->tty_driver->ports[uart_port->line] = &state->port;

    ret = tty_register_device(driver->tty_driver, (int)uart_port->line,
                              uart_port->dev);
    if (ret) {
        driver->tty_driver->ports[uart_port->line] = NULL;
        state->uart_port = NULL;
        uart_port->state = NULL;
        return ret;
    }

    return 0;
}

void uart_remove_one_port(struct uart_driver *driver, struct uart_port *port) {
    struct uart_state *state;

    if (!driver || !port || !driver->state || port->line >= driver->nr)
        return;

    state = &driver->state[port->line];
    if (state->uart_port != port)
        return;

    if (state->port.open_count && port->ops && port->ops->shutdown)
        port->ops->shutdown(port);
    state->port.open_count = 0;

    if (driver->tty_driver) {
        tty_unregister_device(driver->tty_driver, (int)port->line);
        driver->tty_driver->ports[port->line] = NULL;
    }

    state->uart_port = NULL;
    port->state = NULL;
}
