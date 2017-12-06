#ifndef __MPU6060__H__
#define __MPU6060__H__
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define NUM_MPU6050_DATA 7
struct mpu6050_data {
	struct i2c_client *drv_client;
	union {
		struct {
			int accel_values[3];
			int gyro_values[3];
			int temperature;
		};
		int data[NUM_MPU6050_DATA];
	};
	spinlock_t	lock;
};

#define TIMER_PERIOD   (1 * HZ)/2 /* 0.5 sec */

#define CLASS__ATTR_RO(_name, _data, func) {		\
	.attr	= {					\
		.name = __stringify(_data),		\
		.mode = S_IRUGO				\
	},						\
	.show	= func,					\
}

#define CLASS_DATA_RO(name, data, func) \
	struct class_attribute class_attr_##name##data = \
		CLASS__ATTR_RO(name, data, func);

#define GET_CLASS_DATA_RO_ATTR(name, data) \
	class_attr_##name##data.attr

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

#define ATTR_GROUPS(name, attr_group) \
	static const struct attribute_group *groups_##name[] = {\
		&attr_group, \
		NULL \
	};

#define ATTR_GET_GROUP(name) \
	group_##name
#define ATTR_GET_GROUPS(name) \
	groups_##name
// ----------------------------------------------------------------------------
/* https://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick
 * https://stackoverflow.com/questions/1872220/is-it-possible-to-iterate-over-arguments-in-variadic-macros
#define ATTR_GEN_NEW(name, data, func, ...) \
   CLASS_DATA_RO(name, data, func) \
   ATTR_GEN_NEW __VA_ARGS__

#define ATTR_EXTRA_GEN_NEW(name, data, func, next) \
	ATTR_GROUP_EXTRA_GEN(GET_CLASS_DATA_RO_ATTR(name, data), \
			ATTR_EXTRA_GEN_NEW(ATTR_GROUP_EXTRA_GEN(next)))

#define ATTR_TEXT(name, data, func, next) \
	name, data, func, next

#define ATTR_GROUP_NEW(group_name, attr_text) \
	ATTR_GEN_NEW(attr_text); \
	static struct attribute *attrs_##group_name##_extra[] = { \
		ATTR_EXTRA_GEN_NEW(attr_text) \
	}; \
	static struct attribute_group group_##group_name = { \
		.attrs =  attrs_##group_name##_extra, \
	};

*/

// ----------------------------------------------------------------------------
// Reading data from I2C2 device to the static mpu6050_data structure
// ----------------------------------------------------------------------------
static int mpu6050_read_data(void);
// ----------------------------------------------------------------------------
// probe fucntion for device driver
// ----------------------------------------------------------------------------
static int mpu6050_probe(struct i2c_client *drv_client,
			 const struct i2c_device_id *id);
// ----------------------------------------------------------------------------
// remove fucntion for device driver
// ----------------------------------------------------------------------------
static int mpu6050_remove(struct i2c_client *drv_client);
// ----------------------------------------------------------------------------
// show function for sysfs
// ----------------------------------------------------------------------------
static ssize_t mpu6050_show(struct class *class, struct class_attribute *attr,
		char *buf);
// ----------------------------------------------------------------------------
// Init module function
// ----------------------------------------------------------------------------
static int mpu6050_init(void);
// ----------------------------------------------------------------------------
// Exit module function
// ----------------------------------------------------------------------------
static void mpu6050_exit(void);
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void timer_callback(unsigned long data);
static int init_driver_timer(void);
static void deinit_driver_timer(void);
static void work_func(struct work_struct *unused);
static ssize_t mpu6050_dev_read(struct file *file, char *buf, size_t count,
		loff_t *ppos);
#endif
