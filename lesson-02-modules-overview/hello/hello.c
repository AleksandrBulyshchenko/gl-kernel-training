#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Bulyshchenko <A.Bulyshchenko@globallogic.com>");
MODULE_DESCRIPTION("HelloWorld Linux module");
MODULE_VERSION("0.1");

static int __init hello_init(void)
{
	printk("Hello, world!\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("Goodbye, world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
