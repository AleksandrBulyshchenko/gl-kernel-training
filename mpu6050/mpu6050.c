#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "mpu6050-regs.h"


enum {
	X = 0,
	Y = 1,
	Z = 2
};

struct mpu6050_data {
	struct i2c_client *drv_client;
	int accel_values[3];
	int gyro_values[3];
	int temperature;
};

static struct mpu6050_data mpu6050_data;

static int mpu6050_read_data(void)
{
	int temp;
	struct i2c_client *drv_client = mpu6050_data.drv_client;

	if (drv_client == 0)
		return -ENODEV;

	/* accel */
	mpu6050_data.accel_values[X] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_XOUT_H));
	mpu6050_data.accel_values[Y] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_YOUT_H));
	mpu6050_data.accel_values[Z] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	mpu6050_data.gyro_values[X] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_XOUT_H));
	mpu6050_data.gyro_values[Y] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_YOUT_H));
	mpu6050_data.gyro_values[Z] = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_GYRO_ZOUT_H));
	/* Temperature in degrees C =
	 * (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53
	 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(drv_client, REG_TEMP_OUT_H));
	mpu6050_data.temperature = (temp + 12420 + 170) / 340;

	dev_info(&drv_client->dev, "sensor data read:\n");
	dev_info(&drv_client->dev, "ACCEL[X,Y,Z] = [%d, %d, %d]\n",
		mpu6050_data.accel_values[X],
		mpu6050_data.accel_values[Y],
		mpu6050_data.accel_values[Z]);
	dev_info(&drv_client->dev, "GYRO[X,Y,Z] = [%d, %d, %d]\n",
		mpu6050_data.gyro_values[X],
		mpu6050_data.gyro_values[Y],
		mpu6050_data.gyro_values[Z]);
	dev_info(&drv_client->dev, "TEMP = %d\n",
		mpu6050_data.temperature);

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

	mpu6050_data.drv_client = drv_client;

	dev_info(&drv_client->dev, "i2c driver probed\n");
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client)
{
	mpu6050_data.drv_client = 0;

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

struct class_ext_attribute {
	struct class_attribute attr;
	void *var;
};

static ssize_t mpu6050_show(struct class *class, struct class_attribute *attr, char *buf)
{
	struct class_ext_attribute *a = container_of(attr, struct class_ext_attribute, attr);
	int *v = a->var;

	mpu6050_read_data();
	return scnprintf(buf, PAGE_SIZE, "%d", *v);
}

#define MPU6050_ATTR(_name, _mode, _var) \
  struct class_ext_attribute mpu6050_class_attr_##_name = \
  { __ATTR(_name, _mode, mpu6050_show, NULL), _var }

MPU6050_ATTR(temperature, 0444, &mpu6050_data.temperature);
MPU6050_ATTR(gyro_x, 0444, &mpu6050_data.gyro_values[X]);
MPU6050_ATTR(gyro_y, 0444, &mpu6050_data.gyro_values[Y]);
MPU6050_ATTR(gyro_z, 0444, &mpu6050_data.gyro_values[Z]);
MPU6050_ATTR(accel_x, 0444, &mpu6050_data.accel_values[X]);
MPU6050_ATTR(accel_y, 0444, &mpu6050_data.accel_values[Y]);
MPU6050_ATTR(accel_z, 0444, &mpu6050_data.accel_values[Z]);

static struct class_attribute *g_mpu6050_class_attrs[] = {
	&mpu6050_class_attr_gyro_x.attr,
	&mpu6050_class_attr_gyro_y.attr,
	&mpu6050_class_attr_gyro_z.attr,
	&mpu6050_class_attr_accel_x.attr,
	&mpu6050_class_attr_accel_y.attr,
	&mpu6050_class_attr_accel_z.attr,
	&mpu6050_class_attr_temperature.attr,
	NULL
};

/*
 * class_* interface as of v4.9 does not provide
 * a function to register/remove a group of attributes
 * so implement smth like it on our own
 */
static int class_create_group(struct class *class, struct class_attribute **attrs)
{
	int i;
	int ret = 0;

	for (i = 0; attrs[i]; i++) {
		ret = class_create_file(class, attrs[i]);
		if (ret) {
			while (i--) {
				class_remove_file(class, attrs[i]);
			}
			break;
		}
	}
	return ret;
}

static void class_remove_group(struct class *class, struct class_attribute **attrs)
{
	for (; *attrs; attrs++)
		class_remove_file(class, *attrs);
}

static struct class *mpu6050_class;

static int mpu6050_init(void)
{
	int ret;

	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		pr_err("mpu6050: failed to add new i2c driver: %d\n", ret);
		goto driver_fail;
	}
	pr_info("mpu6050: i2c driver created\n");

	/* Create class */
	mpu6050_class = class_create(THIS_MODULE, "mpu6050");
	if (IS_ERR(mpu6050_class)) {
		ret = PTR_ERR(mpu6050_class);
		pr_err("mpu6050: failed to create sysfs class\n");
		goto class_fail;
	}
	pr_info("mpu6050: sysfs class created\n");

	ret = class_create_group(mpu6050_class, g_mpu6050_class_attrs);
	if (IS_ERR_VALUE(ret)) {
		pr_err("mpu6050: failed to create attributes\n");
		goto group_fail;
	}
	pr_info("mpu6050: sysfs class attributes created\n");

	pr_info("mpu6050: module loaded\n");
	return 0;

group_fail:
	class_destroy(mpu6050_class);

class_fail:
	i2c_del_driver(&mpu6050_i2c_driver);

driver_fail:

	return ret;
}

static void mpu6050_exit(void)
{
	class_remove_group(mpu6050_class, g_mpu6050_class_attrs);
	class_destroy(mpu6050_class);
	pr_info("mpu6050: sysfs class attributes removed\n");

	i2c_del_driver(&mpu6050_i2c_driver);
	pr_info("mpu6050: i2c driver deleted\n");

	pr_info("mpu6050: module exited\n");
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_AUTHOR("Maksym Glubokiy <m.glubokiy@gmail.com>");
MODULE_DESCRIPTION("mpu6050 I2C acc&gyro");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
