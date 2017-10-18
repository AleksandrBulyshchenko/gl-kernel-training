#include <linux/module.h>
#include <storage.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandr Bulyshchenko <A.Bulyshchenko@globallogic.com>");
MODULE_DESCRIPTION("Shared variable importer module");
MODULE_VERSION("0.1");

static int __init importer_init(void) {
	printk("[%s]: Hello, I'm importing shared_data (0x%X)\n",
		THIS_MODULE->name, shared_data);
	return 0;
}

static void __exit importer_exit(void) {
	shared_data++;
	printk("[%s]: Goodbye, shared_data is 0x%X now.\n",
		THIS_MODULE->name, shared_data);
}

module_init(importer_init);
module_exit(importer_exit);
