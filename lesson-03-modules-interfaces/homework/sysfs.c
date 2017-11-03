#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/device.h>

#define MODULE_TAG                     "sysfs_example: "
#define MODULE_SYSFS_DIR               "sysfs_example"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Yaroslav Syrytsia <me@ys.lc>");
MODULE_DESCRIPTION("sysfs example");
MODULE_VERSION("0.1");

static struct kobject *sysfs_kobj;
static char sysfs_example_buffer[16];
static u32 sysfs_example_size = 0;

static ssize_t example_show(struct class *class, struct class_attribute *attr, char *buf)
{
	strncpy(buf, sysfs_example_buffer, sysfs_example_size);
	return sysfs_example_size;
}

static ssize_t example_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	sysfs_example_size = min(count, sizeof(sysfs_example_buffer));
	strncpy(sysfs_example_buffer, buf, sysfs_example_size);
	return count;
}

static CLASS_ATTR_RW(example);

static struct attribute *sysfs_attrs[] = {
	&class_attr_example.attr,
	NULL,
};

static struct attribute_group sysfs_attr_group = {
	.attrs = sysfs_attrs,
};


static int sysfs_module_init(void)
{
	int err;

	pr_info(MODULE_TAG "init ...\n");

	sysfs_kobj = kobject_create_and_add(MODULE_SYSFS_DIR, kernel_kobj);
	if (!sysfs_kobj)
		return -ENOMEM;

	err = sysfs_create_group(sysfs_kobj, &sysfs_attr_group);
	if (err)
		kobject_put(sysfs_kobj);
	else
		pr_info(MODULE_TAG "/sys/kernel/%s created\n", MODULE_SYSFS_DIR);
	return err;
}

static void sysfs_module_exit(void)
{
	kobject_put(sysfs_kobj);
	pr_info(MODULE_TAG "/sys/kernel/%s deleted\n", MODULE_SYSFS_DIR);
	pr_info(MODULE_TAG "exit\n");
}

module_init(sysfs_module_init);
module_exit(sysfs_module_exit);
