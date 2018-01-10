#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/kthread.h>
#include <linux/sched.h> 
#include <linux/delay.h>
#include <linux/rtmutex.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#include <linux/rtc.h>
#include "mpu6050-regs.h"

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Ruslan V. Sukhov <sukhov@karazin.ua>" );
MODULE_VERSION( "1.1" );

struct mpu6050_data {
	struct timespec t;
	int accel_values[3];
	int gyro_values[3];
	int temperature;
	
};

#define MPU6050_BUF_SIZE 10
struct mpu6050_buf_data {
	unsigned int size;
	unsigned int read_index;
	unsigned int write_index;
	unsigned int count;
	struct mpu6050_data* buf;
};

struct mpu6050_dev_context {
	struct cdev cdev;
	struct class* devclass;	
	struct i2c_client* dev_client;
	struct mpu6050_buf_data* buf_data;
	struct task_struct* read_task;
	struct rt_mutex buf_mutex;
	struct rt_mutex dev_current_mutex;
	struct rt_mutex dev_all_mutex; 
}* context;

static struct mpu6050_buf_data* mpu6050_buf_init(void) {
	struct mpu6050_buf_data* bd;
	bd = kmalloc(sizeof(struct mpu6050_buf_data),GFP_KERNEL);
	if (!bd) 
	{
		printk(KERN_ERR "mpu6050: mpu6050_buf_data memory allocation error");
		return 0;
	}
	bd->size = MPU6050_BUF_SIZE;
	bd->read_index = 0;
	bd->write_index = 0;
	bd->count = 0;

	bd->buf = 0;
	bd->buf = kmalloc(sizeof(struct mpu6050_data) * (bd->size),GFP_KERNEL);
	if (!(bd->buf)) {
		kfree(bd);
		printk(KERN_ERR "mpu6050: mpu6050_buf memory allocation error");
		return 0;
	}
	return bd;
}

static void mpu6050_buf_deinit(struct mpu6050_buf_data* bd)
{
	kfree(bd->buf);
	kfree(bd);
	bd = 0;
	return;
}

static int mpu6050_buf_add(struct mpu6050_buf_data* bd, struct mpu6050_data* d)
{
	int i;
	for (i = 0; i < 3; i++)	{
		(bd->buf[bd->write_index]).accel_values[i] = (d->accel_values)[i];
		(bd->buf[bd->write_index]).gyro_values[i] = (d->gyro_values)[i];	
	}
	(bd->buf[bd->write_index]).temperature = d->temperature;
	(bd->buf[bd->write_index]).t = d->t;
	
	if (bd->count < bd->size) {
		(bd->write_index)++;
		if (bd->write_index == bd->size) bd->write_index = 0;
		(bd->count)++;	
	}
	else {
		(bd->write_index)++;
		(bd->read_index)++;
		if (bd->write_index == bd->size) bd->write_index = 0;
		if (bd->read_index == bd->size) bd->read_index = 0;
	}	
	return 0;
}

static int mpu6050_buf_get(struct mpu6050_buf_data* bd, struct mpu6050_data* d)
{
	if (bd->count > 0) {
		int i;
		for (i = 0; i < 3; i++)	{
			(d->accel_values)[i] = (bd->buf[bd->read_index]).accel_values[i];
			(d->gyro_values)[i] = (bd->buf[bd->read_index]).gyro_values[i];	
		}
		d->temperature = (bd->buf[bd->read_index]).temperature;
		d->t = (bd->buf[bd->read_index]).t;
		(bd->read_index)++;
		if (bd->read_index == bd->size) bd->read_index = 0;
		(bd->count)--;
	}
	return 0;
}

