/*
 * jit.c -- the just-in-time module
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

/*
 * This module is a silly one: it only embeds short code fragments that show
 * how time delays can be handled in the kernel.
 */

int jit_nr_devs = 5;	/* Temporary fix; determines no. of lines return by `cat` */
int delay	= HZ;		/* the default delay, expressed in jiffies */

module_param(delay, int, 0);

MODULE_AUTHOR("Salym Senyonga");
MODULE_LICENSE("GPL");

/* Use these as data pointers, to implement four files in one function */
enum jit_files {
	JIT_BUSY,
	JIT_SCHED,
	JIT_QUEUE,
	JIT_SCHEDTO
};

/*
 * /proc fs stuff : using seq_file
 */

/* The sequence iteration methods */
static void *jit_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos >= jit_nr_devs)
		return NULL;
	return jit_nr_devs + *pos;
}

static void *jit_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	if (++(*pos) >= jit_nr_devs)
		return NULL;
	return jit_nr_devs + *pos;
}

static void jit_seq_stop(struct seq_file *s, void *v)
{
}

static int jit_seq_show(struct seq_file *s, void *v)
{
	seq_printf(s, "Jit Proc File Operational\n");
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
	entry = proc_create_data("jitseq", 0, NULL, &jit_proc_ops, NULL);
}

static void jit_remove_proc(void)
{
	remove_proc_entry("jitseq", NULL);
}
/*
 * This function prints one line of data, after sleeping one second. It can
 * sleep in different ways, according to the data pointer
 */
int jit_fn(char *buf, char **start, off_t offset, int len, int *eof, void *data)
{
	unsigned long j0, j1;	/* jiffies */
	wait_queue_head_t wait;

	init_waitqueue_head(&wait);
	j0 = jiffies;
	j1 = j0 + delay;

	switch((long) data) {
		case JIT_BUSY:
			while (time_before(jiffies, j1))
				cpu_relax();
			break;
		case JIT_SCHED:
			while (time_before(jiffies, j1))
				schedule();
			break;
		case JIT_QUEUE:
			wait_event_interruptible_timeout(wait, 0, delay);
			break;
		case JIT_SCHEDTO:
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(delay);
			break;
	}

	j1 = jiffies;	/* actual value after we delayed */

	len = sprintf(buf, "%9li %9li\n", j0, j1);
	*start = buf;
	return len;
}

/*
 * This file, on the other hand, returns the current time forever
 */
int jit_currenttime(char *buf, char **start, off_t offset, int len, int *eof,
		void *data)
{
	struct timeval tv1;
	struct timespec tv2;
	unsigned long j1;
	u64 j2;

	j1 = jiffies;
	j2 = get_jiffies_64();
	do_gettimeofday(&tv1);
	tv2 = current_kernel_time();

	/* print */
	len = 0;
	len += sprintf(buf, "0x%08lx 0x%016Lx %10i.%06i\n" "%40i.%09i\n", j1,
			j2, (int) tv1.tv_sec, (int) tv1.tv_usec,
			(int) tv2.tv_sec, (int) tv2.tv_nsec);
	*start = buf;
	return len;
}

/*
 * The timer example follows
 */

int tdelay = 10;
module_param(tdelay, int, 0);

/* This data structure used as "data" for the timer and tasklet functions */
struct jit_data {
	struct timer_list timer;
	struct tasklet_struct tlet;
	int hi;				/* tasklet or tasklet_hi */
	wait_queue_head_t wait;
	unsigned long prev_jiffies;
	unsigned char *buf;
	int loops;
};

#define JIT_ASYNC_LOOPS 5

void jit_timer_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *) arg;
	unsigned long j = jiffies;
	data->buf += sprintf(data->buf, "%9li %3li %i %6i %i %s\n", j,
			     j - data->prev_jiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->timer.expires += tdelay;
		data->prev_jiffies = j;
		add_timer(&data->timer);
	}
	else {
		wake_up_interruptible(&data->wait);
	}
}

