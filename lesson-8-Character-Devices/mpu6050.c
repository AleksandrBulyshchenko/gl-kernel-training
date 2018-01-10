#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "mpu6050-regs.h"
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/fs.h>

#define BUF_SIZE  10

static struct i2c_client *driver_client;

struct mpu6050_data {
	int accel_values[3];
	int gyro_values[3];
	int temperature;
	u64 timestamp;
};

static struct mpu6050_data g_mpu6050_data[BUF_SIZE];
static char number = 0;

static int mpu6050_read_data(struct mpu6050_data* p) {
	int temp;

	if (driver_client == 0)
		return -ENODEV;

	/* accel */
	p->accel_values[0] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_ACCEL_XOUT_H));
	p->accel_values[1] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_ACCEL_YOUT_H));
	p->accel_values[2] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	p->gyro_values[0] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_GYRO_XOUT_H));
	p->gyro_values[1] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_GYRO_YOUT_H));
	p->gyro_values[2] = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_GYRO_ZOUT_H));
	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)(
			(u16) i2c_smbus_read_word_swapped(driver_client, REG_TEMP_OUT_H));
	p->temperature = (temp + 12420 + 170) / 340;
	p->timestamp = jiffies;
	return 0;
}

static int mpu6050_probe(struct i2c_client *drv_client,
		const struct i2c_device_id *id) {
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

	driver_client = drv_client;

	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client) {
	driver_client = 0;
	printk(KERN_INFO "mpu6050: i2c driver removed\n");
	return 0;
}

static const struct i2c_device_id mpu6050_idtable[] =
		{ { "gl_mpu6050", 0 }, { } };

