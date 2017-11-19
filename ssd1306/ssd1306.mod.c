#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0xbba89470, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xfed3a75b, __VMLINUX_SYMBOL_STR(i2c_del_driver) },
	{ 0xff5f331a, __VMLINUX_SYMBOL_STR(i2c_register_driver) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x6ce5cd72, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x33f4ad6a, __VMLINUX_SYMBOL_STR(i2c_smbus_write_byte_data) },
	{ 0xbac6c622, __VMLINUX_SYMBOL_STR(class_create_file_ns) },
	{ 0x5e106075, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x8d3060dd, __VMLINUX_SYMBOL_STR(devm_kmalloc) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0xa119dba9, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x916eb858, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x6d0fc796, __VMLINUX_SYMBOL_STR(class_remove_file_ns) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("i2c:lcd_ssd1306");
MODULE_ALIAS("of:N*T*CDAndy,lcd_ssd1306*");