/* the /proc function: allocate everything to allow concurrency */
int jit_timer(char *buf, char **start, off_t offset, int len, int *eof,
	      void *unused_data)
{
	struct jit_data *data;
	char *buf2 = buf;
	unsigned long j = jiffies;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	init_timer(&data->timer);
	init_waitqueue_head(&data->wait);

	/* write the first lines in the buffer */
	buf2 += sprintf(buf2, " time	delta	inirq	pid	cpu	command\n");
	buf2 += sprintf(buf2, "%9li	%3li	%i	%6i	%i	%s\n",
			j, 0L, in_interrupt() ? 1 : 0, current->pid,
			smp_processor_id(), current->comm);

	/* fill the data for our timer function */
	data->prev_jiffies	= j;
	data->buf		= buf2;
	data->loops = JIT_ASYNC_LOOPS;

	/* register the timer */
	data->timer.data	= (unsigned long) data;
	data->timer.function	= jit_timer_fn;
	data->timer.expires	= j + tdelay;	/* parameter */
	add_timer(&data->timer);

	/* wait for the buffer to fill */
	wait_event_interruptible(data->wait, !data->loops);
	if (signal_pending(current))
		return -ERESTARTSYS;
	buf2 = data->buf;
	kfree(data);
	*eof = 1;
	return buf2 - buf;
}

void jit_tasklet_fn(unsigned long arg)
{
	struct jit_data *data = (struct jit_data *) arg;
	unsigned long j = jiffies;
	data->buf += sprintf(data->buf, "%9li	%3li	%i	%6i	%i	%s\n",
			     j, j - data->prev_jiffies, in_interrupt() ? 1 : 0,
			     current->pid, smp_processor_id(), current->comm);

	if (--data->loops) {
		data->prev_jiffies = j;
		if (data->hi)
			tasklet_hi_schedule(&data->tlet);
		else
			tasklet_schedule(&data->tlet);
	}
	else {
		wake_up_interruptible(&data->wait);
	}
}

/* The /proc function: allocate everything to allow concurrency */
int jit_tasklet(char *buf, char **start, off_t offset, int len, int *eof,
		void *arg)
{
	struct jit_data *data;
	char *buf2 = buf;
	unsigned long j = jiffies;
	long hi = (long) arg;

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	init_waitqueue_head(&data->wait);

	/* write the first lines in the buffer */
	buf2 += sprintf(buf2, "	time	delta	inirq	pid	cpu	command\n");
	buf2 += sprintf(buf2, "	%9li	%3li	%i	%6i	%i	%s\n",
			j, 0L, in_interrupt() ? 1 : 0, current->pid,
			smp_processor_id(), current->comm);

	/* fill the data for our tasklet function */
	data->prev_jiffies = j;
	data->buf	= buf2;
	data->loops	= JIT_ASYNC_LOOPS;

	/* register the tasklet */
	tasklet_init(&data->tlet, jit_tasklet_fn, (unsigned long) data);
	data->hi = hi;
	if (hi)
		tasklet_hi_schedule(&data->tlet);
	else
		tasklet_schedule(&data->tlet);

	/* wait for the buffer to fill */
	wait_event_interruptible(data->wait, !data->loops);

	if (signal_pending(current))
		return -ERESTARTSYS;
	buf2 = data->buf;
	kfree(data);
	*eof = 1;
	return buf2 - buf;
}

int __init jit_init(void)
{
	/*create_proc_read_entry("currenttime", 0, NULL, jit_currenttime, NULL);
	create_proc_read_entry("jitbusy", 0, NULL, jit_fn, (void *)JIT_BUSY);
	create_proc_read_entry("jitsched",0, NULL, jit_fn, (void *)JIT_SCHED);
	create_proc_read_entry("jitqueue",0, NULL, jit_fn, (void *)JIT_QUEUE);
	create_proc_read_entry("jitschedto", 0, NULL, jit_fn, (void *)JIT_SCHEDTO);

	create_proc_read_entry("jittimer", 0, NULL, jit_timer, NULL);
	create_proc_read_entry("jittasklet", 0, NULL, jit_tasklet, NULL);
	create_proc_read_entry("jittasklethi", 0, NULL, jit_tasklet, (void *)1);
	*/
	jit_create_proc();

	return 0; /* success */
}

void __exit jit_cleanup(void)
{
	/*remove_proc_entry("currenttime", NULL);
	remove_proc_entry("jitbusy", NULL);
	remove_proc_entry("jitsched", NULL);
	remove_proc_entry("jitqueue", NULL);
	remove_proc_entry("jitschedto", NULL);
	remove_proc_entry("jittimer", NULL);
	remove_proc_entry("jittasklet", NULL);
	remove_proc_entry("jittasklet_hi", NULL);
	*/
	jit_remove_proc();
}

module_init(jit_init);
module_exit(jit_cleanup);
