/*
 * jit_sched.c -- Another approach to delaying execution.
 *
 * This one explicitly releases the processor, by calling the schedule,
 * rather than busy waiting, as the jit_busy module does.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>	/* we're using the seq_file interface */
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>	/* schedule() */

#include <asm/hardirq.h>

int jit_sched_lines	= 5;	/* Temporary fix; determines no. of lines return by `cat` */
int delay		= HZ;	/* the default delay, expressed in jiffies */

module_param(delay, int, 0);

MODULE_AUTHOR("Salym Senyonga");
MODULE_LICENSE("GPL");

/*
 * /proc fs stuff : using seq_file
 */

/* The sequence iteration methods */
static void *jit_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= jit_sched_lines)
		return NULL;
	return jit_sched_lines + *pos;
}

static void *jit_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	if (++(*pos) >= jit_sched_lines)
		return NULL;
	return jit_sched_lines + *pos;
}

static void jit_seq_stop(struct seq_file *s, void *v)
{
}

static int jit_seq_show(struct seq_file *s, void *v)
{
	unsigned long j0, j1;	/* jiffies */

	j0 = jiffies;
	j1 = j0 + delay;

	while (time_before(jiffies, j1))
		schedule();
	j1 = jiffies;	/* value after we delayed */
	seq_printf(s, "%9li %9li\n", j0, j1);

	return 0;
}

/* build up the seq_ops structure */
static struct seq_operations jit_seq_ops = {
	.start	= jit_seq_start,
	.next	= jit_seq_next,
	.stop	= jit_seq_stop,
	.show	= jit_seq_show
};

/* the open() method that connects the /proc file the the seq_ops */
static int jit_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &jit_seq_ops);
}

/* Initialize a fops structure for the /proc file. Required by the seq_file
 * interface. We only are concerned about the open() method
 */

static struct file_operations jit_proc_ops = {
	.owner		= THIS_MODULE,
	.open		= jit_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release
};

static void jit_create_proc(void)
{
	struct proc_dir_entry *entry;
	entry = proc_create_data("jit_sched", 0, NULL, &jit_proc_ops, NULL);
}

static void jit_remove_proc(void)
{
	remove_proc_entry("jit_sched", NULL);
}

int __init jit_init(void)
{
	jit_create_proc();

	return 0; /* success */
}

void __exit jit_cleanup(void)
{
	jit_remove_proc();
}

module_init(jit_init);
module_exit(jit_cleanup);
