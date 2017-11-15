#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "mpu6050-regs.h"

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR("Oleksii Kogutenko");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Kernel training: 06 - MPU6050 mudule");

struct mpu6050_data
{
	struct i2c_client *drv_client;
	int accel_values[3];
	int gyro_values[3];
	int temperature;
};

static struct mpu6050_data g_mpu6050_data;
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
static int mpu6050_read_data(void)
{
	int temp;

	if (g_mpu6050_data.drv_client == 0)
		return -ENODEV;

	/* accel */
	g_mpu6050_data.accel_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_XOUT_H));
	g_mpu6050_data.accel_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_YOUT_H));
	g_mpu6050_data.accel_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	g_mpu6050_data.gyro_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_XOUT_H));
	g_mpu6050_data.gyro_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_YOUT_H));
	g_mpu6050_data.gyro_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_GYRO_ZOUT_H));
	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(g_mpu6050_data.drv_client, REG_TEMP_OUT_H));
	g_mpu6050_data.temperature = (temp + 12420 + 170) / 340;

	printk(KERN_INFO "mpu6050: sensor data read:\n");
	printk(KERN_INFO "mpu6050: ACCEL[X,Y,Z] = [%d, %d, %d]\n",
		g_mpu6050_data.accel_values[0],
		g_mpu6050_data.accel_values[1],
		g_mpu6050_data.accel_values[2]);
	printk(KERN_INFO "mpu6050: GYRO[X,Y,Z] = [%d, %d, %d]\n",
		g_mpu6050_data.gyro_values[0],
		g_mpu6050_data.gyro_values[1],
		g_mpu6050_data.gyro_values[2]);
	printk(KERN_INFO "mpu6050: TEMP = %d\n", g_mpu6050_data.temperature);
	return 0;
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
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

    /*ret = sysfs_create_group(&drv_client->dev.kobj, &group_mpu6050);
    if (IS_ERR_VALUE(ret)) {
        printk(KERN_ERR "Can't create sysfs group: error %d\n", ret);
        return ret;
    } */         


	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = 0;
    //sysfs_remove_group(&drv_client->dev.kobj, &group_mpu6050);
	printk(KERN_INFO "mpu6050: i2c driver removed\n");
	return 0;
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static const struct i2c_device_id mpu6050_idtable [] = {
	{ "mpu6050", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "mpu6050",
	},

	.probe = mpu6050_probe ,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
static ssize_t mpu6050_accel_x_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.accel_values[0]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_accel_y_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.accel_values[1]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_accel_z_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.accel_values[2]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_gyro_x_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.gyro_values[0]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_gyro_y_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.gyro_values[1]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_gyro_z_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.gyro_values[2]);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_temperature_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

	sprintf(buf, "%d\n", g_mpu6050_data.temperature);
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static ssize_t mpu6050_all_show(struct class *class, struct class_attribute *attr, char *buf)
{
	mpu6050_read_data();

    sprintf(buf, "accel: {%d, %d, %d}\ngyro: {%d, %d, %d}\ntemperature: %d\n", 
            g_mpu6050_data.accel_values[0],
            g_mpu6050_data.accel_values[1],
            g_mpu6050_data.accel_values[2],
            g_mpu6050_data.gyro_values[0],
            g_mpu6050_data.gyro_values[1],
            g_mpu6050_data.gyro_values[2],
            g_mpu6050_data.temperature
            );
	return strlen(buf);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

#define CLASS__ATTR_RO(_name, _data) {						\
	.attr	= { 											\
		.name = __stringify(_data), 						\
		.mode = S_IRUGO 									\
	},														\
	.show	= _name##_##_data##_show,						\
}

#define CLASS_DATA_RO(name, data) \
    struct class_attribute class_attr_##name##data = CLASS__ATTR_RO( name, data );

#define CLASS_DATA_RO_ATTR(name, data) \
    class_attr_##name##data.attr

