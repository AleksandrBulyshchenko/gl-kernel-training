#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/timekeeping.h>

static struct _sysfs_context {
	struct class *root_class;
	unsigned long time_passed_seconds;
	struct timespec64 time_prev_abs;
	u32 fib_max;
} sysfs_context = {
	.fib_max = 10,
	.time_prev_abs = {
		.tv_sec = -1,
	},
};

static u32 fibonacci(u32 n)
{
	switch (n) {
		case 0:
		case 1:
			return n;
		default:
			return fibonacci(n-1) + fibonacci(n-2);
	}
}

static ssize_t time_passed_show(struct class *class, struct class_attribute *attr, char *buf)
{
	unsigned long result = 0;
	unsigned long now = get_seconds();

	if (sysfs_context.time_passed_seconds) {
		result = now - sysfs_context.time_passed_seconds;
	}

	sysfs_context.time_passed_seconds = now;
	return scnprintf(buf, PAGE_SIZE, "%lu\n", result);
}

static ssize_t time_prev_absolute_show(struct class *class, struct class_attribute *attr, char *buf)
{
	struct timespec64 *prev = &sysfs_context.time_prev_abs;
	struct timespec64 now;
	struct tm tm;
	ssize_t len = 0;

	now = current_kernel_time64();
	if (timespec64_valid(prev)) {
		time64_to_tm(prev->tv_sec, 0, &tm);
		len = scnprintf(buf, PAGE_SIZE,  "%04ld-%02d-%02d %02d:%02d:%02d\n",
			/* Date */
			 tm.tm_year + 1900,
			 tm.tm_mon + 1,
			 tm.tm_mday,
			 /* Time */
			 tm.tm_hour,
			 tm.tm_min,
			 tm.tm_sec);
	}
	else {
		len = scnprintf(buf, PAGE_SIZE, "0\n");
	}

	*prev = now;
	return len;
}

static ssize_t time_fib_show(struct class *class, struct class_attribute *attr, char *buf)
{
	u32 n;
	int b = 0;

	pr_info("xxx: fibonacci: %u\n", sysfs_context.fib_max);
	for (n = 0; n < sysfs_context.fib_max; n++) {
		b += scnprintf(buf + b, PAGE_SIZE - b, "%u ", fibonacci(n));
		schedule_timeout_killable(HZ);
	}

	b += scnprintf(buf + b, PAGE_SIZE - b, "\n");
	return b;
}

static ssize_t time_fib_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	const u32 fib_limit = 32;
	u32 new = 0;
	int r;

	r = kstrtouint(buf, 10, &new);
	if (r)
		return r;

	sysfs_context.fib_max = min(fib_limit, new);
	pr_info("xxx: new fibonacci limit: %u\n", sysfs_context.fib_max);
	return count;
}

/* Class attributes */
static CLASS_ATTR_RO(time_passed);
static CLASS_ATTR_RO(time_prev_absolute);
static CLASS_ATTR_RW(time_fib);

static void sysfs_context_destroy(void)
{
	if (sysfs_context.root_class)
		class_destroy(sysfs_context.root_class);
}

int __init root_class_init(void)
{
	int res;

	sysfs_context.root_class = class_create(THIS_MODULE, "x-class");
	if (IS_ERR(sysfs_context.root_class)) {
		pr_err("bad class create\n");
		goto exit;
	}

	res = class_create_file(sysfs_context.root_class, &class_attr_time_passed);
	res = class_create_file(sysfs_context.root_class, &class_attr_time_prev_absolute);
	res = class_create_file(sysfs_context.root_class, &class_attr_time_fib);
	pr_info("'xxx' module initialized\n");
	return res;

exit:
	sysfs_context_destroy();
	return -ENOMEM;
}

void root_class_cleanup(void)
{
	class_remove_file(sysfs_context.root_class, &class_attr_time_passed);
	class_remove_file(sysfs_context.root_class, &class_attr_time_prev_absolute);
	class_remove_file(sysfs_context.root_class, &class_attr_time_fib);
	sysfs_context_destroy();
	return;
}

module_init(root_class_init);
module_exit(root_class_cleanup);
MODULE_LICENSE("GPL");
