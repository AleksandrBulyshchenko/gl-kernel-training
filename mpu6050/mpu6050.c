#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include "mpu6050-regs.h"

#define IMU_BUFFFSIZE 10

// IMU raw data structure
struct mpu6050_s{
	struct i2c_client *drv_client;
	u32 ts_ms;
	s16 acc_raw[3];
	s16 gyr_raw[3];
	s16 acc_scale;
	s16 gyr_scale;
	s16 temperature;
};

// IMU element of buffer
struct imu_buff_e{
	u32 ts_ms;
	s16 acc_mg[3];
	s16 gyr_dgps[3];
};

// IMU buffer
struct imu_buffer_s{
	struct imu_buff_e buff[IMU_BUFFFSIZE];
	s16 size;
	s16 r_ctr;
	s16 w_ctr;
} buffer = {.size = IMU_BUFFFSIZE, .r_ctr = 0, .w_ctr = 0};

static struct mpu6050_s g_mpu6050_data;

static int majorNumber;
static struct class *attr_class;
static struct device *Device_rt, *Device_b;

static struct task_struct *thread_tmr;
//static struct timer_list imu_upd_tmr;

static u16 buff_data_len(struct imu_buffer_s *b)
{
	u16 retval = 0;
	if (b->w_ctr > b->r_ctr) {
		retval = b->w_ctr-b->r_ctr;
	} else if (b->w_ctr < b->r_ctr) {
		retval = b->size-b->r_ctr+b->w_ctr;
	}
	return retval;
}

static void buff_rctr_inc(struct imu_buffer_s *b)
{
	if (b->r_ctr >= (b->size-1)) {
		b->r_ctr = 0;
	} else {
		b->r_ctr++;
	}
}

static void buff_wctr_inc(struct imu_buffer_s *b)
{
	if (b->w_ctr >= (b->size-1)) {
		b->w_ctr = 0;
	} else {
		b->w_ctr++;
	}
}

void imu_buff_wr(struct imu_buff_e *data)
{
	u8 i = 0;

	if (buffer.size == (buff_data_len(&buffer) + 1)) {
		// Buffer is full, replace r_ctr
		buff_rctr_inc(&buffer);
	}
	// Write the data
	for (; i < 3; i++) {
		buffer.buff[buffer.w_ctr].acc_mg[i] = data->acc_mg[i];
		buffer.buff[buffer.w_ctr].gyr_dgps[i] = data->gyr_dgps[i];
		buffer.buff[buffer.w_ctr].ts_ms = data->ts_ms;
	}

	// Increment write counter
	buff_wctr_inc(&buffer);
}

bool imu_buff_rd(struct imu_buff_e *data)
{
	u8 i = 0;
	if (buff_data_len(&buffer)) {
		for (; i < 3; i++) {
			data->acc_mg[i] = buffer.buff[buffer.r_ctr].acc_mg[i];
			data->gyr_dgps[i] = buffer.buff[buffer.r_ctr].gyr_dgps[i];
		}
		data->ts_ms = buffer.buff[buffer.r_ctr].ts_ms;

		buff_rctr_inc(&buffer);

		return true;
	}
	return false;
}

