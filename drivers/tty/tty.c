#include <os/errno.h>
#include <os/string.h>
#include <os/tty.h>
#include <os/sched.h>

static int tty_inbuf_full(struct tty *tty)
{
    return tty->count == TTY_BUF_SIZE;
}

static int tty_inbuf_empty(struct tty *tty)
{
    return tty->count == 0;
}

static void tty_inbuf_push(struct tty *tty, char ch)
{
    if (tty_inbuf_full(tty)) {
        return;
    }

    tty->inbuf[tty->head] = ch;
    tty->head = (tty->head + 1) % TTY_BUF_SIZE;
    tty->count++;
}

static int tty_inbuf_pop(struct tty *tty, char *ch)
{
    if (tty_inbuf_empty(tty)) {
        return 0;
    }

    *ch = tty->inbuf[tty->tail];
    tty->tail = (tty->tail + 1) % TTY_BUF_SIZE;
    tty->count--;
    return 1;
}

static void tty_putc(struct tty *tty, char ch)
{
    if (tty->putc) {
        tty->putc(ch, tty->driver_data);
    }
}

static void tty_echo_char(struct tty *tty, char ch)
{
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

static void tty_flush_line(struct tty *tty)
{
    unsigned int i;

    for (i = 0; i < tty->line_len; i++) {
        tty_inbuf_push(tty, tty->linebuf[i]);
    }
    tty->line_len = 0;
}

static int tty_sleep_if_empty(struct tty *tty)
{
    struct task_struct *task = current;
    struct rq *rq;
    unsigned long rq_flags;
    unsigned long tty_flags;
    unsigned long wq_flags;

    if (task == NULL || task->status != TASK_RUNNING) {
        return 0;
    }

    wq_flags = spin_lock_irqsave(&tty->read_wait.lock);

    tty_flags = spin_lock_irqsave(&tty->lock);
    if (!tty_inbuf_empty(tty)) {
        spin_unlock_irqrestore(&tty->lock, tty_flags);
        spin_unlock_irqrestore(&tty->read_wait.lock, wq_flags);
        return 0;
    }
    spin_unlock_irqrestore(&tty->lock, tty_flags);

    task->status = TASK_SLEEPING;
    rq = this_rq();
    rq_flags = spin_lock_irqsave(&rq->lock);
    task->sched_class->dequeue_task(rq, task);
    spin_unlock_irqrestore(&rq->lock, rq_flags);

    task->wait.private = task;
    wait_queue_add(&tty->read_wait, &task->wait);

    spin_unlock_irqrestore(&tty->read_wait.lock, wq_flags);

    sched();
    return 1;
}

void tty_init(struct tty *tty, void (*putc)(char ch, void *data), void *data)
{
    if (tty == NULL) {
        return;
    }

    memset(tty, 0, sizeof(*tty));
    spin_lock_init(&tty->lock);
    init_waitqueue_head(&tty->read_wait);
    tty->echo = 1;
    tty->canonical = 1;
    tty->putc = putc;
    tty->driver_data = data;
}

void tty_receive_char(struct tty *tty, char ch)
{
    unsigned long flags;
    int wake = 0;
    int echo_backspace = 0;
    char echo_ch = 0;

    if (tty == NULL) {
        return;
    }

    if (ch == '\r') {
        ch = '\n';
    }

    flags = spin_lock_irqsave(&tty->lock);

    if (tty->canonical && (ch == '\b' || ch == 127)) {
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
        wake_up_one(&tty->read_wait);
    }
}

ssize_t tty_read(struct tty *tty, char *buf, size_t size)
{
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

ssize_t tty_write(struct tty *tty, const char *buf, size_t size)
{
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
