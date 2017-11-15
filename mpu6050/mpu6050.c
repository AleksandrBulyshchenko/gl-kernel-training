#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/sysfs.h>
#include <linux/time.h>

#include "mpu6050-regs.h"
#include "mpu6050_data.h"
#include "mpu6050_cdev.h"

//data
#define ELEMENTS_COUNT 10
static struct mpu6050_data_holder g_mpu6050_data;


//functionality
static size_t get_attribute_index(struct kobj_attribute const *attribute);

static int mpu6050_read_data(bool debug)
{
	int temp;
	const struct i2c_client *drv_client = g_mpu6050_data.drv_client;
	struct mpu6050_data_elements* current_element =
		get_active_element(&g_mpu6050_data);
	struct mpu6050_data_elements element;
	u64 msecs = jiffies_to_msecs(get_jiffies_64());

	if (drv_client == 0)
		return -ENODEV;
	if (current_element) {
		u64 diff = msecs - current_element->extra_data[INDEX_TIMESTAMP];
		if (diff < MSEC_PER_SEC) {
			if (debug)
				dev_info(&drv_client->dev, "data reading skipped: %llu\n",
					diff);
			return 0;
		}
	}

	/* accel */
	element.data[INDEX_ACCEL_X] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_XOUT_H));
	element.data[INDEX_ACCEL_Y] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_YOUT_H));
	element.data[INDEX_ACCEL_Z] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	element.data[INDEX_GYRO_X] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_XOUT_H));
	element.data[INDEX_GYRO_Y] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_YOUT_H));
	element.data[INDEX_GYRO_Z] =
		(s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_ZOUT_H));
	/* Temperature in degrees C =
	 * (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53
	 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_TEMP_OUT_H));
	element.data[INDEX_TEMPERATURE] = (temp + 12420 + 170) / 340;

	// Extra data
	element.extra_data[INDEX_TIMESTAMP] = msecs;

	add_mpu6050_element(&g_mpu6050_data, &element);

	if (debug) {
		dev_info(&drv_client->dev, "sensor data read:\n");
		dev_info(&drv_client->dev, "ACCEL[X,Y,Z] = [%d, %d, %d]\n",
			element.data[INDEX_ACCEL_X],
			element.data[INDEX_ACCEL_Y],
			element.data[INDEX_ACCEL_Z]);
		dev_info(&drv_client->dev, "GYRO[X,Y,Z] = [%d, %d, %d]\n",
			element.data[INDEX_GYRO_X],
			element.data[INDEX_GYRO_Y],
			element.data[INDEX_GYRO_Z]);
		dev_info(&drv_client->dev, "TEMP = %d\n",
			element.data[INDEX_TEMPERATURE]);
		dev_info(&drv_client->dev, "TIMESTAMP = %llu\n",
			element.extra_data[INDEX_TIMESTAMP]);
	}
	return 0;
}

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

static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = 0;

	dev_info(&drv_client->dev, "i2c driver removed\n");
	return 0;
}

static const struct i2c_device_id mpu6050_idtable[] = {
	{ "mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "mpu6050",
	},

	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};

static ssize_t data_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	size_t index = get_attribute_index(attr);
	mpu6050_read_data(true);

	if (index < INDEX_COUNT && g_mpu6050_data.element_iter_current)
		sprintf(buf, "%d\n", g_mpu6050_data.element_iter_current->data.data[index]);
	else
		buf[0] = '\0';
	return strlen(buf);
}

static const struct kobj_attribute g_kobj_attributes[INDEX_COUNT] = {
	__ATTR(accel_x, S_IRUGO, data_show, NULL),
	__ATTR(accel_y, S_IRUGO, data_show, NULL),
	__ATTR(accel_z, S_IRUGO, data_show, NULL),
	__ATTR(gyro_x, S_IRUGO, data_show, NULL),
	__ATTR(gyro_y, S_IRUGO, data_show, NULL),
	__ATTR(gyro_z, S_IRUGO, data_show, NULL),
	__ATTR(temperature, S_IRUGO, data_show, NULL)
};

static const struct attribute *g_attributes[INDEX_COUNT + 1] = {
    &g_kobj_attributes[INDEX_ACCEL_X].attr,
    &g_kobj_attributes[INDEX_ACCEL_Y].attr,
    &g_kobj_attributes[INDEX_ACCEL_Z].attr,
    &g_kobj_attributes[INDEX_GYRO_X].attr,
    &g_kobj_attributes[INDEX_GYRO_Y].attr,
    &g_kobj_attributes[INDEX_GYRO_Z].attr,
    &g_kobj_attributes[INDEX_TEMPERATURE].attr,
    NULL,
};

// Gets a number of attributes + 1 (null-terminated)
static size_t const attributes_count(void)
{
	#if 1
	size_t const count = INDEX_COUNT + 1;
	#else
	size_t const full_size = sizeof(g_attributes);
	static size_t const count = full_size ? full_size / sizeof(g_attributes[0]) : 0;
	#endif
	return count;
}

// Gets index of attribute in common array of data
static size_t get_attribute_index(struct kobj_attribute const *attribute)
{
	size_t const count = attributes_count();
	size_t index = 0;
	if (count)
	{
		index = (size_t)(attribute - &g_kobj_attributes[0]);
		if (index >= count)
		{
			pr_err("%s wrong computing of data index, corrected: %ld -> %ld\n",
				THIS_MODULE->name, (long)index, (long)(count - 1));
			index = count - 1;
		}
	}
	return index;
}

// Module initialization
static struct kobject *g_kobject = NULL;

static void free_sysfs(void)
{
	if (g_kobject) {
		sysfs_remove_files(g_kobject, g_attributes);
		kobject_put(g_kobject);

		pr_info("mpu6050: sysfs data destroyed\n");
	}
}

static int mpu6050_init(void)
{
	int ret;
	struct cdevs_holder *cdevs = get_cdevs();

	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		pr_err("mpu6050: failed to add new i2c driver: %d\n", ret);
		return ret;
	}
	pr_info("mpu6050: i2c driver created\n");

	g_kobject = kobject_create_and_add("mpu6050", kernel_kobj);
	if (!g_kobject) {
		ret = -ENOMEM;
		pr_err("mpu6050: failed to create sysfs kobject: %d\n", ret);
		goto error1;
	}

	ret = sysfs_create_files(g_kobject, g_attributes);
	if (ret) {
		pr_err("mpu6050: failed to create sysfs data attributes: %d\n", ret);
		goto error2;
	}
	pr_info("mpu6050: sysfs data attributes created\n");

	init_mpu6050_data(&g_mpu6050_data, ELEMENTS_COUNT, mpu6050_read_data);

	ret = init_cdevs(cdevs, &g_mpu6050_data);
	if (ret) {
		pr_err("mpu6050: failed to create cdevs: %d\n", ret);
		goto error3;
	}

	pr_info("mpu6050: module loaded\n");
	return 0;

error3:
	free_mpu6050_data(&g_mpu6050_data);
error2:
	free_sysfs();
error1:
	i2c_del_driver(&mpu6050_i2c_driver);
	return ret;
}

static void mpu6050_exit(void)
{
	free_cdevs(get_cdevs());
	free_mpu6050_data(&g_mpu6050_data);
	free_sysfs();

	i2c_del_driver(&mpu6050_i2c_driver);
	pr_info("mpu6050: i2c driver deleted\n");

	pr_info("mpu6050: module exited\n");
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_AUTHOR("Andriy.Khulap <andriy.khulap@globallogic.com>+Sergii.Romantsov");
MODULE_DESCRIPTION("mpu6050 I2C acc&gyro");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
