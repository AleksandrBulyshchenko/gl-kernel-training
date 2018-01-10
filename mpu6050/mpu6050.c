#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "mpu6050-regs.h"
 
#define MPU6050_VALUE_NUM 7

struct mpu6050_data
{
	struct i2c_client *drv_client;
	int values[MPU6050_VALUE_NUM];
};

static struct mpu6050_data g_mpu6050_data;

static int mpu6050_read_data(void)
{
	int temp;

	if (g_mpu6050_data.drv_client == 0)
		return -ENODEV;

	/* accel */
	g_mpu6050_data.values[0] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_XOUT_H));
	g_mpu6050_data.values[1] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_YOUT_H));
	g_mpu6050_data.values[2] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	g_mpu6050_data.values[3] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_XOUT_H));
	g_mpu6050_data.values[4] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_YOUT_H));
	g_mpu6050_data.values[5] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_ZOUT_H));
	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_TEMP_OUT_H));
	g_mpu6050_data.values[6] = (temp + 12420 + 170) / 340;

	printk(KERN_INFO "mpu6050: sensor data read:\n");
	printk(KERN_INFO "mpu6050: ACCEL=%d, %d, %d\n",
		g_mpu6050_data.values[0],
		g_mpu6050_data.values[1],
		g_mpu6050_data.values[2]);
	printk(KERN_INFO "mpu6050: GYRO=%d, %d, %d\n",
		g_mpu6050_data.values[3],
		g_mpu6050_data.values[4],
		g_mpu6050_data.values[5]);
	printk(KERN_INFO "mpu6050: TEMP = %d\n", g_mpu6050_data.values[6]);
	return 0;
}

static int mpu6050_probe(struct i2c_client *drv_client, const struct i2c_device_id *id)
{
	int ret;

	printk(KERN_INFO "mpu6050: i2c client address is 0x%X\n", drv_client->addr);

	/* Read who_am_i register */
	ret = i2c_smbus_read_byte_data(drv_client, REG_WHO_AM_I);
	if (IS_ERR_VALUE(ret)) {
		printk(KERN_ERR "mpu6050: i2c_smbus_read_byte_data() failed with error: %d\n", ret);
		return ret;
	}
	if (ret != MPU6050_WHO_AM_I) {
		printk(KERN_ERR "mpu6050: wrong i2c device found: expected 0x%X, found 0x%X\n", MPU6050_WHO_AM_I, ret);
		return -1;
	}
	printk(KERN_INFO "mpu6050: i2c mpu6050 device found, WHO_AM_I register value = 0x%X\n", ret);

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

	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = 0;

	printk(KERN_INFO "mpu6050: i2c driver removed\n");
	return 0;
}

static const struct i2c_device_id mpu6050_idtable [] = {
	{ "gl_mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "gl_mpu6050",
	},

	.probe = mpu6050_probe ,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};

static ssize_t show_value(struct class *class, struct class_attribute *attr, char *buf);

struct class_attribute class_attr_array[MPU6050_VALUE_NUM] = {
	{ .attr = { .name = "accel_x", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "accel_y", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "accel_z", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "gyro_x", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "gyro_y", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "gyro_z", .mode = S_IRUGO }, .show = &show_value, },
	{ .attr = { .name = "temperature", .mode = S_IRUGO }, .show = &show_value, },
};

static ssize_t show_value(struct class *class, struct class_attribute *attr, char *buf) {
	int index = attr - class_attr_array;
	int value = 0;
	mpu6050_read_data();
	if (index >=0 && index < MPU6050_VALUE_NUM) {
		value = g_mpu6050_data.values[index];
	}
	sprintf(buf, "%d\n", value);
	return strlen(buf);
}

static struct class *attr_class = 0;

static int mpu6050_init(void)
{
	int ret, i;

	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "mpu6050: i2c driver created\n");

	/* Create class */
	attr_class = class_create(THIS_MODULE, "mpu6050");
	if (IS_ERR(attr_class)) {
		ret = PTR_ERR(attr_class);
		printk(KERN_ERR "mpu6050: failed to create sysfs class: %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "mpu6050: sysfs class created\n");

	for (i = 0; i < MPU6050_VALUE_NUM; ++i) {
		ret = class_create_file(attr_class, &class_attr_array[i]);
		if (ret) {
			printk(KERN_ERR "mpu6050: failed to create sysfs class attribute accel_x: %d\n", ret);
			return ret;
		}
 	}

	printk(KERN_INFO "mpu6050: sysfs class attributes created\n");

	printk(KERN_INFO "mpu6050: module loaded\n");
	return 0;
}

static void mpu6050_exit(void)
{
	if (attr_class) {
		int i;

		for (i = 0; i < MPU6050_VALUE_NUM; ++i) {
			class_remove_file(attr_class, &class_attr_array[i]);
		}
		class_destroy(attr_class);									
	}

	i2c_del_driver(&mpu6050_i2c_driver);
	printk(KERN_INFO "mpu6050: i2c driver deleted\n");

	printk(KERN_INFO "mpu6050: module exited\n");
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_AUTHOR("Andriy.Khulap <andriy.khulap@globallogic.com>");
MODULE_DESCRIPTION("mpu6050 I2C acc&gyro");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
