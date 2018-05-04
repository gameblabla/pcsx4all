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
	{ 0x5990ffc, "module_layout" },
	{ 0xa17f8558, "dma_free_coherent" },
	{ 0x60247532, "__iounmap" },
	{ 0x345fd387, "dma_alloc_coherent" },
	{ 0xacf2df89, "__ioremap" },
	{ 0xb6091ec0, "__copy_user" },
	{ 0x7ca341af, "kernel_thread" },
	{ 0x2bc95bd4, "memset" },
	{ 0x28e1a108, "cdev_add" },
	{ 0x6669bd04, "cdev_init" },
	{ 0x88b719c6, "device_create" },
	{ 0x758f1847, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xf9a482f9, "msleep" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x4d3430e2, "class_destroy" },
	{ 0x6d81f5f5, "cdev_del" },
	{ 0x398047d9, "device_destroy" },
	{ 0xea147363, "printk" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "7374A566D8868B84E46768F");
