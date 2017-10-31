#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>

#define LEN_MSG 160
static char buf_msg[LEN_MSG + 1] = "Hello from module!\n";


static ssize_t xxx_show(struct class *class, struct class_attribute *attr,
						char *buf)
{
	strcpy(buf, buf_msg);
	pr_info("read %ld\n", (long)strlen(buf));
	return strlen(buf);
}

static ssize_t xxx_store(struct class *class, struct class_attribute *attr,
						 const char *buf, size_t count)
{
	pr_info("write %ld\n", (long)count);
	strncpy(buf_msg, buf, count);
	buf_msg[count] = '\0';
	return count;
}

CLASS_ATTR_RW(xxx);

static struct class *x_class;

int __init x_init(void)
{
	int res;

	x_class = class_create(THIS_MODULE, "x-class");
	if (IS_ERR(x_class))
		pr_info("bad class create\n");
	res = class_create_file(x_class, &class_attr_xxx);
	pr_info("'xxx' module initialized\n");
	return res;
}

void x_cleanup(void)
{
	class_remove_file(x_class, &class_attr_xxx);
	class_destroy(x_class);
}

module_init(x_init);
module_exit(x_cleanup);

MODULE_LICENSE("GPL");