MODULE_DEVICE_TABLE( i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = { .driver =
		{ .name = "gl_mpu6050", },

.probe = mpu6050_probe, .remove = mpu6050_remove, .id_table = mpu6050_idtable, };

static int major = 0;
module_param( major, int, S_IRUGO);

static ssize_t dev_read(struct file * file, char * buf, size_t count,
		loff_t *ppos) {
	char b[100];
	int len = 0;
	int last = number - 1;
	if (last < 0) {
		last = BUF_SIZE - 1;
	}
	sprintf(b,
			"{\"temp\": \"%d\",\"ax\": \"%d\",\"ay\": \"%d\",\"az\": \"%d\",\"gx\": \"%d\",\"gy\": \"%d\",\"gz\": \"%d\"}\n",
			g_mpu6050_data[last].temperature,
			g_mpu6050_data[last].accel_values[0],
			g_mpu6050_data[last].accel_values[1],
			g_mpu6050_data[last].accel_values[2],
			g_mpu6050_data[last].gyro_values[0],
			g_mpu6050_data[last].gyro_values[1],
			g_mpu6050_data[last].gyro_values[2]);

	len = strlen(b);
	printk( KERN_INFO "=== read : %ld\n", (long)count );
	printk( KERN_INFO "=== len : %ld\n", (long)len );
	if (count < len)
		return -EINVAL;
	if (*ppos != 0) {
		printk( KERN_INFO "=== read return : 0\n" );  // EOF
		return 0;
	}
	if (copy_to_user(buf, b, len))
		return -EINVAL;
	*ppos = len;
	printk( KERN_INFO "=== read return : %d\n", len );
	return len;
}

static ssize_t dev_all(struct file * file, char * buf, size_t count,
		loff_t *ppos) {
	char b[100];
	int len = 0;
	int i = 0;
	char string[1000] = "";
	for (; i < BUF_SIZE; i++) {
		sprintf(b, "%llu:gyro=%d,%d,%d acc=%d,%d,%d\n",
				g_mpu6050_data[i].timestamp, g_mpu6050_data[i].gyro_values[0],
				g_mpu6050_data[i].gyro_values[1],
				g_mpu6050_data[i].gyro_values[2],
				g_mpu6050_data[i].accel_values[0],
				g_mpu6050_data[i].accel_values[1],
				g_mpu6050_data[i].accel_values[2]);
		strcat(string, b);
	}

	len = strlen(string);
	printk( KERN_INFO "=== read : %ld\n", (long)count );
	printk( KERN_INFO "=== len : %ld\n", (long)len );
	if (count < len)
		return -EINVAL;
	if (*ppos != 0) {
		printk( KERN_INFO "=== read return : 0\n" );  // EOF
		return 0;
	}
	if (copy_to_user(buf, string, len))
		return -EINVAL;
	*ppos = len;
	printk( KERN_INFO "=== read return : %d\n", len );
	return len;
}

static const struct file_operations dev_fops = { .owner = THIS_MODULE, .read =
		dev_all, };

static const struct file_operations dev_fops1 = { .owner = THIS_MODULE, .read =
		dev_read, };

#define DEVICE_FIRST  0
#define DEVICE_COUNT  2
#define MODNAME "mpu6050_mod"
#define DEVNAME "dyn"

static struct cdev hcdev;
static struct cdev hcdev1;
static struct class *devclass;

static struct file *f;
#define SCREEN_NAME "/dev/tty0"

int thread_func(void *a) {
	ssize_t n = 0;
	loff_t offset = 0;
	char b[100]="hi how are you?\n";
	while (true) {
		if (++number >= BUF_SIZE) {
			number = 0;
		}
		ssleep(1);
		if (kthread_should_stop()) {
			do_exit(0);
		}
		mpu6050_read_data(&g_mpu6050_data[number]);

		sprintf(b, "\r\ntemperature=%d\r\ngyroX=%d\r\ngyroY=%d\r\ngyroZ=%d\r\naccX=%d\r\naccY=%d\r\naccZ =%d\r\n",
		 
				g_mpu6050_data[number].temperature,
				g_mpu6050_data[number].gyro_values[0],
				g_mpu6050_data[number].gyro_values[1],
				g_mpu6050_data[number].gyro_values[2],
				g_mpu6050_data[number].accel_values[0],
				g_mpu6050_data[number].accel_values[1],
				g_mpu6050_data[number].accel_values[2]);
 		f = filp_open( SCREEN_NAME, O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR ); 		
 		if( IS_ERR( f ) ) { 
			printk( "! file open failed: %s\n", SCREEN_NAME );
		}
		if( ( n = vfs_write( f, b, strlen( b ), &offset ) ) != strlen( b ) ) {
			printk( "! failed to write: %d\n", n );
		}
		filp_close( f, NULL );
	}

	return 0;
}

static struct task_struct *thread;

static int mpu6050_init(void) {
	int ret, i;
	dev_t dev;
	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "mpu6050: i2c driver created\n");
	/* Create files */
	if (major != 0) {
		dev = MKDEV(major, DEVICE_FIRST);
		ret = register_chrdev_region(dev, DEVICE_COUNT, MODNAME);
	} else {
		ret = alloc_chrdev_region(&dev, DEVICE_FIRST, DEVICE_COUNT, MODNAME);
		major = MAJOR(dev);
	}
	if (ret < 0) {
		printk( KERN_ERR "=== Can not register char device region\n" );
		goto err;
	}
	cdev_init(&hcdev, &dev_fops);
	cdev_init(&hcdev1, &dev_fops1);
	hcdev.owner = THIS_MODULE;
	ret = cdev_add(&hcdev, dev, 1);
	ret = cdev_add(&hcdev1, dev, 2);
	if (ret < 0) {
		unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);
		printk( KERN_ERR "=== Can not add char device\n" );
		goto err;
	}
	devclass = class_create(THIS_MODULE, "dyn_class");
	for (i = 0; i < DEVICE_COUNT; i++) {
		dev = MKDEV(major, DEVICE_FIRST + i);
		device_create(devclass, NULL, dev, NULL, "%s_%d", DEVNAME, i);
	}
	/* Create thread */
	thread = kthread_create(thread_func, NULL, "mpu6050");
	if ((thread)) {
		wake_up_process(thread);
	}
	printk( KERN_INFO "======== module installed %d:[%d-%d] ===========\n",
			MAJOR( dev ), DEVICE_FIRST, MINOR( dev ) );
	err: return ret;
}

static void mpu6050_exit(void) {
	dev_t dev;
	int i;
	for (i = 0; i < DEVICE_COUNT; i++) {
		dev = MKDEV(major, DEVICE_FIRST + i);
		device_destroy(devclass, dev);
	}
	class_destroy(devclass);
	cdev_del(&hcdev);
	unregister_chrdev_region(MKDEV(major, DEVICE_FIRST), DEVICE_COUNT);

	i2c_del_driver(&mpu6050_i2c_driver);
	printk(KERN_INFO "mpu6050: i2c driver deleted\n");

	if(!kthread_stop(thread))
		printk(KERN_INFO "Thread stopped");

	printk(KERN_INFO "mpu6050: module exited\n");
}

module_init( mpu6050_init);
module_exit( mpu6050_exit);

MODULE_AUTHOR("Aleksandr Nikitin <supersashanikitin@gmail.com>");
MODULE_DESCRIPTION("mpu6050 I2C acc&gyro");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
