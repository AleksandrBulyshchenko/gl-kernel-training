
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Aleksandr Bulyshchenko <A.Bulyshchenko@globallogic.com>");
MODULE_DESCRIPTION("Example for procfs read/write");
MODULE_VERSION("0.1");


#define MODULE_TAG		"example_module "
#define PROC_DIRECTORY	"example"
#define PROC_FILENAME	"buffer"
#define BUFFER_SIZE		10


static char *proc_buffer;
static size_t proc_msg_length;
static size_t proc_msg_read_pos;

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

static int example_read(struct file *file_p, char __user *buffer,
						size_t length, loff_t *offset);
static int example_write(struct file *file_p, const char __user *buffer,
						 size_t length, loff_t *offset);

static const struct file_operations proc_fops = {
	.read  = example_read,
	.write = example_write,
};


static int create_buffer(void)
{
	proc_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (proc_buffer == NULL)
		return -ENOMEM;
	proc_msg_length = 0;

	return 0;
}


static void cleanup_buffer(void)
{
	if (proc_buffer) {
		kfree(proc_buffer);
		proc_buffer = NULL;
	}
	proc_msg_length = 0;
}


static int create_proc_example(void)
{
	proc_dir = proc_mkdir(PROC_DIRECTORY, NULL);
	if (proc_dir == NULL)
		return -EFAULT;

	proc_file = proc_create(PROC_FILENAME, S_IFREG | S_IRUGO | S_IWUGO,
							proc_dir, &proc_fops);
	if (proc_file == NULL)
		return -EFAULT;

	return 0;
}


static void cleanup_proc_example(void)
{
	if (proc_file) {
		remove_proc_entry(PROC_FILENAME, proc_dir);
		proc_file = NULL;
	}
	if (proc_dir) {
		remove_proc_entry(PROC_DIRECTORY, NULL);
		proc_dir = NULL;
	}
}


static int example_read(struct file *file_p, char __user *buffer,
						size_t length, loff_t *offset)
{
	size_t left;

	if (length > (proc_msg_length - proc_msg_read_pos))
		length = (proc_msg_length - proc_msg_read_pos);

	left = copy_to_user(buffer, &proc_buffer[proc_msg_read_pos], length);

	proc_msg_read_pos += length - left;

	if (left)
		pr_err(MODULE_TAG "failed to read %u from %u chars\n",
			   left, length);
	else
		pr_notice(MODULE_TAG "read %u chars\n", length);

	return length - left;
}


static int example_write(struct file *file_p, const char __user *buffer,
						 size_t length, loff_t *offset)
{
	size_t msg_length;
	size_t left;

	if (length > BUFFER_SIZE) {
		pr_warn(MODULE_TAG "reduse message length from %u to %u chars\n",
				length, BUFFER_SIZE);
		msg_length = BUFFER_SIZE;
	} else
		msg_length = length;

	left = copy_from_user(proc_buffer, buffer, msg_length);

	proc_msg_length = msg_length - left;
	proc_msg_read_pos = 0;

	if (left)
		pr_err(MODULE_TAG "failed to write %u from %u chars\n",
			   left, msg_length);
	else
		pr_notice(MODULE_TAG "written %u chars\n", msg_length);

	return length;
}


static int __init example_init(void)
{
	int err;

	err = create_buffer();
	if (err)
		goto error;

	err = create_proc_example();
	if (err)
		goto error;

	pr_notice(MODULE_TAG "loaded\n");
	return 0;

error:
	pr_err(MODULE_TAG "failed to load\n");
	cleanup_proc_example();
	cleanup_buffer();
	return err;
}


static void __exit example_exit(void)
{
	cleanup_proc_example();
	cleanup_buffer();
	pr_notice(MODULE_TAG "exited\n");
}


module_init(example_init);
module_exit(example_exit);
