#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include "mpu6050-regs.h"
#include "mpu6050.h"
#include "mpu6050_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksii Kogutenko");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel training: 07 - MPU6050 mudule + cdev");

#define DBG(x)
#define DBG1(x)
#define DBG2(x)

// ----------------------------------------------------------------------------
static struct mpu6050_data g_mpu6050_data;
static struct mpu6050_self_test_data g_mpu6050_self_test_data;
// ----------------------------------------------------------------------------
static const struct i2c_device_id mpu6050_idtable[] = {
	{ "mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);
// ----------------------------------------------------------------------------

CLASS_DATA_RO(mpu6050, accel_x, mpu6050_show);
CLASS_DATA_RO(mpu6050, accel_y, mpu6050_show);
CLASS_DATA_RO(mpu6050, accel_z, mpu6050_show);
CLASS_DATA_RO(mpu6050, gyro_x, mpu6050_show);
CLASS_DATA_RO(mpu6050, gyro_y, mpu6050_show);
CLASS_DATA_RO(mpu6050, gyro_z, mpu6050_show);
CLASS_DATA_RO(mpu6050, temperature, mpu6050_show);
CLASS_DATA_RO(mpu6050, all, mpu6050_show);
CLASS_DATA_RO(mpu6050, self_test, mpu6050_show);

ATTR_GROUP(mpu6050,
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_x),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_y),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_z),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_x),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_y),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_z),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, temperature),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, all),
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, self_test),
		NULL
	)))))))))
)

ATTR_GROUPS(mpu6050, ATTR_GET_GROUP(mpu6050))
// ----------------------------------------------------------------------------
struct class mpu6050_class = {
	.name			= "mpu6050",
	.class_groups	= ATTR_GET_GROUPS(mpu6050),
	.owner			= THIS_MODULE,
};
// ----------------------------------------------------------------------------
static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "mpu6050",
	},

	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};
// ----------------------------------------------------------------------------
static struct {
	struct timer_list	timer;
	spinlock_t			lock;
} timer;
// ----------------------------------------------------------------------------
static struct cdev mpu6050_dev;
static const struct file_operations mpu6050_dev_fops = {
	.owner = THIS_MODULE,
	.read  = mpu6050_dev_read,
    .unlocked_ioctl = mpu6050_dev_ioctl,
    .compat_ioctl = mpu6050_dev_ioctl
};
static int major;
module_param(major, int, 0444);
struct task_struct *p_mpu6050_thread;
DECLARE_COMPLETION(comp_one_sec);
/* ----------------------------------------------------------------------------
 * timer callback
 * ------------------------------------------------------------------------- */
void timer_callback(unsigned long data)
{
	unsigned long flags = 0;
	u64 jiff, diff;

	spin_lock_irqsave(&timer.lock, flags);

	jiff = get_jiffies_64();
	diff = (jiff - timer.timer.expires) + TIMER_PERIOD + jiff;

	mod_timer(&timer.timer, diff);

	spin_unlock_irqrestore(&timer.lock, flags);
	complete(&comp_one_sec);
}
/* ----------------------------------------------------------------------------
 * init_driver_timer
 * ------------------------------------------------------------------------- */
static int init_driver_timer(void)
{
	int ret = 0;

	spin_lock_init(&timer.lock);
	init_timer(&timer.timer);
	setup_timer(&timer.timer, timer_callback, 0);
	ret = mod_timer(&timer.timer, get_jiffies_64() + TIMER_PERIOD);

	return ret;
}
/* ---------------------------------------------------------------------------
 * deinit_driver_timer
 * ------------------------------------------------------------------------ */
static void deinit_driver_timer(void)
{
	del_timer(&timer.timer);
}
/* ---------------------------------------------------------------------------
 * mpu6050_read_data
 * ------------------------------------------------------------------------ */
#define I2C_READ(client, addr) \
	(s16)((u16)i2c_smbus_read_word_swapped(client, addr))