static int mpu6050_read_data(struct mpu6050_data* p_mpu6050_data)
{
	int temp;

	if (context->dev_client == 0)
		return -ENODEV;

	/* accel */
	p_mpu6050_data->accel_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_ACCEL_XOUT_H));
	p_mpu6050_data->accel_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_ACCEL_YOUT_H));
	p_mpu6050_data->accel_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_ACCEL_ZOUT_H));
	/* gyro */
	p_mpu6050_data->gyro_values[0] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_GYRO_XOUT_H));
	p_mpu6050_data->gyro_values[1] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_GYRO_YOUT_H));
	p_mpu6050_data->gyro_values[2] = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_GYRO_ZOUT_H));
	/* temp */
	/* Temperature in degrees C = (TEMP_OUT Register Value  as a signed quantity)/340 + 36.53 */
	temp = (s16)((u16)i2c_smbus_read_word_swapped(context->dev_client, REG_TEMP_OUT_H));
	p_mpu6050_data->temperature = (temp + 12420 + 170) / 340;
	p_mpu6050_data->t = current_kernel_time();
	
	printk(KERN_INFO "mpu6050: sensor data read:\n");
	printk(KERN_INFO "ACC = %d; %d; %d\n",
		p_mpu6050_data->accel_values[0],
		p_mpu6050_data->accel_values[1],
		p_mpu6050_data->accel_values[2]);
	printk(KERN_INFO "GYRO = %d; %d; %d\n",
		p_mpu6050_data->gyro_values[0],
		p_mpu6050_data->gyro_values[1],
		p_mpu6050_data->gyro_values[2]);
	printk(KERN_INFO "TEMP = %d\n", p_mpu6050_data->temperature);
	return 0;
}

static int period_ms = 1000;
module_param( period_ms, int, S_IRUGO );
#define MPU6050_THREAD_NAME "mpu6050_read_thread"
static int mpu6050_read_thread( void* data ) {
	struct mpu6050_data tmp_data;
	struct timespec start_time, stop_time;
	for(;;) {
		start_time = current_kernel_time();
		set_current_state(TASK_UNINTERRUPTIBLE); 
		if(kthread_should_stop()) break;
		mpu6050_read_data(&tmp_data);
		rt_mutex_lock(&context->buf_mutex);
		mpu6050_buf_add(context->buf_data, &tmp_data);
		rt_mutex_unlock(&context->buf_mutex);
		stop_time = current_kernel_time();
		msleep(period_ms - (stop_time.tv_nsec - start_time.tv_nsec)/1000000);	
	}
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

	context->dev_client = drv_client;

	printk(KERN_INFO "mpu6050: i2c driver probed\n");
	return 0;
}

