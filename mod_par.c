/*
 * A trivial module serving the sole purpose of demonstrating the concept of
 * module parameters. That is, modules that accept parameters as they are being
 * loaded.
 *
 * This module is used as:
 *	insmod mod_par.ko <par1=val> <par2=val>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>		/* module_param */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("Illustrating module parameters");

/* The parameters that can be passed in: how many times we say hello, and
 * to whom
 */

static char *whom  = "world";
static int howmany = 1;

module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

static int __init mod_par_init(void)
{
	int i;
	for (i = 0; i < howmany; i++)
		printk(KERN_ALERT "(%d) Hello, %s\n", i, whom);
	return 0;
}

static void __exit mod_par_exit(void)
{
	printk(KERN_ALERT "Huh, that's done with\n");
}

module_init(mod_par_init);
module_exit(mod_par_exit);
