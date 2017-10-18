#include <linux/module.h>
#include <storage.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Bulyshchenko <A.Bulyshchenko@globallogic.com>");
MODULE_DESCRIPTION("Shared variable exporter module");
MODULE_VERSION("0.1");

static int __init exporter_init(void) {
	printk("[%s]: Hello, I export shared_data (0x%X).\n",
		THIS_MODULE->name, shared_data);
	return 0;
}

static void __exit exporter_exit(void) {
	printk("[%s]: Goodbye, shared_data was 0x%X.\n",
		THIS_MODULE->name, shared_data);
}

module_init(exporter_init);
module_exit(exporter_exit);
