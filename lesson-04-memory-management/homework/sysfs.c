#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/slab_def.h>

#define LEN_MSG 160
static char static_buf_msg[ LEN_MSG + 1 ] = "Hello from module!\n";

static struct _sysfs_context {
	struct class *root_class;
	struct _buffer {
		char *kmem;
		char *kmalloc;
	} buffer;
	struct kmem_cache *kmem_cache;
} sysfs_context = { };

static ssize_t static_show(struct class *class, struct class_attribute *attr, char *buf)
{
	strcpy(buf, static_buf_msg);
	pr_info("read %ld\n", (long)strlen(buf));
	return strlen(buf);
}

static ssize_t static_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	pr_info("write %ld\n", (long)count);
	strncpy(static_buf_msg, buf, count);
	static_buf_msg[ count ] = '\0';
	return count;
}

static ssize_t kmalloc_show(struct class *class, struct class_attribute *attr, char *buf)
{
	const char *ptr = "";

	if (sysfs_context.buffer.kmalloc) {
		ptr = sysfs_context.buffer.kmalloc;
	}

	strcpy(buf, ptr);
	pr_info("read %ld\n", (long)strlen(buf));
	return strlen(buf);
}

static ssize_t kmalloc_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	size_t len = min(count, (size_t)LEN_MSG); /* make GCC happy */

	if (!sysfs_context.buffer.kmalloc) {
		sysfs_context.buffer.kmalloc = kmalloc(LEN_MSG, GFP_KERNEL | __GFP_NOFAIL);
	}

	strlcpy(sysfs_context.buffer.kmalloc, buf, len);
	return count;
}

static ssize_t kmem_show(struct class *class, struct class_attribute *attr, char *buf)
{
	const char *ptr = "";

	if (sysfs_context.buffer.kmem) {
		ptr = sysfs_context.buffer.kmem;
	}

	strcpy(buf, ptr);
	pr_info("read %ld\n", (long)strlen(buf));
	return strlen(buf);
}

static ssize_t kmem_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	size_t len = min(count, (size_t)LEN_MSG); /* make GCC happy */

	if (!sysfs_context.buffer.kmem) {
		sysfs_context.buffer.kmem = kmem_cache_alloc(sysfs_context.kmem_cache, GFP_KERNEL | __GFP_NOFAIL);
	}

	strlcpy(sysfs_context.buffer.kmem, buf, len);
	return count;
}

/* Class attributes */
static CLASS_ATTR_RW(static);
static CLASS_ATTR_RW(kmalloc);
static CLASS_ATTR_RW(kmem);

static void sysfs_context_destroy(void)
{
	if (sysfs_context.root_class)
		class_destroy(sysfs_context.root_class);

	if (sysfs_context.buffer.kmalloc)
		kfree(sysfs_context.buffer.kmalloc);

	if (sysfs_context.kmem_cache) {
		if (sysfs_context.buffer.kmem)
			kmem_cache_free(sysfs_context.kmem_cache, sysfs_context.buffer.kmem);
		kmem_cache_destroy(sysfs_context.kmem_cache);
	}
}

int __init root_class_init(void)
{
	int res;

	sysfs_context.root_class = class_create(THIS_MODULE, "x-class");
	if (IS_ERR(sysfs_context.root_class)) {
		pr_err("bad class create\n");
		goto exit;
	}

	sysfs_context.kmem_cache = kmem_cache_create("x-class-kmem", LEN_MSG, 0, 0, NULL);
	if (IS_ERR(sysfs_context.kmem_cache)) {
		pr_err("bad kmem_cache_create\n");
		goto exit;
	}

	res = class_create_file(sysfs_context.root_class, &class_attr_static);
	res = class_create_file(sysfs_context.root_class, &class_attr_kmalloc);
	res = class_create_file(sysfs_context.root_class, &class_attr_kmem);
	pr_info("'xxx' module initialized\n");
	return res;

exit:
	sysfs_context_destroy();
	return -ENOMEM;
}

void root_class_cleanup(void)
{
	class_remove_file(sysfs_context.root_class, &class_attr_static);
	class_remove_file(sysfs_context.root_class, &class_attr_kmalloc);
	class_remove_file(sysfs_context.root_class, &class_attr_kmem);
	sysfs_context_destroy();
	return;
}

module_init(root_class_init);
module_exit(root_class_cleanup);
MODULE_LICENSE("GPL");