static int mpu6050_read_data(void)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	struct i2c_client *drv_client = g_mpu6050_data.drv_client;
	int counter;

	if (drv_client == 0)
		return -ENODEV;

	data[0] = I2C_READ(drv_client, REG_ACCEL_XOUT_H);
	data[1] = I2C_READ(drv_client, REG_ACCEL_YOUT_H);
	data[2] = I2C_READ(drv_client, REG_ACCEL_ZOUT_H);
	data[3] = I2C_READ(drv_client, REG_GYRO_XOUT_H);
	data[4] = I2C_READ(drv_client, REG_GYRO_YOUT_H);
	data[5] = I2C_READ(drv_client, REG_GYRO_ZOUT_H);
	data[6] = I2C_READ(drv_client, REG_TEMP_OUT_H);
	/* Temperature in degrees C =
	 * (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53
	 */
	data[6] = (data[6] + 12420 + 170) / 340;

	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(g_mpu6050_data.data, data, sizeof(data));
	counter = g_mpu6050_data.counter++;
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	//dev_info(&drv_client->dev, "sensor data read %d:\n", counter);
	return 0;
}
/* ---------------------------------------------------------------------------
 * mpu6050_self_test
 * ------------------------------------------------------------------------ */
static int mpu6050_self_test(struct i2c_client *drv_client)
{
	u8 r, ra;
	/* Set self test mode */
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0xE0 | (MPU6050_GFS_SEL << 3));
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0xE0 | (MPU6050_AFS_SEL << 3));
	msleep(100); //sleep 100ms

	/* read values in test mode*/
	mpu6050_read_data();
	memcpy(g_mpu6050_self_test_data.test_value.data, g_mpu6050_data.data, 
			sizeof(g_mpu6050_self_test_data.test_value.data));

	/* read Registers 13 to 16 – Self Test Registers */
	ra = (u8)i2c_smbus_read_byte_data(drv_client, REG_ST_A);

	r  = (u8)i2c_smbus_read_byte_data(drv_client, REG_ST_X);
	g_mpu6050_self_test_data.self_test.accel_values[0] = ((r >> 3) & 0x01c) | ((ra >> 4) & 0x03);
	g_mpu6050_self_test_data.self_test.gyro_values[0]  = (r & 0x01F);

	r  = (u8)i2c_smbus_read_byte_data(drv_client, REG_ST_Y);
	g_mpu6050_self_test_data.self_test.accel_values[1] = ((r >> 3) & 0x01c) | ((ra >> 2) & 0x03);
	g_mpu6050_self_test_data.self_test.gyro_values[1]  = (r & 0x01F);

	r  = (u8)i2c_smbus_read_byte_data(drv_client, REG_ST_Z);
	g_mpu6050_self_test_data.self_test.accel_values[2] = ((r >> 3) & 0x01c) | ((ra >> 0) & 0x03);
	g_mpu6050_self_test_data.self_test.gyro_values[2]  = (r & 0x01F);

	/* Set normal mode */
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0x00 | (MPU6050_GFS_SEL << 3));
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0x00 | (MPU6050_AFS_SEL << 3));
	msleep(100); //sleep 100ms

	/* read values in normal mode */
	mpu6050_read_data();
	memcpy(g_mpu6050_self_test_data.value.data, g_mpu6050_data.data, 
			sizeof(g_mpu6050_self_test_data.test_value.data));
	return 0;
}
/* ---------------------------------------------------------------------------
 * mpu6050_probe
 * ------------------------------------------------------------------------ */
static int mpu6050_probe(struct i2c_client *drv_client,
			 const struct i2c_device_id *id)
{
	int ret;

	dev_info(&drv_client->dev,
			"i2c client address is 0x%X\n", drv_client->addr);

	/* Read who_am_i register */
	ret = i2c_smbus_read_byte_data(drv_client, REG_WHO_AM_I);
	if (IS_ERR_VALUE(ret)) {
		dev_err(&drv_client->dev,
			"i2c_smbus_read_byte_data() failed with error: %d\n",
			ret);
		return ret;
	}
	if (ret != MPU6050_WHO_AM_I) {
		dev_err(&drv_client->dev,
			"wrong i2c device found: expected 0x%X, found 0x%X\n",
			MPU6050_WHO_AM_I, ret);
		return -1;
	}
	dev_info(&drv_client->dev,
		"i2c mpu6050 device found, WHO_AM_I register value = 0x%X\n",
		ret);

	/* Setup the device */
	/* No error handling here! */
	i2c_smbus_write_byte_data(drv_client, REG_SMRDIV, 0);
	i2c_smbus_write_byte_data(drv_client, REG_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0x00 | (MPU6050_GFS_SEL << 3));
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0x00 | (MPU6050_AFS_SEL << 3));
	i2c_smbus_write_byte_data(drv_client, REG_FIFO_EN, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_PIN_CFG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_ENABLE, 0);
	i2c_smbus_write_byte_data(drv_client, REG_USER_CTRL, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_1, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_2, 0);

	g_mpu6050_data.drv_client = drv_client;

	mpu6050_self_test(drv_client);

	dev_info(&drv_client->dev, "i2c driver probed\n");
	return 0;
}
/* ---------------------------------------------------------------------------
 * mpu6050_remove
 * ------------------------------------------------------------------------ */
