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
	int counter;	//counter of reading from device
};

#define NUM_MPU6050_SELF_TEST_DATA 6
struct mpu6050_self_test_data {
	union {		/*values from Registers 13 to 16 – Self Test Registers*/
		struct {
			int accel_values[3];
			int gyro_values[3];
		};
		int data[NUM_MPU6050_SELF_TEST_DATA];
	} self_test;
	union {		/*values in test mode*/
		struct {
			int accel_values[3];
			int gyro_values[3];
		};
		int data[NUM_MPU6050_SELF_TEST_DATA];
	} test_value;
	union {			/*values in normal mode*/
		struct {
			int accel_values[3];
			int gyro_values[3];
		};
		int data[NUM_MPU6050_SELF_TEST_DATA];
	} value;
};

/* 0.1 sec */
#define TIMER_PERIOD   ((1 * HZ)/10)

#define CLASS__ATTR_RO(_name, _data, func) {\
	.attr	= {\
		.name = __stringify(_data),\
		.mode = 0444\
	},\
	.show	= func,\
}

#define CLASS_DATA_RO(name, data, func)\
	struct class_attribute class_attr_##name##data =\
		CLASS__ATTR_RO(name, data, func);

#define GET_CLASS_DATA_RO_ATTR(name, data)\
	class_attr_##name##data.attr

#define ATTR_GROUP_EXTRA_GEN(current, next)\
	&current,\
	next

#define ATTR_GROUP(_name, attr_extra)\
	static struct attribute *attrs_##_name##_extra[] = {\
		attr_extra\
	};\
	static struct attribute_group group_##_name = {\
		.attrs =  attrs_##_name##_extra,\
	};

#define ATTR_GROUPS(name, attr_group)\
	static const struct attribute_group *groups_##name[] = {\
		&attr_group,\
		NULL\
	};

#define ATTR_GET_GROUP(name)\
	group_##name
#define ATTR_GET_GROUPS(name)\
	groups_##name

static int mpu6050_read_data(void);
static int mpu6050_self_test(struct i2c_client *drv_client);
static int mpu6050_probe(struct i2c_client *drv_client,
			 const struct i2c_device_id *id);
static int mpu6050_remove(struct i2c_client *drv_client);
static ssize_t mpu6050_show(struct class *class, struct class_attribute *attr,
		char *buf);
static int mpu6050_init(void);
static void mpu6050_exit(void);
static void timer_callback(unsigned long data);
static int init_driver_timer(void);
static void deinit_driver_timer(void);
static ssize_t mpu6050_dev_read(struct file *file, char *buf, size_t count,
		loff_t *ppos);
static long mpu6050_dev_ioctl(struct file *, unsigned int, unsigned long);
static int mpu6050_thread(void *param);
#endif
