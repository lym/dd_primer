/*
 * ofd.c - A hello world driver
 */

#include <linux/kernel.h> /* printk defn */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>	/* dev_t defn */
#include <linux/kdev_t.h>	/* MAJOR, MINOR macro defns */
#include <linux/fs.h>		/* alloc_chrdev_region defn */
#include "/home/lym/kernel_src/devel/tools/lib/lockdep/uinclude/linux/kern_levels.h" /* defines the kernel log-levels */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("Hello world Driver");

static dev_t first;		/* Global var. for first dev number */

static int ofd_init(void)	/* constructor */
{
	printk(KERN_INFO "Bonjour! ofd registred");

	if (alloc_chrdev_region(&first, 0, 3, "trivial_dev") < 0)
		return -1;
	printk(KERN_INFO "<Major, Minor>: <%d, %d>\n", MAJOR(first), MINOR(first));
	return 0;
}

static void ofd_exit(void)	/* destructor */
{
	unregister_chrdev_region(first, 3);
	printk(KERN_INFO "Au revour! ofd unregistered");
}

module_init(ofd_init);
module_exit(ofd_exit);
