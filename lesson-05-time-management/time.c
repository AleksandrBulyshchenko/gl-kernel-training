#include <linux/module.h>
#include <linux/device.h>
#include <linux/time.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("Lesson 5: time management APIs");
MODULE_VERSION("0.1");

static struct class *time_management_class;

/**
 * Implement kernel module with API in sysfs or procfs, which returns time (in seconds) passed since previous read of it.
 */
static ssize_t prev_read_rel_time_show(struct class *class, struct class_attribute *attr, char *buf)
{
	static u64 last_read_jiffies = 0;
	u64 time_passed = 0;

	/* using do_gettimeofday() and then timespec_sub() could give more precise results */
	if (0 != last_read_jiffies) {
		time_passed = get_jiffies_64() - last_read_jiffies;
		do_div(time_passed, HZ);
	}

	last_read_jiffies = get_jiffies_64();

	return scnprintf(buf, PAGE_SIZE, "%llus\n", time_passed);
}
CLASS_ATTR_RO(prev_read_rel_time);

/**
 * Try to implement kernel module with API in sysfs or procfs, which returns absolute time of previous reading.
 */
static ssize_t prev_read_abs_time_show(struct class *class, struct class_attribute *attr, char *buf) {
	static struct timeval prev_read_time = {0,};
	struct tm split_time;
	int ret;

	time_to_tm(prev_read_time.tv_sec, 0, &split_time);
	ret = scnprintf(buf, PAGE_SIZE, "%02d.%02d.%04ld %02d:%02d:%02d UTC\n",
	  split_time.tm_mday,
	  split_time.tm_mon + 1,
	  split_time.tm_year + 1900,
	  split_time.tm_hour,
	  split_time.tm_min,
	  split_time.tm_sec);

	do_gettimeofday(&prev_read_time);

	return ret;
}
CLASS_ATTR_RO(prev_read_abs_time);

/**
 * Implement kernel module with API in sysfs or procfs, which return Fibonacci sequence. Each element of sequence should be generated once in a second.
 */
struct fibonacci_seq {
	u64 prev_number;
	u64 curr_number;
	unsigned int seq_number;

	spinlock_t        spin_lock;
	struct timer_list timer;
};
struct fibonacci_seq fibonacci_seq;

static void fibonacci_next(struct fibonacci_seq *seq) {
	u64 prev;
	unsigned long flags;

	BUG_ON(NULL == seq);

	spin_lock_irqsave(&seq->spin_lock, flags);

	prev = seq->curr_number;
	seq->curr_number = seq->curr_number + seq->prev_number;
	seq->prev_number = prev;
	seq->seq_number++;

	spin_unlock_irqrestore(&seq->spin_lock, flags);
}

static u64 fibonacci_get_current(struct fibonacci_seq *seq, unsigned int *seq_number) {
	unsigned int ret;
	unsigned long flags;

	spin_lock_irqsave(&seq->spin_lock, flags);

	ret = seq->curr_number;
	if (seq_number) {
		*seq_number = seq->seq_number;
	}

	spin_unlock_irqrestore(&seq->spin_lock, flags);

	return ret;
}

#define TIMER_PERIOD_JIFFIES (1 * HZ) /* 1 sec */

static void fibonacci_timer_callback(unsigned long data) {
	struct fibonacci_seq *seq = (struct fibonacci_seq *)data;

	fibonacci_next(seq);
	mod_timer(&seq->timer, get_jiffies_64() + (TIMER_PERIOD_JIFFIES));
}

static void fibonacci_timer_init(struct fibonacci_seq *seq) {
	seq->prev_number = 0;
	seq->curr_number = 1;
	seq->seq_number  = 0;
	spin_lock_init(&seq->spin_lock);

	init_timer(&seq->timer);
	setup_timer(&seq->timer, fibonacci_timer_callback, (unsigned int)seq);
	mod_timer(&seq->timer, get_jiffies_64() + TIMER_PERIOD_JIFFIES);
}

static void fibonacci_timer_destroy(struct fibonacci_seq *seq) {
	del_timer(&seq->timer);
}

static ssize_t fibonacci_seq_show(struct class *class, struct class_attribute *attr, char *buf) {
	u64 current_fibonacci_number;
	unsigned int current_seq_number;

	current_fibonacci_number = fibonacci_get_current(&fibonacci_seq, &current_seq_number);

	return scnprintf(buf, PAGE_SIZE, "seq: %u, value: %llu\n", current_seq_number, current_fibonacci_number);
}
CLASS_ATTR_RO(fibonacci_seq);

static int __init time_init(void) {
	int ret;

	time_management_class = class_create(THIS_MODULE, "time-management");
	if(IS_ERR(time_management_class)) {
		printk(KERN_ERR "%s: bad class create\n", THIS_MODULE->name);
		return -1;
	}

	ret = class_create_file(time_management_class, &class_attr_prev_read_rel_time);
	if (0 != ret)
		goto err_rel_time;

	ret = class_create_file(time_management_class, &class_attr_prev_read_abs_time);
	if (0 != ret)
		goto err_abs_time;

	ret = class_create_file(time_management_class, &class_attr_fibonacci_seq);
	if (0 != ret)
		goto err_fibonacci;

	fibonacci_timer_init(&fibonacci_seq);

	printk(KERN_INFO "%s: module initialized\n", THIS_MODULE->name);

	return ret;

err_fibonacci:
	class_remove_file(time_management_class, &class_attr_prev_read_abs_time);
err_abs_time:
	class_remove_file(time_management_class, &class_attr_prev_read_rel_time);
err_rel_time:
	class_destroy(time_management_class);
	return ret;
}

static void __exit time_exit(void) {
	fibonacci_timer_destroy(&fibonacci_seq);
	class_remove_file(time_management_class, &class_attr_fibonacci_seq);
	class_remove_file(time_management_class, &class_attr_prev_read_abs_time);
	class_remove_file(time_management_class, &class_attr_prev_read_rel_time);
	class_destroy(time_management_class);
	printk(KERN_INFO "%s: module done\n", THIS_MODULE->name);
}

module_init(time_init);
module_exit(time_exit);
