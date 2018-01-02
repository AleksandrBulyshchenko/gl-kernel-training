#include "mpu6050_cdev.h"

static int open_cdev(struct inode *, struct file *);
static int release_cdev(struct inode *, struct file *);
static ssize_t read_line(struct file *, char __user *, size_t, loff_t *);
static ssize_t read_full(struct file *, char __user *, size_t, loff_t *);

static struct cdevs_holder g_cdevs_holder;

struct file_operations const g_fops_line = {
	.read = read_line,
	.open = open_cdev,
	.release = release_cdev,
	.owner = THIS_MODULE
};

struct file_operations const g_fops_full = {
	.read = read_full,
	.open = open_cdev,
	.release = release_cdev,
	.owner = THIS_MODULE
};

struct cdevs_holder *get_cdevs(void)
{
	return &g_cdevs_holder;
}

struct cdev_instance *get_cdev(dev_t dev_no)
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

void free_cdevs(struct cdevs_holder *cdevs)
{
	if (!cdevs)
		return;
	if (cdevs->cdev_line.cdev) {
		unregister_chrdev_region(cdevs->cdev_line.dev_no, 1);
		cdev_del(cdevs->cdev_line.cdev);
		cdevs->cdev_line.cdev = NULL;
	}
	if (cdevs->cdev_full.cdev) {
		unregister_chrdev_region(cdevs->cdev_full.dev_no, 1);
		cdev_del(cdevs->cdev_full.cdev);
		cdevs->cdev_full.cdev = NULL;
	}
	pr_info("%s %s unintialized majors %d/%d\n", THIS_MODULE->name,
		__func__, cdevs->cdev_line.major, cdevs->cdev_full.major);
}

int init_cdevs(struct cdevs_holder *cdevs, struct mpu6050_data_holder *data)
{
	int result = -1;

	if (!cdevs || !data)
		return result;

	result = alloc_chrdev_region(
		&cdevs->cdev_line.dev_no, 0, 1, "mpu6050_line");
	if (result < 0)
		goto error1;
	cdevs->cdev_line.major = MAJOR(cdevs->cdev_line.dev_no);
	cdevs->cdev_line.read_all = false;
	cdevs->cdev_line.cdev = cdev_alloc();
	cdevs->cdev_line.cdev->ops = &g_fops_line;
	cdevs->cdev_line.cdev->owner = THIS_MODULE;
	cdevs->cdev_line.first_element = true;
	result = cdev_add(cdevs->cdev_line.cdev, cdevs->cdev_line.dev_no, 1);
	if (result < 0)
		goto error1;

	result = alloc_chrdev_region(
		&cdevs->cdev_full.dev_no, 0, 1, "mpu6050_full");
	if (result < 0)
		goto error1;
	cdevs->cdev_full.major = MAJOR(cdevs->cdev_full.dev_no);
	cdevs->cdev_full.read_all = false;
	cdevs->cdev_full.cdev = cdev_alloc();
	cdevs->cdev_full.cdev->ops = &g_fops_full;
	cdevs->cdev_full.cdev->owner = THIS_MODULE;
	cdevs->cdev_full.first_element = true;
	result = cdev_add(cdevs->cdev_full.cdev, cdevs->cdev_full.dev_no, 1);
	if (result < 0)
		goto error1;

	cdevs->data = data;
	pr_info("%s %s intialized majors %d/%d\n", THIS_MODULE->name,
		__func__, cdevs->cdev_line.major, cdevs->cdev_full.major);
	return 0;

error1:
	free_cdevs(cdevs);
	return result;
}

static int open_cdev(struct inode *node, struct file *file)
{
	pr_info("%s %s node %p/%u\n",
		THIS_MODULE->name, __func__, node, node ? node->i_rdev : 0);
	if (node) {
		struct cdev_instance *cdev = get_cdev(node->i_rdev);

		if (cdev) {
			cdev->read_all = false;
			cdev->first_element = true;
		}
	}

	return 0;
}

static int release_cdev(struct inode *node, struct file *file)
{
	pr_info("%s %s node %p\n",
		THIS_MODULE->name, __func__, node);
	return 0;
}

static char const g_format_line[] = "%lu: gyro=%d:%d:%d acc=%d:%d:%d\n";
static size_t const g_size_line = sizeof(g_format_line) + 21 + 6 * 11;

static ssize_t read_to_user_buffer(struct mpu6050_data_elements *element,
	char __user *buffer_to, size_t count)
{
	ssize_t result = 0;
	char buffer[g_size_line];
	size_t length = 0;
	int printed = 0;

	if (!element || !buffer_to || !count)
		return result;

	printed = snprintf(buffer, g_size_line, g_format_line,
		(unsigned long)element->extra_data[EXTRA_INDEX_TIMESTAMP],
		element->data[INDEX_GYRO_X],
		element->data[INDEX_GYRO_Y],
		element->data[INDEX_GYRO_Z],
		element->data[INDEX_ACCEL_X],
		element->data[INDEX_ACCEL_Y],
		element->data[INDEX_ACCEL_Z]);
	length = printed > 0 && printed < g_size_line ? printed + 1 : 1;
	length = length < count ? length : count;
	buffer[length - 1] = '\0';
	result = copy_to_user(buffer_to, buffer, length);
	if (!result)
		result = length;
	else
		result = 0;

	return result;
}

static ssize_t read_line(
	struct file *file, char __user *buffer_to, size_t count, loff_t *off)
{
	unsigned long result = 0;
	struct cdevs_holder *holder = get_cdevs();
	struct cdev_instance *cdev;
	struct mpu6050_data_elements element;
	bool has_element;

	if (!holder || !holder->cdev_line.cdev
		|| !holder->data)
		return result;
	cdev = &holder->cdev_line;
	if (cdev->read_all)
		return result;

	has_element = get_active_element(holder->data, &element);
	if (!has_element)
		return result;

	result = read_to_user_buffer(&element, buffer_to, count);

	cdev->read_all = true;
	pr_info("%s %s read to %p/%lu from %lu buffer bytes -> %lu\n",
		THIS_MODULE->name, __func__,
		buffer_to, (long)count, (long)g_size_line, (long)result);
	return result;
}

static ssize_t read_full(
	struct file *file, char __user *buffer_to, size_t count, loff_t *off)
{
	unsigned long result = 0;
	struct cdevs_holder *holder = get_cdevs();
	struct cdev_instance *cdev;
	struct mpu6050_data_elements element;
	bool element_read = false;

	if (!holder || !holder->cdev_full.cdev
		|| !holder->data)
		return result;
	cdev = &holder->cdev_full;
	/* all data are read */
	if (cdev->read_all)
		return result;

	if (cdev->first_element)
		element_read = get_first_element(holder->data, &element);
	else
		element_read = get_next_element(holder->data, &element);

	pr_info("%s %s element read: %d\n",
		THIS_MODULE->name, __func__, element_read);

	if (element_read)
		cdev->first_element = false;
	else
		return result;

	result = read_to_user_buffer(&element, buffer_to, count);

	if (is_last_element(holder->data))
		cdev->read_all = true;

	pr_info("%s %s read to %p/%lu from %lu buffer bytes -> %lu, read_all: %s\n",
		THIS_MODULE->name, __func__,
		buffer_to, (long)count, (long)g_size_line, (long)result,
		cdev->read_all ? "yes" : "no");
	return result;
}
