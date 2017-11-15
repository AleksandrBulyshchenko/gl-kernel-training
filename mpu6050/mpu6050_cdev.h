#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include "mpu6050_data.h"


struct cdev_instance {
	unsigned int major;
	dev_t dev_no;
	dev_t read_all;
	struct cdev *cdev;
};

struct cdevs_holder {
	struct cdev_instance cdev_line;
	struct cdev_instance cdev_full;
	struct mpu6050_data_holder *data;
};

static int open_cdev(struct inode *, struct file *);
static int release_cdev(struct inode *, struct file *);
static ssize_t read_line(struct file *, char __user *, size_t, loff_t *);
static ssize_t read_full(struct file *, char __user *, size_t, loff_t *);

struct file_operations g_fops_line = {
	read: read_line,
	open: open_cdev,
	release: release_cdev,
	owner: THIS_MODULE
};

struct file_operations g_fops_full = {
	read: read_full,
	open: open_cdev,
	release: release_cdev,
	owner: THIS_MODULE
};

static struct cdevs_holder *get_cdevs(void)
{
	static struct cdevs_holder g_cdevs_holder;
	return &g_cdevs_holder;
}

static struct cdev_instance *get_cdev(dev_t dev_no)
{
	struct cdevs_holder *cdevs = get_cdevs();
	struct cdev_instance *result = NULL;
	if (!cdevs)
		return result;
	if (cdevs->cdev_line.dev_no == dev_no)
		result = &cdevs->cdev_line;
	else if (cdevs->cdev_full.dev_no == dev_no)
		result = &cdevs->cdev_full;
	return result;
}

static void free_cdevs(struct cdevs_holder *cdevs)
{
	if (!cdevs)
		return;
	if (cdevs->cdev_line.cdev)
	{
		unregister_chrdev_region(cdevs->cdev_line.dev_no, 1);
		cdev_del(cdevs->cdev_line.cdev);
		cdevs->cdev_line.cdev = NULL;
	}
	if (cdevs->cdev_full.cdev)
	{
		unregister_chrdev_region(cdevs->cdev_full.dev_no, 1);
		cdev_del(cdevs->cdev_full.cdev);
		cdevs->cdev_full.cdev = NULL;
	}
	pr_info("%s %s unintialized majors %d/%d \n", THIS_MODULE->name,
		__FUNCTION__, cdevs->cdev_line.major, cdevs->cdev_full.major);
}

static int init_cdevs(struct cdevs_holder *cdevs, struct mpu6050_data_holder *data)
{
	int result = -1;
	if (!cdevs || !data)
		return result;

	result = alloc_chrdev_region(&cdevs->cdev_line.dev_no, 0, 1, "mpu6050_line");
	if (result < 0)
		goto error1;
	cdevs->cdev_line.major = MAJOR(cdevs->cdev_line.dev_no);
	cdevs->cdev_line.read_all = false;
	//cdevs->cdev_line.dev = MKDEV(cdevs->cdev_line.major, 0);
	cdevs->cdev_line.cdev = cdev_alloc();
	cdevs->cdev_line.cdev->ops = &g_fops_line;
	cdevs->cdev_line.cdev->owner = THIS_MODULE;
	result = cdev_add(cdevs->cdev_line.cdev, cdevs->cdev_line.dev_no, 1);
	if (result < 0)
		goto error1;

	result = alloc_chrdev_region(&cdevs->cdev_full.dev_no, 0, 1, "mpu6050_full");
	if (result < 0)
		goto error1;
	cdevs->cdev_full.major = MAJOR(cdevs->cdev_full.dev_no);
	cdevs->cdev_line.read_all = false;
	//cdevs->cdev_full.dev = MKDEV(cdevs->cdev_full.major, 0);
	cdevs->cdev_full.cdev = cdev_alloc();
	cdevs->cdev_full.cdev->ops = &g_fops_full;
	cdevs->cdev_full.cdev->owner = THIS_MODULE;
	result = cdev_add(cdevs->cdev_full.cdev, cdevs->cdev_full.dev_no, 1);
	if (result < 0)
		goto error1;

	cdevs->data = data;
	pr_info("%s %s intialized majors %d/%d \n", THIS_MODULE->name, __FUNCTION__,
		cdevs->cdev_line.major, cdevs->cdev_full.major);
	return 0;

error1:
	free_cdevs(cdevs);
	return result;
}


static int open_cdev(struct inode *node, struct file *file)
{
	pr_info("%s %s node %p/%u\n",
		THIS_MODULE->name, __FUNCTION__, node, node ? node->i_rdev : 0);
	if (node) {
		struct cdev_instance *cdev = get_cdev(node->i_rdev);
		if (cdev)
			cdev->read_all = false;
	}

	return 0;
}

static int release_cdev(struct inode *node, struct file *file)
{
	pr_info("%s %s node %p\n",
		THIS_MODULE->name, __FUNCTION__, node);
	return 0;
}

static char const format_line[] = "%lu: gyro=%d:%d:%d acc=%d:%d:%d\n";
static size_t const size_line = sizeof(format_line) + 21 + 6 * 11;

static ssize_t read_line(struct file *file, char __user *buffer_to, size_t count, loff_t *off)
{
	unsigned long result = 0;
	struct cdevs_holder *holder = get_cdevs();
	struct cdev_instance *cdev;
	if (!holder && !holder->cdev_line.cdev
		&& !holder->data && !holder->data->read_data)
		return result;
	cdev = &holder->cdev_line;
	if (cdev->read_all)
		return result;

	holder->data->read_data(true);
	if (!holder->data->element_iter_current)
		return result;

	if (count >= size_line) {
		char buffer[size_line];
		size_t length = 1;
		struct mpu6050_data_elements *element =
			&holder->data->element_iter_current->data;
		int printed = snprintf(buffer, size_line, format_line,
			(unsigned long)element->extra_data[INDEX_TIMESTAMP],
			element->data[INDEX_GYRO_X], element->data[INDEX_GYRO_Y],
			element->data[INDEX_GYRO_Z], element->data[INDEX_ACCEL_X],
			element->data[INDEX_ACCEL_Y], element->data[INDEX_ACCEL_Z]);
		length = printed + 1;
		length = length < size_line ? length : size_line;
		buffer[length - 1] = '\0';
		result = copy_to_user(buffer_to, buffer, length);
		if (!result)
			result = length;
		cdev->read_all = true;
	}
	else {
		//TODO: implement dyn-buffer
	}
	pr_info("%s %s read to %p/%lu from %lu buffer bytes -> %lu \n",
		THIS_MODULE->name, __FUNCTION__,
		buffer_to, (long)count, (long)size_line, (long)result);
	return result;
}

static ssize_t read_full(struct file *file, char __user *buffer_to, size_t count, loff_t *off)
{
	return 0;
}