static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = NULL;

	dev_info(&drv_client->dev, "i2c driver removed\n");
	return 0;
}
/* ---------------------------------------------------------------------------
 * mpu6050_show
 * ------------------------------------------------------------------------ */
/* Self test results:
 *      ST:      {18, 15, 18} {13, 12, 14}
 *      TV:      {1536, 2376, 10996} {5517, -5433, 5688}
 *      V:       {-3520, -2076, 5840} {-122, 101, -171}
 *
 * FTgx  = 25*131*1.046^(13-1) = 5618.126564392 // Factory Trim (FT) Value
 * STRgx = 5517-(-122)=5639                     // Self-Test Response
 * C%gx  = (5639-5618.13)/5618.13 = 0.0037      //change from factory trim of the self-test response
 *
 * FTax  = 4096*0.34*(0.92^((18-1)/30))/0.34 = 3906.96676433
 * STRax = 1536-(-3520) = 5056
 * C%ax  = (5056-3906.97)/3906.97 = 0.294
 * */
static ssize_t mpu6050_show(struct class *class, struct class_attribute *attr,
		char *buf)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	const char *name = attr->attr.name;
	int counter;

	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(data, g_mpu6050_data.data, sizeof(data));
	counter = g_mpu6050_data.counter;
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	pr_info("[%s]:: attr.name:%s\n", __func__, name);

	if (!strcmp(name, "all")) {
		sprintf(buf,
				"\n\taccel:\t {%d, %d, %d}\n\tgyro:\t {%d, %d, %d}\n\ttemperature:\t %d\n\tcounter:\t %d\n",
				data[0], data[1], data[2],
				data[3], data[4], data[5],
				data[6], counter
				);
	} else if (!strcmp(name, "self_test")) {
		sprintf(buf,
				"\n\tST:\t {%d, %d, %d} {%d, %d, %d}\n\tTV:\t {%d, %d, %d} {%d, %d, %d}\n\tV:\t {%d, %d, %d} {%d, %d, %d}\n",
				g_mpu6050_self_test_data.self_test.data[0],  g_mpu6050_self_test_data.self_test.data[1],  g_mpu6050_self_test_data.self_test.data[2],
				g_mpu6050_self_test_data.self_test.data[3],  g_mpu6050_self_test_data.self_test.data[4],  g_mpu6050_self_test_data.self_test.data[5],
				g_mpu6050_self_test_data.test_value.data[0], g_mpu6050_self_test_data.test_value.data[1], g_mpu6050_self_test_data.test_value.data[2],
				g_mpu6050_self_test_data.test_value.data[3], g_mpu6050_self_test_data.test_value.data[4], g_mpu6050_self_test_data.test_value.data[5],
				g_mpu6050_self_test_data.value.data[0],      g_mpu6050_self_test_data.value.data[1],      g_mpu6050_self_test_data.value.data[2],
				g_mpu6050_self_test_data.value.data[3],      g_mpu6050_self_test_data.value.data[4],      g_mpu6050_self_test_data.value.data[5]
				);

	} else if (!strcmp(name, "temperature"))
		sprintf(buf, "%d\n", data[6]);
	else if (!strcmp(name, "gyro_z"))
		sprintf(buf, "%d\n", data[5]);
	else if (!strcmp(name, "gyro_y"))
		sprintf(buf, "%d\n", data[4]);
	else if (!strcmp(name, "gyro_x"))
		sprintf(buf, "%d\n", data[3]);
	else if (!strcmp(name, "accel_z"))
		sprintf(buf, "%d\n", data[2]);
	else if (!strcmp(name, "accel_y"))
		sprintf(buf, "%d\n", data[1]);
	else if (!strcmp(name, "accel_x"))
		sprintf(buf, "%d\n", data[0]);

	return strlen(buf);
}
/* ---------------------------------------------------------------------------
 * work_func
 * ------------------------------------------------------------------------ */
