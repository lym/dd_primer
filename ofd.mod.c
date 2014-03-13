#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x25cfd397, "module_layout" },
	{ 0xc5e35f9c, "cdev_del" },
	{ 0xa08a0fd7, "device_destroy" },
	{ 0x89e07698, "cdev_add" },
	{ 0xe663280e, "cdev_init" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xc174d41e, "class_destroy" },
	{ 0xf00fe4d0, "device_create" },
	{ 0x34d03cd1, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x2f287f0d, "copy_to_user" },
	{ 0x362ef408, "_copy_from_user" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F30B0F731D29891E5652D9C");
