/*
 * ofd.c - A hello world driver
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include "/home/lym/kernel_src/devel/tools/lib/lockdep/uinclude/linux/kern_levels.h" /* defines the kernel log-levels */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("Hello world Driver");

static int ofd_init(void)	/* constructor */
{
	printk(KERN_INFO "Namaskar: ofd registred");
	return 0;
}

static void ofd_exit(void)	/* destructor */
{
	printk(KERN_INFO "Alvida: ofd unregistered");
}

module_init(ofd_init);
module_exit(ofd_exit);

