/*
 * jiffies.c - A module to aid in understanding how to measure time lapses.
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
#include <linux/jiffies.h>

static dev_t first;		/* Global var. for first dev number */
static struct cdev c_dev;	/* Global variable for the char device structure */
static struct class *cl;	/* Global variable for the device class */

/*
 * Open and Close
 */

static int ofd_open(struct inode *i, struct file *filp)
{
	printk(KERN_INFO "Driver: open()\n");
	return 0;
}

static int ofd_close(struct inode *i, struct file *filp)
{
	printk(KERN_INFO "Driver: close()\n");
	return 0;
}

/*
 * Data Management
 */

static char c;

static ssize_t ofd_read(struct file *filp, char __user *buf, size_t len,
		       loff_t *off)
{
	printk(KERN_INFO "Driver: read()\n");
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

static ssize_t ofd_write(struct file *filp, const char __user *buf,
			 size_t len, loff_t *off)
{
	printk(KERN_INFO "Driver: write()\n");
	if (copy_from_user(&c, buf + (len - 1), 1) != 0)
		return -EFAULT;
	else
		printk("%s", &c);
		return len;
}

/*
 * Time test stuff
 */
void timer_stuff(void)
{
	unsigned long j, k, stamp_1, stamp_half, stamp_n;
	long elapsed, elapsed_msec;

	j = jiffies;		/* read the current value */
	stamp_1		= j + HZ;	/* 1 second in the future */
	stamp_half	= j + (HZ / 2);	/* half a second */
	stamp_n		= j + (HZ / 1000); /* n milliseconds */

	printk(KERN_INFO "%lu\n %lu\n %lu\n %lu\n", j, stamp_1, stamp_half, stamp_n);

	k = jiffies;		/* read another value */
	elapsed = (long) k - (long) j;
	elapsed_msec	= elapsed * (1000 / HZ);
	printk(KERN_INFO "%ld milliseconds elapsed since first read", elapsed_msec);

}



/*
 * Add the device-specific file operations to the file_operations structure
 */

static struct file_operations ofd_fops = {
	.owner	 = THIS_MODULE,
	.open    = ofd_open,
	.release = ofd_close,
	.read	 = ofd_read,
	.write	 = ofd_write
};

static int __init ofd_init(void)	/* constructor */
{
	printk(KERN_INFO "Bonjour! ofd registred");

	if (alloc_chrdev_region(&first, 0, 1, "trivial_dev") < 0)
		return -1;

	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL) {
		unregister_chrdev_region(first, 1);
		return -1;
	}

	if (device_create(cl, NULL, first, NULL, "mynull") == NULL) {
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	/* initialize the char device structure */
	cdev_init(&c_dev, &ofd_fops);
	if (cdev_add(&c_dev, first, 1) == -1) {
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
	}
	//printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(first), MINOR(first));
	timer_stuff();

	return 0;
}

static void __exit ofd_exit(void)	/* destructor */
{
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev_region(first, 1);
	printk(KERN_INFO "Au revour! ofd unregistered");
}

module_init(ofd_init);
module_exit(ofd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("Hello world Driver");
