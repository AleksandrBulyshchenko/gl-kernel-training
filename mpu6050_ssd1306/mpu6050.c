#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/cdev.h>
#include "mpu6050-regs.h"
#include "mpu6050.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleksii Kogutenko");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel training: 07 - MPU6050 mudule + cdev");

// ----------------------------------------------------------------------------
static struct mpu6050_data g_mpu6050_data;
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

ATTR_GROUP(mpu6050,
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_x),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_y),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, accel_z),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_x),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_y),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, gyro_z),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, temperature),
		ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(mpu6050, all),
			NULL
		))))))))
	)

/*ATTR_GROUP_NEW(mpu6050,
		ATTR_TEXT(mpu6050, all, mpu6050_show,
		ATTR_TEXT(mpu6050, temperature, mpu6050_show,
		ATTR_TEXT(mpu6050, gyro_z, mpu6050_show,
		ATTR_TEXT(mpu6050, gyro_y, mpu6050_show,
		ATTR_TEXT(mpu6050, gyro_x, mpu6050_show,
		ATTR_TEXT(mpu6050, accel_z, mpu6050_show,
		ATTR_TEXT(mpu6050, accel_y, mpu6050_show,
		ATTR_TEXT(mpu6050, accel_x, mpu6050_show,)))))))))
		*/
ATTR_GROUPS(mpu6050, ATTR_GET_GROUP(mpu6050))
// ----------------------------------------------------------------------------
struct class mpu6050_class = {
	.name		  = "mpu6050",
	.class_groups = ATTR_GET_GROUPS(mpu6050),
	.owner		  = THIS_MODULE,
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
	spinlock_t	lock;
} timer;
// ----------------------------------------------------------------------------
static struct work_struct i2c_read_work;
// ----------------------------------------------------------------------------
static struct cdev mpu6050_dev;
static const struct file_operations mpu6050_dev_fops = {
	.owner = THIS_MODULE,
	.read  = mpu6050_dev_read,
};
static int major = 0;
module_param(major, int, S_IRUGO);
// ----------------------------------------------------------------------------
// timer callback
// ----------------------------------------------------------------------------
void timer_callback(unsigned long data)
{
	unsigned long flags = 0;
	u64 jiff, diff;

	spin_lock_irqsave(&timer.lock, flags);

	schedule_work(&i2c_read_work);

	jiff = get_jiffies_64();
	diff = (jiff - timer.timer.expires) + TIMER_PERIOD + jiff;

	mod_timer(&timer.timer, diff);

	spin_unlock_irqrestore(&timer.lock, flags);
}
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
static int init_driver_timer(void)
{
	int ret = 0;

	spin_lock_init(&timer.lock);
	init_timer(&timer.timer);
	setup_timer(&timer.timer, timer_callback, 0);
	ret = mod_timer(&timer.timer, get_jiffies_64() + TIMER_PERIOD);

	return ret;
}
// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------
static void deinit_driver_timer(void)
{
	del_timer(&timer.timer);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#define I2C_READ(client, addr) \
	(s16)((u16)i2c_smbus_read_word_swapped(client, addr))
static int mpu6050_read_data(void)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	struct i2c_client *drv_client = g_mpu6050_data.drv_client;

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
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	/*dev_info(&drv_client->dev, "sensor data read:\n");
	dev_info(&drv_client->dev, "ACCEL[X,Y,Z] = [%d, %d, %d]\n",
		g_mpu6050_data.accel_values[0],
		g_mpu6050_data.accel_values[1],
		g_mpu6050_data.accel_values[2]);
	dev_info(&drv_client->dev, "GYRO[X,Y,Z] = [%d, %d, %d]\n",
		g_mpu6050_data.gyro_values[0],
		g_mpu6050_data.gyro_values[1],
		g_mpu6050_data.gyro_values[2]);
	dev_info(&drv_client->dev, "TEMP = %d\n",
		g_mpu6050_data.temperature);
	*/
	return 0;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
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
	i2c_smbus_write_byte_data(drv_client, REG_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_FIFO_EN, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_PIN_CFG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_ENABLE, 0);
	i2c_smbus_write_byte_data(drv_client, REG_USER_CTRL, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_1, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_2, 0);

	g_mpu6050_data.drv_client = drv_client;

	dev_info(&drv_client->dev, "i2c driver probed\n");
	return 0;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = NULL;

	dev_info(&drv_client->dev, "i2c driver removed\n");
	return 0;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static ssize_t mpu6050_show(struct class *class, struct class_attribute *attr,
		char *buf)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	const char *name = attr->attr.name;

	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(data, g_mpu6050_data.data, sizeof(data));
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

	pr_info("[%s]:: attr.name:%s\n", __func__, name);

	if (!strcmp(name, "all")) {
		sprintf(buf,
				"accel: {%d, %d, %d}\ngyro: {%d, %d, %d}\ntemperature: %d\n",
				data[0], data[1], data[2],
				data[3], data[4], data[5],
				data[6]
				);
	} else
	if (!strcmp(name, "temperature"))
		sprintf(buf, "%d\n", data[6]);
	else
	if (!strcmp(name, "gyro_z"))
		sprintf(buf, "%d\n", data[5]);
	else
	if (!strcmp(name, "gyro_y"))
		sprintf(buf, "%d\n", data[4]);
	else
	if (!strcmp(name, "gyro_x"))
		sprintf(buf, "%d\n", data[3]);
	else
	if (!strcmp(name, "accel_z"))
		sprintf(buf, "%d\n", data[2]);
	else
	if (!strcmp(name, "accel_y"))
		sprintf(buf, "%d\n", data[1]);
	else
	if (!strcmp(name, "accel_x"))
		sprintf(buf, "%d\n", data[0]);

	return strlen(buf);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void work_func(struct work_struct *unused)
{
		mpu6050_read_data();
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static ssize_t mpu6050_dev_read(struct file *file, char *buf, size_t count,
		loff_t *ppos)
{
	unsigned long flags;
	int data[NUM_MPU6050_DATA];
	char buf_str[65];

	spin_lock_irqsave(&g_mpu6050_data.lock, flags);
	memcpy(data, g_mpu6050_data.data, sizeof(data));
	spin_unlock_irqrestore(&g_mpu6050_data.lock, flags);

		snprintf(buf_str, sizeof(buf_str) - 1,
				"%d, %d, %d, %d, %d, %d, %d",
				data[0], data[1], data[2],
				data[3], data[4], data[5],
				data[6]
				);

	if (copy_to_user(buf, buf_str, strlen(buf_str)))
		return -EINVAL;

	return strlen(buf_str);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static int mpu6050_init(void)
{
	int ret;
	dev_t dev;

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
		pr_err("mpu6050: can not add char device\n" );
		goto alloc_chrdev_err;
	}

	device_create(&mpu6050_class, NULL, dev, NULL, "mpu6050_x" );

	INIT_WORK(&i2c_read_work, work_func);
	ret = init_driver_timer();
	if (ret) {
		pr_err("Unable to init timer: %d\n", ret);
		goto create_class_err;
	}
	pr_info("mpu6050: sysfs class attributes created\n");

	pr_info("mpu6050: module loaded\n");

	return ret;
alloc_chrdev_err:
create_class_err:
err:
	i2c_del_driver(&mpu6050_i2c_driver);
	return ret;
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void mpu6050_exit(void)
{
	dev_t dev;
	deinit_driver_timer();
	cancel_work_sync(&i2c_read_work);

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
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

module_init(mpu6050_init);
module_exit(mpu6050_exit);