static int mpu6050_remove(struct i2c_client *drv_client)
{
	context->dev_client = 0;

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

#define MPU6050_STR_SIZE 100
static char mpu6050_str[MPU6050_STR_SIZE];

#define DEVICE_FIRST  0
#define DEVICE_COUNT  2

#define DEVICE_CURRENT_NUMBER  0
#define DEVICE_ALL_NUMBER 1

static int major = 0;
module_param( major, int, S_IRUGO );

static int dev_open(struct inode *inode, struct file *file)
{
	int minor = MINOR(inode->i_rdev);
	struct mpu6050_dev_context *ctx = container_of(inode->i_cdev, struct mpu6050_dev_context, cdev);
	switch (minor) {
	case DEVICE_CURRENT_NUMBER: 
		if (rt_mutex_trylock(&ctx->dev_current_mutex) == 0) {
			printk(KERN_INFO "mpu6050: %d minor device is busy\n", minor);
			return -EBUSY;
		}
		break;
	case DEVICE_ALL_NUMBER: 
		if (rt_mutex_trylock(&ctx->dev_all_mutex) == 0) {
			printk(KERN_INFO "mpu6050: %d minor device is busy\n", minor);
			return -EBUSY;
		}
		break;
	default:
		printk(KERN_ERR "mpu6050: unknown minor number: %d\n", minor);
		return -EFAULT;
	}
	printk(KERN_INFO "mpu6050: %d minor device is locked\n", minor);
	file->private_data = ctx;
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	int minor = MINOR(inode->i_rdev);
	struct mpu6050_dev_context *ctx = file->private_data;
	switch (minor) {
	case DEVICE_CURRENT_NUMBER: 
		rt_mutex_unlock(&ctx->dev_current_mutex);
		break;
	case DEVICE_ALL_NUMBER: 
		rt_mutex_unlock(&ctx->dev_all_mutex);
		break;
	default:
		printk(KERN_ERR "mpu6050: unknown minor number: %d\n", minor);
		return -EFAULT;
	}
	printk(KERN_INFO "mpu6050: %d minor device is unlocked\n", minor);
	return 0;
}

static ssize_t dev_current_read( struct file * file, char * buf,size_t count, loff_t *ppos ) {
	struct mpu6050_dev_context *ctx = file->private_data;
	struct mpu6050_buf_data * p_buf_data = ctx->buf_data;
	struct mpu6050_data tmp_data;
	unsigned long local_time;
	struct rtc_time rtc_t;

	int len = 0;
	memset((void*)mpu6050_str, 0, MPU6050_STR_SIZE);
	rt_mutex_lock(&ctx->buf_mutex);
	printk(KERN_INFO "mpu6050: sample buffer is locked\n");
	if (p_buf_data->count > 0) {
		mpu6050_buf_get(p_buf_data, &tmp_data);
		local_time = (u32)(tmp_data.t.tv_sec - (sys_tz.tz_minuteswest * 60));
		rtc_time_to_tm(local_time, &rtc_t);
		sprintf(mpu6050_str, "%04d-%02d-%02d %02d:%02d:%02d:%03d\nACC = %d; %d; %d\nGYRO = %d; %d; %d;\nT = %d\n", 
			rtc_t.tm_year + 1900, rtc_t.tm_mon + 1, rtc_t.tm_mday, 
			rtc_t.tm_hour, rtc_t.tm_min, rtc_t.tm_sec, tmp_data.t.tv_nsec / 1000000,
			tmp_data.accel_values[0], tmp_data.accel_values[1], tmp_data.accel_values[2], 
			tmp_data.gyro_values[0], tmp_data.gyro_values[1], tmp_data.gyro_values[2],
			tmp_data.temperature);
		len = strlen( mpu6050_str );
	}
	rt_mutex_unlock(&ctx->buf_mutex);
	printk(KERN_INFO "mpu6050: sample buffer is unlocked\n");
	if( count < len ) return -EINVAL;
	if( *ppos != 0 ) return 0;
	if( copy_to_user( buf, mpu6050_str, len ) ) return -EINVAL;
	*ppos = len;
	return len;
}

static ssize_t dev_all_read( struct file * file, char * buf,size_t count, loff_t *ppos ) {
	struct mpu6050_dev_context *ctx = file->private_data;
	struct mpu6050_buf_data * p_buf_data = ctx->buf_data;
	struct mpu6050_data tmp_data;
	int offset = 0, len = 0, i, tmp;
	unsigned long local_time;
	struct rtc_time rtc_t;
	rt_mutex_lock(&ctx->buf_mutex);
	printk(KERN_INFO "mpu6050: sample buffer is locked\n");
	tmp = p_buf_data->count;
	memset((void*)mpu6050_str, 0, MPU6050_STR_SIZE);
	sprintf(mpu6050_str,"Buffer contains %d samples:\n", tmp);
	len = strlen( mpu6050_str );

	offset += len;
	if( copy_to_user( buf, mpu6050_str, len ) ) return -EINVAL;
	for(i = 0; i < tmp; i++) {
		len = 0;
		memset((void*)mpu6050_str, 0, MPU6050_STR_SIZE);
		mpu6050_buf_get(p_buf_data, &tmp_data);
		local_time = (u32)(tmp_data.t.tv_sec - (sys_tz.tz_minuteswest * 60));
		rtc_time_to_tm(local_time, &rtc_t);
		sprintf(mpu6050_str, "%04d-%02d-%02d %02d:%02d:%02d:%03d\nACC = %d; %d; %d\nGYRO = %d; %d; %d;\nT = %d\n", 
			rtc_t.tm_year + 1900, rtc_t.tm_mon + 1, rtc_t.tm_mday, 
			rtc_t.tm_hour, rtc_t.tm_min, rtc_t.tm_sec, tmp_data.t.tv_nsec / 1000000,
			tmp_data.accel_values[0], tmp_data.accel_values[1], tmp_data.accel_values[2], 
			tmp_data.gyro_values[0], tmp_data.gyro_values[1], tmp_data.gyro_values[2],
			tmp_data.temperature);
		len = strlen( mpu6050_str );
		if( copy_to_user( buf + offset, mpu6050_str, len ) ) return -EINVAL;
		offset += len;
	}
	rt_mutex_unlock(&ctx->buf_mutex);
	printk(KERN_INFO "mpu6050: sample buffer is unlocked\n");
	if( count < offset ) return -EINVAL;
	if( *ppos != 0 ) return 0;
	*ppos = offset;
	return offset;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t size, loff_t *pos) {
	struct inode *inode = file_inode(file);
	int minor = MINOR(inode->i_rdev);
	struct mpu6050_dev_context *ctx = file->private_data;

	if (!(ctx->dev_client)) {
		printk(KERN_INFO "mpu6050: i2c driver not probed\n");
		return -EIO;
	}

	switch (minor) {

	case DEVICE_CURRENT_NUMBER:
		return dev_current_read(file, buf, size, pos);

	case DEVICE_ALL_NUMBER:
		return dev_all_read(file, buf, size, pos);

	default:
		printk(KERN_ERR "mpu6050: unknown minor number: %d\n", minor);
		return -EFAULT;
	}
}

static const struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,	
	.read  = dev_read,
};

