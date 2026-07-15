#include <os/spinlock.h>
#include <os/tty.h>
#include <os/tty_buffer.h>
#include <os/tty_ldisc.h>
#include <os/tty_port.h>
#include <os/err.h>
#include <os/atomic.h>
#include <os/printk.h>
#include <os/kmalloc.h>

static spinlock_t tty_ldiscs_lock = SPINLOCK_INIT;
static struct tty_ldisc_ops *tty_ldiscs[NR_LDISCS];

int tty_register_ldisc(struct tty_ldisc_ops *new_ldisc) {
	unsigned long flags;

	if (new_ldisc->num < N_TTY || new_ldisc->num >= NR_LDISCS)
		return -EINVAL;

	flags = spin_lock_irqsave(&tty_ldiscs_lock);
	tty_ldiscs[new_ldisc->num] = new_ldisc;
	spin_unlock_irqrestore(&tty_ldiscs_lock, flags);

	return 0;
}

void tty_unregister_ldisc(struct tty_ldisc_ops *ldisc) {
	unsigned long flags;

	flags = spin_lock_irqsave(&tty_ldiscs_lock);
	tty_ldiscs[ldisc->num] = NULL;
	spin_unlock_irqrestore(&tty_ldiscs_lock, flags);
}

static struct tty_ldisc_ops *get_ldops(int disc) {
	unsigned long flags;
	struct tty_ldisc_ops *ldops, *ret;

	flags = spin_lock_irqsave(&tty_ldiscs_lock);
	ret = ERR_PTR(-EINVAL);
	ldops = tty_ldiscs[disc];
    ret = ERR_PTR(-EAGAIN);
	if (ldops) {
        atomic_inc(&ldops->refcnt);
		ret = ldops;
	}
	spin_unlock_irqrestore(&tty_ldiscs_lock, flags);
	return ret;
}

static void put_ldops(struct tty_ldisc_ops *ldops) {
	unsigned long flags;

	flags = spin_lock_irqsave(&tty_ldiscs_lock);
	atomic_dec(&ldops->refcnt);
	spin_unlock_irqrestore(&tty_ldiscs_lock, flags);
}


struct tty_ldisc *tty_ldisc_get(struct tty_struct *tty, int disc) {
	struct tty_ldisc *ld;
	struct tty_ldisc_ops *ldops;

	if (disc < 0 || disc >= NR_LDISCS)
		return ERR_PTR(-EINVAL);

	/*
	 * Get the ldisc ops - we may need to request them to be loaded
	 * dynamically and try again.
	 */

    /*暂时还不支持动态加载*/
	ldops = get_ldops(disc);
	if (IS_ERR(ldops)) {
		return ERR_CAST(ldops);
	}

	/*
	 * There is no way to handle allocation failure of only 16 bytes.
	 * Let's simplify error handling and save more memory.
	 */
	ld = kmalloc(sizeof(struct tty_ldisc));
	ld->ops = ldops;
	ld->tty = tty;
	if (ldops->open) {
		int ret = ldops->open(tty);

		if (ret) {
			kfree(ld);
			put_ldops(ldops);
			return ERR_PTR(ret);
		}
	}

	return ld;
}

void tty_ldisc_put(struct tty_ldisc *ld) {
	if (!ld) return;

	if (ld->ops->close)
		ld->ops->close(ld->tty);
	put_ldops(ld->ops);
	kfree(ld);
}

struct tty_ldisc *tty_ldisc_ref(struct tty_struct *tty) {
	if (!tty)
		return NULL;

	return tty->ldisc;
}

void tty_ldisc_deref(struct tty_ldisc *ld) {
	(void)ld;
}

/**
 * tty_ldisc_flush		-	flush line discipline queue
 * @tty: tty to flush ldisc for
 *
 * Flush the line discipline queue (if any) and the tty flip buffers for this
 * @tty.
 */
void tty_ldisc_flush(struct tty_struct *tty)
{
	struct tty_ldisc *ld = tty_ldisc_ref(tty);

	tty_buffer_flush(tty, ld);
	if (ld)
		tty_ldisc_deref(ld);
}

/**
 *	tty_ldisc_init		-	ldisc setup for new tty
 *	@tty: tty being allocated
 *
 *	Set up the line discipline objects for a newly allocated tty. Note that
 *	the tty structure is not completely set up when this call is made.
 */

void tty_ldisc_init(struct tty_struct *tty)
{
	struct tty_ldisc *ld = tty_ldisc_get(tty, N_TTY);
	if (IS_ERR(ld))
		panic("n_tty: init_tty");
	tty->ldisc = ld;
}

/**
 *	tty_ldisc_init		-	ldisc cleanup for new tty
 *	@tty: tty that was allocated recently
 *
 *	The tty structure must not becompletely set up (tty_ldisc_setup) when
 *      this call is made.
 */
void tty_ldisc_deinit(struct tty_struct *tty)
{
	tty_ldisc_put(tty->ldisc);
	tty->ldisc = NULL;
}