/*
 * sleep.c -- the writers awake the readers
 *
 * You have to write before you can read ;-)
 *
 * Test by having read and write calls in separate terminal windows.
 */

#include <linux/module.h>
#include <linux/init.h>

#include <linux/sched.h>	/* current and everything */
#include <linux/kernel.h>
#include <linux/fs.h>		/* register_chrdev();	*/
#include <linux/types.h>
#include <linux/wait.h>		/* sleep-related stuff	*/

MODULE_LICENSE("GPL");

static int sleepy_major = 0;
static DECLARE_WAIT_QUEUE_HEAD(wq);
static int flag		= 0;

ssize_t sleepy_read(struct file *filp, char __user *buf, size_t count,
		    loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) going to sleep\n", current->pid,
			current->comm);
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	return 0;	/* EOF */
}

ssize_t sleepy_write(struct file *filp, const char __user *buf, size_t count,
		     loff_t *pos)
{
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n",
			current->pid, current->comm);
	flag = 1;
	wake_up_interruptible(&wq);
	return count;		/* succeed to avoid retrial */
}

struct file_operations sleepy_fops = {
	.owner	= THIS_MODULE,
	.read	= sleepy_read,
	.write	= sleepy_write
};

int sleepy_init(void)
{
	int result;

	/*
	 * Register your major, and accept a dynamic number
	 */
	result = register_chrdev(sleepy_major, "sleepy", &sleepy_fops);
	if (result < 0)
		return result;
	if (sleepy_major == 0)
		sleepy_major = result;	/* dynamic */
	return 0;
}

void sleepy_cleanup(void)
{
	unregister_chrdev(sleepy_major, "sleepy");
}

module_init(sleepy_init);
module_exit(sleepy_cleanup);