static int mpu6050_update(struct mpu6050_s *imu)
{
	int temp;

	if (imu->drv_client == 0)
		return -ENODEV;

	/* accel */
	imu->acc_raw[0] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_ACCEL_XOUT_H));
	imu->acc_raw[1] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_ACCEL_YOUT_H));
	imu->acc_raw[2] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	imu->gyr_raw[0] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_GYRO_XOUT_H));
	imu->gyr_raw[1] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_GYRO_YOUT_H));
	imu->gyr_raw[2] = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_GYRO_ZOUT_H));

	imu->ts_ms = jiffies_to_msecs(jiffies);

	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(imu->drv_client, REG_TEMP_OUT_H));
	imu->temperature = (temp + 12420 + 170) / 340;

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

	g_mpu6050_data.drv_client = drv_client;
	g_mpu6050_data.acc_scale = 16383;
	g_mpu6050_data.gyr_scale = 131;

	printk(KERN_INFO "mpu6050: i2c mpu6050 device found, WHO_AM_I register value = 0x%X\n", ret);

	/* Setup the device */
	/* No error handling here! */
	i2c_smbus_write_byte_data(drv_client, REG_CONFIG, 0 | 0b00000001); // Enable DLPF, sampleFrq = 1000Hz
	i2c_smbus_write_byte_data(drv_client, REG_GYRO_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_ACCEL_CONFIG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_FIFO_EN, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_PIN_CFG, 0);
	i2c_smbus_write_byte_data(drv_client, REG_INT_ENABLE, 0);
	i2c_smbus_write_byte_data(drv_client, REG_USER_CTRL, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_1, 0);
	i2c_smbus_write_byte_data(drv_client, REG_PWR_MGMT_2, 0);

	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}

void timu_upd_tmr_handler(unsigned long data)
{

	struct imu_buff_e s;
	u8 i = 0;

	// Update imu data in case, if they are old:
	if (jiffies_to_msecs(jiffies)-g_mpu6050_data.ts_ms > 100) {
		mpu6050_update(&g_mpu6050_data);
	}

	for (; i < 3; i++) {
		printk("here\n");
		s.acc_mg[i] = (s32)(g_mpu6050_data.acc_raw[i])*1000/g_mpu6050_data.acc_scale;
		s.gyr_dgps[i] = g_mpu6050_data.gyr_raw[i]/g_mpu6050_data.gyr_scale;
	}
	s.ts_ms = g_mpu6050_data.ts_ms;

	imu_buff_wr(&s);

	/*if (mod_timer(&imu_upd_tmr, jiffies + msecs_to_jiffies(1000))) {
		printk("mpu6050: Timer starting fault/n");
	}*/

}

int thread_hand(void *data)
{
	struct completion *finished = (struct completion *)data;

	while (!kthread_should_stop()) {
		timu_upd_tmr_handler(0);
		msleep(1000);
	}
	complete(finished);
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client)
{
	g_mpu6050_data.drv_client = 0;

	printk(KERN_INFO "mpu6050: i2c driver removed\n");
	return 0;
}

static const struct i2c_device_id mpu6050_idtable[] = {
{ "gl_mpu6050", 0 },
{ }
};

MODULE_DEVICE_TABLE(i2c, mpu6050_idtable);

static struct i2c_driver mpu6050_i2c_driver = {
	.driver = {
		.name = "gl_mpu6050",
	},

	.probe = mpu6050_probe,
	.remove = mpu6050_remove,
	.id_table = mpu6050_idtable,
};

static int dev_uevent_h(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0440);
	return 0;
}


static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	char message[100];
	u16 mlen = 0, ncplen = 0;

	u8 inode = iminor(filep->f_path.dentry->d_inode);

	if (inode == 1) {

		struct imu_buff_e rdata;

		if (imu_buff_rd(&rdata)) {
			sprintf(message, "ts=%i;acc=%i:%i:%i;gyr=%i:%i:%i\n",
					rdata.ts_ms,
					rdata.acc_mg[0],
					rdata.acc_mg[1],
					rdata.acc_mg[2],
					rdata.gyr_dgps[0],
					rdata.gyr_dgps[1],
					rdata.gyr_dgps[2]);

			mlen = strlen(message);

			ncplen = copy_to_user(buffer, message, mlen);
		}
	} else if (inode == 0) {

		mpu6050_update(&g_mpu6050_data);

		sprintf(message, "ts=%i;acc=%d:%d:%d;gyr=%i:%i:%i\n",
				g_mpu6050_data.ts_ms,
				((s32)g_mpu6050_data.acc_raw[0])*1000/g_mpu6050_data.acc_scale,
				((s32)g_mpu6050_data.acc_raw[1])*1000/g_mpu6050_data.acc_scale,
				((s32)g_mpu6050_data.acc_raw[2])*1000/g_mpu6050_data.acc_scale,
				g_mpu6050_data.gyr_raw[0]/g_mpu6050_data.gyr_scale,
				g_mpu6050_data.gyr_raw[1]/g_mpu6050_data.gyr_scale,
				g_mpu6050_data.gyr_raw[2]/g_mpu6050_data.gyr_scale);

		mlen = strlen(message);

		ncplen = copy_to_user(buffer, message, mlen);
	}

	return mlen-ncplen;
}