static int mpu6050_thread(void *param)
{
	pr_info("[%s]+++\n", __func__);
	while (!kthread_should_stop()) {
		if (wait_for_completion_timeout(&comp_one_sec, TIMER_PERIOD))
			mpu6050_read_data();
	}
	pr_info("[%s]---\n", __func__);
	return 0;
}
/* ---------------------------------------------------------------------------
 * mpu6050_dev_ioctl
 * ------------------------------------------------------------------------ */
static long mpu6050_dev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int __user *p = (int __user *)arg;
	int retval = 0;
	int data[NUM_MPU6050_DATA];
	unsigned long flags;

    DBG(pr_info("[%s]:: %p, %x, %x\n", __func__, f, cmd, arg));

	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(data, g_mpu6050_data.data, sizeof(data));
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	switch(cmd) {
		case MPU6050_GET_ACCEL_X:
            DBG(pr_info("[%s]:: MPU6050_GET_ACCEL_X: %d\n", __func__, data[0]);)
			put_user(data[0], p);
			break;
		case MPU6050_GET_ACCEL_Y:
            DBG(pr_info("[%s]:: MPU6050_GET_ACCEL_Y: %d\n", __func__, data[1]);)
			put_user(data[1], p);
			break;
		case MPU6050_GET_ACCEL_Z:
            DBG(pr_info("[%s]:: MPU6050_GET_ACCEL_Z: %d\n", __func__, data[2]);)
			put_user(data[2], p);
			break;
		case MPU6050_GET_GYRO_X:
            DBG(pr_info("[%s]:: MPU6050_GET_GYRO_X: %d\n", __func__, data[3]);)
			put_user(data[3], p);
			break;
		case MPU6050_GET_GYRO_Y:
            DBG(pr_info("[%s]:: MPU6050_GET_GYRO_Y: %d\n", __func__, data[4]);)
			put_user(data[4], p);
		case MPU6050_GET_GYRO_Z:
            DBG(pr_info("[%s]:: MPU6050_GET_GYRO_Z: %d\n", __func__, data[5]);)
			put_user(data[5], p);
		case MPU6050_GET_ACCEL_VAL:
            DBG(pr_info("[%s]:: MPU6050_GET_ACCEL_VAL: %d %d %d\n", __func__, data[0], data[1], data[2]);)
            retval = copy_to_user(p, data, sizeof(struct mpu6050_simple_values));
			break;
		case MPU6050_GET_GYRO_VAL:
            DBG(pr_info("[%s]:: MPU6050_GET_GYRO_VAL: %d %d %d\n", __func__, data[4], data[5], data[6]);)
            retval = copy_to_user(p, &data[3], sizeof(struct mpu6050_simple_values));
			break;
		case MPU6050_GET_TEMPERATURE:
            DBG(pr_info("[%s]:: MPU6050_GET_TEMPERATURE: %d\n", __func__, data[6]);)
			put_user(data[6], p);
		case MPU6050_GET_ALL_VAL:
            DBG(pr_info("[%s]:: MPU6050_GET_ALL_VAL: %d %d %d %d %d %d %d\n", __func__,
                    data[0], data[1], data[2], data[3], data[5], data[5], data[6]);)
            retval = copy_to_user(p, data, sizeof(struct mpu6050_values));
			break;
		case MPU6050_GET_DIV_VAL:
            DBG(pr_info("[%s]:: MPU6050_GET_DIV_VAL: %d %d\n", __func__,
                    MPU6050_AGAIN, MPU6050_GGAIN);)
			data[0] = MPU6050_AGAIN;
			data[1] = MPU6050_GGAIN;
            retval = copy_to_user(p, data, sizeof(struct mpu6050_div_values));
			break;
		default:
            pr_err("[%s]:: wrong IOCTL: %d %lx\n", __func__, cmd, arg);
            return -EINVAL;
	}
	return retval;
}
/* ---------------------------------------------------------------------------
 * mpu6050_dev_read
 * ------------------------------------------------------------------------ */