static char* DEVICE_FILE_SUFFIX[DEVICE_COUNT] = {
	"current",
	"all"
};

#define MOD_NAME "mpu6050_mod"

static int mpu6050_dev_init(void) {
	int ret, i;
	dev_t dev;

	context = kmalloc(sizeof(struct mpu6050_dev_context), GFP_KERNEL);
	if (!context) {
		ret = -ENOMEM;
		goto err;
	}
	ret = i2c_add_driver(&mpu6050_i2c_driver);
	if (ret < 0) {
		printk(KERN_ERR "mpu6050: failed to add new i2c driver: %d\n", ret);
		goto err;
	}
	printk(KERN_INFO "mpu6050: i2c driver created\n");	

	if( major != 0 ) {
		dev = MKDEV( major, DEVICE_FIRST );
		ret = register_chrdev_region( dev, DEVICE_COUNT, MOD_NAME );
	}
	else {
		ret = alloc_chrdev_region( &dev, DEVICE_FIRST, DEVICE_COUNT, MOD_NAME );
		major = MAJOR( dev );  // не забыть зафиксировать!
	}
	if( ret < 0 ) {
		printk( KERN_ERR "mpu6050: can not register char device region\n" );
		goto err;
	}
	
	printk( KERN_INFO "mpu6050: context = 0x%X\n", (unsigned int)context );
	rt_mutex_init(&context->buf_mutex);
	rt_mutex_init(&context->dev_current_mutex);
	rt_mutex_init(&context->dev_all_mutex);
	cdev_init(&context->cdev,&dev_fops);
	context->cdev.owner = THIS_MODULE;
	context->cdev.ops = &dev_fops;
	ret = cdev_add( &context->cdev, dev, DEVICE_COUNT );
	if( ret < 0 ) {
		unregister_chrdev_region( MKDEV( major, DEVICE_FIRST ), DEVICE_COUNT );
		printk( KERN_ERR "mpu6050: can not add char device\n" );
		goto err;
	}
	context->devclass = class_create( THIS_MODULE, "mpu6050" );
	for (i = 0; i < DEVICE_COUNT; i++) {
	#define DEV_NAME "mpu6050"
		dev = MKDEV( major, DEVICE_FIRST + i);		
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26) 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, const char *fmt, ...); */ 
      		device_create( context->devclass, NULL, dev, "%s_%s", DEV_NAME, DEVICE_FILE_SUFFIX[i] );
	#else           // прототип device_create() изменился! 
	/* struct device *device_create( struct class *cls, struct device *parent, 
                                 dev_t devt, void *drvdata, const char *fmt, ...); */ 
		device_create( context->devclass, NULL, dev, NULL, "%s_%s", DEV_NAME, DEVICE_FILE_SUFFIX[i] ); 
	#endif
	}
	printk( KERN_INFO "mpu6050:======= module installed %d:%d ==========\n",
	MAJOR( dev ), MINOR( dev ) );

	context->buf_data = mpu6050_buf_init();
	if (!(context->buf_data)) {
		unregister_chrdev_region( MKDEV( major, DEVICE_FIRST ), DEVICE_COUNT );
		printk( KERN_ERR "mpu6050: can not allocate buffer for mpu6050 data\n" );
		goto err;	
	} 	
	context->read_task = kthread_run( mpu6050_read_thread, (void*)NULL, MPU6050_THREAD_NAME );
err:
	return ret;
}

static void mpu6050_dev_deinit(void) {
	dev_t dev;
	int i;
	mpu6050_buf_deinit(context->buf_data);
	kthread_stop(context->read_task);
	i2c_del_driver(&mpu6050_i2c_driver);

	for (i = 0; i < DEVICE_COUNT; i++) {
		dev = MKDEV( major, DEVICE_FIRST + i);		
		device_destroy( context->devclass, dev );
	}
	class_destroy( context->devclass );
	cdev_del( &context->cdev );
	unregister_chrdev_region( MKDEV( major, DEVICE_FIRST ), DEVICE_COUNT );
	printk( KERN_INFO "mpu6050:=========== module removed ==============\n" );
}

static int __init dev_init( void ) {
	int ret;
	ret = mpu6050_dev_init();
	return ret;
}

static void __exit dev_exit( void ) {
	mpu6050_dev_deinit();
}

module_init( dev_init );
module_exit( dev_exit );