static struct file_operations fops = {
	.read = dev_read,
};

DECLARE_COMPLETION(finished);

static int mpu6050_init(void)
{
	int ret;

	/* Create i2c driver */
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", ret);
		goto drv_err;
	}
	printk(KERN_INFO "mpu6050: i2c driver created\n");

	majorNumber = register_chrdev(0, "mpu6050", &fops);
	if (majorNumber < 0) {
		printk(KERN_INFO "failed to register a major number\n");
		goto chrdev_err;
	}


	/* Create class */
	attr_class = class_create(THIS_MODULE, "mpu6050");
	if (IS_ERR(attr_class)) {
		ret = PTR_ERR(attr_class);
		printk(KERN_INFO "mpu6050: failed to create sysfs class: %d\n", ret);
		goto class_cr_err;
	}

	attr_class->dev_uevent = dev_uevent_h;
	printk(KERN_INFO "mpu6050: sysfs class created\n");

	// Register the device driver
	Device_rt = device_create(attr_class, NULL, MKDEV(majorNumber, 0), NULL, "mpu6050!imu_rt");
	if (IS_ERR(Device_rt)) {
		printk(KERN_INFO "Failed to create the device\n");
		goto devrt_cr_err;
	}

	Device_b = device_create(attr_class, NULL, MKDEV(majorNumber, 1), NULL, "mpu6050!imu_b");
	if (IS_ERR(Device_b)) {
		printk(KERN_INFO "Failed to create the device\n");
		goto devb_cr_err;
	}

	thread_tmr = kthread_run(thread_hand, &finished, "timer_thread");
	if (IS_ERR(thread_tmr)) {
		printk(KERN_INFO "Failed to create the thread\n");
		goto thr_cr_err;
	}

	/* Timer was before used for reading imu data (it doesn't work)
	 * Now timers task is handled by kthread
	 * */

	/*setup_timer(&imu_upd_tmr, timu_upd_tmr_handler, 0);
	if (mod_timer(&imu_upd_tmr, jiffies + msecs_to_jiffies(1000))) {
		printk(KERN_INFO "Failed to create the timer\n");
		goto err_tmr;
	}*/

	printk(KERN_INFO "mpu6050: module loaded\n");

	return 0;

//err_tmr:
//	del_timer(&imu_upd_tmr);
thr_cr_err:
	device_destroy(attr_class, MKDEV(majorNumber, 1));
devb_cr_err:
	device_destroy(attr_class, MKDEV(majorNumber, 0));
devrt_cr_err:
	class_destroy(attr_class);
class_cr_err:
	unregister_chrdev(majorNumber, "mpu6050!imu_rt");
chrdev_err:
	i2c_del_driver(&mpu6050_i2c_driver);
drv_err:
	return -1;
}

static void mpu6050_exit(void)
{
//	del_timer(&imu_upd_tmr);
	kthread_stop(thread_tmr);
	wait_for_completion(&finished);
	device_destroy(attr_class, MKDEV(majorNumber, 0));
	device_destroy(attr_class, MKDEV(majorNumber, 1));
	class_destroy(attr_class);
	unregister_chrdev(majorNumber, "mpu6050!imu_rt");
	i2c_del_driver(&mpu6050_i2c_driver);

	printk(KERN_INFO "mpu6050: module exited\n");
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_AUTHOR("Andriy.Khulap <andriy.khulap@globallogic.com>"
			  "Serhii.Rybalko <rybalko.serhii.o@gmail.com>");
MODULE_DESCRIPTION("mpu6050 I2C acc&gyro");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