static ssize_t mpu6050_dev_read(struct file *file, char *buf, size_t count,
		loff_t *ppos)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	char buf_str[265];
	size_t buf_len = 0;
	int counter = 0;
	static int prev_counter;

	pr_info("[%s] (%p, %p, %d, %p(%lld))\n", __func__,
			file, buf, count, ppos, (ppos) ? *ppos : 0);
	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(data, g_mpu6050_data.data, sizeof(data));
	counter = g_mpu6050_data.counter;
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	if (counter == prev_counter) {
		pr_info("Reading avoid: %d\n", counter);
		return 0;
	}
	prev_counter = counter;

	snprintf(buf_str, sizeof(buf_str) - 1,
			"%+5d %+5d %+5d %+5d %+5d %+5d %+5d %d %d\n",
			data[0], data[1], data[2], 
			data[3], data[4], data[5], 
			data[6],
			MPU6050_GGAIN,
			MPU6050_AGAIN);
	buf_len = strlen(buf_str);

	if (buf_len > count) {
		pr_err("Reading more lengths .... (%d > %d)\n", buf_len, count);
		return -EINVAL;
	}

	if (copy_to_user(buf, buf_str, buf_len))
		return -EINVAL;

	if (ppos)
		*ppos += buf_len;

	return buf_len;
}
/* ---------------------------------------------------------------------------
 * mpu6050_init
 * ------------------------------------------------------------------------ */
static int mpu6050_init(void)
{
	int ret;
	dev_t dev;

	memset(&g_mpu6050_data, 0, sizeof(g_mpu6050_data));
	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		pr_err("mpu6050: failed to add new i2c driver: %d\n", ret);
		return ret;
	}
	pr_info("mpu6050: i2c driver created\n");

	ret = class_register(&mpu6050_class);
	if (ret) {
		pr_err("Unable to register mpu6050_class class\n");
		goto err;
	}

	ret = alloc_chrdev_region(&dev, 0, 1, "mpu6050");
	major = MAJOR(dev);
	if (ret < 0) {
		pr_err("mpu6050: can not register char device\n");
		goto alloc_chrdev_err;
	}

	cdev_init(&mpu6050_dev, &mpu6050_dev_fops);
	mpu6050_dev.owner = THIS_MODULE;

	ret = cdev_add(&mpu6050_dev, dev, 1);
	if (ret < 0) {
		unregister_chrdev_region(MKDEV(major, 0), 1);
		pr_err("mpu6050: can not add char device\n");
		goto alloc_chrdev_err;
	}

	device_create(&mpu6050_class, NULL, dev, NULL, "mpu6050_x");

	ret = init_driver_timer();
	if (ret) {
		pr_err("Unable to init timer: %d\n", ret);
		goto create_class_err;
	}
	pr_info("mpu6050: sysfs class attributes created\n");

	p_mpu6050_thread = kthread_run(mpu6050_thread, NULL, "mpu6050_thread");

	pr_info("mpu6050: module loaded\n");

	return ret;
alloc_chrdev_err:
create_class_err:
err:
	i2c_del_driver(&mpu6050_i2c_driver);
	return ret;
}
/* ---------------------------------------------------------------------------
 * mpu6050_exit
 * ------------------------------------------------------------------------ */
static void mpu6050_exit(void)
{
	dev_t dev;

	kthread_stop(p_mpu6050_thread);

	deinit_driver_timer();

	dev = MKDEV(major, 0);
	device_destroy(&mpu6050_class, dev);
	cdev_del(&mpu6050_dev);
	unregister_chrdev_region(MKDEV(major, 0), 1);

	class_unregister(&mpu6050_class);
	pr_info("mpu6050: sysfs class destroyed\n");

	i2c_del_driver(&mpu6050_i2c_driver);
	pr_info("mpu6050: i2c driver deleted\n");

	pr_info("mpu6050: module exited\n");
}
/* ---------------------------------------------------------------------------
 *
 * ------------------------------------------------------------------------ */
module_init(mpu6050_init);
module_exit(mpu6050_exit);