// ----------------------------------------------------------------------------------------
CLASS_DATA_RO(mpu6050, accel_x);
CLASS_DATA_RO(mpu6050, accel_y);
CLASS_DATA_RO(mpu6050, accel_z);
CLASS_DATA_RO(mpu6050, gyro_x);
CLASS_DATA_RO(mpu6050, gyro_y);
CLASS_DATA_RO(mpu6050, gyro_z);
CLASS_DATA_RO(mpu6050, temperature);
CLASS_DATA_RO(mpu6050, all);

#define ATTR_GROUP_EXTRA_GEN(current, next) \
        &current, \
        next

#define ATTR_GROUP(_name, attr_extra) \
    static struct attribute *attrs_##_name##_extra[] = { \
        attr_extra \
    }; \
    static struct attribute_group group_##_name = { \
        .attrs =  attrs_##_name##_extra, \
    };
//        .name  = __stringify(_name), \

#define ATTR_GROUPS(name, attr_group) \
    static const struct attribute_group *groups_##name[] = {\
        &attr_group, \
        NULL \
    };

ATTR_GROUP(mpu6050,
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, accel_x),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, accel_y),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, accel_z),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, gyro_x),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, gyro_y),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, gyro_z),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, temperature),
        ATTR_GROUP_EXTRA_GEN(CLASS_DATA_RO_ATTR(mpu6050, all),
            NULL
        )))))))) 
    )

ATTR_GROUPS(mpu6050, group_mpu6050)
//ATTRIBUTE_GROUPS(group_mpu6050); 

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
/*static void my_class_create_release(struct class *cls)                                                           
{
    pr_debug("%s called for %s\n", __func__, cls->name);
    kfree(cls);
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
/*struct class *my_class_create(struct module *owner, const char *name)
{
    static struct lock_class_key key;
    struct class *cls;
    int retval;
    cls = kzalloc(sizeof(*cls), GFP_KERNEL);
    if (!cls) {
        retval = -ENOMEM;
        goto error;
    }
    cls->name = name;
    cls->owner = owner;
    cls->class_release = my_class_create_release;
    cls->class_groups = groups_mpu6050; 
    retval = __class_register(cls, key);
    if (retval)
        goto error;
    return cls;
error:
    kfree(cls);
    return ERR_PTR(retval);
    //__class_create(owner, name, &__key);
}
#define MY_CREATE_CLASS(name, goto_err) \
    ({ \
        name##_class = class_create( THIS_MODULE, #name ); \
        if( IS_ERR( name##_class ) ) { \
            int res = 0; \
            res = PTR_ERR(name##_class); \
            printk(KERN_ERR "Bad class create "#name"\n" ); \
            goto goto_err; \
        } \
        res; \
    })
    */
struct class mpu6050_class = {
	.name         = "mpu6050_class",
	.class_groups = groups_mpu6050,
	.owner        = THIS_MODULE,
};

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
static int mpu6050_init(void)
{
	int res;

	/* Create i2c driver */
	res = i2c_add_driver(&mpu6050_i2c_driver);
	if (res) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", res);
		return res;
	}
	printk(KERN_INFO "mpu6050: i2c driver created\n");

    //MY_CREATE_CLASS(mpu6050, create_class_err);
	res = class_register(&mpu6050_class);
	if (res) {
		pr_err("Unable to register mpu6050_class class\n");
		goto create_class_err;
	}    

    printk(KERN_INFO "mpu6050: sysfs class attributes created\n");

	printk(KERN_INFO "mpu6050: module loaded\n");

    return res;
create_class_err:
    //REMOVE_CLASS(mpu6050);
	//class_unregister(&mpu6050_class);
    i2c_del_driver(&mpu6050_i2c_driver);     
    return res;
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

static void mpu6050_exit(void)
{
	/*if (mpu6050_class)*/ {
		printk(KERN_INFO "mpu6050: sysfs class attributes removed\n");
        //REMOVE_CLASS(mpu6050);
        class_unregister(&mpu6050_class);
		printk(KERN_INFO "mpu6050: sysfs class destroyed\n");
	}

	i2c_del_driver(&mpu6050_i2c_driver);
	printk(KERN_INFO "mpu6050: i2c driver deleted\n");

	printk(KERN_INFO "mpu6050: module exited\n");
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------

module_init(mpu6050_init);
module_exit(mpu6050_exit);

