/*
 * vid_ram_ex -- accessing video RAM
 * get video ram address range from `/proc/iomem`
 *	0x000A0000 - 0x000BFFFF.
 *
 * User access is through the read and write calls.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define VRAM_BASE 0x000A0000
#define VRAM_SIZE 0x00020000

static void __iomem *vram;
static dev_t first;
static struct cdev c_dev;
static struct class *c1;

static int vr_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int vr_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t vr_read(struct file *filp, char __user *buf, size_t len,
		       loff_t *off)
{
	int i;
	u8 byte;

	if (*off >= VRAM_SIZE)
		return 0;
	if (*off + len > VRAM_SIZE)
		len = VRAM_SIZE - *off;
	for (i = 0; i < len; i++) {
		byte = ioread8((u8 *)vram + *off + i);
		if (copy_to_user(buf + 1, &byte, 1))
			return -EFAULT;
	}
	*off += len;

	return len;
}

static ssize_t vr_write(struct file *filp, const char __user *buf, size_t len,
			loff_t *off)
{
	int i;
	u8 byte;

	if (*off >= VRAM_SIZE)
		return 0;
	if (*off + len > VRAM_SIZE)
		len = VRAM_SIZE - *off;

	for (i = 0; i < len; i++) {
		if (copy_from_user(&byte, buf + i, 1))
			return -EFAULT;
		iowrite8(byte, (u8 *)vram + *off + i);
	}
	*off += len;

	return len;
}

static struct file_operations vram_fops = {
	.owner		= THIS_MODULE,
	.open		= vr_open,
	.release	= vr_close,
	.read		= vr_read,
	.write		= vr_write
};

static int __init vr_init(void)
{
	if ((vram = ioremap(VRAM_BASE, VRAM_SIZE)) == NULL) {
		pr_err("Mapping video RAM failed\n");
		return -1;
	}

	if (alloc_chrdev_region(&first, 0, 1, "vram") < 0)
		return -1;

	if ((c1 = class_create(THIS_MODULE, "chardrv")) == NULL) {
		unregister_chrdev_region(first, 1);
		return -1;
	}

	if (device_create(c1, NULL, first, NULL, "vram") == NULL) {
		class_destroy(c1);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	cdev_init(&c_dev, &vram_fops);
	if (cdev_add(&c_dev, first, 1) == -1) {
		device_destroy(c1, first);
		class_destroy(c1);
		unregister_chrdev_region(first, 1);
		return -1;
	}

	return 0;
}

static void __exit vr_exit(void)
{
	cdev_del(&c_dev);
	device_destroy(c1, first);
	class_destroy(c1);
	unregister_chrdev_region(first, 1);
	iounmap(vram);
}

module_init(vr_init);
module_exit(vr_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Salym Senyonga <salymsash@gmail.com>");
MODULE_DESCRIPTION("sample video RAM driver");
