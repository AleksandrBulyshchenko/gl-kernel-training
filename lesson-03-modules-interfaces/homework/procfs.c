#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MODULE_TAG                      "procfs_example: "
#define MODULE_PROCFS_DIR               "procfs_example"
#define MODULE_PROCFS_FILE              "buffer"
#define MODULE_PROCFS_FILE_SIZE         128

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Yaroslav Syrytsia <me@ys.lc>");
MODULE_DESCRIPTION("procfs example");
MODULE_VERSION("0.1");

static struct _procfs_context {
	struct proc_dir_entry *dir;
	struct proc_dir_entry *file;
	struct _procfs_buffer {
		char *data;
		size_t size;
		loff_t pos;
	} buffer;
} procfs_context = { };

static int procfs_file_read(struct file *file_p, char __user *buffer, size_t length, loff_t *offset)
{
	struct _procfs_buffer *buf = &procfs_context.buffer;
	size_t left;

	if (length > (buf->size - buf->pos))
		length = buf->size - buf->pos;

	left = copy_to_user(buffer, buf->data + buf->pos, length);
	buf->pos += length - left;

	if (left)
		pr_err(MODULE_TAG "failed to read %zu from %zu chars\n", left, length);
	else
		pr_info(MODULE_TAG "read %zu chars\n", length);

	return length - left;
}


static int procfs_file_write(struct file *file_p, const char __user *buffer, size_t length, loff_t *offset)
{
	struct _procfs_buffer *buf = &procfs_context.buffer;
	size_t msg_length = length;
	size_t left;

	if (msg_length > MODULE_PROCFS_FILE_SIZE) {
		pr_warn(MODULE_TAG "reduse message length from %zu to %zu chars\n", length, MODULE_PROCFS_FILE_SIZE);
		msg_length = MODULE_PROCFS_FILE_SIZE;
	}

	left = copy_from_user(buf->data, buffer, msg_length);

	buf->size = msg_length - left;
	buf->pos = 0;

	if (left)
		pr_err(MODULE_TAG "failed to write %zu from %zu chars\n", left, msg_length);
	else
		pr_info(MODULE_TAG "written %zu chars\n", msg_length);

	return length;
}


static const struct file_operations procfs_file_operations = {
	.owner = THIS_MODULE,
	.read  = procfs_file_read,
	.write = procfs_file_write,
};

static void procfs_context_free(void) {

	/* Cleanup the buffer */
	kfree(procfs_context.buffer.data);
	procfs_context.buffer.data = NULL;
	procfs_context.buffer.size = 0;
	procfs_context.buffer.pos = 0;

	/* proc related stuff */
	if (procfs_context.file) {
		remove_proc_entry(MODULE_PROCFS_FILE, procfs_context.dir);
		procfs_context.file = NULL;
	}

	if (procfs_context.dir) {
		remove_proc_entry(MODULE_PROCFS_DIR, NULL);
		procfs_context.dir = NULL;
	}
}

static int procfs_context_init(void)
{
	int err = 0;

	procfs_context.buffer.data = kmalloc(MODULE_PROCFS_FILE_SIZE, GFP_KERNEL);
	if (!procfs_context.buffer.data) {
		err = -ENOMEM;
		goto exit;
	}

	procfs_context.buffer.size = 0;
	procfs_context.buffer.pos = 0;

	procfs_context.dir = proc_mkdir(MODULE_PROCFS_DIR, NULL);
	if (!procfs_context.dir) {
		err = -EFAULT;
		goto exit_cleanup;
	}

	procfs_context.file = proc_create(MODULE_PROCFS_FILE, S_IFREG | S_IRUGO | S_IWUGO, procfs_context.dir, &procfs_file_operations);
	if (!procfs_context.file) {
		err = -EFAULT;
		goto exit_cleanup;
	}

	return 0;

exit_cleanup:
	procfs_context_free();
exit:
	return err;
}

static int __init procfs_init(void)
{
	int err;

	pr_info(MODULE_TAG "init ...");
	err = procfs_context_init();
	if (err == 0) {
		pr_info(MODULE_TAG "/proc/%s/%s created\n", MODULE_PROCFS_DIR, MODULE_PROCFS_FILE);
	}
	return err;
}

static void __exit procfs_exit(void)
{
	procfs_context_free();
	pr_info(MODULE_TAG "/proc/%s/%s deleted\n", MODULE_PROCFS_DIR, MODULE_PROCFS_FILE);
	pr_info(MODULE_TAG "exit");
}


module_init(procfs_init);
module_exit(procfs_exit);
