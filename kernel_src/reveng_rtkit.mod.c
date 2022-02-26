#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xeeab4c1e, "module_layout" },
	{ 0x3c03d27a, "cdev_del" },
	{ 0xc498609c, "device_destroy" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x7c038979, "class_destroy" },
	{ 0xba9cf391, "device_create" },
	{ 0xbd59df1c, "__class_create" },
	{ 0x44d7ad45, "cdev_add" },
	{ 0xdc74c97c, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x1a81ff9c, "pv_ops" },
	{ 0x593c1bac, "__x86_indirect_thunk_rbx" },
	{ 0x7f7b1cfd, "unregister_kprobe" },
	{ 0x9d447e54, "register_kprobe" },
	{ 0xb0e602eb, "memmove" },
	{ 0x37a0cba, "kfree" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x20000329, "simple_strtoul" },
	{ 0x4d497d69, "current_task" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x6b8206ec, "module_put" },
	{ 0x25461b72, "try_module_get" },
	{ 0x5a921311, "strncmp" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x35dc3d42, "commit_creds" },
	{ 0x80c1c04b, "prepare_creds" },
	{ 0x6d289335, "init_task" },
	{ 0x2ea2c95c, "__x86_indirect_thunk_rax" },
	{ 0xc5850110, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "7AF9CD433DA298ABD62AB15");
