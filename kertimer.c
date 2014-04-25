/*
 * kertimer.c - testing the Kernel timer api <linux/timer.h>
 */

#include <linux/kernel.h>	/* printk defn */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>	/* dev_t defn */
#include <linux/kdev_t.h>	/* MAJOR, MINOR macro defns */
#include <linux/fs.h>		/* alloc_chrdev_region defn */
#include <linux/device.h>
#include <linux/cdev.h>		/* cdev_add and cdev_init */
#include <linux/uaccess.h>	/* copy_to_user and copy_from_user */
#include <linux/timer.h>
#include <linux/sched.h>	/* jiffies */

static dev_t first;		/* Global var. for first dev number */
static struct cdev c_dev;	/* Global variable for the char device structure */
static struct class *cl;	/* Global variable for the device class */

static struct timer_list lazy_timer;
static int delay	= HZ;

/* the procrastinating function */
static void lazy(unsigned long arg)
{
	pr_info("Lazy finally waking up....");
}

/* Open and Close */

static int kt_open(struct inode *i, struct file *filp)
{
	pr_info("Driver: open()\n");
	return 0;
}

static int kt_close(struct inode *i, struct file *filp)
{
	pr_info("Driver: close()\n");
	return 0;
}

/* Data Management */

static char c;

static ssize_t kt_read(struct file *filp, char __user *buf, size_t len,
		       loff_t *off)
{
	unsigned long j		= jiffies;
	unsigned long data	= 9687;		/* temporary fix */

	pr_info("In Read Method just before timer switched on");

	init_timer(&lazy_timer);
	lazy_timer.data		= data;
	lazy_timer.function	= lazy;
	lazy_timer.expires	= j + delay;
	add_timer(&lazy_timer);

	pr_info("Driver: read()\n");
	if (*off == 0) {
		if (copy_to_user(buf, &c, 1) != 0)
			return -EFAULT;
		else {
			(*off)++;
			return 1;
		}
	}
	else {
		return 0;
	}
}

static ssize_t kt_write(struct file *filp, const char __user *buf,
			 size_t len, loff_t *off)
{
	pr_info("Driver: write()\n");
	if (copy_from_user(&c, buf + (len - 1), 1) != 0)
		return -EFAULT;
	else
		printk("%s", &c);
		return len;
}

/*
 * Add the device-specific file operations to the file_operations structure
 */

static struct file_operations kt_fops = {
	.owner	 = THIS_MODULE,
	.open    = kt_open,
	.release = kt_close,
	.read	 = kt_read,
	.write	 = kt_write
};

static int __init kt_init(void)
{
	pr_info("Bonjour! Kertimer registred");

	if (alloc_chrdev_region(&first, 0, 1, "kertimer") < 0)
		return -1;

	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL) {
		unregister_chrdev_region(first, 1);
		return -1;
	}

	if (device_create(cl, NULL, first, NULL, "kertimer") == NULL) {
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	/* initialize the char device structure */
	cdev_init(&c_dev, &kt_fops);
	if (cdev_add(&c_dev, first, 1) == -1) {
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	return 0;
}

static void __exit kt_exit(void)
{
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);
	pr_info("Au revour! kertimer unregistered");
}

module_init(kt_init);
module_exit(kt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("Hello world Driver");
